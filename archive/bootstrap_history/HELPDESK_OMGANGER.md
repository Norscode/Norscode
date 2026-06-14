# Helpdesk: Omgangsmønster og avkrysningsliste

Oppdatert: 2026-06-12  
Omfang: `examples/helpdesk.no` og `docs/helpdesk_api_smoke.http`

## Omgang 0 — Gjeldende status

- [x] API-kjerne (`examples/helpdesk.no`) står i en fungerende web/API-form.
- [x] `./bin/nc check examples/helpdesk.no` går gjennom.
- [x] `GET /tickets` har:
  - filter (`status`, `assignee`, `created_by`, `q`)
  - sortering (`priority`, `id`, `created_at`, `updated_at`, hver med asc/desc)
  - paginering (`limit`, `offset`)
  - validering for ugyldig input.
- [x] `GET /tickets/my` har:
  - filter (`assigned`, `unassigned`)
  - statusfilter (`open`, `in_progress`, `closed`)
  - sortering (`id`, `created_at`, `updated_at`, asc/desc)
  - paginering med validering.
- [x] Audit-lignende historikk for viktige mutasjoner er i stede.
- [x] `docs/helpdesk_api_smoke.http` oppdatert med positive og negative cases.
- [x] Strengere pagineringsvalidering:
  - ugyldig `limit`/`offset` gir 400.
- [x] Baseline-tekstadferd oppdatert for tom/ugyldige filtre og søk.

## Omgang 1 — Adminpanel + tilgangskontroll (mangler)

- [x] Definer og implementer tydelig rollemodell (RBAC):
  - [x] roller: `user`, `agent`, `admin`
  - [x] tillatelser: lesing, endring, administrasjon
  - [x] normaliserte guard-funksjoner for rolle/roller.
- [x] Beskytt admin-endepunkter med eksplisitt adgangskontroll:
  - [x] 401 for manglende autentisering
  - [x] 403 for feil rolle/tilgang.
- [x] Legg inn admin-endepunkter:
  - [x] `GET /admin/users` (liste)
  - [x] `GET /admin/roles` (roller/rettigheter)
  - [x] `POST /admin/users` (rolleoppdatering)
  - [x] `PATCH /admin/tickets/{id}` (admin-overstyring)
  - [x] `DELETE /admin/tickets/{id}` (hard delete)
  - [x] `GET /admin/audit` (hendelseslogg)
- [x] `GET /admin/metrics` (enkle nøkkeltall)
- [x] Datastruktur og migrasjon:
  - [x] lagt inn administrativ bruker/rolle-seksjon i lagring
  - [x] lagt inn audit-logg-objekter med `actor`, `action`, `resource`, `timestamp`, `meta`
  - [x] bakoverkompatibilitet fra tidligere lagring.
- [x] Konsistent feilformat (helt på hele API, inkludert admin):
  - [x] introdusert felles feilrespons-helper ( `error`, `message`, `code`, `status` )
  - [x] brukt gjennomgående på alle endpoint-feil i hele API-et.
- [x] Dokumentasjon og smoke:
  - [x] legg til admin-tilgangsscenarioer i `docs/helpdesk_api_smoke.http`
  - [x] 200-happy-path for admin
  - [x] avvisning for underprivilegerte roller
  - [x] test av rolleendring og audit-synlighet
- [x] Frontend planlegging for admin:
  - [x] avklares adminpanel som separat rute (`/admin/panel`)
  - [x] tydelig og enkel visning med nøkkelstatistikk
  - [x] bruker-liste og siste audit-events i admin UI
  - [x] admin- tilgangssperre for UI-rute (`/admin/panel`) med 401/403.

## Omgang 2 — Kvalitetssikring (etter Omgang 1)

- [ ] Kjør faktiske runtime-smoke mot server:
  - [x] `./bin/nc serve examples/helpdesk.no` (kommandoen er no noverande med native selfhost-statusmelding)
  - [x] oppdatert `tools/smoke_helpdesk_http.sh` med utvida verifisering av viktige /admin og kanter for når serve er tilgjengeleg
  - [ ] verifiser responser mot `docs/helpdesk_api_smoke.http`.
  - [x] Prøvd akkurat no: `./bin/nc serve examples/helpdesk.no` returnerer:
    - `nc serve er enno ikkje fullført i native selfhost-kjeden.`
    - `App-fil: examples/helpdesk.no`
    - `Ønska port: 4173`
    - `For no: bruk ./bin/nc run examples/helpdesk.no for å verifisere appen direkte.`
  - [x] `./tools/smoke_helpdesk_http.sh` kan ikkje verektast enno: serveren får ikkje starta.
