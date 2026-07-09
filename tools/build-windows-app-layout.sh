#!/usr/bin/env sh
# Norscode-first wrapper: Windows app-layout ligg i tools/build-windows-app-layout.no.
# Shell-delen under er berre avgrensa reserveveg når runtime manglar exec_prosess.
set -eu

usage() {
  cat >&2 <<'EOF'
bruk: nc build-windows-app-layout [--out DIR] [--name NAMN] <EXE_PATH>

Byggjer ein enkel Windows app-layout rundt ein eksisterande norscode.exe.
Standard output: build/windows-app/Norscode/
EOF
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT_ROOT="$ROOT_DIR/build/windows-app"
APP_NAME="Norscode"
EXE_PATH=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --out)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      OUT_ROOT="$2"
      shift 2
      ;;
    --name)
      [ "$#" -ge 2 ] || { usage; exit 1; }
      APP_NAME="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      if [ -z "$EXE_PATH" ]; then
        EXE_PATH="$1"
        shift
      else
        usage
        exit 1
      fi
      ;;
  esac
done

export NORSCODE_ENABLE_EXEC_PROSESS="${NORSCODE_ENABLE_EXEC_PROSESS:-1}"
export NORSCODE_ROOT="$ROOT_DIR"
export NORSCODE_WINDOWS_LAYOUT_OUT="$OUT_ROOT"
export NORSCODE_WINDOWS_LAYOUT_NAME="$APP_NAME"
export NORSCODE_WINDOWS_EXE_PATH="$EXE_PATH"

OUT="${TMPDIR:-/tmp}/norscode_windows_layout_$$.log"
rc=0

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

"$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/build-windows-app-layout.no" >"$OUT" 2>&1 || rc=$?
if [ "$rc" -eq 0 ]; then
  print_file "$OUT"
  rm -f "$OUT"
  exit 0
fi
if [ -s "$OUT" ] && ! has_exec_gap "$OUT"; then
  print_file "$OUT"
  rm -f "$OUT"
  exit "$rc"
fi
rm -f "$OUT"

if [ -z "$EXE_PATH" ]; then
  printf 'Feil: manglar path til Windows-native .exe\n' >&2
  exit 1
fi
case "$EXE_PATH" in
  /*) EXE_ABS="$EXE_PATH" ;;
  *) EXE_ABS="$ROOT_DIR/$EXE_PATH" ;;
esac
case "$OUT_ROOT" in
  /*) OUT_ABS="$OUT_ROOT" ;;
  *) OUT_ABS="$ROOT_DIR/$OUT_ROOT" ;;
esac
if [ ! -f "$EXE_ABS" ]; then
  printf 'Feil: fann ikkje .exe-artefakt: %s\n' "$EXE_ABS" >&2
  exit 1
fi

app_dir="$OUT_ABS/$APP_NAME"
bin_dir="$app_dir/bin"
doc_dir="$app_dir/docs"
rm -rf "$app_dir"
mkdir -p "$bin_dir" "$doc_dir"
cp "$EXE_ABS" "$bin_dir/norscode.exe"
cp "$EXE_ABS" "$bin_dir/nc.exe"
cat > "$bin_dir/nc.cmd" <<'EOF'
@echo off
set SCRIPT_DIR=%~dp0
"%SCRIPT_DIR%norscode.exe" %*
EOF
cat > "$bin_dir/nc.ps1" <<'EOF'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $ScriptDir "norscode.exe") @args
EOF
cat > "$app_dir/README.txt" <<'EOF'
Norscode for Windows
====================

App-layout:
- bin/norscode.exe
- bin/nc.exe
- bin/nc.cmd
- bin/nc.ps1

Køyring:
- PowerShell: ./bin/nc.ps1 --help
- CMD: ./bin/nc.cmd --help
- Direkte: ./bin/norscode.exe --help
EOF
cat > "$doc_dir/LAYOUT.txt" <<'EOF'
Windows app-layout for Norscode
===============================

Denne katalogen er den første app-layouten for Windows-sporet.
Han gjer ZIP-distribusjonen meir stabil enn ein laus .exe, og er grunnlaget for seinare installasjon og oppgradering.
EOF
printf 'Bygde Windows app-layout: %s\n' "$app_dir"
