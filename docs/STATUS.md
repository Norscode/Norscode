# Dokumentasjonsstatus

Dette viser status for dokumentasjonen som faktisk ligg i repoet.

## Sjølvstendigheitsstatus 2026-07-16

- Aktiv compile-, bundle-, test- og scaffold-flyt er eigd av Norscode `.no` og
  køyrer med `NODE_BIN=/nonexistent`. Node/JavaScript, Python og C er ikkje del
  av den aktive kjeda; eigarskaps- og C/Python-gatene er grøne.
- `dist/norscode_native` er byte-identisk med committed macOS ARM64-stage0.
  Committed Linux x86_64- og ARM64-seedar har same hash som dei køyrde
  Zig/Argon2-kandidatane. Begge Linux-arkitekturar har grøn runtime-gap-gate i
  Docker.
- Windows x86_64 runtime, IOCP/SChannel, AppContainer og Argon2 krysskompilerer
  og lenkar. Dette er ikkje ekte Windows-attestasjon:
  `production_ready_windows=false` og `production_ready_all_platforms=false`
  står ved lag fram til køyring på ein faktisk Windows-vert.
- `tools/windows_runtime_attestation.no` gir no éi fail-closed Windows-port som
  sjølv køyrer backend/filsystem/prosess, ekte AppContainer, nettverk,
  SChannel TLS 1.3, IOCP og Argon2id før ho skriv ein commit- og
  binærhashbunden rapport. Hovud-CI og Windows-release signerer rapporten med
  GitHub build provenance. `tools/platform_readiness_v3600.no` godtek berre
  rapporten når `gh attestation verify`, commit, rein spora kjelde og runtime-
  SHA-256 alle samsvarer; krysskompilering eller ein laus JSON-fil kan derfor
  ikkje gjere Windows grøn.
- `std.runtime_status` rapporterer 19 av 22 runtimeområde som stabile (86 %).
  `network_runtime`, `process_api` og `runtime_security` er
  framleis `delvis` på grunn av reelle plattform-/backendbevis som står att.
- `std.stdlib_status` rapporterer 57 av 61 modular som stabile (93 %).
  `sikkerheit`, `dns`, `tls_acme` og `domenehost` er framleis eksperimentelle.
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

- [docs/INDEX.md](INDEX.md)
- [docs/USER_MANUAL.md](USER_MANUAL.md)
- [docs/LEARNING_GUIDE.md](LEARNING_GUIDE.md)
- [docs/DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)
- [docs/LANE_MAP.md](LANE_MAP.md)
- [docs/_archive/ARCHIVE_INDEX.md](_archive/ARCHIVE_INDEX.md)

## Vedlikehald

## Runtime-gate 2026-07-13

## Helpdesk-deployment 2026-07-13

