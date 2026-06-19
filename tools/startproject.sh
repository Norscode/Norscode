#!/usr/bin/env sh
set -eu

PROJECT_DIR=""
OUTPUT_DIR=""
PROJECT_NAME=""
DB_FILE="app.db"
WITH_AUTH="usann"
WITH_ADMIN="usann"
WITH_EMAIL="usann"
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"

usage() {
  printf 'bruk: startproject <målmappe> [--name <prosjektnamn>] [--path <sti>] [--db <db-fil>]\n' >&2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --name)
      shift
      [ "$#" -gt 0 ] || { usage; exit 2; }
      PROJECT_NAME="$1"
      ;;
    --path)
      shift
      [ "$#" -gt 0 ] || { usage; exit 2; }
      OUTPUT_DIR="$1"
      ;;
    --db)
      shift
      [ "$#" -gt 0 ] || { usage; exit 2; }
      DB_FILE="$1"
      ;;
    --with-auth)
      WITH_AUTH="sann"
      ;;
    --without-auth)
      WITH_AUTH="usann"
      ;;
    --with-admin)
      WITH_ADMIN="sann"
      ;;
    --without-admin)
      WITH_ADMIN="usann"
      ;;
    --help|-h)
      usage
      printf '  <målmappe>   Målkatalog for prosjektet\n'
      printf '  --name       Prosjektnamn i norcode.toml\n'
      printf '  --path       Alternativt utmatingsområde\n'
      printf '  --db         Databasesti (standard: app.db)\n'
      printf '  --with-auth / --without-auth   Slå auth-malen av/på\n'
      printf '  --with-admin / --without-admin Slå admin-malen av/på\n'
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

[ -n "$PROJECT_DIR" ] || { usage; exit 2; }

if [ -z "$PROJECT_NAME" ]; then
  PROJECT_NAME="$(basename "$PROJECT_DIR")"
fi

if [ -z "$OUTPUT_DIR" ]; then
  OUTPUT_DIR="$PROJECT_DIR"
fi

if [ -e "$OUTPUT_DIR" ] && [ -n "$(ls -A "$OUTPUT_DIR" 2>/dev/null)" ]; then
  printf 'feil: "%s" er ikkje tom\n' "$OUTPUT_DIR" >&2
  exit 2
fi

mkdir -p \
  "$OUTPUT_DIR/src/templates" \
  "$OUTPUT_DIR/src/templates/auth" \
  "$OUTPUT_DIR/src/templates/pages" \
  "$OUTPUT_DIR/src/templates/partials" \
  "$OUTPUT_DIR/src/auth" \
  "$OUTPUT_DIR/apps/core/tests" \
  "$OUTPUT_DIR/migrations" \
  "$OUTPUT_DIR/locale" \
  "$OUTPUT_DIR/static" \
  "$OUTPUT_DIR/static/media" \
  "$OUTPUT_DIR/tests/payloads" \
  "$OUTPUT_DIR/tests" \
  "$OUTPUT_DIR/examples" \
  "$OUTPUT_DIR/deploy" \
  "$OUTPUT_DIR/docs"

ln -sfn "$ROOT_DIR/std" "$OUTPUT_DIR/std"
ln -sfn "$ROOT_DIR/selfhost" "$OUTPUT_DIR/selfhost"
ln -sfn "$ROOT_DIR/runtime" "$OUTPUT_DIR/runtime"
ln -sfn "$ROOT_DIR/compiler" "$OUTPUT_DIR/compiler"

cat > "$OUTPUT_DIR/norcode.toml" <<EOF_TOML
[project]
name = "${PROJECT_NAME}"
version = "0.1.0"
entry = "app.no"
EOF_TOML

cat > "$OUTPUT_DIR/innstillingar.toml" <<EOF_CFG
[global]
PROFILE = "development"
DATABASE_URL = "${DB_FILE}"
SECURE_HEADERS = "sann"
REQUIRE_ALLOWED_HOSTS = "sann"
DEBUG = sann
SERVER_HOST = "0.0.0.0"
SERVER_PORT = 4173
ALLOWED_HOSTS = "localhost,127.0.0.1"
SECURITY_MIDDLEWARE = "sann"
REQUEST_LOG = "usann"
REQUEST_LOG_INCLUDE_BODY = "usann"
RATE_LIMIT_ENABLED = "usann"
RATE_LIMIT_MAX = "120"
RATE_LIMIT_KEY = "ip"
BRUTEFORCE_LOGIN_ENABLED = "sann"
BRUTEFORCE_LOGIN_MAX = "3"
BRUTEFORCE_LOGIN_WINDOW_SEK = "120"
BRUTEFORCE_LOGIN_KEY = "ip"
SESSION_TIMEOUT = 86400
LOG_LEVEL = "INFO"
CACHE_BACKEND = "minne"
CACHE_DIR = "cache/"
LOCALE_DIR = "locale/"
DEFAULT_LANGUAGE = "nn"
EMAIL_BACKEND = "konsoll"
EMAIL_HOST = "localhost"
EMAIL_PORT = 587
EMAIL_USER = ""
EMAIL_PASS = ""
EMAIL_FROM = "noreply@localhost"

[development]
DEBUG = sann
SESSION_TIMEOUT = 86400

[testing]
DEBUG = usann
SESSION_TIMEOUT = 900
DATABASE_URL = ":memory:"

[production]
DEBUG = usann
SESSION_TIMEOUT = 3600
EOF_CFG

cat > "$OUTPUT_DIR/app.no" <<'EOF_APP'
bruk std.web_app_stack som stack
bruk std.web som web
bruk src.config som config
bruk src.models som models
bruk src.views som views
bruk src.cache som cache_mod
bruk src.i18n som i18n
bruk src.routes som routes
bruk src.route_registry som route_registry
bruk src.static som static_mod
bruk src.security som security
bruk src.middleware som middleware
EOF_APP
if [ "$WITH_ADMIN" = "sann" ]; then
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP_ADMIN_IMPORT'
bruk src.admin_bootstrap som admin_bootstrap
EOF_APP_ADMIN_IMPORT
fi
if [ "$WITH_AUTH" = "sann" ]; then
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP_AUTH_IMPORT'
bruk src.auth.login som auth_login
bruk src.auth.register som auth_register
bruk src.auth.logout som auth_logout
bruk src.auth.guards som auth_guards
EOF_APP_AUTH_IMPORT
fi
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP'

funksjon _init_app() -> ordbok_tekst {
    la app = stack.bootstrap(config.INNSTILLINGAR_STI, config.DATABASE_STI)
    models.ensure_schema(app["conn"])
EOF_APP
if [ "$WITH_ADMIN" = "sann" ]; then
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP_ADMIN_BOOT'
    admin_bootstrap.registrer_modellar(app["register"], app["conn"])
EOF_APP_ADMIN_BOOT
fi
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP'
    returner app
}

funksjon dep_app_config(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = web.dependency("app_config")
    la _ = ctx
    returner config.hent()
}

funksjon dep_request_meta(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = web.dependency("request_meta")
    returner {
        "method": web.request_method(ctx),
        "path": web.request_path(ctx),
        "request_id": web.request_id(ctx)
    }
}

funksjon route_index(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /")
    la _ctx = ctx
    returner web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, "<!doctype html><html><body><h1>Norscode app OK</h1></body></html>")
}

funksjon route_cache_demo(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /cache-demo")
    la app = _init_app()
    la svar = views.cache_demo(ctx)
    la _ = stack.lukk(app)
    returner svar
}

funksjon route_i18n_demo(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /i18n-demo")
    la app = _init_app()
    la svar = views.i18n_demo(ctx)
    la _ = stack.lukk(app)
    returner svar
}

funksjon route_api_health(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/health")
    la svar = views.api_health(ctx)
    returner svar
}

funksjon route_api_echo(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/echo")
    la svar = views.api_echo(ctx)
    returner svar
}

funksjon route_api_openapi(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /openapi.json")
    la svar = views.api_openapi(ctx)
    returner svar
}

funksjon route_api_dependency(ctx: ordbok_tekst, app_config: ordbok_tekst, req_meta: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/dependency")
    web.use_dependency("app_config")
    web.use_dependency("request_meta")
    la app_namn: tekst = "Norscode"
    hvis har_nokkel(app_config, "name") { app_namn = app_config["name"] }
    la body = "{\"method\":\"" + req_meta["method"] + "\",\"path\":\"" + req_meta["path"] + "\",\"app_name\":\"" + app_namn + "\"}"
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, body)
}

funksjon route_api_docs(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /docs")
    la svar = views.api_docs(ctx)
    returner svar
}

funksjon route_api_query(ctx: ordbok_tekst) -> ordbok_tekst {
    la svar = views.api_query(ctx)
    returner svar
}

funksjon route_api_validate(ctx: ordbok_tekst) -> ordbok_tekst {
    la svar = views.api_validate(ctx)
    returner svar
}

funksjon route_api_item(ctx: ordbok_tekst) -> ordbok_tekst {
    la svar = views.api_item(ctx)
    returner svar
}

funksjon route_api_payload(ctx: ordbok_tekst) -> ordbok_tekst {
    la svar = views.api_payload(ctx)
    returner svar
}

funksjon route_api_nested(ctx: ordbok_tekst) -> ordbok_tekst {
    la svar = views.api_nested(ctx)
    returner svar
}

funksjon route_api_headers(ctx: ordbok_tekst) -> ordbok_tekst {
    la svar = views.api_headers(ctx)
    returner svar
}

funksjon route_api_response_model(ctx: ordbok_tekst) -> ordbok_tekst {
    la svar = views.api_response_model(ctx)
    returner svar
}

funksjon route_api_request_model(ctx: ordbok_tekst) -> ordbok_tekst {
    la svar = views.api_request_model(ctx)
    returner svar
}

funksjon route_static(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /static")
    la _ = ctx
    returner static_mod.svar(web.request_path(ctx))
}
EOF_APP
if [ "$WITH_ADMIN" = "sann" ]; then
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP_ADMIN_ROUTE'

funksjon route_admin_overview(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /admin")
    la app = _init_app()
    la ap_ctx = stack.app_ctx(app, ctx)
    hvis ikkje auth_guards.krev_rolle(ap_ctx, app["conn"], "admin") {
        la _ = stack.lukk(app)
        returner web.response_builder(403, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"ikkje tilgjengeleg: krev admin\"}")
    }
    la svar = admin_bootstrap.oversikt(app["conn"], app["register"], ap_ctx)
    la _ = stack.lukk(app)
    returner svar
}

funksjon route_admin_liste(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /admin/{tabell}")
    la app = _init_app()
    la ap_ctx = stack.app_ctx(app, ctx)
    hvis ikkje auth_guards.krev_rolle(ap_ctx, app["conn"], "admin") {
        la _ = stack.lukk(app)
        returner web.response_builder(403, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"ikkje tilgjengeleg: krev admin\"}")
    }
    la svar = admin_bootstrap.liste(app["conn"], app["register"], ap_ctx)
    la _ = stack.lukk(app)
    returner svar
}

funksjon route_admin_ny(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /admin/{tabell}/ny")
    la app = _init_app()
    la ap_ctx = stack.app_ctx(app, ctx)
    hvis ikkje auth_guards.krev_rolle(ap_ctx, app["conn"], "admin") {
        la _ = stack.lukk(app)
        returner web.response_builder(403, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"ikkje tilgjengeleg: krev admin\"}")
    }
    la svar = admin_bootstrap.ny(app["conn"], app["register"], ap_ctx)
    la _ = stack.lukk(app)
    returner svar
}

funksjon route_admin_rediger(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /admin/{tabell}/{id}/rediger")
    la app = _init_app()
    la ap_ctx = stack.app_ctx(app, ctx)
    hvis ikkje auth_guards.krev_rolle(ap_ctx, app["conn"], "admin") {
        la _ = stack.lukk(app)
        returner web.response_builder(403, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"ikkje tilgjengeleg: krev admin\"}")
    }
    la svar = admin_bootstrap.rediger(app["conn"], app["register"], ap_ctx)
    la _ = stack.lukk(app)
    returner svar
}

funksjon route_admin_lagre(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /admin/{tabell}/{id}/lagre")
    web.route("POST /admin/{tabell}/lagre")
    la app = _init_app()
    la ap_ctx = stack.app_ctx(app, ctx)
    hvis ikkje auth_guards.krev_rolle(ap_ctx, app["conn"], "admin") {
        la _ = stack.lukk(app)
        returner web.response_builder(403, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"ikkje tilgjengeleg: krev admin\"}")
    }
    la svar = admin_bootstrap.lagre(app["conn"], app["register"], ap_ctx)
    la _ = stack.lukk(app)
    returner svar
}

funksjon route_admin_slett(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /admin/{tabell}/{id}/slett")
    la app = _init_app()
    la ap_ctx = stack.app_ctx(app, ctx)
    hvis ikkje auth_guards.krev_rolle(ap_ctx, app["conn"], "admin") {
        la _ = stack.lukk(app)
        returner web.response_builder(403, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"ikkje tilgjengeleg: krev admin\"}")
    }
    la svar = admin_bootstrap.slett(app["conn"], app["register"], ap_ctx)
    la _ = stack.lukk(app)
    returner svar
}
EOF_APP_ADMIN_ROUTE
else
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP_ADMIN_ROUTE_DISABLED'

funksjon route_admin_overview(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = ctx
    returner web.response_builder(404, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"admin er deaktivert\"}")
}
EOF_APP_ADMIN_ROUTE_DISABLED
fi
if [ "$WITH_AUTH" = "sann" ]; then
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP_AUTH_ROUTE'

funksjon route_login(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /auth/login")
    la app = _init_app()
    la svar = auth_login.utfør(stack.app_ctx(app, ctx), app["conn"])
    la _ = stack.lukk(app)
    returner svar
}
funksjon route_login_skjemat(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /auth/login")
    la app = _init_app()
    la ap_ctx = stack.app_ctx(app, ctx)
    la svar = auth_login.skjema(ap_ctx, app["conn"])
    la _ = stack.lukk(app)
    returner svar
}
funksjon route_register(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /auth/register")
    la app = _init_app()
    la svar = auth_register.utfør(stack.app_ctx(app, ctx), app["conn"])
    la _ = stack.lukk(app)
    returner svar
}
funksjon route_register_skjemat(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /auth/register")
    la app = _init_app()
    la ap_ctx = stack.app_ctx(app, ctx)
    la svar = auth_register.skjema(ap_ctx, app["conn"])
    la _ = stack.lukk(app)
    returner svar
}
funksjon route_logout(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /auth/logout")
    la app = _init_app()
    la svar = auth_logout.utfør(stack.app_ctx(app, ctx), app["conn"])
    la _ = stack.lukk(app)
    returner svar
}
funksjon route_profile(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /profile")
    la app = _init_app()
    la ap_ctx = stack.app_ctx(app, ctx)
    hvis ikkje auth_guards.krev_innlogging(ap_ctx, app["conn"]) {
        la _ = stack.lukk(app)
        returner web.response_builder(401, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"logg inn på nytt\"}")
    }
    la brukar = ap_ctx["__auth_brukar__"]
    la _ = stack.lukk(app)
    returner stack.ok_json({"ok": "sann", "brukar": brukar})
}
EOF_APP_AUTH_ROUTE
else
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP_AUTH_ROUTE_DISABLED'

funksjon route_login(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /auth/login")
    returner web.response_builder(404, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"auth er deaktivert\"}")
}

funksjon route_login_skjemat(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /auth/login")
    returner web.response_builder(404, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"auth er deaktivert\"}")
}

funksjon route_register(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /auth/register")
    returner web.response_builder(404, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"auth er deaktivert\"}")
}

funksjon route_register_skjemat(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /auth/register")
    returner web.response_builder(404, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"auth er deaktivert\"}")
}

funksjon route_logout(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /auth/logout")
    returner web.response_builder(404, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"auth er deaktivert\"}")
}

funksjon route_profile(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /profile")
    returner web.response_builder(404, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"auth er deaktivert\"}")
}
EOF_APP_AUTH_ROUTE_DISABLED
fi
cat >> "$OUTPUT_DIR/app.no" <<'EOF_APP'

funksjon _svar_med_security(ctx: ordbok_tekst, svar: ordbok_tekst) -> ordbok_tekst {
    returner middleware.køyr(ctx, svar)
}

funksjon dispatcher(ctx: ordbok_tekst) -> ordbok_tekst {
    la svar = web.handle_request(ctx)
    returner _svar_med_security(ctx, svar)
}

funksjon start() -> heiltall {
    la ctx = web.request_context("GET", "/", {}, {}, "")
    la app = _init_app()
    la res = dispatcher(stack.app_ctx(app, ctx))
    la _ = stack.lukk(app)
    skriv(web.response_status(res))
    skriv("\\n")
    returner 0
}

funksjon main() -> heiltall {
    skriv("Norscode app OK")
    skriv("\\n")
    returner 0
}
EOF_APP

cat > "$OUTPUT_DIR/manage.no" <<'EOF_MANAGE'
bruk std.cli som cli
bruk std.web_app_stack som stack
bruk std.pathlib som pl
bruk std.fil som fil
bruk std.orm som orm
bruk std.test som test
bruk src.config som config
bruk src.models som models

funksjon _migrasjon_state_fil() -> tekst {
    returner "migrations/schema_state.json"
}

funksjon _auto_migrasjon_fil() -> tekst {
    returner "migrations/migrations.json"
}

funksjon _null_til_4(n: heiltall) -> tekst {
    hvis n < 10 { returner "000" + tekst_fra_heltall(n) }
    hvis n < 100 { returner "00" + tekst_fra_heltall(n) }
    hvis n < 1000 { returner "0" + tekst_fra_heltall(n) }
    returner tekst_fra_heltall(n)
}

