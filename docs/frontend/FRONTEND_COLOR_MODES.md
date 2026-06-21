# Frontend Color Modes

Frontend bør kunne støtte mørk eller lys modus når det gir verdi.

## Mål

- gjøre brukeropplevelsen mer komfortabel
- la appen tilpasse seg systempreferanser
- holde tema-bytte forutsigbart

## Modell

- lys modus kan være standard
- mørk modus kan legges til som alternativ
- tema bør styres av tydelige tokens
- `std.stil` kan uttrykke dette med `tema_lys(...)`, `tema_mørk(...)` og `tema_data(...)`

## Regler

- bruk bare tolkede tokens, ikke hardkodede farger overalt
- gjør mode-bytte eksplisitt
- sørg for at kontrast fortsatt er god
- del token-kjelde mellom komponenter ved å lese verdier med `token_ref(...)`

## Når dette er ferdig

- appen kan vises i lys og mørk stil uten å brekke layout

Se også [docs/FRONTEND_DESIGN_TOKENS.md](./FRONTEND_DESIGN_TOKENS.md).
