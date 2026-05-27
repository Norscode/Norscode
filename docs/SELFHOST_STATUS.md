# Selfhost-status for Norscode

Denne filen brukes for å følge overgangen fra Python-basert bootstrap til selfhosted Norscode.

## Mål

Norscode skal kunne kompilere, teste og kjøre seg selv uten at Python er normal runtime.

## Statusoversikt

| Område | Status | Kommentar |
|---|---:|---|
| CLI/binær flyt | Delvis | `dist/norscode` og `bin/nc` finnes; `norcode/cli.py` er normal modulær vei, og den gjenværende bootstrap-kompatibiliteten ligger eksplisitt i `legacy_main.py` og `bootstrap/python_entry.py`. |
| Parser parity | Klar / overvåkes | Selfhost parity rapporterer 100% dekning, men CI viser fortsatt enkelte IR-mismatch. |
| IR disasm | Delvis | Python og selfhost avviker fortsatt på strict IR-cases og enkelte uttrykk. |
| Uttrykksparser | Delvis | `1 -> 0` viser mismatch: selfhost/Python forventning må avklares og samles. |
| IR fra kilde | Delvis | Flere snapshot-cases viser tom selfhost-output mot Python-output. |
| Testsystem | Delvis | Testene kjører, men CI bruker fortsatt Python-orakel og Python-verktøy. |
| Web/runtime | Tidlig | Web-eksempler kompilerer, men dette er ikke ferdig selfhost-runtime. |
| Pakking/release | Delvis | Release-pakker finnes, men Python brukes fortsatt i bygg og publisering. |

## Kjente CI-feil som må ryddes

### 1. `test_selfhost.no`

Feil rundt implication/operator-disasm:

```text
0: PUSH 1
1: PUSH 0
2: SWAP
3: NOT
4: SWAP
5: OR
6: PRINT
7: HALT
```

mot

```text
0: PUSH 1
1: PUSH 0
2: NOT
3: OR
4: PRINT
5: HALT
```

Dette må avgjøres som én offisiell IR-kontrakt. Deretter må både Python og selfhost følge samme kontrakt.

### 2. IR snapshot parity

Selfhost gir tom output for enkelte `.nlir` cases der Python gir `PUSH`, `ADD`, `PRINT`, `HALT`.

Dette peker på at selfhost mangler komplett IR-linjeparser eller strict-disasm-logikk.

## Prioritet nå

1. Lag en tydelig IR-kontrakt for `->`, `NOT`, `OR`, `SWAP`.
2. Implementer ekte `.nlir` tokenizer/parser i selfhost.
3. Gjør `ir-disasm --strict` lik mellom Python og selfhost.
4. Fjern hardkodede snapshot-cases.
5. Når IR-disasm er grønn, flytt neste del av compiler til selfhost.

## Regler for nye endringer

- Nye compiler-features skal ha selfhost-sjekk eller selfhost-plan.
- Python-endringer skal merkes som bootstrap/legacy hvis de ikke har selfhost-ekvivalent.
- `norcode/cli.py` er normal modulær vei; `norcode/legacy_main.py` og `norcode/bootstrap/python_entry.py` er eksplisitt kompatibilitetslag for den gjenværende Python-flaten.
- CI-feil skal ikke løses ved å senke krav uten dokumentert grunn.
- Målet er færre Python-avhengigheter for hver fase.

## Neste konkrete patch etter denne filen

Lag eller oppdater en IR-kontraktfil:

```text
docs/IR_CONTRACT.md
```

Den skal definere offisiell output for:

- `PUSH`
- `ADD`
- `PRINT`
- `HALT`
- `NOT`
- `OR`
- `SWAP`
- `OVER`
- strict-feil for ukjent opcode
- strict-feil for ugyldig heltall
- implication `a -> b`

## Les videre

- [`docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md)
- [`docs/SELFHOST_DIAGNOSTICS.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_DIAGNOSTICS.md)
- [`docs/SELFHOST_CI_GATES.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_CI_GATES.md)
- [`docs/SELFHOST_RELEASE_CHECKLIST.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_RELEASE_CHECKLIST.md)
- [`docs/SELFHOST_FALLBACK_CONTRACT.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_FALLBACK_CONTRACT.md)
- [`docs/SELFHOST_BOOTSTRAP_INVENTORY.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_BOOTSTRAP_INVENTORY.md)
- [`docs/SELFHOST_REMAINING_ROADMAP.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_REMAINING_ROADMAP.md)
