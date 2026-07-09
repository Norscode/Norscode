# Selfhost release-sjekkliste

Målet er å gjere release, installasjon og tilbakerulling føreseieleg utan å dra inn vedlikehald eller bootstrap i normalflyten.

## Før release

- [ ] `./bin/nc --version` viser rett versjon
- [ ] lokal preflight går grønt: `./bin/nc release-preflight`
- [ ] streng GitHub/release-preflight går grønt: `./bin/nc release-preflight --strict`
- [ ] samla lokal grønnliste går grønt: `./bin/nc local-green`
- [ ] samla streng grønnliste går grønt: `./bin/nc local-green --strict`
- [ ] relevant release- og installasjonstest går grønt
- [ ] release-pakke kan byggast med `./bin/nc package-release <versjon>`
- [ ] stage-0 release-artefaktar kan byggast med `./bin/nc stage0-release-assets --platform <plattform>`
- [ ] release- og installasjonsflata krev ikkje C-verktøykjede i normal drift
- [ ] `./bin/nc selfhost-drift-guard` går grønt før release
- [ ] Linux Gen1 stage-0-ELF-kandidat blir bygd grønt i GitHub CI
- [ ] Full Linux ELF self-compile-paritet med `NC_OM6B_RUN_STAGE0=1` er anten grøn eller
      eksplisitt vurdert som ikkje-releaseblokkerande for denne releasen
- [ ] inga release-instruks peikar brukaren mot C/Python, `tools/maint/*`, `NORSCODE_BOOTSTRAP_C=1` eller generert C-løype
- [ ] CI-wrapperar under `tools/*.sh` køyrer reelle kontrollar og skjuler ikkje manglande stage0-evner som grøne hopp-over-steg

## Bygg release

```bash
./bin/nc package-release <versjon>
```

Verifiser:

- [ ] `.tar.gz`-arkivet er oppretta
- [ ] tilsvarande `.sha256` er oppretta
- [ ] eventuelle stage-0-artefaktar ligg under `release-artifacts/stage0/` eller valt `--out-dir`, ikkje i repo-rota
- [ ] stage-0 `.sha256` er flyttbar og viser berre filnamnet, ikkje lokal absolutt sti
- [ ] sjekksum stemmer med arkivet
- [ ] release-arkivet inneheld ikkje C som normal produktavhengigheit
- [ ] release-dokumentasjonen peikar på `bin/nc` / `dist/norscode_native`, ikkje historiske vedlikehaldsløyper
- [ ] nye funksjonar er sjekka med `./bin/nc feature-check [fil.no ...]`
- [ ] release/install-flaten krev ikkje C-verktøykjede

## Installer release

```bash
./bin/nc install-release release-artifacts/norscode-language-<versjon>.tar.gz --prefix /srv/norscode
```

Verifiser:

- [ ] `current`-lenken peikar på rett versjon
- [ ] `bin/nc --version` verkar frå installert release
- [ ] installert `bin/nc` kan køyre normal kommando i installert miljø
- [ ] installert release krev ikkje C-verktøykjede for normal bruk

## Tilbakerulling

Viss ei release må rullast tilbake:

1. Peik `current` tilbake til førre stabile release.
2. Køyr installert `nc --version`.
3. Køyr installert `nc doctor`.
4. Stadfest at tenesta eller brukarflyten er tilbake på førre stabile versjon.

## Ferdig når

- [ ] Release kan byggjast, verifiserast og installerast mekanisk utan å referere til vedlikehaldsløyper i normal bruksdokumentasjon
- [ ] Tilbakerulling er dokumentert og repeterbar
- [ ] Brukaren treng ikkje utviklarhjelp for normal installasjon
