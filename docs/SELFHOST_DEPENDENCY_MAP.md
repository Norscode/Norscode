# Selfhost Dependency Map

Dette dokumentet er Omgang 1 for selvstendighetsarbeidet.
Målet er å gjøre det helt tydelig hva som fortsatt eies av Python, hva som allerede har Norscode-ekvivalent, og hva som må flyttes først.

## Kort status

- [x] Python-flaten er kartlagt
- [x] Selfhost-ekvivalenter er identifisert
- [x] Python-bootstrap-/orchestration-ansvaret er delt inn i tydelige grupper
- [x] Normal CLI-vei er flyttet til `norcode/cli.py`; Python-flaten er nå eksplisitt bootstrap-kompatibilitet
- [x] Minimumskjernen for selfhost-v1 er definert
- [x] Denne oversikten kan brukes som inngang for senere omganger

## Hva som er Python i `compiler/`

### Frontend og analyse

- `compiler/lexer.py` - Python-lexer for dagens compiler frontend
- `compiler/parser.py` - Python-parser for dagens compiler frontend
- `compiler/semantic.py` - semantic analyzer og type-/scope-sjekk
- `compiler/ast_nodes.py` - AST-node-definisjoner brukt av Python-frontenden
- `compiler/ast_bridge.py` - AST serialisering/parity-/bridge-lag
- `compiler/loader.py` - modul-lading og import-graph for Python-pipelinen

### Backend og runtime

- `compiler/bytecode_backend.py` - Python bytecode-compiler og VM-flyt
- `compiler/interpreter.py` - legacy/runtime-interpreter og web/runtime-biter
- `compiler/cgen.py` - C-generering for legacy/bootstrapping
- `compiler/native/pipeline.py` - native pipeline og bootstrapverktøy
- `compiler/native/arithmetic_lowering.py`
- `compiler/native/comparison_lowering.py`
- `compiler/native/control_flow_lowering.py`
- `compiler/native/elf_writer.py`
- `compiler/native/function_lowering.py`
- `compiler/native/register_allocator.py`

### Selfhost- og bootstrap-broer

- `compiler/selfhost_ast_bridge.py`
- `compiler/selfhost_chain.py`
- `compiler/selfhost_parser.py`
- `compiler/selfhost_whole_compile.py`

### Støtte og pakking

- `compiler/formatter.py`
- `compiler/toml_compat.py`
- `compiler/__init__.py`
- `compiler/native/__init__.py`

## Hva som allerede har selfhost-ekvivalent

Dette betyr ikke at alt er ferdig, bare at en tilsvarende Norscode-vei allerede finnes eller er på plass som egen modul-/sporflate.

| Python-modul | Status | Norscode-/selfhost-ekvivalent |
|---|---:|---|
| `compiler/lexer.py` | Delvis | [`selfhost/lexer.no`](/Users/jansteinar/Projects/Norscode/selfhost/lexer.no), [`selfhost/lexer/lexer_m1.no`](/Users/jansteinar/Projects/Norscode/selfhost/lexer/lexer_m1.no) |
| `compiler/parser.py` | Delvis | [`selfhost/parser.no`](/Users/jansteinar/Projects/Norscode/selfhost/parser.no), [`selfhost/parser_real.no`](/Users/jansteinar/Projects/Norscode/selfhost/parser_real.no) |
| `compiler/semantic.py` | Delvis | [`selfhost/semantic.no`](/Users/jansteinar/Projects/Norscode/selfhost/semantic.no), [`selfhost/frontend/selfhost_semantic.no`](/Users/jansteinar/Projects/Norscode/selfhost/frontend/selfhost_semantic.no) |
| `compiler/ast_nodes.py` | Delvis | [`selfhost/ast.no`](/Users/jansteinar/Projects/Norscode/selfhost/ast.no) |
| `compiler/ast_bridge.py` | Delvis | [`selfhost/serialization.no`](/Users/jansteinar/Projects/Norscode/selfhost/serialization.no), AST-format docs |
| `compiler/bytecode_backend.py` | Delvis | [`selfhost/bytecode_backend.no`](/Users/jansteinar/Projects/Norscode/selfhost/bytecode_backend.no), [`selfhost/backend.no`](/Users/jansteinar/Projects/Norscode/selfhost/backend.no) |
| `compiler/interpreter.py` | Delvis | [`selfhost/vm.no`](/Users/jansteinar/Projects/Norscode/selfhost/vm.no), [`selfhost/runtime_core/runtime_core.no`](/Users/jansteinar/Projects/Norscode/selfhost/runtime_core/runtime_core.no) |
| `compiler/native/*` | Delvis | [`selfhost/native_execution/*`](/Users/jansteinar/Projects/Norscode/selfhost/native_execution/) |
| `compiler/selfhost_chain.py` | Ja | [`selfhost/bootstrap.no`](/Users/jansteinar/Projects/Norscode/selfhost/bootstrap.no), [`selfhost/bootstrap_compiler/*`](/Users/jansteinar/Projects/Norscode/selfhost/bootstrap_compiler/) |
| `compiler/selfhost_parser.py` | Delvis | parser-relaterte selfhost-moduler og parity-spor |
| `compiler/selfhost_whole_compile.py` | Delvis | [`selfhost/full_selfhost_pipeline.no`](/Users/jansteinar/Projects/Norscode/selfhost/full_selfhost_pipeline.no) |
| `compiler/loader.py` | Delvis | [`selfhost/module_linker.no`](/Users/jansteinar/Projects/Norscode/selfhost/module_linker.no), [`selfhost/modules.no`](/Users/jansteinar/Projects/Norscode/selfhost/modules.no) |
| `compiler/formatter.py` | Nei | ingen full selfhost-ekvivalent ennå |
| `compiler/cgen.py` | Nei | legacy C-spor, ingen full selfhost-ekvivalent ennå |
| `compiler/toml_compat.py` | Nei | støtte skjer foreløpig via Python/bootstrap |

