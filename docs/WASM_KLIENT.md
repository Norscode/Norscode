# Klientlogikk i Norscode kompilert til WebAssembly (W0–W3)

Status: milepælane W0, W1, W2 og W3 i WASM-sporet (app-kjensle del 2). Klientkode blir skriven i Norscode og kompilert til ein WasmGC-modul. Nettlesaren startar modulen med ein liten lastar som blir **emittert frå Norscode-data**. Det finst ingen handskriven JavaScript i repoet.

- **W0** gav vegen frå kjelde til nettlesar: heiltal, kontrollflyt, kall og DOM-vertsfunksjonar.
- **W1** gav verdimodellen: tekst, desimaltal, lister og ordbøker, med same semantikk som VM-en. Eit paritetskorpus køyrer kvart program i VM-en og i Chrome og krev identisk utskrift.
- **W2** gav unnatak (`prøv`/`fang`/`endeleg`, `utsett`, typa `fang`, innebygde feil som fangbare unnatak), closures og funksjonsverdiar, strukturmetodar, og ein stakk- og typesjekk i validatoren.
- **W3** gav runtime-biblioteket `std/wasm_rt.no`: tekstfunksjonar, JSON, sortering og SHA-256 skrivne i vanleg Norscode og kompilerte av same backend, med tree-shaking. Vertsfunksjonane tek tekstverdiar.

## Bygg og køyr

```sh
./bin/nc compile examples/wasm_teljar/klient.no -o build/wasm/wasm_teljar/klient.ncb.json
NC_WASM_NCB=build/wasm/wasm_teljar/klient.ncb.json NC_WASM_UT=build/wasm/wasm_teljar \
    ./bin/nc run tools/nc_wasm.no
./bin/nc serve examples/wasm_teljar/app.no --port 8080
```

`tools/nc_wasm.no` kompilerer først runtime-biblioteket (sjå «Runtime-bibliotek») og lenkar det inn i NCB-en. Så skriv han tre filer til `NC_WASM_UT`, som må liggje under `build/wasm/`:

| Fil | Innhald |
|---|---|
| `app.wasm` | Modulen |
| `nc.js` | Lastaren, berre for innsyn |
| `klient_data.no` | Norscode-modul med bytane som hex, lastaren og versjonane. Serve-appen importerer han, sidan `nc serve` ikkje har disk-kapabilitet. |

Alt er generert og blir aldri committa. Både `build/` og `*.wasm` er git-ignorerte.

Verktøyet validerer modulen med `std/wasm_les.no` før det skriv han, og skriv éi linje:

```
WASM: app.wasm <n> byte, lastar <m> byte, v=<12 hex> importar=<a,b> eksportar=<m,i> atom=<…> rt=<…>
```

Importane og eksportane er dei validatoren las ut av bytane. `atom` er runtime-atoma modulen fekk med, og `rt` er funksjonane frå `std/wasm_rt.no` (utan modulprefikset).

Lowringa er rask i VM-fastmodus (`NORSCODE_VM_FAST=1`). `liste.no` tek 0,4 s. I standard-VM-en tek same modul om lag 110 s, fordi kvart funksjonskall og kvart oppslag som gjev ei liste eller ordbok kostar millisekund der. Bygg difor med CLI-en i fastmodus. Testane gjer det same.

## Modular

| Fil | Rolle |
|---|---|
| `std/wasm_lowring.no` | NCB → WASM: analyse (stakkhøgder og unnatakstilstand), dispatch-lykkje, unnatakshandterar, closures, seksjonar |
| `std/wasm_atom.no` | Runtime-atom: verdimodellen som WASM-funksjonar, skrivne med ein liten assembler |
| `std/wasm_les.no` | Validator: struktur (W0/W1) og stakk- og typesjekk (W2) |
| `std/wasm_rt.no` | Runtime-bibliotek i vanleg Norscode (W3) |
| `std/wasm_vert.no` | Vertstabell, klient-API og HTML-escape |
| `std/wasm_js.no` | Norscode→JS-emitter for lastaren |
| `tools/nc_wasm.no` | CLI |

Ingen av dei ligg i `nc_main`-lukkinga, så dei krev ingen reseed. `std/wasm_rt.no` importerer `std/sha256.no` (som ligg i lukkinga), men endrar han ikkje.

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
| JavaScriptCore i macOS 26.6.2 (= Safari 26.6) | same modular | **[V]** Alle 18 korpusmodulane (W3: 23) gav identisk utskrift i `jsc`, utan uhandterte unnatak (skriptet ligg utanfor repoet) |
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

