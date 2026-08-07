# Sluttplan: Norscode 100 % selvstendig

Dette dokumentet er sluttplanen som lukkar dei 21 opne punkta i
[NORSCODE_SELFSTENDIGHET_PLAN.md](NORSCODE_SELFSTENDIGHET_PLAN.md)
(status 2026-08-06: 71 av 92 verifiserte). Planen følgjer same reglar som
arbeidsloggen: ei oppgåve blir berre avhuka når den levande porten er grøn,
kandidatfiler er ikkje promotert stage0, og dokumentasjon kan ikkje markere
noko ferdig før gatebeviset finst.

**Definisjonen av ferdig er lista «Endeleg godkjenning» i arbeidsloggen.**
Denne planen ordnar berre vegen dit i seks milepælar med eksplisitte
avhengigheiter og portar.

---

## Sammendrag (bokmål)

Norscode er allerede selvstendig på filnivå: null aktive Python-, C-, shell-
eller Zig-kilder i normal utviklings-, test- og CI-flate, og CI kjører
gjennom Norscode-binæren selv. Det som gjenstår før prosjektet kan kalles
**helt** selvstendig, faller i seks milepæler som bygger på hverandre:

**A — Stabiliser generasjonen (1–2 uker).** Attester Windows-kandidaten på
ekte Windows og promoter stage0 kontrollert, slik at det ene sanne
release-preflight-avviket forsvinner. Kjør deretter én komplett
`local-green --strict` på samme commit. Alt etterpå måles mot denne
generasjonen.

**B — Rene native bygg (2–4 uker).** Fjern GCC/Zig/Docker-overgangene fra
Linux-byggene, løs ARM64-minnetoppen med isolert/strømmet
modulmaterialisering, og lukk ABI-restansene (Windows-tråd, freestanding
atomics, rmdir/chmod/range-read i stage0). Målet er byte-identisk Gen1/Gen2
og signert attestasjon på alle fire plattformer for samme generasjon.

**C — Release- og pakkeflyt (1–2 uker, parallelt med B).** Grønne levende
macOS- og Linux-app-releaser uten eksterne normaloperasjoner, og en
eksplisitt, kort allowlist over lovlige smale plattformverktøy (codesign,
notarytool, gh, swiftc-grensen).

**D — Ren TLS/krypto (2–4 måneder, største faglige løft).** Ren Norscode
TLS 1.3 med differentialtesting mot OpenSSL-adapteren, konstanttidsrevisjon,
deretter ekstern sikkerhetsrevisjon. Først da flyttes OpenSSL (og SQLite/Zig)
permanent til valgfrie adaptere.

**E — Linux-drift og kvalitetsporter (3–6 uker, parallelt med D).**
systemd/journald/procfs-ABI med E2E-attestasjon, og utskifting av de siste
Python-kvalitetsportene med Norscode-eide porter.

**F — Endelig godkjenning.** Hele sluttlisten kjøres grønn på én commit og
én kandidatgenerasjon; statusmatrisene oppdateres; versjonen tagges. Etter
dette er selvstendighet et vedlikeholdsregime, ikke et prosjekt.

Kritisk sti: **A → B → F**, med D som lengste enkeltløp. C og E kan gå
parallelt. Resten av dokumentet er arbeidsplanen i repo-stil.