funksjon _hent_tilfeldig_data(sti: tekst) -> ordbok_tekst {
    hvis ikkje fil.finnes(sti) { returner {} }
    la rå = fil.les_eller_tom(sti)
    hvis rå == "" { returner {} }
    prøv {
        la parsed = builtin.json_parse_raw(rå)
        hvis builtin.type(parsed) == "ordbok" { returner parsed }
        hvis builtin.type(parsed) == "liste" {
            la wrapper: ordbok_tekst = {}
            wrapper["items"] = parsed
            returner wrapper
        }
    } fang (e) {
        returner {}
    }
    returner {}
}

funksjon _tabell_felt(snapshot: liste<ordbok_tekst>, tabell: tekst) -> liste<tekst> {
    la i: heiltall = 0
    mens i < lengde(snapshot) {
        la e = snapshot[i]
        hvis e["table"] == tabell {
            hvis har_nokkel(e, "fields") og builtin.type(e["fields"]) == "liste" {
                returner e["fields"]
            }
            returner []
        }
        i = i + 1
    }
    returner []
}

funksjon _felt_finnes(liste_felter: liste<tekst>, felt: tekst) -> boolsk {
    la i: heiltall = 0
    mens i < lengde(liste_felter) {
        hvis liste_felter[i] == felt { returner sann }
        i = i + 1
    }
    returner usann
}

funksjon _siste_auto_namn() -> tekst {
    la state = _hent_tilfeldig_data(_auto_migrasjon_fil())
    hvis ikkje har_nokkel(state, "items") {
        returner "0000"
    }

    la migrasjonar = state["items"]
    hvis builtin.type(migrasjonar) != "liste" {
        returner "0000"
    }

    la hoygast = 0
    la i: heiltall = 0
    mens i < lengde(migrasjonar) {
        la m = migrasjonar[i]
        hvis builtin.type(m) == "ordbok" {
            la namn = m["name"]
            la del = builtin.split(namn, "_")
            hvis lengde(del) > 0 {
                la pref = del[0]
                hvis pref != "" {
                    prøv {
                        la tal = builtin.heltall_fra_tekst(pref)
                        hvis tal > hoygast { hoygast = tal }
                    } fang (e) {
                        // ignorerar ugyldige prefiksar
                    }
                }
            }
        }
        i = i + 1
    }
    returner _null_til_4(hoygast)
}

funksjon _les_snapshot() -> liste<ordbok_tekst> {
    la raw = _hent_tilfeldig_data(_migrasjon_state_fil())
    hvis har_nokkel(raw, "models") og builtin.type(raw["models"]) == "liste" {
        returner raw["models"]
    }
    returner []
}

funksjon _skriv_snapshot(snapshot: liste<ordbok_tekst>) {
    fil.skriv_fil(_migrasjon_state_fil(), builtin.json_stringify({"models": snapshot}))
}

funksjon _hent_auto_migrasjonar() -> liste<ordbok_tekst> {
    la raw = _hent_tilfeldig_data(_auto_migrasjon_fil())
    hvis har_nokkel(raw, "items") og builtin.type(raw["items"]) == "liste" {
        returner raw["items"]
    }
    returner []
}

funksjon _lagre_auto_migrasjonar(migrasjonar: liste<ordbok_tekst>) {
    fil.skriv_fil(_auto_migrasjon_fil(), builtin.json_stringify({"items": migrasjonar}))
}

funksjon _bygg_migrasjonstekst(forrige: liste<ordbok_tekst>, no: liste<ordbok_tekst>) -> ordbok_tekst {
    la up: liste<tekst> = []
    la down: liste<tekst> = []

    la i: heiltall = 0
    mens i < lengde(no) {
        la e = no[i]
        la tabell = e["table"]
        la felt = e["fields"]
        hvis builtin.type(felt) != "liste" { felt = [] }
        la tidlegare = _tabell_felt(forrige, tabell)
        hvis lengde(tidlegare) == 0 {
            legg_til(up, "CREATE TABLE " + tabell + " (" + tekst_saman(felt, ",") + ");")
            legg_til(down, "DROP TABLE IF EXISTS " + tabell + ";")
        } ellers {
            la j: heiltall = 0
            mens j < lengde(felt) {
                la felt_def = felt[j]
                hvis ikkje _felt_finnes(tidlegare, felt_def) {
                    legg_til(up, "ALTER TABLE " + tabell + " ADD COLUMN " + felt_def + ";")
                    legg_til(down, "-- automatisk migrasjon manglar reversering: " + felt_def + " i " + tabell)
                }
                j = j + 1
            }
        }
        i = i + 1
    }

    la k: heiltall = 0
    mens k < lengde(forrige) {
        la e_prev = forrige[k]
        la tabell = e_prev["table"]
        hvis tabell == "" { k = k + 1; fortsett }
        la tabell_felt = e_prev["fields"]
        hvis builtin.type(tabell_felt) != "liste" { tabell_felt = [] }
        la nye_felt = _tabell_felt(no, tabell)
        hvis lengde(nye_felt) == 0 {
            legg_til(up, "DROP TABLE IF EXISTS " + tabell + ";")
            legg_til(down, "CREATE TABLE " + tabell + " (" + tekst_saman(tabell_felt, ",") + ");")
        }
        k = k + 1
    }

    hvis lengde(up) == 0 {
        la svar: ordbok_tekst = {}
        svar["up"] = ""
        svar["down"] = ""
        returner svar
    }

    la svar: ordbok_tekst = {}
    svar["up"] = tekst_saman(up, "\n")
    svar["down"] = tekst_saman(down, "\n")
    returner svar
}

funksjon _normaliser_tekst(sti: tekst) -> tekst {
    hvis lengde(sti) >= 3 {
        hvis builtin.slice(sti, lengde(sti) - 3, lengde(sti)) == ".no" {
            returner builtin.slice(sti, 0, lengde(sti) - 3)
        }
    }
    returner sti
}

funksjon _sti_til_modul(sti_full: tekst, base_sti: tekst) -> tekst {
    la s = sti_full
    hvis tekst_starter_med(s, base_sti) {
        s = builtin.slice(s, lengde(base_sti), lengde(s))
        hvis lengde(s) > 0 og builtin.slice(s, 0, 1) == "/" {
            s = builtin.slice(s, 1, lengde(s))
        }
    }

    s = _normaliser_tekst(s)

    la deler = builtin.split(s, "/")
    la modul: tekst = ""
    la i: heiltall = 0
    mens i < lengde(deler) {
        la bit = deler[i]
        hvis bit != "" og bit != "." {
            hvis modul == "" { modul = bit } ellers { modul = modul + "." + bit }
        }
        i = i + 1
    }
    returner modul
}

funksjon _legg_til_unik(modular: liste<tekst>, namn: tekst) {
    la i: heiltall = 0
    mens i < lengde(modular) {
        hvis modular[i] == namn { returner }
        i = i + 1
    }
    legg_til(modular, namn)
}

funksjon _finn_tester() -> liste<tekst> {
    la base = pl.sti_tekst(pl.gjeldande_katalog())
    la modular: liste<tekst> = []
    la test_dir = pl.ny(base + "/tests")
    hvis pl.er_katalog(test_dir) {
        la rå_t = pl.rglob(test_dir, "test_*.no")
        la i: heiltall = 0
        mens i < lengde(rå_t) {
            la p = builtin.json_parse(rå_t[i])
            la namn = _sti_til_modul(p["sti"], base)
            _legg_til_unik(modular, namn)
            i = i + 1
        }
    }

    la apps_dir = pl.ny(base + "/apps")
    hvis pl.er_katalog(apps_dir) {
        la app_kat = pl.iterer_katalog(apps_dir)
        la ai: heiltall = 0
        mens ai < lengde(app_kat) {
            la app_p = builtin.json_parse(app_kat[ai])
            hvis pl.er_katalog(pl.ny(app_p["sti"])) {
                la test_kat = pl.koble(pl.ny(app_p["sti"]), "tests")
                hvis pl.er_katalog(test_kat) {
                    la raw = pl.rglob(test_kat, "test_*.no")
                    la i: heiltall = 0
                    mens i < lengde(raw) {
                        la p = builtin.json_parse(raw[i])
                        la namn = _sti_til_modul(p["sti"], base)
                        _legg_til_unik(modular, namn)
                        i = i + 1
                    }
                }
            }
            ai = ai + 1
        }
    }

    returner modular
}

funksjon _køyr_test_modul(modul: tekst) -> ordbok_tekst {
    la resultat: ordbok_tekst = {}
    resultat["namn"] = modul
    prøv {
        la _ = builtin.ncb_call_fn(modul + ".start", {})
        resultat["bestått"] = "sann"
        resultat["feil"] = ""
        skriv("  ✓ " + modul)
    } fang (e) {
        resultat["bestått"] = "usann"
        resultat["feil"] = tekst(e)
        skriv("  ✗ " + modul)
        skriv("    " + tekst(e))
    }
    returner resultat
}

funksjon _fixture_element_for_lagring(item: ordbok_tekst) -> ordbok_tekst {
    la row: ordbok_tekst = {}
    la modell: tekst = ""
    hvis har_nokkel(item, "model") { modell = tekst(item["model"]) }
    ellers hvis har_nokkel(item, "table") { modell = tekst(item["table"]) }
    hvis modell == "" { returner {} }

    hvis har_nokkel(item, "fields") {
        la fields_raw = item["fields"]
        row = {}
        la keys = nøkler(fields_raw)
        la fi: heiltall = 0
        mens fi < lengde(keys) {
            la key = keys[fi]
            row[key] = tekst(fields_raw[key])
            fi = fi + 1
        }
    } ellers {
        row = {}
        la keys = nøkler(item)
        la i: heiltall = 0
        mens i < lengde(keys) {
            la key = keys[i]
            hvis key != "model" og key != "table" og key != "pk" {
                row[key] = tekst(item[key])
            }
            i = i + 1
        }
    }

    returner {"model": modell, "rad": row}
}

funksjon _last_fixture_fil(conn: tekst, sti_fil: tekst) -> heiltall {
    prøv {
        la innhald = pl.les_tekst(pl.ny(sti_fil))
        la parsed = builtin.json_parse_raw(innhald)
        la element: liste<ordbok_tekst> = []
        hvis builtin.type(parsed) == "liste" { element = parsed } ellers hvis builtin.type(parsed) == "ordbok" { legg_til(element, parsed) }

        la i: heiltall = 0
        mens i < lengde(element) {
            la norm = _fixture_element_for_lagring(element[i])
            hvis har_nokkel(norm, "model") {
                la modell = tekst(norm["model"])
                la rad = norm["rad"]
                la _ = orm.lagre(conn, modell, rad)
            }
            i = i + 1
        }
        returner 0
    } fang (e) {
        skriv("Feil ved lasting av fixture " + sti_fil + ": " + tekst(e))
        returner 1
    }
    returner 0
}

funksjon makemigrations(args: ordbok_tekst) -> heiltall {
    la _ = args
    la now = models.modeller()
    la before = _les_snapshot()

    hvis lengde(before) == 0 {
        _skriv_snapshot(now)
        skriv("Ingen autogenererte migrasjonar enda. Startsnapshot lagra.")
        returner 0
    }

    la diff = _bygg_migrasjonstekst(before, now)
    la up = diff["up"]
    hvis up == "" {
        skriv("Ingen modellendringar. Ingenting å gjere.")
        returner 0
    }

    la siste = _null_til_4(heltall_fra_tekst(_siste_auto_namn()) + 1)
    la auto_namn = siste + "_auto"

    la auto = _hent_auto_migrasjonar()
    la ny: ordbok_tekst = {}
    ny["name"] = auto_namn
    ny["opp_sql"] = up
    ny["ned_sql"] = diff["down"]
    legg_til(auto, ny)

    _lagre_auto_migrasjonar(auto)
    _skriv_snapshot(now)
    skriv("Laga migrasjon: " + auto_namn)
    returner 0
}

funksjon test(args: ordbok_tekst) -> heiltall {
    la db_sti = cli.flagg(args, "db", test.test_db())
    la fixtures_arg = cli.flagg(args, "fixtures", "")
    la app = stack.bootstrap(config.INNSTILLINGAR_STI, db_sti)

    la migrasjon_status = stack.køyr_migrasjonar(app["conn"], models.migrasjonar())
    hvis migrasjon_status < 0 {
        la _ = stack.lukk(app)
        returner 1
    }

    hvis fixtures_arg != "" {
        la base = pl.sti_tekst(pl.gjeldande_katalog()) + "/tests/payloads"
        la payload_dir = pl.ny(base)
        hvis tekst_starter_med(fixtures_arg, "all") {
            hvis pl.er_katalog(payload_dir) {
                la filer = pl.rglob(payload_dir, "*.json")
                la i: heiltall = 0
                mens i < lengde(filer) {
                    la f = builtin.json_parse(filer[i])
                    la kod = _last_fixture_fil(app["conn"], f["sti"])
                    hvis kod > 0 { 
                        la _ = stack.lukk(app)
                        returner 1
                    }
                    i = i + 1
                }
            }
        } ellers {
            la liste = builtin.split(fixtures_arg, ",")
            la i: heiltall = 0
            mens i < lengde(liste) {
                la namn = builtin.trim(liste[i])
                hvis namn == "" { i = i + 1; fortsett }

                la sti = base + "/" + namn
                hvis tekst_starter_med(namn, "/") { sti = namn }
                hvis builtin.index_of(sti, ".json") == -1 { sti = sti + ".json" }
                la kod = _last_fixture_fil(app["conn"], sti)
                hvis kod > 0 {
                    la _ = stack.lukk(app)
                    returner 1
                }
                i = i + 1
            }
        }
    }

    la modular = _finn_tester()
    la total = lengde(modular)
    la bestått: heiltall = 0
    la feila: heiltall = 0

    skriv("\n=== Norscode testkøyrar ===")
    skriv("Køyrer " + tekst_fra_heltall(total) + " testmodularer...")

    la i: heiltall = 0
    mens i < total {
        la r = _køyr_test_modul(modular[i])
        hvis r["bestått"] == "sann" { bestått = bestått + 1 } ellers { feila = feila + 1 }
        i = i + 1
    }

    la _ = stack.lukk(app)
    skriv("")
    skriv("Bestått: " + tekst_fra_heltall(bestått) + "/" + tekst_fra_heltall(total))
    hvis feila > 0 {
        skriv("Feila:   " + tekst_fra_heltall(feila))
        returner 1
    }
    skriv("Alle testar bestått!")
    returner 0
}

funksjon migrate(args: ordbok_tekst) -> heiltall {
    la db_sti = cli.flagg(args, "db", config.DATABASE_STI)
    la app = stack.bootstrap(config.INNSTILLINGAR_STI, db_sti)
    la r = stack.køyr_migrasjonar(app["conn"], models.migrasjonar())
    la _ = stack.lukk(app)
    returner r
}

funksjon status(args: ordbok_tekst) -> heiltall {
    la db_sti = cli.flagg(args, "db", config.DATABASE_STI)
    la app = stack.bootstrap(config.INNSTILLINGAR_STI, db_sti)
    la r = stack.status_migrasjonar(app["conn"], models.migrasjonar())
    la _ = stack.lukk(app)
    returner r
}

funksjon main() -> heiltall {
    la app = cli.ny("manage")
    cli.kommando_med_flagg(app, "makemigrations", "Opprett autogenerert migrasjon frå modelldefinisjon", "makemigrations", [])
    cli.kommando_med_flagg(app, "migrate", "Køyr migrasjonar", "migrate", ["db"])
    cli.kommando_med_flagg(app, "status", "Vis migrasjonsstatus", "status", ["db"])
    cli.kommando_med_flagg(app, "test", "Køyr testpakke med testdatabase og fixture-lasting", "test", ["db", "fixtures"])
    la _ = cli.køyr(app)
    returner 0
}

main()
EOF_MANAGE

cat > "$OUTPUT_DIR/src/config.no" <<'EOF_CONFIG'
bruk std.innstillingar som cfg

funksjon _normaliser_profil(p: tekst) -> tekst {
    la namn = tekst_til_store(p)
    hvis namn == "" {
        returner "global"
    }
    hvis namn == "DEV" {
        returner "development"
    }
    hvis namn == "DEVELOP"
      eller namn == "DEVELOPMENT" {
        returner "development"
    }
    hvis namn == "TEST" {
        returner "testing"
    }
    hvis namn == "TESTING" {
        returner "testing"
    }
    hvis namn == "PROD" {
        returner "production"
    }
    hvis namn == "PRODUCTION" {
        returner "production"
    }
    returner "global"
}

funksjon _merge(overordnet: ordbok_tekst, underordna: ordbok_tekst) -> ordbok_tekst {
    la nøklar = nøkler(underordna)
    la i: heiltall = 0
    mens i < lengde(nøklar) {
        la nøkkel = nøklar[i]
        overordnet[nøkkel] = underordna[nøkkel]
        i = i + 1
    }
    returner overordnet
}

funksjon _finn_global_toppnivå(base: ordbok_tekst) -> ordbok_tekst {
    la resultat: ordbok_tekst = {}
    la nøklar = nøkler(base)
    la i: heiltall = 0
    mens i < lengde(nøklar) {
        la nøkkel = nøklar[i]
        hvis tekst_inneholder(nøkkel, "_") {
            i = i + 1
            fortsett
        }
        hvis nøkkel != "__kjelde__" {
            resultat[nøkkel] = base[nøkkel]
        }
        i = i + 1
    }
    returner resultat
}

funksjon profil() -> tekst {
    la raw = cfg.last(INNSTILLINGAR_STI)
    la val = cfg.hent(raw, "PROFILE", "global")

    la miljø = miljo_hent("NORSCODE_PROFILE")
    hvis miljø != "" { val = miljø }
    miljø = miljo_hent("NORSCODE_APP_PROFILE")
    hvis miljø != "" { val = miljø }

    returner _normaliser_profil(val)
}

