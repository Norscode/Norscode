# Frontend Browser Auth

Browser-auth bør håndtere token og cookies på en eksplisitt måte.

## Mål

- gjøre innlogging og autorisasjon enkel
- støtte både token- og cookie-basert flyt
- unngå skjult auth-oppførsel

## Modell

- auth-status kan leses fra en sentral kilde
- token kan sendes i API-klienten når det er riktig
- cookies brukes når det passer bedre for browser-flyt

## Regler

- aldri lagre hemmeligheter utydelig
- skill mellom autentisering og autorisasjon
- la auth state være tilgjengelig for UI når nødvendig

## Når dette er ferdig

- frontend kan sende inn brukeren til riktige sider
- API-kall kan autentiseres på en konsistent måte

Se også:

- [docs/FRONTEND_API_CLIENT.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_API_CLIENT.md)
- [docs/FRONTEND_FEEDBACK_STATES.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_FEEDBACK_STATES.md)
