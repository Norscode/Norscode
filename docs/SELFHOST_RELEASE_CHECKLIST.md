# Selfhost release checklist

Mål:
Gjøre release, installasjon og rollback så forutsigbart at det kan kjøres punkt for punkt uten å kjenne bootstrap-historikken.

## Før release

- [ ] `./bin/nc --version` viser riktig versjon
- [ ] `./bin/nc --legacy-python-fallback doctor` går grønt via den gjenværende Python-kompatibiliteten
- [ ] `./bin/nc --legacy-python-fallback smoke` går grønt via den gjenværende Python-kompatibiliteten
- [ ] `python3 -m pytest tests/test_distribution_commands.py tests/test_release_install_flow.py` går grønt
- [ ] release-pakke kan bygges med `bash package-release.sh <versjon>`

## Bygg release

```bash
bash package-release.sh <versjon>
```

Verifiser:

- [ ] `.tar.gz`-arkivet ble opprettet
- [ ] tilsvarende `.sha256` ble opprettet
- [ ] checksum matcher arkivet

## Installer release

```bash
bash tools/install-release.sh release-artifacts/norscode-language-<versjon>.tar.gz --prefix /srv/norscode
```

Verifiser:

- [ ] `current`-lenken peker på riktig versjon
- [ ] `bin/nc --version` virker fra installert release
- [ ] `bin/nc --legacy-python-fallback doctor` kan kjøres i installert miljø via den gjenværende Python-kompatibiliteten

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
