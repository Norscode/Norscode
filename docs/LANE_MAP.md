# Lane Map

Dette dokumentet viser hvor du skal gå avhengig av hva du trenger:

- normal bruk
- eksplisitt bootstrap eller legacy
- ren historikk

## Aktiv normalvei

Dette er veien de fleste bør bruke først:

- [`docs/START_HER.md`](START_HER.md)
- [`docs/CLI_CONTRACT.md`](CLI_CONTRACT.md)
- [`./bin/nc`](../bin/nc)
- [`dist/norscode`](../dist/norscode)

## Eksplisitt bootstrap og legacy

Dette er støtteflater som er dokumenterte, men ikke normal produktvei:

- [`./bin/bootstrap`](../bin/bootstrap)
- [`tools/build-bootstrap-binary.sh`](../tools/build-bootstrap-binary.sh)
- [`tools/bootstrap_wrapper.py`](../tools/bootstrap_wrapper.py)
- legacy-navn som `nor`, `nc`, `nl` og `norsklang`

## Ren historikk

Dette er dokumentasjon og spor som skal leses som arkiv:

- [`docs/ARCHIVE_INDEX.md`](ARCHIVE_INDEX.md)
- migreringsdokumenter som allerede er merket som historikk

## Tommelfingerregel

- Start med normalveien.
- Bruk bootstrap/legacy bare når du feilsøker eller verifiserer overgang.
- Bruk arkivet når du trenger å forstå historikk eller utfasing.
