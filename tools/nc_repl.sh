#!/bin/sh
# tools/nc_repl.sh — Python-fri REPL for Norscode
# Brukar norscode_native (eller nc-vm som fallback)

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
NC_NATIVE="$ROOT/dist/norscode_native"
NC_VM="$ROOT/dist/nc-vm"

# Velg runner
if [ -x "$NC_NATIVE" ]; then
    _run_file() { NORSCODE_CMD=run NORSCODE_FILE="$1" "$NC_NATIVE" 2>&1; }
elif [ -x "$NC_VM" ]; then
    _run_file() { "$NC_VM" --nc-run "$1" 2>&1; }
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
    tmpfile="$(mktemp /tmp/nc_repl_XXXXXX.no)"
    cat > "$tmpfile" << EOF
funksjon start() {
    la _res = $line
    skriv(tekst_fra_heltall(_res))
}
EOF
    result=$(_run_file "$tmpfile")
    ec=$?
    rm -f "$tmpfile"
    
    if [ $ec -eq 0 ]; then
        printf '=> %s\n' "$result"
    else
        # Try as string expression
        tmpfile2="$(mktemp /tmp/nc_repl_XXXXXX.no)"
        cat > "$tmpfile2" << EOF
funksjon start() {
    skriv($line)
}
EOF
        result2=$(_run_file "$tmpfile2")
        ec2=$?
        rm -f "$tmpfile2"
        if [ $ec2 -eq 0 ]; then
            printf '=> %s\n' "$result2"
        else
            printf 'Feil: %s\n' "$result2"
        fi
    fi
done

printf 'Takk for no!\n'
