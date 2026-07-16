# Selfhost Handlingsplan

Dette er den aktive planen for å stabilisere selfhost-kjernen i Norscode.

## Status i eitt bilete

- [x] **Fase 1 er fullført**
- [x] **Fase 2 er fullført**

## Fase 2: Operasjonell bygg-plass

- [x] Fase 2 er fullført
- [x] Måleindikatorar er definerte
- [x] Kvar leveranse har eigne godkjenningskrav
- [x] Dokumentasjon er i tråd med faktisk implementasjon

Aktiv bygglogg for Fase 2:

- [x] Sjå [docs/SELFHOST_PHASE2_BACKLOG.md](./SELFHOST_PHASE2_BACKLOG.md)
- [x] Sjå [docs/SELFHOST_PHASE2_SPRINT2_PLAN.md](./SELFHOST_PHASE2_SPRINT2_PLAN.md)
- [x] Sjå [docs/SELFHOST_PHASE2_STATUS.md](./SELFHOST_PHASE2_STATUS.md)
- [x] Sjå [docs/SELFHOST_PHASE2_CI_STATUS.md](./SELFHOST_PHASE2_CI_STATUS.md)
- [x] Sjå [docs/SELFHOST_PHASE2_DOD.md](./SELFHOST_PHASE2_DOD.md)
- [x] Sjå [docs/SELFHOST_PHASE2_REGRESSION_TESTS.md](./SELFHOST_PHASE2_REGRESSION_TESTS.md)
- [x] Sjå [docs/SELFHOST_PHASE2_MILESTONE_TEST_MAP.md](./SELFHOST_PHASE2_MILESTONE_TEST_MAP.md)
- [x] Sjå [docs/SELFHOST_PHASE2_MIGRATION_NOTE.md](./SELFHOST_PHASE2_MIGRATION_NOTE.md)

## Slik les du planen raskt

`[X]` = ferdig · `[~]` = pågår · `[ ]` = ikkje starta

### Status

- [x] **Phase 1: Fullført språk-kjerne**
- [x] **Phase 2: Breiare runtime/standardbibliotek og verktøy**

### Hva er gjort no?

- [x] Deterministisk tokenisering
- [x] Deterministisk parsing (under stabilisering)
- [x] Stabil AST-kontrakt
- [x] Grunnleggjande semantisk validering
- [x] Parity-fixtures for parser og AST
- [x] Dokumentert normalveg frå `.no` til AST/bytecode

### Fase 3: Vidare utviding

- [x] Oppstartsskisse er dokumentert i [docs/SELFHOST_PHASE3_KICKOFF.md](./SELFHOST_PHASE3_KICKOFF.md)
- [x] Første backlog er dokumentert i [docs/SELFHOST_PHASE3_BACKLOG.md](./SELFHOST_PHASE3_BACKLOG.md)
- [x] Første AST-kontrakt er dokumentert i [docs/SELFHOST_PHASE3_AST_CONTRACT_V1.md](./SELFHOST_PHASE3_AST_CONTRACT_V1.md)
- [x] Første semantic-kjerne er dokumentert i [docs/SELFHOST_PHASE3_SEMANTIC_CORE_V1.md](./SELFHOST_PHASE3_SEMANTIC_CORE_V1.md)
- [x] Første IR-til-bytecode-løype er dokumentert i [docs/SELFHOST_PHASE3_IR_BYTECODE_V1.md](./SELFHOST_PHASE3_IR_BYTECODE_V1.md)
- [x] Fase-3-status er dokumentert i [docs/SELFHOST_PHASE3_STATUS.md](./SELFHOST_PHASE3_STATUS.md)
- [x] Integrasjonsmap er dokumentert i [docs/SELFHOST_PHASE3_INTEGRATION_MAP.md](./SELFHOST_PHASE3_INTEGRATION_MAP.md)
- [x] Integrasjonsstatus er dokumentert i [docs/SELFHOST_PHASE3_INTEGRATION_STATUS.md](./SELFHOST_PHASE3_INTEGRATION_STATUS.md)
- [x] Smoke er dokumentert i [docs/SELFHOST_PHASE3_SMOKE.md](./SELFHOST_PHASE3_SMOKE.md)
- [x] Regresjon er dokumentert i [docs/SELFHOST_PHASE3_REGRESSION.md](./SELFHOST_PHASE3_REGRESSION.md)
- [x] IR-smoke er dokumentert i [docs/SELFHOST_PHASE3_IR_SMOKE.md](./SELFHOST_PHASE3_IR_SMOKE.md)
- [x] IR-regresjon er dokumentert i [docs/SELFHOST_PHASE3_IR_REGRESSION.md](./SELFHOST_PHASE3_IR_REGRESSION.md)
- [x] ABI og FFI er dokumentert i [docs/SELFHOST_PHASE3_ABI_FFI_V1.md](./SELFHOST_PHASE3_ABI_FFI_V1.md)
- [x] ABI og FFI-status er dokumentert i [docs/SELFHOST_PHASE3_ABI_FFI_STATUS.md](./SELFHOST_PHASE3_ABI_FFI_STATUS.md)
- [x] Betre backend-optimalisering er dokumentert i [docs/SELFHOST_PHASE3_BACKEND_OPTIMIZATION_V1.md](./SELFHOST_PHASE3_BACKEND_OPTIMIZATION_V1.md)
- [x] Backend-optimaliseringstatus er dokumentert i [docs/SELFHOST_PHASE3_BACKEND_OPTIMIZATION_STATUS.md](./SELFHOST_PHASE3_BACKEND_OPTIMIZATION_STATUS.md)
- [x] Verktøy og feilsøkingsstøtte er dokumentert i [docs/SELFHOST_PHASE3_TOOLING_DEBUGGING_V1.md](./SELFHOST_PHASE3_TOOLING_DEBUGGING_V1.md)
- [x] Verktøy og feilsøkingsstatus er dokumentert i [docs/SELFHOST_PHASE3_TOOLING_DEBUGGING_STATUS.md](./SELFHOST_PHASE3_TOOLING_DEBUGGING_STATUS.md)
- [x] Breiare standardbibliotek er dokumentert i [docs/SELFHOST_PHASE3_STDLIB_BREADTH_V1.md](./SELFHOST_PHASE3_STDLIB_BREADTH_V1.md)
- [x] Breiare standardbibliotek-status er dokumentert i [docs/SELFHOST_PHASE3_STDLIB_BREADTH_STATUS.md](./SELFHOST_PHASE3_STDLIB_BREADTH_STATUS.md)

