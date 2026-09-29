# Klientlogikk i Norscode kompilert til WebAssembly (W0–W4)

Status: milepælane W0–W4 i WASM-sporet (app-kjensle del 2). Klientkode blir skriven i Norscode og kompilert til ein WasmGC-modul. Nettlesaren startar modulen med ein liten lastar som blir **emittert frå Norscode-data**. Det finst ingen handskriven JavaScript i repoet.

- **W0** gav vegen frå kjelde til nettlesar: heiltal, kontrollflyt, kall og DOM-vertsfunksjonar.
- **W1** gav verdimodellen: tekst, desimaltal, lister og ordbøker, med same semantikk som VM-en. Eit paritetskorpus køyrer kvart program i VM-en og i Chrome og krev identisk utskrift.
- **W2** gav unnatak (`prøv`/`fang`/`endeleg`, `utsett`, typa `fang`, innebygde feil som fangbare unnatak), closures og funksjonsverdiar, strukturmetodar, og ein stakk- og typesjekk i validatoren.
- **W3** gav runtime-biblioteket `std/wasm_rt.no`: tekstfunksjonar, JSON, sortering og SHA-256 skrivne i vanleg Norscode og kompilerte av same backend, med tree-shaking. Vertsfunksjonane tek tekstverdiar.
- **W4** gav vertsfunksjonane for ei ekte interaktiv side (tekst frå DOM-en, attributt og klassar, hendingar med data og delegering, tidtakarar, navigasjon, lokal lagring, handtakstabell med `slepp`), atoma `fjern_nokkel` og `tid_ms`, ein cache for lowra runtime-funksjonar, serve-integrasjonen `std/wasm_serve.no` (appen importerer ikkje lenger noko under `build/`), og demoen `examples/wasm_skjema/`.

## Bygg og køyr

```sh
NC_WASM_KJELDE=examples/wasm_skjema/klient.no NC_WASM_APP=examples/wasm_skjema/app.no \
    NC_WASM_UT=build/wasm/wasm_skjema ./bin/nc run tools/nc_wasm.no
./bin/nc serve build/wasm/wasm_skjema/serve.no --port 8080
```

`tools/nc_wasm.no` kompilerer klientkjelda (`NC_WASM_KJELDE`, i ein barneprosess) og runtime-biblioteket (sjå «Runtime-bibliotek»), og lenkar det inn i NCB-en. `NC_WASM_NCB` (ein NCB frå `nc compile`) går framleis. Så skriv han desse filene til `NC_WASM_UT`, som må liggje under `build/wasm/`:

| Fil | Innhald |
|---|---|
| `app.wasm` | Modulen |
| `nc.js` | Lastaren, berre for innsyn |
| `klient_data.no` | Norscode-modul med bytane som éin rå tekstliteral (W4; i W0–W3 var det hex), lastaren og versjonane |
| `serve.no` | (med `NC_WASM_APP`, W4) inngangen for `nc serve`: importerer appen og `klient_data`, og leverer modulen og lastaren med `std/wasm_serve.no` |

Alt er generert og blir aldri committa. Både `build/` og `*.wasm` er git-ignorerte. Mappene i `NC_WASM_APP` og `NC_WASM_UT` blir modulnamn (`build.wasm.x.klient_data`), så dei må vere gyldige namn og ikkje nøkkelord (t.d. ikkje `test`).

Verktøyet validerer modulen med `std/wasm_les.no` før det skriv han, og skriv éi linje:

```
WASM: app.wasm <n> byte, lastar <m> byte, v=<12 hex> importar=<a,b> eksportar=<m,i> atom=<…> rt=<…> rt_cache=<treff>/<n>
```

Importane og eksportane er dei validatoren las ut av bytane. `atom` er runtime-atoma modulen fekk med, og `rt` er funksjonane frå `std/wasm_rt.no` (utan modulprefikset). `rt_cache` er kor mange av `std.*`-funksjonane i modulen som kom frå cachen (W4, sjå «Cache av lowra runtime-funksjonar»).

Lowringa er rask i VM-fastmodus (`NORSCODE_VM_FAST=1`). `liste.no` tek 0,4 s. I standard-VM-en tek same modul om lag 110 s, fordi kvart funksjonskall og kvart oppslag som gjev ei liste eller ordbok kostar millisekund der. Bygg difor med CLI-en i fastmodus. Testane gjer det same.

## Modular

| Fil | Rolle |
|---|---|
| `std/wasm_lowring.no` | NCB → WASM: analyse (stakkhøgder og unnatakstilstand), dispatch-lykkje, unnatakshandterar, closures, seksjonar, flyttbare kroppar (W4) |
| `std/wasm_atom.no` | Runtime-atom: verdimodellen som WASM-funksjonar, skrivne med ein liten assembler |
| `std/wasm_les.no` | Validator: struktur (W0/W1) og stakk- og typesjekk (W2) |
| `std/wasm_rt.no` | Runtime-bibliotek i vanleg Norscode (W3) |
| `std/wasm_vert.no` | Vertstabell, klient-API, lister over trygge attributt, taggar og hendingar, og HTML-escape |
| `std/wasm_js.no` | Norscode→JS-emitter for lastaren |
| `std/wasm_serve.no` | Serve-integrasjon: modul- og lastarsvar, script-tag og CSP (W4) |
| `tools/nc_wasm.no` | CLI |

