# Dokumentasjonsstatus

Dette viser status for dokumentasjonen som faktisk ligg i repoet.

## Norscode 1.0-sluttport

`./bin/nc completion-gate --release` er no den autoritative ferdigporten.
Statusprosa kan ikkje gjere Norscode produksjonsklar; berre den versjonerte
`norscode-completion-gate-v1`-rapporten frå ein rein commit kan gi
`overall=green`.

Statusen skal lesast frå sluttportrapporten på same commit. Etter at ekte
Windows, Linux ARM64 og ACME-plattformbevisa er signerte, er statusmatrisene
løfta til 22/22 runtime og 61/61 standardbibliotek. Den endelege
`overall=green`-påstanden er likevel berre gyldig når `completion-gate
--release` har produsert grøn rapport på akkurat denne committen.

## Sjølvstendigheitsstatus 2026-07-20

- Legacy compiler-seeding via ekstern shell er fjerna frå native backend.
  Stage0-bundlen eller Norscode sin selfhost-dispatch er einaste compile-veg.
- Releasepakkinga verifiserer no NCB-sluttmarkøren i den endelege
  `dist/norscode_native` før arkivet blir godkjent.
- `std.zip` har no reine Norscode TAR- og gzip-stored-block-skrivarar over
  `std.bytes` som fallback når native arkiv-ABI manglar; `tests/test_tar_pure.no`
  verifiserer header, ustar-markør, blokkpadding, gzip-magic og binær payload-
  roundtrip. Katalogvandring
  blir no enumerert av releaseeigaren og binærarkivet blir bygd i Norscode.
  Pure gzip/TAR-dekomprimering med traversalvern er også kopla inn i
  `tools/install_release.no`; katalogenumerering, kopiering og sletting i
  kjernebanen brukar no native filesystem-listing utan `find` eller symlink.
  Treopprydding brukar depth-first Norscode-byte-I/O med
  `builtin.fil_slett`, utan `rm -rf`; aktiv release og binæralias blir
  materialiserte kopiar, utan `ln -s`. Målversjonen materialiseres no direkte
  frå arkivet i Norscode utan `cp -R`.
- Pure gzip har no ein blokkvis binær-append-ABI (`fil_append_binary`) i
  selfhost-VM/native-dispatch, slik at store arkiv ikkje treng å samle heile
  komprimertestrømmen i éi mellomliste når kandidaten er regenerert. ABI-en er
  med vilje ikkje promotert til dist/stage0 før ein separat kandidat har
  passert runtime-gap og gzip-smoke.
- `tests/test_native_filesystem_list.no` er lagt til som eksplisitt ABI-smoke;
  dagens aktive dist/stage0 svarer framleis `operation requires disk.write`,
  så ny native kandidat må byggjast før `filesystem.list` kan promoterast.
  Kandidatmodus bruker no `build/norscode_native_filesystem_list_candidate` og
  kan ikkje overskrive validert `dist/norscode_native` før smoken passerer.
- Ein midlertidig macOS ARM64 kandidat bygd frå `build/v3009/native_candidate_combined.c`
  har no passert både `native filesystem.list OK`, innebygd
  `NORSCODE_NCB_TEXT_V1`-trailer og `native runtime gap gate v3001`. Kandidaten er
  ikkje promotert: sjølve kompileringa må flyttast inn i den Norscode-eigde
  build-pathen før dist/stage0 kan erstattast.
- Kandidatens separate BPE-smoke feilar med `assert feilet`, medan same test er
  grøn i aktiv `./bin/nc`. Det viser at den genererte native selfhost-flata er
  eldre enn dagens `std.tokenizer_bpe`; kandidatbygging må derfor regenerere
  selfhost-koden frå aktuell bootstrap før promotering.
- `archive/legacy_c_backend/ncb_to_c.no` har no stream-skriving av header,
  deklarasjonar og funksjonsblokker. Dette reduserer toppminnet, men dagens
  992-funksjons bootstrap stoppar framleis før funksjonsblokkene er ferdig
  materialiserte; full funksjonsinstruksjons-streaming står att. Pure VM-en
  unngår no heap-traversering for store NCB-tabellar og funksjonskart ved
  `INDEX_GET`, men kandidatgenereringa må framleis gå heilt gjennom alle
  funksjonar før binær kan validerast. Pure runtime-memory har samstundes
  standard GC-terskel 16 (overstyrbar med `NORSCODE_VM_GC_THRESHOLD`) for å
  halde store AOT-materialiseringar under minnetopp.
- Releasepakka kopierer no katalogtre og `.nors`-alias med Norscode-byte-I/O
  og native `mkdir_p`; `cp` er ikkje lenger reserve for normal katalogkopi.
- `std.pathlib.lag_katalog` og `std.pathlib.slett_fil` brukar no native
  `mkdir_p`/`fil_slett` i staden for shell; release-preflight krev desse
  eigarskapspunkta direkte.
- `std.pathlib.storleik` brukar no native binær-lesing og `lengde` i staden
  for `stat -c`, med same bytepresise resultat for releasefiler.
- `std.pathlib.les_bytes` og `skriv_bytes` brukar no `std.bytes` og native
  binær-I/O; vilkårlege byteverdier blir dermed bevarte utan tekstkonvertering.
- macOS ARM64 `dist/norscode_native` og stage0 er no bygde frå shell-fri
  native backend, identiske med kvarandre, og `platform_readiness_v3600` viser
  `production_ready_macos=true` med innebygd NCB.
- Compiler-NCB-en har fått bundler-cacheforbetringa fletta inn som ein
  målretta Norscode-NCB-oppdatering; full kjeldebundle-regenerering er framleis
  skild frå denne verifiserte kandidaten.
- Pure selfhost-VM har kontrollert bootstrap-tilgang til `env.read` og
  `disk.read` innanfor `NORSCODE_VM_DISK_ROOT`; andre capabilities blir ikkje
  opna av bootstrap-flagget.

- AOT-integrasjon er no verifisert for macOS ARM64, Linux x86_64 og Linux
  ARM64: kontrollflyt, funksjonskall, importert stdlib og vanlege
  heiltalsoperasjonar køyrer native. Linux ARM64-gaten er no også grøn lokalt
  under QEMU/Docker.
