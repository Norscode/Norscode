# Frontend Cache Headers

Statiske filer bør leveres med tydelige cache-hoder.

## Mål

- gjøre frontend raskere
- redusere unødvendig trafikk
- holde cachepolicy forutsigbar

## Modell

- statiske assets kan få lang cache
- HTML kan få kortere cache eller ingen cache
- endringer i assets bør være identifiserbare

## Regler

- cache statiske filer aggressivt når de er versjonerte
- cache HTML mer forsiktig
- ha en enkel strategi for invalidasjon

## Når dette er ferdig

- frontend kan levere statiske filer effektivt

Se også [docs/FRONTEND_ASSETS.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ASSETS.md).