Ingen av dei ligg i `nc_main`-lukkinga, så dei krev ingen reseed. `std/wasm_rt.no` importerer `std/sha256.no` (som ligg i lukkinga), men endrar han ikkje. `std/wasm_serve.no` importerer `std/web.no`, som heller ikkje ligg i lukkinga.

## Slik skriv du ein klientmodul

Ein klientmodul er ei vanleg Norscode-fil med `start()`. Sjå `examples/wasm_skjema/` for eit fullt døme.

1. **Klienten** (`klient.no`): `bruk std.wasm_vert som vert`. Modulglobalar held tilstanden (handtak, lister, ordbøker). `start()` finn elementa og registrerer lyttarane:

   ```
   bruk std.wasm_vert som vert

   la ut_h: heltall = 0

   funksjon skreiv() -> heltall {
       vert.sett_tekst(ut_h, "Hei, " + builtin.trim(vert.hending_verdi()) + "!")
       returner 0
   }

   funksjon start() -> heltall {
       ut_h = vert.finn("#ut")
       vert.lytt(vert.finn("#namn"), "input", "skreiv")
       returner 0
   }
   ```

   - Tilbakekall er funksjonar i same modul **utan parametrar**. Namnet står som tekstkonstant (`"skreiv"`), og funksjonen blir eksportert som `_skreiv`. Hendingsdata blir lesne med `vert.hending_verdi()`, `hending_tast()`, `hending_id()`, `hending_data("id")` og `hending_mål()`.
   - Mange like element (rader i ei liste): gje dei `data-nc-klikk="fjern"` (i HTML-en, eller med `vert.sett_handlar(h, "klikk", "fjern")`) og `data-id`, og registrer éin gong: `vert.deleger("klikk", "fjern")`. Ingen lyttar per rad.
   - Lag element med `vert.lag("li")`, fyll dei med `sett_tekst`/`sett_attr`, set dei inn med `legg_inn`, og `slepp` handtaka du ikkje treng meir. Brukardata går aldri til HTML: `sett_tekst` set `textContent`, og `sett_html` escapar alltid verdien.
   - Reglar som ikkje rører DOM-en (validering, filtrering), bør vere reine funksjonar. Dei køyrer då likt i VM-en og kan testast der (sjå `tests/test_wasm_w4_chrome.no`).
2. **Appen** (`app.no`): `bruk std.wasm_serve som ws`. WASM-sida blir levert med `ws.side(html)` (CSP med `'wasm-unsafe-eval'`), og HTML-en har `ws.script_tag()` i `<head>`. Alle andre sider brukar `ws.vanleg_side(html)`. Appen importerer ingenting under `build/`, og sida må fungere (utan klientlogikk) når WASM manglar.
3. **Bygg** med `NC_WASM_KJELDE`, `NC_WASM_APP` og `NC_WASM_UT` (sjå over), og **server** `build/wasm/<app>/serve.no`.
4. **Test**: bygg ein testklient med `NC_WASM_TESTBYGG=1` som importerer klienten, kallar `start()` og køyrer brukarsteg med testkrokane (`test_skriv`, `test_send`, `test_klikk`, `test_tast`). `skriv(...)` går då til `<pre id="nc-logg">`, som headless Chrome les (sjå `tests/fixtures/wasm_skjema_testklient.no`).

Det som ikkje går, blir avvist ved kompilering med ei liste over alle feil: attributtnamn som ikkje er konstantar eller ikkje er trygge, taggar som `script`, ukjende hendingar, tilbakekall med parametrar eller som ikkje finst, og testkrokar utan testbygg.

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

Init (`i`) og innpakkinga av tilbakekall (`_<namn>`, W3: `k0`, `k1`, …) kallar koden inne i `try_table`. Eit unnatak som ingen fangar, blir skrive som `ERROR: <tekst(v)>`, som VM-en skriv før han avsluttar. Modulen returnerer så normalt, og ingenting når JavaScript. Korpustesten krev at konsollen i Chrome er fri for «Uncaught», og har ein positiv kontroll (`kontroll_trap`) som viser at sjekken ser ein trap.

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

## Cache av lowra runtime-funksjonar (W4)

Runtime-biblioteket blir lowra på nytt i kvart bygg, sjølv om det nesten aldri endrar seg. W4 lagrar dei lowra funksjonane i `build/wasm/rt_cache/<nøkkel>.json` (mappa kan setjast med `NC_WASM_RT_CACHE_MAPPE`, og `NC_WASM_RT_CACHE=0` slår cachen av).

- **Flyttbare kroppar.** Når ein `std.*`-funksjon (runtime-biblioteket og det han importerer, ikkje closure-hjelparar) blir emittert, set lowringa `k["reloc"]`. Hjelparane skriv då ein *merkelapp* (ei ordbok) i staden for bytar som avheng av modulen: funksjons-, atom-, global-, dispatch- og typeindeksar (`F`, `A`, `G`, `D`, `TL`, `FT`), tekstliteralar (`L`) og funksjonsnamn i feilmeldingar (`N`). Kroppen blir lagra som *bitar*: bytelister og merkelappar om kvarandre. Ein kropp som brukar vertskonstantar eller innpakkarar, blir ikkje lagra.
- **Lenking.** `_lenk_bitar` byter merkelappane ut med bytane for modulen som blir bygd. Det skjer på same stad i rekkjefølgja som emitteringa, så literalar og typar blir registrerte i same rekkjefølgje, og modulen er **byte-identisk** med og utan cache. Lenkinga går per bit, ikkje per byte, og kodeseksjonen blir skriven rett inn i modulen (éin kopi av kvar byte, ikkje tre som før).
- **Invalidering.** Nøkkelen er sha256 over kjeldene til backenden (`lowring.backend_kjelder()`: `std/wasm_lowring.no`, `wasm_atom.no`, `wasm_binary.no` og `wasm_vert.no`). Ein funksjon blir berre henta når NCB-koden hans (`json_stringify` av funksjonen) er den same som då han vart lagra. Lowringa les fila først når ein funksjon kan cachast, så program utan rt-funksjonar slepp å rekne nøkkelen (om lag 110 ms).
- **Utan cache** blir kroppane emitterte direkte, som før W4 (`rt_cache=0/0`). Då syner ein feil i lenkinga som ein skilnad mellom bygga, og `test_wasm_w4` samanliknar dei.