funksjon hent_for_profil(profil_namn: tekst) -> ordbok_tekst {
    la base = cfg.last(INNSTILLINGAR_STI)
    la samlet: ordbok_tekst = _finn_global_toppnivå(base)
    samlet = _merge(samlet, cfg.seksjon(base, "global"))
    la navn = _normaliser_profil(profil_namn)
    hvis navn != "global" {
        samlet = _merge(samlet, cfg.seksjon(base, navn))
    }
    returner samlet
}

la INNSTILLINGAR_STI = "innstillingar.toml"
la APP_CFG = hent_for_profil(profil())
la PROFIL = profil()
la DATABASE_STI = APP_CFG["DATABASE_URL"]

funksjon hent() -> ordbok_tekst {
    returner APP_CFG
}
EOF_CONFIG

cat > "$OUTPUT_DIR/src/security.no" <<'EOF_SECURITY'
bruk std.web som web
bruk std.mw som mw
bruk src.config som config

funksjon _host_ein() -> liste<tekst> {
    la cfg = config.hent()
    hvis ikkje har_nokkel(cfg, "ALLOWED_HOSTS") { returner [] }
    returner builtin.split(cfg["ALLOWED_HOSTS"], ",")
}

funksjon _trim_host(host: tekst) -> tekst {
    la i: heiltall = builtin.index_of(host, ":")
    hvis i > 0 { returner builtin.trim(builtin.slice(host, 0, i)) }
    returner builtin.trim(host)
}

funksjon _require_host_venleg(cfg: ordbok_tekst) -> boolsk {
    hvis ikkje har_nokkel(cfg, "REQUIRE_ALLOWED_HOSTS") { returner sann }
    la r = tekst_til_liten(cfg["REQUIRE_ALLOWED_HOSTS"])
    hvis r == "usann" eller r == "false" eller r == "0" eller r == "nei" { returner usann }
    returner sann
}

funksjon _security_header_on(cfg: ordbok_tekst) -> boolsk {
    hvis ikkje har_nokkel(cfg, "SECURE_HEADERS") { returner sann }
    la v = tekst_til_liten(cfg["SECURE_HEADERS"])
    returner v != "usann" og v != "false" og v != "0" og v != "nei"
}

funksjon host_tillatt(ctx: ordbok_tekst) -> boolsk {
    la cfg = config.hent()
    hvis ikkje _require_host_venleg(cfg) { returner sann }
    la host = web.request_header(ctx, "host")
    hvis host == "" { returner sann }
    la tillat = _host_ein()
    hvis lengde(tillat) == 0 { returner sann }
    la norm = _trim_host(host)
    la i: heiltall = 0
    mens i < lengde(tillat) {
        hvis builtin.trim(tillat[i]) == norm { returner sann }
        i = i + 1
    }
    returner usann
}

funksjon ikkke_tillatt_host(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = ctx
    returner web.response_builder(403, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":false,\"feil\":\"Host er ikkje tillate\"}")
}

funksjon _ikkje_tillatt_host(ctx: ordbok_tekst) -> ordbok_tekst {
    returner ikkke_tillatt_host(ctx)
}

funksjon sikre(ctx: ordbok_tekst, svar: ordbok_tekst) -> ordbok_tekst {
    hvis ikkje host_tillatt(ctx) { returner ikkke_tillatt_host(ctx) }
    la cfg = config.hent()
    hvis ikkje _security_header_on(cfg) { returner svar }
    returner mw.legg_til_sikkerhets_headers(svar, mw.sikkerhets_standard())
}
EOF_SECURITY

cat > "$OUTPUT_DIR/src/middleware.no" <<'EOF_MW'
bruk std.web som web
bruk std.mw som mw
bruk src.config som config
bruk src.security som security

funksjon _er_sann(cfg: ordbok_tekst, nøkkel: tekst, fallback: tekst) -> boolsk {
    hvis ikkje har_nokkel(cfg, nøkkel) { returner tekst_til_liten(fallback) == "sann" }
    la verdi = tekst_til_liten(cfg[nøkkel])
    returner verdi != "usann" og verdi != "false" og verdi != "0" og verdi != "nei"
}

funksjon bygg_stack() -> liste<ordbok_tekst> {
    la cfg = config.hent()
    la stack: liste<ordbok_tekst> = []

    hvis _er_sann(cfg, "REQUEST_LOG", "usann") {
        la nivå = "INFO"
        hvis har_nokkel(cfg, "LOG_LEVEL") {
            nivå = cfg["LOG_LEVEL"]
        }
        la inkluder_body = _er_sann(cfg, "REQUEST_LOG_INCLUDE_BODY", "usann")
        legg_til(stack, mw.logging_cfg(nivå, inkluder_body))
    }

    hvis _er_sann(cfg, "RATE_LIMIT_ENABLED", "usann") {
        la maks = "120"
        hvis har_nokkel(cfg, "RATE_LIMIT_MAX") { maks = cfg["RATE_LIMIT_MAX"] }
        la nøkkel = "ip"
        hvis har_nokkel(cfg, "RATE_LIMIT_KEY") {
            nøkkel = cfg["RATE_LIMIT_KEY"]
        }
        legg_til(stack, mw.rate_cfg(heltall_fra_tekst(maks), nøkkel))
    }

    hvis _er_sann(cfg, "SECURITY_MIDDLEWARE", "sann") {
        la sk = mw.sikkerhets_standard()
        sk["type"] = "sikkerheit"
        legg_til(stack, sk)
    }

    returner stack
}

funksjon køyr(ctx: ordbok_tekst, svar: ordbok_tekst) -> ordbok_tekst {
    hvis ikkje security.host_tillatt(ctx) {
        returner security.ikkke_tillatt_host(ctx)
    }
    la stack = bygg_stack()
    la ctx_med_mw = mw.pipeline_request(ctx, stack)

    hvis mw.er_rate_limited(ctx_med_mw) {
        la rate_svar = mw.rate_limit_response()
        returner mw.pipeline_response(rate_svar, ctx_med_mw, stack)
    }

    la svar_med_tryggleikar = security.sikre(ctx_med_mw, svar)
    returner mw.pipeline_response(svar_med_tryggleikar, ctx_med_mw, stack)
}
EOF_MW

cat > "$OUTPUT_DIR/src/models.no" <<'EOF_MODELS'
bruk std.fil som fil
bruk std.migrasjon som migrasjon
bruk std.orm som orm

funksjon _automigrasjonar_fil() -> tekst {
    returner "migrations/migrations.json"
}

funksjon schema_kategori() -> liste<tekst> {
    returner [
        "id INTEGER PRIMARY KEY AUTOINCREMENT",
        "namn TEXT NOT NULL",
        "oppretta TEXT DEFAULT (datetime('now'))"
    ]
}

funksjon schema_tag() -> liste<tekst> {
    returner [
        "id INTEGER PRIMARY KEY AUTOINCREMENT",
        "namn TEXT NOT NULL",
        "oppretta TEXT DEFAULT (datetime('now'))"
    ]
}

funksjon schema_produkt() -> liste<tekst> {
    returner [
        "id INTEGER PRIMARY KEY AUTOINCREMENT",
        "namn TEXT NOT NULL",
        "pris INTEGER DEFAULT 0",
        "lager INTEGER DEFAULT 0",
        "kategori_id INTEGER",
        "oppretta TEXT DEFAULT (datetime('now'))",
        "FOREIGN KEY(kategori_id) REFERENCES kategori(id)"
    ]
}

funksjon schema_produkt_tag() -> liste<tekst> {
    returner [
        "id INTEGER PRIMARY KEY AUTOINCREMENT",
        "produkt_id INTEGER NOT NULL",
        "tag_id INTEGER NOT NULL",
        "FOREIGN KEY(produkt_id) REFERENCES produkt(id)",
        "FOREIGN KEY(tag_id) REFERENCES tag(id)",
        "UNIQUE(produkt_id, tag_id)"
    ]
}

funksjon relasjonar_many_to_many() -> liste<ordbok_tekst> {
    la m2m: liste<ordbok_tekst> = []
    la l: ordbok_tekst = {}
    l["table"] = "produkt_tag"
    l["felt"] = "produkt_id INTEGER, tag_id INTEGER"
    l["left_alias"] = "t"
    l["right_alias"] = "p"
    legg_til(m2m, l)
    returner m2m
}

funksjon modeller() -> liste<ordbok_tekst> {
    la modellar: liste<ordbok_tekst> = []
    la kategori: ordbok_tekst = {}
    kategori["table"] = "kategori"
    kategori["fields"] = schema_kategori()
    legg_til(modellar, kategori)
    la tag: ordbok_tekst = {}
    tag["table"] = "tag"
    tag["fields"] = schema_tag()
    legg_til(modellar, tag)
    la produkt: ordbok_tekst = {}
    produkt["table"] = "produkt"
    produkt["fields"] = schema_produkt()
    legg_til(modellar, produkt)
    la produkt_tag: ordbok_tekst = {}
    produkt_tag["table"] = "produkt_tag"
    produkt_tag["fields"] = schema_produkt_tag()
    legg_til(modellar, produkt_tag)
    returner modellar
}

funksjon ensure_schema(conn: tekst) -> ordbok_tekst {
    la modellar = modeller()
    la i: heiltall = 0
    mens i < lengde(modellar) {
        la m = modellar[i]
        orm.migrer(conn, m["table"], m["fields"])
        i = i + 1
    }
    returner {}
}

funksjon _initial_migrasjonar() -> liste<ordbok_tekst> {
    la opp = []
    legg_til(opp, migrasjon.lag(
        "0001_initial",
        "CREATE TABLE IF NOT EXISTS kategori (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT NOT NULL, oppretta TEXT DEFAULT (datetime('now')));",
        "DROP TABLE IF EXISTS kategori;"
    ))
    legg_til(opp, migrasjon.lag(
        "0001_initial_2",
        "CREATE TABLE IF NOT EXISTS tag (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT NOT NULL, oppretta TEXT DEFAULT (datetime('now')));",
        "DROP TABLE IF EXISTS tag;"
    ))
    legg_til(opp, migrasjon.lag(
        "0001_initial_3",
        "CREATE TABLE IF NOT EXISTS produkt (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT NOT NULL, pris INTEGER DEFAULT 0, lager INTEGER DEFAULT 0, kategori_id INTEGER, oppretta TEXT DEFAULT (datetime('now')), FOREIGN KEY(kategori_id) REFERENCES kategori(id));",
        "DROP TABLE IF EXISTS produkt;"
    ))
    legg_til(opp, migrasjon.lag(
        "0001_initial_4",
        "CREATE TABLE IF NOT EXISTS produkt_tag (id INTEGER PRIMARY KEY AUTOINCREMENT, produkt_id INTEGER NOT NULL, tag_id INTEGER NOT NULL, FOREIGN KEY(produkt_id) REFERENCES produkt(id), FOREIGN KEY(tag_id) REFERENCES tag(id), UNIQUE(produkt_id, tag_id));",
        "DROP TABLE IF EXISTS produkt_tag;"
    ))
    returner opp
}

funksjon _auto_migrasjonar() -> liste<ordbok_tekst> {
    hvis ikkje fil.finnes(_automigrasjonar_fil()) {
        returner []
    }

    la raw = fil.les_eller_tom(_automigrasjonar_fil())
    hvis raw == "" { returner [] }
    la parsed = builtin.json_parse_raw(raw)
    hvis builtin.type(parsed) != "ordbok" {
        returner []
    }
    hvis ikkje har_nokkel(parsed, "items") {
        returner []
    }

    la item = parsed["items"]
    hvis builtin.type(item) != "liste" { returner [] }

    la opp = []
    la i: heiltall = 0
    mens i < lengde(item) {
        la m = item[i]
        hvis builtin.type(m) != "ordbok" {
            i = i + 1
            fortsett
        }
        hvis har_nokkel(m, "name") og har_nokkel(m, "opp_sql") og har_nokkel(m, "ned_sql") {
            legg_til(opp, migrasjon.lag(m["name"], m["opp_sql"], m["ned_sql"]))
        }
        i = i + 1
    }
    returner opp
}

funksjon migrasjonar() -> liste<ordbok_tekst> {
    la opp = _initial_migrasjonar()
    la ekstra = _auto_migrasjonar()
    la j: heiltall = 0
    mens j < lengde(ekstra) {
        legg_til(opp, ekstra[j])
        j = j + 1
    }
    returner opp
}

funksjon all_products(conn: tekst) -> liste<ordbok_tekst> {
    returner orm.alle(conn, "produkt")
}

funksjon tags_for_produkt(conn: tekst, produkt_id: heiltall) -> liste<ordbok_tekst> {
    returner orm.reverser_many_to_many(conn, "produkt_tag", "tag", "produkt_id", produkt_id, "tag_id", "id", "t")
}

funksjon produkt_for_tag(conn: tekst, tag_id: heiltall) -> liste<ordbok_tekst> {
    returner orm.reverser_many_to_many(conn, "produkt_tag", "produkt", "tag_id", tag_id, "produkt_id", "id", "p")
}

funksjon many_to_many_tabellar() -> liste<ordbok_tekst> {
    returner relasjonar_many_to_many()
}
EOF_MODELS

cat > "$OUTPUT_DIR/migrations/schema_state.json" <<'EOF_SCHEMA_STATE'
{"models":[
  {"table":"kategori","fields":["id INTEGER PRIMARY KEY AUTOINCREMENT","namn TEXT NOT NULL","oppretta TEXT DEFAULT (datetime('now'))"]},
  {"table":"tag","fields":["id INTEGER PRIMARY KEY AUTOINCREMENT","namn TEXT NOT NULL","oppretta TEXT DEFAULT (datetime('now'))"]},
  {"table":"produkt","fields":["id INTEGER PRIMARY KEY AUTOINCREMENT","namn TEXT NOT NULL","pris INTEGER DEFAULT 0","lager INTEGER DEFAULT 0","kategori_id INTEGER","oppretta TEXT DEFAULT (datetime('now'))","FOREIGN KEY(kategori_id) REFERENCES kategori(id)"]},
  {"table":"produkt_tag","fields":["id INTEGER PRIMARY KEY AUTOINCREMENT","produkt_id INTEGER NOT NULL","tag_id INTEGER NOT NULL","FOREIGN KEY(produkt_id) REFERENCES produkt(id)","FOREIGN KEY(tag_id) REFERENCES tag(id)","UNIQUE(produkt_id, tag_id)"]}
]}
EOF_SCHEMA_STATE

cat > "$OUTPUT_DIR/src/views.no" <<'EOF_VIEWS'
bruk std.web_app_stack som stack
bruk std.web som web
bruk std.cache som cache_lib
bruk src.template som template
bruk src.routes som routes
bruk src.messages som messages
bruk src.cache som cache_mod
bruk src.i18n som i18n

funksjon index(ctx: ordbok_tekst, conn: tekst, register: ordbok_tekst) -> ordbok_tekst {
    la _ = conn
    la _ = register
    la brukar = ""
    hvis har_nokkel(ctx, "__auth_brukar__") { brukar = ctx["__auth_brukar__"] }
    la innhald = "<p>" + i18n.t("Welcome") + "</p>"
    la rute_map = routes.index()
    la meldingar = messages.hent(ctx, conn)
    innhald = innhald + "<p>Tilgjengelege ruter: " + tekst_fra_heltall(lengde(nøkler(rute_map))) + "</p>"
    la html = template.render("pages/home.html", {
        "tittel": i18n.t("Home page"),
        "brukar": brukar,
        "meldingar": meldingar,
        "innhald": innhald
    })
    returner web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, html)
}

funksjon cache_demo(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = ctx
    la cache = cache_mod.instans()
    la cache_key = "cache_demo_homepage"
    la cached = cache_lib.hent(cache, cache_key, "")
    hvis cached != "" {
        returner web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, cached)
    }

    la innhald = "<h1>Cache-demo</h1><p>Første treff vart generert no.</p>"
    la _ = cache_lib.sett(cache, cache_key, innhald, 120)
    returner web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, innhald)
}

funksjon i18n_demo(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = ctx
    la melding = i18n.t_ctx("Hello, %(name)s!", {"name": "venn"})
    returner web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, "<h1>" + melding + "</h1>")
}

funksjon api_health(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = ctx
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"service\":\"norscode\"}")
}

funksjon api_echo(ctx: ordbok_tekst) -> ordbok_tekst {
    prøv {
        la payload = web.request_json(ctx)
        la body = "{\"ok\":\"sann\",\"payload\":" + builtin.json_stringify(payload) + "}"
        returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, body)
    } fang (e) {
        la _ = e
        returner web.validation_bad_request(["body må vere gyldig JSON-objekt"])
    }
}

funksjon api_openapi(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = ctx
    la cfg = config.hent()
    la prosjekt_namn: tekst = "Norscode API"
    hvis har_nokkel(cfg, "name") {
        prosjekt_namn = cfg["name"]
    }
    la spec = web.openapi_json(prosjekt_namn, "1.0.0")
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, spec)
}

funksjon api_docs(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = ctx
    la cfg = config.hent()
    la prosjekt_namn: tekst = "Norscode API"
    hvis har_nokkel(cfg, "name") {
        prosjekt_namn = cfg["name"]
    }
    la html = web.docs_html(prosjekt_namn, "1.0.0")
    returner web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, html)
}

funksjon api_query(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /api/v1/query")
    la feil: liste<tekst> = []
    la namn = web.request_query_required_or_error(ctx, "name", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    la body = "{\"ok\":\"sann\",\"name\":\"" + namn + "\"}"
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, body)
}

funksjon api_validate(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/validate")
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
    web.route("GET /api/v1/items/{id}")
    la feil: liste<tekst> = []
    la id = web.request_param_int_or_error(ctx["__params__"], "id", feil)
    hvis lengde(feil) > 0 {
        returner web.validation_bad_request(feil)
    }
    returner web.response_builder(200, {"content-type": "application/json; charset=utf-8"}, "{\"ok\":\"sann\",\"id\":" + tekst_fra_heltall(id) + "}")
}

