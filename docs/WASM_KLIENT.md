# Klientlogikk i Norscode kompilert til WebAssembly (W0–W8)

Status: milepælane W0–W8 i WASM-sporet (app-kjensle del 2). Klientkode blir skriven i Norscode og kompilert til ein WasmGC-modul. Nettlesaren startar modulen med ein liten lastar som blir **emittert frå Norscode-data**. Det finst ingen handskriven JavaScript i repoet.

- **W0** gav vegen frå kjelde til nettlesar: heiltal, kontrollflyt, kall og DOM-vertsfunksjonar.
- **W1** gav verdimodellen: tekst, desimaltal, lister og ordbøker, med same semantikk som VM-en. Eit paritetskorpus køyrer kvart program i VM-en og i Chrome og krev identisk utskrift.
- **W2** gav unnatak (`prøv`/`fang`/`endeleg`, `utsett`, typa `fang`, innebygde feil som fangbare unnatak), closures og funksjonsverdiar, strukturmetodar, og ein stakk- og typesjekk i validatoren.
- **W3** gav runtime-biblioteket `std/wasm_rt.no`: tekstfunksjonar, JSON, sortering og SHA-256 skrivne i vanleg Norscode og kompilerte av same backend, med tree-shaking. Vertsfunksjonane tek tekstverdiar.
- **W4** gav vertsfunksjonane for ei ekte interaktiv side (tekst frå DOM-en, attributt og klassar, hendingar med data og delegering, tidtakarar, navigasjon, lokal lagring, handtakstabell med `slepp`), atoma `fjern_nokkel` og `tid_ms`, ein cache for lowra runtime-funksjonar, serve-integrasjonen `std/wasm_serve.no` (appen importerer ikkje lenger noko under `build/`), og demoen `examples/wasm_skjema/`.
- **W5** gav nettlesar-benken `std/wasm_nettlesar.no` og CLI-en `tools/wasm_nettlesar.no`: Chrome og JavaScriptCore frå Norscode, rydding av berre eigne prosessar, app-kjensle målt i WASM-sida sjølv og sidelastingstid frå Chrome-netloggen (sjå «Nettlesar-benken»).
- **W6** gav asynk nett via tilbakekall: `std/wasm_nett.no` (fetch mot same opphav, JSON, status og headerar, tidsgrense og avbryting, tilbakekall i sende-rekkjefølgje) og demoen `examples/wasm_deltakarar/` (sjå «Nett»).
- **W7** gav lokal lagring i IndexedDB: `std/wasm_lager.no` (database med oppgraderingsskjema, put/hent/slett/liste/tel, område- og indeksspørjingar, éin transaksjon per batch, tilbakekall i rekkjefølgje, LagerFeil, database per brukar og sletting ved utlogging), og ein lokal kopi i deltakar-demoen som blir vist straks ved omlasting (sjå «Lokal lagring»).
- **W8** gav ein installerbar app som startar frå cache og verkar utan nett: web-app-manifest og ikon (PNG laga i Norscode) frå `std/wasm_pwa.no`, og ein service worker der logikken er Norscode kompilert til WebAssembly (`std/wasm_sw.no`) og `sw.js` er ein generert lastar; precache, strategiar per rute, aldri cache av private svar, versjon over alle precache-kroppar, oppdateringsflyt med ventande versjon og klikk, og CSP-kravet for workeren (sjå «Installerbar app og service worker»).

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
| `sw.wasm`, `sw_lastar.js` | (med `NC_WASM_SW=1`, W8) service workeren og SW-lastaren; `klient_data.no` får `sw_wasm()`, `sw_versjon()` og `sw_lastar()`, og `serve.no` rutene til workeren, manifestet, ikona og offline-sida |

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
| `std/wasm_nett.no` | Nett for klientmodular: fetch mot same opphav med tilbakekall, kø i sende-rekkjefølgje, feilklassar (W6) |
| `std/wasm_lager.no` | Lokal lagring i IndexedDB: skjema, op-ar, batch som éin transaksjon, kø i rekkjefølgje, LagerFeil, personvern (W7) |
| `tools/nc_wasm.no` | CLI |
| `std/wasm_nettlesar.no` | Nettlesar-benken: Chrome, jsc, serve, rydding, logg, mål og netlogg (W5; berre testar og verktøy) |
| `std/wasm_jsc.no` | Vertsshimen for `jsc -e` som JS-tre (W5) |
| `tools/wasm_nettlesar.no` | CLI for benken: `royk`, `korpus`, `maal`, `offline` (W5) |
| `std/wasm_sw.no` | Service workeren i Norscode (blir `sw.wasm`, W8) |
| `std/wasm_pwa.no` | PWA på tenaren: manifest, ikon, offline-side, versjon, `sw.js` og svara (W8) |
| `std/wasm_pwa_klient.no` | Service worker frå sida: registrering og oppdateringsflyt (W8) |
| `std/png_enkel.no` | PNG med palett og lagra deflate-blokker, og ein validator (W8) |

Ingen av dei ligg i `nc_main`-lukkinga, så dei krev ingen reseed. `std/wasm_rt.no` importerer `std/sha256.no` (som ligg i lukkinga), men endrar han ikkje. `std/wasm_serve.no` importerer `std/web.no`, som heller ikkje ligg i lukkinga. `std/wasm_nettlesar.no` brukar `std/prosess.no` og prosess-ABI-en (som testane i W0–W4).

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
   - **Nett (W6):** `bruk std.wasm_nett som nett`, så `nett.hent_json("/api/x", fun(svar) -> vis(svar["json"]), fun(feil) -> vis_feil(feil))`. Tilbakekalla til nett er closures med éin parameter (ikkje funksjonsnamn), og dei kjem i den rekkjefølgja førespurnadene vart sende. Sjå «Nett (W6)» og `examples/wasm_deltakarar/`.
   - **Lokal lagring (W7):** `bruk std.wasm_lager som lager`, så `la db = lager.opne(lager.brukar_db_namn("nc-app", uid), 1, skjema, fun(d) -> klar(), fun(f) -> vis_feil(f))` og `lager.køyr(db, [lager.op_put("dok", v), …], fun(r) -> …, fun(f) -> …)`. Same tilbakekall-API som nett. Sjå «Lokal lagring (W7)».
2. **Appen** (`app.no`): `bruk std.wasm_serve som ws`. WASM-sida blir levert med `ws.side(html)` (CSP med `'wasm-unsafe-eval'`), og HTML-en har `ws.script_tag()` i `<head>`. Alle andre sider brukar `ws.vanleg_side(html)`. Appen importerer ingenting under `build/`, og sida må fungere (utan klientlogikk) når WASM manglar.
3. **Bygg** med `NC_WASM_KJELDE`, `NC_WASM_APP` og `NC_WASM_UT` (sjå over), og **server** `build/wasm/<app>/serve.no`.
4. **Test**: bygg ein testklient med `NC_WASM_TESTBYGG=1` som importerer klienten, kallar `start()` og køyrer brukarsteg med testkrokane (`test_skriv`, `test_send`, `test_klikk`, `test_tast`). `skriv(...)` går då til `<pre id="nc-logg">`, som headless Chrome les (sjå `tests/fixtures/wasm_skjema_testklient.no`).
   Nettlesar-benken (W5) gjer resten: `bruk std.wasm_nettlesar som benk`, `benk.bygg`, `benk.ny_økt`, `benk.start_serve`, `benk.last_side` (gjev loggen, DOM-en, «Uncaught» og mål) og `benk.rydd`. Eller frå kommandolinja: `NC_NETTLESAR_KJELDE=… NC_NETTLESAR_APP=… NC_NETTLESAR_KREV=… ./bin/nc run tools/wasm_nettlesar.no`. Asynkrone steg (nett) kan kjedast med closures som klienten kallar når sida er oppdatert (sjå `tests/fixtures/wasm_deltakarar_testklient.no`).

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

**Nett (W6)** — brukte av `std/wasm_nett.no`, ikkje direkte av klientkoden:

| Funksjon | JS-operasjon |
|---|---|
| `hent_start(førespurnad, "funksjon")` → handtak | `Q(S(…), x[…])`: `fetch` med førespurnaden (JSON), mode og credentials `same-origin`, `AbortSignal.any([k.signal, AbortSignal.timeout(t)])`; handtaket er AbortController-en `k` (`slepp` avbryt) |
| `hent_svar()` → tekst | `R`: konvolutten til det siste svaret, `id\nstatus\nheaderar-JSON\nkropp`, eller `id\n0\nfeilnamn\n` |

**Lokal lagring (W7)** — brukte av `std/wasm_lager.no`, ikkje direkte av klientkoden:

| Funksjon | JS-operasjon |
|---|---|
| `lager_start(id, førespurnad, "funksjon")` | `I(id, S(…), x[…])`: éin IndexedDB-førespurnad (opne/oppgradere, ein batch som éin transaksjon, lukke eller slette databasen), styrt av JSON-en frå Norscode; tilbakekallet blir kalla med konvolutten i `J` |
| `lager_svar()` → tekst | `J`: `JSON.stringify([id, feilnamn, melding, data])` |
| `fjern_lokalt(nøkkel)` | `localStorage.removeItem` (utlogging) |

**Service worker (W8)** — sida brukar dei gjennom `std/wasm_pwa_klient.no`, workeren gjennom `std/wasm_sw.no` (berre i SW-lastaren):

| Funksjon | JS-operasjon |
|---|---|
| `sw_registrer(url, "funksjon")` | `G`: `navigator.serviceWorker.register(url)`, `update()`, lyttarar på `updatefound`/`statechange`, `controllerchange` og `ready`; hendinga i `O` |
| `sw_status()` → tekst | `O + " " + [kontrollert, installerer, ventar, aktiv]` som 0/1 |
| `sw_send(melding)` | `(B.waiting || B.installing).postMessage(melding)`: den ventande workeren, eller den som nettopp er installert (i `statechange` er `B.waiting` ikkje alltid oppdatert enno, målt) |
| `sw_lytt("hending", "funksjon")` | (SW) `A[hending] = x[funksjon]` |
| `sw_inn()` → tekst | (SW) dataa til hendinga (`D`; før den første: konfigurasjonen `C`) |
| `sw_ut(plan)` | (SW) planen (`Y`) |

W8-testkrokar: `test_cachar("funksjon")` (Cache Storage som JSON `[[namn, [url, …]], …]`), `test_bilete(url, "funksjon")` (`Image.decode` → «breiddxhøgd» eller «feil») og `test_resultat()` (svaret, `M`).

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
| `Q`, `R` (W6) | fetch og konvolutten til det siste svaret (berre når modulen importerer `hent_start`/`hent_svar`) |
| `I`, `J` (W7) | IndexedDB og konvolutten til det siste svaret (berre når modulen importerer `lager_start`/`lager_svar`) |
| `G`, `B`, `O` (W8) | Registreringa av service workeren, registreringa og den siste hendinga (berre med PWA-importane) |
| `M` (W8) | Svaret frå testkrokane `test_cachar` og `test_bilete` (berre testbygg) |

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
| Heile produksjonstabellen utan nett (37 vertsfunksjonar: 33 for klientkoden og 4 interne, utan testkrokane) | 2 550 byte (uendra) | 2 560 byte (planbudsjettet, uendra) |
| Nett-gruppa (W6: `hent_start`, `hent_svar`, hjelparane `Q` og `R`) | 405 byte | 448 byte (eige budsjett) |
| Heile produksjonstabellen med nett (39) | 2 955 byte | 3 008 byte (2 560 + 448) |
| Lagrings-gruppa (W7: `lager_start`, `lager_svar`, `fjern_lokalt`, hjelparane `I` og `J`) | 1 256 byte | 1 280 byte (eige budsjett) |
| Heile produksjonstabellen med nett og lagring (42) | 4 211 byte | 4 288 byte (2 560 + 448 + 1 280) |
| Heile tabellen med testkrokane, nett og lagring (47) | 4 523 byte | 4 800 byte (W6: 3 520) |
| PWA-gruppa (W8: `sw_registrer`, `sw_status`, `sw_send`, `G`, `B`, `O`) | 706 byte | 736 byte (eige budsjett) |
| Heile produksjonstabellen med nett, lagring og PWA (W8) | 4 917 byte | 5 024 byte |
| Heile tabellen med testkrokane (W8, med PWA og `test_cachar`/`test_bilete`/`test_resultat`) | 5 585 byte | 5 920 byte (3 520 + 1 280 + 736 + 384) |
| SW-lastaren (W8, `sw.js` utan konfigurasjonen) | 1 751 byte | 1 792 byte (eige tak) |
| Deltakar-demoen (W8, med PWA) | 4 236 byte | 4 576 byte (2 560 + 1 280 + 736) |
| Skjema-demoen (22 importar) | 1 755 byte | 2 048 byte |
| Deltakar-demoen (W7, 27 importar med nett og lagring) | 3 402 byte (W6: 2 027) | 3 840 byte (2 560 + 1 280, `test_wasm_w6` og `test_wasm_w7_demo`) |
| W4-proben (7 importar: `finn`, `sett_tekst`, `hent_verdi`, `deleger`, `hending_data`, `lagre_lokalt`, `nå_ms`) | 994 byte | |