Målt i fastmodus på macOS (heile CLI-en, inkludert kompilering, validering og skriving):

| Program | Utan cache | Kald cache | Varm cache | Modul |
|---|---|---|---|---|
| `rt_json` (19 `std.*`-funksjonar) | 4,05 s | 4,41 s | 3,38 s (−17 %) | byte-identisk, `v=3903de47c40e` som i W3 |
| Skjema-demoen (9) | 2,77 s | 3,02 s | 2,81 s | byte-identisk |
| `fak` (ingen) | 1,20 s | 1,20 s | 1,20 s | |

W3 brukte 4,3 s på `rt_json`. Utan cache er W4 raskare fordi kodeseksjonen blir skriven rett inn i modulen. Med varm cache blir både analysen og emitteringa av rt-funksjonane spart. Det som står att, er mest valideringa (1,4 s av 3,4 s for `rt_json`), atoma (0,3 s) og nøkkelen (0,1 s). For små rt-bruk er cachen omtrent null i netto.

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

## Subsett (W2–W4)

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
- W4: `fjern_nokkel` (også `fjern_nøkkel`) fjernar nøkkelen og held rekkjefølgja til resten (`array.copy` éin plass ned). Ein manglande nøkkel er ein no-op, og resultatet er 0, som i VM-en. `tid_ms` er `Date.now()` frå verten.

Alt anna gjev kompileringsfeil med ei liste over alle funna. Det gjeld:
- `område`, som VM-en heller ikkje har;
- VM-en sin `json_parse` (tekstverdiar) og builtins som ikkje finst i VM-en (t.d. `tekst_til_liste`). Rapporten har eit hint om den støtta vegen;
- asynkrone funksjonar;
- ukjende builtins;
- hopp ut av `prøv` og `utsett` i lykkjer (sjå over).

Det finst ingen stille fallback.

## Vertsfunksjonar (`bruk std.wasm_vert som vert`)

I VM-en kastar stubbane «wasm_vert.<namn> finst berre i nettlesaren». WASM-backenden byter kalla ut med importar frå modulen `nc`, og lastaren får berre dei som blir brukte.

**DOM (W0, W3)**

| Funksjon | JS-operasjon |
|---|---|
| `finn(selektor)` → handtak | `document.querySelector` (selektoren kan vere rekna ut) |
| `sett_tekst(h, verdi)` | `textContent = tekst(verdi)` |
| `sett_html(h, "mal med {}", verdi)` | `innerHTML`: malen er ein konstant, og `tekst(verdi)` blir alltid escapa |
| `lytt_klikk(h, "funksjon")` | `click`-lyttar (W0; `lytt` er den generelle) |

**Lese frå DOM-en (W4)** — tekst frå JS til WASM (resultattypen `tekst`):

| Funksjon | JS-operasjon |
|---|---|
| `hent_verdi(h)` | `value` |
| `hent_tekst(h)` | `textContent` |
| `hent_attr(h, namn)` | `getAttribute(namn) ?? ""` |

**Endre DOM-en (W4)**

| Funksjon | JS-operasjon |
|---|---|
| `sett_verdi(h, verdi)` | `value = tekst(verdi)` |
| `sett_attr(h, "attributt", verdi)` | `setAttribute`. Namnet må vere ein konstant på lista `trygge_attributt()` eller `data-*`/`aria-*` (ikkje `data-nc-*`). Aldri `on…`, `href`, `src`, `style`, `srcdoc`, `action` eller `formaction`. |
| `fjern_attr(h, "attributt")` | `removeAttribute` (same liste) |
| `sett_klasse(h, klasse, på)` | `classList.toggle(klasse, på)` (sanning som i `hvis`) |
| `vis(h, på)` | `hidden = !på` |
| `lag("tag")` → handtak | `document.createElement`. Taggen må vere på lista `trygge_taggar()` (ikkje `script`, `style`, `iframe`, `object`, `embed`, `link`, `meta`, `base`, `svg`, `math`). |
| `legg_inn(forelder, barn)` | `append` |
| `fjern_element(h)` | `remove()` |
| `tøm(h)` | `replaceChildren()` |
| `fokus(h)` | `focus()` |
| `slepp(h)` | Frigjer handtaket (sjå under) |

**Hendingar (W4)** — `hending` er ein konstant: `klikk` (click), `input`, `endring` (change), `send` (submit, med `preventDefault`) eller `tast` (keydown). Funksjonen er eit tilbakekall utan parametrar i same modul.