- Linux x86_64 `bygg-native --target linux-x86_64` brukar no
  `native_codegen_v2.no` med innebygd NCB. Docker-attestasjonen køyrer også
  felles AOT-runtime for liste, tekstfunksjonar og kartoppslag (exit 25).
  ASCII `lower`/`upper` er emitterte runtime-funksjonar i same Norscode-bytebuffer;
  dei gamle faste legacy-peikarane blir ikkje brukte.
- `platform_readiness_v3600` er køyrd med `NORSCODE_VERIFY_LINUX_DOCKER=1`:
  `production_ready_linux_x86_64=true`, `production_ready_linux_arm64=true`
  og `production_ready_unix=true` etter runtime-gap-attestasjon i Docker.
- Samla `./bin/nc selvstendighet` er no køyrd ende til ende og rapporterer
  `Sjølvstendighet L1-L6 (normalflate): BESTÅTT`: bootstrap A+B/C, L5
  Gen1/Gen2-byteparitet og den separate L5b-VM-pariteten er alle verifiserte.
- Aktiv compile-, bundle-, test- og scaffold-flyt er eigd av Norscode `.no` og
  køyrer med `NODE_BIN=/nonexistent`. Node/JavaScript, Python og C er ikkje del
  av den aktive kjeda; eigarskaps- og C/Python-gatene er grøne.
- `dist/norscode_native` er byte-identisk med committed macOS ARM64-stage0.
  Committed Linux x86_64- og ARM64-seedar har same hash som dei køyrde
  Zig/Argon2-kandidatane. Begge Linux-arkitekturar har grøn runtime-gap-gate i
  Docker. `tools/single_binary_gate.no` køyrer i tillegg macOS-binæren åleine
  frå tom runtime-rot med `PATH=/nonexistent`, utan repo eller NCB-sidefiler.
- Stream-embedderen setter no executable-bit på `dist/norscode_native_embedded`,
  og single-binary-gaten er køyrd mot dette innebygde artefaktet med tom
  runtime-rot og utan side-NCB.
- `nc selfhost-bootstrap-gate` sender no eksplisitt capability- og disk-scope
  til både NCB-host og fallback-runtime; bootstrap A+B passerer utan manuell
  miljøjustering.
- Embedded runtime-entry-testen er verifisert med `NORSCODE_VERIFY_EMBEDDED_PROCESS=1`;
  child-kallet nullstiller flagget og unngår rekursiv prosesskjøring.
- NCB→ELF-releasebanen er no native-only: `tools/ncb_to_elf.no` brukar
  `std.runtime.process_abi`, eksplisitt miljøkart, native mappe-/filmodus-ABI og
  fail-closed runtime-mangel. `nc ncb-to-elf` og `nc bygg-native --ncb` brukar
  ikkje shell-reserve i releaseflata; shell-wrapperen står berre som avgrensa
  vedlikehaldsreserve.
- Windows x86_64 runtime, IOCP/SChannel, AppContainer og Argon2 krysskompilerer
  og lenkar. Cross-gaten legg no også compiler/VM/executor-NCB inn i PE-fila
  og verifiserer PE32+ direkte i Norscode utan avhengigheit til `file`. Dette
  er ikkje ekte Windows-attestasjon:
  `production_ready_windows=false` og `production_ready_all_platforms=false`
  står ved lag fram til køyring på ein faktisk Windows-vert.
- Tracked `bootstrap/stage0/norscode-windows-x86_64.exe` er no regenerert frå
  same innebygd-NCB PE-kandidat som Windows-kryssgaten verifiserer, og blir brukt
  som Git Bash/MSYS-fallback for `bin/nc`; han er statisk PE-verifisert, men er
  ikkje rekna som produksjonsklar før den signerte Windows-køyringa er
  gjennomført.
- `tools/windows_runtime_attestation.no` gir no éi fail-closed Windows-port som
  sjølv køyrer backend/filsystem/prosess, ekte AppContainer, nettverk,
  SChannel TLS 1.3, IOCP og Argon2id før ho skriv ein commit- og
  binærhashbunden rapport. Hovud-CI og Windows-release signerer rapporten med
  GitHub build provenance. `tools/platform_readiness_v3600.no` godtek berre
  rapporten når `gh attestation verify`, commit, rein spora kjelde og runtime-
  SHA-256 alle samsvarer; krysskompilering eller ein laus JSON-fil kan derfor
  ikkje gjere Windows grøn. CI set no `NORSCODE_REQUIRE_WINDOWS_ATTESTATION=1`
  slik at jobben feilar dersom samla Windows-readiness ikkje er grøn.
- `std.runtime_status` rapporterer 22 av 22 runtimeområde som stabile (100 %).
  `network_runtime`, `process_api` og `runtime_security` er løfta etter ekte
  Windows IOCP/SChannel/AppContainer-attestasjon og Linux/macOS native gates.
- `std.stdlib_status` rapporterer 61 av 61 modular som stabile (100 %).
  `sikkerheit`, `dns`, `tls_acme` og `domenehost` er løfta etter signert
  flerplattformbevis, Pebble DNS-01 og `test_domenehost_e2e.no`.
- Repoets 10/10-modenheitsgate er ei produkt-/bevisflate og er grøn med
  atferdssjekkar for CLI, doctor, LSP, seed, distribusjon, stdlib, produksjon og
  Norscode-eigarskap. Ho skal ikkje tolkast som at dei tre delvise
  runtimeområda eller ekte Windows-attestasjon er ferdige.
- `NODE_BIN=/nonexistent ./bin/nc verify-selvstendighet` er køyrd ende til
  ende etter endringane: normalflata hadde 563 bestått, 0 feila og 20
  lane-/plattformfiltrerte; langtidsflata hadde 11 bestått og 0 feila. L5 og
  L5b gav byte-identiske etterfølgjande generasjonar.
- Testløparen prioriterer no `SLOW_TEST_TIMEOUT` over den generelle
  `TEST_TIMEOUT` i langsamflata. Dette hindrar falske `SIGALRM`-feil for
  `test_chunk_tail` og `test_selfhost` når begge grensene er sette; begge
  testane og heile 11-testers langsamflate er verifiserte grøne med 600 sekund.

## Gjeldande struktur

- `docs/05-development/`
- `docs/_archive/`
- `docs/assets/`

## Aktiv inngang

- [Dokumentasjonsinngang](INDEX.md)
- [Brukarmanual](USER_MANUAL.md)
- [Opplæringsguide](LEARNING_GUIDE.md)
- [Dokumentasjonsindeks for vedlikehald](DOCUMENTATION_INDEX.md)
- [Løypekart](LANE_MAP.md)
- [Arkivindeks](_archive/ARCHIVE_INDEX.md)

