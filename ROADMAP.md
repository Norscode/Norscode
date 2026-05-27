# Norscode — Vegkart

> Sist oppdatert: mai 2026  
> Status: selfhost-pipeline under ferdigstilling; fase 0 er lokalt verifisert, men GitHub Actions-bekreftelse gjenstår

---

## Nåværende tilstand

| Komponent | Tilstand |
|---|---|
| Lexer (`selfhost/lexer/lexer_m1.no`) | ✅ Aktiv, kjørbar |
| Parser (`selfhost/parser.no`) | ✅ Aktiv, kjørbar |
| Semantikk (`selfhost/compiler/semantic.no`) | ✅ Aktiv, kjørbar |
| IR → Bytecode (`selfhost/compiler/ir_to_bytecode.no`) | ✅ Aktiv, kjørbar |
| VM (`selfhost/vm.no`) | ✅ Aktiv, kjørbar |
| CLI (`selfhost/main.no`) | ✅ Aktiv, kjørbar |
| Python bootstrap/runtime (`compiler/`, `norcode/legacy_main.py`, `norcode/bootstrap/python_entry.py`) | 🔄 Legacy fallback — begrenset og eksplisitt |
| CI (GitHub Actions) | 🟡 `phase0-verify` er på plass; ekstern grønn-bekreftelse gjenstår |

---

## Fase 0 — Rydding og CI-stabilisering (blokkerer alt)

Ingenting kan gå framover med CI-feil. De kjente blokkeringene er ryddet ut av workflowene, og det finnes nå en egen `phase0-verify`-gate. Resten er å bekrefte grønn CI i GitHub Actions og fullføre siste opprydding i fase 0 og den gjenværende Python-bootstrap-flaten.

Se også [docs/SELFHOST_PHASE0_REMAINING_ROUNDS.md](docs/SELFHOST_PHASE0_REMAINING_ROUNDS.md) for en kort, omgangsbasert arbeidsliste over det som gjenstår.
Verifiser sluttstatus med `bash tools/selfhost_phase0_verify.sh`.
For en samlet plan for full selvstendighet, se [docs/SELFHOST_FULL_AUTONOMY_PLAN.md](docs/SELFHOST_FULL_AUTONOMY_PLAN.md).

**CI-endringer som ble gjort:**

1. **macOS** — `./bin/nc test` var en dårlig vei for denne jobben.  
   Status: byttet til `./bin/nc --legacy-python-fallback selfhost-bootstrap-gate` i `ci.yml`.

2. **Linux** — Docker-byggesteget refererte slettet C-kode.  
   Status: Docker-steget er fjernet fra release-flyten, og `tools/docker-build-linux.sh` er nå en legacy-wrapper rundt native bootstrap-pakken.

3. **Windows** — smoke-testen er nå rettet mot en ren test.  
   Status: `ci.yml` bruker en ren test uten `std.web`, så denne blokkeren er ryddet.

**Daud kode som allereie er fjerna eller gjort overflødig:**
- `norsklang/` (3 filer) — aldri i bruk, gamalt alias
- `setup.py` — erstatta av `pyproject.toml`
- `tests/test_ci_selfhost_ready_gate.py` og 4 andre pytest-filer som refererer slettet C VM
- `tools/export_ncbb_as_bin.py`, `export_ncbb_as_c.py`, `generate_linux_bootstrap_artifacts.py`, `v42_trace_probe.py`
- `build/norcode-native-stage0.py`

**Leveranse:** Lokal verifikasjon er på plass; neste milepæl er bekreftet grønn CI i GitHub Actions.

---

## Fase 1 — Fullføre selfhost

**Mål:** Norscode kompilerer seg selv uten Python. Python-VM kan fjernes.

Selfhost-kjeden (`lexer → parser → semantic → ir_to_bytecode → vm`) er teknisk på plass, men har hull:

### 1.1 Verifisering av selfhost-pipeline ende-til-ende

- Kjør `selfhost/main.no compile selfhost/parser.no` og verifiser at outputen er gyldig NCB JSON
- Kjør `selfhost/main.no compile selfhost/compiler/ir_to_bytecode.no`
- Kjør `selfhost/main.no compile selfhost/vm.no`
- **Kriterium:** Alle tre produserer NCB uten feil

### 1.2 Manglende VM-instruksjoner og builtins