> **Framdrift 2026-08-07 (verifisert på tre uavhengige runtimar):**
> Milepæl D er godt i gang — fem reine krypto-modular er inne og RFC-verifiserte:
> `std/sha512.no` (NIST), `std/hkdf.no` (RFC 5869), `std/x25519.no` (RFC 7748
> inkl. §6.1 DH), `std/ed25519.no` (RFC 8032) og `std/tls13_keyschedule.no`
> (RFC 8448). **Alle fem grøne på (1) ARM64-stage0, (2) native macOS (`nc test`
> 1/1 kvar + `nc feature-check BESTÅTT`) og (3) Linux-CI (Krypto-smoke-workflowen
> grøn på grein `krypto-tls-primitiver`)** — D1-primitiva er difor `[x]`. Til saman gjev dei
> — med den alt verifiserte `std.chacha20_poly1305` — heile suiten
> TLS_CHACHA20_POLY1305_SHA256 i rein Norscode. Att står handshake-tilstandsmaskina
> (D2), differentialtesting og ekstern revisjon (D3). Milepæl A er ikkje køyrbar
> frå sky-økta (krev Mac + Windows-host + GitHub CI); sjå
> [MILEPÆL_A_RUNBOK.md](MILEPÆL_A_RUNBOK.md).
>
> **Framdrift 2026-08-07 (D2-kjerne, verifisert på Linux ARM64-stage0):**
> Tre nye reine Norscode-modular utgjer kjernen i TLS 1.3-tilstandsmaskina:
> `std/tls13_record.no` (record-lag, RFC 8446 §5, ChaCha20-Poly1305),
> `std/tls13_handshake.no` (meldingskodek, RFC 8446 §4) og
> `std/tls13_handshake_flow.no` (1-RTT handtrykk, loopback klient↔tenar).
> Tre nye testar er grøne på committed ARM64-stage0 (via miljøkontrakt
> `NORSCODE_CMD`/`NORSCODE_FILE`/`NORSCODE_ROOT`): `test_tls13_record_chacha`
> (RFC 8439 §2.8.2 keystream/nonce-anker + RFC 8448 IV-utleiing + rundtur +
> tukla-tag-avvising), `test_tls13_handshake_codec` (parsar RFC 8448
> ClientHello/ServerHello eksakt + rundtur på alle byggjarar) og
> `test_tls13_handshake_flow` (ECDHE-løyndom lik begge sider, Ed25519
> CertificateVerify verifisert, server+client Finished-MAC verifisert,
> applikasjonsdata over record-laget, fail-closed på tukla signatur).
> **Utvida same dag:** X.509-kjedevalidering (`std/x509_chain.no`, med
> Ed25519-sertifikatbyggjar) + `test_x509_chain_ed25519` (8 kontrollar,
> fail-closed); fuzzing av alle parserane (`test_tls13_fuzz_parsers`, ingen
> krasj — parserane herda mot indeks-utanfor-grensa); og eit differensial-KAT
> (`test_tls13_differential_kat`) som reproduserer heile RFC 8448-løyndomstreet
> frå rå meldingsbytes. Til saman **11 grøne kryptotestar** på ARM64-stage0.
> Att i D2: (1) integrasjon over den native socket-ABI-en (ikkje berre
> loopback), (2) RSA/ECDSA-signaturkjede + CRL/OCSP for web-PKI-interop,
> (3) live differensial-interop mot OpenSSL-adapteren. Difor `[~]` på dei
> attståande D2-punkta. Ikkje attestert på macOS/Windows og ikkje køyrt
> gjennom `nc test`/`feature-check` i denne økta (krev Mac-runtime).

---

## Milepæl A – Stabiliser generasjonen

Mål: éin frosen, fullt verifisert kandidatgenerasjon som alle seinare
milepælar målast mot. Lukkar release-preflight- og strict-punkta i
«Endeleg godkjenning».

- [ ] Bygg Windows-kandidat A/B byte-identisk frå gjeldande kjeldegenerasjon
      (siste målte paritetsbrot: kandidat 9 547 062 byte mot stage0
      11 604 280 byte er eit reelt generasjonsavvik, ikkje nondeterminisme).
- [ ] Køyr `tools/windows_runtime_attestation.no` på ekte Windows-host i CI
      med SChannel, AppContainer, IOCP, filesystem, prosess og Argon2id, og
      GitHub-signert provenance bunde til kjeldecommit og kandidat-SHA-256.
- [ ] Promoter Windows-stage0 kontrollert gjennom
      `tools/promote_attested_windows_stage0.no`; bevar førre generasjon som
      hashverifisert rollback under `bootstrap/stage0/rollback/`.
- [ ] Køyr `./bin/nc release-preflight --strict` og krev 0 feil (ikkje éin).
- [ ] Køyr komplett `./bin/nc local-green --strict` på same commit innanfor
      dei nye tidsgrensene (fire timar ytre L5b-kaldbygg, seks timar CI).
