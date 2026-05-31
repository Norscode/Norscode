# Frontend Error and Loading States

Feil- og loading-tilstander bør være en standard del av frontend-designen.

## Mål

- gjøre appen tydelig når data lastes
- vise feil på en forståelig måte
- unngå at brukergrensesnittet virker “frosset”

## Modell

- loading-state vises mens data hentes eller handlinger pågår
- feil-state vises når noe går galt
- tom-state vises når data finnes, men er tomme

## Regler

- bruk tydelige labels
- vis hva brukeren kan gjøre videre
- skill mellom nettverksfeil, valideringsfeil og tomt innhold
- unngå å skjule feil i generisk tekst

## Eksempler

- spinner eller skeleton ved lasting
- feilmelding med retry-tilbud
- tom visning med kort forklaring
- bygg disse tilfellene med `std.html.loading_state(...)`, `std.html.alert(...)` og `std.html.empty_state(...)` der det passer
- bruk klassefamiliene i [docs/FRONTEND_CSS_CLASS_CONTRACT.md](./FRONTEND_CSS_CLASS_CONTRACT.md) når du utvider disse mønstrene

## Når dette er ferdig

- brukeren skjønner alltid hva appen gjør
- loading og feil er en del av standardkomponentene

Se også:

- [docs/FRONTEND_STATE_MODEL.md](./FRONTEND_STATE_MODEL.md)
- [docs/FRONTEND_CACHE_MODEL.md](./FRONTEND_CACHE_MODEL.md)