Planen hadde 400 byte for W0-settet og 1 KB for eit typisk øy-program. Teljaren er framleis 543 byte (`H` og instansieringa åleine er om lag 300), og eit lite interaktivt program med lyttarar er rundt 1 KB. Skjema-demoen er større fordi han brukar 22 vertsfunksjonar og alle hjelparane (`L` åleine er om lag 300 byte).

**Budsjettet for nett (W6).** Produksjonstabellen var 2 550 av 2 560 byte etter W4, så nett-gruppa fekk plass berre ved at noko anna vart kortare eller at taket vart justert. Grunngjevinga for eit eige budsjett i staden for å heve 2 560: tree-shakinga gjer at gruppa berre kostar modular som brukar `std/wasm_nett.no` (teljaren og skjema-demoen er uendra), og `Q` er éin fetch-kjede der det meste er påkravd API (`AbortSignal.any`/`timeout`, `Object.assign` for mode og credentials, `Object.fromEntries` for headerane). Planutkastet sitt `hent` (2 311 byte for heile settet) hadde verken tidsgrense, avbryting, headerar eller feilnamn. Den einaste innsparinga som vart gjord, er at `js_streng` skriv linjeskift som `\n` i staden for `\u000a`. `test_wasm_lastar` handhevar begge taka: resten ≤ 2 560 og nett-gruppa ≤ 448.

**Budsjettet for lagring (W7).** Lagrings-gruppa har eige budsjett av same grunn: tree-shakinga gjer at berre modular som brukar `std/wasm_lager.no` betaler (teljaren, skjema-demoen og nett-klientane er byte-identiske med W6). Gruppa er 1 256 byte fordi IndexedDB-API-et er ordrikt: sju hendingsfelt (`onupgradeneeded`, `onsuccess`, `onerror`, `onblocked`, `oncomplete`, `onabort`, `onversionchange`), `objectStoreNames`/`indexNames`, `createObjectStore`/`createIndex`/`deleteObjectStore`/`deleteIndex` og `IDBKeyRange`. Alt anna er i Norscode: skjemaet kjem som data, og kvar op er eit metodenamn og ein argumentliste frå `std/wasm_lager.no` (`x[o.m].apply(x,o.a)`), så `I` har ingen greiner per op. Der var to val som kosta: `onversionchange` (ein gamal fane blokkerer ikkje oppgraderingar) og fjerning av indeksar og lager ved oppgradering (om lag 140 byte til saman). `test_wasm_lastar` handhevar taket (≤ 1 280) og at gruppa berre kjem med lagrings-importane.

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

## Nett (W6): `std/wasm_nett.no`

Asynkrone API-ar er callback-baserte (planen §3.1). Klientkoden sender ein førespurnad og får nøyaktig eitt tilbakekall seinare. Tilbakekalla er **closures med éin parameter** (`fun(svar) -> …`), så dei kan fange tilstand og kalle funksjonar i modulen. All logikk ligg i Norscode; lastaren gjer berre fetch.

```
bruk std.wasm_nett som nett

nett.hent_json("/api/deltakarar", fun(s) -> vis(s["json"]), fun(f) -> vis_feil(f))
nett.send_json("POST", "/api/deltakarar", {"deltakarar": liste}, fun(s) -> lagra(s), fun(f) -> vis_feil(f))
la f = nett.med_header(nett.med_tidsgrense(nett.førespurnad("GET", "/api/treg"), 2000), "X-Nc", "1")
la id = nett.send(f, fun(s) -> ok(s), null)
nett.avbryt(id)
```

| Funksjon | |
|---|---|
| `førespurnad(metode, url)` | `{"metode", "url", "headerar": {}, "kropp": null, "tidsgrense_ms": 8000, "json": usann}` |
| `med_header(f, namn, verdi)`, `med_tekst(f, kropp, type)`, `med_json(f, verdi)`, `forvent_json(f)`, `med_tidsgrense(f, ms)` | Byggjarar (headernamn med små bokstavar; `med_json` = `json_stringify` + `Content-Type`/`Accept` + forventa JSON-svar) |
| `send(f, ok, feil)` → id | Validerer og sender. Feil argument kastar `HentFeil: ugyldig førespurnad: …` med ein gong |
| `hent(url, ok, feil)`, `hent_json(url, ok, feil)`, `send_json(metode, url, verdi, ok, feil)` | Snarvegar |
| `avbryt(id)` → bool | Avbryt ein førespurnad som ikkje er ferdig (usann når id-en er ukjend eller alt ferdig) |
| `ventande()` | Førespurnader som ikkje er leverte |
| `valider(f)`, `førespurnad_json(id, f)`, `tolk_konvolutt(k)`, `resultat(f, k)`, `feil_tekst(feil)`, `kast_feil(feil)` | Reine funksjonar (paritetskorpuset `w6_nett`) |
| `test_registrer(f, ok, feil)`, `test_motta(konvolutt)` | Berre for testar: køa utan nettverk (korpuset `w6_nett_ko`) |

**Svar og feil.** `ok` får `{"id", "status", "ok": sann, "kropp", "headerar"}` (headernamn med små bokstavar, `Object.fromEntries(r.headers)`) og `"json"` når JSON var forventa. `feil` får `{"type": "HentFeil", "art", "id", "metode", "url", "status", "melding", "kropp", "headerar"}`:

| `art` | Når | `status` |
|---|---|---|
| `status` | svar utanfor 2xx (kroppen er med, t.d. `{"feil": […]}` frå tenaren) | 404, 500, … |
| `nettverk` | ingen kontakt (tenaren nede, offline, avvist av nettlesaren; `TypeError`) | 0 |
| `tidsavbrot` | tidsgrensa gjekk ut (`TimeoutError`) | 0 |
| `avbrote` | `avbryt(id)` (`AbortError`) | 0 |
| `json` | 2xx, men kroppen var ikkje gyldig JSON (`rt.json_parse`, melding som `JSON: uventa teikn: i`) | 2xx |

`type` gjer at `kast feil` blir fanga av `fang (e: HentFeil)`. `kast_feil(feil)` kastar `feil_tekst(feil)` («HentFeil: status GET /api/404: status 404»), som òg blir fanga slik. Utan feil-tilbakekall (`null`) blir feilen skriven som `ERROR: HentFeil: …`.

**Garantiar**
- **Same opphav:** URL-en må vere ein sti (`/…`, ikkje `//…`, utan mellomrom og linjeskift). Lastaren set `mode` og `credentials` til `same-origin` *etter* førespurnaden frå Norscode (`Object.assign`), så dei kan ikkje overstyrast. `test_wasm_lastar` sjekkar det.
- **Rekkjefølgje:** tilbakekalla kjem i den rekkjefølgja førespurnadene vart sende, same kva rekkjefølgje svara kjem i. Køa ligg i Norscode (`_kø`, `_ventar`): eit svar som kjem før eit tidlegare, ventar til det tidlegare er levert. Ein tidsavbroten eller avbroten førespurnad får feil-tilbakekallet på sin plass i køa.
- **Eitt tilbakekall per førespurnad**, og eit tilbakekall som kastar, gjev `ERROR: <tekst>` (som eit uhandtert unnatak) utan at dei neste i køa blir stoppa. Ingenting når JavaScript, så konsollen er fri for «Uncaught».
- **Tidsgrense** 8 000 ms som standard (A2), 1–600 000 ms. **Avbryting** via handtaket: `avbryt` kallar `slepp`, som kallar `abort()` på AbortController-en.
- **JSON som VM-en:** førespurnaden er `json_stringify` (same tekst som i VM-en), og svaret blir lese med `rt.json_parse` (same verdiar som `json_parse_raw` i VM-en for gyldig JSON, og kastar for ugyldig). Paritetskorpuset køyrer same kode i VM-en, Chrome og jsc.

**Konvolutten.** Lastaren kallar tilbakekallet `__hent_ferdig` (eksportert frå `std.wasm_nett`) når eit svar er klart, og `hent_svar()` gjev `id\nstatus\nheaderar-JSON\nkropp` (feil: `id\n0\nfeilnamn\n`). `tolk_konvolutt` deler på dei tre første linjeskifta, så kroppen kan innehalde linjeskift.

**Nettlesarstøtte.** `AbortSignal.any` finst frå Chrome 116, Firefox 124 og Safari 17.4 [A]. For modular med unnatak (alle som brukar `std/wasm_nett.no`) er golvet framleis exnref (Chrome 137, Firefox 131, Safari 18.4), så Firefox 131 og nyare, der begge finst, er det reelle golvet. Chrome 154 er testa [V].

**`nc serve` og parallelle førespurnader.** `nc serve` tek éi tilkopling om gongen. Chrome opnar opptil seks tilkoplingar per opphav, og ein førespurnad som blir avbroten (eller får tidsavbrot) medan han ventar på ei tilkopling, kan etterlate ein open sokkel utan førespurnad. Då ventar `nc serve` på han, og alt anna står [V: med ni parallelle førespurnader og eitt avbrot hekk 4 av 10 køyringar; med seks om gongen og avbrot etter 20 ms 0 av 8 (og alle testkøyringane sidan). Eit tidsavbrot i ein andre bolk, etter at tilkoplingane frå den første var brukte, hekk 1 av 2]. Testklienten held seg difor til seks om gongen og avbryt først når førespurnaden er skriven. Ein apptenar med fleire tilkoplingar (A6/R1) fjernar problemet.

**Funn: headernamn under `nc serve`.** `nc serve` gjev headerane med namna slik klienten skreiv dei, og `web.request_header` samanliknar eksakt. `fetch` skriv namna med små bokstavar, men nettlesaren sender `Cookie` med stor forbokstav, så `web.request_cookie(ctx, …)` finn han ikkje. Demoen les headerane uavhengig av store og små bokstavar (`app.header`). `std/web.no` er ikkje endra.

### Deltakar-demoen

`examples/wasm_deltakarar/` er ei side der lista blir henta frå og lagra til tenaren via JSON-ruta `/api/deltakarar`:
- Ved start: `hent_json`, med «Hentar deltakarlista …» i statusfeltet, knappane av og `aria-busy="true"` til svaret kjem.
- «Legg til» validerer med `reglar.no` (dei same reglane som tenaren brukar) og legg til lokalt; «Ulagra endringar» blir synleg. «Fjern» er delegert.
- «Lagre på tenaren» sender heile lista (`send_json("POST", …)`). Tenaren parsar med `rt.json_parse` og validerer med `reglar.valider_liste` på nytt; 400 med `{"feil": […]}` blir vist i feilfeltet (`role="alert"`).
- «Hent på nytt», og feilmeldingar for nettverk, tidsavbrot og status (`klient.feilmelding`, ein rein funksjon som testane køyrer i VM-en).
- Lagring: `nc serve` har korkje disk eller minne mellom førespurnader, så tenaren lagrar lista i ein HttpOnly-informasjonskapsel (`SameSite=Strict`, prosentkoda JSON). API-et til klienten er det same som mot ein database.

**Lokal kopi (W7).** Av/på-knappen «Hugs lista på denne eininga» (`aria-pressed`) lagrar valet i `localStorage` og lista i IndexedDB (databasen `nc-deltakarar-<sha256("demo")[0:16]>`; ein app med innlogging brukar uid-en), i éin transaksjon kvar gong lista endrar seg: tøm, alle radene (nøkkelen er plassen i lista) og tilstanden (`ulagra`). Ved neste lasting blir kopien lesen medan tenaren blir spurd, og vist straks med statusen «Viser lagra kopi frå eininga (N deltakarar) – hentar frå tenaren …». Svaret frå tenaren ventar til kopien er lesen, så rekkjefølgja er fast. Når tenaren svarar, vinn tenarlista, unnateke når kopien har ulagra endringar: dei blir ståande, og statusen seier «Tenaren har N deltakarar. Du har ulagra endringar frå førre gong: lagre dei eller hent på nytt.» Eit nytt trykk på knappen slettar databasen og valet (`lager.logg_ut`). `#lokal` (`role="status"`) viser tilstanden til kopien, og feil i den lokale lagringa får eigne meldingar (`klient.lokal_feilmelding`).

