# macOS ARM64 assert_eq-kandidat

Status: implementert og lokalt testa, ikkje promotert. Endringa gjeld òg ARM64-emitteren som Linux ARM64 bruker, men faktisk Linux ARM64-køyring av denne rettinga er ikkje stadfesta her.

Den pinna macOS-kodegeneratoren samanlikna berre 64-bit nyttelast for andre verdiar enn tekst. Dermed kunne assert_eq(1, sann) gi falsk PASS, og to bytebufferar med likt innhald kunne bli avviste. Kallet returnerte dessutan det andre argumentet; VM-kontrakten er 0.

Rett konsum av NCB normaliserer no gyldige assert_eq-kall til ein reell NCB-funksjon med COMPARE_EQ, formatert THROW ved ulikskap og retur 0 ved likskap. Han bruker den vanlege samanlikninga og kallkonvensjonen. Fullbygg, delar, plan og ferske batchar går gjennom normaliseringa. Reserverte namnekollisjonar blir avviste utan overskriving, og gjenteken normalisering er idempotent. Den gamle ufullstendige maskinkodehjelparen er erstatta, og ein unormalisert direkte builtin-veg blir avvist.

## Testbevis

- test_assert_eq_value_semantics.no består i VM før endringa, men feilar som faktisk Mach-O med den pinna gamle kodegeneratoren. Etter endringa består han både med kjeldekøyrd emitter og ein ny native kodegeneratorkandidat. Testprogrammet har rapport uløyste=0, stubba=0.
- Testen kontrollerer avvising av bool/heiltal, null/heiltal, tekst/heiltal, ulike lister og bytes; aksept av like nøsta lister, bytes, null, desimaltal og blanda taltypar; feilprefiks og returverdien 0.
- test_macos_assert_eq_lowering.no kontrollerer idempotens, kollisjonsavvising, byteparitet fullbygg/delar og batch/flat-link, og køyrer faktiske Mach-O-image frå alle lenkevegane på macOS ARM64. Fullbygg og batch internerer strengkonstantar ulikt og er ikkje hevda byte-identiske. Faktisk feiltype 1/sann skal avslutte med feil i både fullbygg og batch.

Dei tre relaterte testane består gjennom testløparen, med null feil og null hoppa testar. Endeleg avgrensa feature-check og diffkontroll består. Heile plattformmatrisa er ikkje ferdig.

## Kontrollert kandidatbygg

Ingen fil under bootstrap eller dist er endra. Den uendra produsenten er bootstrap/native_codegen_macos-arm64_on_macos-arm64.macho frå basen, SHA-256 e1ac1c25a0a5283accb640fcfff5355d53e99061e2e1f128294f311bbedf77e2. codegen_prebuilt.vel på den uendra kjelderota må gje status ok før han blir brukt. I ei eiga kandidatrot blir kjelda bundla med:

~~~sh
NORSCODE_USE_PRECOMPILED_SELFHOST=0 NORSCODE_DISABLE_MODULE_CACHE=1 ./bin/nc bundle --output build/assert-codegen.ncb.json selfhost/native_execution/macho_arm64_codegen.no
~~~

Køyr den verifiserte produsenten med NC_INPUT sett til denne NCB-fila og NC_OUTPUT til build/assert-codegen-gen1. Køyr deretter gen1 og gen2 på same NCB for å produsere gen2 og gen3. Bruk NC_BUILTIN_REPORT=1 og kontroller faktiske returkodar, rapportar og filhashar.

Observert NCB SHA-256: 46accdea5b873be2f7f4f782d18c04fe544fdc1a2217eaac694496edc9a19f4b.

- Gen1: eb7b0343d92cc401d66639016e0b4c8101646f60bda60196091190ea33110255.
- Gen2 = Gen3: 83885951c59d72473dda8bbdb256e8cac35b5a86b3722a95666af7bcef0f895f.

Gen1 er laga av den gamle emitteren; interne assert_eq-kall får først den nye implementasjonen når den nye emitteren produserer Gen2. Gen2/Gen3-fikspunktet erstattar ikkje krav om funksjonelle testar.

## Det som står att

Kodegeneratorkandidaten rapporterer framleis sju eksisterande native stubbar (native_fs_unlink, rmdir, chmod, rename, symlink, stat og sha256_bytes_hex). Dette er ikkje ein komplett runtime eller eit bevis på 100 prosent sjølvstende. Kandidatbygg og testing på alle relevante vert/mål, heile verifiseringsløypa og separat godkjenning av eventuell promotering står att. Kjeldeendringa skal avvise gamle prebuilt-hashar; hashfiler skal ikkje oppdaterast for å få gamle binærar godtekne. Den pinna seeden og gamle prebuilt-artefaktar har framleis den gamle åtferda fram til ei godkjend utskifting.

Faktisk test_native_codegen_srchash.no feilar med exit 1 på bootstrap/native_codegen_linux-arm64_on_linux-x86_64.elf.depshash etter kjeldeendringa. Denne ferskheitsfeilen er ikkje overstyrt; nye krysskodegeneratorar må byggjast og verifiserast.
