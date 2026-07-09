# Frontend HTML

`std.html` er lavnivåflaten for server-rendert HTML i Norscode.
Pakken gir escaping, attributter, vanlige tagger, skjemataggar, tabellar og standard små script-kontrakter.

## Semantiske Tagger

- `section(...)`
- `article(...)`
- `aside(...)`
- `figure(...)`
- `figcaption(...)`
- `dialog(...)`
- `header(...)`
- `main(...)`
- `footer(...)`
- `nav(...)`

## Tekst og Metadata

- `h1(...)`
- `h2(...)`
- `p(...)`
- `span(...)`
- `small(...)`
- `strong(...)`
- `time(...)`

Bruk `std.html` når du vil bygge markup direkte.
Bruk de høyere pakkene når du vil ha ferdige komponenter: `std.html_components`, `std.html_forms`, `std.html_islands`, `std.html_state`, `std.html_router` og `std.html_templates`.