Resultat i Chrome 154 (`tests/test_wasm_w7_demo.no`, testbygg, same profil): i lasting 1 kom lista frå tenaren, knappen lagra ho lokalt, og «Ny Person» vart lagd til og Berit fjerna (ulagra). I lasting 2 (tenaren svarar etter 800 ms) vart dei 10 radene frå eininga (siste «Ny Person») viste før tenarsvaret, dei ulagra endringane vart ståande då tenaren svara med standardlista, «Lagre» lagra dei, og knappen sletta kopien. I lasting 3 var valet borte, databasen ny og tom, og lista kom frå tenaren. Ein fersk profil i lasting 2 viste ingen lokal kopi, berre standardlista (negativ kontroll). Ingen «Uncaught».

Resultat i Chrome 154 (testbygg, `tests/test_wasm_w6_chrome.no`): lasta 10, la til 1, lagra 11, henta dei 11 att frå tenaren, fjerna 1, ei ugyldig liste gav «Kunne ikkje lagre: Deltakar 1: Namnet må ha minst 2 teikn. Deltakar 1: E-postadressa manglar namn før @.», og etter `/api/stopp` gav «Hent på nytt» «Kunne ikkje hente lista: fekk ikkje kontakt med tenaren.» med lista ståande. Ingen «Uncaught». I produksjonsbygg viser `/` dei 10 frå tenaren når sida er ferdig.

## Lokal lagring (W7): `std/wasm_lager.no`

Klientkoden lagrar data på eininga i IndexedDB med same tilbakekall-API som nett: kvar førespurnad får nøyaktig eitt tilbakekall seinare (ein closure med éin parameter), og feil er ei fangbar `LagerFeil`-ordbok. All logikk ligg i Norscode; lastaren har hjelparen `I`, som berre gjer IndexedDB-kalla som førespurnaden frå Norscode beskriv.

```
bruk std.wasm_lager som lager

la skjema = [lager.lager("dok", "id", [lager.indeks("type", "type", usann), lager.indeks("epost", "epost", sann)]),
             lager.lager_auto("logg", "nr", []),
             lager.lager("meta", "", [])]
la db = lager.opne(lager.brukar_db_namn("nc-app", uid), 1, skjema, fun(d) -> klar(d), fun(f) -> vis_feil(f))
lager.put(db, "dok", {"id": 7, "type": "brev", "epost": "a@b.no"}, fun(k) -> 0, fun(f) -> vis_feil(f))
lager.indeks_liste(db, "dok", "type", lager.lik("brev"), fun(l) -> vis(l), fun(f) -> vis_feil(f))
lager.køyr(db, [lager.op_tøm("dok"), lager.op_put("dok", a), lager.op_put("dok", b)], fun(r) -> 0, fun(f) -> vis_feil(f))
lager.logg_ut(db, lager.brukar_db_namn("nc-app", uid), ["nc-app-val"], fun(x) -> ut(), fun(f) -> vis_feil(f))
```

| Funksjon | |
|---|---|
| `lager(namn, nøkkelsti, indeksar)`, `lager_auto(…)`, `fjerna(namn)`, `indeks(namn, sti, unik)` | Skjemaet. `nøkkelsti` er feltet i verdien som er nøkkelen (`""`: nøkkelen blir gjeven med `put_med_nøkkel`); `lager_auto` lagar nøkkelen (1, 2, …) når verdien manglar feltet; `sti` kan vere nøsta (`"adresse.post"`) |
| `opne(namn, versjon, skjema, ok, feil)` → db | Opnar og oppgraderer. Gjev databasen med ein gong; alt som blir gjort før opninga er ferdig, ventar på ho. `ok(db)`, der `db["oppgradert_frå"]` er versjonen før oppgraderinga (0 for ein ny database) eller `null` |
| `put`, `put_med_nøkkel`, `hent`, `slett` (nøkkel eller område), `liste` (område), `tel`, `indeks_liste`, `indeks_hent` | Éin op i éin transaksjon; `ok` får resultatet |
| `køyr(db, ops, ok, feil)` | Ein batch (`op_put`, `op_put_med_nøkkel`, `op_hent`, `op_slett`, `op_liste` og `op_nøklar` med grense, `op_tel`, `op_tøm`, `op_indeks_liste`, `op_indeks_tel`, `op_indeks_hent`) som **éin** transaksjon, readwrite når ein op skriv, elles readonly. `ok` får éin verdi per op: put → nøkkelen, hent → verdien eller `null`, liste → verdiane, nøklar → nøklane, tel → talet, slett og tøm → `null` |
| `lik(k)`, `frå(a, open)`, `opp_til(b, open)`, `mellom(a, b, a_open, b_open)`, `prefiks(p)` | Område (`IDBKeyRange`); `null` er alt. Namnet `til` er eit nøkkelord i Norscode |
| `lukk(db)`, `slett_database(namn, ok, feil)`, `logg_ut(db, namn, lokale_nøklar, ok, feil)` | Lukk, slett, og utlogging (lukk, fjern nøklane i localStorage, slett databasen) |
| `brukar_db_namn(prefiks, uid)` | `prefiks-<16 hex av sha256(uid)>` |
| `ventande()` | Førespurnader som ikkje er leverte |
| `valider_skjema`, `skjema_json`, `er_nøkkel`, `op_js`, `batch_json`, `json_trygg`, `tolk_konvolutt`, `feil_art`, `batch_resultat`, `feil_tekst`, `kast_feil` | Reine funksjonar (paritetskorpuset `w7_lager`) |
| `test_db`, `test_registrer`, `test_motta` | Berre for testar: køa utan IndexedDB (korpuset `w7_lager_ko`) |

**Val: verdiar som JSON-tekst.** Ein verdi blir lagra som posten `{k: nøkkel, v: json_stringify(verdi), i: {indeks: verdi}}`, og lesen med `rt.json_parse`. Grunngjeving:
- **Paritet:** same verdi kjem attende som i VM-en. Ein strukturert kopi (JS-objekt) ville gjort `2.0` til `2` (heltall), heiltal over 2⁵³ unøyaktige (`9007199254740993` → `…992`), og `JSON.parse` avviser kontrollteikn i strengar. Testen lagrar alle desse og får dei attende byte-like.
- **Ingen omvending:** lastaren ser berre tekst (førespurnaden inn, konvolutten ut), så `I` treng inga kopiering av objekt, og verdien blir ikkje omsett frå Norscode til JS og attende.
- **Storleik:** JSON-teksten er om lag like stor som ein strukturert kopi på disk (IndexedDB lagrar begge serialiserte). Indeksverdiane ligg ein gong til i `i`.
- Nøklar og indeksverdiar må vere JS-verdiar, sidan IndexedDB sorterer dei. Dei blir henta ut av verdien i Norscode (nøkkelstien og indeksstiane), så lastaren har faste stiar: `keyPath: "k"` og `"i.<indeks>"`.

**Nøklar.** `heltall` (|k| ≤ 2⁵³ − 1), endeleg `desimaltall` eller `tekst`; anna gjev `LagerFeil` «data» med ein gong. Tal sorterer før tekst og etter verdi (`1` og `1.0` er same nøkkel), tekst etter UTF-16-kodeeiningar (bytevis for tekst utan teikn over U+FFFF). Ein indeksverdi som ikkje er ein gyldig nøkkel (eller manglar), blir ikkje indeksert, som i IndexedDB.

**Skjema og oppgradering.** Ved ein høgare versjon lagar `I` lager og indeksar som manglar, fjernar indeksar som ikkje lenger står i skjemaet, og fjernar lager berre når skjemaet seier det (`lager.fjerna("namn")`), så data ikkje forsvinn fordi eit lager vart gløymt i skjemaet. Å endre nøkkelstien, `auto` eller `unik` på noko som finst, krev eit nytt namn. Namn på lager og indeksar er ASCII-bokstavar, siffer og `_`.

**Rekkjefølgje og konsistens.**
- Tilbakekalla kjem i den rekkjefølgja førespurnadene vart gjorde (køa ligg i Norscode, som i nett), også når IndexedDB svarar i ei anna rekkjefølgje (readonly-transaksjonar kan bli ferdige i vilkårleg rekkjefølgje).
- Eit tilbakekall som kastar, gjev `ERROR: <tekst>` og stoppar ikkje dei neste. Utan feil-tilbakekall blir feilen skriven som `ERROR: LagerFeil: …`. Ingenting når JavaScript som «Uncaught» (konvolutten blir laga i `Promise.then(…).catch(…)`).
- Ein batch er éin transaksjon: feilar éin op (unik indeks brote, ukjent lager eller indeks, lukka database), blir alt i batchen rulla tilbake. Eit unnatak i `I` (t.d. `NotFoundError` frå `transaction()`) avbryt transaksjonen og svarar éin gong (onabort blir fjerna først).
- Ei opning som blir levert som «blokkert» og som IndexedDB fullfører seinare, blir lukka straks.

**Feil** (`{"type": "LagerFeil", "art", "id", "op", "database", "namn", "melding"}`; `fang (e: LagerFeil)` fangar både ordboka og `kast_feil(f)`):

| `art` | Når |
|---|---|
| `open` | Databasen kunne ikkje opnast (ingen IndexedDB, avslått lagring, anna opningsfeil); køyr som venta på opninga, får same art |
| `blokkert` | Ein annan fane med eldre kode held databasen open (`blocked`). Tilkoplingar frå `std/wasm_lager.no` lukkar seg sjølve ved `versionchange`, så dette skjer ikkje mellom faner med same kode |
| `kvote` | `QuotaExceededError` |
| `transaksjon` | Transaksjonen feila og vart rulla tilbake (`ConstraintError`, `NotFoundError`, `InvalidStateError`, `AbortError`, …) |
| `versjon` | Databasen på eininga har ein høgare versjon (`VersionError`) |
| `data` | Ugyldig nøkkel, verdi, område, namn eller skjema (kasta med ein gong, før noko blir sendt), `DataError`, eller ein lagra verdi som ikkje er gyldig JSON |

**Personvern** (frå app-planen):
- Innlogga data skal ikkje liggje att på eininga utan at brukaren har valt det. `std/wasm_lager.no` lagrar ingenting av seg sjølv; appen spør (demoen har ein av/på-knapp, og valet er det einaste i `localStorage`).
- Éin database per brukar: `brukar_db_namn(prefiks, uid)` = `prefiks-` + 16 hex-teikn av sha256(uid), så uid-en ikkje står i klartekst i lista over databasar, og ein annan brukar på same eining ser ein annan (tom) database.
- Utlogging: `logg_ut(db, namn, lokale_nøklar, …)` lukkar databasen, fjernar nøklane i `localStorage` og slettar databasen. Testen lastar sida på nytt med same profil og finn ein ny, tom database.

**Konvolutten.** `I` kallar tilbakekallet `__lager_ferdig` (eksportert frå `std.wasm_lager`), og `lager_svar()` gjev `JSON.stringify([id, feilnamn, melding, data])`: for opning `[handtak, gamal versjon]`, for ein batch resultata per op (postar `{k, v, i}`, nøklar eller tal). Førespurnaden er JSON utan rå kontrollteikn (`json_trygg`: `json_stringify` escapar berre `\n`, `\r` og `\t`, og `JSON.parse` avviser dei andre inne i strengar).

**Chrome og virtuell tid.** Med `--virtual-time-budget` går den virtuelle tida vidare når sida er ledig, og ho står berre stille medan ein førespurnad er på nettverket. IndexedDB-svar tel ikkje, så Chrome dumpa DOM-en før opninga var ferdig [V: 0 av 3 lastingar kom forbi `indexedDB.open`, med og utan virtuell tid]. Testklientane held difor ein «puls» i gang (`nett.hent("/api/puls")` om att og om att til dei er ferdige); då kom alle IndexedDB-svara (100 put i éin transaksjon på om lag 100 ms) [V]. Pulsen er berre i testklientane, ikkje i `std/wasm_lager.no`.

