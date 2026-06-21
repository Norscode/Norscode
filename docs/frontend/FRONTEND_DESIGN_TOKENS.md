# Frontend Design Tokens

Design tokens er den grunnleggende visuelle kontrakten for frontend.

## Mål

- gi konsistent fargebruk
- gi stabil spacing og typografi
- gjøre det lett å bygge flere komponenter med samme språk

## Token-typer

- farger
- spacing
- radius
- skygger
- typografi
- brytepunkter

## Regler

- bruk tokens i stedet for hardkodede verdier
- hold navngiving enkel og forutsigbar
- skill mellom funksjonelle og dekorative tokens
- la `std.stil` modellere tokens som CSS-variabler med norske hjelparar som `token_farge(...)`, `token_spacing(...)` og `token_radius(...)`
- la komponentar lese verdiar via `token_ref(...)` i staden for å gjenta rå verdiar

## Eksempel

- `color.background`
- `color.surface`
- `color.text`
- `space.2`
- `space.4`
- `radius.md`

## Når dette er ferdig

- komponenter kan styles med samme visuelle språk
- design kan endres uten at hver komponent må skrives om

Se også [std/stil.no](../../std/stil.no) og [docs/frontend/FRONTEND_COLOR_MODES.md](./FRONTEND_COLOR_MODES.md).
