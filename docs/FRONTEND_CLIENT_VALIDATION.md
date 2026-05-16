# Frontend Client Validation

Klientvalidering skal gi rask og tydelig tilbakemelding før submit.

## Mål

- fange enkle feil tidlig
- hjelpe brukeren med å rette input
- redusere unødvendige serverkall

## Modell

- validering kjører på felt og skjema
- feil vises nær feltet
- submit kan blokkeres når obligatoriske regler feiler

## Regler

- valider enkelt og tydelig
- ikke skjul serverregler i klientkode
- hold meldinger forståelige
- gi brukeren nok informasjon til å rette feilen

## Typiske regler

- påkrevde felt må fylles ut
- e-post må ha riktig format
- tall må være innenfor gyldig område
- passord må oppfylle minimumskrav

## Når dette er ferdig

- brukeren får umiddelbar feedback
- skjemaer blir enklere å bruke uten å vente på serveren

Se også [docs/FRONTEND_FORM_BINDING.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_FORM_BINDING.md).
