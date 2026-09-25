# Seed-porten (tools/seed_gate_tests.txt)

Testane som er `krev_ny_seed`-klassifiserte i `tools/nc_test.no` OG som handlar om
**runtime-paritet** (fil/prosess/sokkel/json/null-semantikk osb.). Fullhost-jobben i
`b2-seed-direct.yml` køyrde lista via harnessen med `NC_NATIVE=<fersk seed>` til han vart
sletta i ef4da9c.

I CI (2026-09-25) er det jobben `seed-gate-linux` i `.github/workflows/ci.yml`, ikkje påkravd:

- **push/PR:** berre testane som slow-lanane tidlegare utsette med `NC_SKIP_HEAVY_COMPILE`
  (`NC_TEST_BERRE_UTSETTE=1`, same predikat: `er_tung_compile` + `er_committed_seed_ustabil`
  i `tools/nc_test.no`, i dag 16). Resten av lista køyrer alt i `native-linux-slow`.
- **workflow_dispatch:** heile lista (`NC_TEST_LIST_FILE=tools/seed_gate_tests.txt`).

Den committa Linux-seeden er den ferske sjølv-hosta seeden (R1), så `native-linux-slow`
køyrer no òg dei 16 (`NC_SKIP_HEAVY_COMPILE` er fjerna der, H2). Berre macOS-slow-lana
(C-æra-seed) utset dei framleis. `native-linux-slow` er likevel raud på R1 frå før
(`test_bundle_metadata_merge` og `test_nc_main_metadata_recovery`: phase8-fiksturen er
avleidd av den regenererte `vm.ncb`-en med 27 globalar, testane ventar 10; J1b byter til
syntetisk fikstur). Ein ny feil blant dei 16 ville gøyme seg bak den raude fargen der, så
push/PR-køyringa av `seed-gate-linux` blir verande som eige, synleg signal til
`native-linux-slow` er grøn. Lokalt kan `NC_TEST_BERRE_UTSETTE=1` brukast til å måle berre
dei 16.

`tools/language_parity_tests.txt` (2026-09-06): testar som treng **kompilator-funksjonar**
(lambda/closure, spread, standardargument, destrukturering, enum, metodar, optional,
typa felt, varargs, grensesnitt — M-planen i docs/SPRAAK_PARITET_PLAN.md). Dei feilar
med Parserfeil på committed seed òg (CI hoppar over dei), og er ikkje seed-arbeid.