funksjon api_payload(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("POST /api/v1/payload")
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
    web.route("POST /api/v1/nested")
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
    web.route("GET /api/v1/headers")
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
    web.route("GET /api/v1/response-model")
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
            la msg = "ResponseValidationError: " + validation_error_body(feil)
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
    web.route("POST /api/v1/request-model")
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
    la body_payload = {
        "ok": "sann",
        "title": title,
        "count": count,
        "active": active,
        "tags": tags
    }
    la err: liste<tekst> = []
    web.response_shape_validate_or_error(body_payload, {
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
    returner web.response_builder(200, {"content-type":"application/json; charset=utf-8"}, builtin.json_stringify(body_payload))
}
EOF_VIEWS

if [ "$WITH_EMAIL" = "sann" ]; then
cat > "$OUTPUT_DIR/src/email.no" <<'EOF_EMAIL'
bruk std.epost som epost_lib
bruk src.config som config

funksjon _konfig() -> ordbok_tekst {
    la s = config.hent()
    la backend = "konsoll"
    hvis har_nokkel(s, "EMAIL_BACKEND") { backend = s["EMAIL_BACKEND"] }
    hvis backend == "smtp" {
        returner epost_lib.cfg_frå_innstillingar(s)
    }
    returner epost_lib.konsoll_cfg()
}

funksjon send(til: tekst, emne: tekst, tekst_melding: tekst) -> boolsk {
    la c = _konfig()
    la svar = epost_lib.send(c, til, emne, tekst_melding)
    returner svar
}

funksjon send_html(til: tekst, emne: tekst, html_melding: tekst, tekst_fallback: tekst) -> boolsk {
    la c = _konfig()
    la svar = epost_lib.send_html(c, til, emne, html_melding, tekst_fallback)
    returner svar
}

funksjon send_mal(til: tekst, emne: tekst, tmpl: tekst, ctx: ordbok_tekst) -> boolsk {
    la c = _konfig()
    la svar = epost_lib.send_mal(c, til, emne, tmpl, ctx)
    returner svar
}

funksjon send_html_mal(til: tekst, emne: tekst, html_tmpl: tekst, tekst_tmpl: tekst, ctx: ordbok_tekst) -> boolsk {
    la c = _konfig()
    la svar = epost_lib.send_html_mal(c, til, emne, html_tmpl, tekst_tmpl, ctx)
    returner svar
}

funksjon send_many(mottakarar: liste<tekst>, emne: tekst, melding: tekst) -> heiltall {
    la c = _konfig()
    la svar = epost_lib.send_mange(c, mottakarar, emne, melding)
    returner svar
}
EOF_EMAIL
fi

cat > "$OUTPUT_DIR/src/i18n.no" <<'EOF_I18N'
bruk std.i18n som i18n_lib
bruk src.config som config

funksjon _konfig() -> ordbok_tekst {
    returner config.hent()
}

funksjon sprak() -> tekst {
    la s = _konfig()
    hvis har_nokkel(s, "DEFAULT_LANGUAGE") { returner s["DEFAULT_LANGUAGE"] }
    returner "nn"
}

funksjon _katalog() -> tekst {
    la s = _konfig()
    hvis har_nokkel(s, "LOCALE_DIR") { returner s["LOCALE_DIR"] }
    returner "locale/"
}

funksjon last(sprak: tekst) -> ordbok_tekst {
    returner i18n_lib.last_med_fallback(_katalog(), sprak, "en")
}

funksjon aktiv() -> ordbok_tekst {
    returner last(sprak())
}

funksjon t(original: tekst) -> tekst {
    la ord = aktiv()
    la tom: ordbok_tekst = {}
    returner i18n_lib._(ord, original, tom)
}

funksjon t_ctx(original: tekst, ctx: ordbok_tekst) -> tekst {
    la ord = aktiv()
    returner i18n_lib._(ord, original, ctx)
}
EOF_I18N

cat > "$OUTPUT_DIR/src/auth/login.no" <<'EOF_AUTH_LOGIN'
bruk std.web som web
bruk std.web_app_stack som stack
bruk std.auth som auth
bruk std.sesjon som sesjon
bruk std.skjema som form_lib
bruk src.forms som forms
bruk src.template som template
bruk src.auth.throttle som auth_throttle
bruk src.messages som messages

funksjon _felt() -> liste<ordbok_tekst> {
    la felte: liste<ordbok_tekst> = []
    legg_til(felte, form_lib.epost_felt("epost", "E-post", sann))
    legg_til(felte, form_lib.passord_felt("passord", "Passord", 1))
    returner felte
}

funksjon _første_feil(resultat: ordbok_tekst) -> tekst {
    la feil = resultat["feil"]
    la nøkkel = nøkler(feil)
    hvis lengde(nøkkel) == 0 { returner "" }
    returner feil[nøkkel[0]]
}

funksjon _med_sesjon_kjeks(resp: ordbok_tekst, csrf_ctx: ordbok_tekst) -> ordbok_tekst {
    hvis csrf_ctx["set_cookie"] == "sann" {
        returner sesjon.set_cookie(resp, csrf_ctx["session_token"], 86400, usann)
    }
    returner resp
}

funksjon _skjema_html(ctx: ordbok_tekst, conn: tekst, resultat: ordbok_tekst) -> tekst {
    la base = form_lib.til_html(_felt(), resultat, "/auth/login", "POST")
    returner forms.form_with_csrf(ctx, conn, base)
}

funksjon skjema(ctx: ordbok_tekst, conn: tekst) -> ordbok_tekst {
    la tomt = form_lib.valider(_felt(), {})
    la csrf_ctx = forms.csrf_context(ctx, conn)
    la meldingar = messages.hent(ctx, conn)
    la html = template.render("auth/login.html", {
        "tittel": "Logg inn",
        "feilmelding": "",
        "meldingar": meldingar,
        "innhald": _skjema_html(ctx, conn, tomt)
    })
    la response = web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, html)
    returner _med_sesjon_kjeks(response, csrf_ctx)
}

funksjon utfør(ctx: ordbok_tekst, conn: tekst) -> ordbok_tekst {
    la data = forms.parse(ctx)
    hvis ikkje forms.csrf_ok(ctx, conn, data) {
        la csrf_ctx = forms.csrf_context(ctx, conn)
        la html = template.render("auth/login.html", {
            "tittel": "Logg inn",
            "feilmelding": "Ugyldig CSRF-token",
            "meldingar": messages.hent(ctx, conn),
            "innhald": _skjema_html(ctx, conn, form_lib.valider(_felt(), data))
        })
        la response = web.response_builder(403, {"content-type": "text/html; charset=utf-8"}, html)
        returner _med_sesjon_kjeks(response, csrf_ctx)
    }
    la validert = form_lib.valider(_felt(), data)
    hvis validert["gyldig"] == "usann" {
        la html = template.render("auth/login.html", {
            "tittel": "Logg inn",
            "feilmelding": _første_feil(validert),
            "meldingar": messages.hent(ctx, conn),
            "innhald": _skjema_html(ctx, conn, validert)
        })
        returner web.response_builder(400, {"content-type": "text/html; charset=utf-8"}, html)
    }
    la verdiar = form_lib.hent_verdiar(validert)
    la epost = verdiar["epost"]
    la passord = verdiar["passord"]
    hvis ikkje auth_throttle.kan_logge_inn(ctx, epost) {
        la html = template.render("auth/login.html", {
            "tittel": "Logg inn",
            "feilmelding": auth_throttle.melding_blokkert(ctx),
            "meldingar": messages.hent(ctx, conn),
            "innhald": _skjema_html(ctx, conn, validert)
        })
        returner web.response_builder(429, {"content-type": "text/html; charset=utf-8"}, html)
    }
    la token = auth.logg_inn(conn, epost, passord)
    hvis token == "" {
        la html = template.render("auth/login.html", {
            "tittel": "Logg inn",
            "feilmelding": "Ugyldig e-post eller passord",
            "meldingar": messages.hent(ctx, conn),
            "innhald": _skjema_html(ctx, conn, validert)
        })
        returner web.response_builder(401, {"content-type": "text/html; charset=utf-8"}, html)
    }
    la innhald = "<p>Innlogging fullført. Du blir no sendt vidare.</p>"
    la html = template.render("pages/home.html", {
        "tittel": "Innlogga",
        "brukar": epost,
        "innhald": innhald
    })
    returner web.response_builder(302, {"content-type": "text/html; charset=utf-8", "set-cookie": sesjon.session_cookie(token, 1), "location": "/"}, html)
}
EOF_AUTH_LOGIN

cat > "$OUTPUT_DIR/src/auth/throttle.no" <<'EOF_AUTH_THROTTLE'
bruk std.web som web
bruk std.mw som mw
bruk src.config som config

funksjon _er_sann(cfg: ordbok_tekst, nøkkel: tekst, fallback: tekst) -> boolsk {
    hvis ikkje har_nokkel(cfg, nøkkel) { returner tekst_til_liten(fallback) == "sann" }
    la verdi = tekst_til_liten(cfg[nøkkel])
    returner verdi != "usann" og verdi != "false" og verdi != "0" og verdi != "nei"
}

funksjon _ip(ctx: ordbok_tekst) -> tekst {
    la ip = web.request_header(ctx, "x-forwarded-for")
    hvis ip == "" { ip = web.request_header(ctx, "x-real-ip") }
    hvis ip == "" { returner "ukjend" }
    la komma = builtin.index_of(ip, ",")
    hvis komma > 0 { returner builtin.trim(builtin.slice(ip, 0, komma)) }
    returner builtin.trim(ip)
}

funksjon _cfg() -> ordbok_tekst {
    la cfg = config.hent()
    la c: ordbok_tekst = {}
    c["aktiv"] = _er_sann(cfg, "BRUTEFORCE_LOGIN_ENABLED", "usann")
    c["maks"] = "3"
    hvis har_nokkel(cfg, "BRUTEFORCE_LOGIN_MAX") {
        c["maks"] = cfg["BRUTEFORCE_LOGIN_MAX"]
    }
    c["window"] = "120"
    hvis har_nokkel(cfg, "BRUTEFORCE_LOGIN_WINDOW_SEK") {
        c["window"] = cfg["BRUTEFORCE_LOGIN_WINDOW_SEK"]
    }
    c["nøkkel"] = "ip"
    hvis har_nokkel(cfg, "BRUTEFORCE_LOGIN_KEY") {
        c["nøkkel"] = cfg["BRUTEFORCE_LOGIN_KEY"]
    }
    returner c
}

funksjon _nøkkel(ctx: ordbok_tekst, epost: tekst) -> tekst {
    la cfg = _cfg()
    hvis cfg["nøkkel"] == "brukar" eller cfg["nøkkel"] == "brukarnamn" {
        hvis epost != "" { returner "brukar:" + epost }
        returner "ip:" + _ip(ctx)
    }
    returner "ip:" + _ip(ctx)
}

funksjon _rate_cfg(maks: tekst, vindauge: tekst) -> ordbok_tekst {
    returner mw.rate_cfg(heltall_fra_tekst(maks), "auth_login")
}

funksjon kan_logge_inn(ctx: ordbok_tekst, epost: tekst) -> boolsk {
    la cfg = _cfg()
    hvis ikkje cfg["aktiv"] { returner sann }
    la nøkkel = _nøkkel(ctx, epost)
    la ok = mw.sjekk_rate(_rate_cfg(cfg["maks"], cfg["window"]), nøkkel)
    returner ok
}

funksjon melding_blokkert(ctx: ordbok_tekst) -> tekst {
    la cfg = _cfg()
    la maks = cfg["maks"]
    la vind = cfg["window"]
    returner "For mange mislykka innloggingsforsøk. Prøv igjen etter " + vind + " sekund (maks " + maks + " forsøk)."
}

EOF_AUTH_THROTTLE

cat > "$OUTPUT_DIR/src/auth/register.no" <<'EOF_AUTH_REGISTER'
bruk std.web som web
bruk std.web_app_stack som stack
bruk std.auth som auth
bruk std.sesjon som sesjon
bruk std.skjema som form_lib
bruk src.forms som forms
bruk src.template som template
bruk src.messages som messages

funksjon _felt() -> liste<ordbok_tekst> {
    la felte: liste<ordbok_tekst> = []
    legg_til(felte, form_lib.tekst_felt("brukarnamn", "Brukarnamn", sann, 2, 80))
    legg_til(felte, form_lib.epost_felt("epost", "E-post", sann))
    legg_til(felte, form_lib.passord_felt("passord", "Passord", 8))
    returner felte
}

funksjon _første_feil(resultat: ordbok_tekst) -> tekst {
    la feil = resultat["feil"]
    la nøkkel = nøkler(feil)
    hvis lengde(nøkkel) == 0 { returner "" }
    returner feil[nøkkel[0]]
}

funksjon _med_sesjon_kjeks(resp: ordbok_tekst, csrf_ctx: ordbok_tekst) -> ordbok_tekst {
    hvis csrf_ctx["set_cookie"] == "sann" {
        returner sesjon.set_cookie(resp, csrf_ctx["session_token"], 86400, usann)
    }
    returner resp
}

funksjon _skjema_html(ctx: ordbok_tekst, conn: tekst, resultat: ordbok_tekst) -> tekst {
    la base = form_lib.til_html(_felt(), resultat, "/auth/register", "POST")
    returner forms.form_with_csrf(ctx, conn, base)
}

funksjon skjema(ctx: ordbok_tekst, conn: tekst) -> ordbok_tekst {
    la tomt = form_lib.valider(_felt(), {})
    la csrf_ctx = forms.csrf_context(ctx, conn)
    la meldingar = messages.hent(ctx, conn)
    la html = template.render("auth/register.html", {
        "tittel": "Registrer ny brukar",
        "feilmelding": "",
        "meldingar": meldingar,
        "innhald": _skjema_html(ctx, conn, tomt)
    })
    la response = web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, html)
    returner _med_sesjon_kjeks(response, csrf_ctx)
}

funksjon utfør(ctx: ordbok_tekst, conn: tekst) -> ordbok_tekst {
    la data = forms.parse(ctx)
    la csrf_ctx = forms.csrf_context(ctx, conn)
    hvis ikkje forms.csrf_ok(ctx, conn, data) {
        la html = template.render("auth/register.html", {
            "tittel": "Registrer ny brukar",
            "feilmelding": "Ugyldig CSRF-token",
            "meldingar": messages.hent(ctx, conn),
            "innhald": _skjema_html(ctx, conn, form_lib.valider(_felt(), data))
        })
        la response = web.response_builder(403, {"content-type": "text/html; charset=utf-8"}, html)
        returner _med_sesjon_kjeks(response, csrf_ctx)
    }
    la validert = form_lib.valider(_felt(), data)
    hvis validert["gyldig"] == "usann" {
        la html = template.render("auth/register.html", {
            "tittel": "Registrer ny brukar",
            "feilmelding": _første_feil(validert),
            "meldingar": messages.hent(ctx, conn),
            "innhald": _skjema_html(ctx, conn, validert)
        })
        returner web.response_builder(400, {"content-type": "text/html; charset=utf-8"}, html)
    }
    la verdiar = form_lib.hent_verdiar(validert)
    la brukarnamn = verdiar["brukarnamn"]
    la epost = verdiar["epost"]
    la passord = verdiar["passord"]
    la id = auth.registrer(conn, brukarnamn, epost, passord)
    hvis id < 0 {
        la html = template.render("auth/register.html", {
            "tittel": "Registrer ny brukar",
            "feilmelding": "Brukar eller e-post er allereie i bruk",
            "meldingar": messages.hent(ctx, conn),
            "innhald": _skjema_html(ctx, conn, validert)
        })
        returner web.response_builder(409, {"content-type": "text/html; charset=utf-8"}, html)
    }
    la _ = sesjon.flash_sett(conn, csrf_ctx["session_token"], "suksess", "Brukar oppretta. Logg inn no.")
    la response = web.response_builder(302, {"content-type": "text/html; charset=utf-8", "location": "/auth/login"}, "")
    returner _med_sesjon_kjeks(response, csrf_ctx)
}
EOF_AUTH_REGISTER

cat > "$OUTPUT_DIR/src/auth/logout.no" <<'EOF_AUTH_LOGOUT'
bruk std.web som web
bruk std.sesjon som sesjon

funksjon utfør(ctx: ordbok_tekst) -> ordbok_tekst {
    la _ = ctx
    returner web.response_builder(200, {"content-type": "text/plain; charset=utf-8", "set-cookie": "session=; HttpOnly; SameSite=Lax; Path=/; Max-Age=0"}, "logout")
}
EOF_AUTH_LOGOUT

cat > "$OUTPUT_DIR/src/auth/routes.no" <<'EOF_AUTH_ROUTES'
funksjon index() -> ordbok_tekst {
    returner {
        "GET /auth/login": "auth.login.skjema",
        "POST /auth/login": "auth.login.utfør",
        "GET /auth/register": "auth.register.skjema",
        "POST /auth/register": "auth.register.utfør",
        "POST /auth/logout": "auth.logout.utfør"
    }
}
EOF_AUTH_ROUTES
cat > "$OUTPUT_DIR/src/auth/index.no" <<'EOF_AUTH_INDEX'
bruk src.auth.routes som auth_routes

funksjon routes() -> ordbok_tekst {
    returner auth_routes.index()
}
EOF_AUTH_INDEX

cat > "$OUTPUT_DIR/src/auth/guards.no" <<'EOF_AUTH_GUARDS'
bruk std.web_app_stack som stack

funksjon krev_innlogging(ap_ctx: ordbok_tekst, conn: tekst) -> boolsk {
    la _ = conn
    returner stack.krev_innlogging(conn, ap_ctx)
}