## Fase 1: Fullført språk-kjerne (ferdig)

Fase 1 er fullført når desse punkta er på plass:

- [x] deterministisk tokenisering
- [x] deterministisk parsing
- [x] stabil AST-kontrakt
- [x] grunnleggjande semantisk validering
- [x] eksplisitte parity-fixtures for parser og AST
- [x] dokumentert normalveg frå `.no` til AST/bytecode

## Leveranser

1. Ein aktiv AST-kontrakt med formatnamn og minimumsfelt.
2. Ein validator for AST-v1 som kan brukast av parser, selfhost og CI.
3. Parity-fixtures for stabile parser- og AST-caser.
4. Ei tydeleg dokumentert språk-kjerne utan skjulte sidevegar.

Status: [x] (leveranse 1–4 er i orden)

## Stabilitetsreglar

- Nye felt kan leggjast til utan å bryte formatet.
- [x] Eksisterande felt skal ikkje skifte tyding.
- [x] Parser og semantic-lag skal feile deterministisk.
- [x] Bootstrap-løypa skal vere kort og tydeleg markert.
- [x] C-regen er fjerna frå aktiv verktøyflate og ligg berre som historisk arkiv.

## Avgrensning

Fase 1 prøver ikkje å løyse alt:

- ikkje full optimalisering
- ikkje komplett standardbibliotek
- ikkje ny native backend som primærveg
- ikkje nye C-baserte normalsteg
- ikkje ny bootstrap-seed i normal utviklings- eller CI-løype

## Neste fase

Fase 4 er starta med oppstartsplan, backlog og første statusområde.

### Fase 4: Vidare utviding

- [x] Oppstartsskisse er dokumentert i [docs/SELFHOST_PHASE4_KICKOFF.md](./SELFHOST_PHASE4_KICKOFF.md)
- [x] Første backlog er dokumentert i [docs/SELFHOST_PHASE4_BACKLOG.md](./SELFHOST_PHASE4_BACKLOG.md)
- [x] Fase-4-status er dokumentert i [docs/SELFHOST_PHASE4_STATUS.md](./SELFHOST_PHASE4_STATUS.md)
- [x] Modulsystem og importflyt er dokumentert i [docs/SELFHOST_PHASE4_MODULE_SYSTEM_V1.md](./SELFHOST_PHASE4_MODULE_SYSTEM_V1.md)
- [x] Modulsystemstatus er dokumentert i [docs/SELFHOST_PHASE4_MODULE_SYSTEM_STATUS.md](./SELFHOST_PHASE4_MODULE_SYSTEM_STATUS.md)
- [x] Breiare standardbibliotek er dokumentert i [docs/SELFHOST_PHASE4_STDLIB_BREADTH_V1.md](./SELFHOST_PHASE4_STDLIB_BREADTH_V1.md)
- [x] Breiare standardbibliotek-status er dokumentert i [docs/SELFHOST_PHASE4_STDLIB_BREADTH_STATUS.md](./SELFHOST_PHASE4_STDLIB_BREADTH_STATUS.md)
- [x] Fase-4 smoke er dokumentert i [docs/SELFHOST_PHASE4_SMOKE.md](./SELFHOST_PHASE4_SMOKE.md)
- [x] Fase-4 regresjon er dokumentert i [docs/SELFHOST_PHASE4_REGRESSION.md](./SELFHOST_PHASE4_REGRESSION.md)
- [x] Fase-4 status er ferdig lukka

