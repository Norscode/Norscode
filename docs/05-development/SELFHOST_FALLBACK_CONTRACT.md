# Selfhost fallback contract

Mål:
Gjøre det helt tydelig at normalflyt er native, og at historiske veier kun er historikk eller isolert vedlikehald.

## Normalflyt

- `./bin/nc` skal bruke native binary først.
- Kommandoer som kan kjøres uten legacy-veier skal gjøre det uten ekstra flagg.
- Brukeren skal ikke trenge å kjenne intern bootstrap-historikk for å bruke normalvegen.

## Historisk fallback

Historiske bootstrap- og legacy-veier kan fortsatt finnes i arkivert dokumentasjon, men de skal ikkje beskrives som ein del av normal drift.
Fallback-banen er historisk og skal ikkje brukes som anbefalt vei.

## Hva arkiverte veier dekker

- overgangs- og historiske beskrivelser for eldre bootstrap-flyt
- dokumentasjon av tidligere diagnostikk- og migreringssteg

## Hva de ikke gjør

- de skal ikke brukes i normalflyt
- de skal ikke skjule at de er historiske eller utdaterte
- de skal ikke være nødvendig for vanlige brukere i daglig bruk

## Verifikasjon

- normal CLI skal fungere uten legacy-flag
- installasjonsskript skal feile tydelig uten å foreslå fallback-flyten
- dokumentasjon skal omtale legacy-veier som historikk, ikkje aktiv standard
