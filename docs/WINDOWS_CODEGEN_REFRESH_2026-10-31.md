# Kodegeneratorar etter Windows-endringane i PR #269

CI `41219138317` feila fire jobbar på same utdaterte x86-depshash.
Generatorane er no faktisk bygde frå PR-kjelda; ferskheitsporten er uendra.

- `native_codegen_v2.no` vart bundla til JSON-NCB gjennom Norscode.
- Den committa x86-generatoren bygde cg1; cg1 bygde cg2; cg2 bygde cg3.
  Alle tre er byte-identiske, SHA-256
  `09af27d79b170f5ac1fe8d1f2ac0d4be2d42559e35749d690f980193cc28a3c8`.
- `tools/build_cross_codegen.no` bygde Linux- og macOS-ARM64-generatorane på
  Linux x86_64, og Linux-/NorscodeOS-x86_64-generatorane på macOS ARM64.
  Dei to Linux-vertede ARM64-binærane vart byte-identiske med dei gamle;
  `.vertcg` er oppdatert til den nye vert-generatoren.
- Kjeldehashar, avhengnadshashar, kryss-proveniens og godkjende binærpinnar er
  oppdaterte saman med binærane. Ingen verdi er omskriven for ein gammal binær
  som ikkje vart bygd på nytt.

## Kontroll

`test_native_codegen_srchash` bestod på macOS og Linux x86_64. Linux-omgangen
bestod òg `test_arm64_kryss_codegen`, `test_win64_os` og `test_pe_emitter`.
To Windows-attestasjonsfiler vart klassifiserte som hoppa av Linux-løparen;
`test_windows_ncb_trailer_validation` og `test_windows_attestation_direct_dispatch`
vart difor køyrde direkte gjennom Mac-VM-en og bestod der.

Gamle og nye x86-generatorar produserte byte-identisk Linux-output for
`aot_common_integer_ops` og `arm64_try_catch`. Dei fire faktiske ELF-køyringane
gav venta exitkodar 11 og 53. Feature-check og diff-kontroll bestod.

Dette implementerer ikkje AppContainer/Job Object. Den positive Windows-porten
skal framleis feile til reell isolasjon finst. ARM64-stage0 frå denne PR-en er
heller ikkje oppdatert i denne generatoromgangen. Full CI må vurderast på ny tip.
