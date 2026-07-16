# Norscode Dokumentasjon

Dette er hovudingangen til dokumentasjonen. Dokumentasjonen er delt i fire nivå:

1. **Bruk**: installasjon, CLI, prosjekt, køyring og feilsøking.
2. **Opplæring**: stegvis innføring i språk, modulbruk og arbeidsflyt.
3. **Korleis Norscode verkar**: selfhost, parser, semantic, bytecode, VM og native runtime.
4. **Historikk og vedlikehald**: gamle fasar, migrasjonar og arkiv.

Start med brukarmanualen dersom du vil bruke Norscode. Start med opplæringsguiden dersom du vil lære språket frå botnen.

## Start her

- [Full brukarmanual](USER_MANUAL.md)
- [Installasjon](../INSTALL.md)
- [Opplæringsguide](LEARNING_GUIDE.md)
- [Treningsguide](TRAINING_GUIDE.md)
- [Dokumentasjonsindeks for vedlikehald](DOCUMENTATION_INDEX.md)
- [Løypekart](LANE_MAP.md)
- [Status](STATUS.md)
- [10/10 modenhetsplan](MODENHET_10_10.md)
- [Brand og ikoner](BRAND.md)
- [Selfhost handlingsplan](SELFHOST_HANDLINGSPLAN.md)

## Brukarmanual

- [Installere og kontrollere Norscode](USER_MANUAL.md#installasjon-og-kontroll)
- [CLI-kommandoar](USER_MANUAL.md#cli-kommandoar)
- [Køyre, sjekke og teste kode](USER_MANUAL.md#koyre-sjekke-og-teste-kode)
- [Prosjekt og apper](USER_MANUAL.md#prosjekt-og-apper)
- [Pakker og standardbibliotek](USER_MANUAL.md#pakker-og-standardbibliotek)
- [AI](USER_MANUAL.md#ai)
- [Server](USER_MANUAL.md#server)
- [NorsDB](USER_MANUAL.md#norsdb)
- [Feilsøking](USER_MANUAL.md#feilsøking)

## Opplæring

- [Leksjon 1: Første program](LEARNING_GUIDE.md#leksjon-1-første-program)
- [Leksjon 2: Variablar og typar](LEARNING_GUIDE.md#leksjon-2-variablar-og-typar)
- [Leksjon 3: Funksjonar](LEARNING_GUIDE.md#leksjon-3-funksjonar)
- [Leksjon 4: Kontrollflyt](LEARNING_GUIDE.md#leksjon-4-kontrollflyt)
- [Leksjon 5: Lister og ordbøker](LEARNING_GUIDE.md#leksjon-5-lister-og-ordbøker)
- [Leksjon 6: Modular](LEARNING_GUIDE.md#leksjon-6-modular)
- [Leksjon 7: JSON og data](LEARNING_GUIDE.md#leksjon-7-json-og-data)
- [Leksjon 8: Bygg ein liten funksjon](LEARNING_GUIDE.md#leksjon-8-bygg-ein-liten-funksjon)

## Korleis Norscode verkar

- [Selfhost-handlingsplan](SELFHOST_HANDLINGSPLAN.md)
- [AST-kontrakt](SELFHOST_PHASE3_AST_CONTRACT_V1.md)
- [Semantisk kjerne](SELFHOST_PHASE3_SEMANTIC_CORE_V1.md)
- [IR og bytecode](SELFHOST_PHASE3_IR_BYTECODE_V1.md)
- [Integrasjonskart](SELFHOST_PHASE3_INTEGRATION_MAP.md)
- [Modulsystem](SELFHOST_PHASE4_MODULE_SYSTEM_V1.md)
- [Standardbibliotek, fase 4](SELFHOST_PHASE4_STDLIB_BREADTH_V1.md)
- [Native ABI](05-development/NATIVE_CODEGEN_V2_ABI.md)

## Utvikling og CI

- [CI-gates](05-development/SELFHOST_CI_GATES.md)
- [Release-sjekkliste](05-development/SELFHOST_RELEASE_CHECKLIST.md)
- [CLI-kontrakt](05-development/CLI_CONTRACT.md)
- [Kompilatorpipeline](05-development/COMPILER_PIPELINE.md)
- [10/10 modenhetsplan](MODENHET_10_10.md)
- [Lane map](LANE_MAP.md)
- [Brand og ikoner](BRAND.md)
- [Dokumentasjonsindeks](DOCUMENTATION_INDEX.md)
- [Migrasjon og deprecations](05-development/SELFHOST_MIGRATION_AND_DEPRECATIONS.md)
- [Fase 2 verktøyindeks](SELFHOST_PHASE2_TOOLING_INDEX.md)
- [Fase 2 status](SELFHOST_PHASE2_STATUS.md)
- [Fase 3 status](SELFHOST_PHASE3_STATUS.md)
- [Fase 4 status](SELFHOST_PHASE4_STATUS.md)
- [Fase 5 status](SELFHOST_PHASE5_STATUS.md)
- [Fase 6 status](SELFHOST_PHASE6_STATUS.md)

## Server, web og Django-paritet

- [Django-paritet sjekkliste](99-django-paritet-sjekkliste.md)
- [Django-paritet i Norscode](99-django-paritet-norscode.no)
- [HTTP-server](HTTP_SERVER.md)
- [HTTP-server sjekkliste](HTTP_SERVER_CHECKLIST.md)
- [API Explorer](api-explorer/index.html)

## Pakker Og Editor

- [Pakkesystem](03-packages/PACKAGE_MANAGER.md)
- [Registry-protokoll](03-packages/REGISTRY_PROTOCOL.md)
- [LSP og IDE](04-how-to-guides/IDE_AND_LSP.md)

## Arkiv

Historiske notat ligg i arkivet under `docs/_archive/ARCHIVE_INDEX.md`. Dei er nyttige for bakgrunn, men dei er ikkje normal brukarveg.

- [Arkivindeks](./_archive/ARCHIVE_INDEX.md)

## Normal kjede

Normal utviklingsløype skal vere fri for Python og C:

```text
.no -> lexer/parser/semantic/bytecode -> NCB JSON -> selfhost/vm.no
```

Bruk:

```bash
./bin/nc run app.no
./bin/nc check app.no
./bin/nc feature-check app.no
./bin/nc test
```
