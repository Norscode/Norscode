# Selfhost bootstrap inventory

Mål:
Gjøre det helt tydelig hva som fortsatt er Python-burden i bootstrap-, verktøy- og fallback-flaten, og hva som allerede er på vei ut av den vanlige brukerflyten.

Dette dokumentet er ment som en praktisk kartlegging, ikke en historisk forklaring.

## Kort status

- [x] Gjenværende bootstrap-burden er kartlagt i grupper
- [x] Skillene mellom bootstrap, verktøy og produktlogikk er tydeliggjort
- [x] Filene som mest sannsynlig kan tynnes ut videre er identifisert
- [x] Dokumentet kan brukes som inngang til Omgang 1 i `SELFHOST_REMAINING_ROADMAP`

## Hva som fortsatt er Python i normal- og overgangsflaten

### 1. Bootstrap-entry og wrapper-lag

Disse filene er fortsatt bærere av bootstrap-orienteringen og er de første stedene en ny bidragsyter møter når noe går via Python eller bootstrap-binary:

- [`bin/nc`](/Users/jansteinar/Projects/Norscode/bin/nc)
- [`main.py`](/Users/jansteinar/Projects/Norscode/main.py)
- [`norcode/bootstrap_ci.py`](/Users/jansteinar/Projects/Norscode/norcode/bootstrap_ci.py)
- [`norcode/ci_pipeline.py`](/Users/jansteinar/Projects/Norscode/norcode/ci_pipeline.py)
- [`norcode/bootstrap_support.py`](/Users/jansteinar/Projects/Norscode/norcode/bootstrap_support.py)
- [`norcode/diagnostics.py`](/Users/jansteinar/Projects/Norscode/norcode/diagnostics.py)
- [`norcode/migrations.py`](/Users/jansteinar/Projects/Norscode/norcode/migrations.py)
- [`norcode/commands/release.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/release.py)
- [`norcode/commands/scaffold_api.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/scaffold_api.py)
- [`norcode/commands/fuzz.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/fuzz.py)
- [`norcode/compiler_service.py`](/Users/jansteinar/Projects/Norscode/norcode/compiler_service.py)
- [`norcode/ir_tools.py`](/Users/jansteinar/Projects/Norscode/norcode/ir_tools.py)
- [`norcode/parity_tools.py`](/Users/jansteinar/Projects/Norscode/norcode/parity_tools.py)
- [`norcode/testing_support.py`](/Users/jansteinar/Projects/Norscode/norcode/testing_support.py)
- [`norcode/repl.py`](/Users/jansteinar/Projects/Norscode/norcode/repl.py)
- [`norcode/linting.py`](/Users/jansteinar/Projects/Norscode/norcode/linting.py)
- [`norcode/smoke.py`](/Users/jansteinar/Projects/Norscode/norcode/smoke.py)
- [`norcode/quality_suites.py`](/Users/jansteinar/Projects/Norscode/norcode/quality_suites.py)
- [`norcode/release.py`](/Users/jansteinar/Projects/Norscode/norcode/release.py)
- [`norcode/scaffold.py`](/Users/jansteinar/Projects/Norscode/norcode/scaffold.py)
- [`norcode/package_registry.py`](/Users/jansteinar/Projects/Norscode/norcode/package_registry.py)
- [`norcode/fuzz.py`](/Users/jansteinar/Projects/Norscode/norcode/fuzz.py)
- [`tools/build-bootstrap-binary.sh`](/Users/jansteinar/Projects/Norscode/tools/build-bootstrap-binary.sh)
- [`tools/bootstrap_wrapper.py`](/Users/jansteinar/Projects/Norscode/tools/bootstrap_wrapper.py)

Forventet levetid:
- `bin/nc` skal forbli tynn og native-first.
- `main.py` skal være eksplisitt bootstrap-/utviklervariant.
- `norcode/bootstrap_ci.py` samler bootstrap-gate og workflow-policy uten å bo i entrypointen.
- `norcode/ci_pipeline.py` samler den bredere CI-lanen uten å bo i entrypointen.
- `norcode/bootstrap_support.py` samler små delte bootstrap-helpers uten å bo i entrypointen.
- `norcode/diagnostics.py` samler delte git- og prosjektmetadata uten å bo i entrypointen.
- `norcode/migrations.py` samler navnemigrering og legacy cleanup uten å bo i entrypointen.
- `norcode/commands/release.py` samler CLI-bindingen for release uten å bo i `main.py`.
- `norcode/commands/scaffold_api.py` samler CLI-bindingen for scaffold-api uten å bo i `main.py`.
- `norcode/commands/fuzz.py` samler CLI-bindingen for fuzz uten å bo i `main.py`.
- `norcode/compiler_service.py` samler load/check/build/run/disasm uten å bo i `main.py`.
- `norcode/ir_tools.py` samler IR-tokenisering, disasm og snapshot-verktøy uten å bo i `main.py`.
- `norcode/parity_tools.py` samler selfhost parity-progress, konsistens og fixture-sync uten å bo i `main.py`.
- `norcode/testing_support.py` samler test-suite- og testfil-hjelpere uten å bo i `main.py`.
- `norcode/repl.py` samler REPL-implementasjonen uten å bo i `main.py`.
- `norcode/linting.py` samler linter-hjelpere uten å bo i `main.py`.
- `norcode/smoke.py` samler release/install-smoke uten å bo i entrypointen.
- `norcode/quality_suites.py` samler bench, serve-e2e, stress og security uten å bo i entrypointen.
- `norcode/release.py` samler release-versjonsbump og changelog-oppdatering uten å bo i entrypointen.
- `norcode/scaffold.py` samler API-scaffold-generatoren uten å bo i entrypointen.
- `norcode/package_registry.py` samler registry, lockfile og dependency-management uten å bo i entrypointen.
- `norcode/fuzz.py` samler negativ parser/runtime-korpus uten å bo i entrypointen.
- `tools/build-bootstrap-binary.sh` og `tools/bootstrap_wrapper.py` er overgangsverktøy for bygging og pakking.

### 2. Installasjon, release og lokal utvikleroppsett

Disse er ikke primær produktlogikk, men de er fortsatt en del av den praktiske bootstrap-flaten:

- [`tools/install.sh`](/Users/jansteinar/Projects/Norscode/tools/install.sh)
- [`tools/install-release.sh`](/Users/jansteinar/Projects/Norscode/tools/install-release.sh)
- [`scripts/dev-setup.sh`](/Users/jansteinar/Projects/Norscode/scripts/dev-setup.sh)
- [`package-release.sh`](/Users/jansteinar/Projects/Norscode/package-release.sh)

Forventet levetid:
- Disse skal være selvforklarende og native-first i normal bruk.
- Eventuelle Python-steg her må være eksplisitt merket som bootstrap eller utviklerstøtte.

### 3. Fallback-, diagnose- og overgangskommandoer

Dette er kommandoer som fortsatt eksisterer for å gjøre overgangen trygg og målbar:

- [`norcode/commands/selfhost_bootstrap_build.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/selfhost_bootstrap_build.py)
- [`norcode/commands/selfhost_bootstrap_check.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/selfhost_bootstrap_check.py)
- [`norcode/commands/selfhost_bytecode_suite.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/selfhost_bytecode_suite.py)
- [`norcode/commands/selfhost_parser_suite.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/selfhost_parser_suite.py)
- [`norcode/commands/selfhost_semantic_suite.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/selfhost_semantic_suite.py)
- [`norcode/commands/selfhost_stdlib_suite.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/selfhost_stdlib_suite.py)
- [`norcode/commands/doctor.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/doctor.py)

