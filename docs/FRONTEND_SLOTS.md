# Frontend Slots

Slots og barn-innhold gjør det mulig å bygge layout og sammensatte komponenter uten å kopiere markup.

Se også de gull-eksemplene i [`docs/FRONTEND_GOLDEN_EXAMPLES.md`](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_GOLDEN_EXAMPLES.md).

## Kontrakt

- en komponent kan ta inn `innhold`
- layouten kan pakke inn `innhold`
- barn-innhold kan være tekst eller ferdig bygget markup

## Foreslått bruk

```no
bruk std.frontend som frontend

funksjon layout_main(tittel: tekst, innhold: tekst) -> tekst {
    returner frontend.layout_main(tittel, innhold)
}
```

## Regler

- bruk slots når en komponent trenger innkapslet innhold
- bruk eksplisitte parametere for alt annet
- hold nesting enkel og forutsigbar
- unngå skjult magi rundt children-data

## Når dette er ferdig

- layouts kan motta sideinnhold direkte
- cards/panels kan pakke inn egen body
- sider kan komponeres av små, gjenbrukbare biter

Se også [docs/FRONTEND_COMPONENT_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_COMPONENT_MODEL.md).
