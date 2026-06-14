# REAL COMPILER V32

- Fixes `sett_inn(...)` semantics in bytecode VM for selfhost chain.
- Existing indexes are now updated instead of always inserting and shifting.
- This removes the selfhost environment length mismatch seen in `tests/test_selfhost.no`.
