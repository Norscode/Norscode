#!/usr/bin/env sh
set -eu

PROJECT_DIR=""
OUTPUT_DIR=""
PROJECT_NAME=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --name)
      shift
      if [ "$#" -eq 0 ]; then
        printf 'bruk: startproject <målmappe> [--name <prosjektnavn>] [--path <sti>]\n' >&2
        exit 2
      fi
      PROJECT_NAME="$1"
      ;;
    --path)
      shift
      if [ "$#" -eq 0 ]; then
        printf 'bruk: startproject <målmappe> [--name <prosjektnavn>] [--path <sti>]\n' >&2
        exit 2
      fi
      OUTPUT_DIR="$1"
      ;;
    --help|-h)
      printf 'bruk: startproject <målmappe> [--name <prosjektnavn>] [--path <sti>]\n'
      printf '  <målmappe>   Målmappe for prosjektet\n'
      printf '  --name       Prosjektnavn i norcode.toml\n'
      printf '  --path       Alternativt utmatingsområde\n'
      exit 0
      ;;
    --*)
      printf 'ukjend flagg: %s\n' "$1" >&2
      exit 2
      ;;
    *)
      if [ -z "$PROJECT_DIR" ]; then
        PROJECT_DIR="$1"
      elif [ -z "$PROJECT_NAME" ]; then
        PROJECT_NAME="$1"
      else
        printf 'for mange argument: %s\n' "$1" >&2
        exit 2
      fi
      ;;
  esac
  shift
done

if [ -z "$PROJECT_DIR" ]; then
  printf 'bruk: startproject <målmappe> [--name <prosjektnavn>]\n' >&2
  exit 2
fi

if [ -z "$PROJECT_NAME" ]; then
  PROJECT_NAME="$(basename "$PROJECT_DIR")"
fi

if [ -z "$OUTPUT_DIR" ]; then
  OUTPUT_DIR="$PROJECT_DIR"
fi

if [ -n "$(ls -A "$OUTPUT_DIR" 2>/dev/null)" ]; then
  printf 'feil: "%s" er ikkje tom\n' "$OUTPUT_DIR" >&2
  exit 2
fi

mkdir -p "$OUTPUT_DIR/src" "$OUTPUT_DIR/apps/core" "$OUTPUT_DIR/apps/core/tests" "$OUTPUT_DIR/tests" "$OUTPUT_DIR/examples" "$OUTPUT_DIR/deploy"

cat > "$OUTPUT_DIR/norcode.toml" <<EOF_TOML
[project]
name = "${PROJECT_NAME}"
version = "0.1.0"
entry = "app.no"
EOF_TOML

cat > "$OUTPUT_DIR/app.no" <<'EOF_APP'
bruk std.web som web

import src.admin
import src.models
import src.views

// Hovudentrypunkt for appen.
// Strukturert som eit lite Django-liknende prosjekt:
// - src/models.no
// - src/views.no
// - src/admin.no
// - apps/core/*.no

funksjon api_hello(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/hello")
    returner src.views.hello(ctx)
}

funksjon api_products(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/products")
    returner src.views.products(ctx)
}

funksjon admin_panel(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /admin")
    returner web.response_builder(
        200,
        {"content-type": "text/html; charset=utf-8"},
        src.admin.render_panel(),
    )
}

funksjon start() -> heltall {
    la hello_req = web.request_context("get", "/api/hello", {}, {}, "")
    la products_req = web.request_context("get", "/api/products", {}, {}, "")
    la admin_req = web.request_context("get", "/admin", {}, {}, "")

    la r_api = web.handle_request(hello_req)
    la r_products = web.handle_request(products_req)
    la r_admin = web.handle_request(admin_req)

    la body_hello = web.response_body(r_api)
    la body_products = web.response_body(r_products)

    skriv("GET /api/hello = ")
    skriv(body_hello)
    skriv("\nGET /api/products = ")
    skriv(body_products)
    skriv("\nGET /admin = ")
    skriv(web.response_body(r_admin))

    assert_eq(web.response_code(r_api), 200)
    assert_eq(web.response_body(r_api), '{"message": "Hello World"}')
    assert_eq(web.response_code(r_products), 200)
    assert_eq(web.response_code(r_admin), 200)
    assert_neq(src.models.count_products(), 0)

    returner 0
}

EOF_APP

cat > "$OUTPUT_DIR/manage.no" <<'EOF_MANAGE'
bruk std.json som json

// Lite Django-inspirert kontroll-fil.
// Køyre gjerne: `./bin/nc run manage.no`

funksjon migrate() -> ordbok_tekst {
    returner json.object({"ok": true, "msg": "Mangler DB i demo-versjonen."})
}

funksjon start() -> heiltall {
    returner 0
}

EOF_MANAGE

cat > "$OUTPUT_DIR/src/models.no" <<'EOF_MODELS'
// In-memory modell i demo-versjon.

