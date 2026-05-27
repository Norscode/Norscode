# Fase 8 — Python nedgraderes

Mål: gjøre selfhost-kompilatoren til primærbane for støttet Norscode-kjerne, mens Python bare er eksplisitt fallback, debug-verktøy og overgangsstøtte.

## Prinsipp

Standardflyt skal være:

```text
.no-kilde
  -> selfhost lexer/parser
  -> selfhost semantic
  -> selfhost IR/backend
  -> runtime/output
```

Python-flyten skal bare brukes når brukeren ber om det, eller når en feature ennå ikke er støttet av selfhost.
I dagens CLI er den eksplisitte legacy-veien `--legacy-python-fallback`.

## CLI-kontrakt

Foreslåtte kommandoer/flagg:

```bash
nc run app.no
nc run app.no --legacy-python-fallback
nc check app.no
nc check app.no --legacy-python-fallback
nc --legacy-python-fallback selfhost-check
nc --legacy-python-fallback selfhost-parity
nc --legacy-python-fallback ci --require-selfhost-ready
```

## Fallback-regler

1. `nc run` prøver selfhost først for støttet kjerne.
2. Hvis selfhost ikke støtter en feature, skal feilen si hvilken feature som mangler.
3. Python brukes bare hvis `--legacy-python-fallback` er satt.
4. Når Python brukes, skal CLI skrive tydelig varsel:

```text
Advarsel: Python-fallback brukes. Dette er overgangsstøtte, ikke primær Norscode-bane.
```

5. CI skal kunne feile hvis fallback brukes i selfhost-klare tester.

## Parity-måling

`nc --legacy-python-fallback selfhost-parity` bør sammenligne:

- tokens
- AST snapshot
- semantic rapport
- IR/disasm snapshot
- output hash

Eksempelrapport:

```text
SELFHOST PARITY
lexer: OK
parser: OK
semantic: OK
ir: OK
hash: OK
```

Ved avvik:

```text
SELFHOST PARITY
parser: DIFF
--- python ---
...
--- selfhost ---
...
```

## CI-krav

CI bør kjøre:

```bash
nc --legacy-python-fallback ci --check-names --require-selfhost-ready
nc --legacy-python-fallback selfhost-check
nc --legacy-python-fallback selfhost-parity
```

## Feature gates

Selfhost-kjernen kan starte smalt. Fase 8 krever ikke full språkparitet, bare at støttet kjerne går selfhost først.

Første støttede kjerne:

- funksjoner
- parametere
- returtype
- `la`
- `returner`
- `hvis` / `ellers`
- `mens`
- funksjonskall
- binære uttrykk
- heltall
- tekst
- bool
- lister
- maps

## Exit-kriterier

Fase 8 er ferdig når:

- `nc run` bruker selfhost som standard for støttet kjerne
- Python krever eksplisitt `--legacy-python-fallback`
- fallback gir tydelig varsel
- CI måler selfhost-parity
- selfhost-klare tester feiler hvis Python brukes skjult

## Videre lesing

- [`docs/SELFHOST_FALLBACK_CONTRACT.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_FALLBACK_CONTRACT.md)
