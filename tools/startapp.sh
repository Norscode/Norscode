#!/usr/bin/env sh
set -eu

PROJECT_ROOT="."
APP_NAME=""
APP_PATH=""

usage() {
  printf 'bruk: nc startapp <app_namn> [--path <prosjektrot>]\n' >&2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --path)
      shift
      [ "$#" -gt 0 ] || { usage; exit 2; }
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

[ -n "$APP_NAME" ] || { usage; exit 2; }

APP_PATH="$PROJECT_ROOT/apps/$APP_NAME"

if [ ! -f "$PROJECT_ROOT/norcode.toml" ]; then
  printf 'klarte ikkje finne norcode.toml i: %s\n' "$PROJECT_ROOT" >&2
  exit 2
fi

if [ -e "$APP_PATH" ] && [ -n "$(ls -A "$APP_PATH" 2>/dev/null)" ]; then
  printf 'feil: app-mappe er ikkje tom: %s\n' "$APP_PATH" >&2
  exit 2
fi

mkdir -p \
  "$APP_PATH/tests" \
  "$APP_PATH/tests/payloads" \
  "$APP_PATH/migrations" \
  "$APP_PATH/templates" \
  "$APP_PATH/templates/$APP_NAME"

cat > "$APP_PATH/${APP_NAME}.no" <<EOF_APP
// Appmodul: ${APP_NAME}

funksjon start() -> heiltall {
    returner 0
}
EOF_APP

cat > "$APP_PATH/views.no" <<EOF_VIEWS
bruk std.web som web
bruk std.mal som mal
import ${APP_NAME}.forms

funksjon dep_app_meta(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = web.dependency("app_meta")
    la _ = ctx
    returner {
        "app": "${APP_NAME}",
        "enabled": "sann"
    }
}

funksjon index(ctx: ordbok_tekst, app_meta: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /${APP_NAME}")
    web.use_dependency("app_meta")
    la _ = app_meta
    la _ = ctx
    la innhald = mal.gjengi_fil("apps/${APP_NAME}/templates/${APP_NAME}/index.html", {
        "app_namn": "${APP_NAME}"
    })
    returner web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, innhald)
}

funksjon api_health(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/health")
    la _ = ctx
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"app\":\"${APP_NAME}\"}")
}

funksjon api_echo(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/${APP_NAME}/echo")
    prøv {
        la payload = forms.parse_json_body(ctx)
        la body = "{\"ok\":\"sann\",\"payload\":" + builtin.json_stringify(payload) + "}"
        returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, body)
    } fang (e) {
        la _ = e
        returner web.validation_bad_request(["body må vere gyldig JSON-objekt"])
    }
}

funksjon api_openapi(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/openapi.json")
    la _ = ctx
    la spec = web.openapi_json("${APP_NAME} API", "1.0.0")
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, spec)
}

funksjon api_docs(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/docs")
    la _ = ctx
    la html = web.docs_html("${APP_NAME} API", "1.0.0")
    returner web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, html)
}

funksjon api_meta(ctx: ordbok_tekst, app_meta: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/meta")
    web.use_dependency("app_meta")
    la _ = ctx
    la app = app_meta["app"]
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"app\":\"" + app + "\"}")
}

funksjon api_dependency(ctx: ordbok_tekst, app_meta: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/dependency")
    web.use_dependency("app_meta")
    la _ = ctx
    la app = app_meta["app"]
    la sti = web.request_path(ctx)
    la metode = web.request_method(ctx)
    la body = "{\"ok\":\"sann\",\"app\":\"" + app + "\",\"path\":\"" + sti + "\",\"method\":\"" + metode + "\"}"
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, body)
}

funksjon api_error(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/error")
    la mode = web.request_query_param(ctx, "mode")
    hvis mode == "bad" {
        returner web.response_error(400, "Bad Request")
    }
    hvis mode == "server" {
        returner web.response_error(500, "Intern feil")
    }
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"app\":\"${APP_NAME}\"}")
}