- Deployment-kontrakten er implementert i `std.deployment_config`, `std.transactional_storage`, `std.job_queue`, `std.quick3`, `std.inbound_email`, `std.vedlegg` og `std.observability`.
- `tests/test_helpdesk_deployment_contract.no` køyrer grønt med database-transaksjon, idempotent jobbkø, Quick3-mapping, MIME-korrelasjon, webhook-signatur og vedleggspolicy.
- Kontrakttesten dekker òg faktisk SQLite-backup/restore, Quick3 retry-plan, deterministisk HTTP-mock med 503/201-statuskø og samlet secrets-navnerom for `QUICK3_API_TOKEN`, `SMTP_PASSWORD` og `INBOUND_WEBHOOK_SECRET`.
- `tests/test_pbkdf2_vector.no` verifiserer standard PBKDF2-HMAC-SHA256 mot kjent testvektor. Passordløypa brukar 120 000 iterasjonar og har native CommonCrypto på macOS og OpenSSL i Linux-releasekandidaten, med Norscode-fallback. `std.scrypt` er rein Norscode med RFC 7914-vektor. Argon2id er verifisert i committed macOS ARM64- og Linux x86_64/ARM64-stage0; ekte Windows-køyring står framleis att før sikkerheit kan kallast produksjonsklar.
- Jobbkøen har no timeout-reclaim: krasja workers blir frigitt til retry, medan jobbar som har nådd maks forsøk går til dead-letter. Indeks-API-et avviser ugyldige SQL-identifikatorar før SQL blir bygd.
- SQLite-lagringa set no `busy_timeout` og WAL-journal ved opning, i tillegg til foreign keys, transaksjonar, migrering, backup og restore.
- MIME-parseren dekodar no `base64` og `quoted-printable`, bevarer rå kropp ved feilsøking, avviser råmeldingar over 10 MiB og har grønn integrasjonstest for HTML, tekst og base64-vedlegg.
- Quick3-klienten avviser HTTP-base-URL, validerer minimumsfelt i typemapping og tilbyr `trygg_responslogg` som berre eksponerer status, forsøk og OK-flagget utan token, body eller rå response.
- `./bin/nc bundle` gir no runtime-capabilities på same måte som `run`, og enkeltmodul- og modulær bundling er verifisert grønt: `admin=std/admin.no` (508 KB) og sju Helpdesk-moduler samla (716 KB). Hybrid-løypa viser no òg underliggjande kjelde-/runtime-diagnose ved compile-feil i staden for å skjule output.
- Native VM-dybdegrensa er 256 under vanleg køyring og compiler-løypa set automatisk 4096 (begge konfigurerbare til 16384). Feilen rapporterer fil, funksjon, linje, kolonne, uttrykk, faktisk djup og aktiv grense. Dette fjernar den tidlegare `For djup rekursjon`-blokkeringa for store program under kompilering utan å la uendeleg runtime-rekursjon presse C-stakken; Helpdesk `app.no` er verifisert med promotert macOS ARM64-runtime og korrekt `NORSCODE_PROJECT_ROOT`.
- Helpdesk har no ein verifisert modulflate i `prosjekter/NorscodeHelpdeskAI/src/`: `storage.no`, `tickets.no`, `vehicles.no`, `email.no`, `quick3.no`, `auth.no` og `admin.no`. Modulkontrakten køyrer reelle ticket-, kjøretøy-, e-post-, Quick3-, PBKDF2- og audit-operasjonar med deduplisering; `storage.no` bruker transaksjonell SQLite v2 med WAL og indeks, og separat v9500-bundle byggjer alle sju modulane. Den store eksisterande `app.no` blir framleis bygd som eiga monolittisk produksjonsflate medan handlerflytting skjer trinnvis.
- `bin/nc run` finn no prosjektroten frå kildefila, slik at prosjekt med eigne `norcode.toml` og `src.*`-modular køyrer med riktig importbase og datamappe.
- Modenheitsgaten for standardbiblioteket er løfta til 57/61 stabile etter grøne roundtrip-, inferens-, tokeniserings-, native socketserver-, modellregister-, native multiprocessing- og lokal q8-media-tester. Fire modulstatusar står framleis eksperimentelle fordi dei krev reell plattform-/leverandørverifisering eller større end-to-end-gater.
- DNS-flata har no også deterministisk wire-query og autoritativt svar for A, CNAME, MX og TXT, native UDP daemon lifecycle med bind/mottak/svar/stop, korrekt JSON-liste for sone-records, verifiserbar SHA-256 DS-digest/validering, native RRSIG-signering og native RRSIG-validering for RSASHA256/ES256/Ed25519; release-/providergate og ekstern multi-plattform-verifisering står framleis att.
- Kryptografi-fallbacken brukar no den reine Norscode SHA-256/HMAC-implementasjonen i staden for ein svak concatenation-hash. `std.scrypt` implementerer RFC 7914 scrypt med testvektor og harde pure-VM-parametergrenser; Argon2id er verifisert mot RFC 9106 med OpenSSL-native backend i lokal macOS `dist`.
- `tests/test_scrypt.no` verifiserer scrypt mot RFC 7914-vektoren for N=16, r=1, p=1, med feil-passord og parametergrenser; implementasjonen ligg i `std/scrypt.no` og bruker berre Norscode SHA-256/HMAC i pure VM.
- `std/argon2id.no` og `tests/test_argon2_native.no` verifiserer RFC 9106 Argon2id med grenser for minne, iterasjonar, parallellitet og lengder. Lokal macOS `dist/norscode_native` og committed macOS/Linux-stage0 har grøn backend-verifisering; Windows x86_64 ABI-kandidaten krysskompilerer, men ekte Windows-køyring står framleis att.
- `std.tls_acme` har no full RFC 8555-flyt med HTTPS directory og Replay-Nonce, ny konto med `jwk`, vidare kall med `kid`, ordre, autorisasjon, challenge, CSR-finalisering og sertifikatnedlasting. Linux CI brukar ein fastlåst Pebble-digest, vel `dns-01`, publiserer korrekt JWK-bunde TXT-svar og lar Pebble validere mot Norscode sin eigen autoritative DNS-daemon over RFC-frama DNS/TCP. `PEBBLE_VA_ALWAYS_VALID` og den eksterne challenge-serveren er fjerna. Windows-attestasjonen køyrer i tillegg native ACME-signering/verifisering; signert resultat frå ekte Windows-runner står att før modulen kan løftast frå eksperimentell til stabil.
- `std.sikkerheit` samlar no passordkrypto og capability-policy gjennom ein eksplisitt Norscode-native fasade; algoritmegrenser, pure-VM fallback, committed macOS/Linux-stage0 og deny-by-default er testa, medan ekte Windows og ekstern sikkerheitsattestasjon framleis står att.
- `std.dns.resolve` bruker no native `dns_lookup` med `net.dns`-capability, numeriske IPv4/IPv6-adresser og eksplisitt feilstatus. Testløparen gir `net.dns` til nettverkstestar utan å opne andre capabilities.
- Aktiv macOS ARM64-runtime har native PBKDF2-HMAC-SHA256 via CommonCrypto, Argon2id via OpenSSL og rein Norscode scrypt, med fallback der backend manglar. Linux-stage0 har OpenSSL PBKDF2/ACME og Zig Argon2id. Kjent PBKDF2-, scrypt- og Argon2id-vektorar og `tests/test_security.no` er grøne; ekte Windows står framleis att.