## Runtime-bibliotek (W3)

`std/wasm_rt.no` er vanleg Norscode. Backenden kompilerer det med same lowring som klientkoden, så det finst ingen handskriven WASM for desse funksjonane. Biblioteket brukar berre builtins som er atom (`lengde`, `slice`, `char_code`, `chr`, `legg_til`, `tekst`, `heltall`, `desimaltall`, `type`, `nøkler`), så det kan ikkje kalle seg sjølv via tabellen.

### Lenking og tree-shaking

1. `tools/nc_wasm.no` skriv ein drivar (`bruk std.wasm_rt som rt`) til `NC_WASM_UT/rt/` og kompilerer han med `nc compile` i ein barneprosess (om lag 45 ms).
2. `lowring.lenk_rt(ncb, rt_ncb)` fører funksjonane, globalnamna og modulinitialiseringane til modulane i rt-NCB-en (utanom `__main__`) inn i klient-NCB-en. Det klienten alt har, blir ikkje overskrive.
3. Lowringa byter `CALL builtin.X` ut med eit kall til `std.wasm_rt.<namn>` etter `rt_tabell()`. Funksjonen blir nåbar, og berre nåbare funksjonar kjem med i modulen. Eit program som berre brukar `split`, får berre `splitt` (2 792 byte). `sha256` dreg med seg `std/sha256.no`, men berre funksjonane `hash` når.
4. Aritet blir sjekka mot rt-funksjonen. Utan lenking (t.d. `lowring.kompiler` direkte) er eit rt-builtin ein kompileringsfeil som peikar på biblioteket.

Program som ikkje brukar biblioteket, er byte-identiske med W2 (`minimal` er framleis byte-eksakt mot fasiten, og heile W1/W2-korpuset har uendra storleik).

### Builtins som blir kopla til biblioteket

| Builtin (og alias) | `std.wasm_rt` | Semantikk (som VM-en) |
|---|---|---|
| `split`, `tekst_splitt` | `splitt` | Per byte med tom skiljar. `split("", ",")` gjev `[""]`, tomme felt blir behaldne, treff frå venstre utan overlapp. |
| `join`, `tekst_join` | `saman` | `tekst(x)` per element. Parvis samansetjing, så kostnaden er O(n log n) og ikkje O(n²). |
| `trim`, `tekst_trim` | `trim_tekst` | Fjernar mellomrom, `\t`, `\n`, `\v`, `\f` og `\r` i begge endar (ikkje NBSP). |
| `replace`, `tekst_erstatt` | `erstatt` | Tom `gammal` gjev teksten uendra. |
| `starts_with`, `tekst_starter_med` | `starter_med` | |
| `ends_with`, `tekst_slutter_med` | `slutter_med` | |
| `contains`, `tekst_inneholder` | `inneheld` | `contains("", "")` er `sann`. |
| `index_of`, `tekst_indeks` | `finn_indeks` | Byteindeks, `-1` utan treff, `0` for tom nål. |
| `lower`, `tekst_sma`, `tekst_til_liten` | `til_små` | Berre ASCII A–Z. |
| `upper`, `tekst_store`, `tekst_til_store` | `til_store` | Berre ASCII a–z. |
| `json_parse_raw` | `json_parse` | Typa verdiar, sjå under. |
| `json_stringify` | `json_skriv` | `selfhost/json.no` sin `json_skriv`. |
| `sha256` | `sha256_hex` | `std/sha256.no` (`hash`), hex over UTF-8-bytane. |
| `verdier` | `verdiar` | Verdiane i nøkkelrekkjefølgja. |

Nytt atom: `desimaltall` (`desimal_av`). Desimaltal blir uendra, og heiltal blir konverterte. Tekst blir parsa av verten med den interne vertsfunksjonen `tekst_flyt` (`parseFloat`, korrekt avrunda som `strtod`). NaN blir `0.0`, som VM-en gjev for ugyldig tekst.

**Store og små bokstavar.** Begge VM-ane endrar berre ASCII. `tekst_til_liten("ÆØÅ")` er `ÆØÅ` og `tekst_til_store("æøå")` er `æøå`, både i VM-en og i WASM. Planen nemnde æøå. Paritet med VM-en vann, og korpuset testar at æøå, `ü`, `é` og `ß` står urørte.

