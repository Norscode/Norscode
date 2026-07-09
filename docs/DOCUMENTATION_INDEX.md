# Dokumentasjonsindeks

Dette er ein kort, stabil indeks for vedlikehald og dokumentkontroll. Den menneskelege hovudinngangen er [Dokumentasjonsinngang](INDEX.md).

## Brukarar

- [Full brukarmanual](USER_MANUAL.md)
- [Installasjon](../INSTALL.md)
- [Opplæringsguide](LEARNING_GUIDE.md)
- [Status](STATUS.md)
- [Merkevare og ikon](BRAND.md)

## Normal arbeidsflyt

- [Selfhost-handlingsplan](SELFHOST_HANDLINGSPLAN.md)
- [CI-gates](05-development/SELFHOST_CI_GATES.md)
- [Release-sjekkliste](05-development/SELFHOST_RELEASE_CHECKLIST.md)
- Lokal release-preflight: `./bin/nc release-preflight`
- Streng release-preflight: `./bin/nc release-preflight --strict`
- Samla lokal grønnliste: `./bin/nc local-green`
- Samla streng grønnliste: `./bin/nc local-green --strict`
- App-release: [Linux](LINUX_APP_RELEASE.md), [macOS](MAC_APP_RELEASE.md), [Windows](WINDOWS_APP_RELEASE.md)
- App-installasjon: [Linux](LINUX_APP_INSTALL.md), [Windows](WINDOWS_APP_INSTALL.md), [macOS signering](MAC_APP_SIGNING.md)

## Språk og runtime

- [AST-kontrakt](SELFHOST_PHASE3_AST_CONTRACT_V1.md)
- [Semantisk kjerne](SELFHOST_PHASE3_SEMANTIC_CORE_V1.md)
- [IR og bytecode](SELFHOST_PHASE3_IR_BYTECODE_V1.md)
- [Modulsystem](SELFHOST_PHASE4_MODULE_SYSTEM_V1.md)
- [Native ABI](05-development/NATIVE_CODEGEN_V2_ABI.md)

## Fase- og statusdokument

- [Fase 2 status](SELFHOST_PHASE2_STATUS.md)
- [Fase 3 status](SELFHOST_PHASE3_STATUS.md)
- [Fase 4 status](SELFHOST_PHASE4_STATUS.md)
- [Fase 5 status](SELFHOST_PHASE5_STATUS.md)
- [Fase 6 status](SELFHOST_PHASE6_STATUS.md)

## Historikk

- [Arkivindeks](./_archive/ARCHIVE_INDEX.md)

## Dokumentasjonsregel

Aktiv dokumentasjon skal peike brukaren mot `./bin/nc`, `dist/norscode_native`, selfhost-runtime og Norscode-baserte verktøy. Release-dokumentasjon skal ha lokal preflight før publisering og streng preflight før GitHub/release, og større rydding skal kunne bevisast med `./bin/nc local-green`; før push/tag skal samla grønnliste kunne køyrast som `./bin/nc local-green --strict`. Historiske C/Python-spor skal merkast som arkiv eller historikk.