- [ ] Køyr full strict i Linux-CI med ekte samla testtotal (ikkje selftest,
      ikkje tom testliste — fail-closed-krava frå køyring `30790807053` og
      `30791877369` står).
- [ ] Frys generasjonen: noter kjeldecommit, dist-hash og alle fire
      stage0-hashar i arbeidsloggen som «generasjon G-A».

Port: `local-green --strict` grøn lokalt **og** i CI på same commit, med
release-preflight på 0 feil. Ingen seinare milepæl får målast mot ein annan
generasjon utan å oppdatere G-A eksplisitt.

## Milepæl B – Rene native bygg på alle plattformer

Mål: lukkar fase 5. Byte-identisk Gen1/Gen2 og signert attestasjon på
macOS ARM64, Linux x86-64, Linux ARM64 og Windows x86-64 for same
generasjon, utan GCC, Zig, C-bundlarar eller Docker-byggeverktøy i
normalflyten.

### B1 Linux x86-64

- [ ] Gjer den verifiserte source-only-kandidaten (947 funksjonar, native
      codegen, utan precompiled maskering) til normal byggveg i CI på ekte
      Linux-runner utan Docker som byggverktøy; Docker kan stå att som rein
      attestasjonssandkasse, ikkje byggkrav.
- [ ] Verifiser `builtin.process_spawn_argv` og heile prosess-ABI-en i den
      native AOT-kandidaten (rettinga frå 2026-08-05 er implementert; krev
      grøn L5/L5b på kandidaten, ikkje berre testfila).
- [ ] Byte-identisk Gen1/Gen2 på Linux x86-64 med cache på og av.

### B2 Linux ARM64

- [ ] Erstatt den historiske GCC/C/OpenSSL-TLS-overgangen med same native
      codegen-bane som x86-64; Zig-byggjarane er alt fjerna.
- [ ] Løys 8,7 GiB-RSS-toppen frå kjeldeekte bygging ved å materialisere
      modulane isolert/strøymt (same fragmentmønster som L5b brukar i dag),
      slik at ARM64-kandidaten kan byggjast utan precompiled maskering.
- [ ] Byte-identisk Gen1/Gen2 på Linux ARM64; signert attestasjon på ekte
      ARM64-host.

### B3 ABI-restansar i tillitsankeret

- [ ] Promoter ein stage0-generasjon som eksponerer binær range-read/slice,
      `chmod`/filmodus og `rmdir` — dette fjernar dei to dokumenterte
      `/bin/chmod`-kompatibilitetsreservane og Windows-tempopprydding-
      åtvaringa i same løft.
- [ ] Fullfør Windows-thread-backend og freestanding ARM64/Windows-atomics
      (`norscode-native-thread-v1`/`norscode-atomic-v1`).
- [ ] Fullfør attverande filesystem-/nettverks-/sikkerheits-ABI-punkt:
      Windows HANDLE-restansar, hard Darwin-minnegrense, attverande
      seccomp-/AppContainer-detaljar.
- [ ] Kvar ny ABI følgjer promoteringsregelen: kandidat → levande smoke →
      full kandidatmatrise → kontrollert atomisk promotering med rollback.

### B4 Samla plattformbevis

- [ ] Éin CI-køyring samlar signerte attestasjonar frå alle fire plattformer
      bundne til same kjeldecommit og kandidatgenerasjon, og skriv
      `production_ready_all_platforms=true` for **gjeldande** generasjon
      (mønsteret frå køyring `30830409254`, no utan overgangsbaner).

Port: `platform_readiness_v3600` grøn på gjeldande generasjon; ingen
GCC/Zig/Docker-byggverktøy i nokon aktiv byggbane; byte-identisk Gen1/Gen2
dokumentert per plattform i arbeidsloggen.

## Milepæl C – Release- og pakkeflyt utan eksterne normaloperasjonar

Mål: lukkar det siste opne fase-3-punktet. Kan gå parallelt med B.

- [ ] Grøn levande macOS-app-release i CI: Norscode ZIP/filmodus, plist-gate
      og signering utan `ditto`/`plutil`; den strukturerte
      `xcrun`/`swiftc`-bana brukast berre gjennom prosess-ABI og feilar
      lukka.