**Nettlesarstøtte.** IndexedDB 2.0 (`getAll`, `getAllKeys`) finst i Chrome 58, Firefox 51 og Safari 10.1 [A], så golvet er framleis exnref/WasmGC. Chrome 154 er testa [V]. JavaScriptCore-skalet (`jsc`) har ikkje IndexedDB: ende-til-ende-testane er SKIP der, og `tests/fixtures/wasm_w7_utan_idb.no` viser at opninga og alt som venta på ho, får `LagerFeil` «open» i rekkjefølgje utan «Uncaught» (same veg som ein nettlesar der lagring er slått av).

**Funn: rt-cachen og ncb_call_fn.** Analysecachen frå W4 tok opp verknadene til ein funksjon når setta av closures, dispatcharar og vertskall var like store før og etter analysen. Var dispatcharen for `ncb_call_fn` med same aritet alt registrert av ein annan funksjon, vart verknadene tekne opp utan han, og eit anna program som henta analysen av `std.wasm_lager._kall` frå cachen, fekk ein modul som validatoren avviste (`venta i32, fekk (ref null eq)`). Lowringa merkjer no opptaket som ureint når analysen rører closures, `CALL_VALUE`, `ncb_call_fn` eller vertsfunksjonar (`_opptak_urein`). `test_wasm_w7` byggjer to program etter kvarandre med ein fersk, delt cache og krev at modulen er byte-lik den utan cache.

## Installerbar app og service worker (W8)

Appen kan installerast, startar frå cache og verkar utan nett. Service worker-logikken er Norscode kompilert til WebAssembly (`sw.wasm`), og `sw.js` er ein **generert lastar**, emittert av `std/wasm_js.no` frå same vertstabell som sidelastaren. Det finst framleis ingen handskriven JavaScript.

| Fil | Rolle |
|---|---|
| `std/wasm_sw.no` | Service workeren i Norscode (blir `sw.wasm`): konfigurasjonen, planane for install, activate, message og fetch, og regelen for kva svar som kan lagrast |
| `std/wasm_pwa.no` | Tenarsida: konfigurasjon som Norscode-data, validering, manifest, ikon, offline-side, head-taggar, versjon, `sw.js` og svara, Clear-Site-Data ved utlogging, kill switch |
| `std/wasm_pwa_klient.no` | Sida: registrering, hendingar (`registrert`, `klar`, `ventar`, `aktivert`, `feil …`, `ustøtta`), `aktiver_ny()` |
| `std/png_enkel.no` | Ikon: indeksert PNG med 1 bit palett, lagra deflate-blokker, CRC-32 og Adler-32, og ein validator (`sjekk`) |

### Bruk

```
# app.no
bruk std.wasm_pwa som pwa

funksjon pwa_konfig() -> ordbok {
    la k = pwa.konfig("Deltakarar", "Deltakarar")
    pwa.med_farger(k, "#1f5f99", "#ffffff")
    pwa.med_skal(k, "/", side_html(""))          # app-skal: HTML utan persondata
    pwa.med_skal(k, "/stil.css", stil_css())
    pwa.med_privat(k, "/api/")                    # aldri cacha, aldri frå cachen
    returner k
}
# i <head>: pwa.head_tags(k)  (manifest, theme-color, ikon)

# klient.no
bruk std.wasm_pwa_klient som pwa
pwa.registrer(fun(s) -> sw_endra(s))            # s["hending"]: ventar → vis «Oppdater»
pwa.aktiver_ny()                                 # klikk på «Oppdater»
```

Bygg med `NC_WASM_SW=1` (i tillegg til `NC_WASM_KJELDE`, `NC_WASM_APP` og `NC_WASM_UT`). `tools/nc_wasm.no` byggjer då òg `sw.wasm` (frå ein generert drivar som kallar `std.wasm_sw.start()`, eller `NC_WASM_SW_KJELDE`) og SW-lastaren, skriv ei ekstra linje `SW: sw.wasm <n> byte, sw-lastar <m> byte, v=<12 hex> importar=… eksportar=… rt=…`, og legg rutene til i `serve.no`:

| Rute | Svar |
|---|---|
| `GET /sw.js` | `let C="<konfig-JSON>"` + SW-lastaren; `text/javascript`, `no-cache`, `Service-Worker-Allowed: /`, CSP med `'wasm-unsafe-eval'` |
| `GET /_nc/sw.wasm?v=…` | modulen, `immutable` ved rett `v` |
| `GET /manifest.webmanifest` | `application/manifest+json`, `no-cache` |
| `GET /_nc/ikon-192.png?v=…`, `ikon-512.png`, `ikon-maskable.png` | `image/png`, `immutable` ved rett `v`, ETag |
| `GET /_nc/offline` | offline-sida (`<body data-nc-offline="1">`, ingen skript, standard-CSP) |

Fail-closed: sida kan ikkje importere vertsfunksjonane til workeren (`sw_lytt`, `sw_inn`, `sw_ut`), og workeren kan berre importere dei og dei interne (`vert.sw_tillatne()`: ikkje DOM, nett, lagring eller `logg_test`). Begge gjev exit 1 med ei melding.

### Workeren (`std/wasm_sw.no`)

SW-lastaren registrerer `install`, `activate`, `message` og `fetch` **synkront** når skriptet blir evaluert (kravet i spesifikasjonen), og kvar hending ventar på `K`: modulen blir henta frå Cache Storage først (`sw.wasm?v=` er precacha, så workeren kan starte utan nett), elles frå nettet. `Z(hending, data)` gjev dataa til Norscode som JSON og får ein plan attende; lastaren utfører berre planen:

| Hending | Data til Norscode | Plan |
|---|---|---|
| install | — | `{c, u}`: cachenamnet og URL-ane som skal precachast. Kvar URL blir henta med `cache: "reload"`, og lagra berre når `lagre` seier ja; elles feilar installasjonen (ein privat eller feil konfigurert app-skal-URL stoppar han) |
| activate | namna på cachane | cachane som skal slettast: alle `nc-…` unnateke den gjeldande |
| message | `{m, k}`: meldinga og talet på faner (`clients.matchAll({includeUncontrolled: true})`) | `{s}`: skipWaiting for `nc-skip` (klikk), og for `nc-auto` når konfigurasjonen tillèt automatisk aktivering og `k ≤ 1` |
| fetch | `{m, u, n, o}`: metode, URL, modus, opphav | `{k, n, l, c, o}`, sjå under |
| lagre | `{s, t, r, c}`: status, type, omdirigert, Cache-Control | `{l}`: 2xx (ikkje 206), `basic`, ikkje omdirigert, utan `private` eller `no-store` (store og små bokstavar, med argument) |

Strategiane (`k`):

| `k` | Når | Kva lastaren gjer |
|---|---|---|
| `cache` | versjonerte ressursar (`?v=` i precachen) | cache først, elles nettet (og lagre) |
| `nett` | app-skalet og dei eksplisitte offline-rutene (nøkkelen er stien utan spørjing) | nettet først og lagre når `lagre` seier ja; utan nett cachen, så offline-sida for navigasjonar |
| `berre` | private stiar og ukjende sider, når det er ein navigasjon | berre nettet, aldri lagra; utan nett offline-sida |
| `forbi` | andre metodar enn GET, andre opphav, private førespurnader som ikkje er navigasjonar (`/api/…`), alt anna | ingen `respondWith` i det heile når modulen er klar (sjå «Funn»), elles `fetch` |

Eit feilsvar (4xx/5xx), eit privat svar eller ei omdirigering skriv difor aldri over ei oppføring, og innlogga HTML blir aldri cacha. Ein handterar som kastar, gjev ein trygg plan (ingenting lagra, ingen skipWaiting, fetch rett til nettet; ved install `null`, så installasjonen feilar). Kan modulen ikkje startast (t.d. CSP), går førespurnaden til nettet.

**Kill switch:** `pwa.slå_av(k)` gjev ein worker som ikkje handterer noko, ikkje precachar, tek seg sjølv i bruk ved `nc-auto` og slettar alle `nc-…`-cachar.

### Konfigurasjon, versjon og validering (`std/wasm_pwa.no`)

Konfigurasjonen workeren får (`C` i `sw.js`): `{v, c, p, i, r, x, o, a, av}` = versjon (64 hex), cachenamn `nc-<16 hex>`, precache-URL-ar, immutable (`?v=`), nett-først-stiar, private stiar, offline-sida, automatisk aktivering med éi fane, kill switch. `std/wasm_sw.no` validerer han (`valider_konfig`) og kastar `SwFeil` når han er ugyldig; då blir ingen handterar registrert, og installasjonen feilar.

Precachen er app-skalet og offline-rutene appen oppgjev, offline-sida, manifestet, ikona, lastaren (`/_nc/nc.js`), `app.wasm?v=` og `sw.wasm?v=`. **Versjonen** er sha256 over konfigurasjonen (med versjonane til klientmodulen, `sw.wasm` og ikona i URL-ane) og kroppane til alt som blir precacha utan `?v=`: app-skalet, offline-rutene, offline-sida, manifestet og lastaren. Han blir rekna ut i `/sw.js`-ruta (sha256 er ein builtin). Endrar éin av dei seg, endrar både `sw.js` og cachenamnet seg (testa for offline-teksten, eit app-skal, klientversjonen, lastaren og aktiveringa), og nettlesaren installerer ein ny worker.

`pwa.valider(k)` (også kalla i `sw_svar` og `manifest_svar`, som kastar `PwaFeil`): namn og kortnamn, `scope` er ein sti som sluttar på `/`, `start_url` innanfor scope **og** app-skal (elles kan appen ikkje starte utan nett), `display`, fargar `#rrggbb`, app-skal er absolutte stiar utan spørjing, ikkje private og ikkje PWA-rutene, og leverte ikon er gyldige PNG-ar med rett storleik.

**Manifest:** `id`, `name`, `short_name`, `description`, `lang`, `start_url`, `scope`, `display` (`standalone`), `theme_color`, `background_color` og ikon 192×192 (`any`), 512×512 (`any`) og 512×512 (`maskable`). **Ikon:** levert av appen (`pwa.med_ikon(k, "192", bytar)`) eller laga av `std/png_enkel.no`: ein ring i bakgrunnsfargen på temafargen, 1 bit palett, 192 px = 4 886 byte og 512 px = 33 366 byte (tak i testen 40 KB). Ringen ligg innanfor tryggleikssona til maskable-ikon (radius 40 %), så 512-biletet blir brukt til begge. Versjonen til eit generert ikon er sha256 av generatorversjonen, storleiken og fargane, så `/sw.js` treng ikkje lage ikona (512 px tek om lag 0,4 s i fastmodus). **Utlogging:** `pwa.utlogging_headerar()` / `pwa.med_utlogging(svar)` gjev `Clear-Site-Data: "cache", "storage"` (Cache Storage, IndexedDB, localStorage og registreringa).

### Sida (`std/wasm_pwa_klient.no`) og oppdateringsflyten

`registrer(ved_endring)` registrerer `/sw.js` (hjelparen `G` i lastaren), ber nettlesaren sjå etter ein ny versjon med ein gong (`update()`, som feilar stille utan nett), og kallar `ved_endring(status())` ved kvar hending. Flyten (standardval frå app-planen):
1. Ein ny versjon blir installert og **ventar**. Sida får `ventar`, og klienten sender `nc-auto` av seg sjølv. Workeren tek han i bruk berre når konfigurasjonen tillèt det (`med_aktivering`, standard sann) og det berre finst **éi fane**.
2. Elles ventar han til brukaren klikkar. Demoen viser «Ein ny versjon er klar.» og knappen «Oppdater»; klikket sender `nc-skip` (`aktiver_ny()`).
3. Når den nye workeren har teke over (`aktivert`, controllerchange), blir sida lasta på nytt med `vert.gå_til` (full navigasjon), men berre når brukaren sjølv bad om det. Ved automatisk aktivering seier demoen «Ein ny versjon er teken i bruk. Han blir lasta neste gong du opnar sida.».
4. Gamle cachar blir sletta i `activate`.

Avgjerda i demoen er ein rein funksjon (`klient.sw_handling(hending, klikka)`), testa i VM-en.

### CSP for service workeren [V]

Det er policyen på **svaret for `sw.js`** som gjeld i workeren, ikkje policyen på sida. Målt i Chrome 154 (headless) med eit scratch-skript og så med den ekte workeren:
- `sw.js` med `script-src 'self'`: `WebAssembly.compile` i workeren feilar med «Compiling or instantiating WebAssembly module violates the following Content Security policy directive because 'unsafe-eval' is not an allowed source of script …» (kjelde: `sw.js`), K blir avvist, installasjonen feilar, og workeren blir aldri aktiv. Utan nett etterpå får profilen ingenting («Page load failed»).
- `sw.js` med `script-src 'self' 'wasm-unsafe-eval'` (`ws.csp_wasm()`, som `pwa.sw_svar` brukar): `wasm-ok`.