| Funksjon | JS-operasjon |
|---|---|
| `lytt(h, "hending", "funksjon")` → handtak | `addEventListener` med ein `AbortController` (handtaket) |
| `deleger("hending", "funksjon")` → handtak | Éin lyttar på `document`: kallar funksjonen når det næraste elementet med `data-nc-<hending>` har verdien `"funksjon"` |
| `sett_handlar(h, "hending", "funksjon")` | `data-nc-<hending>="funksjon"` (til `deleger`; begge er konstantar) |
| `hending_verdi()` | `target.value` til hendinga |
| `hending_tast()` | `key` (keydown) |
| `hending_id()` | `id` til elementet med lyttaren (ved delegering: elementet med `data-nc-…`) |
| `hending_data(nøkkel)` | `data-<nøkkel>` på same element |
| `hending_mål()` → handtak | same element som handtak |

Hendingsdata blir lesne medan tilbakekallet køyrer (lastaren held hendinga i `V` og elementet i `T`). Eit `data-nc-klikk` som peikar på ein funksjon som ikkje er delegert for `klikk`, gjer ingenting.

**Tidtakarar, navigasjon og lagring (W4)**

| Funksjon | JS-operasjon |
|---|---|
| `etter(ms, "funksjon")` → handtak | `setTimeout` |
| `intervall(ms, "funksjon")` → handtak | `setInterval` |
| `builtin.tid_ms()` | `Date.now()` (atomet `tid_ms`, same builtin som i VM-en) |
| `gå_til(url)` | `location.assign`, berre når `new URL(url, location.href).origin` er same opphav |
| `hent_sti()` | `location.pathname` |
| `hent_parameter(namn)` | `new URLSearchParams(location.search).get(namn) ?? ""` |
| `lagre_lokalt(nøkkel, verdi)` | `localStorage.setItem` (to tekstverdiar) |
| `hent_lokalt(nøkkel)` | `localStorage.getItem(nøkkel) ?? ""` |
| `skriv(x)` | `console.log` (i testbygg: `#nc-logg`) |

**Testkrokar** (berre med `NC_WASM_TESTBYGG=1`; elles kompileringsfeil): `test_klikk(h)` (`click()`), `test_skriv(h, verdi)` (set `value` og sender `input` som boblar), `test_tast(h, tast)` (`keydown` med `key`) og `test_send(h)` (`requestSubmit()`, som sender `submit` gjennom lyttarane).

**Handtakstabellen.** Eit handtak er ein indeks i tabellen `E` i lastaren. `slepp(h)`:
- aborterer ein lyttar (handtaket frå `lytt`/`deleger` er ein `AbortController`), eller stoppar ein tidtakar (`clearTimeout`, som òg stoppar intervall);
- set plassen til `null`, så nettlesaren kan frigjere elementet;
- legg indeksen i frilista `F`. Når modulen importerer `slepp`, plasserer lastaren nye handtak i ledige plassar (`P`), så tabellen ikkje veks med kvar gjengjeving. Skjema-demoen lagar og slepper 200 handtak per gjengjeving; etter ti gjengjevingar er handtaka framleis små (testen sjekkar det).

Eit handtak skal ikkje brukast etter `slepp` (plassen kan vere gjenbrukt).

**Interne** (W1, W3, W4) blir importerte av atoma og har ingen Norscode-stubb:

| Import | JS | Brukt av |
|---|---|---|
| `logg(s)` | `console.log(S(a,b))` | `skriv(x)` og `ERROR: …` |
| `logg_test(s)` | `document.getElementById("nc-logg").append(S(a,b))` | same, i testbygg |
| `flyt_tekst(x, p)` | `U(a.toExponential(),b)` | `tekst(desimaltall)` (W4: `U` i staden for `W`) |
| `tekst_flyt(s)` (W3) | `parseFloat(S(a,b))` | `desimaltall(tekst)` |
| `nå_ms()` (W4) | `Date.now()` | `builtin.tid_ms()` |

### Typar over grensa

Tabellen i `vertsfunksjonar()` skildrar kvar funksjon med namn, parametertypar, resultat og operasjonen som eit JS-uttrykkstre.

| Parametertype | Norscode → WASM → JS |
|---|---|
| `tekst` | tekstkonstant → (peikar, lengd) i minnet → streng |
| `hending`, `attributt`, `tag` (W4) | som `tekst`, men kontrollert mot `hendingar()`, `trygt_attributt()` og `trygg_tag()` ved kompilering |
| `funksjon` | konstant funksjonsnamn → eksportnamnet `_<namn>` → `x["_namn"]` |
| `verdi` | `tekst(x)` i postkassa → streng. W4: fleire `verdi`-parametrar ligg etter kvarandre (atomet `til_minne_ved`); alle `tekst(x)` blir rekna ut først, sidan `tekst(desimaltal)` sjølv brukar postkassa |
| `handtak`, `tal` (W4) | heltall → i32 → tal |
| `bool` (W4) | sanning som i `hvis` → i32 0/1 |
| `heltall`, `desimal`, `postkasse` | i64 (BigInt), f64, adresse (interne) |

| Resultattype | |
|---|---|
| `handtak` | i32 → i31ref |
| `ingen` | `null` |
| `tekst` (W4) | Importen får adressa til postkassa som siste argument. Lastaren skriv strengen som UTF-8 med `U` (som veks minnet når teksten er større enn det som er att), og gjev talet på bytar. Innpakkaren byggjer teksten med atomet `frå_minne` (éi lykkje; WasmGC har ingen instruksjon som kopierer frå lineært minne til ein GC-array). |
| `lengd`, `desimal` | interne |