## Hva som fortsatt bare er Python i praksis

Disse områdene har fortsatt Python som primærmotor eller bootstrap-bro:

- formattering
- C-generering
- deler av native pipeline og native backend
- legacy interpreter/runtime-flyter
- modul-lading/import-resolusjon
- TOML-kompatibilitet
- bootstrap/orchestration i den gjenværende Python-bootstrapen

## Python bootstrap dependency map

Den gjenværende Python-bootstrapen er fortsatt et bootstrap- og orchestration-senter. Den drar inn disse hovedgruppene:

### 1. CLI og modulær dispatch

- `norcode.command_dispatch`
- `norcode.commands.registry`

### 2. Frontend/compiler-kjerne

- `compiler.lexer`
- `compiler.parser`
- `compiler.semantic`
- `compiler.loader`
- `compiler.ast_nodes`
- `compiler.ast_bridge`

### 3. Backend og runtime

- `compiler.bytecode_backend`
- `compiler.native.pipeline`
- `compiler.selfhost_chain`
- `compiler.selfhost_whole_compile`
- `compiler.interpreter`
- `compiler.cgen`

### 4. Verktøy og bootstrap

- `argparse`, `subprocess`, `tarfile`, `zipfile`, `tempfile`
- HTTP-servere, filsystem- og releaseverktøy
- CI-, snapshot- og verifikasjonsflyt

### 5. Hva dette betyr

- `norcode/cli.py` er den normale modulære veien for CLI-dispatch.
- Den gjenværende Python-bootstrapen er fortsatt ikke liten nok til å være “bare oppstart” i full forstand.
- Frontend- og HTML-delen er allerede ute av bootstrap som hovedmotor.
- Resten av selvstendighetsarbeidet ligger primært i compiler/runtime og bootstrap-verktøy utenfor frontend.

## Minimumskjerne for selfhost-v1

Dette er den minste selfhost-kjernen som gir mening for videre arbeid:

- lexer for identifikatorer, nøkkelord, tall, tekst, operatorer og punctuation
- parser for funksjoner, variabler, retur, if/else, while, kall og binære uttrykk
- AST-format som kan serialiseres deterministisk
- semantic-sjekk for navn, scopes, typer og returflyt
- IR-/bytecode-output for små programmer
- parity-flyter mot Python-motoren

Det er denne kjernen som gjør at senere selfhost-omganger kan bygges trygt uten å måtte gjette hva “v1” egentlig betyr.

## Minimumskjerne som skal være låst før Omgang 2

- [x] Selfhost lexer-spor finnes
- [x] Selfhost parser-spor finnes
- [x] Selfhost semantic-spor finnes
- [x] Selfhost backend-spor finnes
- [x] Selfhost bootstrap-spor finnes
- [ ] Parity og stabilitet er fortsatt under arbeid

## Les videre

- [`docs/SELFSTENDIG_NORSCODE_ROADMAP.md`](/Users/jansteinar/Projects/Norscode/docs/SELFSTENDIG_NORSCODE_ROADMAP.md)
- [`docs/SELFHOST_STATUS.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_STATUS.md)
- [`docs/SELFHOST_NO_PYTHON_NO_C_PLAN.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_NO_PYTHON_NO_C_PLAN.md)
- [`docs/PHASE8_PYTHON_FALLBACK.md`](/Users/jansteinar/Projects/Norscode/docs/PHASE8_PYTHON_FALLBACK.md)
- [`docs/SELFHOST_RUNTIME_SEPARATION.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_RUNTIME_SEPARATION.md)