funksjon api_auth_login(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/${APP_NAME}/auth/login")
    la feil: liste<tekst> = []
    la payload: ordbok_tekst = {}
    prøv {
        payload = web.request_json(ctx)
    } fang (e) {
        la _ = e
        returner web.validation_bad_request(["body må vere gyldig JSON-objekt"])
    }
    la epost = web.request_json_text_or_error(payload, "epost", feil)
    la passord = web.request_json_text_or_error(payload, "passord", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    hvis epost == "" eller passord == "" {
        returner web.validation_bad_request(["epost og passord er påkravde"])
    }
    hvis epost != "demo@example.com" eller passord != "hemmelig" {
        returner web.response_error(401, "Ugyldig innlogging")
    }
    la token = "token:" + epost
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"token\":\"" + token + "\",\"brukar\":\"" + epost + "\"}")
}

funksjon api_auth_login_form(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/auth/login")
    la _ = ctx
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"form\":\"auth/login\",\"skjemafelt\":[\"epost\",\"passord\"]}")
}

funksjon api_auth_register(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/${APP_NAME}/auth/register")
    la feil: liste<tekst> = []
    la payload: ordbok_tekst = {}
    prøv {
        payload = web.request_json(ctx)
    } fang (e) {
        la _ = e
        returner web.validation_bad_request(["body må vere gyldig JSON-objekt"])
    }
    la epost = web.request_json_text_or_error(payload, "epost", feil)
    la passord = web.request_json_text_or_error(payload, "passord", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    hvis epost == "" eller passord == "" {
        returner web.validation_bad_request(["epost og passord er påkravde"])
    }
    la token = "token:" + epost
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"brukar\":\"" + epost + "\",\"token\":\"" + token + "\"}")
}

funksjon api_auth_register_form(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/auth/register")
    la _ = ctx
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"form\":\"auth/register\",\"skjemafelt\":[\"epost\",\"passord\"]}")
}

funksjon api_auth_logout(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/${APP_NAME}/auth/logout")
    la _ = ctx
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"utlogging\":\"fullført\"}")
}

funksjon api_auth_profile(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/profile")
    la feil: liste<tekst> = []
    la token = web.request_query_required_or_error(ctx, "token", feil)
    hvis lengde(feil) > 0 {
        returner web.response_error(401, "Ikkje logga inn")
    }
    hvis token != "token:demo@example.com" {
        returner web.response_error(401, "Ugyldig token")
    }
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"brukar\":{\"epost\":\"demo@example.com\"}}")
}

funksjon api_query(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/query")
    la feil: liste<tekst> = []
    la namn = web.request_query_required_or_error(ctx, "name", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"name\":\"" + namn + "\"}")
}

funksjon api_validate(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/${APP_NAME}/validate")
    la feil: liste<tekst> = []
    la payload: ordbok_tekst = {}
    prøv {
        payload = web.request_json(ctx)
    } fang (e) {
        la _ = e
        returner web.validation_bad_request(["body må vere gyldig JSON-objekt"])
    }
    la name = ""
    hvis har_nokkel(payload, "name") { name = tekst(payload["name"]) }
    hvis name == "" { legg_til(feil, "name er påkravd") }
    la age: heiltall = web.request_json_int_or_error(payload, "age", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    la body = "{\"ok\":\"sann\",\"name\":\"" + name + "\",\"age\":" + tekst_fra_heltall(age) + "}"
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, body)
}

funksjon api_item(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/items/{id}")
    la feil: liste<tekst> = []
    la id = web.request_param_int_or_error(ctx["__params__"], "id", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"id\":" + tekst_fra_heltall(id) + "}")
}

funksjon api_payload(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/${APP_NAME}/payload")
    la feil: liste<tekst> = []
    la payload: ordbok_tekst = {}
    prøv {
        payload = web.request_json(ctx)
    } fang (e) {
        la _ = e
        returner web.validation_bad_request(["body må vere gyldig JSON-objekt"])
    }
    la title = web.request_json_text_or_error(payload, "title", feil)
    la count = web.request_json_int_or_error(payload, "count", feil)
    la active = web.request_json_bool_or_error(payload, "active", feil)
    la tags: liste<tekst> = web.request_json_list_or_error(payload, "tags", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    la response_payload = {
        "ok": "sann",
        "title": title,
        "count": count,
        "active": active,
        "tags": tags
    }
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, builtin.json_stringify(response_payload))
}

