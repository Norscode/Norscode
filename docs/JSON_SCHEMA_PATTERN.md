# JSON- og schema-mapping

Norscode bruker `std.json` til å parse JSON til `ordbok_tekst`.
For enkle og tydelige schema-kontrakter anbefales eksplisitte mapper-funksjoner per domeneobjekt.

Hovedregelen er:
- parse JSON til `ordbok_tekst`
- konverter felt med `json.hent_tekst`, `json.hent_tall`, `json.hent_bool`, `json.hent_array_tekst` og `json.hent_array_tall`
- bygg et stabilt modellobjekt i en egen funksjon
- bruk samme mapper tilbake til JSON når du skal serialisere

Dette gjør kontrakten lett å lese og lett å teste.

## Anbefalt stil

- `bruker_fra_json(payload)` validerer og mapper innkommende data
- `bruker_til_json(bruker)` serialiserer modellen tilbake
- `påkrevd_felt(...)` kan være en liten hjelpefunksjon hvis du vil gi tydelig feil

## Eksempel

Se [examples/json_schema.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/json_schema.no) for en praktisk og minimal mapper.

## Når dette er nok

Denne stilen er riktig valg når:
- schemaet er lite eller moderat stort
- du vil ha tydelig kontroll over hvilke felt som aksepteres
- du vil holde runtime og API enkelt

Når schemaet blir stort, bør du fortsatt dele det opp i små mapper per modell i stedet for å samle alt i én stor funksjon.