`test_wasm_w8_oppdatering` (steg u5) er den negative kontrollen: same bygg med `pwa.sw_svar_med_csp(…, ws.csp_standard())`. `'wasm-unsafe-eval'` opnar ikkje for `eval`, og sw.js er det einaste skriptet utanom WASM-sidene som får han.

### Funn i Chrome [V]

- **Virtuell tid:** med `--virtual-time-budget` ventar ikkje Chrome på at workeren blir installert (som IndexedDB i W7). Testklientane held ein puls (`/api/puls`) i gang til workeren er `klar`. Utan puls (produksjonsbygget) blir ikkje installasjonen ferdig før DOM-en blir dumpa.
- **Aktivering:** Chrome aktiverer ikkje ein ventande worker (heller ikkje etter `skipWaiting`) så lenge den gamle har hendingar i gang, og ein jamn straum av `fetch` gjennom den gamle (pulsen) held han oppteken. Lastaren slepp difor `forbi`-førespurnader rett til nettet utan `respondWith` når modulen er klar, og testklienten byter pulsen ut med éin treg førespurnad om gongen (`/api/vent`, 1 s) medan han ventar på `aktivert`.
- **skipWaiting under install** (første utkast, planen `s` ved install) gjorde ikkje at Chrome aktiverte workeren når han var installert, målt med eit reint JS-probe og med den ekte workeren. Den automatiske aktiveringa går difor via `nc-auto`-meldinga, med talet på faner, etter at sida har fått `ventar`.
- **`statechange` og `B.waiting`:** når den nye workeren blir `installed`, er `registration.waiting` ikkje alltid oppdatert i `statechange`-handteraren. Auto-meldinga gjekk då til `null` og forsvann (om lag 1 av 3 køyringar). `sw_send` sender difor til `B.waiting || B.installing`.
- **Aktivering tek tid:** `controllerchange` kjem når den nye workeren byrjar å aktiverast, før `activate` (som slettar den gamle cachen) er ferdig. Testklienten ventar med treige førespurnader til det berre er éin cache att.
- **Kopien før respondWith:** `c.put(n, s.clone())` inne i `caches.open().then()` gav «Response body is already used» (Uncaught i konsollen, fanga av testen): kopien blir laga synkront før svaret går vidare.
- **Navigasjon:** headless Chrome med `--dump-dom` og virtuell tid dumpar aldri DOM-en etter ein JS-navigasjon (målt: hang til tidsgrensa). Omlastinga etter aktiveringa er difor slått av i testbygget (`last_på_nytt`: usann), og avgjerda er testa i VM-en.
- **Offline utan workeren:** Chrome skriv «Page load failed: net::ERR_CONNECTION_REFUSED» og dumpar ingenting; benken stoppar då med ein gong (`side_feila`) i staden for å vente til tidsgrensa.
- **HTTP-cachen:** `sw.wasm?v=` er `immutable`, så nettlesaren finn han i HTTP-cachen òg utan nett. Ein mutasjon der K berre hentar frå nettet, gav difor grøn demotest; at modulen kjem frå Cache Storage, er testa i lastaren (`test_wasm_lastar`).

### Storleikar (W8)

| | Byte |
|---|---|
| `sw.wasm` (std/wasm_sw.no med JSON frå runtime-biblioteket) | 23 274 |
| SW-lastaren (`sw_lastar.js`) | 1 751 (tak 1 792) |
| `sw.js` for demoen (konfigurasjon + lastar) | 2 473 |
| PWA-gruppa i sidelastaren (`G`, `B`, `O`, `sw_registrer`, `sw_status`, `sw_send`) | 706 (tak 736) |
| Testkrokane `test_cachar`, `test_bilete`, `test_resultat` (`M`) | 352 (tak 384) |
| Deltakar-demoen, produksjon: `app.wasm` / `nc.js` | 54 949 / 4 236 (W7: 52 213 / 3 402) |
| Ikon 192 / 512 px | 4 886 / 33 366 |

Planen hadde 1,5 KB for SW-lastaren. Utkastet der (1 168 byte) hadde verken svarmetadata til Norscode (regelen for lagring ligg i Norscode, ikkje i lastaren), kopien før `respondWith`, fallbacken til nettet når modulen ikkje kan startast, den synkrone gjennomsleppinga av `forbi` eller talet på faner i meldingane. `test_wasm_lastar` handhevar det nye taket, proveniensen for kvart token i `sw.js` og at han berre kan importere SW-funksjonane.

### Demoen utan nett

`examples/wasm_deltakarar/` er installerbar: app-skalet er sida og stilarket (lista kjem frå `/api/`, som er privat), og utan nett blir den lokale kopien frå W7 vist før feilen frå tenaren, med statusen «Viser lagra kopi frå eininga (N deltakarar). Tenaren svarar ikkje, så lista kan vere eldre enn på tenaren.» (feilen ventar no på kopien, som svaret frå tenaren gjorde i W7).

Resultat i Chrome 154 (`test_wasm_w8_demo`, testbygg, same profil): første besøk installerte workeren og precacha 12 URL-ar i éin cache `nc-<16 hex>` (om lag 5,3 s, mest ikona og precachen mot `nc serve`); `/mi-side` (utan `Cache-Control: private`, men privat i konfigurasjonen) og `/api/` vart aldri lagra, og `/nyheiter?ny=1` oppdaterte offline-ruta medan 403 og eit privat svar ikkje gjorde det. Med `nc serve` stoppa kom sida frå cachen (om lag 0,4 s), den lokale kopien vart vist, ikona var 192×192 og 512×512 frå cachen, `/nyheiter` var «nyheiter v2», og `/finst-ikkje` og `/mi-side` gav offline-sida. Ein fersk profil utan nett fekk ingenting. Ingen «Uncaught».

CLI: `NC_NETTLESAR_MODUS=offline NC_NETTLESAR_KJELDE=tests/fixtures/wasm_deltakarar_pwa_testklient.no NC_NETTLESAR_APP=tests/fixtures/wasm_deltakarar_pwa_testapp.no NC_NETTLESAR_TESTBYGG=1 NC_NETTLESAR_STI='/test?steg=1' NC_NETTLESAR_STI_OFFLINE='/test?steg=3' NC_NETTLESAR_KREV='ikon frå cachen' ./bin/nc run tools/wasm_nettlesar.no` gav `offline OK`; med `NC_NETTLESAR_SW=0` (utan workeren) «den andre lastinga feila» og exit ≠ 0.

## Nettlesar-benken (W5)

`std/wasm_nettlesar.no` er éin stad for det alle nettlesartestane treng, skrive i Norscode (`tests/fixtures/wasm_chrome_hjelp.no` er no eit tynt lag over han; `wasm_test_hjelp` har framleis sitt eige `bygg`, sidan det å importere benken der kosta om lag 3 s per test i standard-VM-en):

| Funksjon | |
|---|---|
| `finn_chrome()`, `finn_jsc()` | `NC_CHROME`/`NC_JSC`, elles standardstiane; "" når ingen svarar (`--version`, `print(6*7)`) |
| `bygg(kjelde, utmappe, testbygg, prefiks, app)` | `tools/nc_wasm.no` i ein barneprosess i fastmodus → `{"ok", "linje", "ut"}` |
| `ny_økt(namn)` | Unik mappe `build/wasm-nettlesar/<namn>-<tid>/` for profilar og netloggar |
| `start_serve(økt, app, port)`, `stopp_serve(økt)`, `serve_køyrer(h)` | Barne-`nc serve` med eksplisitt miljø, venta til «Socket-lytting aktiv» |
| `last_side(økt, url, op)` | Headless Chrome med flagga frå §4.4 → `{"dom", "konsoll", "logg", "uncaught", "ms", "tidsavbrot", "mål", "sidelasting"}`. `op`: `profil` (same namn = same profil), `behald_profil`, `virtuell_ms` (standard 5 000), `netlog`, `grense_ms` (standard 60 000) |
| `rydd(økt)` | Stoppar serve, drep det som er att med økt-mappa i kommandolinja (`pkill -f`), sjekkar med `pgrep` og slettar mappa → `{"drepne", "attverande"}` |
| `logg_frå_dom`, `har_uncaught`, `mål_frå_logg`, `sidelasting_frå_netlog`, `skriv_resultat` | Tolking og resultatfiler (`build/wasm-nettlesar/resultat/<namn>.json`, aldri committa) |
| `shim.jsc_køyr(jsc, utmappe)` (`std/wasm_jsc.no`) | Modulen og lastaren i JavaScriptCore → `{"logg", "uncaught", "exit", "stderr"}` |

**Berre eigne prosessar.** Kvar Chrome-prosess (også hjelpeprosessane) har profilmappa i kommandolinja, og profilane ligg alltid under `build/wasm-nettlesar/`. `drep_med` nektar ei nål utan ei benkmappe, så `pkill -f` kan ikkje treffe andre prosessar. `test_wasm_w5` startar ein prosess med økt-mappa og ein utan, og krev at `rydd` drep den eine og let den andre vere.

**CLI:** `./bin/nc run tools/wasm_nettlesar.no` med `NC_NETTLESAR_MODUS`:
- `royk` (standard): teljar-demoen (eller `NC_NETTLESAR_KJELDE`/`APP`/`STI`), DOM-en må innehalde `NC_NETTLESAR_KREV`, ingen «Uncaught».
- `korpus`: `NC_NETTLESAR_KJELDE` er ei kommaliste med korpusprogram; kvart blir bygd i testbygg og køyrt i Chrome og i jsc mot «#=»-fasiten.
- `maal`: app-kjensle (sjå under), median over `NC_NETTLESAR_GONGER` lastingar.
- `offline`: last sida, stopp `nc serve`, last om att med same profil (W8: service workeren). Utan `NC_NETTLESAR_KREV` blir berre resultatet skrive.
- `NC_CHROME=/finst/ikkje` gjev exit 2 og «fann ikkje nettlesar». Etter kvar køyring er ingen prosessar med `build/wasm-nettlesar` att (CLI-en skriv `rydding: n prosessar drepne, 0 att`, og exit-koden er 1 om nokon er att).

**Virtuell tid.** Med `--virtual-time-budget` står klokka i sida stille medan ein førespurnad ventar på nettverket, og ho går berre når sida køyrer JavaScript [V: `Date.now()` gjekk 63 ms i ei travel lykkje, men berre 77 ms medan ein førespurnad venta 1,5 s på tenaren]. Ein tidsgrense på 200 ms slo difor aldri til mot ein treg tenar. Testklienten held sida oppteken i 60 ms etter at han har sendt, så tidsgrensa på 20 ms går ut. Utan virtuell tid dumpar Chrome DOM-en ved `load`, før WASM-en er ferdig, så virtuell tid er framleis vegen.

### App-kjensle (målt)

`tests/fixtures/wasm_kjensle_testklient.no` måler i WASM-sida sjølv med `builtin.tid_ms()` (`Date.now()`), frå hendinga blir send gjennom testkroken og den ekte hendingsdelegeringa til DOM-en er oppdatert, som gjennomsnitt over mange hendingar. Etter kvar hending blir DOM-en lesen og samanlikna med det venta (`feil` skal vere 0; ei hending som ikkje når fram, syner). Sidelastingstida kjem frå Chrome-netloggen (`--log-net-log`): frå starten av dokumentet til slutten av den siste førespurnaden til same opphav. `nc serve` loggar ikkje førespurnader, så tenarloggen gjev ingen tider.

Skjema-demoen, macOS (Apple Silicon), Chrome 154 headless, `nc serve` i fastmodus, `maal` med median over 5 lastingar:

| Mål | Median | Alle fem |
|---|---|---|
| `start_ms`: `s.start()` med 50 rader | 2 ms | 3, 2, 2, 2, 1 |
| `filter_us`: input i filteret → 7 eller 50 rader gjengjevne | 0,30 ms | 0,40, 0,30, 0,30, 0,30, 0,33 |
| `klikk_us`: delegert «Fjern» → rad borte og lista gjengjeven | 0,40 ms | 0,40, 0,50, 0,40, 0,50, 0,40 |
| `send_us`: submit → validering, ny rad, gjengjeving, lokal lagring, status | 1,2 ms | 1,4, 1,1, 1,2, 1,2, 1,2 |
| `feil` (hendingar utan venta DOM-endring) | 0 | 0, 0, 0, 0, 0 |
| Sidelasting (`/test`, 70 KB side + lastar + 12 KB modul) | 55 ms | 114 (kald), 58, 50, 55, 52 |

