# Omgang 7 · full runtime-paritet — gap-inventar

**Kjernen i Omgang 7 er ein Docker/CI-differensial-loop** (køyr heile `std/`- +
`selfhost/`-testflata gjennom `tools/differensial_sele.no` på ARM64-Linux, tett
kvart avvik). Den kan IKKJE drivast frå macOS — native binærar må byggjast + køyrast
og codegen-endringane må vere rebygde. Dette dokumentet SCOPER kva loopen skal tette.

## Ferdig

- **M1 streng typesjekk** — compile-tid (`semantic.no` Pass 3 → `typecheck.kt_koyr`,
  av som standard). Skjer under kompilering, ikkje i native runtime → **ingen
  native jobb**. ✓
- **Omgang 3–7 EXECUTION-VERIFISERT på ekte ARM64 Linux** (Docker linux/arm64,
  Apple Silicon). Kjør `tools/verify_omgang_docker.sh` (fixturar i
  `tools/fixtures/ncb_arm64/`). Alle exit 42:
  - O3 closures: capture (0-arg) + capture+arg (CALL_VALUE arg-passing).
  - O4 struct-konstruktor + kall_metode: TYPE-dispatch (Punkt.sum vs Kvadrat.sum)
    + metode-med-argument.
  - O5 M10: default, rest, spread (`utvid`), optional (`DUP`), destrukturering.
  - O6 typa unnatak: typa-catch + catch-all + PROPAGERING (NettFeil forbi FilFeil).
  - O7 defer/finally: normal + return-gjennom + throw-gjennom.
  - **5 enkoding-bugs funne+fiksa** som strukturell codegen ikkje kunne sjå (sjå
    commit-historikk + [[omgang-native-codegen]]-minnet).

## Attståande native-hol (det Omgang 7-loopen tettar)

### 1. defer/finally-opcodar (ARM64) — `FINALLY_PUSH/RUN/END`, `LOAD_PENDING`, `CLEAR_PENDING` *(GJORT)*

`prøv { } endeleg { }` (finally). Design:
- **cleanup-stakk** (globalar `__cleanup_top__`/`__cleanup_base__`, region etter
  exc-regionen): record `[finally-adresse][lagra __exc_top__]` (rå verdi, ikkje
  count → inga divisjon; pruning = `str lagra → __exc_top__`).
- **pending-tilstand i LOKALE slots** (2 per finally-funksjon) → per-ramme,
  recursion-trygt (ikkje globalt som ville klobba nøsta kall).
- **FINALLY_PUSH**: adr finally + lagra __exc_top__ → cleanup-record. **FINALLY_RUN**:
  pop, pending=normal, `br` finally. **FINALLY_END**: dispatch pending_ctrl
  (1=return→ret_lbl, 2=throw→throw_lbl, elles→after_lbl). **LOAD/CLEAR_PENDING**:
  pending_val-slot.
- **RETURN gata på per-funksjon finally-bruk** (ikkje-finally-funksjonar er BYTE-
  IDENTISKE → avgrensa blast-radius): om cleanup ikkje tom → køyr finally med
  pending=return. **THROW**: om cleanup ikkje tom OG lagra exc_top == gjeldande
  (ingen handler inni finally) → køyr finally med pending=throw; FINALLY_END
  re-kastar → nøsta finallys via iterasjon.

Strukturelt verifisert (kompilerer). ⚠ **Maskinkode-ENKODINGA (inkl. den byte-
kritiske RETURN-endringa + M6-THROW-utvidinga) er handkoda + kryss-sjekka, IKKJE
køyrt** — svakast verifiserte biten. Docker/CI ARM64-Linux (`test_vm_finally`) er
fasit. Fixtur `diff_finally.no` (tolk-sida = exit 42, seed parsar `endeleg`).

### 2. x86-64-backend (`native_codegen_v2.no`) etterslep

Omgang 1–6 er gjort på ARM64 (`macho_arm64_codegen.no`, delt macOS+Linux-ELF).
x86-64 manglar: null-trygg INDEX_SET (delvis), BUILD_LAMBDA, CALL_VALUE, DUP,
struct-konstruktor + kall_metode, M10 rest/default/utvid, TRY_*/LOAD_EXCEPTION,
typa THROW-dispatch. B2-seed-målet er ARM64-Linux, så x86 er lågare prioritet, men
kravd for full differensial-paritet på x86-verten.

### 3. Gap-stubba builtins (~28) — runtime-ABI-flata

Fail-closed `NATIV-GAP:<namn>` (nc_main pakkar i prøv/fang → ærleg «ustøtta»).
Familiar:
- **Prosess/system:** `process_spawn_argv`, `process_operation`, `exec_prosess`,
  `system_operation`, `argv_operation`, `host_exec_ncb_json`.
- **Filsystem:** `filesystem_read/write_operation`, `mkdir_p`, `native_mkdir_p`.
- **Nettverk:** `network_operation`, `dns_lookup`, `socket_send/recv_bytes`,
  `socket_settimeout`.
- **Krypto:** `acme_sign`, `acme_verify` (+ sha256/pbkdf2/argon2id i eldre notat).
- **Compute:** `jit_operation`, `tensor_operation`, `atomic_operation`, `wasm_selftest`.
- **Kompilator:** `kompiler_fil`, `ncb_call_fn`, `ncb_route_handlers`.
- **Typar:** `desimaltall` (flyttal — NY runtime-type, eige løft).
- **web.\*** (heile familien).

Desse er OS-integrasjon (syscalls) eller compute-tunge — kvar er eit eige løft, ikkje
opcode-arbeid. Dei fleste er alt kartlagde i B2-native-minnet.

## Definisjon av «full runtime-paritet» (Omgang 7 ferdig)

`NORSCODE_DIFF_STRICT=1 tools/differensial_sele.no` grøn over HEILE `std/`- +
`selfhost/`-testflata på ARM64-Linux (Docker/CI): **null differensial-avvik**.
Rekkjefølgje: (1) execution-verifiser Omgang 3–6 i Docker/CI, (2) tett defer/finally,
(3) tett builtin-hol testflata faktisk krev, (4) x86-backend for x86-verts-paritet.
