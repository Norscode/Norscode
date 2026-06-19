# Selfhost Fase 2 - Status

Dette dokumentet samlar den noverande fase-2-statusen i ei kort oversikt.

## Kort status

- [x] Fase 2 er fullført
- [x] ABI-minimum v1 er dokumentert
- [x] Første runtime-mål er dokumentert
- [x] Standardbibliotek-løypa er delvis konkretisert
- [x] Kvalitet og CI er ferdig gate

## Det som er bygd

### ABI og integrasjon

- [x] ABI-minimum v1
- [x] Offentlege runtime-API er kartlagt fullt ut
- [x] Kall-kontraktar for builtin og extern-modular er dokumentert
- [x] Eit første end-to-end FFI-smoketestløp er på plass
- [x] Migrasjonsnotat frå intern bytecode til stabilt API er skrive

### Runtime og verktøy

- [x] Runtime-målskisse
- [x] `selfhost/runtime/production_runtime.no` er styrkt med betre kontraktar
- [x] Production-runtime-mål er samla i [docs/SELFHOST_PHASE2_RUNTIME_OPERATING_PLAN.md](./SELFHOST_PHASE2_RUNTIME_OPERATING_PLAN.md)
- [x] `selfhost/runtime/production_runtime.no` har samla runtime-konstruktør og statusflate
- [x] Runtime-kjernas forward-ref placeholderar er samla i ein liten hjelpefunksjon
- [x] Baseline benchmark er definert og køyrbar i CI
- [x] Diagnostikkplan for parser/semantic/IR er dokumentert i [docs/SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md](./SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md)
- [x] Verktøy og feilsøkingsindeks er samla i [docs/SELFHOST_PHASE2_TOOLING_INDEX.md](./SELFHOST_PHASE2_TOOLING_INDEX.md)

### Backend og optimalisering

