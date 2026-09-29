# Klientlogikk i Norscode kompilert til WebAssembly (W0, W1)

Status: milepælane W0 og W1 i WASM-sporet (app-kjensle del 2). Klientkode blir skriven i Norscode og kompilert til ein WasmGC-modul. Nettlesaren startar modulen med ein liten lastar som blir **emittert frå Norscode-data**. Det finst ingen handskriven JavaScript i repoet.

- **W0** gav vegen frå kjelde til nettlesar: heiltal, kontrollflyt, kall og DOM-vertsfunksjonar.
- **W1** gav verdimodellen: tekst, desimaltal, lister og ordbøker, med same semantikk som VM-en. Eit paritetskorpus køyrer kvart program i VM-en og i Chrome og krev identisk utskrift.

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
| `std/wasm_lowring.no` | NCB → WASM: analyse, dispatch-lykkje, seksjonar |
| `std/wasm_atom.no` | Runtime-atom (W1): verdimodellen som WASM-funksjonar, skrivne med ein liten assembler |
| `std/wasm_les.no` | Strukturell validator |
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
| `ordbok` | `6 $ordbok struct {mut i32 n, mut ref $arr nøklar, mut ref $arr verdiar}` | Innsetjingsordna. Ein overskriven nøkkel held plassen sin. Oppslag er lineære i W1 (hashtabell i W10). |
| `null` | `ref.null eq` | |

Semantikken ligg i **atom**: små WASM-funksjonar i `std/wasm_atom.no`, t.d. `add`, `lik`, `cmp`, `tekst_av`, `indeks_hent` og `ordbok_set`.

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
- Deling på `0` eller `0.0` gjev `DivisjonMedNull: divisjon med null ved <funksjon>`. For `%` blir det `ModuloMedNull: …`.
- `MIN / −1` gjev `MIN`.
- Er divisoren ein heiltalskonstant ≠ 0, manglar feilvegen (`div_k`/`mod_k`). Då treng modulen ingen import.

**`==` og `!=`**
- Tal blir samanlikna som tal: `1 == 1.0`.
- Tekst blir samanlikna byte for byte.
- Lister blir samanlikna strukturelt.
- Ordbøker blir samanlikna på referanse.
- Ulike typar er ulike: `1 != "1"`, `sann != 1`, `null != 0`.

**`<`, `<=`, `>` og `>=`**
- Tal og tal blir samanlikna som tal.
- Tekst og tekst blir samanlikna bytevis, utan forteikn, så `"å" > "z"`.
- `null` tel som 0 mot tal og mot `null`.
- Alle andre kombinasjonar gjev `usann`.

**Sanning** (`hvis`, `ikkje`, `og`, `eller`)
- `usann`, `null`, `0`, `0.0` og `""` er usanne.
- Alt anna er sant, også `[]` og `{}`.

**`tekst(x)`**
- `null` gjev `""`.
- Bool gjev `sann` eller `usann`.
- Liste og ordbok gjev `[verdi]`, som i VM-en.
- Desimaltal blir formaterte som i VM-en (`std/desimaltall_tekst.no`):
  - Kortaste siffer som les attende til same bit.
  - Heiltalsverdi under 2⁵³ utan `.0`.
  - Fast form for 10⁻⁵ < |x| < 10¹⁷.
  - Elles `1e20` eller `1.25e-5`.
  - `inf`, `-inf` og `nan`.
  - Sifra kjem frå verten (`Number.prototype.toExponential`, som gjev dei kortaste). WASM-sida gjer resten av formateringa.

**Indeksering**
- `liste[i]` utanfor gjev `IndeksFeil: indeks i utanfor lista (lengde n) ved <funksjon>`.
- `tekst[i]` gjev éin byte, eller `""` utanfor.
- `ordbok[k]` gjev `null` når nøkkelen manglar.

**`heltall(tekst)`**
- Godtek berre `-` og siffer.
- Overflyt wrappar som i VM-en: `99999999999999999999` gjev `7766279631452241919`.
- Ugyldig tekst gjev `Ugyldig heiltall: <tekst>`.

