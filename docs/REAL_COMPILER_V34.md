# REAL COMPILER V34

V34 legger til mer målrettet feilsøking for selfhost-kjeden.

Nytt:
- `selfhost-chain-run --trace-focus <tekst>`
- `selfhost-chain-run --repeat-limit N`
- tilsvarende flagg for `selfhost-chain-check`

Dette gjør det lettere å finne hvor `tests/test_selfhost.no` går i fastlås eller repeterer samme VM-tilstand.
