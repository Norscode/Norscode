# Prekompilerte bootstrap-modular

Sidan JSON-avviklinga J1b (2026-09-25) ligg det **ingen committa selfhost-cache**
her. Einaste fila som står att er `vm.ncb.json`, som er runtime-payload for
attestasjons-, Windows- og releaseverktøya. Ho blir generert med
`./bin/nc regen-bootstrap` og fjerna i J1c (`build/payloads/`).

**Kjelda er standardvegen.** `nc compile`, `nc run` og testharnessen kompilerer
`selfhost/*.no` og `std/*.no` frå kjelde. Ein kjeldehash-verifisert cache er
valfri og ligg alltid under `build/`:

- `tools/materialize_bootstrap_cache.no` skriv `<rot>/precompiled/manifest.json`
  og `<rot>/stdlib/manifest.json` med fragment. Set `NORSCODE_BOOTSTRAP_CACHE=<rot>`
  (standard `build/cache/bootstrap`). Eit fragment blir berre brukt når modulnamn,
  kjeldehash, artefakthash og kompilatorfingeravtrykk stemmer.
- `tools/materialize_l5_precompiled.no` hentar L5b-cachemodulane frå ein verifisert
  L5 Gen1 (`build/l5/compiler_v1.ncb.json`) til `build/l5/precompiled/`.
  `tools/selfcompile_l5b.no` køyrer han sjølv i cache-på-modus (selvstendighet 7a).

Ikkje legg `*.ncb.json` inn her att. `tools/no_c_python_active_surface.no`
(`committed_bytecode`) tel spora bytekode.