funksjon krev_rolle(ap_ctx: ordbok_tekst, conn: tekst, rolle: tekst) -> boolsk {
    la _ = conn
    returner stack.krev_rolle(conn, ap_ctx, rolle)
}

funksjon _standard_rettar() -> ordbok_tekst {
    la m: ordbok_tekst = {}
    m["admin"] = ["*"]
    m["redaktør"] = ["admin.overview", "admin.edit", "admin.view"]
    m["brukar"] = ["admin.view"]
    returner m
}

funksjon krev_permission(ap_ctx: ordbok_tekst, conn: tekst, rett: tekst) -> boolsk {
    la _ = conn
    la rolle = "usynleg"
    hvis stack.bruker_rolle(ap_ctx) != "" {
        rolle = stack.bruker_rolle(ap_ctx)
    }
    hvis rolle == "admin" { returner sann }
    la rettar = _standard_rettar()
    hvis ikkje har_nokkel(rettar, rolle) { returner usann }
    la l = rettar[rolle]
    la i: heiltall = 0
    mens i < lengde(l) {
        hvis l[i] == rett {
            returner sann
        }
        i = i + 1
    }
    returner usann
}

funksjon har_permission(ap_ctx: ordbok_tekst, conn: tekst, rett: tekst) -> boolsk {
    la _ = conn
    la rolle = "usynleg"
    hvis stack.bruker_rolle(ap_ctx) != "" {
        rolle = stack.bruker_rolle(ap_ctx)
    }
    hvis rolle == "admin" { returner sann }
    la rettar = _standard_rettar()
    hvis ikkje har_nokkel(rettar, rolle) { returner usann }
    la l = rettar[rolle]
    la i: heiltall = 0
    mens i < lengde(l) {
        hvis l[i] == rett { returner sann }
        i = i + 1
    }
    returner usann
}
EOF_AUTH_GUARDS

cat > "$OUTPUT_DIR/src/forms.no" <<'EOF_FORMS'
bruk std.skjema som skjema
bruk std.web som web
bruk std.sesjon som sesjon

funksjon parse(ctx: ordbok_tekst) -> ordbok_tekst {
    la body = web.request_body(ctx)
    prøv {
        la parsed = builtin.json_parse_raw(body)
        la type_verdi = builtin.type(parsed)
        hvis type_verdi == "ordbok" {
            returner parsed
        }
    } fang (e) {
    }
    la data = parse_form(body)
    returner data
}

