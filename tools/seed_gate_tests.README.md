# Seed-porten (tools/seed_gate_tests.txt)

Testane som er `krev_ny_seed`-klassifiserte i `tools/nc_test.no` OG som handlar om
**runtime-paritet** (fil/prosess/sokkel/json/null-semantikk osb.). Fullhost-jobben i
`b2-seed-direct.yml` køyrer lista via harnessen med `NC_NATIVE=<fersk seed>` og gjev
tabellen over kva som står att før promotering av den sjølv-hosta Linux-seeden.

`tools/language_parity_tests.txt` (2026-09-06): testar som treng **kompilator-funksjonar**
(lambda/closure, spread, standardargument, destrukturering, enum, metodar, optional,
typa felt, varargs, grensesnitt — M-planen i docs/SPRAAK_PARITET_PLAN.md). Dei feilar
med Parserfeil på committed seed òg (CI hoppar over dei), og er ikkje seed-arbeid.
