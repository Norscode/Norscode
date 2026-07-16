# CLI Contract

Dette dokumentet beskriver den stabiliserte kontrakten for Norscode-CLI-en.

## Stabil bruk

- Primærkommandoen i repoet er `./bin/nc`.
- Normal kjede er `./bin/nc` → `dist/norscode_native` → NCB JSON / VM / native ELF.
- Eventuelle alias skal ikke få ny funksjonalitet som avviker fra `./bin/nc`.
- Kommandooversikten er tilgjengelig med `./bin/nc help` og `./bin/nc commands`.
- Normal testgate er `./bin/nc test`. Tester som ikkje er klare for rask native-runner skal liggje eksplisitt som `native-unsupported` eller slow i testløparen, ikkje feile tilfeldig i normalflata.

## Exit-koder

- `0`: kommandoen fullførte uten feil.
- `1`: runtime-, verifikasjons- eller valideringsfeil.
- `2`: ugyldig bruk eller parserfeil håndteres av argparse.
- andre koder: kun når underliggende verktøy eksplisitt returnerer det.

## Legacy-policy

- Nye brukere skal møte `Norscode` og `norcode`, ikke `norsklang`.
- Legacy-navn brukes bare for migrering og bakoverkompatibilitet.
- Eventuelle nye brudd skal dokumenteres først, implementeres deretter.

Se også [`docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md`](./SELFHOST_MIGRATION_AND_DEPRECATIONS.md) for den samlede migreringsregelen og leserekkefølgen.

## Migreringshistorikk

- Den historiske fallback-veien i CLI-wrapperne er fjernet som normal vei og skal ikke brukes i aktiv drift.
- `bin/bootstrap` bærer den eksplisitte bootstrap-flaten.
- `commands`-kommandoen er lagt til som kontraktoversikt for den aktive CLI-flata.

## Eksplisitt fallback

Når historiske veier fortsatt trengs, skal det alltid være synlig at de er historikk:

- `bin/bootstrap` er den tydelige veien inn i bootstrap-flaten fra `nc`.
- Vanlige kommandoer skal ikke falle tilbake til den historiske banen i normal bruk.
- Historiske veier skal ikke omtales som standard eller anbefalt vei.
- Fallback-oppførselen er dokumentert i [`docs/SELFHOST_FALLBACK_CONTRACT.md`](./SELFHOST_FALLBACK_CONTRACT.md).

## Kommandooversikt

Bruk `./bin/nc help` eller `./bin/nc commands` for oversikten over aktive kommandoer.

## Verifisert normalflate

Siste lokale gatekøyring, 2026-06-27:

- `./bin/nc test --no-color`: 326 bestått, 0 feilet, 99 hoppet som `native-unsupported` eller slow.
- `./bin/nc feature-check app.no`: bestått.
- `./bin/nc selfhost-bootstrap-gate`: bestått.
- `./bin/nc bootstrap-self`: bestått.
- `./bin/nc verify-seed`: bestått.
- `./bin/nc verify-selvstendighet`: full eigarskaps-, bootstrap-, L5/L5b- og testgate.
- `tests/test_ai.no`: bestått.
