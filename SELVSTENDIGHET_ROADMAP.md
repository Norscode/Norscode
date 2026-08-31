# Selvstendighet frå #179 — disiplinert, alltid-grøn veikart

**Baseline:** commit `52593ad` (PR #179, «Fix self-sufficiency seed verification
order»), siste fullt grøne tilstand. C-backa verktøykjede med fungerande GC.

**Kvifor denne linja:** b2-independence prøvde å fjerne C (og GC-en) i eit *big
bang* FØR den sjølvhosta erstatninga var ferdig → ~150 samtidige feil rota i ein
ufullstendig runtime (ingen GC). Denne greina byggjer selvstendighet inkrementelt
frå den grøne baselinen i staden.

## Grunnprinsipp

> **Fjern aldri ei C-avhengnad før den pure-Norscode-erstatninga er BEVIST grøn.
> CI-grønt er porten — kvart steg må passere.**

## Fasar

- **Fase 0 — Etabler porten.** Push denne greina → GitHub, stadfest CI grøn på
  baselinen. Utan ein fungerande grøn-port er «hald grønt» umogleg å verifisere.
- **Fase 1 — Port b2 sitt validerte, C-frie arbeid (grønt kvart steg).**
  Cherry-pick det som IKKJE treng GC og er bevist: språkfunksjonar (enum, lambda/
  closure, metode, destrukturering), krypto i rein Norscode (sha256/pbkdf2/web-
  push/TLS), ELF stage-0-forbetringar. Gjenvinn b2 sin ekte framgang, disiplinert.
- **Fase 2 — Fjern C-bibliotek, kvart gated.** sqlite→NorsDB (pure), Metal-GPU-
  C-runtime, legacy-C-backend. For kvar: bygg+verifiser pure erstatning grøn
  FØRST, deretter fjern C-en.
- **Fase 3 — Sjølvhosta seed-bygging (GC-en).** Fullfør sjølvhosta native codegen:
  legg inn GC (validert fundament, sjå AOT_GC_DESIGN nedanfor) + tett codegen-hull
  (crypto-gap-rute, JIT, minne-effektiv batched Mach-O mot OOM). Litmus: sjølvhosta
  codegen byggjer ein seed som passerer HEILE grøne suita (inkl. 10k-hmac). Først
  då: bytt seed-bygging C→sjølvhosta, fjern C-bootstrap.
- **Fase 4 — Siste C-fjerning + attestering.** Fjern gjenverande C; selvstendig-
  hets-gatane (L1–L6, active-surface) grøne med null C.

## Avgjerande sekvensering

Contained wins (Fase 1–2) FØRST — lågrisiko, held momentum, gir umiddelbar
selvstendighets-framgang utan å røre den skjøre runtime-en. Det harde GC/codegen-
byttet (Fase 3) SIST og fullt gated. Stikk motsett av b2 sin feil.

## GC-fundament (frå b2-undersøkinga, portast i Fase 3)

Validerte komponentar klare til port (b2-independence-commits, sjå
`docs/05-development/AOT_GC_DESIGN.md` der):
- Objekt-layout kartlagt (int/bool/streng/liste/map — peikarfelt + storleik).
- Mark-trace-logikk validert (gc_trace følgjer objekt-grafen korrekt).
- Reclaim-motor validert (gc_alloc fri-liste-pop + gc_free push, size-fit).
- Rot-skann (stakk er komplett rotsett; globals tomme).
- Empirisk: ~65KB/iter garbage-rate; bump-reset utrygt → full mark-sweep påkravd.
Att: allokeringsfri maskinkode-mark+sweep + re-pek allokatorar + safepoint-wiring.

## Status

- [x] Fase 0: roadmap dokumentert
- [x] Fase 0: grøn CI-port stadfesta (run 33334581579 — verify-linux + hard
  ELF Gen1==Gen2-gate begge grøne på `selvstendig-fra-179`)
  - Greina held seg **uavhengig** av main (main absorberte B2 sin big-bang, er
    raud, divergerer ~1190 filer). Forsoning utsett til Fase 4. PR #186 = draft.
  - Portfiksar: seed-materialisering før feature-check (`d658ca4`); bin/nc
    fallback til committa stage-0-seed på Linux/macOS (`3fcb241`); branch i
    push-trigger (`38b8fa5`).
