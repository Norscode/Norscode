# Python-fil-avvikling plan

## Status: Python-fri for alle primær- og dev-kommandoar

### Python-fri (via nc-vm):
- `bin/nc run foo.no`    → dist/nc-vm --nc-run
- `bin/nc compile foo.no` → dist/nc-vm --nc-compile
- `bin/nc build foo.no`  → dist/norcode-bootstrap-compile
- `bin/nc check foo.no`  → dist/norcode-bootstrap-compile
- `bin/nc format foo.no` → selfhost/formatter.no (nc-vm)
- `bin/nc lint foo.no`   → selfhost/linter.no (nc-vm)
- `bin/nc doctor`        → selfhost/doctor.no (nc-vm)
- `bin/nc repl`          → tools/nc_repl.sh (nc-vm)
- `bin/nc test`          → tools/nc_test.sh

### Eksplisitt avvikla (krev --legacy-python-fallback):
- `nc serve`             → norcode/commands/serve.py
- `nc smoke`             → norcode/commands/smoke.py (bruk nc test)
- `nc bench`             → norcode/commands/bench.py
- `nc migrate-names`     → norcode/commands/migrate_names.py
- `nc diagnose`          → norcode/commands/diagnose.py (bruk nc doctor)
- `nc stress`            → norcode/commands/stress.py
- `nc fuzz`              → norcode/commands/fuzz.py
- `nc add/update/lock/packages` → norcode/commands/package_registry_commands.py
- `nc scaffold-api`      → norcode/commands/scaffold_api.py

### Python-filer som framleis er i bruk (via --legacy-python-fallback):
- `norcode/legacy_main.py`    → --legacy-python-fallback entrypoint
- `norcode/server_runtime.py` → nc serve (legacy)
- `compiler/interpreter.py`   → legacy bootstrap-gate
- `compiler/toml_compat.py`   → TOML-parsing (legacy)

### Python-fri bootstrap-pipeline:
- `tools/build-bootstrap-binary.sh` byggjer nc-vm automatisk (clang/cc) om ikkje tilgjengeleg
- `selfhost-bootstrap-gate` køyrer Python-fri i alle CI-workflows
- `--legacy-python-fallback` er fjerna frå alle CI selfhost-bootstrap-gate-kall

### Neste steg mot full Python-fjerning:
1. Flytt `compiler/` til `legacy/compiler/` (behold for --legacy)
2. Vurder fjerning av heile norcode/commands/ (alle avvikla)
3. Slett norcode/server_runtime.py når nc serve ikkje lenger er i bruk
