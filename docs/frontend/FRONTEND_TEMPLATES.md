# Frontend Templates

`std.html_templates` gir gjenbrukbare side- og innholdsformer oppå `std.html` og `std.html_components`.
Pakken passer når flere sider skal dele samme struktur uten å bygge et større template-system.

## Byggeklosser

- `sidehode(...)`
- `innholdsseksjon(...)`
- `artikkel(...)`
- `liste_side(...)`
- `detalj_side(...)`
- `dashboard_side(...)`
- `admin_side(...)`

## Ferdigsider

- `dokumentasjon_side(...)`
- `blogg_side(...)`
- `produkt_side(...)`

`detalj_side(...)` passer for ticket-, bruker- og admin-detaljer med metadata og hovedinnhold.
`dashboard_side(...)` passer for sider med metrikkrad og arbeidsinnhold.
`admin_side(...)` gir en enkel administrasjonsside med navigasjon og body.

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

Eksempel med dashboard:

```norscode
tmpl.dashboard_side(
    "Dashboard",
    "Oversikt",
    "<p>12 åpne saker</p>",
    "<p>Siste aktivitet</p>",
    "/app.css"
)
```
