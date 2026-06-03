# Selfhost migration and deprecations

Dette dokumentet samler det kanoniske bildet for migrering, legacy-navn og hva som fortsatt er bootstrap i Norscode.
For et samlet kart over aktive og historiske flater, se [`LANE_MAP`](./LANE_MAP.md).
For et samlet kart over historiske dokumenter, se [`ARCHIVE_INDEX`](./ARCHIVE_INDEX.md).

## Kort versjon

- Normal bruk skal møte `Norscode` og `norcode`.
- `./bin/nc` er normal vei for brukere.
- `./bin/bootstrap` er eksplisitt bootstrap og utviklerstøtte.
- `norcode/cli.py` er en historisk modulær CLI-vei som fortsatt kan dukke opp i eldre spor.
- Legacy-navn og gamle filnavn kan eksistere så lenge de trengs for kompatibilitet.
- Den historiske kompatibilitetsflaten er ikke normal produktflyt.
- Migrering skal være dokumentert, ikke gjettet.

## Hva som fortsatt er bootstrap

Dette er fortsatt overgangs- eller støtteflaten:

- `./bin/bootstrap`
- `tools/build-bootstrap-binary.sh`
- `tools/build_norscode_native.sh` (stage-0; ikkje legacy C-VM)
- `scripts/regen_fraser.no` (dev-verktøy utanfor `tools/`)
- eksplisitte bootstrap-veier
- eventuelle gjenværende historiske kompatibilitetsspor i arkivert dokumentasjon

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
6. Behold den historiske referansen kun som eksplisitt legacy, ikke som daglig vei.

## Leserekkefølge

Hvis du er ny bidragsyter, les i denne rekkefølgen:

1. [`START_HER`](./START_HER.md)
2. [`CLI_CONTRACT`](./CLI_CONTRACT.md)
3. [`SELFHOST_STATUS`](./SELFHOST_STATUS.md)
4. [`ARCHIVE_INDEX`](./ARCHIVE_INDEX.md)
5. [`SELFHOST_HANDLINGSPLAN`](./SELFHOST_HANDLINGSPLAN.md)
6. [`SELFHOST_FALLBACK_CONTRACT`](./SELFHOST_FALLBACK_CONTRACT.md)
7. [`SELFHOST_RELEASE_CHECKLIST`](./SELFHOST_RELEASE_CHECKLIST.md)
8. [`SELFHOST_DIAGNOSTICS`](./SELFHOST_DIAGNOSTICS.md)
9. [`LANE_MAP`](./LANE_MAP.md) hvis du trenger å skille aktiv, legacy og arkiv.

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

- [`DEPRECATION_POLICY`](./DEPRECATION_POLICY.md)
- [`LEGACY_POLICY`](./LEGACY_POLICY.md)
- [`MAINTENANCE_POLICY`](./MAINTENANCE_POLICY.md)
- [`SELFHOST_STATUS`](./SELFHOST_STATUS.md)
