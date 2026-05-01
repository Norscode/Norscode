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

## Migreringshistorikk

- Python-fallback i CLI-wrapperne er fjernet som standard vei.
- `main.py` er nå eksplisitt bootstrap i stedet for skjult fallback.
- `commands`-kommandoen er lagt til som generert kontraktoversikt.

## Kommandooversikt

Bruk `norcode commands` for den genererte, maskinlesbare oversikten over aktive kommandoer.