### Lastaren

`std/wasm_js.no` byggjer lastaren token for token frå tabellen. Berre importerte funksjonar og hjelparane dei brukar (med avhengnader) kjem med:

| Hjelpar | Rolle |
|---|---|
| `E` | Handtakstabellen |
| `F`, `P` (W4) | Frilista, og plassering av nye handtak (berre når modulen importerer `slepp`) |
| `S` | UTF-8 frå minnet |
| `H` | HTML-escape, frå `escape_tabell()` |
| `U` (W4) | UTF-8 inn i minnet med vekst (erstattar `W` frå W1, som ikkje voks) |
| `N` (W4) | Norske hendingsnamn → DOM-hendingar, frå `hendingar()` |
| `V`, `T` (W4) | Gjeldande hending og elementet ho gjeld |
| `L` (W4) | Lyttar (direkte eller delegert), med `AbortController` og `preventDefault` for `send` |

Emitteren (W4) set parentesar etter presedensen i JS (`a-(b-c)`, `(a??b)||c`), har nodane `vilkår` (`?:`), `ikkje` (`!`) og `sekvens` (`(a,b)`), og skriv nøklar med æ, ø og å utan hermeteikn (`tøm:`, `gå_til:`).

**Minne «m»**
- Det aktive datasegmentet ligg først, med tekstkonstantane til vertsfunksjonane (selektorar, malar, attributt- og hendingsnamn, eksportnamn).
- Så kjem **postkassa**, 8-justert. Der ligg tekst over JS-grensa i begge retningar. Ho veks med `memory.grow` frå WASM-sida (`til_minne`, `til_minne_ved`) og frå JS-sida (`U`).
- Tekstliteralar som er verdiar, ligg i eit **passivt** segment 0 og blir laga med `array.new_data`.

**Lastarstorleik**, testa i `tests/test_wasm_lastar.no`:

| Lastar | Storleik | Tak |
|---|---|---|
| Teljar (W0-settet) | 543 byte (uendra) | 600 byte |
| Eitt import (`sett_tekst`) | 247 byte (uendra) | 400 byte |
| Heile produksjonstabellen (37 vertsfunksjonar: 33 for klientkoden og 4 interne, utan testkrokane) | 2 550 byte | 2 560 byte (planbudsjettet) |
| Heile tabellen med testkrokane (42) | 2 866 byte | 3 072 byte |
| Skjema-demoen (22 importar) | 1 755 byte | 2 048 byte |
| W4-proben (7 importar: `finn`, `sett_tekst`, `hent_verdi`, `deleger`, `hending_data`, `lagre_lokalt`, `nå_ms`) | 994 byte | |

Planen hadde 400 byte for W0-settet og 1 KB for eit typisk øy-program. Teljaren er framleis 543 byte (`H` og instansieringa åleine er om lag 300), og eit lite interaktivt program med lyttarar er rundt 1 KB. Skjema-demoen er større fordi han brukar 22 vertsfunksjonar og alle hjelparane (`L` åleine er om lag 300 byte).

## HTML-innsetjing og escaping

`sett_html` er den einaste vegen til `innerHTML`.

- **Malen** må vere ein tekstkonstant i kjelda, det vil seie HTML som utviklaren har skrive. Kompilatoren avviser alt anna.
- **Verdien** blir alltid sendt gjennom escape-hjelparen `H` i lastaren før han blir sett inn i staden for `{}`.
  - `H` blir emittert frå `std.wasm_vert.escape_tabell()`: `&`, `<`, `>`, `"` og `'`.
  - Det er same tabell som `std.html.escape` brukar.
  - `wasm_vert.html_escape` gir identisk tekst i VM-en. Testen påstår likskapen.

Brukardata kan dermed ikkje nå `innerHTML` uescapa. W3: verdien til `sett_html` og `sett_tekst` er ein tekstverdi (typen `verdi`: `tekst(x)` i postkassa). `sett_html` sender han alltid gjennom `H`, og `sett_tekst` set `textContent`, som nettlesaren ikkje tolkar som HTML. `test_wasm_korpus_w3` sjekkar begge i Chrome, med ein `<script>`-tagg og HTML-teikn i verdien.

## Serve-integrasjon, CSP, versjonering og cache (W4)

`std/wasm_serve.no` er utanfor `nc_main`-lukkinga og blir brukt av appen og av den genererte inngangen:

