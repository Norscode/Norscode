# Selfhost Fase 2 - Byggbacklog

Målet med denne lista er å gjere første Fase-2-arbeid målbart og startbart.

## Sprint 1 (arkivert / fullført for fase 2)

- [x] S1-01: Dokumenter ABI-minimum-kontraktar
  - [x] Kvar runtime-funksjon som vert eksponert i FFI-lista
  - [x] Typemønster og feil-/null-return-kriterium definert
  - [x] Signaturfiler i `std` merkte med versjonsfelt

- [x] S1-02: Oppdatere `docs/SELFHOST_HANDLINGSPLAN.md` med klare DoD-kriterium per sprint
  - [x] DoD ferdig på skrivefelt
  - [x] Akseptkriterium kobla til CI-kommandoar

- [x] S1-03: Standardbibliotek-gap
  - [x] Kartlegg dei 20 modulane som er avgjerande for sjølvstendig utvikling
  - [x] Marker kvar modul som `kan-brukast`, `under-arbeid`, `mangler`
  - [x] Lag 5 minimale regresjonstestar per kritisk modul

- [x] S1-04: Runtime-kontraktstatus
  - [x] Registrer alle uavklarte kontraktar og manglar i runtime-filer
  - [x] Velg 2 runtime-komponentar for konkrete implementasjonar
  - [x] Lag eigne issue-liknande oppgåver med akseptkriterium

- [x] S1-05: CI-kvalitet for handlingsplan
  - [x] Eit raskt statusmål i `maintenance` (`report-json`) for Fase 2 milepælar
  - [x] Sjekk at normal `./bin/nc ci` rapporterer om kvar milepæl

## Sprint 2 (fullført byggsteg)

- [x] S2-01: ABI-minimum v1
  - [x] Definer stabilt namnrom for eksporterte runtime-funksjonar
  - [x] Liste over signaturar som ikkje kan endrast utan ny versjon
  - [x] Dokumenter kva som er intern kontra offentleg flate
  - [x] Kartlegg offentlege runtime-API som skal vere stabile
  - [x] Dokumenter kall-kontraktar for builtin og extern-modular
  - [x] Skriv migrasjonsnotat frå intern bytecode til stabilt API

- [x] S2-02: Standardbibliotek-løype v1
  - [x] Velg 5 modular med høgast nytteverdi for normal utvikling
  - [x] Gi kvar modul ein enkel status: `ferdig`, `pågår`, `mangler`
  - [x] Legg inn ein minimumstest per modul
  - [x] Lag ei statusmatrise for kva som er klart / under arbeid / manglar
  - [x] Dokumenter praktiske brukscase per modul

- [x] S2-03: Runtime-stabiliseringsmål
  - [x] Velg to minimale runtime-kontraktar som skal bytast ut med sterkare implementasjon
  - [x] Definer feilmeldingar og grenser for kvar funksjon
  - [x] Dokumenter avgrensingar som må vere synlege i CI
  - [x] Eit første end-to-end FFI-smoketestløp er på plass
  - [x] Dokumenter diagnostikk for parser, semantic og IR
  - [x] Stram inn patchpunkt og kontrakttekst i runtime-kjernen

- [x] S2-04: CI-status v1
  - [x] Lag ei kort og maskinlesbar vennleg statusflate for fase 2
  - [x] Definer kva som er klar / under arbeid / manglar
  - [x] Knytt statusflata direkte til `./bin/nc ci`
  - [x] Lag eitt fast rapportpunkt for fase-2-gate
  - [x] Køyre ein baseline benchmark i same gate

- [x] S2-05: DoD og regresjonar
  - [x] Definer DoD for fase 2-leveransar
  - [x] Dokumenter kva som tel som ferdig for ABI/runtime/std/CI
  - [x] Lag ei samla liste over fase 2-regresjonstestar
  - [x] Legg inn første regresjonstest som dekker statusflater og artefaktar
  - [x] Knytt alle milepælar til konkrete regresjonstestar

## Måte å markere ferdig

Ein oppgåve er ferdig når:

- kriterium er avkrysa
- endring er dokumentert i `docs/SELFHOST_HANDLINGSPLAN.md`
- ei minimal validering er mogleg å køyre utan Python/C i normal kjede

## Neste steg

Fase 2 er no lukka. Vidare arbeid går inn i fase 3 og seinare utviding.

Statusreferansar:

- `docs/SELFHOST_PHASE2_STATUS.md`
- `docs/SELFHOST_PHASE2_ABI_MINIMUM_V1.md`
- `docs/SELFHOST_PHASE2_RUNTIME_API_V1.md`
- `docs/SELFHOST_PHASE2_CALL_CONTRACTS_V1.md`
- `docs/SELFHOST_PHASE2_FFI_SMOKETEST_V1.md`
- `docs/SELFHOST_PHASE2_RUNTIME_TARGETS.md`
- `docs/SELFHOST_PHASE2_RUNTIME_OPERATING_PLAN.md`
- `docs/SELFHOST_PHASE2_DIAGNOSTICS_PLAN.md`
- `docs/SELFHOST_PHASE2_BACKEND_OPTIMIZATION_PLAN.md`
- `docs/SELFHOST_PHASE2_STDLIB_LØYPE_V1.md`
- `docs/SELFHOST_PHASE2_STDLIB_STATUS_MATRIX.md`
- `docs/SELFHOST_PHASE2_STDLIB_USECASES.md`
- `docs/SELFHOST_PHASE2_CI_STATUS.md`
- `docs/SELFHOST_PHASE2_DOD.md`
- `docs/SELFHOST_PHASE2_REGRESSION_TESTS.md`
- `docs/SELFHOST_PHASE2_MILESTONE_TEST_MAP.md`
- `docs/SELFHOST_PHASE2_MIGRATION_NOTE.md`