### JSON

**`json_stringify`** er ein port av `json_skriv` i `selfhost/json.no`:
- Kompakt, med nøklar i innsetjingsrekkjefølgje.
- Berre `"`, `\`, `\n`, `\r` og `\t` blir escapa. Andre kontrollteikn og all UTF-8 går uendra.
- Heiltalsverdige desimaltal får `.0` (`2.0`). `inf` og `nan` blir `null`.

**`json_parse_raw`** (`rt.json_parse`) gjev same verdi som VM-en for gyldig JSON:
- Objekt blir ordbøker. Ein duplikatnøkkel overskriv verdien og held plassen.
- Tal utan brøk og eksponent blir `heltall`, med wrap ved overflyt som i VM-en (`12345678901234567890` → `-6101065172474983726`). Elles blir dei `desimaltall`.
- `\uXXXX` og surrogatpar blir UTF-8. Einslege surrogatar blir tre bytar, som i VM-en.
- Rå kontrollteikn i strengar blir godtekne, sidan `json_stringify` ikkje escapar dei og rundturen må gå.

**Ugyldig JSON kastar** eit fangbart unnatak. Tekstane følgjer stilen i `selfhost/json.no`:

| Feil | Tekst |
|---|---|
| Uventa teikn (også tekst etter verdien, `[1,]`, `01`) | `JSON: uventa teikn: X` (heile UTF-8-teiknet) |
| Slutt på input | `JSON: uventa slutt` |
| Streng utan avsluttande `"` | `JSON: uavslutta streng` |
| Ukjend escape | `JSON: ugyldig escape: \x` |
| Ugyldig hex i `\u` | `JSON: ugyldig \u-escape` |

Alle byrjar med `JSON:`, så `fang (e: JSON)` fangar dei. VM-en sin `json_parse_raw` kastar ikkje, men gjev ein delvis verdi (`"[1,"` → `[1, 0]`). Det er udefinert oppførsel i VM-en, og WASM er strengare. Program som skal oppføre seg likt, kan kalle `rt.json_parse` direkte (sjå `rt_json_feil` i korpuset).

**VM-en sin `json_parse`** (utan `_raw`) gjev tekstverdiar (`[1,2]` → `{"0": "1", "1": "2"}`). Han blir avvist som `ustøtta builtin: json_parse`, med eit hint om `json_parse_raw`.

### Funksjonar utan builtin

Klientkoden skriv `bruk std.wasm_rt som rt`. Same kode køyrer då i VM-en (som Norscode) og i nettlesaren.

| Funksjon | |
|---|---|
| `rt.sorter(l, samanlikn)` | Stabil flettesortering nedanfrå og opp, som gjev ei ny liste. `samanlikn(a, b) < 0` set `a` før `b`. Komparatoren er ein closure og kan fange variablar. |
| `rt.sortert(l)` | Stabil sortering med `<` (tal mot tal, tekst bytevis). |
| `rt.tekst_til_liste(s)` | UTF-8-teikna som ei liste av tekstar (`"a😀☃b"` → 4 element). VM-en har ingen slik builtin, så `builtin.tekst_til_liste` gjev kompileringsfeil med hint. |

### Storleik

Tala gjeld ein tom app som brukar heile biblioteket (`tests/fixtures/wasm_w3_alle.no`): 20 198 byte ukomprimert, med alle 17 offentlege rt-funksjonane og hjelparane deira. Planbudsjettet er 60 KB. `test_wasm_w3` har taket 24 KB.

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

**Kostnad:** 47 ms for ein modul på 1 KB i fastmodus, og om lag 12 s i standard-VM-en. CLI-en og testane validerer difor i fastmodus, i barneprosessar (W3: også dei handbygde modulane i `test_wasm_les` og korpusmodulane i `test_wasm_w0`). For `rt_json` (17 KB) brukar CLI-en 1,9 s på lowringa og 1,2 s på valideringa i fastmodus.

## Subsett (W2, W3)

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
- W3: `desimaltall`, og builtinane i `rt_tabell()` (sjå «Runtime-bibliotek»).

Alt anna gjev kompileringsfeil med ei liste over alle funna. Det gjeld:
- `område`, som VM-en heller ikkje har;
- VM-en sin `json_parse` (tekstverdiar) og builtins som ikkje finst i VM-en (t.d. `tekst_til_liste`). Rapporten har eit hint om den støtta vegen;
- asynkrone funksjonar;
- ukjende builtins;
- hopp ut av `prøv` og `utsett` i lykkjer (sjå over).