funksjon api_nested(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/${APP_NAME}/nested")
    la feil: liste<tekst> = []
    la payload: ordbok_tekst = {}
    prøv {
        payload = web.request_json(ctx)
    } fang (e) {
        la _ = e
        returner web.validation_bad_request(["body må vere gyldig JSON-objekt"])
    }
    la user: ordbok_tekst = web.request_json_object_or_error(payload, "user", feil)
    la meta: ordbok_tekst = web.request_json_object_or_error(payload, "meta", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    la user_name = web.request_json_text_or_error(user, "name", feil)
    la user_age: heiltall = web.request_json_int_or_error(user, "age", feil)
    la source = web.request_json_text_or_error(meta, "source", feil)
    la ref = ""
    hvis har_nokkel(meta, "ref") { ref = tekst(meta["ref"]) }
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    la response_payload = {
        "ok": "sann",
        "user": {
            "name": user_name,
            "age": user_age
        },
        "meta": {
            "source": source
        }
    }
    hvis ref != "" { response_payload["meta"] = {
        "source": source,
        "ref": ref
    } }
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, builtin.json_stringify(response_payload))
}

funksjon api_headers(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/headers")
    la feil: liste<tekst> = []
    la request_id = web.request_header_or_error(ctx, "x-request-id", feil)
    la x_count = web.request_header_int_or_error(ctx, "x-count", feil)
    la x_active = web.request_header_bool_or_error(ctx, "x-active", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    la active_text = "false"
    hvis x_active { active_text = "true" }
    la body = "{\"ok\":\"sann\",\"request_id\":\"" + request_id + "\",\"count\":" + tekst_fra_heltall(x_count) + ",\"active\":" + active_text + "}"
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, body)
}

funksjon api_response_model(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/${APP_NAME}/response-model")
    la mode = web.request_query_param(ctx, "mode")
    la body_payload: ordbok_tekst = {}
    hvis mode == "bad" {
        la feil: liste<tekst> = []
        la u_validert: ordbok_tekst = {
            "ok": sann,
            "count": "tre",
            "active": sann,
            "items": "ikkje_liste"
        }
        web.response_shape_validate_or_error(u_validert, {
            "ok": "tekst",
            "count": "heltall",
            "active": "boolsk",
            "items": "liste"
        }, feil)
        hvis lengde(feil) > 0 {
            la svar = {"ok":"usann","error":"ResponseValidationError","details":feil}
            returner web.response_builder(500, {"content-type":"application/json; charset=utf-8"}, builtin.json_stringify(svar))
        }
    }
    body_payload = {
        "ok": "sann",
        "count": 2,
        "active": sann,
        "items": ["a","b","c"]
    }
    la err: liste<tekst> = []
    web.response_shape_validate_or_error(body_payload, {
        "ok": "tekst",
        "count": "heltall",
        "active": "boolsk",
        "items": "liste"
    }, err)
    hvis lengde(err) > 0 {
        la svar = {"ok":"usann","error":"ResponseValidationError","details":err}
        returner web.response_builder(500, {"content-type":"application/json; charset=utf-8"}, builtin.json_stringify(svar))
    }
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, builtin.json_stringify(body_payload))
}

funksjon api_request_model(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/${APP_NAME}/request-model")
    la feil: liste<tekst> = []
    la payload: ordbok_tekst = {}
    prøv {
        payload = forms.parse_json_body(ctx)
    } fang (e) {
        la _ = e
        returner web.validation_bad_request(["body må vere gyldig JSON-objekt"])
    }
    la title = web.request_json_text_or_error(payload, "title", feil)
    la count = web.request_json_int_or_error(payload, "count", feil)
    la active = web.request_json_bool_or_error(payload, "active", feil)
    la tags: liste<tekst> = web.request_json_list_or_error(payload, "tags", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    la response_payload = {
        "ok": "sann",
        "title": title,
        "count": count,
        "active": active,
        "tags": tags
    }
    la err: liste<tekst> = []
    web.response_shape_validate_or_error(response_payload, {
        "ok": "tekst",
        "title": "tekst",
        "count": "heltall",
        "active": "boolsk",
        "tags": "liste"
    }, err)
    hvis lengde(err) > 0 {
        la svar = {"ok":"usann","error":"ResponseValidationError","details":err}
        returner web.response_builder(500, {"content-type":"application/json; charset=utf-8"}, builtin.json_stringify(svar))
    }
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, builtin.json_stringify(response_payload))
}
EOF_VIEWS

