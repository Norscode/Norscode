# Frontend Form Binding

Skjema-binding kobler inputfelt til data og state.

## Mål

- holde skjemaer enkle å bygge
- gjøre feltverdier eksplisitte
- redusere duplisering mellom UI og data

## Modell

- hvert felt binder til en navngitt verdi
- skjemaet har en samlet data-modell
- bindingen skal være lesbar og forutsigbar

## Regler

- bruk tydelige feltnavn
- hold feltverdier i komponentstate eller en dedikert form-state
- gjør submit-data lett å serialisere
- la bindingen være synlig i koden

## Praktisk eksempel

- `navn` binder til `form.navn`
- `alder` binder til `form.alder`
- `aktiv` binder til `form.aktiv`

## Når dette er ferdig

- skjemaer kan bygges uten manuell synkronisering av hvert felt
- data og UI henger tydelig sammen

Se også [docs/FRONTEND_INPUT_COMPONENTS.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_INPUT_COMPONENTS.md).