- [~] Fase 1: cherry-pick b2 sitt C-frie validerte arbeid (grønt kvart steg)
  - [x] Krypto-bolk: `builtin.sha256 → std.sha256.hash` (`95550ba`), pbkdf2_sha256
    + random_hex → pure (`3b3389b`). CI grøn. Inert for committa seed/stage-0.
  - [x] Binær-NCB-bolk: rein NCB-codec (`9abfa49`), varint/zigzag (`93063da`),
    dual-format serde (`fe4e752`), opt-in output (`9112b4b`), null-decode-fiks
    (`fc6e681`). **Validert grøn via `nc run-pure` (exit 42)** — ikkje berre landa.
    Premature test i `tests/pending_seed_activation/` (`78a0d05`).
  - **Validerings-verktøy funne:** `nc run-pure <fil.no>` (hybrid-bundlar → rein
    VM) aktiverer nye `selfhost.*`-modular UTAN seed-rebygg, og propagerer stdout +
    returverdi→exit. Avdekte `null→0`-bug i porten (missa b2-followup `edcf29f`) →
    cherry-picka. Køyr run-pure på kvar port sin test for ekte «bevist grøn».
  - **Strukturell grense funnen:** portert b2-kode er DVALE på committa seed
    (seed-aktiveringsgapet — nye `selfhost.*`-modular er ikkje i seeden sin
    innebygde bunt). Sann validering krev soft-seed (`run-ncb-pure`) eller Fase 3
    seed-rebygg. Vidare b2-C-frie arbeid (atomics, vm-builtins, defer/finally)
    er dessutan velda til b2 sine store kjerne-omskrivingar (`macho_arm64_codegen.no`
    ~7000-linjes divergens, `vm.no` ~337) → ikkje reine cherry-picks; krev manuell
    kirurgisk port eller seed-rebygg.
  - [x] **Nøkkelinnsikt:** nye `std.*`-modular resolverer frå KJELDE på committa
    seed (ulikt `selfhost.*`) → INGEN aktiveringsgap; testar køyrer grønt direkte
    via `nc test`. Dette opna den store, reine Fase 1-lana:
  - [x] **Rein Norscode-krypto/stdlib/TLS portert frå b2, alt validert via `nc test`:**
    - Hash: md5 (RFC 1321), sha1 (RFC 3174), sha512 (NIST), blake2b (RFC 7693).
    - KDF/MAC: hkdf (RFC 5869), argon2id_pure (RFC 9106).
    - Koding/OTP: base32 + totp (RFC 4648/6238).
    - AEAD: chacha20_poly1305 (RFC 8439), AES-GCM (aes+ghash+aes_gcm, NIST).
    - Signatur/ECDH: ed25519 (RFC 8032), x25519 (RFC 7748).
    - **TLS 1.3 (rein Norscode):** tls13_handshake, tls13_keyschedule (RFC 8448),
      tls13_record, tls13_handshake_flow (full handshake).
    - Stdlib/JS-paritet: js_liste, js_objekt, js_streng, js_tal, hendelseslokke,
      kanal, pixel_diff, wasm_binary, protocol_stream, backup_aead.
  - **Blokkert/utsett tail (dokumentert):** rsa/ecdsa_p256/x509/x509_chain krev
    `std.bigint` (annan sesjon eig bigint/ecdsa/vapid/webpush); norsdb-kjeda brukar
    `;`-syntaks committa seed sin parser avviser (kompilator-mismatch); linux_drift/
    auth_mfa/http_download krev native-builtins (`system_operation`/`system_info`)
    ikkje i seeden; pbkdf2-multiblock krev endring i core `std/krypto.no` (32-byte-cap);
    atomics/vm-builtins/defer-finally velda til b2-kjerne-omskrivingar. Alle desse
    gate-ar på Fase 3 seed-rebygg eller annan sesjon.
- [~] Fase 2: fjern C-bibliotek, kvart gated
  - [x] **sqlite → NorsDB**: pure `std.db`/NorsDB verifisert grøn FØRST
    (test_db_features/repository/integration), so sletta vendra
    `third_party/sqlite/sqlite3.{c,h}` (9.7 MB, `7916320`) + rydda alle
    vendra-refs i seed-bygg-verktøy/ci.yml/allowlist (`2e7c902`). Gate grøn.
  - **Strukturell grense:** resten av Fase 2 er kopla til Fase 3. C-en som står
    att ER seed-bygg-motoren:
    - Metal-GPU-C (`nc_metal_tensor.c`) er `#include`-a i `nc_native_main.c`.
    - legacy-C-backend (`nc_native_main.c`/`nc_runtime_mini.c`) + `build/v3009/*.c`
      er sjølve C-seed-bygget (ci.yml `clang`/`cc`).
    - System-dynamisk libsqlite3 (dlopen; committa Linux-seed lenkar libsqlite3.so)
      + `tests/native_gc_*.c` (GC-design, Fase 3).
    Å fjerne desse før sjølvhosta seed-rebygg (Fase 3) bryt «pure erstatning grøn
    FØRST» (ingen sjølvhosta seed-bygg enno). Difor: Fase 2-restar gate-ar på Fase 3.
- [~] Fase 3: sjølvhosta seed-bygging (GC-en) — den harde veggen
  - [x] Landa validert GC-design: `docs/05-development/AOT_GC_DESIGN.md` (frå b2
    sin scoping). Objekt-layout kartlagt, mark-logikk validert (gc.no gc_trace),
    live-map-sweep vald (headrar forkasta), rot-sett = native stakk (`functions`).
  - **Kvifor dette er annleis enn Fase 1/2:** GC er hand-emittert maskinkode i
    `native_codegen_v2.no` — DVALE på committa seed OG ikkje `run-pure`-validerbar
    (run-pure køyrer ikkje native codegen). Einaste validering = **byggje eit
    sjølvhosta seed + køyre litmus** (10k-hmac må ikkje SIGSEGV). Det krev minne-
    tungt bygg (OOM på 16 GB Mac → Docker/CI). Chicken-and-egg: seed-bygget treng
    GC-en for å passere litmus.
  - **Validerings-loop (må etablerast FØRST):** `selfcompile-stage0-elf` byggjer
    alt eit Gen1 ELF-seed i CI (grøn). Utvid til: bygg sjølvhosta seed → køyr
    10k-hmac-litmus. Det REPRODUSERER veggen (raud baseline) = grønt-målet for GC.
  - **Attståande (fleir-sesjons, frå AOT_GC_DESIGN F2–F6):** port F1–F3-primitiv
    (raw_load64/obj_addr/stack-skann — konflikt mot vår diverga `native_codegen_v2`/
    `vm.no`, krev kirurgisk merge); allokeringsfri maskinkode mark+sweep + fri-liste;
    re-pekte HOT-allokatorar; safepoint-terskel-wiring; ELF-paritet + arm64-port.
    Kvar fase isolert-validert (litmus før full flate) — GC-bug korrupterer alt.
  - Litmus for heile Fase 3: sjølvhosta codegen byggjer seed som passerer HEILE
    grøne suita (inkl. 10k-hmac). Først då: bytt seed-bygging C→sjølvhosta.