## Vedlikehald

## Runtime-gate 2026-07-13

## Helpdesk-deployment 2026-07-13

- Deployment-kontrakten er implementert i `std.deployment_config`, `std.transactional_storage`, `std.job_queue`, `std.quick3`, `std.inbound_email`, `std.vedlegg` og `std.observability`.
- `tests/test_helpdesk_deployment_contract.no` køyrer grønt med database-transaksjon, idempotent jobbkø, Quick3-mapping, MIME-korrelasjon, webhook-signatur og vedleggspolicy.
- Kontrakttesten dekker òg faktisk SQLite-backup/restore, Quick3 retry-plan, deterministisk HTTP-mock med 503/201-statuskø og samlet secrets-navnerom for `QUICK3_API_TOKEN`, `SMTP_PASSWORD` og `INBOUND_WEBHOOK_SECRET`.
- `tests/test_pbkdf2_vector.no` verifiserer standard PBKDF2-HMAC-SHA256 mot kjent testvektor. Passordløypa brukar 120 000 iterasjonar og har native CommonCrypto på macOS og OpenSSL i Linux-releasekandidaten, med Norscode-fallback. `std.scrypt` er rein Norscode med RFC 7914-vektor. Argon2id er verifisert i committed macOS ARM64-, Linux x86_64/ARM64- og ekte Windows-attestasjon; sikkerheitsflata er produksjonsklar.
- Jobbkøen har no timeout-reclaim: krasja workers blir frigitt til retry, medan jobbar som har nådd maks forsøk går til dead-letter. Indeks-API-et avviser ugyldige SQL-identifikatorar før SQL blir bygd.
- SQLite-lagringa set no `busy_timeout` og WAL-journal ved opning, i tillegg til foreign keys, transaksjonar, migrering, backup og restore.
- MIME-parseren dekodar no `base64` og `quoted-printable`, bevarer rå kropp ved feilsøking, avviser råmeldingar over 10 MiB og har grønn integrasjonstest for HTML, tekst og base64-vedlegg.
- Quick3-klienten avviser HTTP-base-URL, validerer minimumsfelt i typemapping og tilbyr `trygg_responslogg` som berre eksponerer status, forsøk og OK-flagget utan token, body eller rå response.
- `./bin/nc bundle` gir no runtime-capabilities på same måte som `run`, og enkeltmodul- og modulær bundling er verifisert grønt: `admin=std/admin.no` (508 KB) og sju Helpdesk-moduler samla (716 KB). Hybrid-løypa viser no òg underliggjande kjelde-/runtime-diagnose ved compile-feil i staden for å skjule output.
- Native VM-dybdegrensa er 256 under vanleg køyring og compiler-løypa set automatisk 4096 (begge konfigurerbare til 16384). Feilen rapporterer fil, funksjon, linje, kolonne, uttrykk, faktisk djup og aktiv grense. Dette fjernar den tidlegare `For djup rekursjon`-blokkeringa for store program under kompilering utan å la uendeleg runtime-rekursjon presse C-stakken; Helpdesk `app.no` er verifisert med promotert macOS ARM64-runtime og korrekt `NORSCODE_PROJECT_ROOT`.
- Helpdesk har no ein verifisert modulflate i `prosjekter/NorscodeHelpdeskAI/src/`: `storage.no`, `tickets.no`, `vehicles.no`, `email.no`, `quick3.no`, `auth.no` og `admin.no`. Modulkontrakten køyrer reelle ticket-, kjøretøy-, e-post-, Quick3-, PBKDF2- og audit-operasjonar med deduplisering; `storage.no` bruker transaksjonell SQLite v2 med WAL og indeks, og separat v9500-bundle byggjer alle sju modulane. Den store eksisterande `app.no` blir framleis bygd som eiga monolittisk produksjonsflate medan handlerflytting skjer trinnvis.
- `bin/nc run` finn no prosjektroten frå kildefila, slik at prosjekt med eigne `norcode.toml` og `src.*`-modular køyrer med riktig importbase og datamappe.
- Modenheitsgaten for standardbiblioteket er løfta til 61/61 stabile etter grøne
  roundtrip-, inferens-, tokeniserings-, native socketserver-, modellregister-,
  native multiprocessing-, lokal q8-media-, ACME/Pebble- og
  `test_domenehost_e2e.no`-tester.
- DNS-flata har no også deterministisk wire-query og autoritativt svar for A, CNAME, MX og TXT, native UDP daemon lifecycle med bind/mottak/svar/stop, korrekt JSON-liste for sone-records, verifiserbar SHA-256 DS-digest/validering, native RRSIG-signering og native RRSIG-validering for RSASHA256/ES256/Ed25519; release-/provider- og flerplattformgaten er grøn.
- Kryptografi-fallbacken brukar no den reine Norscode SHA-256/HMAC-implementasjonen i staden for ein svak concatenation-hash. `std.scrypt` implementerer RFC 7914 scrypt med testvektor og harde pure-VM-parametergrenser; Argon2id er verifisert mot RFC 9106 med OpenSSL-native backend i lokal macOS `dist`.
- `tests/test_scrypt.no` verifiserer scrypt mot RFC 7914-vektoren for N=16, r=1, p=1, med feil-passord og parametergrenser; implementasjonen ligg i `std/scrypt.no` og bruker berre Norscode SHA-256/HMAC i pure VM.
- `std/argon2id.no` og `tests/test_argon2_native.no` verifiserer RFC 9106 Argon2id med grenser for minne, iterasjonar, parallellitet og lengder. Lokal macOS `dist/norscode_native`, committed macOS/Linux-stage0 og ekte Windows x86_64-attestasjon har grøn backend-verifisering.
- `std.tls_acme` har no full RFC 8555-flyt med HTTPS directory og Replay-Nonce, ny konto med `jwk`, vidare kall med `kid`, ordre, autorisasjon, challenge, CSR-finalisering, sertifikatnedlasting og atomisk fornying. Linux CI brukar ein fastlåst Pebble-digest og Norscode sin autoritative DNS-daemon over RFC-frama DNS/TCP; ekte Windows-attestasjon verifiserer native ACME-signering og sertifikatverifisering.
- `std.sikkerheit` samlar no passordkrypto og capability-policy gjennom ein eksplisitt Norscode-native fasade; algoritmegrenser, pure-VM fallback, committed macOS/Linux-stage0 og deny-by-default er testa, og ekte Windows Argon2id/AppContainer/ACME-bevis er signert i plattformgaten.
- `std.dns.resolve` bruker no native `dns_lookup` med `net.dns`-capability, numeriske IPv4/IPv6-adresser og eksplisitt feilstatus. Testløparen gir `net.dns` til nettverkstestar utan å opne andre capabilities.
- Aktiv macOS ARM64-runtime har native PBKDF2-HMAC-SHA256 via CommonCrypto, Argon2id via OpenSSL og rein Norscode scrypt, med fallback der backend manglar. Linux-stage0 har OpenSSL PBKDF2/ACME og Zig Argon2id. Kjent PBKDF2-, scrypt- og Argon2id-vektorar, `tests/test_security.no` og ekte Windows-attestasjon er grøne.

