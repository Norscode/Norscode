# v951-v960 release readiness

## Current status on 2026-06-27

This document is historical. Current release readiness is tracked by:

```text
prosjekter/NorscodeAIKernel/status/production_readiness.json
```

Current gate result is `production_ready=true`.

Dette er ei rein statusrunde.

## Klart

- Web-ruter er samla i `prosjekter/NorscodeWeb/app.no`.
- DEV-login brukar fast session-id og er tydeleg merka ikkje-produksjon.
- Admin, CMS-demo, status-JSON, policy-sider og AI-demo er synlege.

## Historical ikkje-produksjonsklart

- `production_ready` skal vere `false`.
- `reason` skal vere `stage0_mangler_random_hex`.
- Login skal vere `dev_only_fixed_session`.

## Gate

Release kan ikkje merkast produksjonsklar før ny native/stage0 støttar sikker random.
