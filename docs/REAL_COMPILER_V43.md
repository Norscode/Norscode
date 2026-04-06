# REAL COMPILER V43

## Mål
Gi selfhost-kjeden en presis uttrykksprobe før selve hotpath-kompileringen, slik at fraser som
`storre eller lik` kan spores gjennom hele kjeden.

## Nytt
- `--expr-probe <tekst>` for `selfhost-chain-run`
- `--expr-probe-log <fil>` for å skrive probe-logg til fil
- samme støtte i `selfhost-chain-check`

## Eksempel
```bash
python3 main.py selfhost-chain-run tests/test_selfhost.no --expr-probe "eller" --expr-probe-log build/expr_probe.log
```