- Lokal macOS ARM64-runtime er promotert til `dist/norscode_native` med OpenSSL 3.6.2 for Argon2id og ACME/DNSSEC-signering. Committed macOS ARM64- og Linux x86_64/ARM64-stage0 er oppdaterte og hashfesta i `bootstrap/stage0/SHA256SUMS`.
- Den promoterte macOS ARM64-runtime-en er statisk lenka mot OpenSSL (ingen OpenSSL-dylib i `otool -L`) og passerer `tests/test_acme_sign_native.no`, `tests/test_acme_verify_native.no`, `tests/test_pbkdf2_vector.no`, `tests/test_argon2_native.no` og `tests/test_security.no`.
- Native runtime-gap, JIT for heltall/desimal/tekst, native tråd- og tensorflate, `builtin.process_spawn_argv` og `tests/test_runtime_v1_contract.no` er verifisert grønne etter promoteringen.
- Adaptive JIT har no ein eigen Norscode-optimaliseringspass med iterert konstantfolding, identitetsreduksjon, SSA-livstidsanalyse og register-/spillplan; dette er verifisert i `tests/test_runtime_jit_optimizer.no`.
- Windows x86_64 native ABI blir no reproducerbart kompilert og lenka gjennom `tools/windows_runtime_cross_compile_gate.no`, inkludert hovudruntime, IOCP/SChannel-backend, Zig Argon2id-objekt, fil/prosess/IOCP-backend-smoke, dynamisk `winsqlite3.dll`/`sqlite3.dll`-laster og AppContainer-smoke-`.exe`; ekte Windows TLS/AppContainer-handshake er verifisert i den signerte runtime-attestasjonen.
- Windows ABI-gaten køyrer no også i hovud-CI på kvar push og pull request, med eksplisitt `process.exec`/disk-scope, korrekt `.exe`-sti for native runtime-testen og opplasta diagnoseartefakt; ekte Windows execution-gate køyrer på faktisk Windows og inngår i den signerte sluttattestasjonen.
- `tools/windows_runtime_execution_gate.no` køyrer backend- og AppContainer-smoke på faktisk Windows-host; på macOS/Linux rapporterer han eksplisitt ekstern verifisering i staden for å gi falsk grønt.
- Windows-release-workflowen byggjer no backend-smoke med Zig og køyrer execution-gate, SChannel, native network, IOCP og filesystem-testar før app-pakking.
- Linux x86_64-kandidaten `build/v3600/linux/norscode_native_linux_x86_64_v3602` er krysskompilert med Zig og køyrd gjennom full `native_runtime_gap_gate_v3001` i Ubuntu 24.04 Docker. Den portable Argon2id-kandidaten `build/v3600/linux/norscode_native_linux_x86_64_v3605_zigargon` inkluderer også OpenSSL for PBKDF2/ACME, passerer målretta Linux-sikkerheitstestar og er kopla inn i `.github/workflows/publish.yml` gjennom `tools/linux_zig_argon_release_gate_v3607.no`; stage0-seed og ekte Windows-køyring inngår i dei hashbundne CI-attestasjonane.
- Linux ARM64-kandidaten `build/v3600/linux/norscode_native_linux_aarch64_v3608_zigargon` er køyrd native i ARM64 Ubuntu med portable Zig Argon2id og OpenSSL. Han passerer prosess-sandbox, multiprocessing, ACME, PBKDF2, security og full testflate: 560/560 bestått, 0 feila og 20 eksplisitt plattformfiltrerte.
- `tools/nc_test.no` vel no stat-/hashsignatur etter faktisk operativsystem, slik at Linux-testar ikkje prøver macOS `stat -f` eller `shasum` før fallback. CLI-eksempelet bruker `TMPDIR` i staden for hardkoda macOS-sti.
- Linux OpenSSL-kandidaten `build/v3600/linux/norscode_native_linux_x86_64_v3603_openssl` blir no reproducerbart bygd av `tools/build_linux_openssl_candidate_v3604.no`; ACME-signering/verifisering og runtime-gap er grøne. Ubuntu OpenSSL 3.0 manglar Argon2 KDF-parametrar, så Argon2id feilar lukka på OpenSSL-banen. `archive/legacy_c_backend/nc_argon2_zig.zig` gir i tillegg ein portable RFC 9106-backend; `tools/build_linux_zig_argon_candidate_v3606.no` byggjer og verifiserer Linux-kandidat med `tests/test_argon2_native.no` grønt.
- Runtime v1-matrisa er no 22 stabile av 22; normal- og slow-testflatene er eksplisitt med i sluttporten og må passere utan uforklarte hopp. Testløparen bruker signaturbasert NCB-cache i eiga cache-mappe (`NC_TEST_CACHE_DIR`), atomisk artefaktflytting og unik arbeidsmappe per køyring (`NC_TEST_RUN_ID`) for å unngå delte eller utdaterte `.ncb.json`-artefaktar. Cache kan slåast av med `NC_TEST_CACHE=0`.
- Media runtime-gaten køyrer no eksplisitt i både macOS- og Linux-hovud-CI med aktiv native runtime, inkludert CPU-diffusjon og binær medie-I/O.
- macOS-CI har ein faktisk Metal compute-gate som kompilerer og køyrer GPU-kernel med buffer-I/O, dispatch, synkronisering og resultatkontroll; `std.tensor.matmul`, `media_neural` og `std.media_diffusion.bilde_med_backend` vel Metal når runtime rapporterer GPU, med SIMD/CPU-fallback. Gaten er no køyrd grøn på Apple M3, inkludert tensor-matmul og diffusjonskernel.
- Aktiv v3010 media-gate er grøn for bytes/binær I/O, lokal mediegenerering, deterministisk CPU-diffusjon, bounded modellserver, modellruntime/register/tillit, tokenizer, transformer, tekstgenerering og HTTP/Base64-medieflyt. `media_neural` eksponerer faktisk tensor-backend per inferens og brukar Metal-matmul når eininga finst, elles den obligatoriske og verifiserte CPU-fallbacken. Metal compute, tensor-ABI og diffusjonsadapter er kompilert og køyrde på Apple M3; media-kontrakten er derfor stabil utan å gjere GPU til eit krav på plattformer som ikkje har Metal.
- GC-promotering er derfor fjernet som aktiv blokkering i `std.runtime_status`; den maskinlesbare 22/22-matrisa og sluttgaten er den autoritative statusen, medan eldre delstatus er historisk.