**`for k i d` over ei ordbok**
- Compileren på denne greina emitterer ikkje `for_iterabel`. Løkka les difor `d[0]`, `d[1]`, … både i VM-en og i WASM, og tekstnøklar gjev `null`.
- Bruk `for k i nøkler(d)`.
- `builtin.for_iterabel` er likevel støtta, med ordbok → nøklar, for når compileren tek han i bruk igjen.

### Feil

Den **VM-definerte feilen** blir skriven med same tekst som VM-en, `ERROR: <melding>`, gjennom `logg`. Deretter stoppar modulen med `unreachable`. Unnatak som kan fangast, kjem i W2.

**Typefeil som VM-en ikkje har definert** stoppar med ein rein trap. Det gjeld t.d. `liste - 1` og `lengde(5)`. VM-en gjev søppelverdiar i desse tilfella.

### Der VM-ane er usamde

VM-en er ikkje eintydig for alle kombinasjonar. Det blei målt med `nc run` på macOS-arm64-seeden (`dist/`) og Linux-x86-64-stage0:

| Uttrykk | macOS-arm64 | Linux-x86-64 | WASM (W1) |
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

## Subsett (W1)

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

**Builtins** (`builtin_tabell` i `std/wasm_lowring.no`)
- `skriv` og `lengde`.
- `tekst` og `tekst_fra_heltall`.
- `heltall` og `heltall_fra_tekst`.
- `legg_til`, `har_nokkel` (også `finnes_nøkkel`) og `nøkler`.
- `slice`, `char_code` og `chr`.
- `type` og `for_iterabel`.

Alt anna gjev kompileringsfeil med ei liste over alle funna. Det gjeld:
- unnatak (`TRY_BEGIN`, `THROW` og liknande), som kjem i W2;
- closures;
- `område`, som VM-en heller ikkje har;
- ukjende builtins.

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
| Typisk W1-program i testbygg (`logg_test`) | 269–278 byte | |
| Med `flyt_tekst` òg | 389 byte | |

## HTML-innsetjing og escaping

`sett_html` er den einaste vegen til `innerHTML`.

- **Malen** må vere ein tekstkonstant i kjelda, det vil seie HTML som utviklaren har skrive. Kompilatoren avviser alt anna.
- **Verdien** blir alltid sendt gjennom escape-hjelparen `H` i lastaren før han blir sett inn i staden for `{}`.
  - `H` blir emittert frå `std.wasm_vert.escape_tabell()`: `&`, `<`, `>`, `"` og `'`.
  - Det er same tabell som `std.html.escape` brukar.
  - `wasm_vert.html_escape` gir identisk tekst i VM-en. Testen påstår likskapen.

Brukardata kan dermed ikkje nå `innerHTML` uescapa. I W1 tek `sett_tekst` og `sett_html` framleis berre heiltal. Når dei får tekstverdiar (W4), skal dei gå same vegen. Parametertypen `verdi` finst alt, med postkasse og `S` i lastaren.

## CSP, versjonering og cache

- Sider som lastar WASM, treng `script-src 'self' 'wasm-unsafe-eval'`.
  - I demoen er det berre `/` som får han. `/om` og `/kontroll/streng-csp` har standard-CSP.
  - `'wasm-unsafe-eval'` opnar ikkje for `eval` eller `new Function`.
