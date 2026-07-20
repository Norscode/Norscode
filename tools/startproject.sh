#!/usr/bin/env sh
# Norscode-first wrapper: prosjektgeneratoren har eigarlogikk i tools/startproject.no.
# Shell-delen under er avgrensa mal-/reserveveg medan store scaffold-malane blir flytta inn i Norscode.
# scaffold_test_conn
# INSERT INTO konto (namn) VALUES ('rulles_tilbake');
# Kontrolltesten skal ikkje bruke den gamle midlertidige raden.
set -eu
ROOT_DIR="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
PROJECT_DIR=""
OUTPUT_DIR=""
PROJECT_NAME=""
DB_FILE="app.db"
WITH_AUTH="usann"
WITH_ADMIN="usann"
usage() { printf 'bruk: nc startproject <målmappe> [--name <prosjektnamn>] [--path <sti>] [--db <db-fil>]\n' >&2; }
while [ "$#" -gt 0 ]; do
  case "$1" in
    --name) shift; [ "$#" -gt 0 ] || { usage; exit 2; }; PROJECT_NAME="$1";;
    --path) shift; [ "$#" -gt 0 ] || { usage; exit 2; }; OUTPUT_DIR="$1";;
    --db) shift; [ "$#" -gt 0 ] || { usage; exit 2; }; DB_FILE="$1";;
    --with-auth) WITH_AUTH="sann";; --with-admin) WITH_ADMIN="sann";;
    --without-auth) WITH_AUTH="usann";; --without-admin) WITH_ADMIN="usann";;
    --help|-h) usage; exit 0;;
    --*) printf 'ukjend flagg: %s\n' "$1" >&2; exit 2;;
    *) [ -n "$PROJECT_DIR" ] && [ -n "$PROJECT_NAME" ] && { printf 'for mange argument: %s\n' "$1" >&2; exit 2; }; [ -n "$PROJECT_DIR" ] && PROJECT_NAME="$1" || PROJECT_DIR="$1";;
  esac
  shift
done
[ -n "$PROJECT_DIR" ] || { usage; exit 2; }
[ -n "$PROJECT_NAME" ] || PROJECT_NAME="$(basename "$PROJECT_DIR")"
[ -n "$OUTPUT_DIR" ] || OUTPUT_DIR="$PROJECT_DIR"
if [ "${NORSCODE_STARTPROJECT_FROM_NO:-usann}" = "sann" ]; then
  [ ! -e "$OUTPUT_DIR" ] || [ -z "$(ls -A "$OUTPUT_DIR" 2>/dev/null)" ] || {
    printf 'feil: "%s" er ikkje tom\n' "$OUTPUT_DIR" >&2
    exit 2
  }
  mkdir -p "$OUTPUT_DIR"
  printf '[project]\nname = "%s"\nversion = "0.1.0"\nentry = "app.no"\n' "$PROJECT_NAME" > "$OUTPUT_DIR/norcode.toml"
  printf 'funksjon start() -> heiltall { skriv("Norscode prosjekt")\n returner 0 }\n' > "$OUTPUT_DIR/app.no"
  exit 0
fi
exec env NORSCODE_ENABLE_EXEC_PROSESS=1 NORSCODE_ROOT="$ROOT_DIR" NORSCODE_STARTPROJECT_DIR="$PROJECT_DIR" NORSCODE_STARTPROJECT_OUTPUT_DIR="$OUTPUT_DIR" NORSCODE_STARTPROJECT_NAME="$PROJECT_NAME" NORSCODE_STARTPROJECT_DB="$DB_FILE" NORSCODE_STARTPROJECT_WITH_AUTH="$WITH_AUTH" NORSCODE_STARTPROJECT_WITH_ADMIN="$WITH_ADMIN" "$ROOT_DIR/bin/nc" run "$ROOT_DIR/tools/startproject.no"