Det finst ingen stille fallback.

## Vertsfunksjonar (`bruk std.wasm_vert som vert`)

| Funksjon | DOM-operasjon |
|---|---|
| `vert.finn(selektor)` | `document.querySelector` → handtak. Selektoren kan vere rekna ut (W3). |
| `vert.sett_tekst(h, verdi)` | `textContent = tekst(verdi)`. Verdien kan vere tekst, tal eller kva som helst (W3). |
| `vert.sett_html(h, "mal med {}", verdi)` | `innerHTML`: malen er ein konstant, og `tekst(verdi)` blir alltid escapa (sjå under) |
| `vert.lytt_klikk(h, "funksjonsnamn")` | `click`-lyttar som kallar funksjonen i same modul |
| `vert.test_klikk(h)` | `element.click()`, berre med `NC_WASM_TESTBYGG=1` |

**Interne** (W1) blir importerte av atoma. Dei har ingen Norscode-stubb:

| Import | JS | Brukt av |
|---|---|---|
| `logg(s)` | `console.log(S(a,b))` | `skriv(x)` og `ERROR: …` |
| `logg_test(s)` | `document.getElementById("nc-logg").append(S(a,b))` | same, i testbygg. Ein headless nettlesar les utskrifta frå DOM-en. |
| `flyt_tekst(x, p)` | `W(a.toExponential(),b)` | `tekst(desimaltall)`. Skriv sifra i postkassa og gjev lengda. |
| `tekst_flyt(s)` (W3) | `parseFloat(S(a,b))` | `desimaltall(tekst)`, og dermed tal med brøk eller eksponent i `json_parse_raw`. Resultattypen er `desimal` (f64). |

W2 har ingen nye importar. Unnatak er heilt inne i modulen, og taggen blir ikkje importert eller eksportert. Lastaren er difor uendra.

Tabellen i `vertsfunksjonar()` skildrar kvar funksjon med namn, parametertypar, resultat og operasjonen som eit JS-uttrykkstre.

- Parametertypane i W1 er `tekst`, `funksjon`, `handtak`, `heltall`, `verdi`, `desimal` og `postkasse`.
- Resultattypane er `handtak`, `ingen`, `lengd` og (W3, berre interne) `desimal`.
- W3: `finn`, `sett_tekst` og verdien til `sett_html` har typen `verdi`. Innpakkaren gjer `tekst(x)` og skriv UTF-8-bytane i postkassa, og lastaren les dei med `S`. Ein vertsfunksjon har høgst éin `verdi`-parameter, sidan alle deler postkassa. `tekst` (berre konstantar) er framleis typen til HTML-malen og funksjonsnamnet til `lytt_klikk`.

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
| Teljar | 543 byte (W2: 529) | 600 byte |
| Eitt import (`sett_tekst`) | 247 byte (W2: 174) | 400 byte |
| Heile tabellen | 825 byte (W2: 774) | 900 byte (planbudsjettet er 2,5 KB) |
| Typisk korpusprogram i testbygg (`logg_test`) | 265–281 byte | |
| Med `flyt_tekst` òg | 389 byte | |
| W3: JSON med desimaltal (`logg_test`, `flyt_tekst`, `tekst_flyt`) | 421 byte | |

Taka er uendra i W3. Tekstverdiane kostar 14 byte i teljarlastaren, som alt hadde `S` for `finn`. For `sett_tekst` åleine kostar dei 73 byte, fordi lastaren no treng `S`. `tekst_flyt` er 32 byte og kjem berre med når modulen parsar desimaltal.

## HTML-innsetjing og escaping

`sett_html` er den einaste vegen til `innerHTML`.

- **Malen** må vere ein tekstkonstant i kjelda, det vil seie HTML som utviklaren har skrive. Kompilatoren avviser alt anna.
- **Verdien** blir alltid sendt gjennom escape-hjelparen `H` i lastaren før han blir sett inn i staden for `{}`.
  - `H` blir emittert frå `std.wasm_vert.escape_tabell()`: `&`, `<`, `>`, `"` og `'`.
  - Det er same tabell som `std.html.escape` brukar.
  - `wasm_vert.html_escape` gir identisk tekst i VM-en. Testen påstår likskapen.

