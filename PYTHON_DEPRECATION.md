# Python-fil-avvikling plan

## Status: praktisk Python-fri for primærbruk

### Allereie Python-fri (via nc-vm):
- `bin/nc run foo.no`    → dist/nc-vm --nc-run
- `bin/nc compile foo.no` → dist/nc-vm --nc-compile  
- `bin/nc build foo.no`  → dist/norcode-bootstrap-compile
- `bin/nc check foo.no`  → dist/norcode-bootstrap-compile
- `bin/nc test`          → tools/nc_test.sh

### Python-filer som KAN fjernast no:
(Erstatta av nc-vm / selfhost-kompilatoren)

- `compiler/lexer.py`         → selfhost/lexer/lexer_m1.no
- `compiler/parser.py`        → selfhost/parser.no  
- `compiler/semantic.py`      → selfhost/compiler/semantic.no
- `compiler/bytecode_backend.py` → selfhost/compiler/ir_to_bytecode.no
- `compiler/selfhost_chain.py` → nc-vm --nc-run/--nc-compile
- `compiler/selfhost_whole_compile.py` → nc-vm bundler

### Python-filer som IKKJE kan fjernast enno:
(Brukt av dev-verktøy eller --legacy-python-fallback)

- `norcode/legacy_main.py`    → --legacy-python-fallback entrypoint
- `norcode/commands/format.py` → `nc format` (ikkje erstatta enno)
- `norcode/commands/lint.py`   → `nc lint` (ikkje erstatta enno)
- `norcode/repl.py`            → `nc repl` (ikkje erstatta enno)
- `norcode/server_runtime.py`  → `nc serve` (ikkje erstatta enno)
- `compiler/interpreter.py`    → selfhost-bootstrap-gate brukar det
- `compiler/toml_compat.py`    → TOML-parsing (ikkje erstatta enno)

### Neste steg mot full Python-fjerning:
1. Flytt `compiler/` til `legacy/compiler/` (behold for --legacy)
2. Lag Python-fri `nc format` via selfhost-formatering
3. Lag Python-fri `nc repl` via nc-vm REPL
4. Fjern norcode/commands/ etter kvart
