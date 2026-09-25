# Seed-porten (tools/seed_gate_tests.txt)

Testane som er `krev_ny_seed`-klassifiserte i `tools/nc_test.no` OG som handlar om
**runtime-paritet** (fil/prosess/sokkel/json/null-semantikk osb.). Fullhost-jobben i
`b2-seed-direct.yml` køyrde lista via harnessen med `NC_NATIVE=<fersk seed>` til han vart
sletta i ef4da9c.

I CI (2026-09-25) er det jobben `seed-gate-linux` i `.github/workflows/ci.yml`, ikkje påkravd,
berre ved **workflow_dispatch**: heile lista (`NC_TEST_LIST_FILE=tools/seed_gate_tests.txt`).

På push/PR dekkjer `native-linux-slow` lista: den committa Linux-seeden er den ferske
sjølv-hosta seeden (R1), og slow-lana køyrer no òg dei 16 testane ho tidlegare utsette
(`er_tung_compile` + `er_committed_seed_ustabil`), sidan `NC_SKIP_HEAVY_COMPILE` er fjerna
der (H2). Berre macOS-slow-lana (C-æra-seed) utset dei framleis. `NC_TEST_BERRE_UTSETTE=1`
køyrer berre dei utsette testane, for lokal måling.

`tools/language_parity_tests.txt` (2026-09-06): testar som treng **kompilator-funksjonar**
(lambda/closure, spread, standardargument, destrukturering, enum, metodar, optional,
typa felt, varargs, grensesnitt — M-planen i docs/SPRAAK_PARITET_PLAN.md). Dei feilar
med Parserfeil på committed seed òg (CI hoppar over dei), og er ikkje seed-arbeid.
