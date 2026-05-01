#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXT_DIR="$ROOT_DIR/vscode-norscode"
TMP_DIR="$(mktemp -d)"
VSIX_PATH="$TMP_DIR/norscode-vscode-ide.vsix"

if ! command -v code >/dev/null 2>&1; then
  cat <<'MSG'
VS Code-cli ('code') ble ikke funnet.
Installer Visual Studio Code først, og legg CLI-kommandoen i PATH:
- Åpne Command Palette → 'Shell Command: Install 'code' command in PATH'.
MSG
  exit 1
fi

if [ ! -d "$EXT_DIR" ]; then
  echo "Fant ikke VSCode-extension i $EXT_DIR"
  exit 1
fi

if ! command -v node >/dev/null 2>&1; then
  echo "Node.js er påkrevd for å bygge .vsix med vsce. Installer Node.js og prøv igjen."
  exit 1
fi

pushd "$EXT_DIR" >/dev/null
if ! command -v vsce >/dev/null 2>&1; then
  echo "Installerer vsce lokalt med npx..."
  npx --yes @vscode/vsce package --out "$VSIX_PATH"
else
  vsce package --out "$VSIX_PATH"
fi
popd >/dev/null

code --install-extension "$VSIX_PATH" --force
rm -rf "$TMP_DIR"

echo "Ferdig. VSCode-utvidelsen er installert fra: $VSIX_PATH"
