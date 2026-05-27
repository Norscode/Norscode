# Python phaseout plan for Norscode

Mål: Python skal fases ut fra Norscode-runtime og vanlig bygg. Python beholdes midlertidig bare som bootstrap- og parity-referanse til selfhost-kompilatoren er stabil.

## Prinsipp

Python skal ikke fjernes i ett steg. Først må Norscode kunne gjøre de samme jobbene selv:

1. parse kilde
2. bygge IR
3. disassemble IR
4. validere strict/non-strict modus
5. kompilere til C/native output
6. kjøre CI uten Python-runtime i hovedløpet

## Fase 0 — nåværende status

Python er fortsatt hovedreferanse. Selfhost er nesten klar, men CI viser fortsatt parity-feil i:

- `tests/test_selfhost.no`
- `IR snapshot parity (python vs selfhost)`
- `tests/ir_sample.nlir`

Dette betyr at Python ikke kan fjernes helt ennå.

## Fase 1 — isoler Python som legacy/bootstrap

- Behold Python som referanse, men merk den som legacy/bootstrap.
- Ikke legg nye funksjoner i Python-versjonen.
- All ny compiler-logikk skal bygges i `selfhost/compiler.no`.
- CI skal fortsatt sammenligne Python mot selfhost til parity er grønn.

## Fase 2 — erstatt hardkodede selfhost-svar

Selfhost-kompilatoren skal slutte å returnere hardkodede svar for enkelttester. Prioritet:

1. `disasm_fra_tokens`
2. `disasm_fra_tokens_strict`
3. `disasm_fra_kilde`
4. `disasm_fra_kilde_strict`
5. `disasm_uttrykk`
6. `kompiler_fra_kilde`
7. `kompiler_fra_linjer`

## Fase 3 — gjør selfhost til primær motor

Når parity er grønn:

- `./bin/nc` skal bruke selfhost som primær compiler.
- Python skal kun brukes med eksplisitt flagg, for eksempel `--legacy-python-fallback`.
- CI skal ha en egen jobb for legacy parity, ikke som hovedbygg.

## Fase 4 — flytt Python ut av hovedkoden

Når selfhost bygger alt stabilt:

- flytt Python-kode til `tools/legacy_python/`
- fjern Python fra vanlig installasjon
- fjern Python fra vanlig build-path
- behold historisk referanse til debugging

## Fase 5 — fjern Python fra CI-hovedløpet

Til slutt skal standard CI være:

```bash
./bin/nc ci --selfhost-only
```

Python kan beholdes i en separat legacy-jobb fram til prosjektet ikke trenger referansen mer.

## Neste konkrete byggesteg

Fiks først `selfhost/compiler.no` slik at `disasm_fra_kilde_strict` og `disasm_fra_tokens_strict` bygger samme IR som Python for `tests/ir_sample.nlir`. Når dette er grønt, kan Python flyttes til legacy-mappe.
