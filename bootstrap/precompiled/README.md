# Prekompilerte bootstrap-modular

Sidan JSON-avviklinga J1b/J1c (2026-09-25) ligg det **ingen committa bytekode**
her. `vm.ncb.json` (runtime-payload for attestasjons-, Windows- og releaseverktøya)
blir generert frå `selfhost/vm.no` av `tools/materialize_runtime_payloads.no`, saman
med `selfhost/vm_executor.ncb.json`. Begge er gitignorerte; stiane er dei same som før
fordi dei er baka inn i seedane. Ein `<fil>.srchash`-markør (kjeldehash + sha256)
lèt verktøyet hoppe over genereringa når kjeldene er uendra. CI genererer dei éin gong
per køyring på Linux og deler dei som artefakt. `./bin/nc regen-bootstrap` tvingar
ny generering.

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
