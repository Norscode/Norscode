# Selfhost Release Checklist

Målet er å gjere release, installasjon og rollback føreseieleg utan å dra inn vedlikehald eller bootstrap i normalflyten.

## Før release

- [ ] `./bin/nc --version` viser rett versjon
- [ ] relevant release- og installasjonstest går grønt
- [ ] release-pakke kan byggast med `bash package-release.sh <versjon>`
- [ ] release- og installasjonsflata krev ikkje C-verktøykjede i normal drift
- [ ] `bash tools/selfhost_drift_guard.sh` går grønt før release
- [ ] Linux ELF self-compile-paritet er grøn i GitHub CI eller eksplisitt vurdert for releasen
- [ ] inga release-instruks peikar brukaren mot `tools/maint/*`, `NORSCODE_BOOTSTRAP_C=1` eller generert C-løype som normal bruk

## Bygg release

```bash
bash package-release.sh <versjon>
```

Verifiser:

- [ ] `.tar.gz`-arkivet er oppretta
- [ ] tilsvarande `.sha256` er oppretta
- [ ] sjekksum stemmer med arkivet
- [ ] release-arkivet inneheld ikkje C som normal produktavhengigheit
- [ ] release-dokumentasjonen peikar på `bin/nc` / `dist/norscode_native`, ikkje historiske vedlikehaldsløyper
- [ ] release/install-flaten krever ikke C-verktøykjede

## Installer release

```bash
bash tools/install-release.sh release-artifacts/norscode-language-<versjon>.tar.gz --prefix /srv/norscode
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
