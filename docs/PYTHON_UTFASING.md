# Python-utfasing for Norscode

Mål: Norscode skal gradvis gå fra Python-styrt compiler/CLI til en selfhosted compiler og runtime skrevet i Norscode.

## Status nå

Python er fortsatt nødvendig for enkelte bootstrap-, test- og parity-steg. Dette betyr ikke at sluttmålet er Python-basert. Python skal behandles som midlertidig verktøy og flyttes bak legacy-/bootstrap-grenser.

## Prinsipp

1. `dist/norscode` og `bin/nc` er primær flyt.
2. `python3 main.py` er legacy/bootstrap, ikke hovedplattform.
3. Nye compiler-funksjoner bør først få selfhost-implementasjon.
4. Python kan brukes som orakel i overgangsfasen, men skal ikke være endelig kilde til sannhet.
5. CI skal gradvis gå fra Python-parity til selfhost-only gates.

## Fase 1 — Stabil selfhost parity

- Hold `update-selfhost-parity-fixtures --suite all` grønn.
- Fjern hardkodede uttrykks-cases i `selfhost/compiler.no` stegvis.
- Bygg ekte tokenisering for IR og uttrykk i Norscode.
- Gjør strict-disasm lik Python for alle kjente IR-cases.

## Fase 2 — Legacy-grense for Python

- Flytt Python-only verktøy til `legacy_python/` eller tilsvarende mappe.
- La `main.py` være bootstrap-wrapper, ikke hoved-compiler.
- Dokumenter alle kommandoer som fortsatt krever Python.
- Ikke legg nye features direkte i Python uten selfhost-plan.

## Fase 3 — Selfhost compiler som primær

- Parser, lexer og IR-generator kjøres fra Norscode.
- Python brukes bare til nød-bootstrap eller migrering.
- CI skal ha en `selfhost-only` jobb.
- Release-pakken skal kunne kjøre uten å kalle Python-runtime i normal bruk.

## Fase 4 — Python fjernes fra normal drift

- `norcode run`, `norcode check`, `norcode test`, `norcode ir-disasm` og `norcode ci` skal gå via selfhost/binær flyt.
- Python-kode beholdes bare som historisk referanse eller slettes når selfhost dekker alt.

## Neste konkrete arbeid

1. Erstatt hardkodet `disasm_fra_kilde` med tokenizer + linjeparser i Norscode.
2. Erstatt hardkodet `disasm_uttrykk` med ekte uttrykksparser.
3. Legg til test som feiler hvis nye Python-only compiler-funksjoner mangler selfhost-notat.
4. Legg til `docs/SELFHOST_STATUS.md` med daglig status over hva som er selfhosted.