| Funksjon | |
|---|---|
| `ws.side(html)` | HTML-side som lastar WASM: CSP med `'wasm-unsafe-eval'`, `no-cache` |
| `ws.vanleg_side(html)` | Alle andre sider: standard-CSP |
| `ws.script_tag()` | `<script type="module" src="/_nc/nc.js"></script>` |
| `ws.script_tag_versjonert(lv)` | Same med `?v=<lv>` (når appen kjenner lastarversjonen) |
| `ws.modul_svar(ctx, bytar, v)` | `application/wasm`, nosniff, standard-CSP, `ETag: "<v>"`; `immutable` i eitt år berre ved `?v=<v>`, elles `no-cache` |
| `ws.lastar_svar(ctx, lastar, lv)` | Same for `text/javascript; charset=utf-8` |
| `ws.csp_standard()`, `ws.csp_wasm()` | CSP-tekstane (til `std/http_cache.no` når PR #206 er fletta) |

- `'wasm-unsafe-eval'` opnar ikkje for `eval` eller `new Function`. Berre sider som lastar WASM får han, ikkje ressursane sjølve.
- `v` for modulen er dei 12 første hex-teikna av `sha256_bytes` over bytane, og `v` for lastaren er sha256 over lastarteksten. Lastaren har `app.wasm?v=<v>` bakt inn.
- Script-taggen peikar på `/_nc/nc.js` utan versjon. Nettlesaren revaliderer han kvar gong, og `nc serve` svarar `304` når `If-None-Match` er lik ETag-en. Modulen er `immutable`. Ei sidevising kostar difor éin 304 for lastaren (~150 byte) og ingen for modulen.

**Hardkoda build-sti (W0) er løyst.** Appen importerer ikkje lenger `build.wasm.<app>.klient_data`. Med `NC_WASM_APP` lagar `tools/nc_wasm.no` inngangen `build/wasm/<app>/serve.no`, som importerer appen (rutene hennar blir med, sidan `nc serve` samlar rutene frå importerte modular) og `klient_data`, og legg til rutene for `/_nc/app.wasm` og `/_nc/nc.js`. Appen kan køyrast, sjekkast med `feature-check` og serverast utan at klienten er bygd. Då er det berre sida utan klientlogikk (og `/_nc/app.wasm` gjev 404).

**Kostnad per førespurnad.** `nc serve` har korkje disk-kapabilitet eller modulglobalar. Ein modulglobal gjev «Ukjent global variabel: __main__.x» under serve [V, målt W4], så bytane kan verken lesast frå disk eller dekodast éin gong og haldast. I W0–W3 var bytane hex og vart dekoda i kvar førespurnad. Målt med ein modul på 40 KB (`nc serve` med `NORSCODE_FAKE_HTTP_REQUESTS`):

| | Fastmodus | Standard-VM |
|---|---|---|
| Hex-dekoding per førespurnad | 730 ms (18 ms/KB) | 2 700 ms (68 ms/KB) |
| Rå tekstliteral (W4) | < 5 ms (ikkje målbart) | < 5 ms |

W4 skriv bytane som **éin rå tekstliteral** i `klient_data.wasm()`. Norscode-strengar er bytebaserte, og lexeren les literalen byte for byte. Det er prøvd for alle 256 byteverdiane under `nc serve`, med `cmp` mot fasiten. Berre `\` og `"` må escapast (mutasjonstestane viser det). Linjeskift, CR, tab og NUL blir likevel escapa for lesbarheit. `test_wasm_serve` skriv ut kostnaden: for teljaren (2 153 B) tok 1 førespurnad 72 ms og 21 førespurnader 75 ms, altså om lag 0 ms per ekstra førespurnad.

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

W4 endra ikkje W1–W3-korpuset: det er byte-identisk med W3, og `rt_json` har same `v`. Tilbakekall blir no eksporterte som `_<namn>` i staden for `k0`, `k1`, …, så modular med tilbakekall har nokre få byte meir i eksportseksjonen.

| Modul (W4) | Storleik (byte) | Lastar (byte) |
|---|---|---|
| `w4_ordbok` (korpus, testbygg) | 4 057 | |
| `tests/fixtures/wasm_w4_vert.no` | 2 146 | 994 |
| Skjema-demoen (`examples/wasm_skjema/klient.no`) | 10 242 | 1 755 |
| Skjema-demoen med testklienten (testbygg) | 13 976 | 2 235 |

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
| `tests/test_wasm_lastar.no` | Byte-tak og tree-shaking. Importnamna i lastaren er lik importseksjonen (også namn med æøå). Strukturen kjem frå tabellen. Ingen JS-fragment i literalane. Kvart lastartoken finst i emitteren eller i tabellen (proveniens). Escape er lik `std.html.escape`. W4: produksjonstabellen ≤ 2 560 byte, med testkrokar ≤ 3 072 og skjema-demoen ≤ 2 048; `U`/`L`/`N`/`P`/`F` blir tree-shaka etter bruk; `setAttribute` og `createElement` berre med namn som er kontrollerte ved kompilering; ingen tildeling til `on…`-felt; navigasjon berre til same opphav; presedens i emitteren. Sjekkane står i `tests/fixtures/wasm_lastar_sjekk.no` og køyrer i ein barneprosess i fastmodus. |
| `tests/test_wasm_serve.no` | Barne-`nc serve` av serve-inngangen (W4): `application/wasm` byte-identisk; `immutable` berre ved rett `v`; ETag og 304; lastaren utan `v` med `no-cache`; `wasm-unsafe-eval` berre på WASM-sida; `<script type="module">`; versjonen endrar seg når éin byte endrar seg; appen åleine (utan bygg) kan serverast; målt kostnad per førespurnad. |
| `tests/test_wasm_w4.no` | Fail-closed rapport for utrygge vertskall (11 feil). Atom og importar for dei nye vegane i ein ekte modul, men ikkje i teljaren. rt-cachen: bygga utan cache, med kald og med varm cache er byte-identiske. Den varme hentar kropp og analyse for alle `std.*`-funksjonane. Ei endra oppføring blir lowra på nytt, og ein øydelagd kropp blir brukt (validatoren avviser han). Filnamnet følgjer backend-kjeldene. Stubbane kastar i VM-en. |
| `tests/test_wasm_w4_chrome.no` | Korpuset `w4_ordbok` (VM, bygg, Chrome). Skjema-demoen i testbygg med brukarsteg gjennom testkrokane mot fasit (reglane er rekna i VM-en), og i produksjonsbygg med `?q=`. Negativ kontroll med standard-CSP. Sjå «Skjema-demoen». |
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
| `w4_ordbok` (W4) | `fjern_nokkel`/`fjern_nøkkel`: rekkjefølgje, manglande nøkkel, nøkkel som kjem attende, alle bort, i lykkje; `tid_ms` |
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
| W4: `onclick` på lista over trygge attributt | `test_wasm_w4` |
| W4: to tekstverdiar utan `til_minne_ved` | `test_wasm_w4` |
| W4: cachen blir ikkje lesen | `test_wasm_w4` |
| W4: feil lenking av funksjonsnamn-literalen (`N`) | `test_wasm_w4` (skilnad mot bygget utan cache) |
| W4: analyseverknadene spela av baklengs | `test_wasm_w4` |
| W4: `tid_ms` gjev 0 | `test_wasm_w4_chrome` (korpuset) |
| W4: `fjern_nokkel` flyttar ikkje verdiane | `test_wasm_w4_chrome` (korpuset) |
| W4: `send` utan `preventDefault` | `test_wasm_w4_chrome` (demoen) |
| W4: delegeringa invertert (`!=`) | `test_wasm_w4_chrome` (demoen) |
| W4: `U` utan minnevekst | `test_wasm_w4_chrome` (stor tekst frå DOM-en) |
| W4: `P` utan gjenbruk | `test_wasm_w4_chrome` («handtak små») |
| W4: fleire tekstverdiar frå same slot | `test_wasm_w4_chrome` (lokal lagring) |
| W4: `frå_minne` forskyvd éin byte | `test_wasm_w4_chrome` |
| W4: `immutable` også ved feil `v` | `test_wasm_serve` |
| W4: `csp_wasm` på vanlege sider | `test_wasm_serve` |
| W4: modulen utan ETag | `test_wasm_serve` |
| W4: `"` eller `\` ikkje escapa i den rå literalen | `test_wasm_serve` |
| W4: `L` utan `N` i avhengnadene | `test_wasm_lastar` (via barneprosessen) |
| W4: `lytt_klikk` via `onclick=` | `test_wasm_lastar` |

Kontrollar:
- Ein semantisk no-op i lenkinga gav grøn `test_wasm_w4`.
- Å ikkje escape linjeskift eller NUL i den rå literalen gav grøn `test_wasm_serve`. Det er ikkje ein feil: lexeren les rå bytar (sjå «Serve-integrasjon»).

### Skjema-demoen

`examples/wasm_skjema/` er ei heil side med klientlogikk i Norscode:
- live validering av namn og e-post, med feilmelding, `aria-invalid`, klassen `ugyldig` og lagre-knappen av eller på;
- «send» utan sidelasting;
- ei liste med 50 namn som blir filtrert medan ein skriv (`#nc-resultat` viser talet på treff), der Escape tømer filteret og `?q=` fyller det ved start;
- «Fjern» per rad via delegering, og ein statusmelding som forsvinn etter 1,5 s.

Lista blir lagra i `localStorage`, og handtaka blir sleppte etter kvar gjengjeving.

`test_wasm_w4_chrome` køyrer han i Chrome 154 (headless, `--virtual-time-budget=5000`):
- Testbygget (`tests/fixtures/wasm_skjema_testklient.no`) gav 33 logglinjer, alle lik fasiten.
- Etter tidtakaren var statusen tom, og DOM-en hadde 50 rader og 50 knappar med `data-nc-klikk="fjern"`.
- Produksjonsbygget gav 7 treff og 7 rader for `/?q=ber`, og 50 for `/`.
- Same side med standard-CSP gav «–» og ingen rader, og det var ingen «Uncaught» i konsollen.

JavaScriptCore (macOS 26.6.2) gav identisk utskrift for `w4_ordbok` (scratch-skript utanfor repoet). Demoen treng ein DOM og er ikkje køyrd i JSC.

### Tid

Tida er målt i sekund med `./bin/nc test` (standard) og `NC_TEST_VM_FAST=1 ./bin/nc test` (fast), med W4-koden. W3-tala står i parentes der testen fanst før.

| Test | macOS standard | macOS fast | Linux standard | Linux fast |
|---|---|---|---|---|
| `test_wasm_w0` | 14 (9) | 7 (6) | 16 | 12 |
| `test_wasm_les` | 7 (6) | 5 (6) | 12 | 9 |
| `test_wasm_w1` | 8 (11) | 6 (5) | 15 | 10 |
| `test_wasm_w2` | 11 (11) | 6 (5) | 13 | 11 |
| `test_wasm_w3` | 14 (11) | 10 (10) | 17 | 13 |
| `test_wasm_w4` (ny) | 21 | 14 | 23 | 20 |
| `test_wasm_lastar` | 10 (23) | 12 (4) | 15 | 14 |
| `test_wasm_serve` | 8 (5) | 4 (3) | 10 | 6 |
| `test_wasm_binary`, `test_wasm` | ≤ 1 | ≤ 1 | 3 | 2 |
| `test_wasm_korpus_chrome` | 53 (53) | 46 (46) | 40 | 38 |
| `test_wasm_korpus_w3` | 28 (30) | 25 (27) | 23 | 24 |
| `test_wasm_w0_chrome` | 11 (12) | 11 (9) | 4 | 3 |
| `test_wasm_w4_chrome` (ny) | 20 | 18 | 16 | 13 |

- **`test_wasm_lastar`:** med W4-tabellen tok emitteringa og tokeniseringa av dei fulle lastarane om lag 3 minutt i standard-VM-en (189 s målt). Sjekkane står no i `tests/fixtures/wasm_lastar_sjekk.no` og køyrer i ein barneprosess i fastmodus, som valideringa i W3.
- **`test_wasm_korpus_chrome`** var 57–58 s ei stund i W4. Årsaka var at kvart bygg rekna nøkkelen til rt-cachen (sha256 over ~175 KB, ~110 ms), også for program utan rt-funksjonar. Lowringa les no cachen først når ein funksjon kan cachast (korpusbygget: 1,42 → 1,20 s; W3: 1,01 s). Resten av skilnaden mot W3 er større modular å laste og ein større vertstabell.
- Ingen wasm-test er over 55 s i standard-VM-en på macOS. Tidene varierer med ±3 s mellom køyringar.
- Linux (Docker `nc-x86tools`, stage0 frå `bootstrap/`, eigen `build/`) har ikkje Chrome, så Chrome-stega er SKIP der. VM- og byggstega i dei valfrie testane køyrer. Alle testane er grøne på begge plattformene og i begge modusane.

## Gjenstår før W5

- **Nettlesarar:**
  - Skjema-demoen er køyrd i Chrome 154, men ikkje i JavaScriptCore (han treng ein DOM). JSC (macOS 26.6.2) er køyrd for `w4_ordbok` med eit scratch-skript utanfor repoet, med identisk utskrift.
  - Firefox er ikkje testa.
  - Safari kan ikkje automatiserast utan å slå på fjernstyring i innstillingane.
- **Lastaren:**
  - Heile produksjonstabellen er 2 550 av 2 560 byte. Nye vertsfunksjonar krev at noko blir kortare, eller eit nytt budsjett.
  - W0-settet er 543 byte, mot 400 i planen.
- **Hendingar og handtak:**
  - Hendingsdata kan berre lesast medan tilbakekallet køyrer. Ei nøsta hending (t.d. `test_klikk` i eit tilbakekall) skriv over `V` og `T`.
  - Eit handtak som blir brukt etter `slepp`, kan peike på eit anna element, fordi plassen blir gjenbrukt. Det finst ingen generasjonssjekk.
  - DOM-feil er JS-unnatak, ikkje Norscode-unnatak, og kan ikkje fangast med `prøv`/`fang`. Døme: `finn` som ikkje finn noko, og så `sett_tekst` på handtaket.
  - `hent_*` på eit element som ikkje finst, gjev ein JS-feil. Han gjev ikkje tom tekst.
- **Serve:**
  - Script-taggen utan versjon gjev éin 304 per sidevising. Ein app som vil unngå han, kan importere `klient_data` og bruke `script_tag_versjonert`, men då er han bunden til `build/` igjen.
  - `csp_wasm` bør flyttast til `std/http_cache.no` når PR #206 er fletta.
  - Modulen ligg som ein tekstliteral i NCB-en til serve-appen, så appen tek like mykje minne som modulen er stor.
- **Runtime:**
  - `tekst_til_liten`/`tekst_til_store` endrar berre ASCII (paritet med VM-en). Filteret i demoen finn difor ikkje «Åse» på «å».
  - VM-en sin `json_parse` (tekstverdiar) er framleis avvist.
  - `fjern_nokkel` med heiltalsnøklar gav SIGSEGV i VM-en (macOS-seeden) og er ikkje med i korpuset.
- **Byggjetid:**
  - Valideringa står for 1,4 av 3,4 s i eit bygg med varm cache (`rt_json`).
  - Atoma blir bygde på nytt i kvart bygg (0,3 s).
  - Cachefilene blir aldri rydda: ei ny fil per endring i backend-kjeldene.
- **Unnatak:** `bryt`/`fortsett` ut av `prøv` og `utsett` i lykkjer er avviste. Dei krev dynamisk try- og opprydjingsstakk, eller at kompilatoren emitterer `TRY_END` før hoppet (reseed).
- **Closures:**
  - Kall av ein fanga closure som `f(x)` inne i ein lambda krev ei endring i kompilatoren (reseed). Til då går det med `ncb_call_fn`.
  - Funksjonsnamn som verdi (`kart(l, dobbel)`) blir `LOAD_NAME` av eit ukjent namn, også i VM-en.
  - Eit kall frå ein lambda til ein funksjon i modulen blir `builtin.<namn>` i VM-en (sett i W4 i `tools/nc_wasm.no`). Difor les lowringa sjølv cachen, i staden for å få ein closure.
- **Ytelse (W10):**
  - Tekstfunksjonane lagar eitt utsnitt per byteposisjon.
  - Handteraren sjekkar typar med tekstsamanlikning.
  - `ncb_call_fn` samanliknar metodenamn lineært.
  - Ordbøker har lineære oppslag.
- **Validator:** subtyping mellom ulike typeindeksar og legacy-unnatak er ikkje støtta.
- **W5 (planen):** nettlesar-testbenk som verktøy. Chrome-hjelparane (`dump_med_konsoll`, testkrokar, `#nc-logg`) er byrjinga.