Deltakar-demoen i produksjonsbygg (`royk`): sidelasting 23 ms frå dokumentet til svaret på `/api/deltakarar` (6 førespurnader: `/` 3 ms, `stil.css` 2, `nc.js` 3, `app.wasm` 4 (27 KB), `/api/deltakarar` 2).

Resultatet står i `build/wasm-nettlesar/resultat/maal.json`.

### JavaScriptCore utan JS-filer

`jsc -e <skript>` køyrer eit skript frå kommandolinja, så det trengst inga JS-fil, verken i repoet eller under `build/`. Skriptet er vertsshimen i `std/wasm_jsc.no` (JS-tre, emittert av `std/wasm_js.no` som lastaren) pluss lastaren `nc.js` som `tools/nc_wasm.no` emitterte:
- `console.log` og `document.getElementById(…).append` skriv kvar loggbit som éi JSON-strenglinje med `print`, så benken set saman den eksakte teksten;
- `TextDecoder`/`TextEncoder` for UTF-8 (`decodeURIComponent(escape(…))`, konstruerbare via `Object.bind(null, o)`);
- `fetch` gjev URL-en attende, og `WebAssembly.instantiateStreaming` les modulfila med `readFile`;
- lastaren får `.catch`, som skriv «Uncaught <feil>» når ein trap når JavaScript.

`test_wasm_w5` køyrer `unntak_endeleg`, `closure_fangst`, `rt_json_feil` og `w4_ordbok` i jsc (macOS 26.6.2, Safari 26.6-motoren) med utskrift lik fasiten, og `kontroll_trap` gjev «Uncaught». `tools/wasm_nettlesar.no` (korpus) køyrer kva som helst av korpuset i både Chrome og jsc; `w6_nett` og `w6_nett_ko` gav fasiten i begge. Programma treng berre `logg_test`/`logg` og tal- og tidsfunksjonane, ikkje DOM-en.

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

W6 (byte):

| Modul | Storleik | Lastar |
|---|---|---|
| `w6_nett` (korpus, testbygg) | 22 400 | |
| Deltakar-demoen (`examples/wasm_deltakarar/klient.no`) | 27 340 | 2 027 |
| `w6_nett_ko` (korpus, testbygg) | 19 710 | |
| Nett-testklienten (testbygg) | 25 530 | 1 148 |

Storleiken kjem mest frå `rt.json_parse`/`json_skriv` (om lag 15 KB, som `rt_json` i W3) og køa. W0–W4-modulane er uendra: nett-importane og `Q` kjem berre med når `std/wasm_nett.no` blir brukt.

W7 (byte):

| Modul | Storleik | Lastar |
|---|---|---|
| `w7_lager` (korpus, testbygg) | 36 605 | |
| `w7_lager_ko` (korpus, testbygg) | 31 123 | |
| `tests/fixtures/wasm_w7_utan_idb.no` (testbygg) | 32 720 | |
| W7-testklienten (testbygg, med nett for pulsen) | 53 000 | 2 583 |
| Deltakar-demoen (produksjon, med lokal kopi) | 52 213 (W6: 27 340) | 3 402 (W6: 2 027) |

`std/wasm_lager.no` dreg med seg `rt.json_parse`/`json_skriv` og `sha256` (for `brukar_db_namn`). Demoen hadde alt JSON-delen frå nett; det nye er om lag 25 KB (lagringsmodulen, SHA-256 og den lokale kopien i klienten). W0–W6-modulane er uendra: lagrings-importane, `I` og `J` kjem berre med når `std/wasm_lager.no` blir brukt.

## Testar

| Test | Kva han dekkjer |
|---|---|
| `tests/test_wasm_w0.no` | Validatoren godtek fasitmodular og avviser éin feil om gongen. Korpuset blir bygd med CLI-en, med dei venta importane og eksportane. `minimal.no` frå CLI-en er byte-eksakt og har ingen rt-funksjonar. Rapporten frå CLI-en er fail-closed: åtte feil i `feil_ustotta.no`, mellom anna `bryt` ut av `prøv` og `utsett` i ei lykkje (W2), ein HTML-mal som ikkje er konstant (W3), og `ustøtta builtin: ukjend`. Sju i testbygg. Versjonen følgjer bytane. W3: lowring og validering skjer berre i CLI-barneprosessane. |
| `tests/test_wasm_w1.no` | GC-fasitmodulen frå proben blir bygd på nytt med assembleren og er byte-eksakt. Kodingane til GC-typane og instruksjonane er rette. W1-validatoren (datacount, array-typar) avviser éin feil om gongen. Tree-shakinga er rett. |
| `tests/test_wasm_les.no` | W2-typesjekken: ein handbygd modul (kontrollflyt, i64, GC, `try_table`/`throw`, `ref.func`, `call_ref`, globalar) blir godteken. 17 variantar med éin feil kvar blir avviste med rett melding (W3: validerte i `tests/fixtures/wasm_les_typesjekk.no` i fastmodus, og testen sjekkar kvar linje). Ekte korpusmodular blir godtekne, og avviste etter mutasjon: utan elementsegmentet, og med feil tag-type. |
| `tests/test_wasm_w2.no` | W2-lowringa på bytenivå: ingen tag- eller elementseksjon utan unnatak og closures. Init i `try_table` og `throw` for `THROW`. `unntak_passar` berre ved typa fang. `kast` er `throw`, ikkje trap. Deklarativt elementsegment og `call_ref` for closures. `$lambda`-typen for `ncb_call_fn`. |
| `tests/test_wasm_w3.no` | W3-lenkinga: `lenk_rt` fører berre rt-modulane inn (ikkje drivaren) og endrar ikkje inndata. Utan lenking er `split` ein kompileringsfeil. Tree-shaking frå CLI-en: `split` gjev `rt=splitt` og ingen desimalimport, `sha256` gjev `rt=sha256_hex`, og `desimaltall(tekst)` gjev `tekst_flyt`. Heile biblioteket har alle 17 offentlege funksjonar og er under taket på 24 KB. Fail-closed rapport med biblioteket lenka inn. |
| `tests/test_wasm_korpus_chrome.no` | Paritetskorpuset for W1, sjå under. |
| `tests/test_wasm_korpus_w2.no` | Paritetskorpuset for W2 (delt ut i W5 for tidsgrensa). |
| `tests/test_wasm_korpus_w3.no` | Paritetskorpuset for W3 (`kh.korpus_w3()`), og tekstverdiar i DOM-en i Chrome: `sett_tekst` via utrekna selektorar, med HTML-teikn og eit desimaltal, og `sett_html` med ein `<script>`-tagg i verdien. |
| `tests/test_wasm_lastar.no` | Byte-tak og tree-shaking. Importnamna i lastaren er lik importseksjonen (også namn med æøå). Strukturen kjem frå tabellen. Ingen JS-fragment i literalane. Kvart lastartoken finst i emitteren eller i tabellen (proveniens). Escape er lik `std.html.escape`. W4: produksjonstabellen ≤ 2 560 byte, med testkrokar ≤ 3 072 og skjema-demoen ≤ 2 048; `U`/`L`/`N`/`P`/`F` blir tree-shaka etter bruk; `setAttribute` og `createElement` berre med namn som er kontrollerte ved kompilering; ingen tildeling til `on…`-felt; navigasjon berre til same opphav; presedens i emitteren. Sjekkane står i `tests/fixtures/wasm_lastar_sjekk.no` og køyrer i ein barneprosess i fastmodus. |
| `tests/test_wasm_serve.no` | Barne-`nc serve` av serve-inngangen (W4): `application/wasm` byte-identisk; `immutable` berre ved rett `v`; ETag og 304; lastaren utan `v` med `no-cache`; `wasm-unsafe-eval` berre på WASM-sida; `<script type="module">`; versjonen endrar seg når éin byte endrar seg; appen åleine (utan bygg) kan serverast; målt kostnad per førespurnad. |
| `tests/test_wasm_w4.no` | Fail-closed rapport for utrygge vertskall (11 feil). Atom og importar for dei nye vegane i ein ekte modul, men ikkje i teljaren. rt-cachen: bygga utan cache, med kald og med varm cache er byte-identiske. Den varme hentar kropp og analyse for alle `std.*`-funksjonane. Ei endra oppføring blir lowra på nytt, og ein øydelagd kropp blir brukt (validatoren avviser han). Filnamnet følgjer backend-kjeldene. Stubbane kastar i VM-en. |
| `tests/test_wasm_w4_chrome.no` | Korpuset `w4_ordbok` (VM, bygg, Chrome). Skjema-demoen i testbygg med brukarsteg gjennom testkrokane mot fasit (reglane er rekna i VM-en), og i produksjonsbygg med `?q=`. Negativ kontroll med standard-CSP. Sjå «Skjema-demoen». |
| `tests/test_wasm_w0_chrome.no` | Valfri. Teljaren etter tre klikk, negativ CSP-kontroll og W0-korpuset mot VM-en. |
| `tests/test_wasm_w5.no` | Benken og CLI-en (W5): `NC_CHROME=/finst/ikkje` gjev exit ≠ 0 og «fann ikkje nettlesar», ukjend modus exit 2; tolking av loggen, «MÅL»-linjer, «Uncaught» og ein netlogg med ein førespurnad til eit anna opphav og ein som aldri vart ferdig; `rydd` drep ein prosess med økt-mappa og ikkje ein utan, og `drep_med` nektar ei nål utan benkmappe; jsc-skriptet. Med Chrome: `royk` gjev exit 0, eit krav som ikkje finst gjev exit 1, `maal` gjev mål og sidelastingstid med `feil=0`, og `pgrep -f build/wasm-nettlesar` finn ingen prosessar etter kvar køyring. Med jsc: fire korpusprogram lik fasiten og `kontroll_trap` med «Uncaught». |
| `tests/test_wasm_w6.no` | W6 utan nettlesar: korpusa `w6_nett` (reine funksjonar) og `w6_nett_ko` (køa: levering i sende-rekkjefølgje med konvoluttar i ei anna rekkjefølgje, tilbakekall som kastar, manglande feil-tilbakekall, ukjend id, ny førespurnad under levering) i VM, bygg og Chrome; stubbane kastar i VM-en; ugyldige førespurnader kastar HentFeil; demo-reglane og `feilmelding` i VM-en; tenaren utan sokkel (GET, POST med informasjonskapsel, `Cookie` med stor forbokstav, 400 for ugyldig JSON, feil form og 61 deltakarar, øydelagd kapsel); bygga har nett-importane og `__hent_ferdig`, og demo-lastaren er ≤ 2 560 byte. |
| `tests/test_wasm_w6_chrome.no` | Valfri. Nett-testklienten (ti førespurnader og offline) lik fasiten, utan «Uncaught», og `nc serve` stoppa av `/api/stopp`; deltakar-demoen i testbygg lik fasiten; produksjonsbygget viser dei 10 frå tenaren. |
| `tests/test_wasm_w7.no` | W7 utan nettlesar: korpusa `w7_lager` (reine funksjonar) og `w7_lager_ko` (køa) i VM, bygg og Chrome; stubbane kastar i VM-en; ugyldige argument kastar LagerFeil «data» utan at noko blir registrert; den lokale kopien i demoen i VM-en (skjema, databasenamn, feilmeldingar); regresjon for rt-cachen (to program etter kvarandre med fersk, delt cache, byte-likt); jsc (valfri): korpusa lik fasiten, og utan IndexedDB gjev opninga LagerFeil «open» i rekkjefølgje utan «Uncaught» (ende til ende er SKIP i jsc). |
| `tests/test_wasm_w7_chrome.no` | Valfri. W7-testklienten i tre lastingar med same profil: opne/oppgradere (v1→v2→v3, ny indeks, nytt lager, fjerna lager, v3 medan v2 er open), CRUD i rekkjefølgje med eit tilbakekall som kastar, verdiar attende like, nøkkelrekkjefølgje, 100 dokument i éin batch, område- og indeksspørjingar, batchar som rullar tilbake (unik indeks, ukjent lager), versjon, lukka database; dei 100 etter omlasting; ein annan brukar ser ein tom database; logg_ut; ny og tom database etter utlogging; lukk medan opninga går (det som alt var bede om, blir køyrt, det som kjem etter, får `transaksjon`). Negativ kontroll: fersk profil finn ingenting. Utan Chrome: bygget med lagrings-importane. |
| `tests/test_wasm_w7_demo.no` | Valfri. Demoen: lokal kopi vist før tenarsvaret (tenaren svarar etter 800 ms), ulagra endringar som blir ståande, lagring, sletting med knappen, ingen kopi etter sletting; fersk profil utan kopi (negativ kontroll); produksjonsbygget viser dei 10 frå tenaren utan lokal kopi. Utan Chrome: bygga, lagrings-importane og lastartaket (3 840 byte). |
| `tests/test_wasm_w8.no` | W8 utan nettlesar: korpuset `w8_sw` (dei reine funksjonane i workeren og statusen på sida) i VM, bygg og Chrome; PNG (CRC-32 og Adler-32 mot kjende vektorar, validatoren fangar øydelagd CRC, avkorting, signatur og komprimerte blokker; ikona 192 og 512 i fastmodus: gyldige, 512 ≤ 40 KB, deterministiske, levert ikon med feil storleik avvist); manifestet og valideringa; versjonen (offline-teksten, eit app-skal, klientversjonen, lastaren og aktiveringa endrar `sw.js` og cachenamnet), ingen private ruter i precachen, workeren godtek konfigurasjonen frå tenaren; stubbane; `sw_handling` i demoen. |
| `tests/test_wasm_w8_serve.no` | Bygget av demoen med workeren (importar, eksportar, SW-lastartaket) og rutene i serve-inngangen utan sokkel (`/sw.js` med CSP, `Service-Worker-Allowed` og `no-cache`, `sw.wasm`, manifest, ikon med `immutable` og ETag, offline-side og head-taggar); fail-closed: SW-funksjonar på sida og DOM i workeren blir avviste. |
| `tests/test_wasm_w8_demo.no` | Valfri. Demoen: installasjon og precache, private svar og feilsvar blir ikkje lagra, sida frå cachen utan nett med den lokale kopien og ikona, offline-sida for ukjende og private sider; fersk profil utan nett får ingenting. |
| `tests/test_wasm_w8_oppdatering.no` | Valfri. Tre versjonar etter kvarandre med same profil: ny versjon ventar på klikk (to cachar, melding og knapp), klikk → aktivert og gamal cache sletta; automatisk aktivering med éi fane; `sw.js` utan `'wasm-unsafe-eval'` gjev ein worker som aldri blir aktiv (CSP-brotet frå `sw.js` i konsollen), og utan nett ingenting. |