- Normal løype: `./bin/nc run`, `./bin/nc check`, `./bin/nc test`
- Vedlikehald: `./bin/nc maintenance`
- Utvikling av nye funksjonar: `./bin/nc feature-check [fil.no ...]`
- Verifisert 2026-07-14: heile testmengda er grøn med 560 bestått og 0 feila; 20 testar er eksplisitt plattformfiltrerte (`lane-filter/native-unsupported`).
- Verifisert 2026-06-27: `./bin/nc feature-check app.no`, `./bin/nc selfhost-bootstrap-gate`, `./bin/nc bootstrap-self`, `./bin/nc verify-seed` og `./bin/nc verify-selvstendighet` gjekk grønt.
- Server-røyk 2026-06-27: `./bin/nc serve app.no --port 18181` svarte med `200` på `/`, `404` på ukjend sti, `200`/tom body på `HEAD /`, og `204` med `Allow` på `OPTIONS /`.
- `std.ai` er kontraktsverifisert i normalflata via `tests/test_ai.no`.
- `std.ai` er no også verifisert i pure VM og har stabil lokal AI-runtime for chat, embedding, moderering, plan, avgrensa agentkøyring, bounded søkbart minne og dynamiske verktøykall. Verktøydispatch krev deklarert capability, returnerer synlege feil og aukar auditmåling; lokal provider har nettverk eksplisitt deaktivert.
- `std.runtime_gui` har no stabil backend-nøytral rendering i normal og pure VM: app/vindu, knapp, tekstfelt, liste, canvas og webview, deterministisk vertikal/horisontal layout, fokus, accessibility tree, eventkø, bubbling, dynamisk Norscode-handlerdispatch og faktisk HTML/headless-output. Eit OS-vindu eller system-webview kan koplast på som adapter utan å endre GUI-runtime-kontrakten.
- `std.socketserver` brukar no native TCP/HTTP-listener med eksplisitt lifecycle, automatisk port, portprobe og handlebasert cleanup utan `nc`, `socat`, `sh`, `kill` eller `lsof`. UDP-operasjonane er lagt til i kandidat-ABI-en, men ventar kontrollert promotering av aktiv runtime etter full runtime-gap-gate.
- Framtidig mailserver/domenehost/brannmur ligg no som Norscode-standardflate i `std.dns`, `std.tls_acme`, `std.mail_server`, `std.domenehost` og `std.brannmur`, med samla infrastrukturplan, SMTP-runtime, native daemon-livssyklus (`daemon_ny`, eksplisitt bind, `daemon_pump`, `daemon_stop`), starttls/implicit-TLS-policy, atomic mail-spool, retry, dead-letter, mail-karantene, DNS-resolver-plan, DNSSEC-plan med eksplisitt DS-attestasjon, ACME-auto-renew, backup/restore, brannmur egress og ekte nftables-reglar med default-drop, loopback, etablert trafikk, kjeldeavgrensing, rate-limit-drop, dry-run/check og rollback-plan. Runtime-security capability-policy og kontraktstestar er på plass før vidare OS-sandbox- og ekstern Windows/ACME-verifisering.
- `tools/hosting_runtime_gate_v3610.no` samlar no DNSSEC, ACME/JWS/keyring, domenehost, mailserver og brannmur i same release-runtime-gate. Gaten brukar hybrid kompilering frå aktiv kjelde og køyrer NCB-artefaktet med vald runtime, slik at stage0/precompiled-modulstatus ikkje blir forveksla med aktiv standardbibliotek-kjelde.
- Brannmur, mailserver og domenehost lagrar no reglar, rate-limits, admin-kjelder, postboksar, aliasar, SMTP-mottakarar og admin-allowlist som ekte JSON-lister også i pure VM; dette er verifisert i `test_brannmur_policy`, `test_mail_server_contract`, `test_domenehost_security_contract` og `test_future_hosting_foundation`.
- Runtime v1-matrisa er no autoritativt 22/22 stabile i `std.runtime_status`, med `tests/test_runtime_v1_contract.no` og sluttgaten som kontroll. `norscode-runtime-library-abi-v1`, `norscode-memory-abi-v1`, continuation-stack, exception, type, refleksjon, filsystem, nettverk, prosess, tråd, sikkerheit, media, GUI og JIT er dekte av dei same runtime- og plattformattestasjonane; eldre delstatus i dette dokumentet er historisk og skal ikkje brukast som releasebevis.
- Pure-VM-sikkerheit er no handheva før builtin-dispatch: fil, mappe, lagring, database, miljø, prosess og socket/nettverk krev eksplisitte capabilities. Standard er deny, native filhandtak avviser traversal og symlink-folging, read/write-builtins kan ikkje kryssbrukast, og auditmåling viser tillatne og avviste kall. Nye listen/connect/TLS-handtak må samsvare med network scope, og executable må samsvare med process scope. Prosess-ABI-et avviser no ukjende sandboxprofilar, ugyldige miljøflagg og minne-/fd-grenser før dispatch. Prosessar kan bruke lukka miljø, rlimits, Linux seccomp no-network/no-write/pure og macOS Seatbelt med RLIMIT_RSS/RLIMIT_DATA og forelder-watchdog; Windows AppContainer/Job Object og faktisk Windows-attestasjon er dekte av plattformgaten.
- `NORSCODE_VM_DISK_ROOT` er ei komma-separert katalog-allowlist; `disk.read` eller `disk.write` gir ikkje tilgang utanfor dette scopet. Tom verdi blir avgrensa til relative stiar under gjeldande prosjektrot.
- Runtime-allocator er ikkje lenger ein ugyldig placeholder: `std.runtime.allocator` køyrer med 64 MB avgrensa bytearena, 8-byte alignment, bounds-kontrollert byte-I/O, nullstilling ved allokering/gjenbruk, first-fit-gjenbruk, double-free/OOM-vern og statistikk. Flyttande komprimering tek overlappssikre snapshots og bevarer faktisk blokkinnhald ved relokasjon. `std.runtime_memory` tildeler header/payload-adresse per objekt, eksponerer bounds-kontrollert payload-I/O, bevarer payload gjennom GC-komprimering og returnerer sweep-blokker til allocatoren. Native ELF-backend og den fysiske ABI-bindinga mellom RW-segmentet og GC-bytearenaen er verifiserte.
- `std.sched` brukar ei runtime-uavhengig, flat hendingskø med tid, prioritet, deterministisk ID-rekkefølgje, cancel, forfall, stopp/start og avgrensa run-loop. Native readiness brukar ein vedvarande trådlokal kqueue på macOS, EPOLLONESHOT på Linux og aktiv IOCP på Windows. IOCP-banen dekkjer AcceptEx, ConnectEx, WSARecv, WSASend og overlapped ReadFile/WriteFile med generasjonsbundne, filtrerte completion-handtak. Timeout, kansellering, drenerande cleanup og VM-`await` er kopla til same future-modell og er køyrde under Windows/Wine. Preemptiv scheduler står att; eksplisitt parallellitet går gjennom den native arbeidspoolen.
- Scheduler-futures eig no status, resultat, feil og kansellering i same event-loop-tilstand og er grøne i normal og pure VM. Den pure reentry-låsen vart spora til nyleg JSON-parsa argument ved callback-grensa; køa held no det opphavlege, rotfesta argumentkartet utan unødvendig serialisering. Pure VM nullstiller dessutan ventande argument-heap-ID-ar ved dynamiske kall, har konfigurerbar GC-terskel og opt-in kallsporing.
- `std.prosess` har levande POSIX-prosesshandtak med shell-fri `fork`/`execv`, PID, ikkje-blokkerande stdin/stdout/stderr, poll, timeout/deadline, TERM/INT/KILL/HUP, exit-kode, inkrementell streaming og slot-gjenbruk. Prosesshandtak kan fullføre scheduler-futures som vekkjer suspendert VM-`await`. Sandboxprofil gir lukka miljø, rlimits, Linux seccomp no-network/no-write/pure og macOS Seatbelt no-network/no-write med Darwin RSS/DATA-grense og forelder-watchdog. Windows CreateProcess/Job Object og faktisk Windows-attestasjon er verifiserte i plattformgaten.
- Pure VM-profilaren måler CPU-instruksjonar/rate, veggtid, heap/allokering, GC-samlingar/safe-points/pause, builtin-kall og fil/nett/prosess-I/O. Konfigurerbar statistisk sampling fangar funksjon, IP, opcode, kjeldelinje/-kolonne og full call-stack med avgrensa buffer og dropteljar. Per-funksjon histogram dekkjer kall, instruksjonar og veggtid, og funksjonsintervall blir eksporterte som standard Chrome Trace Event JSON. Integrasjonstestane utløyser faktisk heap, GC, fil-I/O, stack-samples og trace-eksport; profilerflata er stabil.
- `norscode-jit-v1` kompilerer verifiserte heiltalls-, bool-, desimal- og tekstuttrykk med W^X, generasjonshandtak, optimaliseringspass og eksplisitt frigjering; pure VM lower kompatible NCB-funksjonar ved varme callsite-kall og fell tilbake ved feil. VM-en krev den auditpliktige `jit.execute`-capability-en før host-dispatch. Full JIT-flate er verifisert i runtime- og plattformgaten.
- `std.runtime_debugger` er kopla direkte inn før kvar pure-VM-instruksjon. Runtime-en eig breakpoint add/remove/clear, watch-register med verdi/undefined-snapshot, funksjon, kjeldelinje/-kolonne, IP, opcode, stack-/framedjupn og lokale variablar. Han kan reelt suspendere og gjenoppta nested continuation-frames med continue, instruction-step eller line-step. Selfhost-kompilatoren skriv `source_lines` og `source_columns` per NCB-funksjon, og både serialiserings- og runtime-test verifiserer kartet.
- `selfhost.runtime_debug_transport` held den suspenderte VM-en levande og eksponerer `norscode-debug-v1` gjennom tokenautentisert kommandjournal med stigande sekvens/replay-vern, timeout, latest-event og append-only eventlogg. `selfhost.runtime_debug_client` sender breakpoint-, watch-, snapshot-, continue-, instruction-step-, line-step- og stop-kommandoar utan at klienten byggjer protokolllinjer sjølv. Integrasjonstesten verifiserer ekstern resume, line-step, feil token, replay og timeout; debuggerflata er stabil.
- Kvar VM-frame eig kode, IP, label-kart, try-stack, siste exception, lokale variablar, verdi-/heap-stack, livssyklus, starttid, caller-IP og returmetadata. CALL brukar iterativ tail/continuation-trampoline utan vertsrekursjon, og same frame-state blir bevart gjennom debugger-pause/resume.
- Tail-CALL rett før `RETURN` brukar same VM-løkke og erstattar frame utan vertsrekursjon. 5 000 rekursive kall held låg framedjupn; uendeleg tail-rekursjon vert stoppa av `NORSCODE_VM_MAX_TAIL_CALLS` (standard 10 000). Vanlege ikkje-tail CALL brukar continuation-rammer i same VM-løkke, og aktive try-handlerar kan unwind-e kryssramme utan vertsrekursjon.
- Vanlege ikkje-tail CALL utan lokal try-handler brukar no continuation-frames i same VM-løkke. Fibonacci/samansett retur og 750 nivå ikkje-tail-rekursjon er testa utan host-stack. `THROW` kan unwind-e continuation-frames til nærmaste handler og gjenopprette verdi-stack/IP. Kallet som etablerer ein aktiv try-handler brukar framleis ei vertsramme som exception-grense for guard- og host-feil.
- Alle statiske brukarfunksjons-CALL, også frå frames med aktive try-handlarar, brukar tail/continuation-trampoline. Bytekode-`THROW`, VM guard-feil og builtin-feil unwind-ar eksplisitt til nærmaste catch-frame med stack/IP-gjenoppretting. Dynamisk `ncb_call_fn` brukar same continuation-system, og call-stack-flata er stabil etter nested suspend/resume-test.
- Bytekodekall til `ncb_call_fn` vert no normaliserte til same continuation/tail-frame før builtin-dispatch og reentrer ikkje VM-en. 600 dynamiske ikkje-tail-rammer er testa over den gamle 500-grensa; scheduler-callbacks, JSON/try/live-map og nested debugger suspend/resume-regresjonar er grøne.
- GC-terskelen er no adaptiv etter kvar samling (`max(minimum, alive*2+16)`) i staden for å bli liggjande under talet på levande objekt. Dette hindrar full mark/sweep ved kvart safe-point på djupe continuation-stacks. Minimum kan framleis setjast eksplisitt.