Brukardata kan dermed ikkje nå `innerHTML` uescapa. W3: verdien til `sett_html` og `sett_tekst` er ein tekstverdi (typen `verdi`: `tekst(x)` i postkassa). `sett_html` sender han alltid gjennom `H`, og `sett_tekst` set `textContent`, som nettlesaren ikkje tolkar som HTML. `test_wasm_korpus_w3` sjekkar begge i Chrome, med ein `<script>`-tagg og HTML-teikn i verdien.

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
| W3: `starts_with("abc", "")`, `ends_with("abc", "")` | `sann` | `usann` | `sann` |
| W3: `join([1, 2.5], "-")` | `1-4612811918334230528` (bitane til 2.5) | krasj | `1-2.5` |
| W3: `json_parse_raw("[1,")` | `[1, 0]` | `[1, 0]` | kastar `JSON: uventa slutt` |

Paritetskorpuset testar berre kombinasjonar der begge VM-ane er samde.

- For `+` følgjer WASM den tekstbaserte regelen. Der er den eine VM-en udefinert.
- For sanning følgjer WASM den tilsikta regelen: `""` og `0.0` er usanne.
- W3: tom prefiks eller suffiks gjev `sann` (som `contains` og `index_of`, og som i JavaScript). `join` brukar `tekst(x)` for element som ikkje er tekst. Ugyldig JSON kastar (sjå «JSON»).

W3-korpuset (5 program) gav same utskrift på begge VM-ane. `tekst_til_liten` blir berre køyrd på korte tekstar, sidan Linux-stage0 heng eller får SEGV på `lower` over 256 byte.

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
- W3 endra ingen av desse: program som ikkje brukar runtime-biblioteket, er byte-identiske med W2.

W3-korpuset (testbygg, byte):

| Modul | Storleik | rt-funksjonar |
|---|---|---|
| `rt_tekst` | 8 206 | tekstfunksjonane og `verdiar` |
| `rt_json` | 17 271 | `json_parse` og `json_skriv` med hjelparar |
| `rt_json_feil` | 15 507 | same |
| `rt_sorter` | 6 360 | `sorter`, `sortert`, `tekst_til_liste` |
| `rt_sha256` | 8 721 | `sha256_hex` (`std/sha256.no`) og `json_skriv` |
| Tom app + heile biblioteket | 20 198 | alle 17 |
| Berre `split` (utan testbygg) | 2 792 | `splitt` |

Modular som brukar `vert.finn` eller `vert.sett_tekst`, har vorte litt større, fordi innpakkaren gjer `tekst(x)` og skriv til postkassa. Dei får då atoma `tekst_av` og `til_minne`.

## Testar