- [ ] Grøn levande Linux-app-release i CI utan 143-avbrot: AppDir + TAR.GZ
      med range-read-kandidaten (v146-banen), sidecar-hash og `0755`-modus
      verifisert i nedlasta artefakt.
- [ ] Skriv den eksplisitte allowlista over smale plattform-/signeringsgrenser
      (`codesign`, `notarytool`, `gh attestation`, `swiftc`-grensa for
      GUI-host, OS-pakkeverktøy der Norscode ikkje kan eige operasjonen) i
      `platform/README` og lås henne i `active-surface`/`release-preflight`.
- [ ] Fjern overgangsbygget frå normal releaseflyt når stage0 frå B3 er
      promotert.

Port: alle release-workflows grøne på gjeldande generasjon; active-surface
rapporterer 0 eksterne normaloperasjonar; alt utanfor allowlista feilar.

## Milepæl D – Rein Norscode TLS/krypto og ekstern revisjon

Mål: lukkar fase 7. Dette er det største faglege løftet og det lengste
enkeltløpet; start så tidleg som råd og køyr parallelt med B/C/E.

### D1 Kryptoprimitiv som manglar

- [x] X25519 nøkkelutveksling i rein Norscode med RFC 7748-vektorar.
      `std/x25519.no` (TweetNaCl-stil feltaritmetikk, 16 limbar radix 2^16,
      maskebasert konstanttids montgomery-stige). RFC 7748 §5.2 vektor 1/2,
      iterert basepunkt og §6.1 DH. **Grøn på native macOS-runtime** (`nc test`
      1/1) og `nc feature-check BESTÅTT` 2026-08-07. Full konstanttidsrevisjon
      høyrer til D3.
- [x] HKDF (RFC 5869) i rein Norscode. `std/hkdf.no` over `hmac_sha256_bytes`;
      RFC 5869 Case 1 og Case 3. Grøn på native macOS-runtime + feature-check.
- [x] Ed25519 signering + verifisering i rein Norscode. `std/ed25519.no`
      (TweetNaCl-stil edwards25519), RFC 8032 §7.1 Test 2 (`public_key`, `sign`
      eksakt; `verify` aksept/avvising). Grøn på native macOS-runtime +
      feature-check. ECDSA-P-256 og konsolidering med ACME/DNSSEC-signering står
      framleis att som eige punkt.
- [x] TLS 1.3-nøkkelplan (RFC 8446 §7) over HKDF. `std/tls13_keyschedule.no`
      (HKDF-Expand-Label, Derive-Secret); RFC 8448 (early `33ad0a1c…`, derived
      `6f2615a1…`). Grøn på native macOS-runtime + feature-check. Full
      handshake-integrasjon (D2) står att.
- [x] **Suitepolicy vald:** TLS_CHACHA20_POLY1305_SHA256 som første reine suite.
      Alle byggeklossane finst no i rein Norscode: X25519 (nøkkelutveksling),
      Ed25519 (signatur), HKDF-SHA256 (nøkkelplan) og den alt verifiserte
      `std.chacha20_poly1305` (RFC 8439 AEAD). AES-GCM blir eventuelt lagt til
      seinare som ekstra suite, ikkje som krav.

#### D1-evidens 2026-08-07

- **Trippel runtime-verifisering (gatebevis som løftar D1-primitiva til `[x]`):**
  (1) ARM64-stage0 via miljøkontrakt; (2) native macOS — `nc test` gav
  `1/1 bestått, 0 feila` for kvar av dei fem testane (`test_sha512_nist`,
  `test_hkdf_rfc5869`, `test_x25519_rfc7748`, `test_ed25519_rfc8032`,
  `test_tls13_keyschedule_rfc8448`), og `nc feature-check` gav `BESTÅTT` med
  `[OK] ingen aktiv C/Python-flate`; (3) Linux-CI — den dedikerte
  `.github/workflows/krypto-smoke.yml` køyrde alle fem via `./bin/nc run` på
  Linux x86_64-stage0 og var **grøn** på grein `krypto-tls-primitiver`.
  Krypto-smoke er isolert frå `vm_active_functions`-gapet (aktiv dist vs.
  kandidat) og Windows-pariteten, så det grøne krysset gjeld reint kryptoen.