Forventet levetid:
- De skal forbli eksplisitte, dokumenterte og lette å slå opp.
- Når selfhost blir helt moden, kan flere av disse enten slankes eller flyttes til rene diagnose-/CI-kommandoer.

Nåværende slankingspunkt:
- Diagnose-logikken ligger nå i [`norcode/diagnostics.py`](/Users/jansteinar/Projects/Norscode/norcode/diagnostics.py) i stedet for å bo i `main.py`.
- `doctor` kaller nå diagnose-logikken direkte og trenger ikke lenger å starte en egen `python3 -m norcode diagnose`-prosess.
- Det gjør diagnoseflaten tynnere, mer deterministisk og mindre avhengig av en ekstra bootstrap-hop.
- `main.py` har ikke lenger egne duplikater av git-/repo-hjelpere; de bor nå i [`norcode/diagnostics.py`](/Users/jansteinar/Projects/Norscode/norcode/diagnostics.py).
- Serverruntimeen ligger nå i [`norcode/server_runtime.py`](/Users/jansteinar/Projects/Norscode/norcode/server_runtime.py).
- `norcode/commands/serve.py` bruker den modulen direkte i stedet for å importere `main.py`.
- `main.py` har ikke lenger egne implementasjoner av diagnose- eller serverruntime-logikk.
- Det gjør bootstrapflaten tynnere, mer modulær og mindre avhengig av ekstra kopi-logikk.
- Bootstrap-gate og workflow-policy ligger nå i [`norcode/bootstrap_ci.py`](/Users/jansteinar/Projects/Norscode/norcode/bootstrap_ci.py) i stedet for i `main.py`.
- `ci --bootstrap-lane`-flyten ligger nå i [`norcode/ci_pipeline.py`](/Users/jansteinar/Projects/Norscode/norcode/ci_pipeline.py) i stedet for i `main.py`.
- `main.py` bruker nå disse modulene direkte for bootstrap-gate, CI-policy og CI-lane, uten egen kopi av den logikken.
- Navnemigrering ligger nå i [`norcode/migrations.py`](/Users/jansteinar/Projects/Norscode/norcode/migrations.py) og er koblet inn som en modulær kommando i stedet for en egen `main.py`-implementasjon.
- Scaffold-generatoren ligger nå i [`norcode/scaffold.py`](/Users/jansteinar/Projects/Norscode/norcode/scaffold.py) og er koblet inn som en modulær kommando i stedet for en egen `main.py`-implementasjon.
- Fuzz-suiten ligger nå i [`norcode/fuzz.py`](/Users/jansteinar/Projects/Norscode/norcode/fuzz.py) og er koblet inn som en modulær kommando i stedet for en egen `main.py`-implementasjon.
- Release-logikken ligger nå i [`norcode/release.py`](/Users/jansteinar/Projects/Norscode/norcode/release.py) i stedet for å bo som en egen `main.py`-implementasjon.
- Registry-, lockfile- og dependency-flaten ligger nå i [`norcode/package_registry.py`](/Users/jansteinar/Projects/Norscode/norcode/package_registry.py) i stedet for å bo som store egne funksjoner i `main.py`.