`selfhost/vm.no` mangler sannsynligvis støtte for:
- `json_parse` / `json_stringify` (brukes i ir_to_bytecode.no)
- `liste_sorter`, `tekst_del`, `tekst_erstatt` og øvrige streng-builtins
- Rekursiv kalling av funksjoner i andre moduler (cross-module dispatch)

Strategi: Legg til manglende builtins i `kall_innebygd()`-funksjonen i vm.no fortløpende etter kvar feil.

### 1.3 Bootstrap-sekvens

Tre steg for å nå Python-frihet:

```
Steg A: Python VM kompilerer selfhost/ → produserer nc.ncb (NCB JSON)
Steg B: nc.ncb kjøres av selfhost/vm.no (via Python VM) — verifiserer at NCB-en er korrekt
Steg C: nc.ncb kjøres av nc.ncb selv (bootstrapped) — ingen Python i løkken
```

Verktøy: `selfhost/bootstrap_gate.no` allerede på plass for steg A/B.  
Nytt arbeid: Steg C — `bin/nc bootstrap-self` som kjører selfhost-VM med selfhost-kompilator.

### 1.4 Slett Python

Etter at steg C er verifisert i CI:
- Slett `compiler/`, `norcode/`, `main.py`
- `tools/bootstrap_wrapper.py` konverteres til ren shell-wrapper (kaller nc.ncb direkte)
- `pyproject.toml` arkiveres eller simplificeres til kun metadata

**Blokkeres av:** Fase 0  
**Leveranse:** `./bin/nc compile selfhost/parser.no` uten Python i PATH

---

## Fase 2 — Typesystem-stabilisering

**Mål:** Kompileringstidsfeil for typefeil. Norskkode vet hva den opererer på.

### 2.1 Type-inferens (Hindley-Milner-delmengde)

Nåværende tilstand: `selfhost/type_inference.no` finst som fil men er uverifisert.

Prioritert delmengde:
- Inferens for `la`-bindinger, funksjonsargumenter og returtyper
- Enkel unifikasjon: `heltall`, `desimaltall`, `tekst`, `boolsk`, `liste[T]`, `ordbok[K,V]`
- Typefeil som stopper kompileringen med linje/kolonne-info

### 2.2 Strukturtyper

```norcode
struktur Punkt {
    x: desimaltall
    y: desimaltall
}
```

- Felt-tilgang typekontrollert
- Strukturkonstruktørar typesjekket
- Ingen implisitt `null` — eksplisitt `Valgfri[T]`

### 2.3 Union-typer og mønstermatching

```norcode
type Resultat = Ok(verdi: heltall) | Feil(melding: tekst)
```

- Uttømmende matching verifisert ved kompilering
- Brukes til å erstatte `prøv/fang` der det gir mening

### 2.4 Generics (parametriske typer)

```norcode
funksjon første[T](liste: liste[T]) -> Valgfri[T]
```

- Monomorfiering ved kompilering (ingen runtime-overhead)
- Typesjekk ved bruk

**Blokkeres av:** Fase 1  
**Leveranse:** `selfhost/type_inference.no` kjøres som pass i kompilerpipelinen. Standard testfiler uten typekommentarer kompilerer fortsatt.

---

## Fase 3 — Deterministisk runtime-stabilisering

**Mål:** Samme input → samme output, alltid, på alle plattformer.

### 3.1 Definerte grenser og overflow

- `heltall` er 64-bit signed (som i dag) — dokumenter dette eksplisitt
- Overflow kaster `OverflowFeil` heller enn undefined behavior
- `desimaltall` følger IEEE 754 strikt

### 3.2 Kanonisk map-iterasjonsrekkefølge

I dag er `ordbok`-iterasjon Python-dict-rekkefølge (innsettingsrekkefølge). Det spesifiseres eksplisitt i språkstandarden og håndheves av VM.

### 3.3 Feilhåndtering — klargjør `prøv/fang`

- Definer hvilke operasjoner kan kaste
- `fang`-grener spesifiserer feiltype
- Ukjente feil propageres ikke stille

### 3.4 Reproduserbare bygg

- Kompileringsoutput er deterministisk (ingen timestamp, ingen hash av adresser)
- Samme kildekode → byte-for-byte identisk NCB JSON på ulike maskiner
- Verifikasjon: CI-jobb som kompilerer to ganger og diff-er output

**Blokkeres av:** Fase 1  
**Leveranse:** `nc build --verify-deterministic fil.no` passerer i CI på Linux og macOS.

---

## Fase 4 — Pakkebehandler

