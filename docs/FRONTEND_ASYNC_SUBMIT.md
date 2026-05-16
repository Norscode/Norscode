# Frontend Async Submit

Async submit betyr at skjemaet kan sende data uten å fryse UI.

## Mål

- gi tydelig feedback mens skjemaet sendes
- hindre dobbeltsending
- holde brukerdata intakt ved feil

## Modell

- submit starter en async handling
- skjemaet går i loading-tilstand
- felter og knapp kan disables mens forespørselen pågår
- ved feil vises melding uten å slette brukerens input

## Regler

- disable submit-knappen under sending
- vurder å disable felt som ikke bør endres underveis
- vis loading tydelig
- bevar feltverdi ved feil

## Når dette er ferdig

- skjemaet føles trygt og responsivt
- brukeren skjønner at innsending pågår

Se også:

- [docs/FRONTEND_FEEDBACK_STATES.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_FEEDBACK_STATES.md)
- [docs/FRONTEND_SERVER_VALIDATION.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_SERVER_VALIDATION.md)