- Lokal macOS ARM64-runtime er promotert til `dist/norscode_native` med OpenSSL 3.6.2 for Argon2id og ACME/DNSSEC-signering. Committed macOS ARM64- og Linux x86_64/ARM64-stage0 er oppdaterte og hashfesta i `bootstrap/stage0/SHA256SUMS`.
- Native runtime-gap, JIT for heltall/desimal/tekst, native tråd- og tensorflate, `builtin.process_spawn_argv` og `tests/test_runtime_v1_contract.no` er verifisert grønne etter promoteringen.
- Adaptive JIT har no ein eigen Norscode-optimaliseringspass med iterert konstantfolding, identitetsreduksjon, SSA-livstidsanalyse og register-/spillplan; dette er verifisert i `tests/test_runtime_jit_optimizer.no`.
- Windows x86_64 native ABI blir no reproducerbart kompilert og lenka gjennom `tools/windows_runtime_cross_compile_gate.no`, inkludert hovudruntime, IOCP/SChannel-backend, Zig Argon2id-objekt, fil/prosess/IOCP-backend-smoke, dynamisk `winsqlite3.dll`/`sqlite3.dll`-laster og AppContainer-smoke-`.exe`; ekte Windows TLS/AppContainer-handshake er framleis ekstern plattformverifisering.
- Windows ABI-gaten køyrer no også i hovud-CI på kvar push og pull request, med eksplisitt `process.exec`/disk-scope, korrekt `.exe`-sti for native runtime-testen og opplasta diagnoseartefakt; ekte Windows execution-gate er framleis separat fordi han må køyre på faktisk Windows.
- `tools/windows_runtime_execution_gate.no` køyrer backend- og AppContainer-smoke på faktisk Windows-host; på macOS/Linux rapporterer han eksplisitt ekstern verifisering i staden for å gi falsk grønt.
- Windows-release-workflowen byggjer no backend-smoke med Zig og køyrer execution-gate, SChannel, native network, IOCP og filesystem-testar før app-pakking.
- Linux x86_64-kandidaten `build/v3600/linux/norscode_native_linux_x86_64_v3602` er krysskompilert med Zig og køyrd gjennom full `native_runtime_gap_gate_v3001` i Ubuntu 24.04 Docker. Den portable Argon2id-kandidaten `build/v3600/linux/norscode_native_linux_x86_64_v3605_zigargon` inkluderer også OpenSSL for PBKDF2/ACME, passerer målretta Linux-sikkerheitstestar og er kopla inn i `.github/workflows/publish.yml` gjennom `tools/linux_zig_argon_release_gate_v3607.no`; stage0-seed og ekte Windows-køyring står framleis separat.
- Linux ARM64-kandidaten `build/v3600/linux/norscode_native_linux_aarch64_v3608_zigargon` er køyrd native i ARM64 Ubuntu med portable Zig Argon2id og OpenSSL. Han passerer prosess-sandbox, multiprocessing, ACME, PBKDF2, security og full testflate: 560/560 bestått, 0 feila og 20 eksplisitt plattformfiltrerte.
- `tools/nc_test.no` vel no stat-/hashsignatur etter faktisk operativsystem, slik at Linux-testar ikkje prøver macOS `stat -f` eller `shasum` før fallback. CLI-eksempelet bruker `TMPDIR` i staden for hardkoda macOS-sti.
- Linux OpenSSL-kandidaten `build/v3600/linux/norscode_native_linux_x86_64_v3603_openssl` blir no reproducerbart bygd av `tools/build_linux_openssl_candidate_v3604.no`; ACME-signering/verifisering og runtime-gap er grøne. Ubuntu OpenSSL 3.0 manglar Argon2 KDF-parametrar, så Argon2id feilar lukka på OpenSSL-banen. `archive/legacy_c_backend/nc_argon2_zig.zig` gir i tillegg ein portable RFC 9106-backend; `tools/build_linux_zig_argon_candidate_v3606.no` byggjer og verifiserer Linux-kandidat med `tests/test_argon2_native.no` grønt.
- Runtime v1-matrisa er no 19 stabile og 3 delvise av 22; full normal testflate er verifisert 16. juli 2026 med 563/563 bestått, 0 feila og 21 eksplisitt plattform-/lanefiltrerte hopp (584 totalt). Slow-lanen er verifisert separat og gjennom topp-porten med 11/11 bestått, 0 feila og 573 filtrerte hopp. Testløparen bruker signaturbasert NCB-cache i eiga cache-mappe (`NC_TEST_CACHE_DIR`), atomisk artefaktflytting og unik arbeidsmappe per køyring (`NC_TEST_RUN_ID`) for å unngå delte eller utdaterte `.ncb.json`-artefaktar. Cache kan slåast av med `NC_TEST_CACHE=0`.
- Media runtime-gaten køyrer no eksplisitt i både macOS- og Linux-hovud-CI med aktiv native runtime, inkludert CPU-diffusjon og binær medie-I/O.
- macOS-CI har ein faktisk Metal compute-gate som kompilerer og køyrer GPU-kernel med buffer-I/O, dispatch, synkronisering og resultatkontroll; `std.tensor.matmul`, `media_neural` og `std.media_diffusion.bilde_med_backend` vel Metal når runtime rapporterer GPU, med SIMD/CPU-fallback. Gaten er no køyrd grøn på Apple M3, inkludert tensor-matmul og diffusjonskernel.
- Aktiv v3010 media-gate er grøn for bytes/binær I/O, lokal mediegenerering, deterministisk CPU-diffusjon, bounded modellserver, modellruntime/register/tillit, tokenizer, transformer, tekstgenerering og HTTP/Base64-medieflyt. `media_neural` eksponerer faktisk tensor-backend per inferens og brukar Metal-matmul når eininga finst, elles den obligatoriske og verifiserte CPU-fallbacken. Metal compute, tensor-ABI og diffusjonsadapter er kompilert og køyrde på Apple M3; media-kontrakten er derfor stabil utan å gjere GPU til eit krav på plattformer som ikkje har Metal.
- GC-promotering er derfor fjernet som aktiv blokkering i `std.runtime_status`; resterende delvise områder er fortsatt eksplisitt merket der de ikke er fullstendig verifisert.

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
- Runtime v1-manglane er samla i `std.runtime_status` og testa med `tests/test_runtime_v1_contract.no`. Bytecode VM, minnehåndtering, heap, call-stack, type-runtime, exception-runtime, runtime-bibliotek, dynamic loader, refleksjon, debugger og profiler er no stabile; dei andre runtime-v1-radene har minst ei delvis Norscode-standardflate. Dette betyr ikkje at aktiv runtime er 100% ferdig. `norscode-runtime-library-abi-v1` verifiserer symbol og atferd for tekst, liste, kart, JSON, tid, fil og sikkerheit direkte i pure VM. Eksplisitte `builtin.*`-kall har prioritet over fuzzy funksjonsoppslag og kan ikkje skyggast av brukar-/modulfunksjonar. VM-en dekkjer heile compiler-opcodeflata og verifiserer opcode, operandantall, labels og ikkje-negative bygg/kall før køyring. `norscode-memory-abi-v1` bind runtime-allocator og native codegen til same 64 MiB RW-segment med eit 64-byte kontrollområde og er verifisert mot ein faktisk ELF-programheader. Fysiske 32-byte `NCG1`-objektheaderar lagrar flags, generation, age, type, payloadstørrelse, `first_edge` og `ref_count`. Kvar referanse er eit unbounded edge-objekt i same arena med `from/to/next`; add/remove, sweep og compaction held kjedene synkroniserte. Registeruavhengig root-walk markerer sykliske grafar, og native sweep frigjer utilgjengelege objekt og edge-blokker direkte i allocatoren. Heapmutasjonar og native mark/sweep eig same atomiske eiertoken/depth-lås; refs, røter, payload og compaction er serialiserte med collector utan delt kartmutasjon. Native `NcVal` ligg no i runtime-eigde arenasider; major-GC returnerer tomme sider og kan fysisk flytte levande objekt til tette sider. Relokering omskriv registrerte frame-røter, liste-/kartreferansar, edge-graf, host-kontekst og cache-epoke, medan native compiler-regionar med rå C-peikarar blir eksplisitt pinna. Safepunkt-entry køyrer collector automatisk ved terskel. `std.runtime_stack` brukar iterative tail/continuation-rammer med kryssramme-unwind og reell nested suspend/resume utan vertsrekursjon. Exception-runtime har typed catch og `endeleg`/`utsett` gjennom aktiv selfhost-kjede. `std.runtime_type` dekkjer generics, union/nullability, arv, metodeoppslag og capability-isolerte host-handtak. `std.inspect` les no VM-symboltabellen og arva type-/dispatchmetadata direkte. `std.runtime_filesystem` har no native rotfesta descriptor-handtak med `openat` gjennom kvart segment, `O_NOFOLLOW`, read/write/append/readwrite, seek, `fstat`, `fsync`, close og sikker `unlinkat`; traversal og slutt-/mellomliggande symlink-angrep er verifiserte i normal og pure VM. Descriptor-readiness er kopla til scheduler-futures med timeout, cancellation og feilpropagering; Windows HANDLE-backend står att. `norscode-native-network-v1` gir nonblocking POSIX listen/connect/accept/read/write/close, `poll(2)`-readiness og scheduler-futures for connect/accept/read/write, verifisert med loopback ping/pong i normal og pure VM. Valfri OpenSSL 3-backend gir ekte TLS 1.3 klient/server-handshake, trust store, SNI, hostname-verifisering, cipher/peer-metadata og kryptert scheduler-I/O; bygg utan backend feilar lukka. TLS-ping og feil-hostname er verifiserte i normal og pure VM; OCSP/CRL/mTLS og Windows SChannel står att. `std.prosess` skil shell- og argv-planar, bevarer argumentgrenser og krev eksplisitt shell-samtykke. `norscode-process-spawn-v1` definerer request/result med cwd, stdin, timeout, outputgrense, PID og exitstatus; VM-en handhevar `process.exec` før `process_spawn_argv`. Shell-fri native `fork`/`execv` med argv-grenser, cwd, stdin, separate ikkje-blokkerande stdout/stderr-piper, timeout, outputgrense, PID og exitstatus er verifisert i normal og pure VM. `norscode-native-process-v1` legg i tillegg til levande PID-handle, poll, inkrementell read, wait, TERM/INT/KILL/HUP, deadline, close og slot-gjenbruk. Sandboxprofil gir lukka miljø, CPU/core/fd-grenser, Linux `no_new_privs` og macOS Seatbelt no-network/no-write; scope, miljø og forbod er verifiserte i normal og pure VM. Windows CreateProcess/Job Object, Linux seccomp-filter og hard Darwin-minnegrense står att. `std.tråd` brukar direkte runtimeobjekt og har idempotent start, recursive mutex-depth, FIFO-overlevering, streng eierkontroll, kansellering, events/barrierar/delt minne og join med deadlock/timeout-status. `norscode-native-thread-v1` definerer spawn/join/cancel/mutex/condition, medan `norscode-atomic-v1` gir load/store/exchange/CAS/fetch-add og memory-order-validering; delte atomic-celler og full livssyklus er grøne i normal og pure VM. Pthread-backend og C11-maskinvareatomics/fences er implementerte; Windows-thread-backend, køyrt Linux-ELF og freestanding ARM64/Windows-atomics står att. Sikkerheit blir handheva i pure-VM, og native filhandtak er symlink-sikre; full OS-sandbox står att. `await` kan no suspendere ei nested VM-ramme med heile continuation-kjeda og gjenoppta henne når `std.sched` fullfører, avviser eller kansellerer den typebevarande future-en. Scheduler-køa er kooperativ og har native OS-pollere/trådpool for macOS, Linux og Windows; eksplisitte runtime-trådar kan køyre som pthread-workers. GUI-runtime har stabil HTML/headless-backend; OS-vindu er adapterarbeid utanfor runtime-kontrakten.
- Pure-VM-sikkerheit er no handheva før builtin-dispatch: fil, mappe, lagring, database, miljø, prosess og socket/nettverk krev eksplisitte capabilities. Standard er deny, native filhandtak avviser traversal og symlink-folging, read/write-builtins kan ikkje kryssbrukast, og auditmåling viser tillatne og avviste kall. Nye listen/connect/TLS-handtak må samsvare med network scope, og executable må samsvare med process scope. Prosess-ABI-et avviser no ukjende sandboxprofilar, ugyldige miljøflagg og minne-/fd-grenser før dispatch. Prosessar kan bruke lukka miljø, rlimits, Linux seccomp no-network/no-write/pure og macOS Seatbelt; Windows AppContainer/Job Object og hard Darwin-minnegrense står att.
- `NORSCODE_VM_DISK_ROOT` er ei komma-separert katalog-allowlist; `disk.read` eller `disk.write` gir ikkje tilgang utanfor dette scopet. Tom verdi blir avgrensa til relative stiar under gjeldande prosjektrot.
- Runtime-allocator er ikkje lenger ein ugyldig placeholder: `std.runtime.allocator` køyrer med 64 MB avgrensa bytearena, 8-byte alignment, bounds-kontrollert byte-I/O, nullstilling ved allokering/gjenbruk, first-fit-gjenbruk, double-free/OOM-vern og statistikk. Flyttande komprimering tek overlappssikre snapshots og bevarer faktisk blokkinnhald ved relokasjon. `std.runtime_memory` tildeler header/payload-adresse per objekt, eksponerer bounds-kontrollert payload-I/O, bevarer payload gjennom GC-komprimering og returnerer sweep-blokker til allocatoren. Native ELF-backend har allereie eit fysisk 64 MB RW-segment; direkte ABI-binding mellom dette segmentet og GC-bytearenaen står att.
- `std.sched` brukar ei runtime-uavhengig, flat hendingskø med tid, prioritet, deterministisk ID-rekkefølgje, cancel, forfall, stopp/start og avgrensa run-loop. Native readiness brukar ein vedvarande trådlokal kqueue på macOS, EPOLLONESHOT på Linux og aktiv IOCP på Windows. IOCP-banen dekkjer AcceptEx, ConnectEx, WSARecv, WSASend og overlapped ReadFile/WriteFile med generasjonsbundne, filtrerte completion-handtak. Timeout, kansellering, drenerande cleanup og VM-`await` er kopla til same future-modell og er køyrde under Windows/Wine. Preemptiv scheduler står att; eksplisitt parallellitet går gjennom den native arbeidspoolen.
- Scheduler-futures eig no status, resultat, feil og kansellering i same event-loop-tilstand og er grøne i normal og pure VM. Den pure reentry-låsen vart spora til nyleg JSON-parsa argument ved callback-grensa; køa held no det opphavlege, rotfesta argumentkartet utan unødvendig serialisering. Pure VM nullstiller dessutan ventande argument-heap-ID-ar ved dynamiske kall, har konfigurerbar GC-terskel og opt-in kallsporing.
- `std.prosess` har levande POSIX-prosesshandtak med shell-fri `fork`/`execv`, PID, ikkje-blokkerande stdin/stdout/stderr, poll, timeout/deadline, TERM/INT/KILL/HUP, exit-kode, inkrementell streaming og slot-gjenbruk. Prosesshandtak kan fullføre scheduler-futures som vekkjer suspendert VM-`await`. Sandboxprofil gir lukka miljø, rlimits, Linux seccomp no-network/no-write/pure og macOS Seatbelt no-network/no-write. Windows CreateProcess/Job Object og hard Darwin-minnegrense står att.
- Pure VM-profilaren måler CPU-instruksjonar/rate, veggtid, heap/allokering, GC-samlingar/safe-points/pause, builtin-kall og fil/nett/prosess-I/O. Konfigurerbar statistisk sampling fangar funksjon, IP, opcode, kjeldelinje/-kolonne og full call-stack med avgrensa buffer og dropteljar. Per-funksjon histogram dekkjer kall, instruksjonar og veggtid, og funksjonsintervall blir eksporterte som standard Chrome Trace Event JSON. Integrasjonstestane utløyser faktisk heap, GC, fil-I/O, stack-samples og trace-eksport; profilerflata er stabil.
- `norscode-jit-v1` kompilerer verifiserte heiltalls- og booluttrykk med samanlikning, negasjon, labels og betinga/ubetinga kontrollflyt til ekte ARM64/x86-64-maskinkode. Runtime-registeret brukar generasjonshandtak, typed retur, avgrensa kode/register, RW-til-RX W^X-overgang og eksplisitt `munmap`; pure VM lower automatisk kompatible NCB-funksjonar ved varme callsite-kall, konstantfoldar, innliner reine ikkje-rekursive kall og fell tilbake ved feil. VM-en krev den auditpliktige `jit.execute`-capability-en før host-dispatch. Desimaltal, tekst/objekt, fleirfunksjons-/rekursiv inlining og avanserte optimaliseringar står att.
- `std.runtime_debugger` er kopla direkte inn før kvar pure-VM-instruksjon. Runtime-en eig breakpoint add/remove/clear, watch-register med verdi/undefined-snapshot, funksjon, kjeldelinje/-kolonne, IP, opcode, stack-/framedjupn og lokale variablar. Han kan reelt suspendere og gjenoppta nested continuation-frames med continue, instruction-step eller line-step. Selfhost-kompilatoren skriv `source_lines` og `source_columns` per NCB-funksjon, og både serialiserings- og runtime-test verifiserer kartet.
- `selfhost.runtime_debug_transport` held den suspenderte VM-en levande og eksponerer `norscode-debug-v1` gjennom tokenautentisert kommandjournal med stigande sekvens/replay-vern, timeout, latest-event og append-only eventlogg. `selfhost.runtime_debug_client` sender breakpoint-, watch-, snapshot-, continue-, instruction-step-, line-step- og stop-kommandoar utan at klienten byggjer protokolllinjer sjølv. Integrasjonstesten verifiserer ekstern resume, line-step, feil token, replay og timeout; debuggerflata er stabil.
- Kvar VM-frame eig kode, IP, label-kart, try-stack, siste exception, lokale variablar, verdi-/heap-stack, livssyklus, starttid, caller-IP og returmetadata. CALL brukar iterativ tail/continuation-trampoline utan vertsrekursjon, og same frame-state blir bevart gjennom debugger-pause/resume.
- Tail-CALL rett før `RETURN` brukar same VM-løkke og erstattar frame utan vertsrekursjon. 5 000 rekursive kall held låg framedjupn; uendeleg tail-rekursjon vert stoppa av `NORSCODE_VM_MAX_TAIL_CALLS` (standard 10 000). Vanlege ikkje-tail CALL brukar continuation-rammer i same VM-løkke, og aktive try-handlerar kan unwind-e kryssramme utan vertsrekursjon.
- Vanlege ikkje-tail CALL utan lokal try-handler brukar no continuation-frames i same VM-løkke. Fibonacci/samansett retur og 750 nivå ikkje-tail-rekursjon er testa utan host-stack. `THROW` kan unwind-e continuation-frames til nærmaste handler og gjenopprette verdi-stack/IP. Kallet som etablerer ein aktiv try-handler brukar framleis ei vertsramme som exception-grense for guard- og host-feil.
- Alle statiske brukarfunksjons-CALL, også frå frames med aktive try-handlarar, brukar tail/continuation-trampoline. Bytekode-`THROW`, VM guard-feil og builtin-feil unwind-ar eksplisitt til nærmaste catch-frame med stack/IP-gjenoppretting. Dynamisk `ncb_call_fn` brukar same continuation-system, og call-stack-flata er stabil etter nested suspend/resume-test.
- Bytekodekall til `ncb_call_fn` vert no normaliserte til same continuation/tail-frame før builtin-dispatch og reentrer ikkje VM-en. 600 dynamiske ikkje-tail-rammer er testa over den gamle 500-grensa; scheduler-callbacks, JSON/try/live-map og nested debugger suspend/resume-regresjonar er grøne.
- GC-terskelen er no adaptiv etter kvar samling (`max(minimum, alive*2+16)`) i staden for å bli liggjande under talet på levande objekt. Dette hindrar full mark/sweep ved kvart safe-point på djupe continuation-stacks. Minimum kan framleis setjast eksplisitt.

