# Frontend Component Model

Frontendens første komponentmodell i Norscode bør være enkel og funksjonell.
Den hører først og fremst hjemme i component mode, men kan også brukes fra template- og native UI-lag.

Se også de lesbare gull-eksemplene i [`docs/FRONTEND_GOLDEN_EXAMPLES.md`](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_GOLDEN_EXAMPLES.md).

Den anbefalte flaten er nå samlet i [`std/frontend.no`](/Users/jansteinar/Projects/Norscode/std/frontend.no), mens [`std/html.no`](/Users/jansteinar/Projects/Norscode/std/html.no) fortsatt er den lavnivå, eksplisitte HTML-byggesteinen:

- komponenter er rene funksjoner som returnerer HTML-vennlig tekst
- komponenter tar inn eksplisitte parametere
- komponenter kan settes sammen hierarkisk
- layout og innhold skal kunne kombineres uten skjult global state
- bruk `std.html` for `tag(...)`, `void_tag(...)`, `class_attr(...)`, `page(...)` og vanlige tag-hjelpere som `div(...)`, `section(...)`, `h1(...)` og `p(...)` der det reduserer rå string-konkatenasjon
- bruk `std.frontend` for anbefalte layout- og komponenthjelpere som `layout_main(...)`, `layout_app(...)`, `nav_link(...)`, `komponent_kort(...)`, `komponent_nav(...)`, `komponent_liste(...)`, `komponent_tabell(...)` og `slot(...)`

`std.frontend` utvider dette med noen sammensatte komponenter som gjør sidebygging raskere og renere:

- `komponent_hero(...)`
- `komponent_alert(...)`
- `komponent_empty(...)`
- `komponent_loading(...)`

## Grunnprinsipper

- Bruk små, navngitte komponenter.
- La komponenter være enkle å lese i kode.
- Returner ferdig markup, ikke muter global tilstand.
- Gjør dataflyten eksplisitt via parametere.

## Foreslått form

```no
funksjon knapp(tekst: tekst, variant: tekst) -> tekst {
    returner "<button class=\"btn btn-" + variant + "\">" + tekst + "</button>"
}
```

## Komponenttyper

- atomiske komponenter: knapper, badge, ikon, input
- sammensatte komponenter: kort, panel, tabell, liste
- layouts: header, footer, main shell
- sider: helhetlige visninger bygget av komponenter

## Slots og barn-innhold

Når en komponent trenger innhold inne i seg selv, bør vi støtte en enkel slot-/barn-modell:

- en komponent tar inn et `innhold`
- layouten pakker rundt innholdet
- barn-innhold kan være ren tekst eller et ferdig HTML-biteresultat

## Hva dette betyr i praksis

Et første frontend-prosjekt bør kunne bruke:

- `frontend/components/`
- `frontend/layouts/`
- `frontend/pages/`
- `std.frontend` som standard inngang for gjenbrukbare komponenter
- `std.html` som lavnivå-verktøy når du trenger full kontroll

Se også [docs/FRONTEND_STRUCTURE.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_STRUCTURE.md) og [docs/FRONTEND_LAYOUT_CONTRACT.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_LAYOUT_CONTRACT.md).
