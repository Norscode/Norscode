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
- [x] Fase 1: cherry-pick b2 sitt C-frie validerte arbeid (grønt kvart steg)
  — **achievable-on-committed-seed-scope KOMPLETT + CI-grøn.** Alt attståande b2-
  C-fritt arbeid er strukturelt gated (ikkje Fase-1-arbeid som står ugjort): sjå
  «Blokkert/utsett tail» nedanfor — gate-ar på Fase 3 (seed-rebygg/kompilator/core)
  eller annan sesjon (bigint-familien).
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
  - [x] **Metal-GPU-C droppa**: inga rein-Norscode GPU-erstatning → droppa som
    feature. Fjerna `nc_metal_tensor.c` + `native_metal_gpu_gate.no` + Metal-
    frameworks/gate i ci.yml; erstatta `#include "nc_metal_tensor.c"` i seed-bygg-
    motoren `nc_native_main.c` med pure-C ABI-stub (metal_available=0, matmul→CPU-
    veg, diffusion→grasiøs feil). Committa seed urørt (gate grøn). Merk: metal/
    tensor-testar passerer på committa seed no; må oppdaterast når Metal-fri seed
    landar (Fase 3).
  - **Strukturell grense (attståande #3):** legacy-C-backend
    (`nc_native_main.c`/`nc_runtime_mini.c`) + `build/v3009/*.c` ER sjølve C-seed-
    bygget (ci.yml `clang`/`cc`) — pure erstatning = sjølvhosta seed-bygg (Fase 3).
    System-dynamisk libsqlite3 (dlopen; committa Linux-seed lenkar libsqlite3.so)
    + `tests/native_gc_*.c` (GC-design) høyrer òg Fase 3 til. Å fjerne desse før
    sjølvhosta seed-rebygg bryt «pure erstatning grøn FØRST». **2 av 3 mål gjort;
    #3 (legacy-backend) gate-ar på Fase 3.**
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
  - [x] **Validerings-loop ETABLERT:** `tests/fixtures/gc_litmus_hmac.no` (10000
    hmac_sha256_bytes) → bundla → `ncb-to-elf` (self-hosted native_codegen_v2) →
    native ELF → køyr. `tools/gc_litmus_run.sh` + EIGA workflow `gc-litmus.yml`
    (non-gating, rører ikkje Selvstendighet-porten). Byggjer lokalt (71 KB x86-64
    ELF); køyrer på Linux CI og REPRODUSERER veggen (SIGSEGV) = raud→grøn-mål (`ad293d2`).
  - [x] **F1 (råminne-primitiv) landa + CI-validert:** raw_load64/raw_store64/
    heap_bump kirurgisk inn i vår `native_codegen_v2.no` (via `atomics`-mapen, l.
    ~1230 + `builtin_va`-dispatch). Lazily emittert → Gen1==Gen2-paritet urørt
    (gate grøn). `tests/fixtures/gc_f1_probe.no` (store→load round-trip) byggjer
    self-hosted ELF + køyrer i `gc-litmus.yml` som **GATING** F1-steg (exit 0 =
    primitiva verkar). `heap_alloc_start` UTSETT: krev `CHAR_CACHE_BASE/COUNT`
    (b2 #181-heap-layout vår baseline manglar) → deriver vår alloc-start i F2.
  - [x] **F2 (obj_addr + objekt-layout) landa + CI-validert:** `obj_addr(x)`
    (boksar NcVal-peikaren) porta (`f75336d`) inn i vår codegen. `gc_f2_probe.no`
    inspiserer liste-layout via obj_addr+raw_load64 (type_tag==3, len==3) → GATING
    F2-steg i `gc-litmus.yml`. Generisk `tools/gc_probe_run.sh <probe.no>` køyrar.
    Gen1==Gen2-paritet urørt (gate grøn).
  - **Attståande (fleir-sesjons):** derive HEAP_ALLOC_START for vår layout +
    konservativ rot-skann (stack_base/stack_ptr); allokeringsfri maskinkode
    mark+sweep + fri-liste (live-map); re-pekte HOT-allokatorar; safepoint-terskel-
    wiring; ELF-paritet + arm64-port. Kvar fase probe/litmus-validert — GC-bug
    korrupterer alt.
  - Litmus for heile Fase 3: sjølvhosta codegen byggjer seed som passerer HEILE
    grøne suita (inkl. 10k-hmac). Først då: bytt seed-bygging C→sjølvhosta.