- `std/sha512.no` (`norscode-sha512-v1`) og `tests/test_sha512_nist.no`: rein
  Norscode SHA-512 med 64-bit ord som `[hi32, lo32]`-par. NIST-vektorar (tom,
  `abc`, 56-byte multiblokk). Trengst av Ed25519 og TLS-transkript.
- `std/hkdf.no` (`norscode-hkdf-v1`) og `tests/test_hkdf_rfc5869.no`: HKDF-Extract
  og -Expand over HMAC-SHA256. Levande grøn mot committed ARM64-stage0 med
  RFC 5869 Case 1 (PRK `077709...b3e5`, 42-byte OKM `3cb25f...5865`) og Case 3
  (tom salt/info, PRK `19ef24...cb04`, OKM `8da4e7...96c8`).
- `std/x25519.no` (`norscode-x25519-v1`) og `tests/test_x25519_rfc7748.no`:
  Curve25519 X25519. Levande grøn mot committed ARM64-stage0 med RFC 7748 §5.2
  vektor 1 (`c3da55...8552`), vektor 2 (`95cbde...7957`), 1-iters basepunkt
  (`422c8e...3079`) og §6.1 DH: offentlege nøklar `8520f0...4e6a`/`de9edb...2b4f`
  og delt løyndom `4a5d9d...1742` likt begge vegar.
- `std/ed25519.no` (`norscode-ed25519-v1`) og `tests/test_ed25519_rfc8032.no`:
  edwards25519 signering/verifisering. Verifisert levande mot ARM64-stage0 med
  RFC 8032 §7.1 Test 2 — `public_key` = `3d4017c3…`, `sign` = `92a009a9…bb0c00`
  (begge eksakt), `verify` aksepterer gyldig og forkastar tukla melding. Kvar
  del testa isolert (Ed25519 er tungt i pure VM; samla test køyrer på native).
- `std/tls13_keyschedule.no` (`norscode-tls13-keyschedule-v1`) og
  `tests/test_tls13_keyschedule_rfc8448.no`: HKDF-Expand-Label + Derive-Secret.
  Verifisert levande mot RFC 8448 (early secret `33ad0a1c…`, derived `6f2615a1…`).
- **Reint TLS-suite-fundament:** X25519 + Ed25519 + HKDF-SHA256 +
  ChaCha20-Poly1305 (alt rein Norscode) dekkjer TLS_CHACHA20_POLY1305_SHA256
  utan ekstern krypto. Det som står att for D er sjølve handshake-tilstandsmaskina
  (D2), differentialtesting og ekstern revisjon (D3).
- Begge testane køyrer òg grønt frå den ekte repo-rota (ikkje berre isolert
  temp-prosjekt), slik at `std.hkdf`/`std.x25519` resolverer frå disk i normal
  importflate. Ingen tillitsanker er endra; `dist` og stage0 er urørte. Køyringa
  er ikkje gjennom `nc test`/`feature-check` (krev Mac-runtime i denne økta) og
  ikkje attestert på macOS/Windows — difor `[~]`, ikkje `[x]`.

### D2 TLS 1.3-tilstandsmaskin

- [~] Klient- og tenarhandshake (RFC 8446): ClientHello/ServerHello,
      nøkkelplan, certificate verify, finished og application data er
      implementert og verifisert som loopback i minnet
      (`std/tls13_handshake_flow.no`, `test_tls13_handshake_flow`).
      Att: køyring over den eksisterande native socket-ABI-en, alerts og
      eksplisitt session-avslutning.
- [~] X.509-kjedevalidering på den strukturerte `std.x509`-lesaren:
      `std/x509_chain.no` gjer gyldigheitsperiode, SAN/hostname (RFC 6125,
      wildcard), Ed25519-signaturkjede og trust-anchor-kontroll, verifisert i
      `test_x509_chain_ed25519` (gyldig kjede, wildcard-SAN, feil vertsnamn,
      utløpt, ikkje-gyldig-enno, tomt trust store, tukla signatur, feil
      utstedar — alle fail-closed). Att: RSA/ECDSA-signaturverifikasjon for
      interop med web-PKI, og eksplisitt CRL/OCSP-plan.
