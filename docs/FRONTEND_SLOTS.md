# Frontend Slots

Slots og barn-innhold gjør det mulig å bygge layout og sammensatte komponenter uten å kopiere markup.

## Kontrakt

- en komponent kan ta inn `innhold`
- layouten kan pakke inn `innhold`
- barn-innhold kan være tekst eller ferdig bygget markup

## Foreslått bruk

```no
funksjon layout_main(tittel: tekst, innhold: tekst) -> tekst {
    returner "<main><h1>" + tittel + "</h1>" + innhold + "</main>"
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

Se også [docs/FRONTEND_COMPONENT_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_COMPONENT_MODEL.md).