- `norscode-atomic-v1` har no ein ekte native C11-backend med avgrensa handle-register, 64-bits load/store/exchange/CAS/fetch-add, fences, destroy, versjonsteller og operasjonsspesifikk memory-order-validering. `std.runtime.atomic` vel native backend automatisk og held VM-fallbacken for runtime utan backend; allocator-låsen er verifisert oppå native backend i normal og pure VM på macOS ARM64- og Linux ARM64-kandidaten. `native_codegen_v2` har i tillegg freestanding x86-64-rutinar for raw-pointer load/store/exchange/CAS/fetch-add/fence; bytepresis normal/pure-test og bygd ELF-disassembly stadfestar `xchgq`, `lock cmpxchgq`, `lock xaddq` og `mfence`. Freestanding ELF-køyring på Linux og eigen freestanding ARM64/Windows-backend står framleis att.

- `norscode-native-thread-v1` har no ein pthread-backend i den lokale native kandidaten med minst 1 MiB stack for interpreter-workers, opaque handle, condition-basert timeout-join og kooperativ cancel. `std.tråd.start_native`, `join_native` og `kanseller_native` held managerstatus og aktiv-teljar synkronisert. Normal og pure VM-test startar to samtidige OS-trådar som aukar same native atomic-celle til eksakt 10 000, og verifiserer deretter timeout, cancel og endeleg join. Handle-registeret blir frigjort etter join og er verifisert med 200 sekvensielle trådar.

