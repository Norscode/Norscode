#!/usr/bin/env sh
# Norscode-first wrapper: eigarskapsregelen ligg i tools/verify_norscode_surface_ownership.no.
# Shell-delen under er avgrensa reserveveg medan runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"

_out="$(mktemp "${TMPDIR:-/tmp}/norscode_surface_ownership.XXXXXX")"

print_file() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    printf '%s\n' "$_line"
  done < "$_file"
}

has_exec_gap() {
  _file="$1"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"Ukjent funksjon: exec_prosess"*|*"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

strip_dot_slash() {
  case "$1" in
    ./*) printf '%s\n' "${1#./}" ;;
    *) printf '%s\n' "$1" ;;
  esac
}

is_active_rel() {
  case "$1" in
    ""|build/*|archive/*|ai_assistent*|ai_assistent_bak_*|tests_bak_*) return 1 ;;
    *) return 0 ;;
  esac
}

is_known_c_artifact() {
  case "$1" in
    build/bootstrap_compiler_bundle_ncb_data.c|\
    build/native_elf_compiler_bundle_ncb_data.c|\
    build/runtime_before_hex_json_fix.c|\
    build/v3009/native_candidate_combined.c|\
    build/v3009/native_candidate_gc.c|\
    build/v3009/native_gc_stress.c|\
    build/v3009/norscode_generated.c|\
    build/v3009/norscode_generated_gc.c|\
    build/v9630/native_candidate_linux.c|\
    build/v9630/norscode_generated.c|\
    build/v9646/native_candidate_linux_aarch64.c|\
    build/v9646/native_candidate_linux_x86_64.c|\
    build/v9646/native_candidate_macos.c|\
    build/v9646/native_candidate_windows.c|\
    build/v9646/norscode_generated.c) return 0 ;;
    *) return 1 ;;
  esac
}

file_contains() {
  _file="$1"
  _needle="$2"
  while IFS= read -r _line || [ -n "$_line" ]; do
    case "$_line" in
      *"$_needle"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}

script_marker_ok() {
  _rel="$1"
  case "$_rel" in
    *.sh)
      file_contains "$_rel" "Norscode-first wrapper" && return 0
      [ "$_rel" = "tools/install.sh" ] && file_contains "$_rel" "Bootstrap POSIX-installer" && return 0
      [ "$_rel" = "tools/omgang6b_compiler_bundle_args.inc.sh" ] && file_contains "$_rel" "Norscode-first include" && return 0
      return 1
      ;;
    *.ps1)
      file_contains "$_rel" "Norscode-first bridge" && return 0
      return 1
      ;;
    *) return 0 ;;
  esac
}

if "$ROOT/bin/nc" run "$ROOT/tools/verify_norscode_surface_ownership.no" >"$_out" 2>&1; then
  print_file "$_out"
  rm -f "$_out"
  exit 0
fi

if ! has_exec_gap "$_out"; then
  print_file "$_out" >&2
  rm -f "$_out"
  exit 1
fi
rm -f "$_out"

missing=0
while IFS= read -r p; do
  p="$(strip_dot_slash "$p")"
  is_active_rel "$p" || continue
  base="${p%.*}"
  if [ ! -f "$base.no" ]; then
    printf '%s manglar %s.no\n' "$p" "$base"
    missing=1
  fi
  if ! script_marker_ok "$p"; then
    printf '%s manglar Norscode-first markør/bridge eller eksplisitt unntak\n' "$p"
    missing=1
  fi
done <<EOF
$(find . -path ./.git -prune -o \( -name '*.sh' -o -name '*.ps1' -o -name '*.js' -o -name '*.swift' \) -print)
EOF

unexpected_c=""
while IFS= read -r c_file; do
  c_file="$(strip_dot_slash "$c_file")"
  is_active_rel "$c_file" || continue
  if ! is_known_c_artifact "$c_file"; then
    if [ -n "$unexpected_c" ]; then
      unexpected_c="${unexpected_c}
${c_file}"
    else
      unexpected_c="$c_file"
    fi
  fi
done <<EOF
$(find . -path ./.git -prune -o -name '*.c' -print)
EOF

if [ -n "$unexpected_c" ]; then
  printf 'uventa C-overflate:\n%s\n' "$unexpected_c"
  missing=1
fi

if [ "$missing" -eq 0 ]; then
  printf '=== Norscode surface ownership: BESTÅTT ===\n'
else
  printf '=== Norscode surface ownership: FEILA ===\n'
fi
exit "$missing"