- [~] Differentialtesting: `test_tls13_differential_kat` reproduserer HEILE
      RFC 8448-løyndomstreet (early→handshake→c/s hs traffic→master→c/s ap
      traffic + IV) frå rå ClientHello/ServerHello-bytes og ECDHE-løyndom, med
      steg-for-steg samanlikning mot RFC-vektorane. Feilinjeksjon (tukla tag,
      feil seq, avkorta meldingar) er dekt i record- og fuzz-testane. Att:
      live interop klient↔tenar mot OpenSSL-adapteren (krev adapteren/host).
- [x] Fuzzing av parserane (handshake-meldingar, record, sertifikat) med
      avgrensa ressursbruk, fail-closed: `test_tls13_fuzz_parsers` køyrer
      deterministisk LCG-fuzzing (vilkårlege + bit-muterte meldingar) gjennom
      alle handshake-, record- og X.509-parserane utan krasj; parserane er
      herda til å returnere feil i staden for å indeksere utanfor grensa.

### D3 Revisjon og adapterisering

- [ ] Konstanttidsrevisjon av heile kryptoflata (Argon2id har alt
      konstanttidskontroll; utvid til samanlikningar, GCM-tag, signaturar).
- [ ] Engasjer ekstern sikkerheitsrevisjon med scope: TLS-tilstandsmaskin,
      X25519/Ed25519/HKDF, ChaCha20-Poly1305, Argon2id, DNSSEC- og
      ACME-signering. Funna handterast som fail-closed portar.
- [ ] Byt standard: rein Norscode TLS som normalveg, OpenSSL som eksplisitt
      valfri adapter (same mønster som Argon2id-flyttinga).
- [ ] Fjern OpenSSL-, SQLite- og Zig-avhengigheitene frå normalkandidatane;
      v127/fixed16-kandidatane viser at plattform-only-bygget alt fungerer.
      Eksterne bibliotek finst etterpå berre som eksplisitte valfrie adapterar
      med eigne differentialportar.

Port: TLS-differentialporten og revisjonsrapporten grøne; normalkandidat
utan OpenSSL/SQLite/Zig-link består full kandidatmatrise; `otool -L`/ELF-
avhengigheiter viser berre systembibliotek.

## Milepæl E – Linux-drift og Norscode-eigde kvalitetsportar

Mål: lukkar dei to siste fase-8-punkta. Kan gå parallelt med D.

- [ ] `norscode-systemd-v1`: unit-status, start/stop/restart og enable via
      D-Bus-API (ikkje `systemctl`-tekst), med fail-closed feilkontrakt.
- [ ] journald-lesing/skriving og procfs-målingar (CPU, minne, fd, uptime)
      som versjonert ABI.
- [ ] Brukar/gruppe-oppslag, chmod/chown og privilegiedropp
      (setuid/setgid/supplementary groups) gjennom smale POSIX-ABI-ar.
- [ ] Levande E2E-attestasjon i isolert Linux-miljø: deploy-state-maskina
      frå `std.deploy_state` køyrer tenestebytte, helseprøve og rollback mot
      faktisk systemd; dette lukkar samstundes systemd-delen av
      deploy-punktet.
- [ ] Erstatt attverande Python-kvalitetsportar: HTTP/TLS-E2E-porten i rein
      Norscode (byggjer på `std.https_front`-E2E-en som alt er levande),
      systemd-isolasjonsport, native skjermbiletefangst-ABI og
      `std.pixel_diff`-porten kopla saman til visuell regresjonsport.
- [ ] Utvid Python-auditen til å dekkje kvalitetsport-flata, slik at null-
      baselinen gjeld heile repoet utanfor `archive/`.

Port: alle kvalitetsportar køyrer gjennom `./bin/nc` utan Python; levande
systemd-E2E grøn i CI; Python-audit rapporterer 0 aktive `.py` totalt.

## Milepæl F – Endeleg godkjenning og vernebånd

Mål: lukkar «Endeleg godkjenning» og gjer selvstendigheit til eit
vedlikehaldsregime.

