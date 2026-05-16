# Frontend Assets

Frontend-sporet trenger en enkel og forutsigbar statisk asset-flate.

## Foreslått struktur

```text
frontend/
└── assets/
    ├── css/
    │   └── app.css
    ├── images/
    └── icons/
```

## Regler

- CSS ligger i `assets/css/`
- bilder ligger i `assets/images/`
- ikoner ligger i `assets/icons/`
- asset-navn skal være stabile og meningsfulle
- statiske filer skal kunne caches hardt

## Minimumskrav

- én hovedstylesheet
- en tydelig plass for logo/bilder
- en tydelig plass for ikoner

## Hvorfor dette er viktig

- gjør frontend enkel å deploye
- gir en klar grense mellom appkode og statiske filer
- gjør caching og CDN-bruk enklere senere

## Hva som gjenstår etter dette

Se [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ROADMAP.md).
