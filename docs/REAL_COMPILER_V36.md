# REAL COMPILER V36

## Mål
Få selfhost-kjeden forbi hotpathen i `selfhost.compiler.uttrykk_til_ops_og_verdier_med_miljo`.

## Endringer
- Python-fastpath i `compiler/bytecode_backend.py` for vanlige uttrykksformer
- støtte for heltall, bool, miljønavn, parenteser og vanlige operatorer
- støtte for flere norske operatoraliaser

## Verifisert
- `python3 -m py_compile compiler/bytecode_backend.py`
- `norcode selfhost-chain-check`
- `norcode selfhost-chain-run tests/test_selfhost.no`

## Status
V36 flytter blokkeringen videre fra `uttrykk_til_ops_og_verdier_med_miljo` til `selfhost.compiler.instruksjon_til_tekst`.
