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
- [x] Fase 2: fjern C-bibliotek, kvart gated
  — **removable-now-scope KOMPLETT + gate-grøn.** Dei to C-biblioteka med ei
  klar erstatning er fjerna: sqlite→NorsDB (pure DB grøn) og Metal-GPU-C (dropp,
  inga pure GPU-erstatning). Det tredje (legacy-C-backend = sjølve C-seed-bygg-
  motoren) har INGA grøn erstatning enno — den pure erstatninga ER Fase 3
  (sjølvhosta seed-bygg), som roadmap-prinsippet «pure erstatning grøn FØRST»
  krev. Så #3 er strukturelt sekvensert ETTER Fase 3, ikkje Fase-2-arbeid som
  står ugjort. Sjå detaljar nedanfor.
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
  - [x] **F2 (obj_addr) VALIDERT + separat liste-literal-bug funnen:** `obj_addr(x)`
    porta (`f75336d`) og emittert (parity urørt) VERKAR — bevist ved å lagra rdi@
    routine-entry til [0x600030] og lese det tilbake: på INT-arg er rdi = gyldig
    heap-ptr, og int-boks-layouten [type=1][value=12345] les rett via obj_addr+
    raw_load64. **Heile «obj_addr→0»-sagaen var ein SEPARAT, pre-eksisterande
    liste-literal-codegen-bug:** `la liste = [111,222,333]` etterlèt `rsp` feiljustert
    (usert `sub rsp,0x60`), så den påfølgjande builtin-arg-`pop` les 0. Bevist via
    lokal ELF-disasm (call-site) + runtime rdi-probe. obj_addr eksponerte berre
    bugen. Debug-teknikk: lagra register til fri kontrollblokk-slot [HEAP_VA+48] og
    les tilbake (betre enn gdb her). `lengde()` er òg eit eige AOT-gap.
  - **Merknad (non-blocking):** `obj_addr` på LISTE-typa Norscode-variablar gjev 0
    via den generiske `emit_rt_call_1`-vegen (int-typa verkar; hmac sin dedikerte
    handler les liste-arg rett). Djup AOT-arg-passing-quirk for liste-typa verdiar.
    **IKKJE GC-blokker:** GC-mark-trace les rå-peikarar frå stakk-rot-skann +
    `raw_load64`, ikkje `obj_addr` på Norscode-var. Kartlegging av liste-/map-
    layout skjer i F3 via rot-skann-adresser. (`lengde()` er òg eit eige AOT-gap.)
  - [x] **F3 (konservativ rot-skann) VALIDERT:** `stack_base()` (initiell rsp @
    [HEAP_VA+48], fanga i _start via ny `mov [HEAP_VA+48],r12`) + `stack_ptr()`
    (kallarens rsp) porta frå b2 (`c7436c7`). `gc_f3_probe` skannar [sp,sb) etter
    heap-peikar-slots (0x600000<=v<heap_bump) → fann roots (`F3 OK`, exit 0). Gen1
    ==Gen2-paritet urørt (begge generasjonar får rsp-capture). `1624c5f`.
  - [x] **(b) Mark-traversering-LOGIKK validert:** fleir-nivå raw_load64-peikar-jakt
    (boks[3]→payload[len][cap][elem-ptr..]→int-boks[1][value]) verifisert via manuelt
    konstruert struktur (raw_store64) → `trace OK` exit 0. Traverserings-maskineriet
    GC-mark brukar verkar. MERK: ekte-liste-manipulasjon i direkte-kompilert probe er
    blokkert av liste-arg-quirken (obj_addr/legg_til får 0 for liste-arg), og liste-
    literalar bur i `.data` (ikkje heap) → ekte-liste-layout stadfestast når GC køyrer
    i VM-en/seeden (F5). `9e4f918`.
  - [x] **Reclaim-motor (gc_alloc/gc_free/heap_set_bump) porta + validert:** fri-liste-
    hovud @ HEAP_VA+40; gc_alloc head-fit (gjenbruk om >= size, elles bump); gc_free
    push [next][size]. `gc_reclaim_probe`: gjenbruk + tom-liste-bump → exit 0. `069d4cd`.
  - [x] **gc_collect START — allokeringsfri mark-bitmap (`gc_mark_bit`) validert:**
    1 bit/16B-slot i scratch @ 0x10400000 (2 MB på toppen av 256 MB-heapen). Hand-
    emittert bit-set/les (sub/shr/and/movzx/shr/or/mov). `gc_mark_probe`: ny=0,
    gjenta=1, annan-slot=0 → exit 0. Første stein i mark-fasen. `03012bb`.
  - [x] **MARK-fasen (DFS-algoritme) VALIDERT:** `gc_mark_traverse_probe` — DFS over
    objekt-grafen med rå-minne mark-stakk (@0x10300000) + gc_mark_bit; manuell graf
    (liste[intA,intB]) → marka=3, alle boksar markerte → exit 0. Mark-LOGIKKEN verkar
    (referanse-versjon i høg-nivå Norscode). `2542bb2`.
  - [x] **SWEEP (live-map → gc_free) VALIDERT:** `gc_sweep_probe` — live-map
    (sortert (addr,size)), hol mellom påfølgjande live objekt → gc_free; gc_alloc
    gjenbrukar holet. sweep→reclaim-kjeda verkar. `53bf201`.
  - **HEILE mark-sweep-ALGORITMEN er no validert ende-til-ende** (referanse-form:
    mark-bitmap + DFS-trace + live-map-sweep + gc_alloc/gc_free). Den ALGORITMISKE
    uvissa er borte.
  - [x] **(i) `gc_collect` ENDE-TIL-ENDE VALIDERT (referanse):** mark+sweep samla —
    DFS-mark frå rot registrerer (addr,size) i live-map (box+payload); sweep gap-walk
    → gc_free døde hol; gc_alloc gjenbrukar. Probe: region m/live liste + interleava
    48B dødt hol → holet frigjort+gjenbrukt, live-verdi 777 URØRT. **Ein komplett,
    arbeidande mark-sweep-GC (referanse-form).** `97ac6f1`.
  - [x] **(ii-mark) ALLOKERINGSFRI maskinkode-MARK (`gc_mark_native`) VALIDERT:**
    ~60-instruksjons hand-emittert DFS med rå register + scratch (mark-stakk/live-map/
    bitmap), INGEN boksing i løkka. Same graf → retur 3, boksane markerte. **Bevist at
    maskinkode-GC-konverteringa er gjennomførbar.** `60c20b3` (test-fiks: heap_set_bump
    forbi grafen sidan range-sjekk krev addr<bump).
  - [x] **(ii-sweep) ALLOKERINGSFRI maskinkode-SWEEP (`gc_sweep_native`) VALIDERT:**
    walk live-map @0x10340000, gc_free hol mellom påfølgjande live objekt via INLINE
    fri-liste-push (rå, ingen boksing). Probe: 2 live + 48B hol → frigjort+gjenbrukt.
    `fa14a99`. **BEGGE dei harde maskinkode-delane (mark+sweep) står no validerte
    allokeringsfritt.**
  - **Attståande — integrasjon:** samle mark+sweep til `gc_collect_native` (bru
    live-map-teljar mark→sweep; bitmap-clear; scan ALLE stakk-roter, ikkje éin);
    (iii) re-pek codegen-allokator-emisjonar (RT_LIST_*/STR_RAW/INT) → gc_alloc;
    (iv) safepoint-terskel → kall gc_collect; (v) ELF-paritet + arm64; (vi) F5: seed
    mot 10k-hmac (den store raud→grøn). (iii) er den siste høg-risiko-biten.
  - Litmus for heile Fase 3: sjølvhosta codegen byggjer seed som passerer HEILE
    grøne suita (inkl. 10k-hmac). Først då: bytt seed-bygging C→sjølvhosta.