### 4. Python som fortsatt er autoritativ fallback-motor

Dette er Python-implementasjonen som fortsatt fungerer som sammenligningsgrunnlag og nødvei:

- [`compiler/lexer.py`](/Users/jansteinar/Projects/Norscode/compiler/lexer.py)
- [`compiler/parser.py`](/Users/jansteinar/Projects/Norscode/compiler/parser.py)
- [`compiler/semantic.py`](/Users/jansteinar/Projects/Norscode/compiler/semantic.py)
- [`compiler/ast_nodes.py`](/Users/jansteinar/Projects/Norscode/compiler/ast_nodes.py)
- [`compiler/ast_bridge.py`](/Users/jansteinar/Projects/Norscode/compiler/ast_bridge.py)
- [`compiler/loader.py`](/Users/jansteinar/Projects/Norscode/compiler/loader.py)
- [`compiler/formatter.py`](/Users/jansteinar/Projects/Norscode/compiler/formatter.py)
- [`compiler/bytecode_backend.py`](/Users/jansteinar/Projects/Norscode/compiler/bytecode_backend.py)
- [`compiler/interpreter.py`](/Users/jansteinar/Projects/Norscode/compiler/interpreter.py)
- [`compiler/cgen.py`](/Users/jansteinar/Projects/Norscode/compiler/cgen.py)
- [`compiler/native/pipeline.py`](/Users/jansteinar/Projects/Norscode/compiler/native/pipeline.py)

Forventet levetid:
- Dette er fallback-, oracle- og overgangskode.
- Den bør ikke være normalproduktets primærvei.

## Hva som kan tynnes videre

Disse områdene er identifisert som gode kandidater for ytterligere slanking:

| Område | Filer | Hvorfor den fortsatt er tung | Retning videre |
| --- | --- | --- | --- |
| Bootstrap-orchestration | `main.py`, `norcode/bootstrap_ci.py`, `norcode/ci_pipeline.py`, `norcode/diagnostics.py` | `main.py` samler fortsatt mange historiske kommandoer og fallback-løp, men de tunge bootstrap-gate/policy/ci-lane/git-delen er flyttet ut | Splitt bootstrap fra produkt og hold resten tynn |
| Wrapper/entry | `bin/nc`, `tools/bootstrap_wrapper.py` | Binder normalflyt og legacy sammen | Hold dem tynne og native-first |
| Build/install | `tools/build-bootstrap-binary.sh`, `tools/install.sh`, `tools/install-release.sh`, `scripts/dev-setup.sh` | Har fortsatt overgangskode og flere alternative ruter | Gjør normalveien eksplisitt og minimal |
| Fallback- og parity-kommandoer | `norcode/commands/selfhost_*` og `norcode/commands/doctor.py` | De må eksistere, men bør være tydelig merket som overgangs-/diagnoseverktøy | Behold, men slank og dokumenter |
| Python-oracle i compiler-kjernen | `compiler/*.py` | Brukes fortsatt som sammenligningsgrunnlag og fallback | Flytt mer og mer til selfhost, men behold som kontrollflate i overgangsperioden |

## Normalflyt versus overgangsflaten

- Normal brukerflyt skal være binary-first.
- Python skal bare dukke opp når brukeren ber om fallback eller når et eksplisitt bootstrap-/utviklersteg krever det.
- Ingen Python-del i denne inventarliste skal være en skjult standardbane.

## Les videre

- [`docs/SELFHOST_REMAINING_ROADMAP.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_REMAINING_ROADMAP.md)
- [`docs/SELFHOST_STATUS.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_STATUS.md)
- [`docs/SELFHOST_DEPENDENCY_MAP.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_DEPENDENCY_MAP.md)
- [`docs/PHASE8_PYTHON_FALLBACK.md`](/Users/jansteinar/Projects/Norscode/docs/PHASE8_PYTHON_FALLBACK.md)
