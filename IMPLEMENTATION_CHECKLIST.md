# Norscode implementation checklist

Målet er å gjøre normalflaten 100% selvstendig og samtidig drive de åpne milepælene fremover i riktig rekkefølge.

## Status now

- [x] Python-filer ute av `tools/`
- [x] `bin/nc serve` peker kun mot native-linja
- [x] `bin/nc selftest` er eksponert i native wrapperen
- [x] Aktiv C/Python-surface passerer lokal gate
- [x] Aktive docs er ryddet mot native-only
- [x] GitHub Actions bekreftet grønn eksternt
- [x] `dist/norscode_native` regenerert fra Norscode-kilde med `REGEN=1`
- [x] Typesystem Fase 2 er ferdig på helper- og regresjonsnivå med grunnleggende typeklassifisering, mismatch-rapport, flere uttrykkstyper i typecheckeren, Program-node støtte, samlekjøring av flere feil i samme pass, map-/struct-mismatch-validering og semantic-kobling for deklarasjoner; typechecker-testene og semantic-testene kjører grønt, og semantic-suiten er no faktisk fylt med testresultat og inkluderer også ein liten returner-utanfor-funksjon-regresjon, ein `hvis`-betingelse-typefeilregresjon, ein `mens`-betingelse-typefeilregresjon, ein `match`-typefeilregresjon, ein duplikat-funksjonsregresjon, ein tom-strukturliteral-regresjon, ein duplikat-strukturfelt-regresjon, ein match-subjekt-typefeilregresjon, ein match-wildcard-ordensregresjon, ein match-wildcard-eller-regresjon og ein ukjent-symbol-regresjon, medan den skjøre listemismatchen blir halde der den alt er dekt av typechecker-suiten
- [x] `db.*`-builtins i selfhost-VM, med pool-/persistens-håndtering låst inn i tester
- [x] Pakkebehandler er nå kjørbar som selvstendig demo, og `toolchain/norspkg/nc.no` dekker no også ein lokal installasjonsflyt med summary-linjer for add/remove/update, info, søk, lock, fetch, install, publish, yank og versjon; `toolchain/norspkg/manifest_parser.no`, `toolchain/norspkg/resolver.no`, `toolchain/norspkg/lockfile_flow.no`, `toolchain/norspkg/package_flow.no`, `toolchain/norspkg/install_flow.no` og `toolchain/norspkg/registry_protocol.no` har alle lokale selftests som går grønt på check-nivå, med checksum-/rapportvalidering i install-flyten, eit lockfile-samandrag som låser inn pakketal, checksums og prosjektidentitet, eigne samandrag for manifest, package/install-flow, resolver og registry-protokollen, samt versjonssamandrag og eigen `nc_selftest()` i nc-selftesten, fetch-status ved manglande lock/feil og ein samla selftest-samandrag i `nc.no`; manifestparseren, resolveren, lockfile-flyten, package-flowen, install-flowen, registry-protokollen, auth, cache og publisher har no også selftest-broer og fleire av dei er no grøne, `auth.no` har no òg eigen samandraglinje, `publisher.no` har no òg eigen samandraglinje, `package_flow.no` har no òg eigen statuslinje, `install_flow.no` har no òg eigen statuslinje, `registry_protocol.no` har no òg eigen statuslinje, `resolver.no` har no òg eigen statuslinje og `lockfile_flow.no` har no òg eigen statuslinje; `cache.no` er no også grønn etter at den tunge installeringsruta vart forenkla
- [x] NorsDB er flyttet inn under `packages/norsdb` med `norsdb.nors` som samla inngangspunkt, `norsdb_smoke.nors` som sjølvstendig smoke-test og `norsdb_selftest.no` som lokal pakke-selftest; README og release notes er oppdaterte til den stille runtime-formen.
- [x] LSP: ferdig i `selfhost/lsp/server.no` med minimal JSON-RPC-flate, selftest, demo-runner med batch/session, trace med request summary, `lsp_stdio_main()`, session/notification-flyt, session summary, probe summary frå dokumenttilstand, dokument-eksistenssjekk, metode-sjekk, method-overview, metodehistorikk, dokumentoppsummering, state summary, method-count i state summary, capabilities summary, notifications summary, notification type summary, protocol summary basert på dokumenttilstand/session, notifications count summary, notifications health summary, notifications overview summary, transport summary, transport overview summary, supported-methods summary, IO-json-batch-stdio summary, framing summary, input-mode summary, transport report med stage og request_kind, entrypoint summary, message-count summary, full summary, route summary, document-exists summary, output summary, document-health summary, document-health-state summary, version-health summary, latest-document-version summary, top status og tri-state banner for demoen; JSON-batch-stdio og Content-Length-framing er på plass, med støtte for fleire framede requests i same payload, toler ledande whitespace, stream-aggregator for sekvensielle payload-chunks utan dobbeltbehandling, stream-mode, hybrid-mode, stream-source og stream-overview for chunks/requests/responses/notifications og newline-separerte chunk-strømmer, `textDocument/publishDiagnostics` er eksplisitt med i metodekriteria, route-kriteria og selftest-sjekkane, transport- og framing-oversikt er bekrefta i selftesten, stream-source er i statusbunken, stream-overview har hybrid-markering og chunk-teljing, `stream_overview=none`-, `request_kind=none`-, `stream_mode=none`-, `stream=none`-, `stream_chunk_count=0`-, `notification=unknown`-, `stream_source=empty`- og `empty_transport_report`-kantdekning er på plass, og det finst ein eksplisitt payload-join selftest, eksplisitt empty-session-dekning for notifications overview, samt ein eksplisitt stdin-mode via `std.sys.stdin_les()` som les til tom linje/EOF i éi løype og brukar faktisk samanføydd stdin-innhald, og skriv `stdin_bytes` og `input_mode` i outputen; selftesten er rydda for ein liten duplikatkontroll
- [x] WASM-backend har nå ei minimal selfhost-bridge i `selfhost/tests/wasm_tests.no` med ein enkel selftest som byggjer eit lite modulobjekt og verifiserer ein WASM-modulshape med 5 seksjonar

## Next actions

1. Verifiser eventuelle restar i normaldocs mot arkivstatusen.
2. Arkiver eller behold dei historiske referansane som eksplisitt vedlikehald.
3. Når du vil ta eit nytt funksjonsspor, vel eitt konkret mål frå det urelaterte arbeidstreet.

## Working rules

- Normal flyt skal ikkje avhenge av Python eller C.
- Legacy og vedlikehald skal vere tydelig merka og isolert.
- Nye funksjoner skal ha tester før eller sammen med implementasjon.
