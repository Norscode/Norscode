# Fil- og objektlagring

Norscode har allerede to nyttige standardbaner:

- `std.fil` for rå filoperasjoner
- `std.lagring` for strukturert objektlagring via JSON

Anbefalingen er å bruke dem slik:

- `std.fil` for tekst, logg, maler og enkle eksportfiler
- `std.lagring` for appdata, config, cache og andre små modellobjekter
- hold filstier og lagringsnøkler i egne hjelpesfunksjoner
- bruk `std.env` og `std.path` for portable temp- og produksjonsstier

## Anbefalt stil

- `les_fil(...)` og `skriv_fil(...)` for ustrukturert tekst
- `last_objekt(...)` og `lagre_objekt(...)` for strukturert data
- ikke bland rå JSON-strenger med domenelogikk i handlerne

## Eksempel

Se [examples/file_object_storage.no](/Users/jansteinar/Projects/language_handoff/projects/language/examples/file_object_storage.no).

Eksemplet viser:
- portabel tempsti
- lagring av rå tekst med `std.fil`
- lagring og lesing av strukturert objektdata med `std.lagring`

Dette er den anbefalte standardflaten for både små verktøy og backend-nære tjenester.