Chrome-hjelparane ligg i `tests/fixtures/wasm_chrome_hjelp.no` (W5: eit tynt lag over nettlesar-benken `std/wasm_nettlesar.no`, med profilane under `build/wasm-nettlesar/fixtur/`). Der les `dump_med_konsoll` DOM-en og Chrome-konsollen (stderr med `--enable-logging`). Serve utan sokkel (`NORSCODE_FAKE_HTTP_REQUESTS`) ligg i `tests/fixtures/wasm_serve_hjelp.no` (W6). Korpushjelparane ligg i `tests/fixtures/wasm_korpus_hjelp.no`, modulinnsyn (seksjonar, kroppar) i `tests/fixtures/wasm_test_hjelp.no`, og `tests/fixtures/wasm_valider_fil.no` validerer ei fil i ein barneprosess. W3: `kh.køyr_korpus` er heile tre-stegs-køyringa, som begge korpustestane brukar.

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
| `w6_nett` (W6) | validering av førespurnader, førespurnaden som JSON, konvolutten (headerar, kropp med linjeskift, feilnamn), klassifisering av 2xx, 3xx, 404, tidsavbrot, avbrote, nettverk og ugyldig JSON, HentFeil kasta og fanga |
| `w6_nett_ko` (W6) | køa utan nettverk: konvoluttar i rekkjefølgja 3, 5, 4, 2, 1 gjev tilbakekall 1–5, eit ok- og eit feil-tilbakekall som kastar, manglande feil-tilbakekall, ukjend id, avbryt, ny førespurnad under levering |
| `w7_lager` (W7) | skjemavalidering (9 tilfelle), skjemaet og opninga som JSON, nøklar (heiltal ved ±2⁵³, desimaltal, tekst, bool, null, liste), op-ane som JSON (put med nøsta indeksstiar, auto-nøkkel, eksplisitt nøkkel, område, grense, indeksar), 15 feil med ein gong, transaksjonsmodus og omfang, JSON utan rå kontrollteikn (rundtur), konvoluttar, feilklassar, resultat (auto-nøkkel sett inn, øydelagd lagra verdi), LagerFeil kasta og fanga, databasenamn per brukar |
| `w7_lager_ko` (W7) | køa utan IndexedDB: konvoluttar i rekkjefølgja 3, 5, 4, 2, 1 gjev tilbakekall 1–5, tilbakekall som kastar, manglande feil-tilbakekall (kvote), ukjend id, eit andre svar på same id, ny førespurnad under levering |
| `kontroll_trap` | ikkje paritet: positiv kontroll for konsollsjekken |

Kvart program har ein **kjend fasit** i `#=`-linjer, og `test_wasm_korpus_chrome` køyrer dei i tre steg:

1. Køyrer kvart program i VM-en (barne-`nc run`) og krev at stdout er lik fasiten.
2. Byggjer kvart program i testbygg med CLI-en. Modulen blir validert, med typesjekk, og må importere `logg_test`. W2-programma må ha `uhandtert`.
3. Startar éin barne-`nc serve` med ei side per program. Headless Chrome køyrer sidene. Teksten i `<pre id="nc-logg">` må vere identisk med VM-utskrifta, og konsollen må vere fri for «Uncaught». Utan Chrome blir dette steget SKIP. `NC_CHROME` vel binær.

`test_wasm_korpus_w3` køyrer W3-programma på same måten (`kh.køyr_korpus`). `rt_json_feil` og `rt_sorter` kallar rt-funksjonane direkte (`bruk std.wasm_rt som rt`), så VM-steget køyrer sjølve biblioteket som Norscode. Dei andre kallar builtins, som er vertsbuiltins i VM-en og biblioteket i WASM. Då samanliknar korpuset semantikken til biblioteket med VM-en.

**Resultat:** alle 18 programma gav identisk utskrift i VM-en og i Chrome 154 (macOS), utan «Uncaught». Dei same 18 modulane gav identisk utskrift i JavaScriptCore frå macOS 26.6.2 (Safari 26.6-motoren), køyrt med eit scratch-skript utanfor repoet. Linux i Docker har ikkje Chrome, så der køyrer steg 1 og 2.

**W3-resultat:** alle 5 W3-programma gav identisk utskrift i VM-en (macOS og Linux) og i Chrome 154, utan «Uncaught», og DOM-sida viste tekstverdiane som venta. Med den endelege W3-koden gav alle 23 korpusmodulane identisk utskrift i JavaScriptCore frå macOS 26.6.2. Scratch-skriptet utanfor repoet køyrer den genererte `nc.js` med ein shim for `document`, `fetch` og `TextEncoder`/`TextDecoder`. W5: scratch-skriptet er erstatta av nettlesar-benken, som køyrer same vegen med `jsc -e` og ein shim emittert frå Norscode-data (sjå «JavaScriptCore utan JS-filer»).

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
| W5: `rydd` utan `drep_med` | `test_wasm_w5` (rydding) |
| W5: `mål_frå_logg` utan krav om `=` | `test_wasm_w5` (tolking) |
| W5: netloggen utan opphavsfilter | `test_wasm_w5` (tolking) |
| W5: jsc-shimen utan UTF-8-dekoding | `test_wasm_w5` (jsc-korpuset) |
| W5: `royk` utan kravet til DOM-en | `test_wasm_w5` (royk med feil krav gav exit 0) |
| W5: klikka i kjensle-klienten treffer ikkje | `test_wasm_w5` (`feil` > 0) |
| W6: levering i kome-rekkjefølgje (utan køa) | `test_wasm_w6` (VM-steget, `w6_nett_ko`) og Chrome |
| W6: ok-tilbakekall utan `fang` | `test_wasm_w6` (VM-steget, `w6_nett_ko`) |
| W6: feil-tilbakekall utan `fang` | `test_wasm_w6_chrome` (nr. 6 kom aldri) |
| W6: lastaren utan `AbortSignal.timeout` | `test_wasm_w6_chrome` (nr. 6 fekk svar) |
| W6: `avbryt` utan `slepp` | `test_wasm_w6_chrome` (nr. 3 fekk svar) |
| W6: 3xx og 4xx som ok | `test_wasm_w6` (VM-steget, `w6_nett`) |
| W6: informasjonskapselen lesen med `web.request_cookie` | `test_wasm_w6` (tenaren) |
| W6: demoen viser ikkje feila frå tenaren | `test_wasm_w6` (`feilmelding` i VM-en) |
| W7: køa leverer i kome-rekkjefølgje | `test_wasm_w7` (VM-steget, `w7_lager_ko`) |
| W7: ok-tilbakekall utan `fang` | `test_wasm_w7` (VM-steget, `w7_lager_ko`) |
| W7: `json_trygg` escapar ikkje kontrollteikn | `test_wasm_w7` (VM-steget, `w7_lager`) |
| W7: same databasenamn for alle brukarar | `test_wasm_w7` (`w7_lager`) |
| W7: auto-nøkkelen blir ikkje sett inn ved lesing | `test_wasm_w7` (`w7_lager`) |
| W7: fiksen i analysecachen fjerna (`urein` ignorert) | `test_wasm_w7` (regresjonen: validatoren avviste `w7_lager_ko`) |
| W7: `I` og `J` alltid med i lastaren | `test_wasm_lastar` |
| W7: unntaket for on…-felt gjeld alle mottakarar | `test_wasm_lastar` |
| W7: éin transaksjon per op (ingen tilbakerulling av batchen) | `test_wasm_w7_chrome` (steg 1: «tel 102, 201 {…}») |
| W7: tilkoplinga lukkar seg ikkje ved `versionchange` | `test_wasm_w7_chrome` (v3 blokkert) |
| W7: `logg_ut` slettar ikkje databasen | `test_wasm_w7_chrome` (steg 3 fann dei 100) |
| W7: `I` utan `.catch` | `test_wasm_w7_chrome` (ukjent lager: «Uncaught», tilbakekallet kom aldri) |
| W7: lukk under opninga sendt før det som venta | `test_wasm_w7_chrome` (steg 3: «tel: LagerFeil transaksjon InvalidStateError») |
| W7: alle batchar readonly | `test_wasm_w7_chrome` |
| W7: demoen held ikkje på ulagra endringar frå eininga | `test_wasm_w7_demo` |
| W7: demoen lagrar utan at brukaren har valt det | `test_wasm_w7_demo` |
| W8: `private` blir ikkje sett på som ein grunn til å ikkje lagre | `test_wasm_w8` (VM-steget, `w8_sw`) og `test_wasm_w8_demo` (steg 3: «privat utgåve») |
| W8: versjonen utan offline-sida | `test_wasm_w8` (versjonen) |
| W8: feil CRC-polynom | `test_wasm_w8` (CRC-vektoren) |
| W8: auto-meldinga ser bort frå konfigurasjonen | `test_wasm_w8` (VM-steget) og `test_wasm_w8_oppdatering` (u2) |
| W8: kopien etter `caches.open` (etter respondWith) | `test_wasm_lastar` |
| W8: `respondWith` for `forbi` òg | `test_wasm_lastar` |
| W8: `finn` tillaten i workeren | `test_wasm_lastar` |
| W8: `G` dreg med seg `M` | `test_wasm_lastar` (hjelparane til PWA-gruppa) |
| W8: sida kan importere `sw_lytt` | `test_wasm_w8_serve` |
| W8: `sw.js` med standard-CSP | `test_wasm_w8_serve` |
| W8: `sw.js` utan `Service-Worker-Allowed` | `test_wasm_w8_serve` |
| W8: statusgrensa 499 i staden for 299 (403 blir lagra) | `test_wasm_w8_demo` (steg 3: «status 403 nekta») |
| W8: private navigasjonar «nett først» med lagring | `test_wasm_w8_demo` (steg 2: `/mi-side` i cachen; testsida har ikkje `Cache-Control: private`, så konfigurasjonen åleine må verne ho) |
| W8: sida sender ikkje `nc-auto` | `test_wasm_w8_oppdatering` (u4: «vart aldri aktivert») |
| W8: `activate` slettar ingen gamle cachar | `test_wasm_w8_oppdatering` (u2: «cachar: 2») |

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