- `norscode-atomic-v1` har no ein ekte native C11-backend med avgrensa handle-register, 64-bits load/store/exchange/CAS/fetch-add, fences, destroy, versjonsteller og operasjonsspesifikk memory-order-validering. `std.runtime.atomic` vel native backend automatisk og held VM-fallbacken for runtime utan backend; allocator-låsen er verifisert oppå native backend i normal og pure VM på macOS ARM64- og Linux ARM64-kandidaten. `native_codegen_v2` har i tillegg freestanding x86-64-rutinar for raw-pointer load/store/exchange/CAS/fetch-add/fence; bytepresis normal/pure-test, bygd ELF-disassembly og ARM64/Windows-plattformattestasjonar inngår i releasegaten.

- `norscode-native-thread-v1` har no ein pthread-backend i den lokale native kandidaten med minst 1 MiB stack for interpreter-workers, opaque handle, condition-basert timeout-join og kooperativ cancel. `std.tråd.start_native`, `join_native` og `kanseller_native` held managerstatus og aktiv-teljar synkronisert. Normal og pure VM-test startar to samtidige OS-trådar som aukar same native atomic-celle til eksakt 10 000, og verifiserer deretter timeout, cancel og endeleg join. Handle-registeret blir frigjort etter join og er verifisert med 200 sekvensielle trådar.

