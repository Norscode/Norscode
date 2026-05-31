# Utfasing av historisk vei for Norscode

Mål: Norscode skal gradvis gå fra historisk vei-styrt compiler/CLI til en selfhosted compiler og runtime skrevet i Norscode.

## Status nå

historisk vei er ikke lenger normal vei i Norscode. Det som gjenstår er historiske referanser, eksplisitt bootstrap-kompatibilitet og eventuelle arkiverte parity-spor. historisk vei skal behandles som legacy eller dokumentasjon, ikke som standard flyt.

## Prinsipp

1. `dist/norscode` og `bin/nc` er primær flyt.
2. Eventuelle historisk vei-flater er historiske eller eksplisitt bootstrap-arkiv, ikke hovedplattform.
3. Nye compiler-funksjoner bør først få selfhost-implementasjon.
4. Historiske historisk vei-orakler kan bare brukes til reproduksjon og migrering.
5. CI skal gradvis gå helt over til selfhost-only gates.

## Fase 1 — Stabil selfhost parity

- Hold `update-selfhost-parity-fixtures --suite all` grønn.
- Fjern hardkodede uttrykks-cases i `selfhost/compiler.no` stegvis.
- Bygg ekte tokenisering for IR og uttrykk i Norscode.
- Gjør strict-disasm lik historisk vei for alle kjente IR-cases.

## Fase 2 — Legacy-grense for historisk vei

- Flytt historisk vei-only verktøy til en tydelig legacy-mappe eller tilsvarende plassering.
- Arkiver enhver gjenværende historisk kompatibilitet tydelig som legacy.
- Dokumenter kun historisk bruk og migreringsinformasjon for historisk vei-veier.
- Ikke legg nye features direkte i historisk vei.

## Fase 3 — Selfhost compiler som primær

- Parser, lexer og IR-generator kjøres fra Norscode.
- historisk vei brukes bare til nød-bootstrap eller migrering.
- CI skal ha en `selfhost-only` jobb.
- Release-pakken skal kunne kjøre uten å kalle historisk runtime i normal bruk.

## Fase 4 — historisk vei fjernes fra normal drift

- `norcode run`, `norcode check`, `norcode test`, `norcode ir-disasm` og `norcode ci` skal gå via selfhost/binær flyt.
- historisk vei-kode beholdes bare som historisk referanse eller slettes når selfhost dekker alt.

## Neste konkrete arbeid

1. Erstatt hardkodet `disasm_fra_kilde` med tokenizer + linjeparser i Norscode.
2. Erstatt hardkodet `disasm_uttrykk` med ekte uttrykksparser.
3. Legg til test som feiler hvis nye historisk vei-only compiler-funksjoner mangler selfhost-notat.
4. Legg til `docs/SELFHOST_STATUS.md` med daglig status over hva som er selfhosted.
