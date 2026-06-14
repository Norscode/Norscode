# Selfhost release checklist

Mål:
Gjøre release, installasjon og rollback så forutsigbart at det kan kjøres punkt for punkt utan å måtte bruke vedlikehalds- eller bootstrap-historikk.

## Før release

- [ ] `./bin/nc --version` viser riktig versjon
- [ ] `./bin/nc selfhost-bootstrap-gate` går grønt
- [ ] relevant release-/install-test går grønt
- [ ] release-pakke kan bygges med `bash package-release.sh <versjon>`
- [ ] release/install-flaten krever ikke C-verktøykjede i normal drift
- [ ] `bash tools/selfhost_drift_guard.sh` går grønt før release
- [ ] Linux ELF self-compile-paritet er grøn i GitHub CI eller eksplisitt vurdert for releasen
- [ ] ingen release-instruks peiker brukaren mot `tools/maint/*`, `NORSCODE_BOOTSTRAP_C=1` eller generated-C-løypa

## Bygg release

```bash
bash package-release.sh <versjon>
```

Verifiser:

- [ ] `.tar.gz`-arkivet ble opprettet
- [ ] tilsvarende `.sha256` ble opprettet
- [ ] checksum matcher arkivet
- [ ] release-arkivet inneholder ikke C som normal produktavhengighet
- [ ] release-dokumentasjonen peiker på `bin/nc` / `dist/norscode_native`, ikkje maintainer-laner

## Installer release

```bash
bash tools/install-release.sh release-artifacts/norscode-language-<versjon>.tar.gz --prefix /srv/norscode
```

Verifiser:

- [ ] `current`-lenken peker på riktig versjon
- [ ] `bin/nc --version` virker fra installert release
- [ ] installert `bin/nc` kan kjøre normal kommando i installert miljø
- [ ] installert release krever ikke C-verktøykjede for normal bruk

## Rollback

Hvis en release må rulles tilbake:

1. Pek `current` tilbake til forrige stabile release.
2. Kjør installert `nc --version`.
3. Kjør installert `nc doctor`.
4. Bekreft at tjenesten/brukerflyten er tilbake på forrige stabile versjon.

## Ferdig når

- [ ] Release kan bygges, verifiseres og installeres mekanisk
- [ ] Rollback er dokumentert og repeterbar
- [ ] Brukeren trenger ikke utviklerassistanse for normal installasjon
