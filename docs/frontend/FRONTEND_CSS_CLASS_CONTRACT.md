# Frontend CSS Class Contract

Dette dokumentet samler de CSS-klassefamiliene som Norscode-frontenden bruker som standard.
Målet er å gjøre Native UI, component mode og de gjenbrukbare partialsene mer forutsigbare.

## Prinsipper

- Bruk enkle, stabile klassefamilier.
- Bruk `block`, `block__element` og `block--variant` der det gir mening.
- Ikke legg inn tilfeldige klasseavvik mellom samme mønster i ulike moduser.
- La testene låse de viktigste klassefamiliene.

## Standard familier

- `shell`
- `app-header`, `app-header__brand`, `app-header__nav`
- `app-main`
- `app-footer`
- `hero`, `hero__actions`
- `card`, `card-body`, `panel`, `panel__body`
- `nav`
- `list`
- `table`
- `alert`, `alert-title`, `alert-body`
- `empty-state`
- `loading-state`, `loading-state__spinner`
- `form`, `form-field`, `form-label`, `form-input`, `form-textarea`, `form-select`
- `form-choice`, `form-choice__input`, `form-choice__text`
- `form-switch`, `form-switch__input`, `form-switch__text`
- `actions`
- `badge`
- `chip`, `chip__text`
- `avatar`, `avatar__image`, `avatar__text`
- `dialog`, `dialog__surface`, `dialog__body`
- `toast`, `toast__actions`
- `breadcrumb`, `breadcrumb__list`, `breadcrumb__item`, `breadcrumb__item--current`
- `toolbar`, `toolbar__actions`
- `sidebar`, `sidebar__layout`, `sidebar__rail`, `sidebar__main`, `sidebar__section`
- `grid`, `grid__items`, `grid__col`
- `tabs`, `tabs__nav`, `tabs__list`, `tabs__tab`, `tabs__content`, `tabs__trigger`
- `accordion`, `accordion__list`, `accordion__item`, `accordion__content`

## Regler for bruk

- Nye komponenter skal gjenbruke en eksisterende familie når de representerer samme UI-mønster.
- Nye varianter skal være tydelig merket med `--variant` eller et nytt `__element` når det er nødvendig.
- Endringer i klassefamiliene bør dekkes av render-tester i `tests/test_html.no` og/eller `tests/test_native_ui.no`.

## Relaterte dokumenter

- [`docs/FRONTEND_COMPONENT_LIBRARY.md`](./FRONTEND_COMPONENT_LIBRARY.md)
- [`docs/FRONTEND_FEEDBACK_STATES.md`](./FRONTEND_FEEDBACK_STATES.md)
- [`docs/FRONTEND_MODES.md`](./FRONTEND_MODES.md)