- Native pthread-synkronisering dekkjer mutex og condition create/lock/unlock/wait/signal/broadcast/timeout/destroy gjennom `thread_sync`. Ende-til-ende-testen held mutex i hovudtråden, startar ein signal-worker, verifiserer at condition-wait frigjer mutexen atomisk og reacquirer henne etter signal, og testar deretter timeout og ressursopprydding. `std.tråd` eksponerer typed wrappers for heile flata.

- Generelle Norscode-funksjonar kan no brukast som native worker-callbacks. Host-runtime sender aktiv NCB-funksjonstabell som intern, ikkje-serialisert pure-VM-kontekst; kvar pthread får eigen interpreter-stack og thread-local unwind/feilbuffer. Normal og pure test køyrer to generelle callbacks parallelt mot same atomic-celle og verifiserer eksakt 5 000 oppdateringar, medan ein kastande callback returnerer strukturert worker-feil utan å skade hovud-VM-en.

- Runtime-memory brukar no atomisk eiertoken per native thread-id og held allokering, refs, roeter, payload-skriving, mark/sweep og metadataoppdatering etter fysisk compaction i same reentrante heaptransaksjon. Native atomic-handle er immutable i Norscode-wrapperen, slik at resultatspegling ikkje muterer delte kart. Native `NcMap` vernar oppslag, innsetting, nøkkelsjekk og fjerning med per-map mutex. To mutatorar og ein collector er verifiserte samtidig mot same NCG1-heap. Kvar native `NcVal` startar med den eksakte 32-byte `norscode-memory-abi-v1`-headeren; ABI-layout, fysisk relokering, generasjonspromotering, minor/major-atferd, edge-syklus og plattformruntime er verifiserte i releasegaten.

- `norscode-process-spawn-v1` har no ein shell-fri POSIX-backend i den lokale native kandidaten. `fork`/`execv` bevarer argv-grenser bokstavleg, handterer cwd og stdin, drenerer stdout/stderr separat med ikkje-blokkerande poll, reap-ar barnet og returnerer timeout, outputgrense, signal eller exitkode med PID. Direkte ABI-test og `std.prosess.start_argv_trygt` køyrer gjennom same backend; pure VM krev framleis `process.exec`.
- `std.runtime_filesystem` gir både kompatible bufferhandtak og `norscode-native-filesystem-v1`: rotfesta POSIX-descriptorar med `openat`/`O_NOFOLLOW` og Windows HANDLE-ar med UTF-16, traversalvern, reparse-avvisning og sluttsti-kontroll. Chunk-I/O, seek, stat, flush, close og delete er handlebasert. Windows overlapped ReadFile/WriteFile går gjennom IOCP og scheduler-futures med timeout, cancellation, filtrert fullføring og vern mot close medan operasjonar står uteståande. Direkte ABI- og scheduler-testar er køyrde under Windows/Wine.
- Pure VM eig no eit stabilt type-register med primitive, heap-, container- og custom typar. `std.runtime_type` brukar registeret for verdi-klassifisering og kan registrere/hente typar. Descriptorane dekkjer invariant generics, unionar, nullable, arv og metodeoppslag. Funksjonssignatur, async-flagg, instruksjonstal og modulfunksjonar kjem direkte frå aktiv NCB-tabell. Host-objekt blir utleverte som VM-eigde, tilbakekallbare bearer-handtak; faktisk dispatch validerer token, type, metodeallowlist, descriptor-capability, runtime-policy og plugin-capability før builtin-kallet. Forfalsking og bruk etter tilbakekall blir avvist. Type-runtime-flata er stabil.
- Flyttande arenakomprimering er no implementert: aktive blokker blir pakka mot starten, pensjonerte friblokker kan ikkje overlappe nye allokeringar, relokasjonstabellen oppdaterer alle levande objektadresser, og vidare allokering held fram frå ny arenaende. Fysisk `memmove` i ELF-heapen er verifisert saman med ABI- og plattformtesten.
- Dynamisk NCB-loading og hot-reload er verifisert i same pure-VM-prosess frå plugin v1 til v2. ABI-format, modul-ID, SHA-256 trust store, navnerom, interne CALL-mål, kollisjonar, generasjon og dependency-rekkefølgje blir kontrollerte. Plugin-capabilities er isolerte frå appen og må tildelast eksplisitt.
- Dynamisk loader er no stabil for runtime v1: rein Norscode SHA-256 er verifisert mot standardvektorar og pinnar NCB-innhald i trust store; manipulert bytekode blir avvist. Dependencies må vere aktive før lasting, og hot-reload, ABI, navnerom, kollisjonar, generasjonar og plugin-capabilities er verifiserte i pure-VM.
- 10/10-modenheit blir styrt av [docs/MODENHET_10_10.md](MODENHET_10_10.md)
- 10/10-bevisflate kan sjekkast med `./bin/nc maintenance maturity`
- `docs/SELFHOST_HANDLINGSPLAN.md` er aktiv plan for normalflata
- `./bin/nc maintenance status|lane|seed|seed-status|verify|report|report-json` er statusflate i Norscode
- Testløparen `tools/nc_test.no` brukar no mtime/byte-storleik-signatur for å gjenbruke gyldig NCB-bytecode; `NC_TEST_CACHE=0` slår av cachen, medan `NC_TEST_CACHE_DIR` kan isolere cache per jobb.
- `stage0_seed_ok` er hovudindikatoren for stage-0 seed i `maintenance`-rapportane
- historiske filer skal liggje i `docs/_archive/` eller `archive/`

