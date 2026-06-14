#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/build-windows-app-layout.sh [--out DIR] [--name NAMN] EXE_PATH

Byggjer ein enkel Windows app-layout rundt ein eksisterande norscode.exe.
Standard output: build/windows-app/Norscode/
EOF
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
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

if [ -z "$EXE_PATH" ]; then
  printf 'Feil: manglar path til Windows-native .exe\n' >&2
  exit 1
fi

case "$OUT_ROOT" in
  /*) ;;
  *) OUT_ROOT="$ROOT_DIR/$OUT_ROOT" ;;
esac

case "$EXE_PATH" in
  /*) ;;
  *) EXE_PATH="$ROOT_DIR/$EXE_PATH" ;;
esac

if [ ! -f "$EXE_PATH" ]; then
  printf 'Feil: fann ikkje .exe-artefakt: %s\n' "$EXE_PATH" >&2
  exit 1
fi

APP_DIR="$OUT_ROOT/$APP_NAME"
BIN_DIR="$APP_DIR/bin"
DOC_DIR="$APP_DIR/docs"

rm -rf "$APP_DIR"
mkdir -p "$BIN_DIR" "$DOC_DIR"

cp "$EXE_PATH" "$BIN_DIR/norscode.exe"
cp "$EXE_PATH" "$BIN_DIR/nc.exe"

cat > "$BIN_DIR/nc.cmd" <<'EOF'
@echo off
set SCRIPT_DIR=%~dp0
"%SCRIPT_DIR%norscode.exe" %*
EOF

cat > "$BIN_DIR/nc.ps1" <<'EOF'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $ScriptDir "norscode.exe") @args
EOF

cat > "$APP_DIR/README.txt" <<EOF
Norscode for Windows
====================

App-layout:
- bin\\norscode.exe
- bin\\nc.exe
- bin\\nc.cmd
- bin\\nc.ps1

Køyring:
- PowerShell: .\\bin\\nc.ps1 --help
- CMD: .\\bin\\nc.cmd --help
- Direkte: .\\bin\\norscode.exe --help
EOF

cat > "$DOC_DIR/LAYOUT.txt" <<EOF
Windows app-layout for Norscode
===============================

Denne katalogen er den første app-layouten for Windows-sporet.
Han gjer ZIP-distribusjonen meir stabil enn ein laus .exe, og er grunnlaget for seinare installasjon og oppgradering.
EOF

printf 'Bygde Windows app-layout: %s\n' "$APP_DIR"