| Test | Kva han dekkjer |
|---|---|
| `tests/test_wasm_w0.no` | Validatoren godtek fasitmodular og avviser éin feil om gongen. Korpuset blir bygd med CLI-en, med dei venta importane og eksportane. `minimal.no` frå CLI-en er byte-eksakt og har ingen rt-funksjonar. Rapporten frå CLI-en er fail-closed: åtte feil i `feil_ustotta.no`, mellom anna `bryt` ut av `prøv` og `utsett` i ei lykkje (W2), ein HTML-mal som ikkje er konstant (W3), og `ustøtta builtin: ukjend`. Sju i testbygg. Versjonen følgjer bytane. W3: lowring og validering skjer berre i CLI-barneprosessane. |
| `tests/test_wasm_w1.no` | GC-fasitmodulen frå proben blir bygd på nytt med assembleren og er byte-eksakt. Kodingane til GC-typane og instruksjonane er rette. W1-validatoren (datacount, array-typar) avviser éin feil om gongen. Tree-shakinga er rett. |
| `tests/test_wasm_les.no` | W2-typesjekken: ein handbygd modul (kontrollflyt, i64, GC, `try_table`/`throw`, `ref.func`, `call_ref`, globalar) blir godteken. 17 variantar med éin feil kvar blir avviste med rett melding (W3: validerte i `tests/fixtures/wasm_les_typesjekk.no` i fastmodus, og testen sjekkar kvar linje). Ekte korpusmodular blir godtekne, og avviste etter mutasjon: utan elementsegmentet, og med feil tag-type. |
| `tests/test_wasm_w2.no` | W2-lowringa på bytenivå: ingen tag- eller elementseksjon utan unnatak og closures. Init i `try_table` og `throw` for `THROW`. `unntak_passar` berre ved typa fang. `kast` er `throw`, ikkje trap. Deklarativt elementsegment og `call_ref` for closures. `$lambda`-typen for `ncb_call_fn`. |
| `tests/test_wasm_w3.no` | W3-lenkinga: `lenk_rt` fører berre rt-modulane inn (ikkje drivaren) og endrar ikkje inndata. Utan lenking er `split` ein kompileringsfeil. Tree-shaking frå CLI-en: `split` gjev `rt=splitt` og ingen desimalimport, `sha256` gjev `rt=sha256_hex`, og `desimaltall(tekst)` gjev `tekst_flyt`. Heile biblioteket har alle 17 offentlege funksjonar og er under taket på 24 KB. Fail-closed rapport med biblioteket lenka inn. |
| `tests/test_wasm_korpus_chrome.no` | Paritetskorpuset (W1 + W2), sjå under. |
| `tests/test_wasm_korpus_w3.no` | Paritetskorpuset for W3 (`kh.korpus_w3()`), og tekstverdiar i DOM-en i Chrome: `sett_tekst` via utrekna selektorar, med HTML-teikn og eit desimaltal, og `sett_html` med ein `<script>`-tagg i verdien. |
| `tests/test_wasm_lastar.no` | Byte-tak og tree-shaking. Importnamna i lastaren er lik importseksjonen. Strukturen kjem frå tabellen. Ingen JS-fragment i literalane. Kvart lastartoken finst i emitteren eller i tabellen (proveniens). Escape er lik `std.html.escape`. |
| `tests/test_wasm_serve.no` | Barne-`nc serve` leverer `application/wasm` byte-identisk, med versjonert cache og CSP per side. |
| `tests/test_wasm_w0_chrome.no` | Valfri. Teljaren etter tre klikk, negativ CSP-kontroll og W0-korpuset mot VM-en. |

Chrome-hjelparane ligg i `tests/fixtures/wasm_chrome_hjelp.no`. Der les `dump_med_konsoll` DOM-en og Chrome-konsollen (stderr med `--enable-logging`). Korpushjelparane ligg i `tests/fixtures/wasm_korpus_hjelp.no`, modulinnsyn (seksjonar, kroppar) i `tests/fixtures/wasm_test_hjelp.no`, og `tests/fixtures/wasm_valider_fil.no` validerer ei fil i ein barneprosess. W3: `kh.køyr_korpus` er heile tre-stegs-køyringa, som begge korpustestane brukar.

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
| `rt_tekst` (W3) | `split`, `join`, `trim`, `tekst_erstatt`/`replace`, `starts_with`, `ends_with`, `contains`, `index_of`, `lower`/`upper` og `verdier`: UTF-8, tomme tekstar og felt, overlapp, æøå urørt av store/små bokstavar |
| `rt_json` (W3) | `json_stringify` med escaping, desimaltal og tomme strukturar; rundtur for 20 NSP/1-liknande dokument; ikkje-kanonisk input, `\u`-escape, surrogatpar, tal (wrap, eksponent, `-0`), duplikatnøklar, 60 nivå nøsting |
| `rt_json_feil` (W3) | 22 ugyldige input med feiltekstane, `fang (e: JSON)`, feil gjennom to rammer, `fang (e: tekst)` bak ein handterar som ikkje passar, og at gyldig input gjev same verdi som builtinen |
| `rt_sorter` (W3) | `rt.sorter` med closure (stigande, synkande, etter lengd og etter felt, fanga variabel), stabilitet, `rt.sortert`, tomme og eittelements lister, 200 element, `rt.tekst_til_liste` |
| `rt_sha256` (W3) | FIPS-vektorane (`abc`, tom, 448 og 896 bit), UTF-8, 55/56/63/64/65/1000 byte |
| `kontroll_trap` | ikkje paritet: positiv kontroll for konsollsjekken |

Kvart program har ein **kjend fasit** i `#=`-linjer, og `test_wasm_korpus_chrome` køyrer dei i tre steg:

1. Køyrer kvart program i VM-en (barne-`nc run`) og krev at stdout er lik fasiten.
2. Byggjer kvart program i testbygg med CLI-en. Modulen blir validert, med typesjekk, og må importere `logg_test`. W2-programma må ha `uhandtert`.
3. Startar éin barne-`nc serve` med ei side per program. Headless Chrome køyrer sidene. Teksten i `<pre id="nc-logg">` må vere identisk med VM-utskrifta, og konsollen må vere fri for «Uncaught». Utan Chrome blir dette steget SKIP. `NC_CHROME` vel binær.