- `v` for modulen er dei 12 første hex-teikna av `sha256_bytes` over bytane. `sha256(tekst(liste))` kolliderer og blir ikkje brukt.
- `v` for lastaren er sha256 over lastarteksten.
- Rett `v` gir `Cache-Control: public, max-age=31536000, immutable`. Feil eller manglande `v` gir `no-cache`.
- `std/http_cache.no` (PR #206) finst ikkje på denne greina, så headerane blir sette direkte i `examples/wasm_teljar/app.no`.

## Storleikar (W1)

Modulane er like store på macOS og Linux. Tala er i byte, og W0-tala står i parentes.

| Modul | Storleik |
|---|---|
| `minimal` | 686 (211) |
| teljar | 2028 (899) |
| `aritm` | 3427 (1635) |
| `kontroll` | 2790 (1243) |
| `fak` | 3156 |
| `tekst_utf8` | 4370 |
| `flyt` | 5227 |
| `liste` | 5751 |
| `ordbok_orden` | 4450 |
| `blanda` | 4744 |
| `feil_div` | 1144 |

Mesteparten av veksten i heiltalsprogramma kjem av at `+` er polymorf. Han dreg med seg `tekst_av`, `fmt_i64` og `konkat` når operandtypane ikkje er kjende. Typeinferens for dette er ein W10-oppgåve.

## Testar

| Test | Kva han dekkjer |
|---|---|
| `tests/test_wasm_w0.no` | Validatoren godtek fasitmodular og avviser éin feil om gongen. Korpuset blir bygd med CLI-en, med dei venta importane og eksportane. `minimal.no` er byte-eksakt både i testprosessen og frå CLI-en. Rapporten er fail-closed (ni feil i `feil_ustotta.no`, inkludert `ustøtta builtin: ukjend`). Versjonen følgjer bytane. |
| `tests/test_wasm_w1.no` | GC-fasitmodulen frå proben blir bygd på nytt med assembleren og er byte-eksakt. Kodingane til GC-typane og instruksjonane er rette. W1-validatoren (datacount, array-typar) avviser éin feil om gongen. Tree-shakinga er rett: `logg`, `flyt_tekst`, `mod_k` utan feilveg og `logg_test` i testbygg. |
| `tests/test_wasm_korpus_chrome.no` | Paritetskorpuset, sjå under. |
| `tests/test_wasm_lastar.no` | Byte-tak og tree-shaking. Importnamna i lastaren er lik importseksjonen. Strukturen kjem frå tabellen. Ingen JS-fragment i literalane. Kvart lastartoken finst i emitteren eller i tabellen (proveniens). Escape er lik `std.html.escape`. Interne importar er med. |
| `tests/test_wasm_serve.no` | Barne-`nc serve` leverer `application/wasm` byte-identisk, med versjonert cache og CSP per side. |
| `tests/test_wasm_w0_chrome.no` | Valfri. Teljaren etter tre klikk, negativ CSP-kontroll og W0-korpuset mot VM-en. |

Chrome-hjelparane ligg i `tests/fixtures/wasm_chrome_hjelp.no`, og korpushjelparane i `tests/fixtures/wasm_korpus_hjelp.no`.

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

Kvart program har ein **kjend fasit** i `#=`-linjer, og `test_wasm_korpus_chrome` køyrer dei i tre steg:

1. Køyrer kvart program i VM-en (barne-`nc run`) og krev at stdout er lik fasiten.
2. Byggjer kvart program i testbygg med CLI-en. Modulen blir validert og må importere `logg_test`.
3. Startar éin barne-`nc serve` med ei side per program. Headless Chrome køyrer sidene, og teksten i `<pre id="nc-logg">` må vere identisk med VM-utskrifta. Utan Chrome blir dette steget SKIP. `NC_CHROME` vel binær.

Resultatet er at alle ni programma gav identisk utskrift i VM-en og i Chrome 154 (macOS). Linux i Docker har ikkje Chrome, så der køyrer steg 1 og 2.

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

### Tid

Tida er målt på macOS, i sekund. Tala i parentes er W0.

| Test | Standard-VM | Fastmodus |
|---|---|---|
| `test_wasm_w0` | 27 (22) | 4 |
| `test_wasm_lastar` | 24 (35) | 3 |
| `test_wasm_w1` | 10 | 3 |
| `test_wasm_korpus_chrome` | 43 | 16 |

Testane byggjer W1-modulane med CLI-en i fastmodus i staden for å lowre og validere fleire KB i standard-VM-en.

## Gjenstår før W2

- **Unnatak:** `TRY_*`, `THROW` og `FINALLY_*`. Feil som kan fangast, med `try_table`/exnref. Typa unnatak.
- **Closures og metodar:** `BUILD_LAMBDA`, `CALL_VALUE` og `ncb_call_fn`.
- **Validator:** stakk- og typesjekk (W1 er framleis strukturell).
- **W3 og seinare:** tekstverdiar til `sett_tekst`/`sett_html`, og `split`, `join` og `json` i `std/wasm_rt.no`.
- **W10:** typeinferens som gjer `+` monomorf og modulane mindre, og hashtabell i ordbøkene.
