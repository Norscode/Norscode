# Binær NCB-versjonskontroll: kandidatstatus

JSON er framleis normalformatet. Kandidatrettinga avviser ukjend versjon og manglande versjonsbyte før binær payload-dekoding i liste-, bytes- og tekstinngangane. Gyldig versjon 1 og JSON blir bevarte.

## Testbevis 2026-10-15

- Ny regresjon feila før rettinga og består etterpå. Fire relaterte format-testar består utan hoppa testar. Utvida direkte test dekkjer òg bytes-inngangen.
- Feature-check for dei endra Norscode-filene består.
- Den eksisterande macOS-kodegeneratoren vart validert mot si uendra kjelderot med codegen_prebuilt.vel før han bygde ein isolert kandidat frå oppdatert JSON-NCB.
- Tre påfølgjande macOS-kodegeneratorkandidatar er byte-identiske: SHA-256 a8a11d4c271cdc11000e8e7f40049e8247485a1f87ed31d891f709d1766f76fa.
- Desse kodegeneratorkandidatane rapporterer uløyste=0, men stubba=7 for eksisterande macOS-runtimeoperasjonar. Dette er eit avgrensa kodegenereringsfikspunkt, ikkje full runtime-attestasjon.
- Kandidaten bygde den nye versjonsregresjonen til Mach-O: uløyste=0 stubba=0. Faktisk native køyring returnerte 0 og test_ncb_serde_version OK.

## Oppdatert verifikasjon 2026-10-31

Dei seks committa kodegeneratorane er no bygde frå PR-kjelda, saman med
kjelde-/avhengnadshashar, kryss-proveniens og godkjende binærpinnar. Ingen port er
svekt. Linux x86_64 og macOS ARM64 vert-generatorar gav kvar tre byte-identiske
generasjonar:

- Linux x86_64: `6e2f13873e26bf20fc6a56cf126afbe44336add221e26ee2600323e4f1da5209`
- macOS ARM64: `7e6b018fe48d31f0456a359f62d61bb3408ab26037e37faf8f52f48665ca87d5`

`tools/build_cross_codegen.no` bygde Linux-/macOS-ARM64 på Linux x86_64 og
Linux-/NorscodeOS-x86_64 på macOS ARM64. Den eksisterande Mac-vert-generatoren
rapporterer framleis sju stubba operasjonar; dette arbeidet er ikkje full
runtime-attestasjon eller løysing av filmodus-ABI-mangelen.

Samla Linux x86_64-køyring gjennom `tools/nc_test.no` bestod 6/6 utan skip:
`test_ncb_serde_version`, `test_ncb_bin_edgecases`, `test_ncb_bin_tekstveg`,
`test_ncb_serde_dualformat`, `test_native_codegen_srchash` og
`test_arm64_kryss_codegen`. Hashporten bestod òg på macOS.

Dei fire formatregresjonane bestod som faktiske macOS ARM64-program bygde gjennom
`nc bygg-native --target macos-arm64` frå kjelde med GC aktivert. Versjons- og
tekstveg-testane bestod òg som Linux x86_64-ELF, kryssbygde gjennom Mac-verten sin
oppdaterte generator og faktisk køyrde på Linux.

Den gamle råkodegen-proben viste bitmønsteret i `tekst(3.5)`, ikkje prov på at
dekodinga endra den numeriske verdien: direkte type- og verdikontroll er korrekt.
Rå NCB frå `nc build` manglar den implisitte desimalformateraren som vanleg
`bygg-native` frå kjelde tek med. Den gamle observasjonen skal difor ikkje lenger
stå som ein stadfesta feil i desimaldekoding. Begge native tekstveg-testane består
gjennom den normale byggestien.

Native testutdata fekk unike filnamn. Ei innleiande omkøyring på overskriven
Mach-O-sti vart avslutta med signal; same binær på fersk fil bestod. Det vart ikkje
gjort ei runtime-endring på grunnlag av denne testharness-observasjonen.

Feature-check for serde og versjonsregresjonen, samt diff-kontroll, bestod.

## Påkravd før innfletting

Full GitHub CI må vurderast på den nye PR-tippen. Stage0-seedane er ikkje oppdaterte
i denne kodegeneratoromgangen; kjelde-/seed-likskap og dekning gjennom distribuert
runtime må framleis vurderast. JSON er normalformatet, og ingen main-merge eller
release-aktivering er utført. Binærane og testbevisa ligg i den eksisterande PR-en
for gjennomgang før eventuell innfletting.