- [ ] Køyr heile sluttlista på éin commit og éin generasjon:
      `active-surface`, `selvstendighet --strict`, `selfcompile-l5b` med og
      utan cache, `local-green --strict`, `release-preflight`, full test- og
      slow-lane utan uklassifiserte hopp.
- [ ] Byte-identisk Gen1/Gen2 dokumentert på alle fire plattformer for same
      generasjon.
- [ ] Signert attestasjon frå macOS, Linux x86-64/ARM64 og ekte Windows på
      gjeldande kandidatgenerasjon i same CI-køyring.
- [ ] Kontroller at ingen aktive `.sh`, `.py`, `.c`, `.h` eller Zig-kjelder
      finst i normal bygg-/release-/CI-flate, og at ingen eksterne prosessar
      utfører operasjonar Norscode sjølv støttar (allowlista frå milepæl C er
      einaste unntak).
- [ ] Synkroniser dokumentasjonen: README, STATUS, runtime-/stdlib-matrisene
      og arbeidsloggen skal samsvare med dei levande portane; arkiver
      historiske påstandar.
- [ ] Tag releasen og skriv sluttnotatet i arbeidsloggen med generasjons-
      hashar og CI-run-ID-ar.
- [ ] Vernebånd etterpå: strict-porten køyrer på kvar hovudgrein-push,
      full plattformattestasjon minst per release, og kvar ny ABI følgjer
      kandidat→matrise→promotering-regelen. Nye avhukingar kan framleis
      opnast att av nye bevis — det er regelen som har halde loggen ærleg.

Port: alle elleve punkta i «Endeleg godkjenning» avhuka på levande bevis.

---

## Rekkjefølgje, avhengigheiter og risiko

Kritisk sti er **A → B → F**: utan frosen generasjon (A) er alle seinare
målingar flytande, og utan rene native bygg (B) kan ikkje sluttattestasjonen
(F) bindast til éin generasjon. C heng på stage0-promoteringa i B3 for dei
siste chmod-/range-read-reservane, men det meste av C kan gå parallelt med B.
D er det lengste enkeltløpet og bør starte straks — D blokkerer berre F, ikkje
B/C/E. E er uavhengig av B/C og blokkerer berre F.

Dei største risikoane, med mottiltak: **(1)** Rein TLS er tryggleikskritisk
— derfor differentialtesting mot OpenSSL-adapteren, fuzzing og ekstern
revisjon som harde portar før standardbyte, aldri eigne-testar åleine.
**(2)** ARM64-minnetoppen ved kjeldeekte bygging — løysinga er strøymd/
isolert modulmaterialisering som L5b alt brukar, ikkje større maskiner; å
auke grenser er forbode av regelverket. **(3)** Generasjonsdrift — kvar gong
kjelda endrar seg vesentleg etter A, må G-A-referansen oppdaterast eksplisitt
og strict køyrast på nytt; det er kostnaden ved sannferdige portar.
**(4)** Committed stage0-alder — fleire CI-feil har kome av at eldre stage0
manglar nyare builtins; B3-promoteringa reduserer denne klassen feil, og
inntil då gjeld miljøkontrakt-mønsteret (`NORSCODE_CMD`/`NORSCODE_FILE`) og
eksplisitt klassifiserte kompatibilitetshopp.

Grove tidsestimat med éin utviklar: A 1–2 veker, B 2–4 veker, C 1–2 veker
(parallelt), D 2–4 månader inkludert ekstern revisjon (revisjonen sjølv har
typisk 4–8 vekers leietid — bestill tidleg), E 3–6 veker (parallelt), F 1
veke. Realistisk samla horisont: **eit halvt år**, der D er takta. Alt anna
enn D kan vere ferdig på 6–8 veker.

## Kva denne planen ikkje endrar

Normal kjede (`.no` → NCB JSON → `selfhost/vm.no`), promoteringsreglane,
tillitsanker-disiplinen og fail-closed-kulturen står urørde — dei er grunnen
til at 71 punkt alt er verifiserte. Planen legg ikkje til nye verktøyspråk,
nye C-steg eller nye stackgrenser, og han flyttar ingen historikk ut av
`archive/`.
