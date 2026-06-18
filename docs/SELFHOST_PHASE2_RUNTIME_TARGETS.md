# Selfhost Fase 2 - Runtime-mål

Dette dokumentet vel dei første runtime-delane som skal gå frå plan til konkret arbeid.

## Valde mål

### 1. `map_memory_page`

Grunn:

- representerer den mest grunnleggjande minnestyringa
- har ein enkel kontrakt
- viser om runtime-kjernen er stabil nok til å handtere kontrollert ressursbruk

Mål:

- eksakt feilmelding når manager manglar
- tydeleg `sann`/`usann`-semantikk
- ingen skjulte sideeffektar utover registrert mapping

### 2. `enqueue_io`

Grunn:

- viser om async I/O-løypa kan bli styrt eksplisitt
- gir ein enkel veg inn i scheduler/IO-integrasjon
- er lett å måle og teste trinnvis

Mål:

- eksakt feilmelding når runtime manglar
- enkel, føreseieleg kø-oppførsel
- klår dokumentasjon for vidare `process_io`-flyt

## Sekundær kandidat

### 3. `process_io`

Grunn:

- bygger vidare på same flate som `enqueue_io`
- gir eit tydeleg “før/etter”-signal for runtime-arbeidet

Mål:

- dra operasjonar frå kø til fullført liste
- ingen stille dropp av operasjonar
- lett å bruke i minimal regressjonstest

## Akseptkriterium

- Dei valde funksjonane står i ABI-minimumsdokumentet
- Ein lesar kan sjå kvifor nett desse er valde
- Dokumentasjonen er kort nok til å fungere som arbeidsreferanse
