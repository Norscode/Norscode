# Frontend Component Model

Frontendens første komponentmodell i Norscode bør være enkel og funksjonell:

- komponenter er rene funksjoner som returnerer HTML-vennlig tekst
- komponenter tar inn eksplisitte parametere
- komponenter kan settes sammen hierarkisk
- layout og innhold skal kunne kombineres uten skjult global state

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

Se også [docs/FRONTEND_STRUCTURE.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_STRUCTURE.md) og [docs/FRONTEND_LAYOUT_CONTRACT.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_LAYOUT_CONTRACT.md).
