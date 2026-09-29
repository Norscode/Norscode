# Klientlogikk i Norscode kompilert til WebAssembly (W0, W1, W2)

Status: milepælane W0, W1 og W2 i WASM-sporet (app-kjensle del 2). Klientkode blir skriven i Norscode og kompilert til ein WasmGC-modul. Nettlesaren startar modulen med ein liten lastar som blir **emittert frå Norscode-data**. Det finst ingen handskriven JavaScript i repoet.

- **W0** gav vegen frå kjelde til nettlesar: heiltal, kontrollflyt, kall og DOM-vertsfunksjonar.
- **W1** gav verdimodellen: tekst, desimaltal, lister og ordbøker, med same semantikk som VM-en. Eit paritetskorpus køyrer kvart program i VM-en og i Chrome og krev identisk utskrift.
- **W2** gav unnatak (`prøv`/`fang`/`endeleg`, `utsett`, typa `fang`, innebygde feil som fangbare unnatak), closures og funksjonsverdiar, strukturmetodar, og ein stakk- og typesjekk i validatoren.

## Bygg og køyr

```sh
./bin/nc compile examples/wasm_teljar/klient.no -o build/wasm/wasm_teljar/klient.ncb.json
NC_WASM_NCB=build/wasm/wasm_teljar/klient.ncb.json NC_WASM_UT=build/wasm/wasm_teljar \
    ./bin/nc run tools/nc_wasm.no
./bin/nc serve examples/wasm_teljar/app.no --port 8080
```

`tools/nc_wasm.no` skriv tre filer til `NC_WASM_UT`, som må liggje under `build/wasm/`:

| Fil | Innhald |
|---|---|
| `app.wasm` | Modulen |
| `nc.js` | Lastaren, berre for innsyn |
| `klient_data.no` | Norscode-modul med bytane som hex, lastaren og versjonane. Serve-appen importerer han, sidan `nc serve` ikkje har disk-kapabilitet. |

Alt er generert og blir aldri committa. Både `build/` og `*.wasm` er git-ignorerte.

Verktøyet validerer modulen med `std/wasm_les.no` før det skriv han, og skriv éi linje:

```
WASM: app.wasm <n> byte, lastar <m> byte, v=<12 hex> importar=<a,b> eksportar=<m,i> atom=<…>
```

Importane og eksportane er dei validatoren las ut av bytane. `atom` er runtime-atoma modulen fekk med.

Lowringa er rask i VM-fastmodus (`NORSCODE_VM_FAST=1`). `liste.no` tek 0,4 s. I standard-VM-en tek same modul om lag 110 s, fordi kvart funksjonskall og kvart oppslag som gjev ei liste eller ordbok kostar millisekund der. Bygg difor med CLI-en i fastmodus. Testane gjer det same.

## Modular

| Fil | Rolle |
|---|---|
| `std/wasm_lowring.no` | NCB → WASM: analyse (stakkhøgder og unnatakstilstand), dispatch-lykkje, unnatakshandterar, closures, seksjonar |
| `std/wasm_atom.no` | Runtime-atom: verdimodellen som WASM-funksjonar, skrivne med ein liten assembler |
| `std/wasm_les.no` | Validator: struktur (W0/W1) og stakk- og typesjekk (W2) |
| `std/wasm_vert.no` | Vertstabell, klient-API og HTML-escape |
| `std/wasm_js.no` | Norscode→JS-emitter for lastaren |
| `tools/nc_wasm.no` | CLI |

Ingen av dei ligg i `nc_main`-lukkinga, så dei krev ingen reseed.

## Verdimodell (WasmGC)

Alle verdiar er `eqref`. GC-typane har faste indeksar:

| Type | WasmGC | Merknad |
|---|---|---|
| `heltall` | `i31ref` i −2³⁰…2³⁰−1, elles `0 $boks struct {i64}` | Kanonisk representasjon, så likskap samanliknar talverdien. Overflyt wrappar som i VM-en. |
| `bool` | `1 $bool struct {i32}` | To singletonar, i global 0 (`usann`) og global 1 (`sann`). |
| `desimaltall` | `2 $flyt struct {f64}` | |
| `tekst` | `3 $tekst array (mut i8)` | UTF-8-bytar som i VM-en: `lengde("blåbær") = 8`. |
| lagring | `4 $arr array (mut eqref)` | |
| `liste` | `5 $liste struct {mut i32 n, mut ref $arr}` | Amortisert vekst: kapasitet ×2 + 4. |
| `ordbok` | `6 $ordbok struct {mut i32 n, mut ref $arr nøklar, mut ref $arr verdiar}` | Innsetjingsordna. Ein overskriven nøkkel held plassen sin. Oppslag er lineære (hashtabell i W10). |
| closure (W2) | `7 $lambda struct {funcref, ref null $arr}` | Berre i modular med closures eller `ncb_call_fn`. Funksjonsreferansen og dei fanga verdiane. |
| `null` | `ref.null eq` | |

Semantikken ligg i **atom**: små WASM-funksjonar i `std/wasm_atom.no`, t.d. `add`, `lik`, `cmp`, `tekst_av`, `indeks_hent`, `ordbok_set`, `kast` og `unntak_passar`.

- Lowringa tek berre med atoma modulen treng, som ei transitiv lukking over `atom_avheng`.
- Atoma ligg rett etter importane i funksjonsindeksane. Då er kvart kall éin LEB-byte.

