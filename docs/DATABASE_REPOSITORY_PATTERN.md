# Repository- og modelmønstre

Dette prosjektet anbefaler at databasekode organiseres i små repository-moduler.

Hovedregelen er enkel:
- én modul per domeneområde
- all SQL bor i repository-modulen
- applikasjonskode kaller repository-funksjoner, ikke rå SQL direkte
- skrivende operasjoner går helst gjennom `db.transaction(...)`
- repository-funksjoner returnerer enkle verdier eller modeller, ikke SQL-detaljer

## Anbefalt form

Et repository bør typisk se slik ut:

- `opprett(...)` for inserts
- `hent(...)` for oppslag
- `list(...)` for lister
- `oppdater(...)` for endringer
- `slett(...)` for fjerning

Modeller bør være små og stabile. I dagens Norscode-flate betyr det ofte:
- en `ordbok_tekst` for JSON-liknende data
- en enkel struktur eller ordbok med faste felter

## Eksempel

Se [examples/db_repository.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/db_repository.no) for en liten og praktisk repository-variant.

Eksemplet viser:
- oppretting av tabell
- innssetting av data via repository
- lesing av data via repository
- bruk av `db.transaction(...)` for skrivende operasjoner

## Anbefaling

For små tjenester er dette nok:
- lag en `repo`-modul per tabell eller domene
- kall `db.pool(...)` eller `db.open(...)` i oppstart
- send `handle` inn i repository-funksjonene
- bruk kun repository-funksjoner fra handlerne

Det gir en ryddig grense mellom:
- HTTP/web-laget
- domenelogikk
- database
