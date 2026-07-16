# Selfhost Release Checklist

Målet er å gjere release, installasjon og rollback føreseieleg utan å dra inn vedlikehald eller bootstrap i normalflyten.

## Før release

- [ ] `./bin/nc --version` viser rett versjon
- [ ] relevant release- og installasjonstest går grønt
- [ ] relevante punkt i [10/10 modenhetsplan](../MODENHET_10_10.md) har kodebevis, bruksbevis og gatebevis
- [ ] release-pakke kan byggast med `NORSCODE_RELEASE_VERSION=<versjon> NORSCODE_ENABLE_EXEC_PROSESS=1 ./bin/nc run tools/package_release.no`
- [ ] release- og installasjonsflata krev ikkje C-verktøykjede i normal drift
- [ ] `./bin/nc run tools/selfhost_drift_guard.no` går grønt før release
- [ ] Linux ELF self-compile-paritet er grøn i GitHub CI eller eksplisitt vurdert for releasen
- [ ] inga release-instruks peikar brukaren mot C/Python, `tools/maint/*`, `NORSCODE_BOOTSTRAP_C=1` eller generert C-løype

## Bygg release

```bash
NORSCODE_RELEASE_VERSION=<versjon> NORSCODE_ENABLE_EXEC_PROSESS=1 ./bin/nc run tools/package_release.no
```

Verifiser:

- [ ] `.tar.gz`-arkivet er oppretta
- [ ] tilsvarande `.sha256` er oppretta
- [ ] sjekksum stemmer med arkivet
- [ ] release-arkivet inneheld ikkje C som normal produktavhengigheit
- [ ] release-dokumentasjonen peikar på `bin/nc` / `dist/norscode_native`, ikkje historiske vedlikehaldsløyper
- [ ] nye funksjonar er sjekka med `./bin/nc feature-check [fil.no ...]`
- [ ] release/install-flaten krever ikke C-verktøykjede

## Installer release

```bash
NORSCODE_INSTALL_ARCHIVE=release-artifacts/norscode-language-<versjon>.tar.gz NORSCODE_INSTALL_PREFIX=/srv/norscode ./bin/nc run tools/install-release.no
```

Verifiser:

- [ ] `current`-lenken peikar på rett versjon
- [ ] `bin/nc --version` verkar frå installert release
- [ ] installert `bin/nc` kan køyre normal kommando i installert miljø
- [ ] installert release krev ikkje C-verktøykjede for normal bruk

## Rollback

Viss ei release må rullast tilbake:

1. Pek `current` tilbake til forrige stabile release.
2. Køyr installert `nc --version`.
3. Køyr installert `nc doctor`.
4. Bekreft at tenesta eller brukarflyten er tilbake på forrige stabile versjon.

## Ferdig når

- [ ] Release kan byggjast, verifiserast og installerast mekanisk utan å referere til vedlikehaldsløyper i normal bruksdokumentasjon
- [ ] Rollback er dokumentert og repeterbar
- [ ] Brukaren treng ikkje utviklarhjelp for normal installasjon
