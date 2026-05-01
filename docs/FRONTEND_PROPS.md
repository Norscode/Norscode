# Frontend Props

Props er den eksplisitte input-flaten for frontend-komponenter.

## Kontrakt

- props skal være vanlige parametere
- parametere skal ha tydelige navn
- alle viktige data skal komme inn eksplisitt
- komponenter skal ikke hente skjulte verdier fra global tilstand når de kan få dem som props

## Eksempel

```no
funksjon knapp(tekst: tekst, variant: tekst, disabled: boolsk) -> tekst {
    la klass = "btn btn-" + variant
    hvis disabled {
        returner "<button class=\"" + klass + "\" disabled>" + tekst + "</button>"
    }
    returner "<button class=\"" + klass + "\">" + tekst + "</button>"
}
```

## Retningslinjer

- bruk tekst for labels og innhold
- bruk boolske parametere for tilstand
- bruk lister når en komponent trenger flere elementer
- bruk ordbøker når data naturlig er strukturert
- valider props tidlig hvis komponenten er kritisk

## Når dette er ferdig

- komponenter kan leses som rene funksjoner
- det er tydelig hvilke data hver komponent trenger
- sideoppsett blir mindre avhengig av skjulte konvensjoner

Se også [docs/FRONTEND_COMPONENT_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_COMPONENT_MODEL.md).