cat > "$APP_PATH/forms.no" <<EOF_FORMS
bruk std.web som web

funksjon parse_form_data(ctx: ordbok_tekst) -> ordbok_tekst {
    la body = web.request_body(ctx)
    la data: ordbok_tekst = {}
    hvis body == "" { returner data }
    la par = builtin.split(body, "&")
    la i: heiltall = 0
    mens i < lengde(par) {
        la del = builtin.trim(par[i])
        la eq = builtin.index_of(del, "=")
        hvis eq > 0 {
            la nøkkel = _url_decode(builtin.slice(del, 0, eq))
            la verdi = _url_decode(builtin.slice(del, eq + 1, lengde(del)))
            data[nøkkel] = verdi
        }
        i = i + 1
    }
    returner data
}

funksjon _url_decode(s: tekst) -> tekst {
    la r = tekst_erstatt(s, "+", " ")
    r = tekst_erstatt(r, "%40", "@")
    r = tekst_erstatt(r, "%20", " ")
    r = tekst_erstatt(r, "%2F", "/")
    r = tekst_erstatt(r, "%3A", ":")
    r = tekst_erstatt(r, "%3F", "?")
    r = tekst_erstatt(r, "%3D", "=")
    r = tekst_erstatt(r, "%26", "&")
    r = tekst_erstatt(r, "%2B", "+")
    r = tekst_erstatt(r, "%25", "%")
    returner r
}

funksjon parse_json_body(ctx: ordbok_tekst) -> ordbok_tekst {
    la payload = web.request_json(ctx)
    returner payload
}
EOF_FORMS

cat > "$APP_PATH/models.no" <<'EOF_MODELS'
funksjon schema() -> liste<tekst> {
    la felt: liste<tekst> = []
    legg_til(felt, "id INTEGER PRIMARY KEY AUTOINCREMENT")
    returner felt
}

funksjon migrasjonar() -> liste<ordbok_tekst> {
    returner []
}
EOF_MODELS

cat > "$APP_PATH/routes.no" <<EOF_ROUTES
funksjon index() -> ordbok_tekst {
    returner {
        "GET /${APP_NAME}": "index",
        "GET /api/v1/${APP_NAME}/health": "api_health",
        "POST /api/v1/${APP_NAME}/echo": "api_echo",
        "GET /api/v1/${APP_NAME}/openapi.json": "api_openapi",
        "GET /api/v1/${APP_NAME}/docs": "api_docs",
        "GET /api/v1/${APP_NAME}/meta": "api_meta",
        "GET /api/v1/${APP_NAME}/dependency": "api_dependency",
        "GET /api/v1/${APP_NAME}/error": "api_error",
        "GET /api/v1/${APP_NAME}/auth/login": "api_auth_login_form",
        "POST /api/v1/${APP_NAME}/auth/login": "api_auth_login",
        "GET /api/v1/${APP_NAME}/auth/register": "api_auth_register_form",
        "POST /api/v1/${APP_NAME}/auth/register": "api_auth_register",
        "POST /api/v1/${APP_NAME}/auth/logout": "api_auth_logout",
        "GET /api/v1/${APP_NAME}/profile": "api_auth_profile",
        "GET /api/v1/${APP_NAME}/query": "api_query",
        "POST /api/v1/${APP_NAME}/validate": "api_validate",
        "GET /api/v1/${APP_NAME}/items/{id}": "api_item",
        "POST /api/v1/${APP_NAME}/payload": "api_payload",
        "POST /api/v1/${APP_NAME}/nested": "api_nested",
        "GET /api/v1/${APP_NAME}/headers": "api_headers",
        "GET /api/v1/${APP_NAME}/response-model": "api_response_model",
        "POST /api/v1/${APP_NAME}/request-model": "api_request_model"
    }
}
EOF_ROUTES

