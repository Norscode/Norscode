#!/usr/bin/env bash
# Norscode-first wrapper: release-asset-bygg ligg i tools/build_stage0_release_assets.no.
# Shell-delen under er berre avgrensa reserveveg når runtime manglar exec_prosess.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT"
STAGE0_PLATFORMS="macos-arm64, macos-x86_64, linux-x86_64, linux-arm64"
if [ "${NORSCODE_STAGE0_PLATFORM:-}" = "" ]; then
  case "$(uname -s):$(uname -m)" in
    Darwin:arm64) NORSCODE_STAGE0_PLATFORM="macos-arm64" ;;
    Darwin:x86_64) NORSCODE_STAGE0_PLATFORM="macos-x86_64" ;;
    Linux:x86_64|Linux:amd64) NORSCODE_STAGE0_PLATFORM="linux-x86_64" ;;
    Linux:aarch64|Linux:arm64) NORSCODE_STAGE0_PLATFORM="linux-arm64" ;;
  esac
  export NORSCODE_STAGE0_PLATFORM
fi
case "${NORSCODE_STAGE0_PLATFORM:-}" in
  macos-arm64|macos-x86_64|linux-x86_64|linux-arm64) ;;
  "")
    printf 'Feil: ukjent plattform\n' >&2
    printf 'plattformar: %s\n' "$STAGE0_PLATFORMS" >&2
    exit 1
    ;;
  *)
    printf 'Feil: ukjent stage0-plattform: %s\n' "$NORSCODE_STAGE0_PLATFORM" >&2
    printf 'plattformar: %s\n' "$STAGE0_PLATFORMS" >&2
    exit 2
    ;;
esac

OUT="${TMPDIR:-/tmp}/norscode_stage0_release_assets_$$.log"
rc=0
cleanup_out() {
  rm -f "$OUT"
}
trap cleanup_out EXIT HUP INT TERM
first_word() {
  _text="$1"
  set -- $_text
  printf '%s' "${1:-}"
}
file_bytes() {
  _file="$1"
  first_word "$(wc -c "$_file")"
}
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
      *"Ukjent funksjon: builtin.exec_prosess"*|*"Ukjent funksjon: builtin.builtin.exec_prosess"*) return 0 ;;
    esac
  done < "$_file"
  return 1
}
"$ROOT/bin/nc" run "$ROOT/tools/build_stage0_release_assets.no" >"$OUT" 2>&1 || rc=$?
if [ "$rc" -eq 0 ]; then
  print_file "$OUT"
  exit 0
fi
if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
  print_file "$OUT"
  exit "$rc"
fi
cleanup_out

if [ ! -x "$ROOT/dist/norscode_native" ]; then
  "$ROOT/bin/nc" fetch-stage0-seed "$NORSCODE_STAGE0_PLATFORM"
fi

out_name="compiler_stage0-$NORSCODE_STAGE0_PLATFORM.elf"
out_dir="${NORSCODE_RELEASE_ARTIFACT_DIR:-$ROOT/release-artifacts/stage0}"
out_path="$out_dir/$out_name"
ncb="$ROOT/build/6b/compiler_stage0.ncb.json"
elf="$ROOT/build/6b/compiler_stage0.elf"
mkdir -p "$out_dir"
mkdir -p "$ROOT/build/6b"
"$ROOT/bin/nc" build-omgang6b-ncb "$ncb"
"$ROOT/bin/nc" ncb-to-elf "$ncb" "$elf"
cp "$elf" "$out_path"
chmod +x "$out_path"
if command -v sha256sum >/dev/null 2>&1; then
  set -- $(sha256sum "$out_path")
else
  set -- $(shasum -a 256 "$out_path")
fi
printf '%s  %s\n' "$1" "$out_name" > "$out_path.sha256"
printf '✓ %s (%s bytes)\n' "$out_path" "$(file_bytes "$out_path")"