funksjon parse_form(body: tekst) -> ordbok_tekst {
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

funksjon _sesjon_token(ctx: ordbok_tekst, conn: tekst) -> tekst {
    la token = web.request_cookie(ctx, "session")
    hvis har_nokkel(ctx, "__sesjon_token__") {
        token = ctx["__sesjon_token__"]
    }
    hvis token == "" { returner "" }
    hvis sesjon.er_gyldig(conn, token) { returner token }
    returner ""
}

funksjon _csrf_nokkel() -> tekst {
    returner "__csrf_token__"
}

funksjon csrf_context(ctx: ordbok_tekst, conn: tekst) -> ordbok_tekst {
    la token = _sesjon_token(ctx, conn)
    la set_cookie = "usann"
    hvis token == "" {
        la token = sesjon.start_med_ttl(conn, ctx, 86400)
        set_cookie = "sann"
    }
    la csrf_token = sesjon.hent(conn, token, _csrf_nokkel(), "")
    hvis csrf_token == "" {
        la csrf_token = builtin.random_hex(64)
        la _ = sesjon.sett(conn, token, _csrf_nokkel(), csrf_token)
    }
    returner {
        "session_token": token,
        "csrf_token": csrf_token,
        "set_cookie": set_cookie
    }
}

funksjon csrf_context_existing(ctx: ordbok_tekst, conn: tekst) -> ordbok_tekst {
    la token = _sesjon_token(ctx, conn)
    hvis token == "" { returner {} }
    la csrf = sesjon.hent(conn, token, _csrf_nokkel(), "")
    hvis csrf == "" { returner {} }
    returner {
        "session_token": token,
        "csrf_token": csrf
    }
}

funksjon csrf_input(ctx: ordbok_tekst, conn: tekst) -> tekst {
    la c = csrf_context(ctx, conn)
    returner "    <input type=\"hidden\" name=\"__csrf_token__\" value=\"" + c["csrf_token"] + "\">"
}

funksjon form_with_csrf(ctx: ordbok_tekst, conn: tekst, base_form: tekst) -> tekst {
    la pos = builtin.index_of(base_form, ">")
    hvis pos < 0 { returner base_form + "\n" + csrf_input(ctx, conn) }
    la start = builtin.slice(base_form, 0, pos + 1)
    la rest = builtin.slice(base_form, pos + 1, lengde(base_form))
    returner start + "\n" + csrf_input(ctx, conn) + "\n" + rest
}

funksjon csrf_ok(ctx: ordbok_tekst, conn: tekst, data: ordbok_tekst) -> boolsk {
    la c = csrf_context_existing(ctx, conn)
    hvis ikkje har_nokkel(c, "csrf_token") { returner usann }
    hvis ikkje har_nokkel(data, "__csrf_token__") { returner usann }
    returner c["csrf_token"] == tekst(data["__csrf_token__"])
}

funksjon parse_verdiar(ctx: ordbok_tekst, felt: liste<ordbok_tekst>) -> ordbok_tekst {
    la data = parse(ctx)
    returner skjema.valider(felt, data)
}
EOF_FORMS

cat > "$OUTPUT_DIR/src/messages.no" <<'EOF_MESSAGES_MODULE'
bruk std.sesjon som sesjon

funksjon _ctx_token(ctx: ordbok_tekst, conn: tekst) -> tekst {
    la token = sesjon.hent_token(ctx)
    hvis token == "" { returner "" }
    hvis sesjon.er_gyldig(conn, token) { returner token }
    returner ""
}

funksjon set_med_token(conn: tekst, token: tekst, type: tekst, melding: tekst) -> boolsk {
    hvis token == "" { returner usann }
    la r = sesjon.flash_sett(conn, token, type, melding)
    returner r > 0
}

funksjon set(ctx: ordbok_tekst, conn: tekst, type: tekst, melding: tekst) -> boolsk {
    la token = _ctx_token(ctx, conn)
    hvis token == "" { returner usann }
    returner set_med_token(conn, token, type, melding)
}

funksjon hent_med_token(conn: tekst, token: tekst) -> ordbok_tekst {
    hvis token == "" { returner {} }
    returner sesjon.flash_hent_alle(conn, token)
}

funksjon hent(ctx: ordbok_tekst, conn: tekst) -> ordbok_tekst {
    la token = _ctx_token(ctx, conn)
    hvis token == "" { returner {} }
    returner hent_med_token(conn, token)
}

funksjon kontekst(ctx: ordbok_tekst, conn: tekst) -> ordbok_tekst {
    la flash = hent(ctx, conn)
    hvis lengde(nøkler(flash)) == 0 { returner {} }
    returner {"meldingar": flash}
}
EOF_MESSAGES_MODULE

cat > "$OUTPUT_DIR/src/cache.no" <<'EOF_CACHE_MODULE'
bruk std.cache som cache
bruk src.config som config

la _globale_cache: ordbok_tekst = cache.fra_innstillingar(config.hent())

funksjon instans() -> ordbok_tekst {
    returner _globale_cache
}

funksjon ny() -> ordbok_tekst {
    returner cache.fra_innstillingar(config.hent())
}

funksjon ny_minne() -> ordbok_tekst {
    returner cache.ny_minne()
}

funksjon ny_fil(mappe: tekst) -> ordbok_tekst {
    returner cache.ny_fil(mappe)
}

funksjon sett(c: ordbok_tekst, nøkkel: tekst, verdi: tekst, ttl_sek: heiltall) -> heiltall {
    returner cache.sett(c, nøkkel, verdi, ttl_sek)
}

funksjon hent(c: ordbok_tekst, nøkkel: tekst, standard: tekst) -> tekst {
    returner cache.hent(c, nøkkel, standard)
}

funksjon slett(c: ordbok_tekst, nøkkel: tekst) -> heiltall {
    returner cache.slett(c, nøkkel)
}

funksjon tøm(c: ordbok_tekst) -> heiltall {
    returner cache.tøm(c)
}

funksjon sett_med_tag(c: ordbok_tekst, nøkkel: tekst, verdi: tekst, ttl_sek: heiltall, tag: tekst) -> heiltall {
    returner cache.sett_med_tag(c, nøkkel, verdi, ttl_sek, tag)
}

funksjon slett_tag(c: ordbok_tekst, tag: tekst) -> heiltall {
    returner cache.slett_tag(c, tag)
}

funksjon set_response(c: ordbok_tekst, nøkkel: tekst, respons: ordbok_tekst, ttl_sek: heiltall) -> heiltall {
    returner cache.cache_respons(c, nøkkel, respons, ttl_sek)
}

funksjon hent_response(c: ordbok_tekst, nøkkel: tekst) -> ordbok_tekst {
    returner cache.hent_respons(c, nøkkel)
}

funksjon hent_eller_set(c: ordbok_tekst, nøkkel: tekst, ttl_sek: heiltall, fn_namn: tekst, arg: tekst) -> tekst {
    returner cache.hent_eller_set(c, nøkkel, ttl_sek, fn_namn, arg)
}
EOF_CACHE_MODULE

cat > "$OUTPUT_DIR/src/template.no" <<'EOF_TEMPLATE'
bruk std.mal som mal
bruk std.web som web

funksjon render(sti: tekst, kontekst: ordbok_tekst) -> tekst {
    returner mal.gjengi_arv("src/templates/" + sti, kontekst, "src/templates/")
}

funksjon render_html(sti: tekst, kontekst: ordbok_tekst) -> ordbok_tekst {
    la innhald = render(sti, kontekst)
    returner web.response_builder(200, {"content-type": "text/html; charset=utf-8"}, innhald)
}

funksjon render_feil(sti: tekst, status: heiltall, tittel: tekst, feilmelding: tekst) -> ordbok_tekst {
    la innhald = render(sti, {
        "tittel": tittel,
        "feilmelding": feilmelding,
        "kode": tekst_fra_heltall(status),
        "innhald": "<p>" + feilmelding + "</p>"
    })
    returner web.response_builder(status, {"content-type": "text/html; charset=utf-8"}, innhald)
}
EOF_TEMPLATE

cat > "$OUTPUT_DIR/src/static.no" <<'EOF_STATIC'
bruk std.web som web
bruk std.mimetypes som mimetype

funksjon _har_triks(sti: tekst) -> boolsk {
    la deler = builtin.split(sti, "/")
    la i: heiltall = 0
    mens i < lengde(deler) {
        hvis deler[i] == ".." { returner sann }
        i = i + 1
    }
    returner usann
}

funksjon _normaliser_sti(sti: tekst) -> tekst {
    hvis ikkje tekst_starter_med(sti, "/static/") {
        hvis sti == "/static" { returner "" }
        returner ""
    }

    la rel = builtin.slice(sti, 8, lengde(sti))
    hvis rel == "" { returner "" }
    hvis _har_triks(rel) { returner "" }

    la deler = builtin.split(rel, "/")
    la rensa: liste<tekst> = []
    la i: heiltall = 0
    mens i < lengde(deler) {
        la bit = deler[i]
        hvis bit == "" eller bit == "." {
            i = i + 1
            fortsett
        }
        legg_til(rensa, bit)
        i = i + 1
    }

    hvis lengde(rensa) == 0 { returner "" }
    returner "static/" + tekst_saman(rensa, "/")
}

funksjon svar(sti: tekst) -> ordbok_tekst {
    la fil_sti = _normaliser_sti(sti)
    hvis fil_sti == "" {
        returner web.response_error(400, "Ugyldig static-sti")
    }
    la ct = mimetype.content_type_header(fil_sti)
    hvis ct == "" { la ct = "application/octet-stream" }
    returner web.response_file(fil_sti, ct)
}
EOF_STATIC

if [ "$WITH_AUTH" = "usann" ]; then
cat > "$OUTPUT_DIR/src/auth/routes.no" <<'EOF_AUTH_ROUTES_EMPTY'
funksjon index() -> ordbok_tekst {
    returner {}
}
EOF_AUTH_ROUTES_EMPTY
fi

if [ "$WITH_ADMIN" = "sann" ]; then
cat > "$OUTPUT_DIR/src/admin_bootstrap.no" <<'EOF_ADMINBOOT'
bruk std.web_app_stack som stack

funksjon registrer_modellar(register: ordbok_tekst, conn: tekst) -> ordbok_tekst {
    la _ = conn
    returner stack.registrer_adminmodell(register, "produkt", ["id", "namn", "pris", "lager", "oppretta"], "Produkt")
}

funksjon oversikt(conn: tekst, register: ordbok_tekst, ctx: ordbok_tekst) -> ordbok_tekst {
    returner stack.admin_oversikt(conn, register, ctx)
}

funksjon liste(conn: tekst, register: ordbok_tekst, ctx: ordbok_tekst) -> ordbok_tekst {
    returner stack.admin_liste(conn, register, ctx)
}

funksjon ny(conn: tekst, register: ordbok_tekst, ctx: ordbok_tekst) -> ordbok_tekst {
    returner stack.admin_ny(conn, register, ctx)
}

funksjon rediger(conn: tekst, register: ordbok_tekst, ctx: ordbok_tekst) -> ordbok_tekst {
    returner stack.admin_rediger(conn, register, ctx)
}

funksjon lagre(conn: tekst, register: ordbok_tekst, ctx: ordbok_tekst) -> ordbok_tekst {
    returner stack.admin_lagre(conn, register, ctx)
}

funksjon slett(conn: tekst, register: ordbok_tekst, ctx: ordbok_tekst) -> ordbok_tekst {
    returner stack.admin_slett(conn, register, ctx)
}
EOF_ADMINBOOT
cat > "$OUTPUT_DIR/src/admin/index.no" <<'EOF_ADMIN_INDEX'
bruk src.admin_bootstrap som admin_bootstrap

funksjon routes() -> ordbok_tekst {
    returner {
        "GET /admin": "admin_bootstrap.oversikt",
        "GET /admin/{tabell}": "admin_bootstrap.liste",
        "GET /admin/{tabell}/ny": "admin_bootstrap.ny",
        "GET /admin/{tabell}/{id}/rediger": "admin_bootstrap.rediger",
        "POST /admin/{tabell}/lagre": "admin_bootstrap.lagre",
        "POST /admin/{tabell}/{id}/lagre": "admin_bootstrap.lagre",
        "POST /admin/{tabell}/{id}/slett": "admin_bootstrap.slett"
    }
}
EOF_ADMIN_INDEX
fi

cat > "$OUTPUT_DIR/src/templates/base.html" <<'EOF_TPL'
<!doctype html>
<html lang="no">
  <head>
    <meta charset="utf-8">
    <title>{{tittel|standard:Norscode}}</title>
    <style>
      body { font-family: Inter, ui-sans-serif, "Segoe UI", Arial, sans-serif; margin: 0; background: #f5f7fb; color: #101828; }
      header, footer { background: #111827; color: #ffffff; padding: 12px 18px; }
      main { max-width: 880px; margin: 18px auto; padding: 0 14px; }
      .kort { background: #ffffff; border-radius: 8px; padding: 16px; box-shadow: 0 6px 20px rgba(16,24,40,.08); }
      .feil { background: #fee4e2; border-left: 3px solid #b42318; padding: 10px; margin-bottom: 10px; border-radius: 6px; }
      form { display: block; }
      form div { margin-bottom: 10px; }
      label { display: block; font-size: .85rem; margin-bottom: 4px; color: #344054; }
      input { width: 100%; padding: 8px 10px; box-sizing: border-box; }
      button { margin-top: 8px; padding: 8px 12px; border: 0; background: #1d2939; color: #fff; border-radius: 6px; }
      nav a { color: #d0d5dd; margin-right: 10px; text-decoration: none; }
    </style>
  </head>
  <body>
    {{inkluder "src/templates/partials/header.html"}}
    <main>
      <div class="kort">
        {{@blokk innhald}}
          <h1>{{tittel|standard:Norskode}}</h1>
          <div>{{innhald|html}}</div>
        {{@slutt_blokk}}
      </div>
    </main>
    {{inkluder "src/templates/partials/footer.html"}}
  </body>
</html>
EOF_TPL

cat > "$OUTPUT_DIR/src/templates/partials/header.html" <<'EOF_HEADER'
<header>
  <nav>
    <a href="/">Hovudside</a>
    <a href="/cache-demo">Cache-demo</a>
    <a href="/i18n-demo">I18n-demo</a>
    <a href="/auth/login">Logg inn</a>
    <a href="/auth/register">Registrer</a>
  </nav>
</header>
EOF_HEADER

cat > "$OUTPUT_DIR/src/templates/partials/footer.html" <<'EOF_FOOTER'
<footer>
  <p>Bygd for Norscode web-stack</p>
</footer>
EOF_FOOTER

cat > "$OUTPUT_DIR/src/templates/partials/messages.html" <<'EOF_MESSAGES'
{{@hvis feilmelding}}
<div class="feil">{{feilmelding}}</div>
{{@slutt}}
{{@hvis meldingar.suksess}}
<div class="melding suksess">{{meldingar.suksess}}</div>
{{@slutt}}
{{@hvis meldingar.info}}
<div class="melding info">{{meldingar.info}}</div>
{{@slutt}}
{{@hvis meldingar.feil}}
<div class="melding feil">{{meldingar.feil}}</div>
{{@slutt}}
{{@hvis meldingar.åtvaring}}
<div class="melding åtvaring">{{meldingar.åtvaring}}</div>
{{@slutt}}
EOF_MESSAGES

cat > "$OUTPUT_DIR/src/templates/pages/home.html" <<'EOF_HOME'
{{@utvid "base.html"}}
{{@blokk innhald}}
  <h1>Hovudside</h1>
  <p>Hei {{brukar|standard:venn}}, velkomen til Norscode.</p>
  {{@hvis bruker}}
    <p>Innlogga brukar: {{brukar}}</p>
  {{@slutt}}
  {{innhald|html}}
{{@slutt_blokk}}
EOF_HOME

cat > "$OUTPUT_DIR/src/templates/auth/login.html" <<'EOF_LOGIN_TPL'
{{@utvid "base.html"}}
{{@blokk innhald}}
  <h1>Logg inn</h1>
  {{inkluder "src/templates/partials/messages.html"}}
  {{innhald|html}}
  <p>Har du ingen konto? <a href="/auth/register">Lag ein konto</a></p>
{{@slutt_blokk}}
EOF_LOGIN_TPL

cat > "$OUTPUT_DIR/src/templates/auth/register.html" <<'EOF_REGISTER_TPL'
{{@utvid "base.html"}}
{{@blokk innhald}}
  <h1>Registrer ny brukar</h1>
  {{inkluder "src/templates/partials/messages.html"}}
  {{innhald|html}}
  <p>Allereie brukar? <a href="/auth/login">Logg inn</a></p>
{{@slutt_blokk}}
EOF_REGISTER_TPL

cat > "$OUTPUT_DIR/src/templates/pages/error.html" <<'EOF_ERROR_TPL'
{{@utvid "base.html"}}
{{@blokk innhald}}
  <h1>Feil {{kode}}</h1>
  <p>{{feilmelding}}</p>
  <a href="/">Tilbake til hovudsida</a>
{{@slutt_blokk}}
EOF_ERROR_TPL

cat > "$OUTPUT_DIR/src/routes.no" <<'EOF_ROUTES'
bruk std.web som web

funksjon index() -> ordbok_tekst {
    returner {
EOF_ROUTES
 if [ "$WITH_ADMIN" = "sann" ] && [ "$WITH_AUTH" = "sann" ]; then
cat >> "$OUTPUT_DIR/src/routes.no" <<'EOF_ROUTES'
        "GET /": "route_index",
        "GET /cache-demo": "route_cache_demo",
        "GET /i18n-demo": "route_i18n_demo",
        "GET /api/v1/health": "route_api_health",
        "POST /api/v1/echo": "route_api_echo",
        "GET /api/v1/dependency": "route_api_dependency",
        "GET /openapi.json": "route_api_openapi",
        "GET /docs": "route_api_docs",
        "GET /api/v1/query": "route_api_query",
        "POST /api/v1/validate": "route_api_validate",
        "GET /api/v1/items/{id}": "route_api_item",
        "POST /api/v1/payload": "route_api_payload",
        "POST /api/v1/nested": "route_api_nested",
        "GET /api/v1/headers": "route_api_headers",
        "GET /api/v1/response-model": "route_api_response_model",
        "POST /api/v1/request-model": "route_api_request_model",
        "GET /admin": "route_admin_overview",
        "GET /admin/{tabell}": "route_admin_liste",
        "GET /admin/{tabell}/ny": "route_admin_ny",
        "GET /admin/{tabell}/{id}/rediger": "route_admin_rediger",
        "POST /admin/{tabell}/lagre": "route_admin_lagre",
        "POST /admin/{tabell}/{id}/lagre": "route_admin_lagre",
        "POST /admin/{tabell}/{id}/slett": "route_admin_slett",
        "GET /auth/login": "route_login_skjemat",
        "POST /auth/login": "route_login",
        "GET /auth/register": "route_register_skjemat",
        "POST /auth/register": "route_register",
        "POST /auth/logout": "route_logout",
        "GET /profile": "route_profile",
        "GET /static/{sti}": "route_static"
EOF_ROUTES
 elif [ "$WITH_ADMIN" = "sann" ]; then
cat >> "$OUTPUT_DIR/src/routes.no" <<'EOF_ROUTES'
        "GET /": "route_index",
        "GET /cache-demo": "route_cache_demo",
        "GET /i18n-demo": "route_i18n_demo",
        "GET /api/v1/health": "route_api_health",
        "POST /api/v1/echo": "route_api_echo",
        "GET /api/v1/dependency": "route_api_dependency",
        "GET /openapi.json": "route_api_openapi",
        "GET /docs": "route_api_docs",
        "GET /api/v1/query": "route_api_query",
        "POST /api/v1/validate": "route_api_validate",
        "GET /api/v1/items/{id}": "route_api_item",
        "POST /api/v1/payload": "route_api_payload",
        "POST /api/v1/nested": "route_api_nested",
        "GET /api/v1/headers": "route_api_headers",
        "GET /api/v1/response-model": "route_api_response_model",
        "POST /api/v1/request-model": "route_api_request_model",
        "GET /admin": "route_admin_overview",
        "GET /admin/{tabell}": "route_admin_liste",
        "GET /admin/{tabell}/ny": "route_admin_ny",
        "GET /admin/{tabell}/{id}/rediger": "route_admin_rediger",
        "POST /admin/{tabell}/lagre": "route_admin_lagre",
        "POST /admin/{tabell}/{id}/lagre": "route_admin_lagre",
        "POST /admin/{tabell}/{id}/slett": "route_admin_slett",
        "GET /static/{sti}": "route_static"
EOF_ROUTES
 elif [ "$WITH_AUTH" = "sann" ]; then
cat >> "$OUTPUT_DIR/src/routes.no" <<'EOF_ROUTES'
        "GET /": "route_index",
        "GET /cache-demo": "route_cache_demo",
        "GET /i18n-demo": "route_i18n_demo",
        "GET /api/v1/health": "route_api_health",
        "POST /api/v1/echo": "route_api_echo",
        "GET /api/v1/dependency": "route_api_dependency",
        "GET /openapi.json": "route_api_openapi",
        "GET /docs": "route_api_docs",
        "GET /api/v1/query": "route_api_query",
        "POST /api/v1/validate": "route_api_validate",
        "GET /api/v1/items/{id}": "route_api_item",
        "POST /api/v1/payload": "route_api_payload",
        "POST /api/v1/nested": "route_api_nested",
        "GET /api/v1/headers": "route_api_headers",
        "GET /api/v1/response-model": "route_api_response_model",
        "POST /api/v1/request-model": "route_api_request_model",
        "GET /auth/login": "route_login_skjemat",
        "POST /auth/login": "route_login",
        "GET /auth/register": "route_register_skjemat",
        "POST /auth/register": "route_register",
        "POST /auth/logout": "route_logout",
        "GET /profile": "route_profile",
        "GET /static/{sti}": "route_static"
EOF_ROUTES
 else
cat >> "$OUTPUT_DIR/src/routes.no" <<'EOF_ROUTES'
        "GET /": "route_index",
        "GET /cache-demo": "route_cache_demo",
        "GET /i18n-demo": "route_i18n_demo",
        "GET /api/v1/health": "route_api_health",
        "POST /api/v1/echo": "route_api_echo",
        "GET /api/v1/dependency": "route_api_dependency",
        "GET /openapi.json": "route_api_openapi",
        "GET /docs": "route_api_docs",
        "GET /api/v1/query": "route_api_query",
        "POST /api/v1/validate": "route_api_validate",
        "GET /api/v1/items/{id}": "route_api_item",
        "POST /api/v1/payload": "route_api_payload",
        "POST /api/v1/nested": "route_api_nested",
        "GET /api/v1/headers": "route_api_headers",
        "GET /api/v1/response-model": "route_api_response_model",
        "POST /api/v1/request-model": "route_api_request_model",
        "GET /static/{sti}": "route_static"
EOF_ROUTES
fi
cat >> "$OUTPUT_DIR/src/routes.no" <<'EOF_ROUTES'
    }
}
EOF_ROUTES

cat > "$OUTPUT_DIR/src/route_registry.no" <<'EOF_ROUTE_REG'
bruk src.routes som routes

funksjon _prefiks(funksjon_namn: tekst) -> tekst {
    hvis tekst_inneholder(funksjon_namn, ".") {
        returner "src." + funksjon_namn
    }
    returner "app." + funksjon_namn
}

funksjon index() -> ordbok_tekst {
    la grunnruter = routes.index()
    la resultat: ordbok_tekst = {}
    la nøklar = nøkler(grunnruter)
    la i: heiltall = 0
    mens i < lengde(nøklar) {
        la nøkkel = nøklar[i]
        la funksjon_namn = tekst(grunnruter[nøkkel])
        resultat[nøkkel] = _prefiks(funksjon_namn)
        i = i + 1
    }
    returner resultat
}
EOF_ROUTE_REG

cat > "$OUTPUT_DIR/apps/core/core.no" <<'EOF_CORE'
funksjon start() -> heiltall {
    returner 0
}
EOF_CORE

cat > "$OUTPUT_DIR/apps/core/README.md" <<'EOF_CORE_README'
# core

Appmodul oppretta av `nc startproject`.
EOF_CORE_README

cat > "$OUTPUT_DIR/apps/core/tests/test_core.no" <<'EOF_CORE_TEST'
import core

funksjon test_start() {
    la r = core.start()
    assert_eq(r, 0)
}
EOF_CORE_TEST

cat > "$OUTPUT_DIR/tests/test_app.no" <<'EOF_TEST'
bruk std.test som test
import app

funksjon test_start() {
    la r = app.start()
    assert_eq(r, 0)
}

funksjon start() -> heiltall {
    la testar = ["test_start"]
    returner test.køyr(testar)
}
EOF_TEST

cat > "$OUTPUT_DIR/tests/test_stack_migrasjon.no" <<'EOF_TEST_MIG'
bruk std.test som test
bruk src.models som models

funksjon test_migrasjon_antal() {
    la migrasjonar = models.migrasjonar()
    assert(lengde(migrasjonar) >= 1)
}

funksjon test_modell_manifest() {
    la modellar = models.modeller()
    assert(lengde(modellar) >= 1)
}

funksjon start() -> heiltall {
    la testar = ["test_migrasjon_antal", "test_modell_manifest"]
    returner test.køyr(testar)
}
EOF_TEST_MIG

cat > "$OUTPUT_DIR/tests/test_stack_auth.no" <<'EOF_TEST_AUTH'
bruk std.test som test
bruk std.db som db
bruk std.sesjon som sesjon
bruk src.config som config
bruk src.messages som messages
bruk src.auth.throttle som auth_throttle
bruk std.web som web

funksjon test_innstillingar_lastes() {
    la cfg = config.hent()
    la _ = cfg["DATABASE_URL"]
    assert(sann)
}

funksjon test_profil_støtte() {
    la cfg = config.hent_for_profil("testing")
    la db = cfg["DATABASE_URL"]
    assert(db == ":memory:")
}

funksjon test_profilteljing() {
    la aktiv = config.profil()
    assert(aktiv == "development")
}

funksjon test_meldings_flash() {
    la conn = db.open(":memory:")
    la _ = sesjon.migrer(conn)
    la token = sesjon.start_med_ttl(conn, {}, 3600)
    la _ = messages.set_med_token(conn, token, "info", "Velkomen")
    la ved_ctx: ordbok_tekst = {"__sesjon_token__": token}
    la frå_kontekst = messages.hent(ved_ctx, conn)
    assert_eq(frå_kontekst["info"], "Velkomen")
    la _ = sesjon.flash_sett(conn, token, "feil", "Sakna")
    la alt = messages.hent_med_token(conn, token)
    assert(har_nokkel(alt, "feil"))
    la _ = db.close(conn)
}

funksjon test_bruteforce_tvinger_rate_limit() {
    la ctx = web.request_context("POST", "/auth/login", {}, {
        "host": "localhost",
        "x-forwarded-for": "203.0.113.15"
    }, "")
    la ok1 = auth_throttle.kan_logge_inn(ctx, "bruker@example.com")
    la ok2 = auth_throttle.kan_logge_inn(ctx, "bruker@example.com")
    la ok3 = auth_throttle.kan_logge_inn(ctx, "bruker@example.com")
    la ok4 = auth_throttle.kan_logge_inn(ctx, "bruker@example.com")
    assert(ok1)
    assert(ok2)
    assert(ok3)
    assert(!ok4)
}

funksjon start() -> heiltall {
    la testar = [
        "test_innstillingar_lastes",
        "test_profil_støtte",
        "test_profilteljing",
        "test_meldings_flash",
        "test_bruteforce_tvinger_rate_limit"
    ]
    returner test.køyr(testar)
}
EOF_TEST_AUTH

cat > "$OUTPUT_DIR/tests/test_stack_security.no" <<'EOF_TEST_SECURITY'
bruk std.test som test
bruk std.web som web
import app
bruk src.middleware som middleware
bruk src.security som security

funksjon test_blockert_host() {
    la blokkert = security.host_tillatt(web.request_context("GET", "/auth/login", {}, {"host": "evil.test"}, ""))
    assert(!blokkert)
}

funksjon test_security_headers() {
    la svar = app.dispatcher(web.request_context("GET", "/", {}, {"host": "localhost"}, ""))
    la h = web.response_header(svar, "x-content-type-options")
    assert(h == "nosniff")
}

funksjon test_middleware_blockerer_evil_host() {
    la blokkerte_svar = middleware.køyr(web.request_context("GET", "/cache-demo", {}, {"host": "evil.test"}, ""), web.response_builder(200, {"content-type":"text/plain"}, "hei"))
    assert_eq(web.response_status(blokkerte_svar), 403)
}

funksjon test_middleware_stack() {
    la stack = middleware.bygg_stack()
    la funne = usann
    la i: heiltall = 0
    mens i < lengde(stack) {
        la mw = stack[i]
        hvis har_nokkel(mw, "type") og mw["type"] == "sikkerheit" { funne = sann }
        i = i + 1
    }
    assert(funne)
}

funksjon start() -> heiltall {
    la testar = [
        "test_blockert_host",
        "test_security_headers",
        "test_middleware_blockerer_evil_host",
        "test_middleware_stack"
    ]
    returner test.køyr(testar)
}
EOF_TEST_SECURITY

cat > "$OUTPUT_DIR/tests/test_stack_web.no" <<'EOF_TEST_WEB'
bruk std.test som test
bruk std.web som web
import app
bruk src.route_registry som route_registry

funksjon test_route_index_finst() {
    la _ = app.route_index
    assert(sann)
}

funksjon test_route_registry_har_stotte() {
    la ruter = route_registry.index()
    assert(har_nokkel(ruter, "GET /"))
    assert(har_nokkel(ruter, "GET /cache-demo"))
    assert(har_nokkel(ruter, "GET /api/v1/health"))
    assert(har_nokkel(ruter, "POST /api/v1/echo"))
    assert(har_nokkel(ruter, "GET /api/v1/dependency"))
    assert(har_nokkel(ruter, "GET /openapi.json"))
    assert(har_nokkel(ruter, "GET /docs"))
    assert(har_nokkel(ruter, "GET /api/v1/query"))
    assert(har_nokkel(ruter, "POST /api/v1/validate"))
    assert(har_nokkel(ruter, "POST /api/v1/request-model"))
    assert(sann)
}

funksjon test_route_cache_demo() {
    la ctx = web.request_context("GET", "/cache-demo", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(web.response_body(svar), "Cache-demo"))
}

funksjon test_route_i18n_demo() {
    la ctx = web.request_context("GET", "/i18n-demo", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(web.response_body(svar), "Hei, venn!"))
}

funksjon test_route_api_health() {
    la ctx = web.request_context("GET", "/api/v1/health", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"ok\":\"sann\""))
}

funksjon test_route_api_echo() {
    la body = builtin.json_stringify({"melding": "hei", "kontekst": "norscode"})
    la ctx = web.request_context("POST", "/api/v1/echo", {"content-type": "application/json"}, {"host": "localhost"}, body)
    la svar = app.dispatcher(ctx)
    la body_ut = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_ut, "\"melding\":\"hei\""))
}

funksjon test_route_api_echo_feil() {
    la ctx = web.request_context("POST", "/api/v1/echo", {"content-type": "application/json"}, {"host": "localhost"}, "ikkje json")
    la svar = app.dispatcher(ctx)
    la body_ut = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_ut, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_ut, "\"body må vere gyldig JSON-objekt\""))
}

funksjon test_route_api_dependency() {
    la ctx = web.request_context("GET", "/api/v1/dependency", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"path\":\"/api/v1/dependency\""))
    assert(tekst_inneholder(body, "\"method\":\"GET\""))
}

funksjon test_route_api_openapi() {
    la ctx = web.request_context("GET", "/openapi.json", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"openapi\":\"3.0.3\""))
    assert(tekst_inneholder(body, "\"/api/v1/health\""))
    assert(tekst_inneholder(body, "\"/api/v1/response-model\""))
    assert(tekst_inneholder(body, "\"/api/v1/request-model\""))
}

funksjon test_route_api_docs() {
    la ctx = web.request_context("GET", "/docs", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "<h1>"))
    assert(tekst_inneholder(body, "\"openapi\":\"3.0.3\""))
}

funksjon test_route_api_query() {
    la ctx = web.request_context("GET", "/api/v1/query", {"name":"hei"}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"name\":\"hei\""))
}

funksjon test_route_api_query_feil() {
    la ctx = web.request_context("GET", "/api/v1/query", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body, "\"name er påkravd\""))
}

funksjon test_route_api_validate() {
    la body = builtin.json_stringify({"name":"norscode","age":"22"})
    la ctx = web.request_context("POST", "/api/v1/validate", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"name\":\"norscode\""))
    assert(tekst_inneholder(body_res, "\"age\":22"))
}

funksjon test_route_api_validate_feil() {
    la body = builtin.json_stringify({"age":"ikkje_tal"})
    la ctx = web.request_context("POST", "/api/v1/validate", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"age må vere heiltal\""))
}

