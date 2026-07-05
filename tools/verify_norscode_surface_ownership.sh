#!/usr/bin/env sh
# Tynn wrapper: Norscode eig regelen, shell er fallback når runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

_out="$(mktemp "${TMPDIR:-/tmp}/norscode_surface_ownership.XXXXXX")"
if "$ROOT/bin/nc" run "$ROOT/tools/verify_norscode_surface_ownership.no" >"$_out" 2>&1; then
  cat "$_out"
  rm -f "$_out"
  exit 0
fi

if ! grep -Eq 'Ukjent funksjon: builtin(\.builtin)?\.exec_prosess' "$_out"; then
  cat "$_out" >&2
  rm -f "$_out"
  exit 1
fi
rm -f "$_out"

missing=0
while IFS= read -r p; do
  base="${p%.*}"
  if [ ! -f "$base.no" ]; then
    printf '%s manglar %s.no\n' "$p" "$base"
    missing=1
  fi
done <<EOF
$(find . -path ./.git -prune -o \( -name '*.sh' -o -name '*.js' -o -name '*.swift' \) -print \
  | sed 's#^\./##' \
  | sort \
  | awk '!/^archive\// && !/^ai_assistent/ && !/^ai_assistent_bak_/ && !/^tests_bak_/')
EOF

actual_c="$(find . -path ./.git -prune -o -name '*.c' -print | sed 's#^\./##' | sort | awk '!/^archive\//')"
if [ -n "$actual_c" ]; then
  expected_c='build/bootstrap_compiler_bundle_ncb_data.c
build/native_elf_compiler_bundle_ncb_data.c
build/v3009/native_candidate_combined.c
build/v3009/norscode_generated.c'
  if [ "$actual_c" != "$expected_c" ]; then
    printf 'uventa C-overflate:\n%s\n' "$actual_c"
    missing=1
  fi
fi

if [ "$missing" -eq 0 ]; then
  printf '=== Norscode surface ownership: BESTÅTT ===\n'
else
  printf '=== Norscode surface ownership: FEILA ===\n'
fi
exit "$missing"
