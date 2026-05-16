# Maintenance Policy

Dette dokumentet beskriver hvordan Norscode vedlikeholdes etter 1.0-kandidaten.

## Mål

- holde CLI, installasjon og bootstrap-forløpet rolig og forutsigbart
- gi nye brukere en tydelig opplevelse uten skjulte fallback-regler
- gjøre brudd sjeldne, dokumenterte og enkle å migrere fra

## Release-kadens

- Patch-releases kan komme fortløpende når vi fikser feil eller regresjoner.
- Minor-releases brukes for nye, bakoverkompatible produktforbedringer.
- Major-releases reserveres for brudd i kontrakt, dataformat eller CLI-flate.
- Alle releaser skal følge samme release-prosess og verifiseringsløp.

## Stottevinduer

- `current` betyr den siste stabile releasen.
- Forrige stabile minor kan få korte tilbakeporter dersom det finnes viktige feil.
- Legacy-aliaser og gamle filnavn skal bare beholdes så lenge migrering krever det.
- Når en gammel vei fases ut, skal den først varsles, deretter dokumenteres, så fjernes.

## Deprecation

- Nye brudd annonseres i release-notater og i relevant dokumentasjon.
- Brukeren skal få en tydelig migreringssti før gammel oppførsel fjernes.
- Legacy-navn kan varsles, men skal ikke være normal vei for nye prosjekter.

## Release-notater

- Hver release skal ha en kort, lesbar oppsummering av hva som endret seg.
- Breaking changes skal stå tydelig først i notatene og ha en egen migreringsforklaring.
- Notatene skal nevne berørte kommandoer, dataformat eller deployflyt når det er relevant.
- Release-notater skal være en del av standard publiseringsflyt, ikke en ettertanke.

## Hva som er normal support

- `norcode` som primær CLI
- `./bin/bootstrap` og `python3 main.py` som eksplisitt bootstrap
- releasepakker som kan installeres, oppgraderes og rulles tilbake mekanisk