### Operasjonar

**`a + b`**
- To heiltal gjev heiltal.
- Heiltal og desimaltal i kva kombinasjon som helst gjev desimaltal.
- Elles blir det tekst: `tekst(a) + tekst(b)`. Til dømes gjev `"a" + 1` `"a1"`, og `"x" + [1]` gjev `"x[verdi]"`.

**`-` og `*`**
- Fungerer for tal.
- Andre typar gjev trap.

**`/` og `%`**
- Heiltal delt på heiltal trunkerer.
- `%` på desimaltal er `a − trunc(a/b)·b`.
- Deling på `0` eller `0.0` kastar `DivisjonMedNull: divisjon med null ved <funksjon>`. For `%` blir det `ModuloMedNull: …`. Begge kan fangast (W2).
- `MIN / −1` gjev `MIN`.
- Er divisoren ein heiltalskonstant ≠ 0, manglar feilvegen (`div_k`/`mod_k`). Då treng modulen ingen import og ingen tag.

**`==` og `!=`**
- Tal blir samanlikna som tal: `1 == 1.0`.
- Tekst blir samanlikna byte for byte.
- Lister blir samanlikna strukturelt.
- Ordbøker og closures blir samanlikna på referanse.
- Ulike typar er ulike: `1 != "1"`, `sann != 1`, `null != 0`.

**`<`, `<=`, `>` og `>=`**
- Tal og tal blir samanlikna som tal.
- Tekst og tekst blir samanlikna bytevis, utan forteikn, så `"å" > "z"`.
- `null` tel som 0 mot tal og mot `null`.
- Alle andre kombinasjonar gjev `usann`.

**Sanning** (`hvis`, `ikkje`, `og`, `eller`)
- `usann`, `null`, `0`, `0.0` og `""` er usanne.
- Alt anna er sant, også `[]`, `{}` og closures.

**`tekst(x)`**
- `null` gjev `""`.
- Bool gjev `sann` eller `usann`.
- Liste, ordbok og closure gjev `[verdi]`, som i VM-en.
- Desimaltal blir formaterte som i VM-en (`std/desimaltall_tekst.no`):
  - Kortaste siffer som les attende til same bit.
  - Heiltalsverdi under 2⁵³ utan `.0`.
  - Fast form for 10⁻⁵ < |x| < 10¹⁷.
  - Elles `1e20` eller `1.25e-5`.
  - `inf`, `-inf` og `nan`.
  - Sifra kjem frå verten (`Number.prototype.toExponential`, som gjev dei kortaste). WASM-sida gjer resten av formateringa.

**`type(x)`** gjev `heltall`, `desimaltall`, `tekst`, `boolsk`, `liste`, `ordbok` eller `ingenting`. Ein closure er `ordbok`, som i VM-en (der han er ei ordbok med `__lambda__`).

**Indeksering**
- `liste[i]` utanfor kastar `IndeksFeil: indeks i utanfor lista (lengde n) ved <funksjon>`, både ved lesing og skriving.
- `tekst[i]` gjev éin byte, eller `""` utanfor.
- `ordbok[k]` gjev `null` når nøkkelen manglar.

**`heltall(tekst)`**
- Godtek berre `-` og siffer.
- Overflyt wrappar som i VM-en: `99999999999999999999` gjev `7766279631452241919`.
- Ugyldig tekst kastar `Ugyldig heiltall: <tekst>`.

**`for k i d` over ei ordbok**
- Compileren på denne greina emitterer ikkje `for_iterabel`. Løkka les difor `d[0]`, `d[1]`, … både i VM-en og i WASM, og tekstnøklar gjev `null`.
- Bruk `for k i nøkler(d)`.
- `builtin.for_iterabel` er likevel støtta, med ordbok → nøklar, for når compileren tek han i bruk igjen.

## Unnatak (W2)

Unnatak brukar WASM-unnatak med **`try_table` og ein tag** (exnref-forslaget). Modulen har éin tag, `$nc_unntak`, med nyttelast `eqref`: den kasta Norscode-verdien. Taggen, handterarane og atoma for unnatak kjem berre med i modular som kan kaste.

### Same maskin som VM-en

VM-en (`selfhost/vm.no`) har per ramme ein try-stakk (`TRY_BEGIN`/`TRY_END`), ein opprydjingsstakk (`FINALLY_PUSH` for `endeleg` og `utsett`) og ein «pending»-tilstand (normal, return eller throw) med verdi. Lowringa speglar dette:

- **Analysen** fører try-stakken T, opprydjingsstakken C og mengda P av moglege pending-verdiar som *abstrakt tilstand per basisblokk*. `TRY_BEGIN`, `TRY_END` og `FINALLY_PUSH` avsluttar blokka, så T og C er konstante i kvar blokk. Tilstanden flyt langs kantane:
  - `FINALLY_RUN` går til endeleg-blokka med P = {normal}.
  - `RETURN` med ein aktiv opprydjing går til endeleg-blokka med P = {return}.
  - Eit unnatak i ei blokk kan gå til fang-blokka for kvar handterar over opprydjingsgrensa (til og med den første som fangar alt), eller til endeleg-blokka med P = {throw}.
  - `FINALLY_END` har berre kantar for dei pending-verdiane som er moglege. Difor blir t.d. etter-etiketten til ei `utsett`-blokk (som berre kan nåast med return eller throw) aldri nådd.
