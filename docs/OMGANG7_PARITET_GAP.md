# Omgang 7 · full runtime-paritet — gap-inventar

**Kjernen i Omgang 7 er ein Docker/CI-differensial-loop** (køyr heile `std/`- +
`selfhost/`-testflata gjennom `tools/differensial_sele.no` på ARM64-Linux, tett
kvart avvik). Den kan IKKJE drivast frå macOS — native binærar må byggjast + køyrast
og codegen-endringane må vere rebygde. Dette dokumentet SCOPER kva loopen skal tette.

## Ferdig

- **M1 streng typesjekk** — compile-tid (`semantic.no` Pass 3 → `typecheck.kt_koyr`,
  av som standard). Skjer under kompilering, ikkje i native runtime → **ingen
  native jobb**. ✓
- **Omgang 3–6-opcodane** (closures, struct-metodar, M10, typa unnatak) — ARM64
  dekt (sjå `docs/OPCODE_MATRISE.md`: BUILD_LAMBDA/CALL_VALUE/TRY_*/LOAD_EXCEPTION
  = `arm=JA`).

## Attståande native-hol (det Omgang 7-loopen tettar)

### 1. defer/finally-opcodar (ARM64) — `FINALLY_PUSH/RUN/END`, `LOAD_PENDING`, `CLEAR_PENDING`

`prøv { } etterpå { }` (finally). KOMPLEKST + kryssande: tolken brukar ein eigen
`cleanup_stack` (skild frå try_stack) + per-ramme `pending_control`/`pending_value`/
`pending_target`, og THROW må KØYRE finally-blokkene under unntaks-propagering og
resume pending-kontroll (return/throw/normal) etter. **Interagerer direkte med den
M6-typa THROW-dispatchen** (Omgang 6), som enno ikkje er køyre-verifisert. Difor:
bør implementerast ETTER at `__throw_dispatch__` er stadfesta i Docker/CI — elles
byggjer ein uverifisert kompleks maskinkode på uverifisert maskinkode.

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
