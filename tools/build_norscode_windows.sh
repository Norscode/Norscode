#!/usr/bin/env bash
# tools/build_norscode_windows.sh
# Bygg dist/norscode_native.exe for Windows x86_64
#
# Fungerer på:
#   - windows-latest GitHub runner (Git Bash / MINGW64)
#   - Linux med x86_64-w64-mingw32-gcc (krysskompilar)
#
# Strategi:
#   1. Finn MinGW gcc (krysskompilar, native MinGW, eller cl.exe)
#   2. Last ned SQLite3-amalgamasjon til build/sqlite3_amalg/ om ikkje til stades
#   3. Smelt saman norscode_generated.c + nc_native_main.c (same som build_norscode_native.sh)
#   4. Kompiler saman med sqlite3.c → dist/norscode_native.exe
#
# Bruk (Windows CI):
#   BOOTSTRAP_C_ROOT=tools bash tools/build_norscode_windows.sh
#
# Bruk (Linux krysskompilar):
#   BOOTSTRAP_C_ROOT=tools bash tools/build_norscode_windows.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist/norscode_native.exe"
BOOTSTRAP_C_ROOT="${BOOTSTRAP_C_ROOT:-${ROOT}/tools}"

SQLITE3_BUILD_DIR="${ROOT}/build/sqlite3_amalg"
SQLITE3_C="${SQLITE3_BUILD_DIR}/sqlite3.c"
SQLITE3_H="${SQLITE3_BUILD_DIR}/sqlite3.h"

# ─── Finn kompilator ─────────────────────────────────────────────────────────
find_compiler() {
    # 1. Krosskompilar på Linux/macOS
    if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
        printf 'x86_64-w64-mingw32-gcc'
        return 0
    fi
    # 2. MinGW gcc på Windows (Git Bash / MSYS2)
    if command -v gcc >/dev/null 2>&1; then
        printf 'gcc'
        return 0
    fi
    # 3. cl.exe (MSVC — Windows native)
    if command -v cl >/dev/null 2>&1; then
        printf 'cl'
        return 0
    fi
    printf 'Feil: fann ingen kompilator (x86_64-w64-mingw32-gcc, gcc, eller cl.exe)\n' >&2
    return 1
}

# ─── Last ned SQLite3-amalgamasjon ───────────────────────────────────────────
ensure_sqlite3() {
    if [ -f "$SQLITE3_C" ] && [ -f "$SQLITE3_H" ]; then
        printf '✓ sqlite3 amalgamasjon er allereie til stades\n'
        return 0
    fi
    mkdir -p "$SQLITE3_BUILD_DIR"

    # Prøv fleire kjende stabile versjonar i rekkefølge
    local found=0
    for url in \
        "https://www.sqlite.org/2024/sqlite-amalgamation-3470200.zip" \
        "https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip" \
        "https://www.sqlite.org/2024/sqlite-amalgamation-3450300.zip"
    do
        local zip="${SQLITE3_BUILD_DIR}/sqlite3_amalg.zip"
        printf 'Lastar ned sqlite3 frå %s...\n' "$url"
        local dl_ok=0
        if command -v curl >/dev/null 2>&1; then
            curl -fsSL -o "$zip" "$url" 2>/dev/null && dl_ok=1 || true
        elif command -v wget >/dev/null 2>&1; then
            wget -q -O "$zip" "$url" 2>/dev/null && dl_ok=1 || true
        else
            printf 'Feil: trenger curl eller wget for å laste ned sqlite3\n' >&2
            return 1
        fi
        if [ "$dl_ok" = "1" ] && [ -s "$zip" ]; then
            found=1
            break
        fi
    done

    if [ "$found" != "1" ]; then
        printf 'Feil: kunne ikkje laste ned sqlite3 amalgamasjon\n' >&2
        return 1
    fi

    # Pakk ut sqlite3.c og sqlite3.h
    local zip="${SQLITE3_BUILD_DIR}/sqlite3_amalg.zip"
    if command -v unzip >/dev/null 2>&1; then
        unzip -o -j "$zip" "*/sqlite3.c" "*/sqlite3.h" -d "$SQLITE3_BUILD_DIR" >/dev/null
    elif command -v python3 >/dev/null 2>&1; then
        python3 - "$zip" "$SQLITE3_BUILD_DIR" <<'PYEOF'
import sys, zipfile, os, shutil
zpath, outdir = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(zpath) as z:
    for name in z.namelist():
        base = os.path.basename(name)
        if base in ('sqlite3.c', 'sqlite3.h'):
            with z.open(name) as src, open(os.path.join(outdir, base), 'wb') as dst:
                shutil.copyfileobj(src, dst)
PYEOF
    else
        printf 'Feil: trenger unzip eller python3 for å pakke ut sqlite3\n' >&2
        return 1
    fi

    rm -f "$zip"
    [ -f "$SQLITE3_C" ] && [ -f "$SQLITE3_H" ] || {
        printf 'Feil: sqlite3.c / sqlite3.h ikkje funne etter utpakking\n' >&2
        return 1
    }
    printf '✓ sqlite3 amalgamasjon klar (%d KB)\n' "$(( $(wc -c < "$SQLITE3_C") / 1024 ))"
}

