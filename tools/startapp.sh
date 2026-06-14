#!/usr/bin/env sh
set -eu

PROJECT_ROOT="."
APP_NAME=""
APP_PATH=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --path)
      shift
      if [ "$#" -eq 0 ]; then
        printf 'bruk: nc startapp <app_namn> [--path <prosjektrot>]\n' >&2
        exit 2
      fi
      PROJECT_ROOT="$1"
      ;;
    --help|-h)
      printf 'bruk: nc startapp <app_namn> [--path <prosjektrot>]\n'
      printf '  <app_namn>      Namn på ny app (obligatorisk)\n'
      printf '  --path          Katalogen med norcode.toml (standard: .)\n'
      exit 0
      ;;
    --*)
      printf 'ukjend flagg: %s\n' "$1" >&2
      exit 2
      ;;
    *)
      if [ -z "$APP_NAME" ]; then
        APP_NAME="$1"
      else
        printf 'for mange argument: %s\n' "$1" >&2
        exit 2
      fi
      ;;
  esac
  shift
done

if [ -z "$APP_NAME" ]; then
  printf 'bruk: nc startapp <app_namn> [--path <prosjektrot>]\n' >&2
  exit 2
fi

APP_PATH="$PROJECT_ROOT/apps/$APP_NAME"

if [ ! -f "$PROJECT_ROOT/norcode.toml" ]; then
  printf 'klarte ikkje finne norcode.toml i: %s\n' "$PROJECT_ROOT" >&2
  exit 2
fi

if [ -n "$(ls -A "$APP_PATH" 2>/dev/null)" ]; then
  printf 'feil: app-mappe er ikkje tom: %s\n' "$APP_PATH" >&2
  exit 2
fi

mkdir -p "$APP_PATH/tests"

cat > "$APP_PATH/${APP_NAME}.no" <<EOF_APP
// Appmodul: ${APP_NAME}

funksjon start() -> heltall {
    returner 0
}
EOF_APP

cat > "$APP_PATH/README.md" <<EOF_README
# ${APP_NAME}

Appmodul oppretta av `nc startapp`.

For å bruke appen i hovudapplikasjonen:

- import ${APP_NAME}
EOF_README

cat > "$APP_PATH/tests/test_${APP_NAME}.no" <<EOF_TEST
import ${APP_NAME}

funksjon test_start() {
    la r = ${APP_NAME}.start()
    assert(r == 0)
}
EOF_TEST

printf 'Oppretta app: %s\n' "$APP_PATH"
printf 'Testfil: %s/tests/test_%s.no\n' "$APP_PATH" "$APP_NAME"
printf 'Neste steg: importera appen frå hovudkoden\n'
