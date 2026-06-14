#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Bruk: bash tools/notarize-macos-app.sh <artefakt.zip|artefakt.dmg|artefakt.pkg>

Notariserer eit macOS-artefakt med xcrun notarytool.
Krev desse miljøvariablane:
  APPLE_ID
  APPLE_TEAM_ID
  APPLE_APP_PASSWORD
EOF
}

if [ "$#" -ne 1 ]; then
  usage
  exit 1
fi

ARTIFACT="$1"

for required in APPLE_ID APPLE_TEAM_ID APPLE_APP_PASSWORD; do
  if [ -z "${!required:-}" ]; then
    printf 'Feil: manglar miljøvariabel %s\n' "$required" >&2
    exit 1
  fi
done

if [ "$(uname -s)" != "Darwin" ]; then
  printf 'Feil: notarize-macos-app krev macOS.\n' >&2
  exit 1
fi

if [ ! -f "$ARTIFACT" ]; then
  printf 'Feil: manglar artefakt: %s\n' "$ARTIFACT" >&2
  exit 1
fi

if ! command -v xcrun >/dev/null 2>&1; then
  printf 'Feil: manglar xcrun/notarytool\n' >&2
  exit 1
fi

printf 'Notariserer artefakt: %s\n' "$ARTIFACT"

xcrun notarytool submit "$ARTIFACT" \
  --apple-id "$APPLE_ID" \
  --team-id "$APPLE_TEAM_ID" \
  --password "$APPLE_APP_PASSWORD" \
  --wait

case "$ARTIFACT" in
  *.dmg|*.app)
    printf 'Staplar notarization-resultat for %s\n' "$ARTIFACT"
    xcrun stapler staple "$ARTIFACT"
    ;;
  *)
    printf 'Merk: stapling blir ikkje køyrd for denne artefakttypen.\n'
    ;;
esac

printf 'Notarization ferdig.\n'
