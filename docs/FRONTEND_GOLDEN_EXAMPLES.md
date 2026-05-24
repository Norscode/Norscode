# Frontend Golden Examples

Dette er de to anbefalte inngangene når du vil se hele frontend-modellen i praksis.
Bruk dem som første referanse for nye sider, nye komponenter og ny UI-struktur.

## Component Mode

- Kilde: [`examples/frontend_golden.no`](/Users/jansteinar/Projects/Norscode/examples/frontend_golden.no)
- Test: [`tests/test_frontend_golden.py`](/Users/jansteinar/Projects/Norscode/tests/test_frontend_golden.py)
- Viser: komponent- og layout-komposisjon i en selvstendig, kjørbar variant av samme mønster som `std.frontend` anbefaler

Eksempel på det som bygges:

```no
frontend.layout_app(
    "Norscode Frontend",
    navigasjon,
    innhold,
    "Gull-eksempel for component mode."
)
```

## Native UI Mode

- Kilde: [`examples/native_ui_golden.nui`](/Users/jansteinar/Projects/Norscode/examples/native_ui_golden.nui)
- Test: [`tests/test_native_ui_golden.py`](/Users/jansteinar/Projects/Norscode/tests/test_native_ui_golden.py)
- Viser: `side`, `hero`, `kort`, `liste` og en enkel layout i ren Norscode-syntaks

Eksempel på det som bygges:

```no
side:
    hero:
        tittel "Velkommen til Norscode"
        tekst "Bygg UI direkte med norsk, blokk-basert syntaks."
    kort:
        tittel "Velkommen"
        tekst "Norscode App"
    liste:
        element "HTML mode"
        element "Component mode"
```

## Hvorfor disse to

- De er korte nok til å lese raskt.
- De viser de anbefalte flatenes faktiske inngangspunkter.
- De har egne render-tester som holder dem stabile over tid.

Se også:

- [`docs/FRONTEND_MODEL.md`](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_MODEL.md)
- [`docs/FRONTEND_MODES.md`](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_MODES.md)
- [`docs/FRONTEND_COMPONENT_MODEL.md`](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_COMPONENT_MODEL.md)
- [`docs/FRONTEND_LAYOUT_CONTRACT.md`](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_LAYOUT_CONTRACT.md)
