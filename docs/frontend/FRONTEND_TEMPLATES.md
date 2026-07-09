# Frontend Templates

`std.html_templates` gir gjenbrukbare side- og innholdsformer oppå `std.html` og `std.html_components`.
Pakken passer når flere sider skal dele samme struktur uten å bygge et større template-system.

## Byggeklosser

- `sidehode(...)`
- `innholdsseksjon(...)`
- `artikkel(...)`
- `liste_side(...)`

## Ferdigsider

- `dokumentasjon_side(...)`
- `blogg_side(...)`
- `produkt_side(...)`

Eksempel:

```norscode
bruk std.html_templates som tmpl

tmpl.liste_side(
    "Saker",
    "Alle åpne saker",
    "<ul><li>Sak 1</li></ul>",
    "Ingen saker funnet",
    "/app.css"
)
```

