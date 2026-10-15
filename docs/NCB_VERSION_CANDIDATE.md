# Binær NCB-versjonskontroll: kandidatstatus

JSON er framleis normalformatet. Kandidatrettinga avviser ukjend versjon og manglande versjonsbyte før binær payload-dekoding i liste-, bytes- og tekstinngangane. Gyldig versjon 1 og JSON blir bevarte.

## Testbevis 2026-10-15

- Ny regresjon feila før rettinga og består etterpå. Fire relaterte format-testar består utan hoppa testar. Utvida direkte test dekkjer òg bytes-inngangen.
- Feature-check for dei endra Norscode-filene består.
- Den eksisterande macOS-kodegeneratoren vart validert mot si uendra kjelderot med codegen_prebuilt.vel før han bygde ein isolert kandidat frå oppdatert JSON-NCB.
- Tre påfølgjande macOS-kodegeneratorkandidatar er byte-identiske: SHA-256 a8a11d4c271cdc11000e8e7f40049e8247485a1f87ed31d891f709d1766f76fa.
- Desse kodegeneratorkandidatane rapporterer uløyste=0, men stubba=7 for eksisterande macOS-runtimeoperasjonar. Dette er eit avgrensa kodegenereringsfikspunkt, ikkje full runtime-attestasjon.
- Kandidaten bygde den nye versjonsregresjonen til Mach-O: uløyste=0 stubba=0. Faktisk native køyring returnerte 0 og test_ncb_serde_version OK.

## Opne feil funne under utvida native kontroll

Den native test_ncb_bin_tekstveg feilar på desimaltalsdekoding: 3.5 blir heiltalsbitane 4615063718147915776. Same feil er reprodusert med uendra ncb_serde-kjelde og den pinna macOS-kodegeneratoren, og med kandidaten. VM-køyringa består. Dette er ein eksisterande plattformmangel; han er ikkje retta eller rekna som PASS her.

Den offisielle test_native_codegen_srchash består før kjeldeendringa, men feilar etterpå med stale .srchash som venta. Ingen hashport er svekt.

## Påkravd før innfletting

Endringa i selfhost/ncb_serde.no gjer dei eksisterande kodegenerator-hashane utdaterte. Ferskheitsporten skal feile; gamle hashar må ikkje skrivast om for å få grønt resultat. Ingen bootstrap-binær, hash, godkjend-binær-pin eller stage-0-seed er endra her.

Før innfletting må isolerte kandidatar byggjast og testast for alle støtta vert-/mål-kombinasjonar, inkludert Linux-fikspunkt, formatregresjonar og eksisterande attestasjonar. Eventuell erstatning av pinna kodegeneratorar krev særskild gjennomgang og godkjenning. Denne kjeldeendringa er ikkje klar for innfletting før det er gjort.