- **Køyring:** ein funksjon med `prøv`, `endeleg` eller `utsett` får dispatch-lykkja pakka i `block (result eqref)` + `try_table (catch $nc_unntak 0)`. Når eit unnatak kjem (frå `kast`, frå eit atom eller frå ein funksjon lenger inne), hamnar verdien i ein handterar etter lykkja. Handteraren vel mål etter tilstanden i blokka som køyrde, akkurat som `THROW` i VM-en:
  1. den øvste passande handteraren over grensa til den øvste opprydjinga → fang-blokka;
  2. elles den øvste opprydjinga → endeleg-blokka, med pending = throw og verdien;
  3. elles kast vidare til kallaren (`throw` igjen).
- `LOAD_EXCEPTION`, `LOAD_PENDING` og `CLEAR_PENDING` les og skriv lokale (unnatak, pending-verdi og pending-type). `FINALLY_END` vel etter pending: normal → etter, return → return-etiketten (`returner` den ventande verdien, som kan gå vidare til ei ytre endeleg-blokk), throw → throw-etiketten (kastar på nytt).
- Stakkslotane under ein handterar ligg i lokale og er urørte, så fang-blokka kan halde fram med stakkhøgda frå `TRY_BEGIN`, som i VM-en.

### Typa `fang`

`fang (e: Type)` brukar atomet `unntak_passar`, som speglar `vm_handler_match` og `vm_exception_type`:

| Kasta verdi | Typen han har |
|---|---|
| ordbok med nøkkelen `type` | `tekst(v["type"])` |
| tekst med `:` etter første teikn | prefikset før første `:` (`"IndeksFeil: …"` → `IndeksFeil`) |
| anna tekst | `tekst` |
| andre verdiar | `vm_type_name`: `null`, `heiltall`, `bool`, `desimaltall`, `liste`, `kart` (ordbøker og closures) |

- `fang (e)`, `fang (e: *)` og `fang (e: alle)` fangar alt. Lowringa avgjer det ved kompilering, utan kall.
- `fang (e: tekst)` fangar all tekst, også `"A: b"`.
- Ein handterar som ikkje passar, blir hoppa over, og unnataket går til den neste utanfor.

### Innebygde feil

`DivisjonMedNull`, `ModuloMedNull`, `IndeksFeil` (lesing og skriving), `Ugyldig heiltall` og `Ugyldig venstre-/høgreskift` blir kasta som Norscode-unnatak med same tekst som VM-en, t.d. `DivisjonMedNull: divisjon med null ved __main__.del`. Atomet `kast` er no `local.get 0; throw $nc_unntak` i staden for logg + `unreachable`.

### Uhandterte unnatak

Init (`i`) og innpakkinga av tilbakekall (`k0`, `k1`, …) kallar koden inne i `try_table`. Eit unnatak som ingen fangar, blir skrive som `ERROR: <tekst(v)>`, som VM-en skriv før han avsluttar. Modulen returnerer så normalt, og ingenting når JavaScript. Korpustesten krev at konsollen i Chrome er fri for «Uncaught», og har ein positiv kontroll (`kontroll_trap`) som viser at sjekken ser ein trap.

Typefeil der VM-en ikkje har definert oppførsel (t.d. `liste − tal`), er framleis reine trap-ar. Dei kan ikkje fangast.

### Ikkje støtta (kompileringsfeil)

- **`bryt` eller `fortsett` ut av ei `prøv`-blokk.** VM-en fjernar ikkje handteraren då (han blir liggjande på try-stakken), så T er ulik der vegane møtest.
- **`utsett` inne i ei lykkje.** VM-en legg på ei ny opprydjing kvar runde, så C veks dynamisk.

Begge gjev «ulik unntakstilstand ved blokk …» i rapporten, i staden for oppførsel som er ulik VM-en. `returner` inne i `prøv` er støtta.

### Nettlesarstøtte for unnatak

| Motor | Kva som er brukt | Status |
|---|---|---|
| Chrome 154 (macOS) | `try_table`/`throw` med tag, WasmGC, `call_ref`, deklarativt elementsegment | **[V]** Heile korpuset (18 program) gav identisk utskrift som VM-en, utan «Uncaught» |
| JavaScriptCore i macOS 26.6.2 (= Safari 26.6) | same modular | **[V]** Alle 18 korpusmodulane gav identisk utskrift i `jsc`, utan uhandterte unnatak (skriptet ligg utanfor repoet) |
| Chrome/Edge | exnref (`try_table`) | [A] frå versjon 137. WasmGC og typa funksjonsreferansar frå 119. |
| Firefox | exnref | [A] frå versjon 131. WasmGC frå 120. Firefox var ikkje installert og er ikkje testa. |
| Safari | exnref | [A] frå 18.4. WasmGC frå 18.2. |

- **Val: berre `try_table`/exnref, ikkje legacy-`try`.** Legacy-unnatak (`try`/`catch`/`delegate`) er utfasa i spesifikasjonen. Dei ville berre senke golvet frå exnref-versjonane til WasmGC-versjonane (Chrome 119–136, Firefox 120–130, Safari 18.2–18.3). Validatoren avviser legacy-opkodane.
- Golvet for klientmodular med unnatak er difor Chrome 137, Firefox 131 og Safari 18.4 [A]. Modular utan unnatak har framleis WasmGC-golvet. Eldre nettlesarar får del 1-oppførsel (sida verkar utan klientlogikk).