`test_wasm_korpus_w3` køyrer W3-programma på same måten (`kh.køyr_korpus`). `rt_json_feil` og `rt_sorter` kallar rt-funksjonane direkte (`bruk std.wasm_rt som rt`), så VM-steget køyrer sjølve biblioteket som Norscode. Dei andre kallar builtins, som er vertsbuiltins i VM-en og biblioteket i WASM. Då samanliknar korpuset semantikken til biblioteket med VM-en.

**Resultat:** alle 18 programma gav identisk utskrift i VM-en og i Chrome 154 (macOS), utan «Uncaught». Dei same 18 modulane gav identisk utskrift i JavaScriptCore frå macOS 26.6.2 (Safari 26.6-motoren), køyrt med eit scratch-skript utanfor repoet. Linux i Docker har ikkje Chrome, så der køyrer steg 1 og 2.

**W3-resultat:** alle 5 W3-programma gav identisk utskrift i VM-en (macOS og Linux) og i Chrome 154, utan «Uncaught», og DOM-sida viste tekstverdiane som venta. Med den endelege W3-koden gav alle 23 korpusmodulane identisk utskrift i JavaScriptCore frå macOS 26.6.2. Scratch-skriptet utanfor repoet køyrer den genererte `nc.js` med ein shim for `document`, `fetch` og `TextEncoder`/`TextDecoder`.

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
| W3: `splitt` utan det siste feltet | `test_wasm_korpus_w3` |
| W3: `\t` ikkje escapa i `json_skriv` | `test_wasm_korpus_w3` |
| W3: ustabil `sorter` (`<= 0`) | `test_wasm_korpus_w3` (VM-steget, `rt_sorter`) |
| W3: `ends_with` kopla til `starter_med` i `rt_tabell` | `test_wasm_korpus_w3` (Chrome-steget, `rt_tekst`) |
| W3: `sett_html` utan escape | `test_wasm_korpus_w3` (DOM-sjekken) |
| W3: `sett_tekst` via `innerHTML` | `test_wasm_korpus_w3` (DOM-sjekken) |
| W3: `sha256_hex` på feil input | `test_wasm_korpus_w3` |
| W3: feiltekst for uavslutta streng endra | `test_wasm_korpus_w3` (VM-steget, `rt_json_feil`) |
| W3: alle rt-kall dreg med `json_parse` (utan tree-shaking) | `test_wasm_w3` |
| W3: `lenk_rt` lenkar òg inn drivaren (`__main__`) | `test_wasm_w3` |
| W3: `område` manglar i feilrapporten | `test_wasm_w0` (CLI-rapporten) |
| W3: høgdesjekken ved `end` slått av (etter flyttinga til barneprosessen) | `test_wasm_les` |

### Tid

Tida er målt i sekund med `./bin/nc test` (standard) og `NC_TEST_VM_FAST=1 ./bin/nc test` (fast). «Før» er målt på same maskin rett før W3-endringane, med W2-koden.

| Test | macOS standard før → etter | macOS fast før → etter | Linux standard | Linux fast |
|---|---|---|---|---|
| `test_wasm_w0` | 69 → 9 | 6 → 6 | 17 | 12 |
| `test_wasm_les` | 47 → 6 | 5 → 6 | 14 | 11 |
| `test_wasm_w1` | 7 → 11 | 4 → 5 | 16 | 12 |
| `test_wasm_w2` | 11 → 11 | 4 → 5 | 16 | 13 |
| `test_wasm_w3` (ny) | 11 | 10 | 20 | 16 |
| `test_wasm_lastar` | 19 → 23 | 3 → 4 | 18 | 10 |
| `test_wasm_serve` | 3 → 5 | 2 → 3 | 10 | 6 |
| `test_wasm_binary`, `test_wasm` | ≤ 1 | ≤ 1 | 4 | 3 |
| `test_wasm_korpus_chrome` | 49 → 53 | 42 → 46 | 45 | 44 |
| `test_wasm_korpus_w3` (ny) | 30 | 27 | 29 | 25 |
| `test_wasm_w0_chrome` | 10 → 12 | 8 → 9 | 4 | 2 |