la produkter = [
    {"id": 1, "name": "Starter", "price": 199},
    {"id": 2, "name": "Pro", "price": 499},
    {"id": 3, "name": "Enterprise", "price": 1299},
]

funksjon all_products() -> liste_orb {
    returner produkter
}

funksjon count_products() -> heiltall {
    returner len(produkter)
}

EOF_MODELS

cat > "$OUTPUT_DIR/src/views.no" <<'EOF_VIEWS'
bruk std.web som web
import src.models

funksjon hello(ctx: ordbok_tekst) -> ordbok_tekst {
    returner web.response_builder(
        200,
        {"content-type": "application/json"},
        '{"message": "Hello World"}',
    )
}

funksjon products(ctx: ordbok_tekst) -> ordbok_tekst {
    bruk std.json som json

    la items = src.models.all_products()
    returner web.response_builder(200, {"content-type": "application/json"}, json.dumps(items))
}

EOF_VIEWS

cat > "$OUTPUT_DIR/src/admin.no" <<'EOF_ADMIN'
funksjon render_panel() -> tekst {
    returner "" +
        "<!doctype html>" +
        "<html><head><title>Admin</title></head>" +
        "<body><main>" +
        "<h1>Admin panel</h1>" +
        "<p>Dette er eit demo-adminpanel på Django-nivå.</p>" +
        "<ul>" +
        "<li><a href=\"/api/hello\">API hello</a></li>" +
        "<li><a href=\"/api/products\">API products</a></li>" +
        "</ul>" +
        "</main></body></html>"
}

EOF_ADMIN

cat > "$OUTPUT_DIR/src/urls.no" <<'EOF_URLS'
// Eksempel på URL-konfigurasjon.
// Appen registrerer ruter i app.no via web.route.

funksjon overview() -> ordbok_tekst {
    returner {
        "routes": [
            "/api/hello",
            "/api/products",
            "/admin",
        ],
    }
}

EOF_URLS

cat > "$OUTPUT_DIR/apps/core/core.no" <<'EOF_CORE'
// Eit Django-liknende app-modul.

funksjon start() -> heiltall {
    returner 0
}

EOF_CORE

cat > "$OUTPUT_DIR/apps/core/README.no" <<'EOF_CORE_README'
# App: core

Dette er ein enkel app-modul som matcher Django-strukturen (`apps/<app_namn>/`).

I eit normalt prosjekt kan du leggje forretningslogikk her, og halde alt
administrativt og API-kode i `src/` + app-filer i `apps/`.

EOF_CORE_README

cat > "$OUTPUT_DIR/apps/core/tests/test_core.no" <<'EOF_CORE_TEST'
import core

funksjon test_start() {
    la svar = core.start()
    assert(svar == 0)
}

EOF_CORE_TEST

cat > "$OUTPUT_DIR/src/routes.no" <<EOF_ROUTES
// Dette er ein plass å plassere fleire ruter over tid.
EOF_ROUTES

cat > "$OUTPUT_DIR/README.md" <<'EOF_README'
# ${PROJECT_NAME}

Startpunkt for ny Norscode-app med eit lite API-demoprosjekt.

Dette prosjektet inneheld ein Django-inspirert struktur:

- `app.no` for hovudruter.
- `manage.no` med enkle prosjekt-kommandoar.
- `src/admin.no`, `src/models.no`, `src/urls.no`, `src/views.no`.
- `apps/core/` som app-modulstruktur.
- `norcode.toml`, `tests/`, `examples/`, `deploy/`.

Ruter i demoen:

- `GET /api/hello` → JSON hello-world.
- `GET /api/products` → JSON med fleire produktar.
- `GET /admin` → HTML admin panel.

## Kom i gang

- Køyre sjekk: `./bin/nc check app.no`
- Køyre appen: `./bin/nc run app.no`
- Køyre demo-kommandoar: `./bin/nc run manage.no`
- Køyre test: `./bin/nc test tests/test_app.no`

Admin-panelet blir rendera frå `src/admin.no`.
EOF_README

cat > "$OUTPUT_DIR/tests/test_app.no" <<EOF_TEST
import app

funksjon test_start() {
    la svar = app.start()
    assert_eq(svar, 0)
}
EOF_TEST

cat > "$OUTPUT_DIR/examples/ping.no" <<EOF_EX
bruk std.web som web

funksjon ping(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /ping")
    returner web.response_builder(200, {"content-type": "text/plain"}, "pong")
}
EOF_EX

cat > "$OUTPUT_DIR/deploy/norscode.service" <<EOF_SERVICE
[Unit]
Description=Norscode app service
After=network.target

[Service]
Type=simple
WorkingDirectory=${OUTPUT_DIR}
ExecStart=/usr/local/bin/nc run ${OUTPUT_DIR}/app.no
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF_SERVICE

printf 'Oppretta nytt prosjekt: %s\n' "$OUTPUT_DIR"
printf 'Prosjektnamn: %s\n' "$PROJECT_NAME"
printf 'Neste steg: cd %s && ./bin/nc check app.no\n' "$OUTPUT_DIR"
