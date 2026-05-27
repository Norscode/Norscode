# CLI Contract

Dette dokumentet beskriver den stabiliserte kontrakten for Norscode-CLI-en.

## Stabil bruk

- Primærkommandoen er `norcode`.
- Legacy-aliasene `nor`, `nc`, `nl` og `norsklang` er kun kompatibilitetsskjell og skal ikke få ny funksjonalitet som avviker fra `norcode`.
- Kommandooversikten er tilgjengelig med `norcode commands`.

## Exit-koder

- `0`: kommandoen fullførte uten feil.
- `1`: runtime-, verifikasjons- eller valideringsfeil.
- `2`: ugyldig bruk eller parserfeil håndteres av argparse.
- andre koder: kun når underliggende verktøy eksplisitt returnerer det.

## Legacy-policy

- Nye brukere skal møte `Norscode` og `norcode`, ikke `norsklang`.
- Legacy-navn brukes bare for migrering og bakoverkompatibilitet.
- Eventuelle nye brudd skal dokumenteres først, implementeres deretter.

Se også [`docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md) for den samlede migreringsregelen og leserekkefølgen.

## Migreringshistorikk

- Python-fallback i CLI-wrapperne er fjernet som standard vei.
- `norcode/legacy_main.py` og `norcode/bootstrap/python_entry.py` bærer nå den eksplisitte Python-kompatibiliteten.
- `norcode/cli.py` bruker modulær registry-dispatch som normal vei.
- `commands`-kommandoen er lagt til som generert kontraktoversikt.

## Eksplisitt fallback

Når Python fortsatt trengs, skal det alltid være synlig og eksplisitt:

- `--legacy-python-fallback` er den tydelige veien inn i Python-flaten fra `nc`.
- `--python-fallback` kan fortsatt fungere som kompatibilitetsalias, men skal ikke brukes i ny dokumentasjon.
- Vanlige kommandoer skal ikke falle tilbake til Python uten at brukeren ber om det.
- Fallback-oppførselen er dokumentert i [`docs/SELFHOST_FALLBACK_CONTRACT.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_FALLBACK_CONTRACT.md).

## Kommandooversikt

Bruk `norcode commands` for den genererte, maskinlesbare oversikten over aktive kommandoer.