## Closures og funksjonsverdiar (W2)

Kompilatoren lowrar `fun(x) -> …` til ein hjelpefunksjon `modul.__lambda_N` og `BUILD_LAMBDA "modul.__lambda_N" [fangarar]`. VM-en byggjer då ei ordbok `{__lambda__, __capture__}` med **kopiar** av dei fanga variablane (by-value).

- `BUILD_LAMBDA` blir `ref.func hjelpar; local.get fangar…; array.new_fixed $arr n; struct.new $lambda`.
- Hjelparen får typen `$fnN = (eqref × n, ref null $arr) → eqref`. Første instruksjonane kopierer fangarane frå arrayen til lokale, så `LOAD_NAME` av ein fanga variabel les lokalen. Ei endring av variabelen etter `BUILD_LAMBDA` syner ikkje, som i VM-en.
- `CALL_VALUE n` (kall på ein lokal variabel som held ein closure) blir `ref.cast $lambda; struct.get` (fangarane og funksjonsreferansen), `ref.cast (ref $fnN)` og `call_ref $fnN`.
- Hjelparane står i eit **deklarativt elementsegment**, som `ref.func` krev.
- Delt tilstand går gjennom referansar, som i VM-en. Ein teljar-closure som fangar ei ordbok og endrar ho, tel.

`builtin.ncb_call_fn(x, a1…an)` blir ein **generert dispatcher per aritet**:
1. Er `x` ein closure → `call_ref`.
2. Elles blir `tekst(x)` samanlikna med namna på metodekandidatane (sjå under), og treffet blir kalla direkte.
3. Utan treff blir `Ukjent selfhost-funksjon: <namn>` kasta, som i VM-en.

Grenser:
- Ein closure kan ikkje kalle ein fanga closure med `f(x)`. Kompilatoren emitterer då `CALL builtin.f`, som òg feilar i VM-en. Bruk `builtin.ncb_call_fn(f, x)`.
- Kall med feil tal argument til ein closure gjev trap (`ref.cast` til feil `$fnN`).

## Strukturar og metodar (W2)

Kompilatoren lowrar ikkje strukturar til eigne typar i NCB-en:

- `Punkt()` blir `CALL builtin.Punkt 0`. VM-en gjev ei ordbok `{"__type__": "Punkt"}`. WASM gjer det same, med eventuelle argument ignorerte, som i VM-en.
- Felt er vanlege nøklar: `p.x` er `INDEX_GET "x"`, og `p.x = v` er `INDEX_SET`.
- `p.metode(a)`, der `p` er ein lokal variabel, blir `builtin.ncb_call_fn(modul + "." + p["__type__"] + ".metode", p, a)`. Dispatcharen samanliknar med alle funksjonane i NCB-en med namnet `modul.Type.metode` og rett aritet. Dei blir tekne med i modulen, sidan typen først er kjend ved køyring.
- `type(p)` er `ordbok`, og `nøkler(p)` byrjar med `__type__`, som i VM-en.
- Struktur-literalen `Punkt { x: 1 }` er broten i kompilatoren på denne greina (`LOAD_NAME Punkt`, «Ukjent variabel» i VM-en). Han er difor ikkje med.

## Validator (`std/wasm_les.no`)

**Strukturelt (W0/W1)**
- Magic og versjon.
- Seksjonsrekkjefølgja, med tag (13) og datacount (12) på rett plass.
- LEB-storleikar og teljingar.
- Alle indeksar.
- Array-instruksjonar på array-typar.
- Datacount.

**Stakk- og typesjekk (W2)** etter valideringsalgoritmen i spesifikasjonen:
- Operandstakk med ein «ukjend» botn etter `unreachable`, `br`, `br_table`, `return`, `throw` og `throw_ref`.
- Kontrollrammer med inn- og uttypar. Blokktypar kan vere tomme, ein verditype eller ein typeindeks.
- `if` utan `else` må ha like inn- og uttypar.
- Greinetypar: `loop` tek inntypane, dei andre tek uttypane. `br_table` krev lik aritet og passande typar for alle etikettar.
- Subtyping av referansar:
  - `none ≤ i31/struct/array/typeindeks ≤ eq ≤ any`;
  - `nofunc ≤ $fn ≤ func`;
  - nullbar mot ikkje-nullbar.
- `ref.test` og `ref.cast` må vere i same hierarki som operanden.

Instruksjonane som er dekte:
- Alle numeriske MVP-instruksjonane og minneinstruksjonane.
- `select` (typa og utypa), lokale og globale. `global.set` krev ein foranderleg global.
- `call` og `call_ref`.
- `struct.new/get/set`: `set` krev eit foranderleg felt, og pakka felt krev `get_s`/`get_u`.
- `array.new/new_default/new_fixed/new_data/get/set/len/fill/copy`.
- `ref.null/is_null/eq/as_non_null/func`, `ref.i31` og `i31.get`.
- `throw`, `throw_ref` og `try_table`. Catch-klausulane blir sjekka mot etiketten *utanfor* try_table, og tag-parametrane må passe med etikettypane.