funksjon test_route_api_payload() {
    la body = builtin.json_stringify({"title":"demo","count":3,"active":sann,"tags":["api","stack"]})
    la ctx = web.request_context("POST", "/api/v1/payload", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"title\":\"demo\""))
    assert(tekst_inneholder(body_res, "\"count\":3"))
    assert(tekst_inneholder(body_res, "\"active\":true"))
    assert(tekst_inneholder(body_res, "\"tags\":[\"api\",\"stack\"]"))
}

funksjon test_route_api_payload_feil() {
    la body = builtin.json_stringify({"count":"abc","active":"ja","tags":{}})
    la ctx = web.request_context("POST", "/api/v1/payload", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"title er påkravd\""))
    assert(tekst_inneholder(body_res, "\"count må vere heiltal\""))
    assert(tekst_inneholder(body_res, "\"active må vere boolsk\""))
    assert(tekst_inneholder(body_res, "\"tags må vere liste\""))
}

funksjon test_route_api_nested() {
    la body = "{\"user\":{\"name\":\"Ola\",\"age\":33},\"meta\":{\"source\":\"cli\",\"ref\":\"v1\"}}"
    la ctx = web.request_context("POST", "/api/v1/nested", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"name\":\"Ola\""))
    assert(tekst_inneholder(body_res, "\"age\":33"))
    assert(tekst_inneholder(body_res, "\"source\":\"cli\""))
    assert(tekst_inneholder(body_res, "\"ref\":\"v1\""))
}

funksjon test_route_api_nested_feil() {
    la body = "{\"user\":{\"age\":\"bad\"},\"meta\":{\"source\":false}}"
    la ctx = web.request_context("POST", "/api/v1/nested", {"content-type":"application/json"}, {"host": "localhost"}, body)
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"name er påkravd\""))
    assert(tekst_inneholder(body_res, "\"age må vere heiltal\""))
    assert(tekst_inneholder(body_res, "\"source må vere tekst\""))
}

funksjon test_route_api_headers() {
    la ctx = web.request_context("GET", "/api/v1/headers", {}, {"host": "localhost", "x-request-id":"abc", "x-count":"3", "x-active":"true"}, "")
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"request_id\":\"abc\""))
    assert(tekst_inneholder(body_res, "\"count\":3"))
    assert(tekst_inneholder(body_res, "\"active\":true"))
}

funksjon test_route_api_headers_feil() {
    la ctx = web.request_context("GET", "/api/v1/headers", {}, {"host": "localhost", "x-request-id":"", "x-count":"tjueto", "x-active":"ja"}, "")
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"x-request-id er påkravd\""))
    assert(tekst_inneholder(body_res, "\"x-count må vere heiltal\""))
    assert(tekst_inneholder(body_res, "\"x-active må vere boolsk\""))
}

funksjon test_route_api_response_model() {
    la ctx = web.request_context("GET", "/api/v1/response-model?mode=good", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"count\":2"))
    assert(tekst_inneholder(body_res, "\"active\":true"))
}

funksjon test_route_api_response_model_feil() {
    la ctx = web.request_context("GET", "/api/v1/response-model?mode=bad", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 500)
    assert(tekst_inneholder(body_res, "\"error\":\"ResponseValidationError\""))
    assert(tekst_inneholder(body_res, "\"count må vere heiltall\""))
}

funksjon test_route_api_request_model() {
    la body = "{\"title\":\"demo\",\"count\":9,\"active\":true,\"tags\":[\"stack\",\"request\"]}"
    la ctx = web.request_context("POST", "/api/v1/request-model", {"content-type":"application/json"}, {"host":"localhost"}, body)
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body_res, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body_res, "\"title\":\"demo\""))
    assert(tekst_inneholder(body_res, "\"count\":9"))
    assert(tekst_inneholder(body_res, "\"active\":true"))
    assert(tekst_inneholder(body_res, "\"tags\":[\"stack\",\"request\"]"))
}

funksjon test_route_api_request_model_feil() {
    la body = "{\"title\":\"\",\"count\":\"na\",\"active\":\"ja\",\"tags\":\"ikkje-liste\"}"
    la ctx = web.request_context("POST", "/api/v1/request-model", {"content-type":"application/json"}, {"host":"localhost"}, body)
    la svar = app.dispatcher(ctx)
    la body_res = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body_res, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body_res, "\"title er påkravd\""))
    assert(tekst_inneholder(body_res, "\"count må vere heiltal\""))
    assert(tekst_inneholder(body_res, "\"active må vere boolsk\""))
    assert(tekst_inneholder(body_res, "\"tags må vere liste\""))
}

funksjon test_route_api_item() {
    la ctx = web.request_context("GET", "/api/v1/items/42", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 200)
    assert(tekst_inneholder(body, "\"ok\":\"sann\""))
    assert(tekst_inneholder(body, "\"id\":42"))
}

funksjon test_route_api_item_feil() {
    la ctx = web.request_context("GET", "/api/v1/items/abc", {}, {"host": "localhost"}, "")
    la svar = app.dispatcher(ctx)
    la body = web.response_body(svar)
    assert_eq(web.response_status(svar), 422)
    assert(tekst_inneholder(body, "\"error\":\"ValidationError\""))
    assert(tekst_inneholder(body, "\"id må vere heiltal\""))
}

funksjon start() -> heiltall {
    la testar = [
        "test_route_index_finst",
        "test_route_registry_har_stotte",
        "test_route_cache_demo",
        "test_route_i18n_demo",
        "test_route_api_health",
        "test_route_api_echo",
        "test_route_api_echo_feil",
        "test_route_api_dependency",
        "test_route_api_openapi",
        "test_route_api_docs",
        "test_route_api_query",
        "test_route_api_query_feil",
        "test_route_api_validate",
        "test_route_api_validate_feil",
        "test_route_api_item",
        "test_route_api_item_feil",
        "test_route_api_payload",
        "test_route_api_payload_feil",
        "test_route_api_nested",
        "test_route_api_nested_feil",
        "test_route_api_headers",
        "test_route_api_headers_feil",
        "test_route_api_response_model",
        "test_route_api_response_model_feil",
        "test_route_api_request_model",
        "test_route_api_request_model_feil"
    ]
    returner test.køyr(testar)
}
EOF_TEST_WEB

cat > "$OUTPUT_DIR/tests/test_stack_cache.no" <<'EOF_TEST_CACHE'
bruk std.test som test
bruk std.web som web
bruk src.cache som cache_mod
bruk src.config som config

funksjon test_cache_config() {
    la cfg = config.hent()
    assert_eq(cfg["CACHE_BACKEND"], "minne")
    assert_eq(cfg["CACHE_DIR"], "cache/")
}

funksjon test_cache_skriv_og_les() {
    la c = cache_mod.ny_minne()
    la _ = cache_mod.slett(c, "stack_cache_testr")
    la satt = cache_mod.sett(c, "stack_cache_testr", "hei", 60)
    assert(satt >= 0)
    la lese = cache_mod.hent(c, "stack_cache_testr", "")
    assert_eq(lese, "hei")
}

funksjon test_cache_svar_cache() {
    la c = cache_mod.ny_minne()
    la respons = web.response_builder(200, {"content-type": "text/plain; charset=utf-8"}, "Hallo cache")
    la _ = cache_mod.set_response(c, "cache_test_svar", respons, 60)
    la henta = cache_mod.hent_response(c, "cache_test_svar")
    assert_eq(web.response_status(henta), 200)
    assert_eq(web.response_body(henta), "Hallo cache")
}

funksjon start() -> heiltall {
    la testar = [
        "test_cache_config",
        "test_cache_skriv_og_les",
        "test_cache_svar_cache"
    ]
    returner test.køyr(testar)
}
EOF_TEST_CACHE

if [ "$WITH_EMAIL" = "sann" ]; then
cat > "$OUTPUT_DIR/tests/test_stack_email.no" <<'EOF_TEST_EMAIL'
bruk std.test som test
bruk src.email som email_mod
bruk src.i18n som i18n

funksjon test_i18n_tolkning_nn() {
    la vising = i18n.t("Welcome")
    assert_eq(vising, "Velkomen")
}

funksjon test_i18n_tolkning_med_kontekst() {
    la vising = i18n.t_ctx("Hello, %(name)s!", {"name": "Ola"})
    assert_eq(vising, "Hei, Ola!")
}

funksjon test_email_send_i_konsollmodus() {
    la vart_sendt = email_mod.send("test@example.com", "Hei frå test", "Dette er ein test")
    assert(vart_sendt)
}

funksjon start() -> heiltall {
    la testar = [
        "test_i18n_tolkning_nn",
        "test_i18n_tolkning_med_kontekst",
        "test_email_send_i_konsollmodus"
    ]
    returner test.køyr(testar)
}
EOF_TEST_EMAIL
fi

cat > "$OUTPUT_DIR/tests/test_stack_static.no" <<'EOF_TEST_STATIC'
bruk std.test som test
bruk std.web som web
bruk src.static som static_mod

funksjon test_static_style_finst() {
    la res = static_mod.svar("/static/style.css")
    assert_eq(web.response_status(res), 200)
    assert(tekst_inneholder(web.response_body(res), "Norscode"))
}

funksjon test_static_ikkje_finst() {
    la res = static_mod.svar("/static/ikkje-finst.css")
    assert_eq(web.response_status(res), 404)
}

funksjon test_static_triks_blokker() {
    la res = static_mod.svar("/static/../seier")
    assert(web.response_status(res) != 200)
}

funksjon start() -> heiltall {
    la testar = [
        "test_static_style_finst",
        "test_static_ikkje_finst",
        "test_static_triks_blokker"
    ]
    returner test.køyr(testar)
}
EOF_TEST_STATIC

cat > "$OUTPUT_DIR/tests/test_stack_orm_query.no" <<'EOF_TEST_STACK_ORM_QUERY'
bruk std.db som db
bruk std.orm som orm
bruk std.test som test
bruk src.models som models

funksjon _lag_test_data() -> tekst {
    la conn = db.open(":memory:")
    la _ = db.execute(conn, "CREATE TABLE produkt (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT, pris INTEGER, kategori TEXT)")
    la _ = db.execute(conn, "INSERT INTO produkt (namn, pris, kategori) VALUES ('alpha', 10, 'A');")
    la _ = db.execute(conn, "INSERT INTO produkt (namn, pris, kategori) VALUES ('beta', 20, 'B');")
    la _ = db.execute(conn, "INSERT INTO produkt (namn, pris, kategori) VALUES ('gamma', 30, 'A');")
    la _ = db.execute(conn, "INSERT INTO produkt (namn, pris, kategori) VALUES ('delta', 40, 'A');")
    returner conn
}

funksjon test_query_filter_order_limit() {
    la conn = _lag_test_data()
    la q = orm.query_set(conn, "produkt")
    la filtrert = orm.filter(q, "kategori = 'A'")
    la sortert = orm.order_by(filtrert, "pris", "DESC")
    la avgrensa = orm.limit(sortert, 2)
    la r = orm.all(avgrensa)
    assert_eq(lengde(r), 2)
    assert_eq(r[0]["namn"], "delta")
    assert_eq(r[1]["namn"], "gamma")
    la _ = db.close(conn)
}

funksjon _lag_relasjon_data() -> tekst {
    la conn = db.open(":memory:")
    la _ = db.execute(conn, "PRAGMA foreign_keys = ON;")
    la _ = db.execute(conn, "CREATE TABLE kategori (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT)")
    la _ = db.execute(conn, "INSERT INTO kategori (namn) VALUES ('Elektronikk');")
    la _ = db.execute(conn, "INSERT INTO kategori (namn) VALUES ('Kontor');")
    la _ = db.execute(conn, "CREATE TABLE produkt_fk (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT, kategori_id INTEGER, FOREIGN KEY(kategori_id) REFERENCES kategori(id))")
    la _ = db.execute(conn, "INSERT INTO produkt_fk (namn, kategori_id) VALUES ('mus', 1);")
    la _ = db.execute(conn, "INSERT INTO produkt_fk (namn, kategori_id) VALUES ('stol', 2);")
    returner conn
}

funksjon test_query_join_fk() {
    la conn = _lag_relasjon_data()
    la q = orm.select(orm.query_set(conn, "produkt_fk"), "produkt_fk.namn, k.namn AS kategori_namn")
    la q2 = orm.join(q, "kategori", "kategori_id", "id", "k")
    la r = orm.all(q2)
    assert_eq(lengde(r), 2)
    assert_eq(r[0]["kategori_namn"], "Elektronikk")
    la _ = db.close(conn)
}

funksjon _lag_many_to_many_data() -> tekst {
    la conn = db.open(":memory:")
    la _ = db.execute(conn, "PRAGMA foreign_keys = ON;")
    la _ = db.execute(conn, "CREATE TABLE rolle (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT)")
    la _ = db.execute(conn, "CREATE TABLE bruker (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT)")
    la _ = db.execute(conn, "CREATE TABLE bruker_rolle (id INTEGER PRIMARY KEY AUTOINCREMENT, bruker_id INTEGER, rolle_id INTEGER, FOREIGN KEY(bruker_id) REFERENCES bruker(id), FOREIGN KEY(rolle_id) REFERENCES rolle(id))")
    la _ = db.execute(conn, "INSERT INTO rolle (namn) VALUES ('admin');")
    la _ = db.execute(conn, "INSERT INTO rolle (namn) VALUES ('editor');")
    la _ = db.execute(conn, "INSERT INTO bruker (namn) VALUES ('Ola');")
    la _ = db.execute(conn, "INSERT INTO bruker_rolle (bruker_id, rolle_id) VALUES (1,1);")
    la _ = db.execute(conn, "INSERT INTO bruker_rolle (bruker_id, rolle_id) VALUES (1,2);")
    la _ = db.execute(conn, "INSERT INTO bruker_rolle (bruker_id, rolle_id) VALUES (2,2);")
    returner conn
}

funksjon test_scaffold_many_to_many_manifest() {
    la m2m = models.many_to_many_tabellar()
    assert_eq(lengde(m2m), 1)
    assert_eq(m2m[0]["table"], "produkt_tag")
    la modellar = models.modeller()
    la funne = usann
    la i: heiltall = 0
    mens i < lengde(modellar) {
        hvis modellar[i]["table"] == "produkt_tag" {
            funne = sann
        }
        i = i + 1
    }
    assert(funne)
}

funksjon test_scaffold_many_to_many_hjelpar() {
    la conn = db.open(":memory:")
    la _ = models.ensure_schema(conn)
    la _ = db.execute(conn, "INSERT INTO kategori (namn) VALUES ('Demo');")
    la _ = db.execute(conn, "INSERT INTO tag (namn) VALUES ('ny');")
    la _ = db.execute(conn, "INSERT INTO produkt (namn, pris, lager, kategori_id) VALUES ('produkt-1', 10, 1, 1);")
    la _ = db.execute(conn, "INSERT INTO produkt_tag (produkt_id, tag_id) VALUES (1, 1);")
    la tag = models.tags_for_produkt(conn, 1)
    assert_eq(lengde(tag), 1)
    assert_eq(tag[0]["namn"], "ny")
    la produkter = models.produkt_for_tag(conn, 1)
    assert_eq(lengde(produkter), 1)
    assert_eq(produkter[0]["namn"], "produkt-1")
    la _ = db.close(conn)
}

funksjon _lag_aggregate_data() -> tekst {
    la conn = db.open(":memory:")
    la _ = db.execute(conn, "CREATE TABLE salg (id INTEGER PRIMARY KEY AUTOINCREMENT, brukarkategori TEXT, belop INTEGER)")
    la _ = db.execute(conn, "INSERT INTO salg (brukarkategori, belop) VALUES ('bronse', 100);")
    la _ = db.execute(conn, "INSERT INTO salg (brukarkategori, belop) VALUES ('bronse', 50);")
    la _ = db.execute(conn, "INSERT INTO salg (brukarkategori, belop) VALUES ('sølv', 75);")
    la _ = db.execute(conn, "INSERT INTO salg (brukarkategori, belop) VALUES ('sølv', 25);")
    la _ = db.execute(conn, "INSERT INTO salg (brukarkategori, belop) VALUES ('gull', 300);")
    returner conn
}

funksjon test_query_many_to_many_join() {
    la conn = _lag_many_to_many_data()
    la q = orm.select(orm.query_set(conn, "bruker"), "bruker.namn AS bruker_namn, r.namn AS rolle_namn")
    la q2 = orm.join_many_to_many(q, "bruker_rolle", "bruker_id", "rolle_id", "rolle", "id", "r", "")
    la rader = orm.all(q2)
    assert_eq(lengde(rader), 3)
    la funnet_admin = usann
    la i: heiltall = 0
    mens i < lengde(rader) {
        hvis rader[i]["rolle_namn"] == "admin" {
            funnet_admin = sann
        }
        i = i + 1
    }
    assert(funnet_admin)
    la _ = db.close(conn)
}

funksjon test_many_to_many_reverser() {
    la conn = _lag_many_to_many_data()
    la roles = orm.reverser_many_to_many(conn, "bruker_rolle", "rolle", "bruker_id", 1, "rolle_id", "id", "r")
    assert_eq(lengde(roles), 2)
    la _ = db.close(conn)
}

funksjon test_query_aggregate_scalar() {
    la conn = _lag_aggregate_data()
    la totalt = orm.aggregate_count(orm.query_set(conn, "salg"), "id")
    la summering = orm.aggregate_sum(orm.query_set(conn, "salg"), "belop")
    assert_eq(totalt, "5")
    assert_eq(summering, "550")
    la _ = db.close(conn)
}

