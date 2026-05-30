#!/bin/sh
# tools/nc_repl.sh — Python-fri REPL for Norscode
# Les uttrykkk og evaluerer via nc-vm --nc-run

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
NC_VM="$ROOT/dist/nc-vm"

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
    result=$("$NC_VM" --nc-run "$tmpfile" 2>&1)
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
        result2=$("$NC_VM" --nc-run "$tmpfile2" 2>&1)
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