I tillegg:
- Globalinitialiseringar og dataoffset blir typesjekka.
- Elementseksjonen (passive og deklarative funksjonssegment) blir lesen.
- `ref.func` krev at funksjonen er deklarert (i eit elementsegment, ein eksport eller ei globalinitialisering).

**Ikkje støtta, gjev feil:**
- legacy-unnatak;
- `br_on_cast` og `br_on_null`;
- `call_indirect` og `return_call*`;
- tabellar;
- ikkje-nullbare lokale;
- deklarerte supertypar.

Typar blir samanlikna på indeks. Backenden dedupliserer funksjonstypane sine, og GC-typane er ulike.

**Kostnad:** 47 ms for ein modul på 1 KB i fastmodus, og om lag 12 s i standard-VM-en. CLI-en og testane validerer difor i fastmodus.

## Subsett (W2)

**Opkodar**
- `PUSH_CONST`: heltall, desimaltall, tekst, bool og null.
- `LOAD_NAME`, `STORE_NAME`, `LOAD_GLOBAL` og `STORE_GLOBAL`.
- `POP` og `DUP`.
- `BINARY_{ADD,SUB,MUL,DIV,MOD,AND,OR,XOR,LSHIFT,RSHIFT}`.
- `COMPARE_{EQ,NE,LT,LE,GT,GE}`.
- `UNARY_NEG` og `UNARY_NOT`.
- `JUMP`, `JUMP_IF_FALSE` og `LABEL`.
- `BUILD_LIST` og `BUILD_MAP`.
- `INDEX_GET` og `INDEX_SET`.
- `CALL` og `RETURN`.
- W2: `TRY_BEGIN`, `TRY_END`, `THROW`, `LOAD_EXCEPTION`, `FINALLY_PUSH`, `FINALLY_RUN`, `FINALLY_END`, `LOAD_PENDING`, `CLEAR_PENDING`, `BUILD_LAMBDA` og `CALL_VALUE`.

**Builtins** (`builtin_tabell` i `std/wasm_lowring.no`)
- `skriv` og `lengde`.
- `tekst` og `tekst_fra_heltall`.
- `heltall` og `heltall_fra_tekst`.
- `legg_til`, `har_nokkel` (også `finnes_nøkkel`) og `nøkler`.
- `slice`, `char_code` og `chr`.
- `type` og `for_iterabel`.
- W2: `ncb_call_fn` (closure eller metodenamn) og struktur-konstruktørar (`builtin.<Stor forbokstav>`).

Alt anna gjev kompileringsfeil med ei liste over alle funna. Det gjeld:
- `område`, som VM-en heller ikkje har;
- asynkrone funksjonar;
- ukjende builtins;
- hopp ut av `prøv` og `utsett` i lykkjer (sjå over).

Det finst ingen stille fallback.

## Vertsfunksjonar (`bruk std.wasm_vert som vert`)

| Funksjon | DOM-operasjon |
|---|---|
| `vert.finn("#id")` | `document.querySelector` → handtak |
| `vert.sett_tekst(h, tal)` | `textContent` |
| `vert.sett_html(h, "mal med {}", tal)` | `innerHTML`, alltid med escape (sjå under) |
| `vert.lytt_klikk(h, "funksjonsnamn")` | `click`-lyttar som kallar funksjonen i same modul |
| `vert.test_klikk(h)` | `element.click()`, berre med `NC_WASM_TESTBYGG=1` |

**Interne** (W1) blir importerte av atoma. Dei har ingen Norscode-stubb:

| Import | JS | Brukt av |
|---|---|---|
| `logg(s)` | `console.log(S(a,b))` | `skriv(x)` og `ERROR: …` |
| `logg_test(s)` | `document.getElementById("nc-logg").append(S(a,b))` | same, i testbygg. Ein headless nettlesar les utskrifta frå DOM-en. |
| `flyt_tekst(x, p)` | `W(a.toExponential(),b)` | `tekst(desimaltall)`. Skriv sifra i postkassa og gjev lengda. |

W2 har ingen nye importar. Unnatak er heilt inne i modulen, og taggen blir ikkje importert eller eksportert. Lastaren er difor uendra.

Tabellen i `vertsfunksjonar()` skildrar kvar funksjon med namn, parametertypar, resultat og operasjonen som eit JS-uttrykkstre.

- Parametertypane i W1 er `tekst`, `funksjon`, `handtak`, `heltall`, `verdi`, `desimal` og `postkasse`.
- Resultattypane er `handtak`, `ingen` og `lengd`.

`std/wasm_js.no` byggjer lastaren token for token frå tabellen. Berre importerte funksjonar og hjelparane dei brukar kjem med:

| Hjelpar | Rolle |
|---|---|
| `E` | Handtak |
| `S` | UTF-8 frå minnet |
| `H` | Escape |
| `W` | UTF-8 inn i minnet (`TextEncoder.encodeInto`) |

**Minne «m»**
- Det aktive datasegmentet ligg først, med tekstkonstantane til vertsfunksjonane (selektorar og malar).
- Så kjem **postkassa**, 8-justert. Der ligg tekst over JS-grensa i begge retningar, og ho veks med `memory.grow` ved behov.
- Tekstliteralar som er verdiar, ligg i eit **passivt** segment 0 og blir laga med `array.new_data`. Modulen har då ein datacount-seksjon.

**Lastarstorleik**, testa i `tests/test_wasm_lastar.no`:

