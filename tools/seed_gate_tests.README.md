# Seed-porten (tools/seed_gate_tests.txt)

Testane som er `krev_ny_seed`-klassifiserte i `tools/nc_test.no` OG som handlar om
**runtime-paritet** (fil/prosess/sokkel/json/null-semantikk osb.). Fullhost-jobben i
`b2-seed-direct.yml` køyrde lista via harnessen med `NC_NATIVE=<fersk seed>` til han vart
sletta i ef4da9c.

I CI (2026-09-25) er det jobben `seed-gate-linux` i `.github/workflows/ci.yml`, ikkje påkravd:

- **push/PR:** berre testane som slow-lanane utset med `NC_SKIP_HEAVY_COMPILE`
  (`NC_TEST_BERRE_UTSETTE=1`, same predikat: `er_tung_compile` + `er_committed_seed_ustabil`
  i `tools/nc_test.no`). Resten av lista køyrer alt i `native-linux-slow`.
- **workflow_dispatch:** heile lista (`NC_TEST_LIST_FILE=tools/seed_gate_tests.txt`).

`tools/language_parity_tests.txt` (2026-09-06): testar som treng **kompilator-funksjonar**
(lambda/closure, spread, standardargument, destrukturering, enum, metodar, optional,
typa felt, varargs, grensesnitt — M-planen i docs/SPRAAK_PARITET_PLAN.md). Dei feilar
med Parserfeil på committed seed òg (CI hoppar over dei), og er ikkje seed-arbeid.
