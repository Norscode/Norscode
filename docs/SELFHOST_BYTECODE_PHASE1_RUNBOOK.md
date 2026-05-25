# Selfhost Bytecode Phase 1 Runbook

Denne runbooken beskriver den minimale bytecode-gaten som nå brukes for Omgang 5 i
[`docs/SELFSTENDIG_NORSCODE_ROADMAP.md`](/Users/jansteinar/Projects/Norscode/docs/SELFSTENDIG_NORSCODE_ROADMAP.md).

## Mål

- verifisere at `selfhost/bytecode_backend.no` kan bygge NCB JSON frå AST
- verifisere at `selfhost/ir.no` kan produsere stabil IR/disasm for den støtta kjernen
- holde ein liten, stabil bytecode-suite som kan kjøres utan å trekke inn ekstra bootstrap-logging

## Kjøring

Kjør bytecode-suiten:

```bash
./bin/nc selfhost-bytecode-suite
```

Kjør den direkte selfhost-testfila:

```bash
./bin/nc run selfhost/tests/bytecode_tests.no
```

Kjør parity-sjekken mot Python-backend:

```bash
python3 -m pytest tests/test_bytecode_parity.py
```

## Hva som testes

- `ir_emitter()` / `ir_emit()` / `ir_disasm()`
- label-emitter for deterministisk IR-output
- bytecode-JSON-format for ei enkel funksjon med `returner`
- deterministisk bytecode-output på same input
- match mellom selfhost-bytecode og Python-bytecode på eit enkelt program

## Kontrakt

- bytecode-backenden arbeider mot AST, ikkje rå tekst
- `selfhost/tests/bytecode_tests.no` bruker `selfhost/parser.no` og `selfhost/semantic.no`
- parity-sjekken bruker den same enkle kildefila mot Python og selfhost

## Forventet resultat

- `./bin/nc selfhost-bytecode-suite` skal rapportere `1/1 OK`
- `./bin/nc run selfhost/tests/bytecode_tests.no` skal avslutte utan feil
- `python3 -m pytest tests/test_bytecode_parity.py` skal passere
