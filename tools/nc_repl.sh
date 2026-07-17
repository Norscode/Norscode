#!/bin/sh
# Norscode-first wrapper: REPL-evalueringa har eigarlogikk i tools/nc_repl.no.
# Shell-delen under er avgrensa interaktiv reserveveg medan runtime manglar exec_prosess.

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
NC_NATIVE="$ROOT/dist/norscode_native"
NC="$ROOT/bin/nc"

if [ -x "$NC_NATIVE" ]; then
    _run_file() { NORSCODE_CMD=run NORSCODE_FILE="$1" "$NC_NATIVE" 2>&1; }
else
    printf 'Feil: treng dist/norscode_native. Køyr: ./bin/nc fetch-stage0-seed\n' >&2
    exit 1
fi

_tmp_base() {
    mktemp "${TMPDIR:-/tmp}/nc_repl_XXXXXX" 2>/dev/null || printf '%s/nc_repl_%s\n' "${TMPDIR:-/tmp}" "$$"
}

_eval_line() {
    line="$1"
    tmpbase="$(_tmp_base)"
    tmpfile="${tmpbase}_expr.no"
    tmpfile2="${tmpbase}_print.no"
    rm -f "$tmpfile" "$tmpfile2"

    {
        printf 'funksjon start() {\n'
        printf '    la _res = %s\n' "$line"
        printf '    skriv(tekst_fra_heltall(_res))\n'
        printf '}\n'
    } > "$tmpfile"
    res="$(_run_file "$tmpfile")"
    rc=$?
    rm -f "$tmpfile"
    if [ "$rc" -eq 0 ]; then
        printf '=> %s\n' "$res"
        rm -f "$tmpfile2"
        return 0
    fi

    {
        printf 'funksjon start() {\n'
        printf '    skriv(%s)\n' "$line"
        printf '}\n'
    } > "$tmpfile2"
    res="$(_run_file "$tmpfile2")"
    rc=$?
    rm -f "$tmpfile2"
    if [ "$rc" -eq 0 ]; then
        printf '=> %s\n' "$res"
        return 0
    fi

    printf 'Feil: %s\n' "$res"
    return 1
}

if [ "$#" -gt 0 ]; then
    _eval_line "$*"
    exit $?
fi

printf 'Norscode REPL (Python-fri)\n'
printf 'Skriv Norscode-uttrykk. Enter for å evaluere. "exit" for å avslutte.\n\n'

while IFS= read -r line; do
    [ "$line" = "exit" ] || [ "$line" = "avslutt" ] || [ "$line" = "quit" ] && break
    [ -z "$line" ] && continue

    _eval_line "$line" || true
done

printf 'Takk for no!\n'