cat > "$APP_PATH/dispatcher.no" <<EOF_DISPATCH
bruk std.web som web
import ${APP_NAME}.views
import ${APP_NAME}.routes

funksjon dispatcher(ctx: ordbok_tekst) -> ordbok_tekst {
    returner web.handle_request(ctx)
}
EOF_DISPATCH

cat > "$APP_PATH/README.md" <<EOF_README
# ${APP_NAME}

Appmodul oppretta av \`nc startapp\`.

Filer i denne appen:

- \`${APP_NAME}.no\` — modul-API.
- \`views.no\` — handteringsfunksjonar.
- \`forms.no\` — form- og JSON-parsing.
- \`models.no\` — modeldefinisjonar og migrasjonar.
- \`routes.no\` — app-ruter.
- \`dispatcher.no\` — app-dispatcher med ruteoppløysing.
- \`tests/\` — testmalar.
- \`tests/payloads/\` — API-eksempelpayload.
  - Ekstra API-ruter i malen:
  - \`/api/v1/\${APP_NAME}/meta\`
  - \`/api/v1/\${APP_NAME}/dependency\`
  - \`/api/v1/\${APP_NAME}/error\`
  - \`/api/v1/\${APP_NAME}/auth/login\`
  - \`/api/v1/\${APP_NAME}/auth/register\`
  - \`/api/v1/\${APP_NAME}/auth/logout\`
  - \`/api/v1/\${APP_NAME}/profile\`
  - \`/api/v1/\${APP_NAME}/query\`
  - \`/api/v1/\${APP_NAME}/validate\`
  - \`/api/v1/\${APP_NAME}/items/{id}\`
  - \`/api/v1/\${APP_NAME}/payload\`
  - \`/api/v1/\${APP_NAME}/nested\`
  - \`/api/v1/\${APP_NAME}/headers\`
  - \`/api/v1/\${APP_NAME}/response-model\`
  - \`/api/v1/\${APP_NAME}/request-model\`
EOF_README

cat > "$APP_PATH/tests/test_${APP_NAME}.no" <<EOF_TEST
import ${APP_NAME}
import ${APP_NAME}.dispatcher
import ${APP_NAME}.routes
bruk std.test som test
bruk std.web som web

funksjon test_start() {
    la r = ${APP_NAME}.start()
    assert_eq(r, 0)
}

funksjon test_routes_inneheld() {
    la ruter = routes.index()
    assert(har_nokkel(ruter, "GET /${APP_NAME}"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/health"))
    assert(har_nokkel(ruter, "POST /api/v1/${APP_NAME}/echo"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/openapi.json"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/docs"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/meta"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/dependency"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/error"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/auth/login"))
    assert(har_nokkel(ruter, "POST /api/v1/${APP_NAME}/auth/login"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/auth/register"))
    assert(har_nokkel(ruter, "POST /api/v1/${APP_NAME}/auth/register"))
    assert(har_nokkel(ruter, "POST /api/v1/${APP_NAME}/auth/logout"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/profile"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/query"))
    assert(har_nokkel(ruter, "POST /api/v1/${APP_NAME}/validate"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/items/{id}"))
    assert(har_nokkel(ruter, "POST /api/v1/${APP_NAME}/payload"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/headers"))
    assert(har_nokkel(ruter, "POST /api/v1/${APP_NAME}/nested"))
    assert(har_nokkel(ruter, "GET /api/v1/${APP_NAME}/response-model"))
    assert(har_nokkel(ruter, "POST /api/v1/${APP_NAME}/request-model"))
}

funksjon test_dispatch_index() {
    la ctx = web.request_context("GET", "/${APP_NAME}", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    assert_eq(web.response_status(svar), 200)
}

funksjon test_api_health() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/health", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"ok\":\"sann\""))
}

funksjon test_api_echo_payload() {
    la body = builtin.json_stringify({"melding": "hei", "app": "${APP_NAME}"})
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/echo", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_ut = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_ut, "\"melding\":\"hei\""))
}

funksjon test_api_echo_route_feil() {
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/echo", {"content-type": "application/json"}, {"host": "localhost"}, "ikkje json")
    la svar = dispatcher.dispatcher(ctx)
    la body_ut = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_ut, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_ut, "\"body må vere gyldig JSON-objekt\""))
}

funksjon test_api_openapi_route() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/openapi.json", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"openapi\":\"3.0.3\""))
    assert(tekst_inneholder(body, "\"/api/v1/${APP_NAME}/health\""))
    assert(tekst_inneholder(body, "\"/api/v1/${APP_NAME}/response-model\""))
    assert(tekst_inneholder(body, "\"/api/v1/${APP_NAME}/request-model\""))
}

funksjon test_api_docs_route() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/docs", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "<h1>${APP_NAME} API</h1>"))
}

funksjon test_api_meta_route() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/meta", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"app\":\"${APP_NAME}\""))
}

funksjon test_api_dependency_route() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/dependency", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body, "\"app\":\"${APP_NAME}\""))
    assert(tekst_inneholder(body, "\"path\":\"/api/v1/${APP_NAME}/dependency\""))
    assert(tekst_inneholder(body, "\"method\":\"GET\""))
}

funksjon test_api_auth_login_form() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/auth/login", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"form\":\"auth/login\""))
}

funksjon test_api_auth_login_route() {
    la body = "{\"epost\":\"demo@example.com\",\"passord\":\"hemmelig\"}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/auth/login", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"token\":\"token:demo@example.com\""))
}

funksjon test_api_auth_login_route_feil() {
    la body = "{\"epost\":\"demo@example.com\",\"passord\":\"feil\"}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/auth/login", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 401)
    assert(tekst_inneholder(body_res, "\"error\":\"Ugyldig innlogging\""))
}

funksjon test_api_auth_register_form() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/auth/register", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"form\":\"auth/register\""))
}

funksjon test_api_auth_register_route() {
    la body = "{\"epost\":\"ny@bruker.no\",\"passord\":\"sterk\"}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/auth/register", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"brukar\":\"ny@bruker.no\""))
}

funksjon test_api_auth_register_route_feil() {
    la body = "{\"epost\":\"ny@bruker.no\"}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/auth/register", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
}

funksjon test_api_auth_logout() {
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/auth/logout", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body, "\"utlogging\":\"fullført\""))
}

funksjon test_api_auth_profile() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/profile?token=token:demo@example.com", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body, "\"epost\":\"demo@example.com\""))
}

funksjon test_api_auth_profile_ikkje_logga_inn() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/profile", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 401)
    assert(tekst_inneholder(body, "\"error\":\"Ikkje logga inn\""))
}

funksjon test_api_auth_profile_ugyldig_token() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/profile?token=ikkje-riktig", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 401)
    assert(tekst_inneholder(body, "\"error\":\"Ugyldig token\""))
}

funksjon test_api_404_route() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/ikkje-finst", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 404)
    assert(tekst_inneholder(body, "\"error\":"))
}

funksjon test_api_405_route() {
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/query", {"content-type":"application/json"}, {"host": "localhost"}, "{}")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 405)
    assert(tekst_inneholder(body, "\"error\":\"Metode ikkje tillatt\""))
}

funksjon test_api_error_route_ok() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/error", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body, "\"app\":\"${APP_NAME}\""))
}

funksjon test_api_error_route_bad() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/error?mode=bad", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 400)
    assert(tekst_inneholder(body, "\"error\":\"Bad Request\""))
}

funksjon test_api_error_route_server() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/error?mode=server", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 500)
    assert(tekst_inneholder(body, "\"error\":\"Intern feil\""))
}

funksjon test_api_query_route() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/query", {"name":"demo"}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"name\":\"demo\""))
}

funksjon test_api_query_route_feil() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/query", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body, "\"name er påkravd\""))
}

funksjon test_api_validate_route() {
    la body = "{\"name\":\"Ada\",\"age\":23}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/validate", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"name\":\"Ada\""))
    assert(tekst_inneholder(body_res, "\"age\":23"))
}

funksjon test_api_validate_route_feil() {
    la body = "{\"name\":\"\",\"age\":\"noe\"}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/validate", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"name er påkravd\""))
    assert(tekst_inneholder(body_res, "\"age må vere heiltal\""))
}

funksjon test_api_payload_route() {
    la body = "{\"title\":\"demo\",\"count\":9,\"active\":true,\"tags\":[\"one\",\"two\"]}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/payload", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"title\":\"demo\""))
    assert(tekst_inneholder(body_res, "\"count\":9"))
    assert(tekst_inneholder(body_res, "\"active\":true"))
    assert(tekst_inneholder(body_res, "\"tags\":[\"one\",\"two\"]"))
}

funksjon test_api_payload_route_feil() {
    la body = "{\"count\":\"abc\",\"active\":\"ja\",\"tags\":{}}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/payload", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"title er påkravd\""))
    assert(tekst_inneholder(body_res, "\"count må vere heiltal\""))
    assert(tekst_inneholder(body_res, "\"active må vere boolsk\""))
    assert(tekst_inneholder(body_res, "\"tags må vere liste\""))
}

funksjon test_api_nested_route() {
    la body = "{\"user\":{\"name\":\"Ola\",\"age\":33},\"meta\":{\"source\":\"cli\",\"ref\":\"v1\"}}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/nested", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"name\":\"Ola\""))
    assert(tekst_inneholder(body_res, "\"age\":33"))
    assert(tekst_inneholder(body_res, "\"source\":\"cli\""))
    assert(tekst_inneholder(body_res, "\"ref\":\"v1\""))
}

funksjon test_api_nested_route_feil() {
    la body = "{\"user\":{\"age\":\"bad\"},\"meta\":{\"source\":false}}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/nested", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"name er påkravd\""))
    assert(tekst_inneholder(body_res, "\"age må vere heiltal\""))
    assert(tekst_inneholder(body_res, "\"source må vere tekst\""))
}

funksjon test_api_headers_route() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/headers", {}, {"host": "localhost", "x-request-id":"app-42", "x-count":"8", "x-active":"true"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"request_id\":\"app-42\""))
    assert(tekst_inneholder(body_res, "\"count\":8"))
    assert(tekst_inneholder(body_res, "\"active\":true"))
}

funksjon test_api_headers_route_feil() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/headers", {}, {"host": "localhost", "x-request-id":"", "x-count":"nei", "x-active":"ja"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"x-request-id er påkravd\""))
    assert(tekst_inneholder(body_res, "\"x-count må vere heiltal\""))
    assert(tekst_inneholder(body_res, "\"x-active må vere boolsk\""))
}

funksjon test_api_response_model_route() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/response-model?mode=good", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"count\":2"))
    assert(tekst_inneholder(body_res, "\"active\":true"))
}

funksjon test_api_response_model_route_feil() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/response-model?mode=bad", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 500)
    assert(tekst_inneholder(body_res, "\"error\":\"ResponseValidationError\""))
    assert(tekst_inneholder(body_res, "\"count må vere heiltall\""))
}

funksjon test_api_request_model_route() {
    la body = "{\"title\":\"demo\",\"count\":8,\"active\":true,\"tags\":[\"app\",\"request\"]}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/request-model", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"title\":\"demo\""))
    assert(tekst_inneholder(body_res, "\"count\":8"))
    assert(tekst_inneholder(body_res, "\"active\":true"))
    assert(tekst_inneholder(body_res, "\"tags\":[\"app\",\"request\"]"))
}

funksjon test_api_request_model_route_feil() {
    la body = "{\"title\":\"\",\"count\":\"na\",\"active\":\"ja\",\"tags\":\"ikkje-liste\"}"
    la ctx = web.request_context("POST", "/api/v1/${APP_NAME}/request-model", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"title er påkravd\""))
    assert(tekst_inneholder(body_res, "\"count må vere heiltal\""))
    assert(tekst_inneholder(body_res, "\"active må vere boolsk\""))
    assert(tekst_inneholder(body_res, "\"tags må vere liste\""))
}

funksjon test_api_item_route() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/items/7", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"id\":7"))
}

funksjon test_api_item_route_feil() {
    la ctx = web.request_context("GET", "/api/v1/${APP_NAME}/items/abc", {}, {"host": "localhost"}, "")
    la svar = dispatcher.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"id må vere heiltal\""))
}

funksjon start() -> heiltall {
    la testar = [
        "test_start",
        "test_routes_inneheld",
        "test_dispatch_index",
        "test_api_health",
        "test_api_echo_payload",
        "test_api_echo_route_feil",
        "test_api_openapi_route",
        "test_api_docs_route",
        "test_api_meta_route",
        "test_api_dependency_route",
        "test_api_auth_login_form",
        "test_api_auth_login_route",
        "test_api_auth_login_route_feil",
        "test_api_auth_register_form",
        "test_api_auth_register_route",
        "test_api_auth_register_route_feil",
        "test_api_auth_logout",
        "test_api_auth_profile",
        "test_api_auth_profile_ikkje_logga_inn",
        "test_api_auth_profile_ugyldig_token",
        "test_api_404_route",
        "test_api_405_route",
        "test_api_error_route_ok",
        "test_api_error_route_bad",
        "test_api_error_route_server",
        "test_api_query_route",
        "test_api_query_route_feil",
        "test_api_validate_route",
        "test_api_validate_route_feil",
        "test_api_payload_route",
        "test_api_payload_route_feil",
        "test_api_nested_route",
        "test_api_nested_route_feil",
        "test_api_headers_route",
        "test_api_headers_route_feil",
        "test_api_response_model_route",
        "test_api_response_model_route_feil",
        "test_api_request_model_route",
        "test_api_request_model_route_feil",
        "test_api_item_route",
        "test_api_item_route_feil"
    ]
    returner test.køyr(testar)
}
EOF_TEST

cat > "$APP_PATH/tests/payload_${APP_NAME}.json" <<EOF_PAY
{
  "action": "api_echo",
  "payload": {
    "app": "${APP_NAME}"
  }
}
EOF_PAY

cat > "$APP_PATH/tests/payloads/${APP_NAME}_echo.json" <<EOF_PAY2
{
  "melding": "Hei frå ${APP_NAME}"
}
EOF_PAY2

cat > "$APP_PATH/tests/payloads/${APP_NAME}_payload.json" <<EOF_PAY3
{
  "title": "Demo ${APP_NAME}",
  "count": 7,
  "active": true,
  "tags": ["api", "app", "${APP_NAME}"]
}
EOF_PAY3

cat > "$APP_PATH/tests/payloads/${APP_NAME}_nested.json" <<EOF_PAY4
{
  "user": {
    "name": "Ola",
    "age": 30
  },
  "meta": {
    "source": "${APP_NAME}",
    "ref": "v1"
  }
}
EOF_PAY4

cat > "$APP_PATH/tests/payloads/${APP_NAME}_request_model.json" <<EOF_PAY5
{
  "title": "Request demo ${APP_NAME}",
  "count": 4,
  "active": true,
  "tags": ["request", "${APP_NAME}"]
}
EOF_PAY5

cat > "$APP_PATH/templates/base.html" <<EOF_TPL
<!doctype html>
<html>
  <body>
    <h1>${APP_NAME}</h1>
    <main>{{innhald|html}}</main>
  </body>
</html>
EOF_TPL

cat > "$APP_PATH/templates/${APP_NAME}/index.html" <<EOF_APP_TPL
<!doctype html>
<html lang="no">
  <body>
    <h1>App: ${APP_NAME}</h1>
    <p>Dette er standardmal for appen {{app_namn}}.</p>
  </body>
</html>
EOF_APP_TPL

printf 'Oppretta app: %s\n' "$APP_PATH"
printf 'Testfil: %s/tests/test_%s.no\n' "$APP_PATH" "$APP_NAME"
printf 'Neste steg: importera appen frå hovudkoden\n'
