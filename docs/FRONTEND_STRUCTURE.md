# Frontend Structure

Dette er den foreslåtte grunnstrukturen for en Norscode-frontend-app.
Den er laget for server-renderte sider med komponentbasert templating som første modell.

## Foreslått struktur

```text
frontend/
├── README.md
├── app.no
├── pages/
├── components/
├── layouts/
├── assets/
│   ├── css/
│   ├── images/
│   └── icons/
├── state/
├── routes.no
└── tests/
```

## Hva mappene brukes til

- `frontend/app.no` er appens inngangspunkt
- `frontend/pages/` inneholder sider og route-nære handlerfunksjoner
- `frontend/components/` inneholder gjenbrukbare UI-komponenter
- `frontend/layouts/` inneholder felles side-skjelett
- `frontend/assets/` inneholder statiske filer
- `frontend/state/` inneholder delt app-state og små datahjelpere
- `frontend/routes.no` samler registrering av ruter og sidekoblinger
- `frontend/tests/` inneholder side- og komponenttester

## Prinsipper

- Hold sider små og lesbare.
- Skill layout fra sideinnhold.
- La komponenter være rene funksjoner der det er mulig.
- Gjør statiske ressurser eksplisitte og lette å cache.
- Behold frontend-ressurser samlet under én tydelig rot.

## Minimal start

En første app trenger bare:

- `app.no`
- én side i `pages/`
- én layout i `layouts/`
- én komponent i `components/`
- én statisk stylesheet-fil i `assets/css/`

Se også [docs/FRONTEND_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_MODEL.md) for valgt modell, og [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ROADMAP.md) for resten av planen.