funksjon test_query_group_by_annotering() {
    la conn = _lag_aggregate_data()
    la q = orm.select(orm.query_set(conn, "salg"), "brukarkategori")
    la q2 = orm.group_by(q, "brukarkategori")
    la q3 = orm.annotate(q2, "SUM(belop)", "total")
    la grupper = orm.all(q3)
    assert_eq(lengde(grupper), 3)
    la funnet_bronse = usann
    la i: heiltall = 0
    mens i < lengde(grupper) {
        hvis grupper[i]["brukarkategori"] == "bronse" {
            assert_eq(grupper[i]["total"], "150")
            funnet_bronse = sann
        }
        i = i + 1
    }
    assert(funnet_bronse)
    la _ = db.close(conn)
}

funksjon _lag_savepoint_data() -> tekst {
    la conn = db.open(":memory:")
    la _ = db.execute(conn, "CREATE TABLE konto (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT);")
    la _ = db.execute(conn, "INSERT INTO konto (namn) VALUES ('start');")
    returner conn
}

funksjon _lag_index_data() -> tekst {
    la conn = db.open(":memory:")
    la _ = db.execute(conn, "CREATE TABLE produkt_meta (id INTEGER PRIMARY KEY AUTOINCREMENT, namn TEXT, verdi INTEGER);")
    la _ = db.execute(conn, "INSERT INTO produkt_meta (namn, verdi) VALUES ('a', 1);")
    la _ = db.execute(conn, "INSERT INTO produkt_meta (namn, verdi) VALUES ('b', 2);")
    returner conn
}

funksjon test_transaksjon_savepoints() {
    la conn = _lag_savepoint_data()
    assert_eq(orm.tell(conn, "konto"), 1)
    la _ = orm.transaksjon_start(conn)
    la _ = orm.transaksjon_start_savepoint(conn, "punkt_a")
    la _ = db.execute(conn, "INSERT INTO konto (namn) VALUES ('midlertidig');")
    la _ = orm.transaksjon_rull_til_savepoint(conn, "punkt_a")
    assert_eq(orm.tell(conn, "konto"), 1)
    la _ = db.execute(conn, "INSERT INTO konto (namn) VALUES ('ny');")
    la _ = orm.transaksjon_release_savepoint(conn, "punkt_a")
    la _ = orm.transaksjon_bekreft(conn)
    assert_eq(orm.tell(conn, "konto"), 2)
    la _ = db.close(conn)
}

funksjon test_orm_indekser() {
    la conn = _lag_index_data()
    la idx_namn = orm.indeks_namn("produkt_meta", "namn", usann)
    assert(orm.legg_indeks(conn, "produkt_meta", "namn", usann, ""))
    assert(orm.indeks_finnes(conn, idx_namn))
    la indeks_liste = orm.indeks_for_tabell(conn, "produkt_meta")
    assert(lengde(indeks_liste) >= 1)
    assert(orm.slett_indeks(conn, idx_namn))
    assert(!orm.indeks_finnes(conn, idx_namn))
    la _ = db.close(conn)
}

funksjon test_orm_constraint_help() {
    la fk = orm.constraint_fk_sql("kategori_id", "kategori", "id")
    la uniq = orm.constraint_unique_sql("kategori_id, namn")
    la check = orm.constraint_check_sql("pris >= 0")
    assert_eq(fk, "FOREIGN KEY(kategori_id) REFERENCES kategori(id)")
    assert_eq(uniq, "UNIQUE(kategori_id, namn)")
    assert_eq(check, "CHECK(pris >= 0)")
    assert_eq(orm.constraint_check_sql("   "), "")
}

funksjon test_fk_hjelp() {
    la conn = _lag_relasjon_data()
    la rel = orm.hent_knytteobjekt(conn, "produkt_fk", 1, "kategori_id", "kategori")
    assert(har_nokkel(rel, "namn"))
    la rel2 = orm.reverser_knyting(conn, "produkt_fk", "kategori_id", "kategori", 2)
    la _ = db.close(conn)
    assert(lengde(rel2) > 0)
}

funksjon test_query_exclude_offset() {
    la conn = _lag_test_data()
    la q = orm.exclude(orm.filter_eq(orm.query_set(conn, "produkt"), "namn", "alpha"), "pris = 10")
    la q2 = orm.offset(orm.order_by(q, "pris", "ASC"), 1)
    la q3 = orm.limit(q2, 2)
    la r = orm.all(q3)
    assert_eq(lengde(r), 2)
    assert_eq(r[0]["namn"], "beta")
    assert_eq(r[1]["namn"], "gamma")
    la _ = db.close(conn)
}

funksjon test_query_first_page_count_get() {
    la conn = _lag_test_data()
    la q = orm.order_by(orm.query_set(conn, "produkt"), "pris", "ASC")
    la første = orm.first(q)
    assert_eq(første["namn"], "alpha")
    la page = orm.page(orm.query_set(conn, "produkt"), 2, 2)
    la sider = orm.page_result(page)
    assert_eq(lengde(sider), 2)
    assert_eq(sider[0]["namn"], "gamma")
    assert_eq(orm.count(orm.query_set(conn, "produkt")), 4)
    la delta = orm.get(orm.filter_eq(orm.query_set(conn, "produkt"), "namn", "delta"))
    assert_eq(delta["pris"], "40")
    la tom = orm.get(orm.filter_eq(orm.query_set(conn, "produkt"), "namn", "manglar"))
    assert(ikke har_nokkel(tom, "id"))
    la _ = db.close(conn)
}

funksjon start() -> heiltall {
    la testar = [
        "test_query_filter_order_limit",
        "test_query_exclude_offset",
        "test_query_join_fk",
        "test_fk_hjelp",
        "test_scaffold_many_to_many_manifest",
        "test_scaffold_many_to_many_hjelpar",
        "test_query_many_to_many_join",
        "test_many_to_many_reverser",
        "test_query_aggregate_scalar",
        "test_query_group_by_annotering",
        "test_transaksjon_savepoints",
        "test_orm_indekser",
        "test_orm_constraint_help",
        "test_query_first_page_count_get"
    ]
    returner test.køyr(testar)
}
EOF_TEST_STACK_ORM_QUERY
if [ "$WITH_ADMIN" = "sann" ]; then
cat > "$OUTPUT_DIR/tests/test_stack_admin.no" <<'EOF_TEST_ADMIN'
bruk std.test som test
bruk src.admin_bootstrap som admin_bootstrap
bruk src.admin.index som admin_index

funksjon test_registrering() {
    la register = {}
    la _ = admin_bootstrap.registrer_modellar(register, "dummy")
    assert(har_nokkel(register, "produkt"))
}

funksjon test_admin_ruter() {
    la ruter = admin_index.routes()
    assert(har_nokkel(ruter, "GET /admin"))
    assert(har_nokkel(ruter, "GET /admin/{tabell}"))
    assert(har_nokkel(ruter, "POST /admin/{tabell}/lagre"))
    assert(har_nokkel(ruter, "POST /admin/{tabell}/{id}/slett"))
}

funksjon start() -> heiltall {
    la testar = [
        "test_registrering",
        "test_admin_ruter"
    ]
    returner test.køyr(testar)
}
EOF_TEST_ADMIN
fi

cat > "$OUTPUT_DIR/tests/payloads/login.json" <<'EOF_PAY_LOGIN'
{
  "epost": "admin@eksempel.no",
  "passord": "hemmelig"
}
EOF_PAY_LOGIN

cat > "$OUTPUT_DIR/tests/payloads/register.json" <<'EOF_PAY_REGISTER'
{
  "brukarnamn": "admin",
  "epost": "admin@eksempel.no",
  "passord": "hemmelig"
}
EOF_PAY_REGISTER

cat > "$OUTPUT_DIR/tests/payloads/api_echo.json" <<'EOF_PAY_API_ECHO'
{
  "melding": "Hei frå prosjekt"
}
EOF_PAY_API_ECHO

cat > "$OUTPUT_DIR/tests/payloads/api_payload.json" <<'EOF_PAY_API_PAYLOAD'
{
  "title": "Demo",
  "count": 10,
  "active": true,
  "tags": ["project", "api"]
}
EOF_PAY_API_PAYLOAD

cat > "$OUTPUT_DIR/tests/payloads/api_nested.json" <<'EOF_PAY_API_NESTED'
{
  "user": {
    "name": "Ola",
    "age": 33
  },
  "meta": {
    "source": "stack",
    "ref": "v1"
  }
}
EOF_PAY_API_NESTED

cat > "$OUTPUT_DIR/tests/payloads/create_product.json" <<'EOF_PAY_PRODUCT'
{
  "model": "produkt",
  "fields": {
  "namn": "Starter",
  "pris": 199,
  "lager": 10
  }
}
EOF_PAY_PRODUCT

cat > "$OUTPUT_DIR/tests/payloads/create_kategori.json" <<'EOF_PAY_KATEGORI'
{
  "model": "kategori",
  "fields": {
    "namn": "Demo-kategori"
  }
}
EOF_PAY_KATEGORI

cat > "$OUTPUT_DIR/tests/payloads/create_tag.json" <<'EOF_PAY_TAG'
{
  "model": "tag",
  "fields": {
    "namn": "Kampanje"
  }
}
EOF_PAY_TAG

cat > "$OUTPUT_DIR/tests/payloads/create_produkt_tag.json" <<'EOF_PAY_PRODUKT_TAG'
{
  "model": "produkt_tag",
  "fields": {
    "produkt_id": 1,
    "tag_id": 1
  }
}
EOF_PAY_PRODUKT_TAG

cat > "$OUTPUT_DIR/static/style.css" <<'EOF_STATIC_STYLE'
:root {
  font-family: system-ui, -apple-system, Segoe UI, Roboto, sans-serif;
}
body {
  margin: 0;
}
EOF_STATIC_STYLE

cat > "$OUTPUT_DIR/static/media/example.txt" <<'EOF_STATIC_MEDIA'
Norscode-static-eksempel.
Denne fila er ei demo av media-katalogen i prosjektet.
EOF_STATIC_MEDIA

cat > "$OUTPUT_DIR/locale/nn.lang" <<'EOF_LOCALE_NN'
"Welcome" = "Velkomen"
"Home page" = "Hovudside"
"Hello, %(name)s!" = "Hei, %(name)s!"
EOF_LOCALE_NN

cat > "$OUTPUT_DIR/locale/en.lang" <<'EOF_LOCALE_EN'
"Welcome" = "Welcome"
"Home page" = "Home page"
"Hello, %(name)s!" = "Hello, %(name)s!"
EOF_LOCALE_EN

cat > "$OUTPUT_DIR/examples/ping.no" <<'EOF_EX'
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

cat > "$OUTPUT_DIR/docs/lifecycle.md" <<'EOF_LIFECYCLE'
# Deploy lifecycle og drift i dette prosjektet

Dette dokumentet beskriv ein enkel deploy-flyt du kan bruke frå dagleg utvikling til produksjon.

## 1) Klargjering før release

- Oppdater avhengigheiter i `innstillingar.toml` ved behov.
- Køyre validering:
  - `./bin/nc check app.no`
  - `./bin/nc check manage.no`
  - `./bin/nc run manage.no migrate`
- Køyre testar:
  - `./bin/nc run manage.no test`
- Valider at migrasjonane er rydda:
  - `./bin/nc run manage.no status`

## 2) Build/prepare

- Vel produksjonsprofil: `NORSCODE_PROFILE=production`.
- Sikre at `deploy/norscode.service` har rett `WorkingDirectory` og `ExecStart`.
- Bygg artefakt i din CI/CD eller pakk repo-state etter ønskjeleg praksis.

## 3) Deploy

- Last opp/oppdater prosjektet på målmiljø.
- Køyre migrasjonar på målmiljø før appstart om nødvendig.
- Start tenesta og sjekk helsestatus.

Standard startkommando:

```sh
./bin/nc run app.no
```

Med systemd:

```sh
systemctl start norscode
systemctl status norscode
```

## 4) Overvaking etter deploy

- Følg loggar frå tjeneste eller runtime.
- Ver observant på autentiserings- og rate-limit-statistikk.
- Sjekk `docs/deploy-log.md` etter kvar deploy med versjons- og rollback-notat.

## 5) Rollback

- Dersom deploy gir regresjon:
  - steng ny versjon,
  - reverter kode eller bytt tilbake siste kjente gode revisjon,
  - køyr migrasjonar som trengst for å rulle tilbake eller justere datastrukturar.
- Logg hendinga i `docs/deploy-log.md` som `rollback`.

## 6) Vedlikehald

- Oppdater `docs/deploy-log.md` ved kvart publiseringssteg.
- Rydd eventuelle `cache`- og temp-filer.
- Gjenta testløpet før neste deploy.

## 7) Oppgåver før neste release

- Fullfør nye migrasjonar i `migrations/`.
- Oppdater `tests/payloads/` og `tests/` ved API-endringar.
- Oppdater denne livssyklusen om flyten endrar seg.
EOF_LIFECYCLE

cat > "$OUTPUT_DIR/docs/deploy-log.md" <<'EOF_DEPLOY_LOG'
# Deploy-logg (manuell)

Bruk denne fila som ei enkel historikk over utgjevingar og deploy-hendingar.

| Dato | Versjon | Kva | Status | Utført av | Kommentar |
| --- | --- | --- | --- | --- | --- |
| 2026-01-01 | 0.1.0 | Initial scaffold | fullført | team | Oppretta prosjekt frå `startproject`. |

## Ny loggrad

- `Dato`: ISO-dato (til dømes `2026-01-01`)
- `Versjon`: prosjektversjon eller commit/tag
- `Kva`: kort omtale (`deploy`, `rollback`, `hotfix`)
- `Status`: `fullført`, `feila`, `rullback`
- `Utført av`: namn/e-post
- `Kommentar`: kva som vart gjort

Legg til ei ny rad for kvar deploy.
EOF_DEPLOY_LOG

{
printf '# %s\n\n' "$PROJECT_NAME"
cat <<'EOF_README'
Dette er eit Django-inspirert Norscode prosjektoppsett med auth, migrations, admin og testmalar.

Struktur:

- `app.no` — hovud app-innsalg.
- `manage.no` — management-kommandoar (`migrate`, `status`).
- `src/` — eigenlogikk (modellar, ruter, visingar og auth).
- `src/route_registry.no` — samlen app-route registry for dispatch.
- `tests/` — testmalar for stack.
- `tests/payloads/` — eksempel JSON-payloadar.
- `apps/` — app-modularisering.
- `docs/` — deploy-historikk og lifecycle-dokumentasjon.
- `innstillingar.toml` — innstillingar.

Kommandoar:

- `cd ${PROJECT_DIR}`
- `./bin/nc check app.no`
- `./bin/nc check manage.no`
- `./bin/nc run manage.no migrate`
- `./bin/nc run manage.no status`
- `./bin/nc run manage.no test`
- `./bin/nc serve app.no --port 8080`
- `./bin/nc startapp users`

Dokumentasjon i prosjektet:

- `docs/lifecycle.md` — deploy-livssyklus frå prep og rollback.
- `docs/deploy-log.md` — manuell deploy-historikk.
- `deploy/norscode.service` — eksempel systemd-service for produksjonskøyring.
- `deploy/` — mappestruktur for operasjonelle deployment-filer.

Test-kommandoar:

- `./bin/nc run manage.no test --db :memory: --fixtures create_product`
  - Laster `tests/payloads/create_product.json` som fixture før test.
  - Testane køyrer via `start()`-metodar i kvar `test_*.no`.

Profil/handsaming av innstillingar:

- Standardprofil blir henta frå `PROFILE` i `innstillingar.toml` (globalt sett `development`).
- Miljøvariabel `NORSCODE_PROFILE` (`development`, `testing`, `production`) kan overstyre profilen ved køyring.
- Spesifikk profil i kode: `config.hent_for_profil("testing")`.
- Overstyring av database for testmiljø: set `DATABASE_URL = ":memory:"` i `[testing]`.
- Køyre med produksjonsprofil:
  - `NORSCODE_PROFILE=production ./bin/nc run app.no`

Auth-API:

- `GET /auth/login`
- `POST /auth/login`
- `GET /auth/register`
- `POST /auth/register`
- `POST /auth/logout`
- `GET /profile`
- `GET /openapi.json`
- `GET /docs`
- `GET /api/v1/health`
- `POST /api/v1/echo`
- `GET /api/v1/dependency`
- `GET /api/v1/query`
- `POST /api/v1/validate`
- `GET /api/v1/items/{id}`
- `POST /api/v1/payload`
- `POST /api/v1/nested`
- `GET /api/v1/headers`
- `GET /api/v1/response-model`
- `POST /api/v1/request-model`
- `GET /static/{sti}` (filer frå `static/`)

Sikkerheit:

- `src/middleware.no` køyrer standard pipeline for security/request-logg-rate-limit.
- `src/security.no` legg til standard security headers på alle svar:
  - `X-Content-Type-Options`
  - `X-XSS-Protection`
  - `Strict-Transport-Security`
  - `X-Frame-Options`
  - `Content-Security-Policy`
- Host-kontroll er aktivert mot `ALLOWED_HOSTS` i `innstillingar.toml`.
- Brute-force-vern på login:
  - `src/auth/throttle.no` tek maks forsøk per IP/brukar innan eit vindauge.
  - Ved for mange feil blir `/auth/login` blokkert med `HTTP 429`.
- Konfigurer med:
  - `BRUTEFORCE_LOGIN_ENABLED`
  - `BRUTEFORCE_LOGIN_MAX`
  - `BRUTEFORCE_LOGIN_WINDOW_SEK`
  - `BRUTEFORCE_LOGIN_KEY` (`ip` eller `brukar`)
EOF_README
} > "$OUTPUT_DIR/README.md"

printf 'Oppretta nytt prosjekt: %s\n' "$OUTPUT_DIR"
printf 'Prosjektnamn: %s\n' "$PROJECT_NAME"
printf 'Neste steg: cd %s && ./bin/nc check app.no\n' "$OUTPUT_DIR"
