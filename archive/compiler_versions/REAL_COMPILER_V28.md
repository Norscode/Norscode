# REAL COMPILER V28

## Hva som er fikset
- `liste_*`-deklarasjoner wraps ikke lenger ukritisk alle uttrykk inn i ekstra liste-lag.
- Selfhost-kjeden kommer nå forbi den tidligere `str + list`-feilen i `tests/test_selfhost.no`.

## Status
- `tests/test_selfhost.no` går lenger enn i v27.
- Neste blokkering er fortsatt i selfhost-kjeden, og må håndteres i et nytt steg.

## Kommandoer
```bash
norcode selfhost-chain-run tests/test_selfhost.no
norcode selfhost-chain-check
```
