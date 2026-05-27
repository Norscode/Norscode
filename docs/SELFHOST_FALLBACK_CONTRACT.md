# Selfhost fallback contract

Mål:
Gjøre det helt tydelig at Python er en eksplisitt nødvei, ikke en skjult standardbane.

## Normalflyt

- `./bin/nc` skal bruke native binary først og `norcode/cli.py` som normal modulær vei når kommandoen er tilgjengelig der.
- Kommandoer som kan kjøres uten Python skal gjøre det uten fallback.
- Brukeren skal ikke trenge å kjenne intern bootstrap-historikk for normal bruk.

## Eksplisitt fallback

Når Python brukes, skal det alltid være fordi brukeren har bedt om det med `--legacy-python-fallback`.

Eksempler:

```bash
./bin/nc --legacy-python-fallback doctor
./bin/nc --legacy-python-fallback smoke
./bin/nc --legacy-python-fallback selfhost-parity --suite m1
```

`--python-fallback` kan fortsatt fungere som kompatibilitetsalias, men det er ikke navnet som skal brukes i ny dokumentasjon.

Fallback skal:

- skrive et tydelig varsel
- være enkel å slå av og på
- ikke være standardvei for vanlige kommandoer

## Hva fallback gjør

- lar overgangs- og diagnosekommandoer kjøre når native binary ikke dekker alt ennå
- gir en eksplisitt vei for debug, bootstrap og historiske verktøy
- fungerer som nødvei når en kommandostøtte ennå ikke er flyttet helt ut av Python

## Hva fallback ikke gjør

- den skal ikke brukes automatisk i normalflyt
- den skal ikke skjule at en kommando er overgangs- eller diagnoseverktøy
- den skal ikke være nødvendig for vanlige brukere i daglig bruk

## Kjent eksplisitt fallback-flate

- `doctor`
- `smoke`
- `selfhost-*` diagnose- og parity-kommandoer
- eksplisitt bootstrap-/utviklerflyt

## Verifikasjon

Disse testene skal holde kontrakten stabil:

- `tests/test_distribution_commands.py`
- eventuelle nye tester for fallback- og bootstrap-veier