- Native pthread-synkronisering dekkjer mutex og condition create/lock/unlock/wait/signal/broadcast/timeout/destroy gjennom `thread_sync`. Ende-til-ende-testen held mutex i hovudtråden, startar ein signal-worker, verifiserer at condition-wait frigjer mutexen atomisk og reacquirer henne etter signal, og testar deretter timeout og ressursopprydding. `std.tråd` eksponerer typed wrappers for heile flata.

- Generelle Norscode-funksjonar kan no brukast som native worker-callbacks. Host-runtime sender aktiv NCB-funksjonstabell som intern, ikkje-serialisert pure-VM-kontekst; kvar pthread får eigen interpreter-stack og thread-local unwind/feilbuffer. Normal og pure test køyrer to generelle callbacks parallelt mot same atomic-celle og verifiserer eksakt 5 000 oppdateringar, medan ein kastande callback returnerer strukturert worker-feil utan å skade hovud-VM-en.

- Runtime-memory brukar no atomisk eiertoken per native thread-id og held allokering, refs, roeter, payload-skriving, mark/sweep og metadataoppdatering etter fysisk compaction i same reentrante heaptransaksjon. Native atomic-handle er immutable i Norscode-wrapperen, slik at resultatspegling ikkje muterer delte kart. Native `NcMap` vernar oppslag, innsetting, nøkkelsjekk og fjerning med per-map mutex. To mutatorar og ein collector er verifiserte samtidig mot same NCG1-heap. Kvar native `NcVal` startar med den eksakte 32-byte `norscode-memory-abi-v1`-headeren: `NCG1` magic, flags, generation, age, type, payload size, first edge og ref count. Objekta kjem frå runtime-eigde arenasider med 1024 alignede slots; frigjorde slots går til fri-liste og blir gjenbrukte. Major-GC returnerer tomme sider straks og flyttar levande objekt til nye tette sider når fragmentering kan redusere sidebruken vesentleg. Frame-roeter, liste-/kartverdiar, edge-objekt, globale host-roeter og trådkontekstar blir relokerte atomisk; cache-epoken avviser gamle trådlokale oppslag. Native compiler-regionar som framleis brukar rå C-peikarar er pinna under rekursive kall. Livssyklustesten verifiserer over 200 000 tildelingar, relokeringstesten beviser endra objektadresser og intakt graf, og heapen kan returnere til null sider. Før markering byggjer runtime fysiske 56-byte NCG1 edge-objekt med same 32-byte header og 24-byte `from/to/next`-payload for alle liste- og kartreferansar. `first_edge` og `ref_count` peikar på grafen, og markering følgjer edge-kjedene; ein to-objekts kartsyklus overlever med rot og blir fullstendig frigjord utan rot. Collector promoterer etter to overlevingar, køyrer konservativ minor-GC med gamle objekt som røter og periodisk major-GC som kan frigjera gamle grafar. Både genererte funksjonar og hosttolken registrerer operandstack/lokale variablar som røter; globale NCB-/funksjonskontekstar, host-labelkart, compiler-kjelde og mellomresultat er eksplisitt rotfesta. Stop-the-world-handshake ber andre aktive pthread-VM-ar parkere ved safepunkt, samlar først når alle er parkert, og avbryt/utset etter 100 ms dersom ein mutator sit i blokkerande OS-I/O. ABI-layout, fysisk relokering, generasjonspromotering, minor/major-atferd, edge-syklus, samtidige rotgrafar og blokkert-traad-deferral er grøne med AddressSanitizer/UBSan. macOS ARM64-runtime er promotert, og Linux ARM64-kandidaten er verifisert med same runtime-ABI; stage0-seed og Windows-backend står framleis att.

