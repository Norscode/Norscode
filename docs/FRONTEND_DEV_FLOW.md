# Frontend Dev Flow

Dette er den enkle arbeidsflyten for frontend-ressurser i Norscode.

## Mål

- gjøre det lett å jobbe med en frontend lokalt
- holde statiske ressurser og appkode tydelig adskilt
- unngå å introdusere unødvendig byggekompleksitet tidlig

## Anbefalt flyt

1. rediger sider, komponenter og layout i `frontend/`
2. rediger statiske filer i `frontend/assets/`
3. kjør appen gjennom `norcode serve`
4. verifiser respons og styling i browser
5. legg inn tester for sideflyt og komponenter når de finnes

## Lokalt oppsett

- frontend-koden ligger samlet under `frontend/`
- `frontend/assets/css/app.css` er første stylesheet
- statiske filer kan serveres direkte som del av appen
- utvikling skjer primært med `norcode serve`

## Hva vi bevisst ikke gjør ennå

- ingen tung bundler som standard
- ingen separat frontend-pipeline før vi trenger det
- ingen SPA-kompleksitet før appen faktisk krever det

## Neste steg

Se [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ROADMAP.md).