### Fase 5: Produktisering og distribusjon

- [x] Oppstartsskisse er dokumentert i [docs/SELFHOST_PHASE5_KICKOFF.md](./SELFHOST_PHASE5_KICKOFF.md)
- [x] Første backlog er dokumentert i [docs/SELFHOST_PHASE5_BACKLOG.md](./SELFHOST_PHASE5_BACKLOG.md)
- [x] Fase-5-status er dokumentert i [docs/SELFHOST_PHASE5_STATUS.md](./SELFHOST_PHASE5_STATUS.md)
- [x] Fase-5 smoke er dokumentert i [docs/SELFHOST_PHASE5_SMOKE.md](./SELFHOST_PHASE5_SMOKE.md)
- [x] Fase-5 regresjon er dokumentert i [docs/SELFHOST_PHASE5_REGRESSION.md](./SELFHOST_PHASE5_REGRESSION.md)
- [x] Fase-5 release og CLI-flate er lukka

### Fase 6: Økosystem og utviklarflyt

- [x] Oppstartsskisse er dokumentert i [docs/SELFHOST_PHASE6_KICKOFF.md](./SELFHOST_PHASE6_KICKOFF.md)
- [x] Første backlog er dokumentert i [docs/SELFHOST_PHASE6_BACKLOG.md](./SELFHOST_PHASE6_BACKLOG.md)
- [x] Fase-6-status er dokumentert i [docs/SELFHOST_PHASE6_STATUS.md](./SELFHOST_PHASE6_STATUS.md)
- [x] Fase-6 smoke er dokumentert i [docs/SELFHOST_PHASE6_SMOKE.md](./SELFHOST_PHASE6_SMOKE.md)
- [x] Fase-6 regresjon er dokumentert i [docs/SELFHOST_PHASE6_REGRESSION.md](./SELFHOST_PHASE6_REGRESSION.md)
- [x] Fase-6 integrasjon og vedlikehald er lukka

## Fase 2: Konkret plan

### Del 2.1 ABI og integrasjon

- [x] ABI-minimum v1 er dokumentert i [docs/SELFHOST_PHASE2_ABI_MINIMUM_V1.md](./SELFHOST_PHASE2_ABI_MINIMUM_V1.md)
- [x] Kartlegg offentlege runtime-API som skal vere stabile i [docs/SELFHOST_PHASE2_RUNTIME_API_V1.md](./SELFHOST_PHASE2_RUNTIME_API_V1.md)
- [x] Dokumenter kall-kontraktar for builtin og extern-modular i [docs/SELFHOST_PHASE2_CALL_CONTRACTS_V1.md](./SELFHOST_PHASE2_CALL_CONTRACTS_V1.md)
- [x] Eit første end-to-end FFI-smoketestløp i [docs/SELFHOST_PHASE2_FFI_SMOKETEST_V1.md](./SELFHOST_PHASE2_FFI_SMOKETEST_V1.md)
- [x] Eit lite migrasjonsnotat frå intern bytecode til stabilt API i [docs/SELFHOST_PHASE2_MIGRATION_NOTE.md](./SELFHOST_PHASE2_MIGRATION_NOTE.md)

### Del 2.2 Runtime og verktøy

- [x] Første runtime-mål er dokumentert i [docs/SELFHOST_PHASE2_RUNTIME_TARGETS.md](./SELFHOST_PHASE2_RUNTIME_TARGETS.md)
- [x] Skriv plan for production-runtime-mål (minne, sandbox, IO, scheduler) i [docs/SELFHOST_PHASE2_RUNTIME_OPERATING_PLAN.md](./SELFHOST_PHASE2_RUNTIME_OPERATING_PLAN.md)
- [x] Reduser uavklarte runtime-kontraktar og historiske mock-stiar innan runtime-kjerne
- [x] Gjennomfør baseline benchmark med reproduksjon i CI
- [x] Forbetre diagnoser for parser/semantic/IR-feil i [docs/SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md](./SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md)
- [x] Verktøy og feilsøkingsindeks i [docs/SELFHOST_PHASE2_TOOLING_INDEX.md](./SELFHOST_PHASE2_TOOLING_INDEX.md)

### Del 2.3 Standardbibliotek og produktivitet