- `norscode-process-spawn-v1` har no ein shell-fri POSIX-backend i den lokale native kandidaten. `fork`/`execv` bevarer argv-grenser bokstavleg, handterer cwd og stdin, drenerer stdout/stderr separat med ikkje-blokkerande poll, reap-ar barnet og returnerer timeout, outputgrense, signal eller exitkode med PID. Direkte ABI-test og `std.prosess.start_argv_trygt` køyrer gjennom same backend; pure VM krev framleis `process.exec`.
- `std.runtime_filesystem` gir både kompatible bufferhandtak og `norscode-native-filesystem-v1`: rotfesta POSIX-descriptorar med `openat`/`O_NOFOLLOW` og Windows HANDLE-ar med UTF-16, traversalvern, reparse-avvisning og sluttsti-kontroll. Chunk-I/O, seek, stat, flush, close og delete er handlebasert. Windows overlapped ReadFile/WriteFile går gjennom IOCP og scheduler-futures med timeout, cancellation, filtrert fullføring og vern mot close medan operasjonar står uteståande. Direkte ABI- og scheduler-testar er køyrde under Windows/Wine.
- Pure VM eig no eit stabilt type-register med primitive, heap-, container- og custom typar. `std.runtime_type` brukar registeret for verdi-klassifisering og kan registrere/hente typar. Descriptorane dekkjer invariant generics, unionar, nullable, arv og metodeoppslag. Funksjonssignatur, async-flagg, instruksjonstal og modulfunksjonar kjem direkte frå aktiv NCB-tabell. Host-objekt blir utleverte som VM-eigde, tilbakekallbare bearer-handtak; faktisk dispatch validerer token, type, metodeallowlist, descriptor-capability, runtime-policy og plugin-capability før builtin-kallet. Forfalsking og bruk etter tilbakekall blir avvist. Type-runtime-flata er stabil.
- Flyttande arenakomprimering er no implementert: aktive blokker blir pakka mot starten, pensjonerte friblokker kan ikkje overlappe nye allokeringar, relokasjonstabellen oppdaterer alle levande objektadresser, og vidare allokering held fram frå ny arenaende. Fysisk `memmove` i ELF-heapen står att.
- Dynamisk NCB-loading og hot-reload er verifisert i same pure-VM-prosess frå plugin v1 til v2. ABI-format, modul-ID, SHA-256 trust store, navnerom, interne CALL-mål, kollisjonar, generasjon og dependency-rekkefølgje blir kontrollerte. Plugin-capabilities er isolerte frå appen og må tildelast eksplisitt.
- Dynamisk loader er no stabil for runtime v1: rein Norscode SHA-256 er verifisert mot standardvektorar og pinnar NCB-innhald i trust store; manipulert bytekode blir avvist. Dependencies må vere aktive før lasting, og hot-reload, ABI, navnerom, kollisjonar, generasjonar og plugin-capabilities er verifiserte i pure-VM.
- 10/10-modenheit blir styrt av [docs/MODENHET_10_10.md](MODENHET_10_10.md)
- 10/10-bevisflate kan sjekkast med `./bin/nc maintenance maturity`
- `docs/SELFHOST_HANDLINGSPLAN.md` er aktiv plan for normalflata
- `./bin/nc maintenance status|lane|seed|seed-status|verify|report|report-json` er statusflate i Norscode
- Testløparen `tools/nc_test.no` brukar no mtime/byte-storleik-signatur for å gjenbruke gyldig NCB-bytecode; `NC_TEST_CACHE=0` slår av cachen, medan `NC_TEST_CACHE_DIR` kan isolere cache per jobb.
- `stage0_seed_ok` er hovudindikatoren for stage-0 seed i `maintenance`-rapportane
- historiske filer skal liggje i `docs/_archive/` eller `archive/`

