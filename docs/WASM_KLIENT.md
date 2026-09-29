# Klientlogikk i Norscode kompilert til WebAssembly (W0)

Status: milepæl W0 i WASM-sporet (app-kjensle del 2). Klientkode blir skriven i Norscode og kompilert til ein WasmGC-modul. Nettlesaren startar modulen med ein liten lastar som blir **emittert frå Norscode-data**. Det finst ingen handskriven JavaScript i repoet.

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

Verktøyet skriv éi linje: `WASM: app.wasm <n> byte, lastar <m> byte, v=<12 hex>`.

## Modular

| Fil | Rolle |
|---|---|
| `std/wasm_lowring.no` | NCB → WASM |
| `std/wasm_les.no` | Strukturell validator |
| `std/wasm_vert.no` | Vertstabell, klient-API og HTML-escape |
| `std/wasm_js.no` | Norscode→JS-emitter for lastaren |
| `tools/nc_wasm.no` | CLI |

Ingen av dei ligg i `nc_main`-lukkinga, så dei krev ingen reseed.

## Verdimodell (WasmGC)

- Alle verdiar er `eqref`.
- `heltall` er `i31ref` innanfor −2³⁰…2³⁰−1, og elles `struct {i64}`.
  - Representasjonen er kanonisk, så likskap samanliknar talverdien.
  - Divisjon og rest trunkerer som i VM-en.
- `bool` er to singleton-structar.
- `null` er `ref.null eq`.
- Nettlesarkrav: Chrome 119+, Firefox 120+ og Safari 18.2+. Eldre nettlesarar viser sida utan klientlogikk.

### W0-subsettet

Støtta:
- heiltal, lokale variablar og modulglobalar;
- `hvis`/`mens`, `og`/`eller`/`ikkje`;
- kall og rekursjon;
- vertsfunksjonane nedanfor.

Tekst finst berre som **konstantar** til vertsfunksjonar.

Alt anna gir kompileringsfeil med ei liste over alle funna. Det gjeld ukjende opkodar, `builtin.*`, flyttal, tekst som verdi og lister. Det finst ingen stille fallback.

## Vertsfunksjonar (`bruk std.wasm_vert som vert`)

| Funksjon | DOM-operasjon |
|---|---|
| `vert.finn("#id")` | `document.querySelector` → handtak |
| `vert.sett_tekst(h, tal)` | `textContent` |
| `vert.sett_html(h, "mal med {}", tal)` | `innerHTML`, alltid med escape (sjå under) |
| `vert.lytt_klikk(h, "funksjonsnamn")` | `click`-lyttar som kallar funksjonen i same modul |
| `vert.test_klikk(h)` | `element.click()`, berre med `NC_WASM_TESTBYGG=1` |

Tabellen i `vertsfunksjonar()` skildrar kvar funksjon med namn, parametertypar, resultat og DOM-operasjonen som eit JS-uttrykkstre.

`std/wasm_js.no` byggjer lastaren token for token frå tabellen. Berre importerte funksjonar og hjelparane dei brukar kjem med.

Teljar-lastaren er 529 byte. Taka blir testa i `tests/test_wasm_lastar.no`:
- 600 byte for teljarsettet;
- 400 byte for eitt import;
- 700 byte for heile tabellen.

## HTML-innsetjing og escaping

`sett_html` er den einaste vegen til `innerHTML`.

- **Malen** må vere ein tekstkonstant i kjelda, det vil seie HTML som utviklaren har skrive. Kompilatoren avviser alt anna.
- **Verdien** blir alltid sendt gjennom escape-hjelparen `H` i lastaren før han blir sett inn i staden for `{}`.
  - `H` blir emittert frå `std.wasm_vert.escape_tabell()`: `&`, `<`, `>`, `"` og `'`.
  - Det er same tabell som `std.html.escape` brukar.
  - `wasm_vert.html_escape` gir identisk tekst i VM-en. Testen påstår likskapen.

Brukardata kan dermed ikkje nå `innerHTML` uescapa. Når tekstverdiar kjem i W1, skal dei gå same vegen.

## CSP, versjonering og cache

- Sider som lastar WASM, treng `script-src 'self' 'wasm-unsafe-eval'`.
  - I demoen er det berre `/` som får han. `/om` og `/kontroll/streng-csp` har standard-CSP.
  - `'wasm-unsafe-eval'` opnar ikkje for `eval` eller `new Function`.
- `v` for modulen er dei 12 første hex-teikna av `sha256_bytes` over bytane. `sha256(tekst(liste))` kolliderer og blir ikkje brukt.
- `v` for lastaren er sha256 over lastarteksten.
- Rett `v` gir `Cache-Control: public, max-age=31536000, immutable`. Feil eller manglande `v` gir `no-cache`.
- `std/http_cache.no` (PR #206) finst ikkje på denne greina, så headerane blir sette direkte i `examples/wasm_teljar/app.no`.

## Testar

| Test | Kva han dekkjer |
|---|---|
| `tests/test_wasm_w0.no` | Validatoren godtek fasitmodular og avviser éin feil om gongen. Korpuset (`tests/wasm_korpus/`) blir kompilert og validert. `minimal.no` er byte-eksakt. Rapporten er fail-closed. Versjonen følgjer bytane. |
| `tests/test_wasm_lastar.no` | Byte-tak og tree-shaking. Importnamna i lastaren er lik importseksjonen. Strukturen kjem frå tabellen. Ingen JS-fragment i literalane. Kvart lastartoken finst i emitteren eller i tabellen (proveniens). Escape er lik `std.html.escape`. |
| `tests/test_wasm_serve.no` | Barne-`nc serve` leverer `application/wasm` byte-identisk, med versjonert cache og CSP per side. |
| `tests/test_wasm_w0_chrome.no` | Valfri. Seier SKIP utan Chrome, og `NC_CHROME` vel binær. Headless Chrome køyrer med flagga frå planen (§4.4). Testen sjekkar teljaren etter tre klikk, gjer ein negativ CSP-kontroll og samanliknar korpusverdiar med VM-en. |
