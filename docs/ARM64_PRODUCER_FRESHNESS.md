# ARM64-kandidatbygg krev verifisert produsent

Status: implementert og lokalt testa. Ingen kodegenerator, runtime eller stage-0 er promotert.

## Funn

tools/build_arm64_seed.no valde ein krysskodegenerator berre frå filnamn og om fila fanst. Han kontrollerte ikkje .vertcg, binærhash eller .srchash/.depshash før materialisering og køyring. Dermed kunne eit bygg omtalt som frå levande kjelde bruke ein forelda emitter. Full CI for PR #254 demonstrerer skiljet: ARM64-bygg/seed-gate 38863111664 er grøn, medan hovud-CI 38863111488 feilar på stale krysskodegeneratorar. Dette er ikkje eit samla ferskt produsentbevis.

## Endring og bevis

Byggjaren bruker no codegen_prebuilt.vel(root, os + '-arm64', 'linux-x86_64') før materialisering eller spawn. Det er den eksisterande kontrollen for både kjeldelukking, vertprodusent og binærhash. Ugyldig eller manglande bevis gir exit 1 med den konkrete grunnen. Ingen gammal hash er omskriven, og ingen valfri bypass er lagd til.

Regresjonen test_arm64_seed_codegen_freshness.no kopierer ein ekte køyrbar lokal Norscode-kodegenerator til ei mellombels ARM64-kryssrute, utan tilhøyrande attestasjon. Før rettinga køyrde byggjaren han og returnerte 0; på macOS vart eit Mach-O-artefakt feilaktig omtalt som Linux ARM64. Etter rettinga blir både Linux- og macOS-målruta avvist før noko kandidat blir skrive. Testen gjenopprettar miljøvariablane og bruker reelle prosessar/artefakt, ikkje eit kunstig PASS-program. Faktisk køyring av feilprodusenten er reprodusert på macOS ARM64; testen støttar òg Linux x86_64. Andre vertar har ikkje ein køyrbar lokal produsent for denne negative prosesskontrollen.

Tre relaterte testar består, null feil/hopp på macOS ARM64. Endeleg avgrensa feature-check og diffkontroll består. På den verkelege endra kjelderota blir den eksisterande Linux ARM64-krysskodegeneratoren faktisk avvist med exit 1 og konkret .depshash-avvik før materialisering.

## Restarbeid

Nye kontrollerte krysskodegeneratorar må byggjast og funksjonstestast på dei relevante vert/mål-parane. Eit eldre Norscode-artefakt kan vere ein nødvendig bootstrap-produsent, men då må han bindast til sin eigen verifiserte kjelderot og oppskrift. Han skal ikkje framstillast som gjeldande kjelde berre ved å omskrive hashmetadata. Kandidatane må gjennom heile matrisen og eiga godkjenning før eventuell promotering. Denne PR-en etablerer kontrollen og påstår ikkje at dei nye kandidatane alt finst.
