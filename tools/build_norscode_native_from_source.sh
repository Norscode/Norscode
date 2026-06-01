#!/usr/bin/env bash
# tools/build_norscode_native_from_source.sh
#
# CI-fallback: bygg dist/norcode-bootstrap-compile frå innebygd C/NCBB (legacy).
# Dette er IKKE det same som dist/norscode_native (NORSCODE_CMD-runtime).
# Bruk bootstrap/stage0/ eller GitHub Release for ekte norscode_native.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist/norcode-bootstrap-compile"

if [ "${NORSCODE_ALLOW_SOURCE_BUILD:-}" != "1" ] && [ "${GITHUB_ACTIONS:-}" != "true" ]; then
    printf 'Kjeldebygg er berre for CI. Sett NORSCODE_ALLOW_SOURCE_BUILD=1 for lokal bygging.\n' >&2
    exit 1
fi

CC="${CC:-cc}"
if command -v clang >/dev/null 2>&1; then CC=clang; fi
if ! command -v "$CC" >/dev/null 2>&1; then
    CC=gcc
fi
if ! command -v "$CC" >/dev/null 2>&1; then
    printf 'Feil: trenger clang eller gcc for CI-kjeldebygg\n' >&2
    exit 1
fi

for f in \
    build/bootstrap_compiler_bundle_ncb_data.c \
    build/native_elf_compiler_bundle_ncb_data.c \
    build/mv_bootstrap_stub_manifest_dispatch.inc
do
    if [ ! -f "$ROOT/$f" ]; then
        printf 'Feil: manglar %s\n' "$f" >&2
        exit 1
    fi
done

mkdir -p "$ROOT/build" "$(dirname "$OUT")"

printf 'Byggjer norcode-bootstrap-compile frå C/NCBB (%s)...\n' "$CC"

"$CC" -Wall -Wextra -O2 \
    -DNORCODE_DUAL_NCBB \
    -DNORCODE_BOOTSTRAP_STUBS \
    -DNORCODE_EMBED_NATIVE_NCBB \
    -DNORCODE_BOOTSTRAP_HOST_EXEC \
    -I "$ROOT/tools/c_minimal_vm" \
    -I "$ROOT/build" \
    -o "$OUT" \
    "$ROOT/tools/c_minimal_vm/bootstrap_compiler_probe_main.c" \
    "$ROOT/tools/c_minimal_vm/compiler_standalone.c" \
    "$ROOT/tools/c_minimal_vm/norcode_p3b_compiler_markers.c" \
    "$ROOT/tools/c_minimal_vm/mv_arena.c" \
    "$ROOT/tools/c_minimal_vm/minimal_vm.c" \
    "$ROOT/tools/c_minimal_vm/mv_builtins.c" \
    "$ROOT/tools/c_minimal_vm/mv_syscall.c" \
    "$ROOT/tools/c_minimal_vm/mv_io.c" \
    "$ROOT/tools/c_minimal_vm/mv_env.c" \
    "$ROOT/tools/c_minimal_vm/mv_json.c" \
    "$ROOT/tools/c_minimal_vm/ncbb_loader.c" \
    "$ROOT/tools/c_minimal_vm/mv_bootstrap_stubs.c" \
    "$ROOT/tools/c_minimal_vm/mv_bootstrap_host_exec.c" \
    "$ROOT/build/native_elf_compiler_bundle_ncb_data.c" \
    "$ROOT/build/bootstrap_compiler_bundle_ncb_data.c"

chmod +x "$OUT"

if ! NORCODE_BOOTSTRAP_VM=1 NORCODE_BOOTSTRAP_CLI=1 NORCODE_ARGC=1 NORCODE_ARG0=selfcheck "$OUT" >/dev/null 2>&1; then
    printf 'Feil: NORCODE selfcheck feila etter kjeldebygg\n' >&2
    exit 1
fi

printf '✓ dist/norcode-bootstrap-compile bygget (%d bytes)\n' "$(wc -c < "$OUT" | tr -d ' ')"