**Mål:** `norcode.toml` fungerer som `cargo.toml` — deklarativ, reproduserbar.

Grunnlag allerede på plass: `norcode.toml` har `[registry]`-seksjon.

### 4.1 Lokal pakkeoppløsning

```toml
[avhengigheter]
std = { versjon = "1.0" }
http = { sti = "../norhttp" }
```

- Pakker er mapper med `norcode.toml` og `src/`
- Ingen nettverkstilgang nødvendig for lokale pakker

### 4.2 Låsefil

`norcode.lock` — pinnes nøyaktige versjoner og sjekksum av alle avhengigheter.  
Låsefilen committes. CI verifiserer at den er oppdatert.

### 4.3 Registry-protokoll

- HTTP GET `registry/<pakke>/<versjon>/manifest.json`
- Manifest inneholder: `navn`, `versjon`, `filer[]`, `sha256`
- Ingen eksekverbar kode i registry — bare kildekode og metadata

### 4.4 Publisering

```
nc publiser --token $NORCODE_TOKEN
```

- Signerer pakken med brukerens nøkkel
- Laster opp til registry
- Automatisk versjonsbump fra `nc versjon patch|minor|major`

**Blokkeres av:** Fase 2 (typesjekk må fungere for pakke-APIer)  
**Leveranse:** `nc hent` laster inn avhengigheter, `nc bygg` bruker dem.

---

## Fase 5 — LSP / IDE-støtte

**Mål:** Go-to-definition, hover, autocomplete og feilmarkering i VS Code og Neovim.

### 5.1 Language Server

Implementeres som `selfhost/lsp/server.no`:
- Kommunikasjon over stdin/stdout (JSON-RPC 2.0)
- Inkrementell parsing (filen holdes i memory, oppdateres på keystrokes)
- Bruker typeinfo fra Fase 2

Minimalt sett av capabilities:
- `textDocument/publishDiagnostics` — syntaks- og typefeil i sanntid
- `textDocument/hover` — type og dokumentasjon ved cursor
- `textDocument/definition` — hopp til definisjon
- `textDocument/completion` — variabel- og funksjonsforslag

### 5.2 VS Code-utvidelse

- Syntax highlighting (TextMate grammar for `.no`-filer)
- Kobler til `nc lsp` som language server
- Publiseres til VS Code Marketplace

### 5.3 Neovim / helix

- `nvim-lspconfig`-konfig dokumentert
- Helix language-config dokumentert

**Blokkeres av:** Fase 2 (typeinfo nødvendig for hover/autocomplete)  
**Leveranse:** VS Code-utvidelse i Marketplace, Neovim-konfig i `docs/editor/`.

---

## Fase 6 — Dokumentasjonsside

**Mål:** `norscode.dev` — komplett språkreferanse, tutorial og stdlib-docs.

### 6.1 Innhold

- **Kom i gang** — installer, skriv første program, kjør det
- **Språkreferanse** — alle syntaks-konstrukter med eksempler
- **Standardbibliotek** — auto-generert fra `std/`-kildekode og doc-kommentarer
- **Norskspråklig terminologi** — ordliste for alle begreper

### 6.2 Dokumentasjons-kommentarer i kildekode

```norcode
/// Returnerer kvadratroten av `x`.
/// Kaster `MatematiskFeil` hvis `x < 0`.
funksjon kvadratrot(x: desimaltall) -> desimaltall
```

Kompilator trekker ut `///`-kommentarer og genererer JSON-manifest.

### 6.3 Statisk site

Bygget med en statisk site-generator (kandidat: skrive en minimal en i Norscode selv).  
Hostes på GitHub Pages eller Cloudflare Pages.

**Blokkeres av:** Fase 2 (doc-kommentarer trenger typeinfo)  
**Leveranse:** `norscode.dev` live med komplett Fase 1-2 dekning.

---

## Fase 7 — WebAssembly-pipeline

**Mål:** `.no`-filer kompilerer til WASM og kjøres i nettleser og Deno/Node.

### 7.1 WASM-backend

Nytt kompileringssteg etter IR: `selfhost/backend/wasm_emitter.no`  
Produserer gyldig `.wasm` binary direkte (ingen LLVM, ingen Emscripten).

Støtte for:
- Alle primitive typer mappes til WASM `i64`/`f64`
- Funksjoner eksporteres med `#[wasm_eksport]`-attributt
- Heap-allokasjon via lineær memory med enkel bump-allocator