| Lastar | Storleik | Tak |
|---|---|---|
| Teljar | 529 byte | 600 byte |
| Eitt import | 174 byte | 400 byte |
| Heile tabellen | 774 byte | 900 byte (planbudsjettet er 2,5 KB) |
| Typisk korpusprogram i testbygg (`logg_test`) | 265–281 byte | |
| Med `flyt_tekst` òg | 389 byte | |

## HTML-innsetjing og escaping

`sett_html` er den einaste vegen til `innerHTML`.

- **Malen** må vere ein tekstkonstant i kjelda, det vil seie HTML som utviklaren har skrive. Kompilatoren avviser alt anna.
- **Verdien** blir alltid sendt gjennom escape-hjelparen `H` i lastaren før han blir sett inn i staden for `{}`.
  - `H` blir emittert frå `std.wasm_vert.escape_tabell()`: `&`, `<`, `>`, `"` og `'`.
  - Det er same tabell som `std.html.escape` brukar.
  - `wasm_vert.html_escape` gir identisk tekst i VM-en. Testen påstår likskapen.

Brukardata kan dermed ikkje nå `innerHTML` uescapa. Enno tek `sett_tekst` og `sett_html` berre heiltal. Når dei får tekstverdiar (W4), skal dei gå same vegen. Parametertypen `verdi` finst alt, med postkasse og `S` i lastaren.

## CSP, versjonering og cache

- Sider som lastar WASM, treng `script-src 'self' 'wasm-unsafe-eval'`.
  - I demoen er det berre `/` som får han. `/om` og `/kontroll/streng-csp` har standard-CSP.
  - `'wasm-unsafe-eval'` opnar ikkje for `eval` eller `new Function`.
