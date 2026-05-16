# REAL COMPILER V38

Nytt i v38:
- hotpath for refleksive delingsfraser i `selfhost.compiler.uttrykk_til_ops_og_verdier_med_miljo`
- støtter tre-tokenformer som `dele seg med` og `dividere seg på`

Verifisert:
- `python3 -m py_compile compiler/bytecode_backend.py main.py`
- `norcode selfhost-chain-run tests/test_selfhost.no`

Status:
- forrige blokkering på tokenet `seg` er borte
- ny blokkering er nå: `ukjent token/navn i uttrykk storre ved token 6`