- [x] Kjør in-prosess helpdesk-smoke via unit-testkjøring:
  - [x] `./bin/nc test tests/test_helpdesk.no` (`tools/smoke_helpdesk_api.sh`).
- [ ] Legg til tests/eksempler for kanttilfeller:
  - [x] `/tickets/{id}` med ugyldig/mangler-id (`/tickets/bad-id` og `/tickets/abc` i tests)
  - [x] `POST /tickets` med manglende felt (`title`, `description`)
  - [x] `PUT /tickets/{id}` med ugyldig id og ugyldig/verdi-feil (`/tickets/1` og `/tickets/abc`).
  - [x] Verifiser admin-adgang via route-nivå i `tests/test_helpdesk.no` for 401/403.
- [x] Oppdater `docs/INDEX.md` med lenke til denne planfilen.

## Neste omgang (klare steg)

- [x] Fullfør tilgangsrobusthet:
  - [x] 401/403-differensiering på alle admin-ruter
  - [x] tydeligere svarformat på feil (f.eks. `error`, `code`) på alle ruter
- [x] Legg inn `GET /admin/metrics`.
- [x] Re-introduser `bin/nc serve`-kommando med eksplisitt statusmelding når runtime manglar.
- [ ] Kjør runtime-smoke med server (`./bin/nc serve examples/helpdesk.no`) og verifiser `docs/helpdesk_api_smoke.http`.
- [x] Inntil CLI/serveren støtter faktisk runtime-serverkall: dokumentert in-prosess workaround via `tests/test_helpdesk.no` og `tools/smoke_helpdesk_api.sh`.
- [x] Prøvd akkurat no: `./bin/nc serve examples/helpdesk.no --port 4173` går no frå statusmelding til native serve-køyring, men blir avbroten av `json_parse`-unntak.
- [ ] Når runtime-smoke er mogleg, kjør via aktiv serverkommando i `bin/nc` og valider `docs/helpdesk_api_smoke.http`.
- [x] Oppdater `docs/INDEX.md` med lenke til denne planfilen.

- [x] Oppdater `dist/norscode_native` frå korrekt CLI-bygg (no frå same source etter sjølvhost-endringar).
- [x] Verifiser `./bin/nc check selfhost/nc_main.no` etter dist-justering.
- [x] Verifiser `./bin/nc bundle examples/helpdesk.no` etter dist-justering.

## Omgang 3 — Full server-gjennopptaking

- [ ] Bygg/installer fungerande native CLI frå korrekt byggetrad etter `selfhost` endringar.
- [x] Løys runtime-bug på `./bin/nc serve` i `dist/norscode_native` (gjer at kommandoen går frå `Ukjend NORSCODE_CMD: serve` til aktiv native serve-ruting).
- [x] Følg opp at `./bin/nc check selfhost/nc_main.no` + `./bin/nc bundle` held fram å bestå etter dist-oppdatering.
- [x] Gå frå statusmelding til full `serve`-runtime i `./bin/nc serve` for `examples/helpdesk.no`.
- [ ] Kjør smoke: `NORSCODE_FAKE_HTTP_REQUEST="GET /health HTTP/1.1\r\n\r\n" NORSCODE_FAKE_HTTP_REQUESTS="..." ./bin/nc serve examples/helpdesk.no`.
- [ ] Kjør endelege API-smoke frå `docs/helpdesk_api_smoke.http` mot ekte `serve`-løp.
- [ ] Løys `json_parse`-unntaket ved lagringsopplegg (unngår avbrot ved `./bin/nc run examples/helpdesk.no`/`serve`).
- [x] Omgang 4 del 1 gjennomført:
  - Endra `std/lagring.no` til å bruke `builtin.json_parse_raw`-kallet med fallback-logikk.
  - Verifiserte at endringa er syntaktisk trygg (`./bin/nc check std/lagring.no`).
  - Finner at `./bin/nc run examples/helpdesk.no` framleis stoppar på:
    `KALL builtin.builtin.json_parse_raw` → `Norscode unntak: `.
  - Neste steg er no å isolere runtime-parser (`selfhost/json.no` / `selfhost/vm.no` builtin-json-sti) som kjelde til unntaket.