- `v` for modulen er dei 12 første hex-teikna av `sha256_bytes` over bytane. `sha256(tekst(liste))` kolliderer og blir ikkje brukt.
- `v` for lastaren er sha256 over lastarteksten.
- Rett `v` gir `Cache-Control: public, max-age=31536000, immutable`. Feil eller manglande `v` gir `no-cache`.
- `std/http_cache.no` (PR #206) finst ikkje på denne greina, så headerane blir sette direkte i `examples/wasm_teljar/app.no`.

## Der VM-ane er usamde

VM-en er ikkje eintydig for alle kombinasjonar. Det blei målt med `nc run` på macOS-arm64-seeden (`dist/`) og Linux-x86-64-stage0:

| Uttrykk | macOS-arm64 | Linux-x86-64 | WASM |
|---|---|---|---|
| `1 + "x"` | peikarsum (søppel) | `"1x"` | `"1x"` |
| `sann + 1` | `2` | `"sann1"` | `"sann1"` |
| `null + 1` | `1` | `"1"` | `"1"` |
| `[1] + [2]` | peikarsum | `"[verdi][verdi]"` | `"[verdi][verdi]"` |
| `ikkje ""` | `usann` | `sann` | `sann` |
| `ikkje 0.0` | `sann` | `usann` | `sann` |
| `[1, 2] == [1, 2.0]` | `usann` | `sann` | `sann` |
| `d[1]` og `d["1"]` | same nøkkel | ulike nøklar | ulike nøklar |
| `"abc"[99]` | éin byte | `""` | `""` |

Paritetskorpuset testar berre kombinasjonar der begge VM-ane er samde.

- For `+` følgjer WASM den tekstbaserte regelen. Der er den eine VM-en udefinert.
- For sanning følgjer WASM den tilsikta regelen: `""` og `0.0` er usanne.

W2-korpuset (9 program) gav same utskrift og exit-kode på begge VM-ane. Det gjeld òg typa fang av ikkje-tekst (`unntak_typar`):
- `kast 42` blir fanga av `fang (e: heiltall)`, men ikkje av `fang (e: heltall)`, på begge VM-ane. Typenamnet kjem frå `vm_type_name` (`heiltall`, `bool`, `kart`, `desimaltall`, `liste`, `null`), og WASM følgjer det.
- Uhandtert unnatak med ei ordbok gjev `ERROR: [verdi]` på begge.

## Storleikar

Modulane er like store på macOS og Linux. Tala er i byte, testbygg (med `logg_test`).

| Modul | W2 | W1 |
|---|---|---|
| `minimal` | 686 | 686 |
| `fak` | 3191 | 3156 |
| `tekst_utf8` | 4405 | 4370 |
| `flyt` | 5262 | 5227 |
| `liste` | 5786 | 5751 |
| `ordbok_orden` | 4485 | 4450 |
| `blanda` | 4779 | 4744 |
| `feil_div` | 1180 | 1144 |
| `unntak_grunn` | 5155 | |
| `unntak_typar` | 4369 | |
| `unntak_endeleg` | 3982 | |
| `unntak_utsett` | 3124 | |
| `unntak_innebygd` | 4127 | |
| `feil_uhandtert` | 1089 | |
| `closure_fangst` | 4236 | |
| `hogare_orden` | 4681 | |
| `struct_metode` | 4511 | |

- W1-programma som kan kaste (divisjon, indeksering, `heltall`), er 35–36 byte større. Dei har fått taggen, `uhandtert` og `try_table` i init. `kast` har mista loggkallet.
- Program utan feilveg er byte-identiske med W1 (`minimal` er framleis byte-eksakt mot fasiten).

## Testar

| Test | Kva han dekkjer |
|---|---|
| `tests/test_wasm_w0.no` | Validatoren godtek fasitmodular og avviser éin feil om gongen. Korpuset blir bygd med CLI-en, med dei venta importane og eksportane. `minimal.no` er byte-eksakt både i testprosessen og frå CLI-en. Rapporten er fail-closed: åtte feil i `feil_ustotta.no`, mellom anna `bryt` ut av `prøv` og `utsett` i ei lykkje (W2), og `ustøtta builtin: ukjend`. Versjonen følgjer bytane. |
| `tests/test_wasm_w1.no` | GC-fasitmodulen frå proben blir bygd på nytt med assembleren og er byte-eksakt. Kodingane til GC-typane og instruksjonane er rette. W1-validatoren (datacount, array-typar) avviser éin feil om gongen. Tree-shakinga er rett. |
| `tests/test_wasm_les.no` | W2-typesjekken: ein handbygd modul (kontrollflyt, i64, GC, `try_table`/`throw`, `ref.func`, `call_ref`, globalar) blir godteken. 17 variantar med éin feil kvar blir avviste med rett melding. Ekte korpusmodular blir godtekne, og avviste etter mutasjon: utan elementsegmentet, og med feil tag-type. |
| `tests/test_wasm_w2.no` | W2-lowringa på bytenivå: ingen tag- eller elementseksjon utan unnatak og closures. Init i `try_table` og `throw` for `THROW`. `unntak_passar` berre ved typa fang. `kast` er `throw`, ikkje trap. Deklarativt elementsegment og `call_ref` for closures. `$lambda`-typen for `ncb_call_fn`. |
| `tests/test_wasm_korpus_chrome.no` | Paritetskorpuset (W1 + W2), sjå under. |
| `tests/test_wasm_lastar.no` | Byte-tak og tree-shaking. Importnamna i lastaren er lik importseksjonen. Strukturen kjem frå tabellen. Ingen JS-fragment i literalane. Kvart lastartoken finst i emitteren eller i tabellen (proveniens). Escape er lik `std.html.escape`. |
| `tests/test_wasm_serve.no` | Barne-`nc serve` leverer `application/wasm` byte-identisk, med versjonert cache og CSP per side. |
| `tests/test_wasm_w0_chrome.no` | Valfri. Teljaren etter tre klikk, negativ CSP-kontroll og W0-korpuset mot VM-en. |

Chrome-hjelparane ligg i `tests/fixtures/wasm_chrome_hjelp.no`. Der les `dump_med_konsoll` DOM-en og Chrome-konsollen (stderr med `--enable-logging`). Korpushjelparane ligg i `tests/fixtures/wasm_korpus_hjelp.no`, modulinnsyn (seksjonar, kroppar) i `tests/fixtures/wasm_test_hjelp.no`, og `tests/fixtures/wasm_valider_fil.no` validerer ei fil i ein barneprosess.

### Paritetskorpuset

`tests/wasm_korpus/` har desse programma:

| Program | Dekkjer |
|---|---|
| `fak` | i64-boks, overflyt, divisjon |
| `tekst_utf8` | bytar, utsnitt, samanlikning, `heltall(tekst)` |
| `flyt` | formatering og blanda aritmetikk |
| `liste` | vekst, likskap, for, argument og retur |
| `ordbok_orden` | rekkjefølgje, `nøkler`, manglande nøkkel, `for` |
| `blanda` | `+` med tekst, samanlikningar, sanning, `type` |
| `feil_div`, `feil_indeks`, `feil_heltall` | `ERROR: …` |
| `unntak_grunn` (W2) | kast og fang, typa fang, feil type til ytre handterar, gjennom seks rammer, omkast, ordbok med `type`, `fang (e: tekst)`, prøv i lykkje |
| `unntak_typar` (W2) | typa fang av liste, bool, kart (ordbok og closure), desimaltall, heiltall og null; heiltall-handterar slepp tekst og lister vidare |
| `unntak_endeleg` (W2) | endeleg ved returner i prøv og fang, nøsta endeleg, unnatak gjennom endeleg i to rammer |
| `unntak_utsett` (W2) | utsett i omvend rekkjefølgje, ved returner og ved unnatak gjennom rammer |
| `unntak_innebygd` (W2) | DivisjonMedNull, ModuloMedNull, IndeksFeil (lesing og skriving), Ugyldig heiltall fanga, typa handterar som ikkje passar |
| `feil_uhandtert` (W2) | endeleg og utsett køyrer, så `ERROR: …` utan «Uncaught» |
| `closure_fangst` (W2) | by-value, teljar-closure, closure frå funksjon, komposisjon via `ncb_call_fn`, closures i liste, fleire fangarar, nøsta |
| `hogare_orden` (W2) | kart, filter, fold og sortering med closure, skrivne i Norscode |
| `struct_metode` (W2) | konstruktør, felt, metodar som endrar self, dispatch på to typar, nøkkelrekkjefølgje |
| `kontroll_trap` | ikkje paritet: positiv kontroll for konsollsjekken |

Kvart program har ein **kjend fasit** i `#=`-linjer, og `test_wasm_korpus_chrome` køyrer dei i tre steg:

1. Køyrer kvart program i VM-en (barne-`nc run`) og krev at stdout er lik fasiten.
2. Byggjer kvart program i testbygg med CLI-en. Modulen blir validert, med typesjekk, og må importere `logg_test`. W2-programma må ha `uhandtert`.
3. Startar éin barne-`nc serve` med ei side per program. Headless Chrome køyrer sidene. Teksten i `<pre id="nc-logg">` må vere identisk med VM-utskrifta, og konsollen må vere fri for «Uncaught». Utan Chrome blir dette steget SKIP. `NC_CHROME` vel binær.

**Resultat:** alle 18 programma gav identisk utskrift i VM-en og i Chrome 154 (macOS), utan «Uncaught». Dei same 18 modulane gav identisk utskrift i JavaScriptCore frå macOS 26.6.2 (Safari 26.6-motoren), køyrt med eit scratch-skript utanfor repoet. Linux i Docker har ikkje Chrome, så der køyrer steg 1 og 2.

Testane er prøvde med mellombelse mutasjonar, og kvar av desse gjorde testen raud:

| Mutasjon | Test |
|---|---|
| Feil fasitlinje | VM-steget |
| `tekst(liste)` → `[verd]` | Chrome-steget |
| Eksponentgrensa 17 → 16 | Chrome-steget |
| Ordbok som legg til ein duplikatnøkkel | Chrome-steget |
| `array.new_data`-opkoden | `test_wasm_w1` |
| Datacount-sjekken fjerna | `test_wasm_w1` |
| `mod_k`-deteksjonen slått av | `test_wasm_w1` |
| W2: `returner` i endeleg mistar verdien | Chrome-steget (`unntak_endeleg`) |
| W2: `unntak_passar` invertert | Chrome-steget (`unntak_grunn`) |
| W2: fangarane i omvend rekkjefølgje | Chrome-steget (`closure_fangst`) |
| W2: subtypesjekken i validatoren slått av | `test_wasm_les` |
| W2: høgdesjekken ved `end` slått av | `test_wasm_les` |
| W2: `ref.func`-deklarasjonssjekken slått av | `test_wasm_les` |
| W2: lowringa utan elementsegment | `test_wasm_les` (CLI-en avviser modulen) |
| W2: `kast` som trap (W1-oppførsel) | `test_wasm_w2` |
| W2: taggen alltid med | `test_wasm_w2` |
| W2: `unntak_passar` også for jokertype | `test_wasm_w2` |
| W2: passivt i staden for deklarativt elementsegment | `test_wasm_w2` |

### Tid

Tida er målt i sekund med `./bin/nc test` (standard) og `NC_TEST_VM_FAST=1 ./bin/nc test` (fast). Tala i parentes er W1.

| Test | macOS standard | macOS fast | Linux standard | Linux fast |
|---|---|---|---|---|
| `test_wasm_w0` | 81 (27) | 5 (4) | 13 | 13 |
| `test_wasm_w1` | 10 (10) | 5 (3) | 10 | 12 |
| `test_wasm_w2` | 13 | 5 | 11 | 10 |
| `test_wasm_les` | 53 | 5 | 13 | 12 |
| `test_wasm_lastar` | 24 (24) | 3 (3) | 10 | 13 |
| `test_wasm_serve` | 6 | 3 | 5 | 6 |
| `test_wasm_korpus_chrome` | 53 (43) | 44 (16) | 35 | 39 |
| `test_wasm_w0_chrome` | 11 | 8 | 2 | 5 |

- `test_wasm_w0` i standard-VM-en er tregare fordi validatoren, med typesjekken, køyrer i testprosessen på fasitmodulane og `minimal`. Typesjekken kostar om lag 12 s per KB der.
- Korpustesten har 18 program i staden for 9, og les konsollen i Chrome.
- Linux (Docker, `nc-x86tools`, stage0 frå `bootstrap/`) har ikkje Chrome, så Chrome-stega er SKIP der. `test_wasm`, `test_wasm_binary` og dei andre W0/W1-testane er òg grøne på begge plattformene og i begge modusane.

## Gjenstår før W3

- **W3 og seinare:**
  - tekstverdiar til `sett_tekst`/`sett_html`;
  - `split`, `join` og `json` i `std/wasm_rt.no`. `json_parse` kan no kaste med W2-unnatak.
- **Unnatak:**
  - `bryt`/`fortsett` ut av `prøv` og `utsett` i lykkjer er avviste. Dei krev dynamisk try- og opprydjingsstakk, eller at kompilatoren emitterer `TRY_END` før hoppet (reseed).
- **Closures:**
  - Kall av ein fanga closure som `f(x)` inne i ein lambda krev ei endring i kompilatoren (reseed). Til då går det med `ncb_call_fn`.
  - Funksjonsnamn som verdi (`kart(l, dobbel)`) blir `LOAD_NAME` av eit ukjent namn, også i VM-en.
- **Ytelse (W10):**
  - Handteraren sjekkar typar med tekstsamanlikning per handterar.
  - `ncb_call_fn` samanliknar metodenamn lineært.
  - Metodekall byggjer namnet med to tekstkonkateneringar. Ein statisk tabell per `__type__` ville vore raskare.
- **Validator:** subtyping mellom ulike typeindeksar (deklarerte supertypar, strukturell likskap mellom rec-grupper) og legacy-unnatak er ikkje støtta.
- **Nettlesarar:** Firefox er ikkje testa (ikkje installert). Golvet for exnref er [A].
