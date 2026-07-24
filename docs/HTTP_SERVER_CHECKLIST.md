# Norscode HTTP-server - statusoversikt

Dette dokumentet viser kva som er ferdig, kva som er blokkert, og kva som framleis gjenstår for HTTP-serverdelen.

## Ferdig

- [x] Innebygd HTTP-serverflate finst i standardbiblioteket
- [x] `serve`-kommando finst i CLI/runtime
- [x] Request-parsing finst for metode, sti, query, headers og body
- [x] Routing finst for faste ruter og enkle path-parametrar
- [x] Enkle respons-hjelparar finst
- [x] `serve` kan no bundle ei NCB-app og svare `GET /` med HTML i fake-request-løypa
- [x] Ekte native socket-server er verifisert i runtimeen via `selfhost/http_server_native.no`
- [x] Ekte socket-lytting i full `serve`-løype er verifisert i lokalt miljø
- [x] Bundling av hjemmeside-rutene er verifisert frå repo-roten med lokal `frontend/`-bro
- [x] Deploy-/serverdokumentasjon finst
- [x] Standard `/helse`-respons finst i `serve_handle_one(...)`
- [x] Standard loggkontekst finst via `standard_server_logg(...)`
- [x] Request-middleware kan køyrast før route-dispatch via `anvend_request_middlewares(...)`
- [x] Response-middleware kan køyrast på `/helse`-svaret via `anvend_response_middlewares(...)`
- [x] Error-middleware kan køyrast på 403/404/405/412-svar via `anvend_error_middlewares(...)`
- [x] Startup-/shutdown-hooks kan køyrast via `køyre_livssyklus_hooks(...)`
- [x] Hook-feil kan fangast via `køyre_livssyklus_hooks_trygt(...)`
- [x] Logging og helseendepunkt er samla i ei standard grunnløype
  - selfhost-løypa har standard `/helse` og standard loggkontekst
  - `std.httpserver` og `std.httpserver_vm` har standard helse-/logg-helperar og standard registrering av `/helse` og `/health`
- [x] Hook-flyta er integrert og dekt av grunnleggjande servernivåtestar
  - selfhost har samla pipeline-test for startup, request, response, error og shutdown
  - std-flatene har eigne samla pipeline-testar for same grunnflyt
- [x] Testar dekkjer no start, route, feil, shutdown og reload på grunnnivå
  - selfhost har samla pipeline-test og reload-helper-test
  - std-flatene har eigne samla pipeline-testar for startup, request, response, error og shutdown
- [x] `std.httpserver` og `std.httpserver_vm` har no ei testbar grunnflate for dispatch og responsprofilar
  - metadata for request/response/error-middleware og startup/shutdown-hooks er på plass
  - request/response-bridge mot `std.web` er på plass
  - standard helse-/logg-helperar og standard `/helse`-/`/health`-registrering er på plass
  - std-dispatch dekkjer no grunnleggjande `204`, `304`, `412`, `HEAD`, `405`, `OPTIONS`, `Allow`, enkel caching/preconditions og registrerte responsprofilar
  - std-dispatch har no små helpers og testkantar for dei vanlege statusmønstra, mellom anna `201`, `202`, `204`, `301`, `304`, `307`, `400`-`504`, `404`, `405`, `OPTIONS` og `HEAD`
  - std-flata har no ein intern felles bygger for standardfeil og ein samla byggjar for standard responsløype, og dei spesifikke helperane er tynne variantar oppå den
  - middleware-/hook-trace er samla i ein intern felles byggjar, og dei offentlege helperane er tynne variantar oppå den
  - startup-/shutdown-hook-trace er samla i ein intern felles byggjar, og dei offentlege hook-helperane er tynne variantar oppå den
  - selfhost-løypa har no samla trace-byggjarar for middleware og hooks, slik at den direkte serverkonteksten og std-flata er meir lik i struktur
  - selfhost-løypa har no samla byggjarar for standard feilrespons og `Allow`-lister i route-dispatchen
  - selfhost-løypa har no samla header-oppslag for cache/precondition-stiane i route-dispatchen
  - selfhost-løypa har no samla avgjerdslogikk for standard handler-responsar etter dispatch
  - selfhost-løypa har no samla helsebyggjar og loggkopling i `/helse`-/`/health`-ruta
  - sjølve helse-route-sjekken i selfhost er trekt ut i ein liten hjelpefunksjon
  - sjølve route-dispatchen i selfhost er trekt ut i ein eigen hjelpefunksjon
  - std-flatene kan no bruke registrert tekstbody, registrert direkte responsobjekt, registrert responsobjekt og registrerte ekstra headers per handler
  - std-flatene har no også ei samla server-syklushelper for startup, request, response og shutdown i same kall
  - std-flatene har no ein samla `register_standard_server_components(...)`-helper for helse, middleware og hooks
- [x] `response_to_http(...)` støttar no:
  - `Content-Type`
  - `Allow`
  - `Cache-Control`
  - `Location`
  - `Set-Cookie`
  - `X-Content-Type-Options`
  - `Content-Security-Policy`
  - `Content-Security-Policy-Report-Only`
  - `Content-Disposition`
  - `ETag`
  - `Access-Control-Allow-Origin`
  - `Vary`
  - `Referrer-Policy`
  - `Permissions-Policy`
  - `Strict-Transport-Security`
  - `X-Frame-Options`
  - `Cross-Origin-Opener-Policy`
  - `Cross-Origin-Opener-Policy-Report-Only`
  - `Cross-Origin-Resource-Policy`
  - `Cross-Origin-Resource-Policy-Report-Only`
  - `Cross-Origin-Embedder-Policy`
  - `Cross-Origin-Embedder-Policy-Report-Only`
  - `Link`
  - `Retry-After`
  - `Server`
  - `Last-Modified`
  - `Accept-Ranges`
  - `Expires`
  - `Content-Language`
  - `Content-Encoding`
  - `Transfer-Encoding`
  - `Pragma`
  - `Connection`
  - `WWW-Authenticate`
  - `Proxy-Authenticate`
  - `Clear-Site-Data`
  - `Refresh`
  - `Timing-Allow-Origin`
  - `X-Permitted-Cross-Domain-Policies`

## Blokkert

- [x] Ingen aktive kodeblokkerarar er stadfesta akkurat no
  - siste lokale verifisering viste at `selfhost/http_server_native.no` og full `./bin/nc serve ...` begge kan lytte via socket
  - tidlegare `permission denied` ser ut til å ha vore miljø- eller kontekstspesifikk og er ikkje den aktive statusen lenger

## Gjenstår

- [ ] Breiare målmiljø-verifisering står att
  - lokal socket-lytting, TLS-modul, statusmatrise og middleware-parity er verifisert
  - neste steg er same løype i fleire miljø enn berre lokal maskin
- [ ] Eventuell vidare hardening kan framleis gjerast
  - feilhåndtering, observabilitet og operativ robustheit er god, men kan styrkast vidare ved behov

## Viktig notat

Importoppløysing for `frontend/...` er no mykje meir robust, og repoet har ei lokal `frontend/`-bro som lar bundleren finne nettsidemodulane frå Norscode-roten. Ekte socket-lytting er verifisert lokalt i full `serve`-løype, TLS/HTTPS er tilgjengeleg via `std.tls_http`, og driftoppsettet er dokumentert. Det som no står att er hovudsakleg breiare målmiljø-verifisering og eventuell vidare hardening.

## Arbeid vidare frå no

1. Verifisere serverløypa i fleire målmiljø enn berre lokal maskin
2. Finjustere operativ hardening ved behov