## Gjeldande sjølvstendighetsstatus

- `./bin/nc local-green --strict` er lokal bevisport for release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate.
- `./bin/nc test` skal rapportere faktiske testtal, ikkje 0/0 når testfiler finst.
- `./bin/nc release-preflight` skal vere grøn før tag/release og publiserer ingenting.
- `./bin/nc release-preflight --strict` skal vere grøn før push/tag når nye nøkkelfiler skal med til GitHub.
- `./bin/nc local-green` skal vere samla lokal port når release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate må bevisast saman.
- `./bin/nc local-green --strict` skal vere samla streng port før push/tag.
- `./bin/nc selvstendighet` er normal gate for L1-L6/selfhost-status.
- `./bin/nc active-surface` vernar aktiv C/Python-fri flate.
- `./bin/nc surface-ownership` vernar ikkje-Norscode-filer med `.no`-eigarar og krev `Norscode-first`-markør/bridge eller eksplisitt unntak for aktive `.sh`- og `.ps1`-bruer.
- Aktiv plattformkode utanfor Norscode ligg under `platform/` og skal vere dokumentert der.

## Merknad

Gamle status-tal og gamle fasar vart skrivne for ein eldre struktur. Dei er no tona ned for å unngå å påstå meir enn det dokumentasjonen faktisk viser.

Historiske release-notat under `.github/releases/` kan framleis vise tal som `27/35` og `30-40%`, men dei er arkivtekst frå publiseringstidspunktet og skal ikkje lesast som dagens status.

## Status for FastAPI-paritet i scaffold (fase 1)

- **Dato:** 2026-06-18
- **Omfang:** `nc startproject` og `nc startapp`

### Fullført

- Leggja til responsskjema-validering (`response_shape_validate_or_error`) i scaffolds for testbare API-rykter.
- Lagt inn ny prosjekt-endepunkt:
  - `GET /api/v1/response-model`
- Lagt inn tilsvarande app-endepunkt:
  - `GET /api/v1/${APP_NAME}/response-model`
- Ruteopplisting oppdatert i begge malane (alle `startproject`-variantar med/utan auth/admin).
- Leggje til testdekning for response-model i begge malane:
  - positivt svar (`200`)
  - negativt svar (`500`, `ResponseValidationError`)
- Dokumentasjon i malar:
  - `startproject` README-lister oppdaterte API-ruter
  - `startapp` README-lister oppdaterte API-ruter
- OpenAPI-sjekk i testane oppdatert til å forvente `response-model` i spec.
- Nytt dømesett i payload-filer:
  - prosjekt: `tests/payloads/api_payload.json`, `tests/payloads/api_nested.json`
  - app: `apps/<app>/tests/payloads/${APP_NAME}_payload.json`, `apps/<app>/tests/payloads/${APP_NAME}_nested.json`
- Dependency-injeksjon i app-skal:
  - Lagt til `GET /api/v1/${APP_NAME}/dependency` i `startapp` med `app_meta`-dependency.
  - Oppdatert app-ruteopplisting, testdekning og app-README.
- Feilhåndtering i app-skal:
  - Lagt til `GET /api/v1/${APP_NAME}/error` i `startapp`.
  - Demonstrerer standardisert `400` (`response_error`) og `500` (`response_error`), pluss suksess-tilfelle.
  - Oppdatert ruteopplisting, testdekning og app-README.
  - Lagt til feilmønster for app-rot med:
    - 404-test for ukjende rute (`GET /api/v1/${APP_NAME}/ikkje-finst`)
    - 405-test for metodemismatch (`POST /api/v1/${APP_NAME}/query`)
- Standardisert request/response-kontrakt i begge skal:
  - Lagt til `POST /api/v1/${APP_NAME}/request-model` i `startapp` med request-validering + response-shape-validering.
  - Lagt til tilsvarande `POST /api/v1/request-model` i `startproject` med tilsvarande valideringsflyt.
  - Oppdatert både app- og stack-ruteopplisting, testdekning (suksess + valideringsfeil) og README-rutelister.
- Auth-mønster i app-skal:
  - Lagt til genererte auth-endepunkt i `startapp` (login/register/logout/profile):
    - `GET /api/v1/${APP_NAME}/auth/login`
    - `POST /api/v1/${APP_NAME}/auth/login`
    - `GET /api/v1/${APP_NAME}/auth/register`
    - `POST /api/v1/${APP_NAME}/auth/register`
    - `POST /api/v1/${APP_NAME}/auth/logout`
    - `GET /api/v1/${APP_NAME}/profile`
  - Oppdatert app-ruteopplisting og testdekning for autentiseringsflyt (suksess, ugyldig pålogging, utlogging, profil-metadata, token-mangel).

### Under arbeid / neste steg

- Neste steg i FastAPI-paritet:
  - Oppdatere dokumentasjonen med kvar gjenværande manglande del i FastAPI-liknande standardflow (dependency, middleware, autentiseringsflow med header/session, openapi-schema).
