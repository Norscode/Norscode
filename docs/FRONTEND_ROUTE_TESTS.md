# Frontend Route Tests

Side- og route-tester verifiserer at navigasjonen fungerer som den skal.

## Mål

- sikre at riktige sider vises for riktige URL-er
- teste fallback og 404-tilfeller
- verifisere at navigasjon og statisk struktur henger sammen

## Modell

- test route-match mot representative path-er
- test aktive lenker og menytilstand
- test direkte lenker og refresh-flyt

## Regler

- route-tester skal være raske
- test både happy path og feil/sprang
- hold testene nær den faktiske navigasjonsmodellen

## Når dette er ferdig

- appens sideflyt kan endres med trygghet
- URL-er og visninger er stabile

Se også [docs/FRONTEND_NAVIGATION_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_NAVIGATION_MODEL.md).
