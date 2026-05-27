# Selfhost migration and deprecations

Dette dokumentet samler det nye, kanoniske bildet for migrering, legacy-navn og hva som fortsatt er bootstrap i Norscode.

## Kort versjon

- Normal bruk skal møte `Norscode` og `norcode`.
- `./bin/nc` er normal vei for brukere.
- `./bin/bootstrap` og `./bin/nc --legacy-python-fallback` er eksplisitt bootstrap og utviklerstøtte.
- `norcode/cli.py` er normal modulær CLI-vei, mens `norcode/legacy_main.py` og `norcode/bootstrap/python_entry.py` er eksplisitt Python-kompatibilitet.
- Legacy-navn og gamle filnavn kan eksistere så lenge de trengs for kompatibilitet.
- Migrering skal være dokumentert, ikke gjettet.

## Hva som fortsatt er bootstrap

Dette er fortsatt overgangs- eller støtteflaten:

- `./bin/bootstrap`
- `./bin/nc --legacy-python-fallback`
- `norcode/legacy_main.py`
- `norcode/bootstrap/python_entry.py`
- aktive wrappers bruker `python -m norcode.legacy_main` når fallback er eksplisitt valgt
- `tools/build-bootstrap-binary.sh`
- `tools/bootstrap_wrapper.py`
- eksplisitte fallback-veier som `--legacy-python-fallback`

Dette er ikke normal produktflyt. Det er støtte for bygg, verifisering, overgang og feilsøking.

## Hva som er kanonisk

Dette er den normale brukerflaten:

- `norcode`
- `./bin/nc`
- `norcode.toml`
- `.norcode/`
- dokumentasjonen under `docs/`

## Legacy-navn og aliaser

Legacy-navn og aliaser er kun for kompatibilitet:

- `nor`
- `nc`
- `nl`
- `norsklang`

Regel:

- nye brukere skal møte `Norscode` og `norcode`
- legacy-navn skal ikke få ny primærfunksjonalitet
- nye prosjekter skal bruke `norcode.toml` og `.norcode/`

## Migreringssti

Når du flytter et gammelt prosjekt eller en gammel arbeidsflyt:

1. Bytt brukerflate til `norcode`.
2. Bytt wrapper- eller scriptkall til `./bin/nc`.
3. Flytt konfigurasjon til `norcode.toml` og `.norcode/`.
4. Bruk eksplisitt bootstrap bare der det virkelig trengs.
5. Verifiser med `norcode diagnose`, `norcode doctor`, `norcode smoke` og `norcode ci`.

## Leserekkefølge

Hvis du er ny bidragsyter, les i denne rekkefølgen:

1. [`docs/START_HER.md`](/Users/jansteinar/Projects/Norscode/docs/START_HER.md)
2. [`docs/CLI_CONTRACT.md`](/Users/jansteinar/Projects/Norscode/docs/CLI_CONTRACT.md)
3. [`docs/SELFHOST_STATUS.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_STATUS.md)
4. [`docs/SELFHOST_BOOTSTRAP_INVENTORY.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_BOOTSTRAP_INVENTORY.md)
5. [`docs/SELFHOST_REMAINING_ROADMAP.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_REMAINING_ROADMAP.md)
6. [`docs/SELFHOST_FALLBACK_CONTRACT.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_FALLBACK_CONTRACT.md)
7. [`docs/SELFHOST_RELEASE_CHECKLIST.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_RELEASE_CHECKLIST.md)
8. [`docs/SELFHOST_DIAGNOSTICS.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_DIAGNOSTICS.md)

## Hvorfor bootstrap fortsatt finnes

Bootstrap finnes fortsatt fordi Norscode fortsatt trenger:

- bygg av release-artifacts
- eksplisitt fallback for sammenligning og feilsøking
- installasjon og verifisering av eldre eller midlertidige flyter
- kompatibilitet med eldre prosjekter og gamle aliaser

Det viktige er at bootstrap er tydelig merket og ikke er standardbanen for vanlige brukere.

## Deprecation-regel

Når noe skal fases ut:

- dokumenter det først
- legg inn migreringssti
- behold kompatibilitet så lenge det er rimelig
- fjern det først når den nye veien er godt etablert

## Se også

- [`docs/DEPRECATION_POLICY.md`](/Users/jansteinar/Projects/Norscode/docs/DEPRECATION_POLICY.md)
- [`docs/LEGACY_POLICY.md`](/Users/jansteinar/Projects/Norscode/docs/LEGACY_POLICY.md)
- [`docs/MAINTENANCE_POLICY.md`](/Users/jansteinar/Projects/Norscode/docs/MAINTENANCE_POLICY.md)
- [`docs/SELFHOST_STATUS.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_STATUS.md)