- [x] Backend-optimalisering er planlagt i [docs/SELFHOST_PHASE2_BACKEND_OPTIMIZATION_PLAN.md](./SELFHOST_PHASE2_BACKEND_OPTIMIZATION_PLAN.md)
- [x] Første konkrete filter i [selfhost/compiler/production_backend.no](../selfhost/compiler/production_backend.no) er lagt inn for å fjerne tomme og `nop`-instruksjonar
- [x] Reproduserbarheitssjekken i [selfhost/compiler/production_backend.no](../selfhost/compiler/production_backend.no) toler no manglande input utan å feile
- [x] Serialiseringsvegen i [selfhost/compiler/production_backend.no](../selfhost/compiler/production_backend.no) handterer no tomme namn og manglande executable meir defensivt
- [x] `serialize_section(...)` normaliserer no manglande data til ei tom liste
- [x] Assembler- og linker-vegen i [selfhost/compiler/production_backend.no](../selfhost/compiler/production_backend.no) toler no manglande input og initierer interne lister ved behov
- [x] Instruksjonskodaren i [selfhost/compiler/production_backend.no](../selfhost/compiler/production_backend.no) registrerer no manglande opcode som feil i staden for å skrive ugyldig bytecode
- [x] Relocation-patcharen i [selfhost/compiler/production_backend.no](../selfhost/compiler/production_backend.no) initierer no interne lister og markerer manglande relocation som uhandtert
- [x] Relocation-patcharen lagrar no ei lita feildetalj når relocation manglar
- [x] Relocation-patcharen skil no ut manglande symbol frå andre formfeil
- [x] Relocation-patcharen skil no ut manglande offset frå andre formfeil
- [x] Relocation-patcharen skil no ut manglande relocation_type frå andre formfeil
- [x] Relocation-patcharen brukar no berre presise feilkategoriar for relocation-forma
- [x] Executable-emitteren i [selfhost/compiler/production_backend.no](../selfhost/compiler/production_backend.no) initierer no interne lister og toler manglande executable
- [x] Serialiserings- og emitter-feil legg no igjen små feildetaljar i eksisterande lister
- [x] Executable-emitteren normaliserer no manglande format til første støtta format
- [x] Reproduserbarheitssjekken legg no igjen presise feildetaljar når `binary_a` eller `binary_b` manglar
- [x] Reproduserbarheitssjekken lagrar no `hash_a`, `hash_b` og ein kort statusgrunn
- [x] Reproduserbarheitssjekken set no eksplisitt statusgrunn når binærar manglar
- [x] Reproduserbarheitssjekken set no ein hash-basert statusgrunn når binærar skil seg
- [x] Fase-2 CI-rapporten viser no `hash_a`, `hash_b` og statusgrunn for reproduksjon
- [x] `production_backend_report(...)` eksporterer no reproduksjonsfelta vidare frå backend-objektet
- [x] Fase-2 CI-rapporten skriv no ein kort reproduksjonslinje før detaljfelta
- [x] Instruksjonskodaren normaliserer no manglande operands til ei tom liste
- [x] Optimaliseringsvegen legg no igjen ei feildetalj når machine_ir manglar
- [x] Optimaliseringsvegen normaliserer no manglande instruksjonslister til tom liste
- [x] Assembler- og linker-vegen normaliserer no eksplisitt format når det manglar
- [x] `ny_production_backend()` samlar no backend-delane i eitt konstruktør-objekt
- [x] `production_backend_status(...)` samlar no status for encoder, serializer, patcher, reproducible, optimized, emitter, assembler og linker
- [x] `production_backend_report(...)` gir no ei samla teljing av ok- og manglande backend-delar
- [x] Fase-2 CI-rapporten brukar no `ny_production_backend()` og `production_backend_report(...)` som live backend-sjekk
- [x] Backend-rapporten gir no ein kort samandragstreng for CI-lesbarheit
- [x] Backend-rapporten gir no også ei kort fokusstreng for det som manglar
- [x] Backend-rapporten gir no ei samla statuslinje for CI-output
- [x] Backend-rapporten gir no ei eksplisitt liste over manglande delar
- [x] Backend-rapporten gir no første manglande del som eit eige felt
- [x] Backend-rapporten gir no ein samla digest for CI-output
- [x] Backend-digesten inkluderer no òg statusgrunn når det finst
- [x] Backend-rapporten gir no eit eige overview-felt som speglar digesten
- [x] Fase-2 CI-rapporten plasserer no `backend_overview` som første backend-linje
- [x] Fase-2 CI-rapporten skriv no ein samla `phase2.overview`-linje først
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_overview` separat
- [x] Fase-2 CI-rapporten har no færre sekundære backend-tal ved primær-linja
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_optimization` separat
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_live` separat
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_summary` separat
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_missing_list` separat
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_status` separat
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_first_missing` separat
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_hash_a` og `backend_hash_b` separat
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_focus` separat
- [x] Fase-2 CI-rapporten skriv no ikkje lenger ut `backend_status_reason` separat
- [x] Assembler- og linker-failar legg no igjen små feildetaljar i eksisterande lister
- [x] Fase-2 CI-rapporten peikar no også på backend-optimaliseringsplanen som ein del av statusfeltet

### Standardbibliotek

- [x] `std/log.no`
- [x] `std/fil.no`
- [x] `std/cache.no`
- [x] `std/lagring.no`
- [x] `std/innstillingar.no`
- [x] `std/sched.no`
- [x] `std/tråd.no`
- [x] Fem modulstatusar er formelt låst
- [x] Minimumstest per modul er på plass
- [x] Standardbibliotek-statusmatrise er lagt inn i [docs/SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md](./SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md)
- [x] Praktiske brukscase er dokumenterte i [docs/SELFHOST_PHASE2_STDLIB_USECASES.md](./SELFHOST_PHASE2_STDLIB_USECASES.md)
- [x] CI-statusflata er definert i [docs/SELFHOST_PHASE2_CI_STATUS.md](./SELFHOST_PHASE2_CI_STATUS.md)
- [x] DoD er definert i [docs/SELFHOST_PHASE2_DOD.md](./SELFHOST_PHASE2_DOD.md)
- [x] Regresjonstestar er samla i [docs/SELFHOST_PHASE2_REGRESSION_TESTS.md](./SELFHOST_PHASE2_REGRESSION_TESTS.md)
- [x] Milepælar er kopla til konkrete testreferansar i [docs/SELFHOST_PHASE2_MILESTONE_TEST_MAP.md](./SELFHOST_PHASE2_MILESTONE_TEST_MAP.md)

## Kva som står att først

- [x] Fylle ut `S2-02` med minimumstest per modul
- [x] Fylle ut `S2-01` med full kartlegging av offentlege runtime-API
- [x] Skrive eit kort CI-oppsett for fase 2-status
- [x] Lage ei enkel “kan brukast / under arbeid / manglar”-matrise
- [x] Kople fase 2-rapporten direkte til `./bin/nc ci`
- [x] Definere DoD og regresjonsliste for fase 2

## Kva vi kan sjå no

- `std`-modulane over har no meir defensive kontraktar og statusflater
- `docs/SELFHOST_HANDLINGSPLAN.md` viser kvar fase 2 er aktiv
- `docs/SELFHOST_PHASE2_BACKLOG.md` viser kva som er ferdig og kva som står att