## Merknad

Gamle status-tal og gamle faser vart skrivne for ein eldre struktur. Dei er no tona ned for å unngå å påstå meir enn det dokumentasjonen faktisk viser.

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
  - `startproject` README-liste oppdaterte API-ruter
  - `startapp` README-liste oppdaterte API-ruter
- OpenAPI-sjekk i testane oppdatert til å forvente `response-model` i spec.
- Nytt eksempelsett i payload-filer:
  - prosjekt: `tests/payloads/api_payload.json`, `tests/payloads/api_nested.json`
  - app: `apps/<app>/tests/payloads/${APP_NAME}_payload.json`, `apps/<app>/tests/payloads/${APP_NAME}_nested.json`
- Dependency-injeksjon i app-skal:
  - Lagt til `GET /api/v1/${APP_NAME}/dependency` i `startapp` med `app_meta`-dependency.
  - Oppdatert app-ruteopplisting, testdekning og app-README.
- Feilhåndtering i app-skal:
  - Lagt til `GET /api/v1/${APP_NAME}/error` i `startapp`.
  - Demonstrerer standardisert `400` (`response_error`) og `500` (`response_error`), pluss suksess-tilfelle.
  - Oppdatert route-opplisting, testdekning og app-README.
  - Lagt til feilmønster for app-rot med:
    - 404-test for ukjende rute (`GET /api/v1/${APP_NAME}/ikkje-finst`)
    - 405-test for metodemismatch (`POST /api/v1/${APP_NAME}/query`)
- Standardisert request/response-kontrakt i begge skal:
  - Lagt til `POST /api/v1/${APP_NAME}/request-model` i `startapp` med request-validering + response-shape-validering.
  - Lagt til tilsvarande `POST /api/v1/request-model` i `startproject` med tilsvarande valideringsflyt.
  - Oppdatert både app- og stack-route-opplisting, testdekning (suksess + valideringsfeil) og README-rutelister.
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
