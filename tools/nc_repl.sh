#!/bin/sh
# tools/nc_repl.sh — Python-fri REPL for Norscode
# Brukar norscode_native.

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
NC_NATIVE="$ROOT/dist/norscode_native"
NC="$ROOT/bin/nc"

# Velg runner
if [ -x "$NC_NATIVE" ]; then
    _run_file() { NORSCODE_CMD=run NORSCODE_FILE="$1" "$NC_NATIVE" 2>&1; }
else
    printf 'Feil: trenger dist/norscode_native. Køyr: bash tools/build_norscode_native.sh\n' >&2
    exit 1
fi

printf 'Norscode REPL (Python-fri)\n'
printf 'Skriv Norscode-uttrykk. Enter for å evaluere. "exit" for å avslutte.\n\n'

while IFS= read -r line; do
    [ "$line" = "exit" ] || [ "$line" = "avslutt" ] || [ "$line" = "quit" ] && break
    [ -z "$line" ] && continue
    
    # Wrap expression in start() function and run
    NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT" NORSCODE_REPL_LINE="$line" "$NC" run "$ROOT/tools/nc_repl.no" || true
done

printf 'Takk for no!\n'
