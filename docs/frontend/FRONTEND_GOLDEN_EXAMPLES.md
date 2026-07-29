# Frontend Golden Examples

Dette er de to anbefalte inngangene når du vil se hele frontend-modellen i praksis.
Bruk dem som første referanse for nye sider, nye komponenter og ny UI-struktur.

For en kort lesesti som binder sammen modell, modes, komponenter og fragmenter, se [`docs/FRONTEND_LEARNING_PATH.md`](./FRONTEND_LEARNING_PATH.md).

## Component Mode

- Kilde: [`examples/frontend_golden.no`](../examples/frontend_golden.no)
- Test: [`tests/test_frontend_golden.py`](../tests/test_frontend_golden.py)
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

- Kilde: [`examples/native_ui_golden.nui`](../examples/native_ui_golden.nui)
- Test: [`tests/test_native_ui_golden.py`](../tests/test_native_ui_golden.py)
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

## Praktisk eksempel

I tillegg er [`examples/frontend.no`](../examples/frontend.no) et godt referansepunkt for server-drevne fragmenter, søk og paginerte lister.

Det eksemplet viser:

- full side og fragment-svar fra samme route
- søk via query-parametre
- paginering og sortering uten JS
- gjenbruk av `std.frontend` og `std.islands`

## Se også

- [`FRONTEND_MODEL`](./FRONTEND_MODEL.md)
- [`FRONTEND_MODES`](./FRONTEND_MODES.md)
- [`FRONTEND_COMPONENT_MODEL`](./FRONTEND_COMPONENT_MODEL.md)
- [`FRONTEND_LAYOUT_CONTRACT`](./FRONTEND_LAYOUT_CONTRACT.md)
- [`FRONTEND_FRAGMENT_EXAMPLES`](./FRONTEND_FRAGMENT_EXAMPLES.md)