- [x] Prioriterte modular er dokumentert i [docs/SELFHOST_PHASE2_STDLIB_LØYPE_V1.md](./SELFHOST_PHASE2_STDLIB_LØYPE_V1.md)
- [x] Sett avkryssingslister for “kan bruke i prosjekt” vs “under arbeid” i [docs/SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md](./SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md)
- [x] Dokumenter praktiske brukscase i [docs/SELFHOST_PHASE2_STDLIB_USECASES.md](./SELFHOST_PHASE2_STDLIB_USECASES.md)
- [x] Løft minst 8 modulmål frå skissa til testdekkje
- [x] Dokumenter avvik frå Python-kompatibilitet der det er kjent
- [x] `std/lagring.no` er bygd vidare med reell JSON-last og tryggare skriv/les-kontrakt
- [x] `std/cache.no` er bygd vidare med opprydding av utløpte nøklar og statusflate
- [x] `std/sched.no` er bygd vidare med statusflate for kø og neste hending
- [x] `std/tråd.no` er bygd vidare med statusoversikt for manager og trådstatus
- [x] `std/innstillingar.no` er bygd vidare med statusflate og miljønøkkeloversikt
- [x] `std/fil.no` er bygd vidare med tryggare inputkontroll og statusflate
- [x] `std/log.no` er bygd vidare med filtrering og statusflate
- [x] Fase 2-status er samla i [docs/SELFHOST_PHASE2_STATUS.md](./SELFHOST_PHASE2_STATUS.md)
- [x] Minimumstest for fase 2 er lagt inn i [tests/test_selfhost_phase2_smoke.no](../tests/test_selfhost_phase2_smoke.no)

### Del 2.4 Kvalitet og CI

- [x] Opprett DoD per leveranse (Definition of Done)
- [x] Legg inn regresjonstestar for alle “Fase 2”-milestenar
- [x] Koble ny plan til `./bin/nc ci`-gata med rapportpunkt
- [x] Gjør statusrapporten maskinlesbar på gate-nivå
- [x] Baseline benchmark er lagt inn i `./bin/nc ci`
- [x] Definer CI-statusflata i [docs/SELFHOST_PHASE2_CI_STATUS.md](./SELFHOST_PHASE2_CI_STATUS.md)
- [x] Definer DoD i [docs/SELFHOST_PHASE2_DOD.md](./SELFHOST_PHASE2_DOD.md)
- [x] Dokumenter regresjonstestar i [docs/SELFHOST_PHASE2_REGRESSION_TESTS.md](./SELFHOST_PHASE2_REGRESSION_TESTS.md)
- [x] Knytt milepælar til konkrete regresjonstestar i [docs/SELFHOST_PHASE2_MILESTONE_TEST_MAP.md](./SELFHOST_PHASE2_MILESTONE_TEST_MAP.md)

### Del 2.5 Backend-optimalisering

- [x] Backend-optimaliseringsplan er dokumentert i [docs/SELFHOST_PHASE2_BACKEND_OPTIMIZATION_PLAN.md](./SELFHOST_PHASE2_BACKEND_OPTIMIZATION_PLAN.md)
- [x] Lowering er forenkla og dokumentert ende-til-ende
- [x] Reproduserbarheit er brukt som eksplisitt backend-gate
- [x] Serialiseringsvegen er gjort tydeleg og stabil

## Målbare godkjenningskriterium

- [x] Kildekoden byggjer utan Python/C i normal utviklingsløype
- [x] Fase 2-delar har eigne grep i CI
- [x] Dokumentasjon svarer til faktisk implementasjon
- [x] Mindre enn 1 P1-regresjon frå selfhost-paritet over 4 påfølgjande køyringar
- [x] Nye funn blir loggført i `docs/_archive/` når designval endrar målbildet

Statusblokk for neste fase:

- [x] **Fase 2 er ferdig**

## Sjølvstendigheitsmål

Målet for aktiv flate er at utvikling, testing, release og nye funksjonar kan køyrast utan Python eller C som arbeidsveg.

- C-regen skal ikkje finnast i aktiv `tools/`- eller CI-flate
- C-regen skal berre finnast i eksplisitt vedlikehaldslane
- stage-0 seed skal vere committed eller lastast ned som ferdig native artefakt
- historisk C skal berre liggje under `archive/`
- normal verifisering skal bruke den eksisterande native CLI-en, ikkje byggje han opp på nytt
- nye funksjonar skal verifiserast med `./bin/nc feature-check [fil.no ...]`

---

## Sjølvstendigheitsmålsnitt (operativt)

- [x] Utvikling, testing, release og feature-gate kan køyrast utan Python/C i aktiv flate.
- [x] Stage-0 seed er ferdig native artefakt, ikkje C/Python-bygg i repoet.
- [x] Normal verifisering er rein og kort.