- **`test_wasm_w0` (69 → 9 s):** lowringa og valideringa av `minimal` og `feil_ustotta` skjer ikkje lenger i testprosessen. Testen les modulen og feilrapporten frå CLI-en, som køyrer i fastmodus. I standard-VM-en tok lowringa av `feil_ustotta` om lag 18 s (to gonger), og typesjekken av `minimal` 15 s.
- **`test_wasm_les` (47 → 6 s):** dei 18 handbygde modulane blir validerte i `tests/fixtures/wasm_les_typesjekk.no` i fastmodus. Testen sjekkar kvar linje mot forventinga.
- Korpuset er delt i to testar, W1+W2 (18 program) og W3 (5 program + DOM-sida). Kvar held seg dermed under minuttet. Ein korpustest med alle 23 ville ha teke om lag 80 s i standard-VM-en.
- Ingen wasm-test er over 55 s i standard-VM-en på macOS. Dei største er dei to korpustestane, og der går tida til barneprosessar og Chrome, ikkje til testprosessen. Utan Chrome (CI) blir dei kortare.
- Tidene varierer med ±3 s mellom køyringar (sjå `test_wasm_w1`, som ikkje er endra).
- Linux (Docker `nc-x86tools`, stage0 frå `bootstrap/`, eigen `build/`) har ikkje Chrome, så Chrome-stega er SKIP der. Alle testane er grøne på begge plattformene og i begge modusane.

## Gjenstår før W4

- **Vertsfunksjonar (W4, import-ABI v1):**
  - Tekst frå DOM-en til WASM (t.d. `input.value`). Vegen finst (`skriv_tekst` til postkassa og resultattypen `lengd`), men han manglar eit atom som byggjer `$tekst` frå minnet, og ein Norscode-stubb.
  - Berre éin `verdi`-parameter per vertsfunksjon, sidan alle deler postkassa. Fleire krev ein forskyving per parameter.
  - Hendingar med data (input, submit), og fleire DOM-operasjonar.
- **Runtime-biblioteket:**
  - `fjern_nokkel` (seks treff i `std/html*.no`, `std/frontend.no` og `examples/`) treng eit atom som fjernar frå `$ordbok` og held rekkjefølgja. Det kan ikkje skrivast i Norscode.
  - VM-en sin `json_parse` (tekstverdiar) er avvist med hint. Skal han støttast, må han speglast nøyaktig.
  - `tekst_til_liten`/`tekst_til_store` endrar berre ASCII, i begge VM-ane og i WASM. Skal æøå med, må VM-en endrast samtidig, elles bryt pariteten.
  - Ugyldig JSON kastar i WASM, men gjev ein delvis verdi i VM-en (udefinert oppførsel der).
- **Byggjetid:** rt-funksjonane blir lowra på nytt i kvart bygg (1,9 s for `rt_json` i fastmodus). Ein cache for lowra rt-funksjonar per kjeldehash ville kutta det.
- **Unnatak:**
  - `bryt`/`fortsett` ut av `prøv` og `utsett` i lykkjer er avviste. Dei krev dynamisk try- og opprydjingsstakk, eller at kompilatoren emitterer `TRY_END` før hoppet (reseed).
- **Closures:**
  - Kall av ein fanga closure som `f(x)` inne i ein lambda krev ei endring i kompilatoren (reseed). Til då går det med `ncb_call_fn`. Ein closure som parameter (som komparatoren til `rt.sorter`) kan kallast direkte.
  - Funksjonsnamn som verdi (`kart(l, dobbel)`) blir `LOAD_NAME` av eit ukjent namn, også i VM-en.
- **Ytelse (W10):**
  - Tekstfunksjonane i biblioteket lagar eitt utsnitt per byteposisjon (`slice` + `char_code`). Eit atom for bytetilgang, eller ein unboxa i32-sti, ville gjere dei raskare.
  - Handteraren sjekkar typar med tekstsamanlikning per handterar.
  - `ncb_call_fn` samanliknar metodenamn lineært.
  - Metodekall byggjer namnet med to tekstkonkateneringar. Ein statisk tabell per `__type__` ville vore raskare.
- **Validator:** subtyping mellom ulike typeindeksar (deklarerte supertypar, strukturell likskap mellom rec-grupper) og legacy-unnatak er ikkje støtta.
- **Nettlesarar:** Firefox er ikkje testa (ikkje installert). Golvet for exnref er [A].