# ─── Bygg norscode_native.exe ────────────────────────────────────────────────
build_exe() {
    local CC="$1"
    local gen="${BOOTSTRAP_C_ROOT}/maint/c/norscode_generated.c"
    local main_c="${ROOT}/tools/maint/c/nc_native_main.c"

    for f in "$gen" "$main_c"; do
        [ -f "$f" ] || { printf 'Feil: manglar %s\n' "$f" >&2; return 1; }
    done
    [ -f "$SQLITE3_C" ] || { printf 'Feil: manglar %s\n' "$SQLITE3_C" >&2; return 1; }

    local merged="${ROOT}/build/nc_win_merged_$$.c"
    mkdir -p "${ROOT}/build" "$(dirname "$OUT")"

    # Smelt saman på same måte som build_from_bootstrap_c() i build_norscode_native.sh
    {
        printf '/* Host FFI forward decl (Omgang 4) */\n'
        printf 'struct NcVal;\n'
        printf 'typedef struct NcVal NcVal;\n'
        printf 'NcVal *nc_fn_builtin_host_exec_ncb_json(NcVal **args, int na);\n'
        printf 'NcVal *nc_fn_builtin_host_kall_bygg_bundle(NcVal **args, int na);\n\n'
        grep -v '#include.*nc_runtime' "$gen" | sed 's/^int main/static int nc_gen_main/'
    } > "$merged"
    cat "$main_c" >> "$merged"

    printf 'Kompilerer norscode_native.exe med %s...\n' "$CC"

    local compile_ok=0
    case "$CC" in
        cl)
            # MSVC — ikkje vanleg i CI, men støtta
            cl.exe /O2 /W0 \
                /Fe:"$OUT" \
                "$merged" "$SQLITE3_C" \
                /link /SUBSYSTEM:CONSOLE \
                && compile_ok=1 || true
            ;;
        *)
            # GCC / MinGW / krysskompilar
            "$CC" -O2 -w \
                -o "$OUT" \
                "$merged" "$SQLITE3_C" \
                -lws2_32 \
                && compile_ok=1 || true
            ;;
    esac

    rm -f "$merged"

    [ "$compile_ok" = "1" ] || {
        printf 'Feil: kompilering feila\n' >&2
        return 1
    }

    [ -f "$OUT" ] || {
        printf 'Feil: %s vart ikkje laga\n' "$OUT" >&2
        return 1
    }

    printf '✓ dist/norscode_native.exe bygget (%d bytes)\n' "$(wc -c < "$OUT" | tr -d ' ')"
}

# ─── Røyktest ────────────────────────────────────────────────────────────────
smoke_exe() {
    [ -f "$OUT" ] || return 1
    local tmp="${TMPDIR:-/tmp}/nc_smoke_$$.no"
    printf 'funksjon start() { returner 0 }\n' > "$tmp"
    if NORSCODE_CMD=run NORSCODE_FILE="$tmp" "$OUT" >/dev/null 2>&1; then
        rm -f "$tmp"
        return 0
    fi
    rm -f "$tmp"
    return 1
}

# ─── Hovudflyt ───────────────────────────────────────────────────────────────
CC="$(find_compiler)"
printf 'Kompilator: %s\n' "$CC"

ensure_sqlite3
build_exe "$CC"

if smoke_exe; then
    printf '✓ Røyktest OK\n'
else
    printf '⚠️  Røyktest feila (normalt ved krysskompilar — kan ikkje køyre .exe på Linux)\n'
fi