### 7.2 JS-interop

```norcode
#[wasm_eksport]
funksjon legg_til(a: heltall, b: heltall) -> heltall {
    returner a + b
}
```

Genererer automatisk TypeScript-deklarasjonsfil (`.d.ts`).

### 7.3 Playground

Online-editor på `norscode.dev/prøv` — skriver Norscode, kjører i WASM i nettleseren.

**Blokkeres av:** Fase 3 (determinisme nødvendig for WASM-paritet)  
**Leveranse:** `nc bygg --mål wasm fil.no` produserer kjørbar `.wasm`.

---

## Fase 8 — Native optimizer

**Mål:** Raskere binærer, lavere minnebruk — uten å endre semantikk.

### 8.1 IR-optimeringer (kjøres på NCB-nivå)

- **Konstant-folding** — `2 + 3` → `5` ved kompilering
- **Dead code elimination** — ubrukte funksjoner og variabler fjernes
- **Inlining** — korte funksjoner inlines der det er lønnsomt
- **Loop-optimering** — invariante beregninger heises ut av løkker

### 8.2 Registerallokering (x86_64 og AArch64)

Nåværende native backend bruker en enkel linear scan.  
Byttes ut med graph-coloring allocator for tettere kode.

### 8.3 Profilstyrt optimering (PGO)

```
nc bygg --profil fil.no        # instrumentert bygg
./fil                          # kjøres med representativ input
nc bygg --bruk-profil fil.no   # optimalisert bygg basert på profil
```

### 8.4 Benchmarksuite

`benchmarks/` — kanonisk suite med kjente programmer (fibonacci, sortering, JSON-parsing).  
CI rapporterer ytelse per commit — regresjoner blokkerer merge.

**Blokkeres av:** Fase 3 (semantikk må være stabil før optimering)  
**Leveranse:** Fibonacci(40) kjører ≥ 2× raskere enn fase 1-binary.

---

## Fase 9 — CI/CD release-automatisering

**Mål:** `git tag v1.2.3` → automatisk release med binærer for alle plattformer.

### 9.1 Release-pipeline (`publish.yml`)

Trigges av `v*`-tagger:
1. Bygg macOS arm64 binary (`dist/nc-macos-arm64`)
2. Bygg Linux x86_64 binary via Docker (`dist/nc-linux-x86_64`)
3. Bygg Windows x64 binary (cross-kompilert eller MSVC runner)
4. Bygg WASM-bundle
5. Publiser GitHub Release med alle artefakter
6. Oppdater `norscode.dev` med ny versjon

### 9.2 Versjonsstrategi

Semantisk versjonering: `MAJOR.MINOR.PATCH`
- `MAJOR` — ikke-bakoverkompatible språkendringer
- `MINOR` — nye features, bakoverkompatibelt
- `PATCH` — bugfixes

Versjon leses fra `norcode.toml [prosjekt] versjon`.  
`nc versjon patch` bumper og oppdaterer toml + lager git-tag.

### 9.3 Nightly builds

Automatisk nightly på `main` — publiseres til `nightly.norscode.dev`.  
Brukes av contributors for å teste siste kode.

### 9.4 Kanariutsetting

Ny versjon rulles ut til 5 % av `nc oppdater`-brukere i 24 timer.  
Hvis krasjrate > 0.1 % rulles den tilbake automatisk.

**Blokkeres av:** Fase 1 (trenger stabile binærer)  
**Leveranse:** `git tag v0.2.0` produserer fungerende binærer for alle tre plattformer innen 20 min.

---

## Avhengighetsgraf

```
Fase 0 (CI + rydding)
    └── Fase 1 (selfhost ferdig)
            ├── Fase 2 (typesystem)
            │       ├── Fase 4 (pakkebehandler)
            │       ├── Fase 5 (LSP)
            │       └── Fase 6 (docs)
            ├── Fase 3 (deterministisk runtime)
            │       ├── Fase 7 (WASM)
            │       └── Fase 8 (optimizer)
            └── Fase 9 (CI/CD release)
```

---

## Neste tre ting å gjøre (i dag)

1. **Fix `ci.yml`** — tre kirurgiske endringer (macOS, Linux Docker, Windows smoke test)
2. **Slett daud Python** — `norsklang/`, `setup.py`, 9 filer totalt
3. **Verifiser selfhost-kjeden ende-til-ende** — `selfhost/main.no compile selfhost/parser.no` skal produsere gyldig NCB