Tida er målt i sekund med `./bin/nc test` (standard) og `NC_TEST_VM_FAST=1 ./bin/nc test` (fast), med W8-koden (siste køyring; Linux inkluderer oppstarten av Docker-behaldaren). W7-tala står i parentes.

| Test | macOS standard | macOS fast | Linux standard | Linux fast |
|---|---|---|---|---|
| `test_wasm_w0` | 15 (15) | 13 (8) | 22 (18) | 13 (13) |
| `test_wasm_les` | 9 (6) | 6 (5) | 15 (13) | 11 (9) |
| `test_wasm_w1` | 13 (9) | 7 (6) | 17 (16) | 11 (11) |
| `test_wasm_w2` | 14 (11) | 7 (6) | 16 (14) | 13 (12) |
| `test_wasm_w3` | 15 (14) | 13 (10) | 17 (17) | 13 (13) |
| `test_wasm_w4` | 20 (19) | 19 (15) | 27 (23) | 25 (21) |
| `test_wasm_lastar` | 19 (13) | 18 (13) | 21 (17) | 22 (17) |
| `test_wasm_serve` | 8 (7) | 7 (3) | 11 (9) | 5 (5) |
| `test_wasm_korpus_chrome` | 32 (29) | 26 (27) | 26 (24) | 25 (22) |
| `test_wasm_korpus_w2` | 32 (31) | 27 (27) | 25 (24) | 24 (23) |
| `test_wasm_korpus_w3` | 31 (28) | 26 (25) | 27 (35) | 25 (23) |
| `test_wasm_w0_chrome` | 13 (12) | 11 (10) | 5 (5) | 3 (2) |
| `test_wasm_w4_chrome` | 25 (24) | 22 (17) | 18 (17) | 15 (14) |
| `test_wasm_w5` | 35 (33) | 30 (24) | 9 (8) | 4 (4) |
| `test_wasm_w6` | 42 (38) | 37 (31) | 39 (38) | 35 (30) |
| `test_wasm_w6_chrome` | 40 (32) | 36 (31) | 14 (15) | 13 (12) |
| `test_wasm_binary`, `test_wasm` | ≤ 1 | ≤ 1 | 4 | 2 |
| `test_wasm_w7` | 50 (45) | 41 (34) | 37 (36) | 36 (33) |
| `test_wasm_w7_chrome` | 21 (19) | 17 (18) | 13 (14) | 13 (12) |
| `test_wasm_w7_demo` | 38 (30) | 35 (30) | 25 (23) | 25 (23) |
| `test_wasm_w8` (ny) | 42 | 25 | 27 | 19 |
| `test_wasm_w8_serve` (ny) | 30 | 29 | 31 | 26 (82 med kald rt-cache) |
| `test_wasm_w8_demo` (ny) | 36 | 31 | 22 | 28 |
| `test_wasm_w8_oppdatering` (ny) | 51 | 50 | 21 | 22 |

- **W8:** `test_wasm_w8_oppdatering` er den tyngste (51 s i standard-VM-en på macOS): eit bygg med workeren (om lag 20 s i fastmodus), fire `nc serve`-oppstartar og seks lastingar i Chrome, der éi er om lag 9 s (nettlesaren ser etter den nye versjonen og installerer han). Ventinga på aktiveringa er korta ned til treige førespurnader på 0,4 s. Linux har ikkje Chrome, så der er berre bygget og serve-variantane med. Den første køyringa etter ei endring i `std/wasm_vert.no` (ny nøkkel for rt-cachen) tek lenger tid: `test_wasm_w8_serve` tok då 82 s i fastmodus på Linux (26 s med varm cache).
- Dei andre testane er 1–8 s tregare enn i W7: rt-cachen fekk ny nøkkel (`std/wasm_vert.no` er endra), og demoen byggjer no òg PWA-gruppa.

- **`test_wasm_lastar`:** med W4-tabellen tok emitteringa og tokeniseringa av dei fulle lastarane om lag 3 minutt i standard-VM-en (189 s målt). Sjekkane står i `tests/fixtures/wasm_lastar_sjekk.no` og køyrer i ein barneprosess i fastmodus, som valideringa i W3.
- **`test_wasm_korpus_chrome`** var 53–55 s etter W4, rett under grensa. W5 delte W2-programma ut i `test_wasm_korpus_w2`.
- **W5-benken og testtida:** ein første versjon av benken importerte emitteren (for jsc) og venta på at hjelpeprosessane til Chrome skulle avslutte etter kvar side. Det gav om lag +3 s per test som importerte `wasm_test_hjelp` og +5 s for korpustesten i standard-VM-en. jsc-køyringa ligg difor i `std/wasm_jsc.no`, `wasm_test_hjelp` importerer ikkje benken (han har sitt eige `bygg`), og `benk.rydd(økt)` drep det som er att til slutt.
- **`test_wasm_w6`** (56 s med Chrome-stega i same test) er delt i `test_wasm_w6` og `test_wasm_w6_chrome`.
- Ingen wasm-test er over 55 s i standard-VM-en (W8: høgst 51 s på macOS og 39 s på Linux). Tidene varierer med ±3 s mellom køyringar.
- **W6-testane** er 6–10 s tregare enn i W6: demoen byggjer no ein modul på 52 KB (den lokale kopien, `std/wasm_lager.no` og SHA-256) i staden for 27 KB.
- **`test_wasm_w7`** hadde først bygga av testklienten og demoen med (60 s i fastmodus); dei er flytte til `test_wasm_w7_chrome` og `test_wasm_w7_demo`, som byggjer dei uansett, og regresjonen for rt-cachen brukar den minste modulen (`wasm_w7_utan_idb`).
- Linux (Docker `nc-x86tools`, stage0 frå `bootstrap/`, eigen `build/`) har ikkje Chrome eller jsc, så dei stega er SKIP der. VM-, bygg- og tenarstega køyrer (også ryddetesten til benken og JSON-rutene til demoen). Alle testane er grøne på begge plattformene og i begge modusane.

## Gjenstår før W9

- **W8 (nytt):**
  - Berre Chrome 154 (headless) er testa med service workeren. Installerbarheita («Installer app» og Application → Manifest i DevTools) er ikkje stadfesta i ekte Chrome; manifestet og ikona er validerte i Norscode, og ikona er dekoda av Chrome (`Image.decode`). Safari og Firefox er ikkje testa, og Safari kan slette data for nettstader som ikkje er installerte.
  - Service workeren krev HTTPS utanom `localhost`/`127.0.0.1`. Kven som terminerer TLS i produksjon, er framleis eit ope spørsmål.
  - Produksjonsbygget kan ikkje testast ende til ende i headless Chrome: utan ein puls blir ikkje installasjonen ferdig før DOM-en blir dumpa. Testbygget køyrer den ekte klienten og workeren.
  - Den fulle navigasjonen etter aktiveringa er slått av i Chrome-testen (headless dumpar ikkje etter ein navigasjon); avgjerda er testa i VM-en, og `vert.gå_til` er testa i W4.
  - «Éi fane» er talet på vindauge `clients.matchAll({includeUncontrolled: true})` gjev når sida sender `nc-auto`; to faner er ikkje testa (`--dump-dom` har éi fane).
  - Versjonen blir rekna ut på tenaren for kvar `/sw.js` (sha256 over om lag 10–20 KB); app-skalet må vere det same som tenaren leverer (appen gjev kroppen i `med_skal`). Leverte ikon blir validerte (PNG-sjekk) i kvar `/sw.js`, om lag 0,35 s for 512 px i fastmodus.
  - Precachen blir henta med `Promise.all` mot `nc serve`, som tek éi tilkopling om gongen; det tek om lag 5 s første gong (mest dei to 512-ikona, 0,4 s kvar i fastmodus). Ein apptenar (A6/R1) og cache av ikona på tenaren ville korte det ned.
  - SW-lastaren er 1 751 byte (tak 1 792) mot 1,5 KB i planen, sjå «Storleikar (W8)».
  - Fragment-førespurnader (ei offline-side per fragment, A4) og NSP/1-synk i workeren er ikkje med (W9).
  - Kill switch-en slettar cachane, men sida avregistrerer ikkje workeren.

- **W7 (nytt):**
  - IndexedDB er berre testa i Chrome 154. JavaScriptCore-skalet har ikkje IndexedDB (ende til ende er SKIP der; berre vegen utan IndexedDB er testa), og Safari og Firefox er ikkje testa med lagring.
  - Chrome med `--virtual-time-budget` ventar ikkje på IndexedDB. Testklientane held ein puls mot tenaren (`/api/puls`); ein testmodus utan puls krev CDP eller at benken sjølv held ein førespurnad open. Produksjonsbygget av demoen kan difor ikkje testast for den lokale kopien (det blir dumpa før IndexedDB svarar); testbygget køyrer den ekte klienten.
  - `kvote` og `blokkert` er klassifiserte og har meldingar i demoen, men er ikkje provoserte i ein test (kvoten krev store mengder data; `blocked` krev ein fane med eldre kode, sidan tilkoplingane frå `std/wasm_lager.no` lukkar seg ved `versionchange`).
  - Ei «blokkert»-opning blir levert som feil med ein gong. IndexedDB fullfører ho kanskje seinare; tilkoplinga blir då lukka, men appen må opne på nytt sjølv.
  - Tilkoplinga som lukkar seg ved `versionchange`, seier ikkje frå til Norscode: neste `køyr` får `transaksjon` (`InvalidStateError`). W9 bør gje ei hending («databasen vart oppgradert i ein annan fane; last sida på nytt»).
  - Ingen markør (cursor): `liste` med grense har ingen retning (nyaste først) og ingen side-for-side-lesing. W9 (synk) treng truleg `openCursor` med retning, eller ein indeks på tid.
  - Nøklar er tal og tekst, ikkje samansette (lister) og ikkje bytar. Samansette indeksar (t.d. `[samling, endra]`) må lagast som tekst.
  - Verdiar er JSON-tekst: `Date`, `Blob` og bytar blir ikkje lagra direkte (bytar kan lagrast som hex eller base64).
  - Lagrings-gruppa er 1 256 byte (eige budsjett 1 280). Heile produksjonstabellen med nett og lagring er 4 211 byte.
  - rt-cachen vart retta for funksjonar med closures og `ncb_call_fn` (sjå «Funn: rt-cachen og ncb_call_fn»); cachefilene blir framleis aldri rydda.
  - Utlogging slettar databasen og nøklane appen nemner; han veit ikkje om andre databasar eller nøklar appen har laga.

- **W5/W6:**
  - `nc serve` tek éi tilkopling om gongen. Parallelle førespurnader med avbrot eller tidsavbrot medan dei ventar på ei tilkopling kan få tenaren til å vente på ein sokkel utan førespurnad (sjå «Nett»). Krev ein apptenar med fleire tilkoplingar (A6/R1); til då: høgst seks om gongen og avbrot etter at førespurnaden er skriven.
  - Tidsgrenser kan berre testast i Chrome med virtuell tid når sida sjølv held klokka i gang (testklienten ventar aktivt i 60 ms). Ein testmodus utan virtuell tid krev CDP eller ein ekstra ressurs som held `load` att.
  - Offline-modusen i benken byggjer no med service workeren (W8), men den første sida må halde nettet i gang til workeren er klar (testbygg med puls).
  - `web.request_header` og `web.request_cookie` skil mellom store og små bokstavar under `nc serve` (demoen går rundt det). Bør rettast i `std/web.no` (utanfor lukkinga) i eit eige steg.
  - Nett-gruppa i lastaren har eige budsjett (448 byte). Heile produksjonstabellen med nett er 2 955 byte.
  - Firefox er framleis ikkje testa. Safari-motoren (jsc) er testa for korpusprogram utan DOM; demoane treng ein DOM og er berre køyrde i Chrome.
  - `hent_*` og `sett_*` på eit element som ikkje finst, gjev framleis ein JS-feil («Uncaught»), ikkje eit Norscode-unnatak (testklienten for demoen måtte unngå det).
  - Sidelastingstida kjem frå Chrome-netloggen; `nc serve` loggar ikkje førespurnader.

- **Nettlesarar:**
  - Skjema- og deltakar-demoen er køyrde i Chrome 154, men ikkje i JavaScriptCore (dei treng ein DOM). Korpusprogramma køyrer i JSC via benken (`jsc -e`, W5); scratch-skriptet frå W2–W4 trengst ikkje lenger.
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
