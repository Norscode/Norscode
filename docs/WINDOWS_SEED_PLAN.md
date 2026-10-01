# Windows-seed: plan for ein Norscode-bygd `norscode-windows-x86_64.exe`

Mål: erstatte den frosne C-æra-binæren `bootstrap/stage0/norscode-windows-x86_64.exe` med ein
`nc_main` for windows-x86_64 som Norscode sin eigen codegen byggjer frå kjelde — same kjede
som dei tre andre seedane (materialisert full-host-NCB → førehandsbygd kryss-codegen → binær),
utan C, Python eller framande lenkjarar.

## 0. Vegkart til 100 % native Windows-seed (2026-10-01, etter #208)

Status etter #208 (fletta til main): den native Windows-**codegenen** er komplett og verifisert
GRØN på ekte Windows (alle syscalls ruta, 0 rå syscall; `nc compile`/fil-I/O/filops/system_info
byte-identiske Linux↔Windows; run 36819849216). Dei tre andre seedane er native i main. Det
EINASTE som står att for «100 % sjølvstendig native Norscode» er å **promotere Windows-seeden**
(erstatte C-æra `e03f820a`). Gaten for promotering er W6-attestasjonen (8 testar på ekte Windows).

**Faste reglar for kvart steg (lærdom frå #208):**
- Kvar endring i `native_codegen_v2.no` (eller importlukkinga) krev REGEN av codegen-fikspunktet:
  bygg `bootstrap/native_codegen_x86_64.elf` (cg1==cg2==cg3), skriv `.srchash`/`.depshash`,
  køyr `tools/build_cross_codegen.no` (kryss + `.vertcg`), oppdater dei 3 godkjend-binær-pinane
  i `tools/active_surface_allowlist.txt`. Elles raud: `test_native_codegen_srchash` + `test_arm64_kryss_codegen`.
- Ingen rå shell i CI (Driftsvakt/`no_c_python_active_surface`, baseline 0): alt gjennom
  `tools/ci_shell_runner.no` med `NORSCODE_VM_CI_COMMAND`-binding (sjå `arm64-seed.yml`).
- Windows-only-verifikasjon berre på ekte `windows-latest` (wine er usann oracle for
  kernel32-eksportar — jf. WaitOnAddress-fella i #208).

**W4 — trådar + katalog (no `enosys`):**
- `clone(56)` → `CreateThread` + entry-thunk (barnet held fram med eigen stakk; Linux-clone-
  modellen må emulerast — eigen Windows-mode-emisjon, ikkje OSTAB-rutine).
- `futex(202)` → `WaitOnAddress`/`WakeByAddressAll` (importer frå `api-ms-win-core-synch-l1-2-0`
  / kernelbase, IKKJE kernel32). Trådutgang → `ExitThread` (eigen nr, ikkje `ExitProcess`).
- `getdents64(217)` → dir-HANDLE via `CreateFileW(FILE_FLAG_BACKUP_SEMANTICS)` +
  `GetFileInformationByHandleEx(FileIdBothDirectoryInfo)` → linux_dirent64. `os_open` må opne
  katalogar med backup-semantics.
- Verifikasjon: trådfikstur + `liste_mappe` på windows-latest.

**W6 — nett / async / TLS / sandkasse (dei 5 harde attestasjonstestane):**
- `test_windows_native_network`: `ws2_32` (`WSAStartup`/`socket`/`connect`/`send`/`recv`/
  `closesocket`) som OSTAB-rutinar; `std/native_gap.no` Windows-backend.
- `test_windows_iocp_scheduler`: `CreateIoCompletionPort`/`GetQueuedCompletionStatus`/
  `PostQueuedCompletionStatus` — async-planleggjaren.
- `test_windows_filesystem_iocp`: overlappa fil-I/O via IOCP.
- `test_windows_schannel_client`: TLS-klient via `secur32` SChannel (`AcquireCredentialsHandle`/
  `InitializeSecurityContext`/`EncryptMessage`/`DecryptMessage`) ELLER bruk den reine-Norscode
  TLS-stakken (x25519/ed25519/chacha20/tls13 finst alt — sjå [[tls-over-sokkel-plan]]) over
  ws2_32-sokkelen; sistnemnde er mest «native».
- `test_windows_process_appcontainer`: `CreateProcessW` + AppContainer-SID-sandkasse.
- Dei 3 krypto-testane (`test_argon2_native`/`test_acme_sign_native`/`test_acme_verify_native`)
  er rein Norscode-compute og bør passere utan nett — verifiser dei fyrst (3/8).

**W5/W7 — bygg + promoter seeden:**
- `tools/build_windows_seed.no` (materialiser full-host nc_main-NCB → kryss-codegen → PE), analog
  til `tools/build_arm64_seed.no`; kryss-codegen `bootstrap/native_codegen_windows-x86_64_on_linux-x86_64.elf`
  (+ hashar/.vertcg/pin) via `tools/build_cross_codegen.no` med `NC_CROSS_TARGETS=windows-x86_64`.
- CI-jobb `windows-seed.yml` (analog `arm64-seed.yml`): kryssbygg på ubuntu → attestasjon på
  windows-latest (dei 8 testane) gjennom `ci_shell_runner` (policy-konform).
- Når grøn: byt `bootstrap/stage0/norscode-windows-x86_64.exe` → den Norscode-bygde, flytt
  C-æra til `bootstrap/stage0/rollback/`, oppdater `SHA256SUMS` + allowlist, og flytt B4-porten
  frå «PE-prefiks == committa» til «committa == bygd frå kjelde» (`NC_WINDOWS_KREV_LIK`).

Estimat: W4 ~2–3 økter, W6 ~3–5 økter (TLS/IOCP tyngst), W5/W7 ~1 økt. Alt må verifiserast på
ekte windows-latest (ingen interaktiv Windows lokalt → iterasjon via CI-push).

## 1. Noverande tilstand (survey 2026-09-27)

### Den frosne exe-en

* `bootstrap/stage0/norscode-windows-x86_64.exe` (8 969 740 B, sha256 `e03f820a…`) er ein
  C-æra-tolkevert (PE32+ x86-64) med innebygd NCB-trailer `NORSCODE_NCB_TEXT_V1`
  (payloadane `vm` og `vm-executor`). Kjelda er sletta (b3feedb); han kan ikkje byggjast frå repoet.
* CI-jobbar som er avhengige av han:
  * `ci.yml: windows-runtime-candidate` (macos-latest) — `tools/build_windows_stage0_candidate.no`
    legg genererte payloadar (J1c) som ny trailer på den committa exe-en → kandidat A/B.
  * `ci.yml: windows-runtime-cross-compile` (windows-latest) — skal-steg går gjennom exe-en
    (`shell: …/norscode-windows-x86_64.exe run tools/ci_shell_runner.no`), `selftest`, og
    `tools/windows_runtime_attestation.no` på kandidaten.
  * `ci.yml: windows-stage0-parity` — B4-porten `tools/verify_windows_stage0_parity.no`
    (PE-prefiks-sha + manifestform + payload == generert).
  * `windows-app-release.yml` — same kandidatbygg + attestasjon ved release.
* Attestasjonen (`tools/windows_runtime_attestation.no`) krev ekte Windows x86-64 og køyrer
  8 testar gjennom runtimen: `test_windows_process_appcontainer`, `test_windows_native_network`,
  `test_windows_schannel_client`, `test_windows_iocp_scheduler`, `test_windows_filesystem_iocp`,
  `test_argon2_native`, `test_acme_sign_native`, `test_acme_verify_native`. Dette er sluttporten.

### Kva som finst av Windows-kode i Norscode

* `selfhost/native_execution/pe_emitter.no` — fast 1536 B PE32+ med éin import
  (`KERNEL32!ExitProcess`); `verifiser_pe` sjekkar nett den forma.
* `selfhost/native_execution/windows_codegen.no` — berre `PUSH_CONST n; RETURN` → ExitProcess(n).
* `selfhost/native_execution/codegen_prebuilt.no` — `windows-x86_64` er eit mål; kjelda er
  `native_codegen_v2.no` (PE-emisjon skal inn der), tolka fallback er med vilje nekta.
  `nc bygg-native --target windows-x86_64` seier «PE-codegen kjem i W2/W5».
* `std/native_sys.no` — systemtabellar for linux-x86_64, linux-arm64, macos-arm64; andre mål kastar.
* `std/native_gap.no` — all OS-logikk (prosess, sokkel, fil, tid) går over `builtin.sys6`
  (rå Linux/Darwin-systemkall). Windows har ingen stabil systemkall-ABI.

### Kva codegenen (native_codegen_v2) krev av eit OS

* Fast layout (absolutte adresser, abs32-adressering): `.text` 0x401000, konstantar 0x700000
  (`DATA_VA`), verts-ABI-side 0x770000 (`HOST_ABI_VA`, globalar/argv/miljøoverlay),
  heap 0x780000 (`hl.HEAP_VA`), GC-metadata opp til 0x6B000000.
  Heapen er 0x6AA00000 B i GC-modus og 2 GiB utan; utan GC ville han kollidere med
  `KUSER_SHARED_DATA` (0x7FFE0000) → **Windows-målet er berre GC-modus**.
* 58 `syscall`-stader i runtime-atoma og inline-builtins (write, read, open, close, lseek,
  stat, access, unlink, rmdir, mkdir, chmod, getdents64, clock_gettime, nanosleep, getrandom,
  uname, getcwd, readlink, mprotect, futex, clone, exit, pluss den generiske `sys6`-vegen).
  Mange av dei ligg mellom handkoda hopp med faste avstandar.
* `_start` les argc/argv/envp frå Linux-startstakken.

## 2. Arkitektur

### 2.1 Bilete (PE32+)

* `ImageBase` 0x400000, `IMAGE_FILE_RELOCS_STRIPPED`, `DllCharacteristics` utan
  `DYNAMIC_BASE`/`HIGH_ENTROPY_VA` (fast adresse, same VA-ar som ELF-en → same maskinkode).
* Seksjonar, samanhengande i VA (Windows-lastaren krev det):

  | seksjon  | VA          | VirtualSize            | innhald                                        | flagg |
  |----------|-------------|------------------------|------------------------------------------------|-------|
  | `.text`  | 0x401000    | til 0x700000           | runtime + funksjonar + Win64-OS-rutinar + `_start` | RX |
  | `.rdata` | 0x700000    | til 0x770000           | konstantsegmentet (`bygg_data`)                | R     |
  | `.hostabi` | 0x770000  | 0x1000                 | verts-ABI-sida                                 | RW    |
  | `.idata` | 0x771000    | til 0x780000           | OS-porttabell + importkatalog + IAT + namn     | RW    |
  | `.heap`  | 0x780000    | `HEAP_SZ` (GC)         | uinitialisert (SizeOfRawData 0)                | RW    |

  Heapen som uinitialisert seksjon gjer at lastaren reserverer heile området atomisk før
  noko anna (stakk, prosessheap) kan hamne der. Risiko: commit-kostnad for ein stor
  skrivbar seksjon; alternativ (VirtualAlloc på fast adresse i `_start`) er dokumentert
  som reserve om Windows nektar.
* Stakk: `SizeOfStackReserve == SizeOfStackCommit` = 16 MiB. Linux veks stakken for
  vilkårlege tilgangar; Windows krev vaktside-probing, og runtimen har rammer > 4 KiB.
  Fullt committa stakk fjernar probe-kravet. Rekursjonsvakta (initiell rsp − 7 MiB) held.
* Subsystem CONSOLE; NCB-traileren blir lagd etter siste rå seksjon (overlay), som på ELF.

### 2.2 OS-grensa: Win64 berre i OS-rutinane

* Kvar Linux-`syscall`-stad blir emittert gjennom éin hjelpar som gjev byte-sekvens per mål.
  Linux: `mov eax,nr ; syscall` (7 B, uendra). Windows: `call qword [abs32 OSTAB + nr*8]`
  (`FF 14 25 disp32`, **òg 7 B**). Der `mov eax,nr` og `syscall` står skilde, emitterer
  Windows-modus instruksjonane imellom først og kallet til slutt — same totale lengd, so dei
  handkoda hoppa over staden står urørte. Stader utan like lange sekvensar (inline-builtins
  som `sys6`) ligg aldri mellom handkoda hopp; funksjonsstorleikane kjem frå målepasset.
* `OSTAB` (512 × 8 B på 0x771000) held peikarar til Norscode-emitterte OS-rutinar, indeksert
  med Linux-nummeret; ukjende nummer peikar på `enosys` (−38, fail-closed).
* Kvar OS-rutine tek Linux-registerkonvensjonen (rdi, rsi, rdx, r10, r8, r9), returnerer
  Linux-form i rax (≥ 0 eller −errno) og klobrar berre rax/rcx/r11 (som `syscall`).
  Inni: ta vare på alle andre register + xmm0–5, align rsp, 32 B skuggeplass, kall
  `KERNEL32`-funksjonar via IAT (`call [abs32 IAT-slot]`), omset GetLastError → errno.
* Fd-modell: 0/1/2 → `GetStdHandle`; andre fd-ar er HANDLE-verdiar (alltid delelege med 4,
  aldri 0/1/2).
* Stiar: UTF-8 → UTF-16 med `MultiByteToWideChar(CP_UTF8)` på stakken.
* `_start` (Windows): `GetCommandLineW` + `CommandLineToArgvW` (shell32) og
  `GetEnvironmentStringsW` → UTF-8 med `WideCharToMultiByte` → argv**/envp** i ein
  `VirtualAlloc`-buffer, same slottar som Linux-`_start`.
* Emittert av ein liten Norscode-assemblar (`selfhost/native_execution/win64_os.no`) med
  etikettar og rel32-fiksar — ingen handrekna hoppavstandar i ny kode.

### 2.3 native_sys / native_gap

* `builtin.native_target()` = `"windows-x86_64"` i Windows-modus.
* `std/native_sys.no` får ein `windows-x86_64`-tabell for dei Linux-nummera OS-rutinane
  emulerer (fil/tid/tilfeldig), elles fail-closed.
* Det som ikkje kan kartleggjast 1:1 (fork/execve/wait4/pipe2/dup2, sokkel, epoll) får ein
  eigen Windows-backend i `std/native_gap.no` over nye OS-atom: prosess via `CreateProcessW` +
  røyr (`CreatePipe`) + `WaitForSingleObject`/`GetExitCodeProcess`; nett via `ws2_32`
  (WSAStartup/socket/connect/…); hendingar via IOCP. Atoma følgjer same mønster
  (Norscode-emittert Win64-rutine bak IAT).

### 2.4 Linux-output byte-identisk

Alle endringar i `native_codegen_v2.no` er gata på målet (`NC_TARGET=windows-x86_64`).
Etter kvar codegen-endring: bygg codegen-ELF på nytt (bundle i Docker med
`bootstrap/stage0/norscode-linux-x86_64` → kompiler med committa
`bootstrap/native_codegen_x86_64.elf` → cg1), og krev at cg1 gjev **byte-lik** Linux-output
med committa codegen-ELF for fikstur-program og for full nc_main-NCB
(`scratch/gcbuild/nc.ncb.json`). Deretter fikspunkt cg1→cg2→cg3 og ny `.srchash/.depshash`,
kryss-`.vertcg` og pinnar i `tools/active_surface_allowlist.txt`.

## 3. Milepælar

Lokal køyreløkke: Docker-image `nc-wine` (debian:12-slim + wine64, `--platform linux/amd64`),
`wrun.sh <exe>` → exit-kode. Wine er naudsynt, men ikkje tilstrekkeleg: sluttporten er ekte
Windows i CI.

| # | Innhald | Akseptansetest |
|---|---------|----------------|
| **W0** | `pe_emitter.no`: generell PE32+-byggjar (seksjonsliste, importtabell for fleire DLL-ar/funksjonar, IAT-slot-kart) + hello world `GetStdHandle`/`WriteFile`/`ExitProcess`. | `tests/test_pe_emitter.no` (strukturell verifikasjon av importkatalog/IAT); under wine: skriv linja, exit-kode lik argumentet. |
| **W1** | `native_codegen_v2` Windows-modus: PE-layout (§2.1), `OSTAB` + OS-rutinar for write/read/exit/clock_gettime/nanosleep/getrandom/mprotect, Windows-`_start` med argv/envp, `native_target`. | `hei.no` under wine: linja + exit 42; fiksturar for tid, tilfeldig, tekst/liste/ordbok, unnatak, GC-stress. Linux-output byte-lik (fiksturar + full nc_main-NCB). |
| **W2** | Filsystem-rutinar: open/openat/close/lseek/stat/access/unlink/rmdir/mkdir/chmod/getdents64 (FindFirstFileW), uname/getcwd/readlink (`GetModuleFileNameW`), `system_info` = Windows. | fil_skriv/fil_les/fil_finnes/mkdir_p/liste_mappe-fiksturar under wine; `system_info()["system"]` inneheld «windows». |
| **W3** | Prosess-backend i `native_gap` (`CreateProcessW`, røyr, timeout, exit-kode, miljø) + `native_sys`-tabell for windows-x86_64. | `process_spawn_argv`-fiksturar under wine; `tools/ci_shell_runner.no` køyrer ein kommando. |
| **W4** | Trådar/atomics (`CreateThread`, `WaitOnAddress`/`WakeByAddressAll` for futex) + GC-safepoint med fleire trådar. | trådtestane i standard-suiten under wine. |
| **W5** | Kryss-codegen `bootstrap/native_codegen_windows-x86_64_on_linux-x86_64.elf` (+ .srchash/.depshash/.vertcg, pin) frå `tools/build_cross_codegen.no`; `tools/build_windows_seed.no` (materialize → kryss-codegen → PE); CI-jobb `windows-seed.yml` analog til `arm64-seed.yml`. | Kryssbygd `nc_main_windows_x86_64.exe`: `version`, `selftest`, `run hei.no` på windows-latest; byte-lik ved to bygg. |
| **W6** | Nett (`ws2_32`), IOCP-planleggjar, SChannel-klient (`secur32`), AppContainer-prosess — dei Windows-spesifikke attestasjonstestane. | Dei 8 attestasjonstestane grøne på windows-latest med den Norscode-bygde exe-en. |
| **W7** | Promotering: exe-en blir `bootstrap/stage0/norscode-windows-x86_64.exe`; B4-porten byter frå «PE-prefiks == committa» til «committa == bygd frå kjelde» (`NC_WINDOWS_KREV_LIK`), kandidat/paritet-jobbane les den nye seeden. | Heile Windows-CI grøn; `windows-stage0-parity` + seed-byte-likskap. |

Estimat: W0–W2 er éi–to økter kvar; W3/W4 to–tre; W5 éi; W6 er den største (tre–fem);
W7 éi. Kring 8 milepælar / 12–18 økter til promoterbar seed.

## 3f. Status (2026-10-01) — VERIFISERT PÅ EKTE WINDOWS (GitHub Actions grøn)

* Workflow `.github/workflows/windows-native-verify.yml` + fikstur `tests/windows_native/`:
  ubuntu byggjer fikstur-PE-ane + nc_main.exe via Norscode-codegen (cg1) med LINID-port;
  **windows-latest køyrer dei på EKTE Windows**. Run 36819849216 GRØN:
  `[WIN-OK] hei exit=42 … tidrand exit=17`, og `nc_main.exe version → "Norscode 0.1.0"`.
  Exitkodane kan ikkje forfalskast → ekte Windows-stadfesting av OSTAB→kernel32-vegen.
* **wine≠Windows-felle funnen og fiksa av det ekte steget:** import av WaitOnAddress/
  WakeByAddressAll frå KERNEL32 gav STATUS_ENTRYPOINT_NOT_FOUND (0xC0000139) ved lasting på
  ekte Windows (dei er api-set/kernelbase, ikkje kernel32; wine hadde dei). Fiks: os_importar_full
  importerer berre det ein implementert rutine kallar.
* **Att for 100 % native Windows-seed:** W6-attestasjon (SChannel/IOCP/AppContainer) + W7
  stage0-promotering (erstatte C-æra `e03f820a`, krev identisk hash); ekte threads/getdents.
  Codegen-vegen er no verifisert på ekte Windows — det var det wine ikkje kunne.

## 3e. Status (2026-10-01) — ALLE syscalls ruta; Windows-nc fullstendig kompilator

* **0 rå Linux-syscall att i Windows-PE-en.** Alle ~55 syscall-stader i native_codegen_v2
  rutar gjennom OSTAB→kernel32/advapi32/shell32 når `NC_TARGET=windows-x86_64`. OS-rutinar i
  win64_os: write/read/exit/getrandom/mprotect/clock_gettime/setup_argv/access/open/close/
  lseek/stat/unlink/rmdir/mkdir/chmod/uname/nanosleep/getcwd/readlink/rename(MoveFileExW).
  getdents(217)/futex(202)/clone(56) → os_enosys (grøne trådar + katalog-listing = W4/dir,
  treng ekte Windows for verifikasjon).
* **Teknikkar:** `emit_sys_clean7` (replace_all for clean7, lengdenøytral); split-drop-mov-eax
  (lengdenøytral); dynamisk `call [OSTAB+VAR*8]` for u_nr(unlink/rmdir)/r_nr(rename/symlink);
  register-indeksert `FF 14 C5` (sys6 med runtime-nr i rax); length-neutral call+2×nop for
  movabs-nanosleep. fil_les read-staden (§5): 4 spennande handkoda hopp justerte ±3.
* **Windows-nc er ein fullstendig, 100 % Norscode-bygd kompilator.** Verifisert (Docker cg1 +
  wine, byte-identisk Linux↔Windows): `nc version`/`nc`(bruk)/`nc compile` (BYTE-IDENTISK NCB);
  fil_finnes/fil_les/fil_skriv; mkdir/fil_slett (dir=1/sletta=1); system_info (system=Windows);
  6/6 språk-fikstur (streng/unnatak/GC/dict/tid/random) LINID+WIN-OK.
* **Att:** batch-vegen `ncval_x86_link_with` sin `_start` er ikkje Windows-tilpassa (les Linux-
  stakk-argv) — nc brukar `kompiler_v2`-vegen som ER komplett, so ikkje eit problem for nc;
  ekte getdents/futex/clone-backend (W4/dir); W5 seed-promotering + W6-attestasjon (IOCP/
  SChannel/AppContainer) på EKTE Windows-CI + W7 promotering — ikkje lokalt køyrbare.

## 3d. Status (2026-10-01) — W1 fullført (argv) + W2 fil-I/O; nc compile på Windows

* **Full argv** (commit 5f8dd2f): `os_setup_argv` (OSTAB-slot 500) synteserer argv frå
  GetCommandLineW→CommandLineToArgvW→WideCharToMultiByte(CP_UTF8) inn i ein VirtualAlloc-
  buffer, skriv {argv**, argc} + tomt envp til verts-ABI-slottane. `builtin.argv_list_v1`
  gjev IDENTISK argc/args på Linux og Windows (verifisert `foo bar` → argc=3).
* **W2 fil-I/O — LANDA** (c07d51d/e1580a6/029e163): nye OS-rutinar med UTF-8→UTF-16-
  stikonvertering: os_access (GetFileAttributesW), os_open (CreateFileW + flagg-kart),
  os_close (CloseHandle), os_lseek (SetFilePointerEx), os_stat (st_mode). Ruta syscall-
  stadene i fil_finnes (access), fil_les_safe (stat), fil_les (open/lseek×2/read/close —
  read-staden er den ikkje-lengdenøytrale §5-staden: 4 spennande handkoda hopp justerte ±3)
  og fil_skriv (open/write/close, alt lengdenøytralt).
* **MILEPÆL: nc_main byggjer som Windows-PE og fungerer som kompilator.** `nc version`,
  `nc` (bruk), `nc compile <fil> -o <ncb>` køyrer under wine med **byte-identisk output**
  som Linux (nc compile NCB sha256-verifisert lik). fil_finnes/fil_les/fil_skriv gjev
  identiske resultat Linux↔wine. Wine er trygg oracle på alle desse (ingen rå syscall att
  i stiane). 6/6 fikstur (hei/tekst/unnatak/gcstress/lister/tidrand) framleis LINID+WIN-OK.
* **W2-review** (win-w2-review, dbaa3ad): fiksa UTF-16-buffer-overflyt (ramme 4128→4160) og
  to os_setup_argv-layout-kollisjonar (envp↔argv argc≥769; argv↔streng argc≥1025) via
  dynamisk argc-basert bufferlayout. Verifisert argc=901 → tomt miljø + rette args.
* **Att (lang hale):** binær fil-I/O (fil_skriv_binar/fil_les_bin), katalog-op (getdents64
  →FindFirstFileW, mkdir/unlink/rmdir/rename/chmod), system_info (uname/getcwd/readlink),
  lstat — alle W2-rutinar som manglar; og W4 (nanosleep/futex/clone/thread-exit, arkitektur:
  CreateThread ≠ clone). W5 (kryss-codegen-seed-bygg) + W6 (attestasjon på EKTE Windows-CI,
  ikkje lokalt køyrbar) + W7 (promotering) står att.

## 3c. Status (2026-09-30) — W1 codegen-innkopling landa (fikstursett)

* **W1 codegen-innkopling — LANDA og verifisert** (commit 661f21d + os_getrandom-fiks 99dabb5).
  `native_codegen_v2.no` emitterer no ein Norscode-bygd PE32+ for `NC_TARGET=windows-x86_64`:
  * Målplumbing: `X86_MAL()` les `NC_TARGET` (standard `linux-x86_64` → byte-identisk), `ER_WINDOWS()`;
    `heap_layout` deler `windows-x86_64` med `linux-x86_64` (identiske VA-ar).
  * PE-container `bygg_pe_bilete`: samanhengande seksjonar per §2.1; `.idata = [OSTAB 4096 B @0x771000]
    ++ [importkatalog @0x772000]`; `.heap` som bss; NCB-trailer som overlay. `OSTAB_VA()`/`IMPORT_VA()`.
  * OS-rutinane (write/read/exit/getrandom/mprotect/clock_gettime + enosys) via `win64.emit_os_routines`
    inn i `.text`, 512-slot OSTAB fylt med VA-ane. `emit_syscall_site` (FF 14 25 disp32, 7 B).
  * Windows-`_start` gata i `kompiler_v2` (stakk-base, rekursjonsvakt, GC-vaktside via `VirtualProtect`,
    `ExitProcess`-exit). **Minimal tom argv/envp** enno (sjå «att» under).
  * Ruta syscall-stader: skriv-write (split 7765), `_start` exit(60), GC-guard mprotect(10),
    now_ms clock_gettime(228), random_byte getrandom(318) — alle lengdebevarande, gata på `ER_WINDOWS()`.
  * **Verifisert seed-uavhengig** (Docker `nc-x86tools` byggjer cg1 frå worktree-kjelde via committa
    `native_codegen_x86_64.elf`; køyrt i `nc-wine`): 6/6 fikstur (hei/tekst/unnatak/gcstress/lister/
    tidrand) **byte-identisk Linux-output** OG **rett stdout + exitkode under wine** via kernel32. Wine
    er trygg oracle på desse (ingen rå syscall i stiane; exitkoden kan ikkje forfalskast).
  * Adversarial review (4 dim + verify): éin ekte bug funnen og fiksa — `os_getrandom` testa heile
    rax på RtlGenRandom sin 1-byte BOOLEAN (fail-closed-brot); fiks `and rax,255`.
* **Att i W1:** full argv/envp i Windows-`_start` (GetCommandLineW/CommandLineToArgvW/
  WideCharToMultiByte — MERK: `builtin.argv()` gjev argc=0 på Linux-baseline òg, så semantikken må
  avklarast før Windows-argv kan verifiserast); batch-vegen `ncval_x86_link_with` sin `_start` (for
  full nc_main / W5); resten av split/bare2-stadene til `os_enosys` inntil W2/W4-rutinane finst.
* **Merk harness:** `scratch/windows/`-skripta (§3b) blir tømde ved sesjonsomstart. Den reproduserte
  raske lykkja: `nc bundle native_codegen_v2.no` → Docker `nc-x86tools` byggjer cg1 via committa
  codegen-ELF (~1,6 s) → cg1 kompilerer fikstur for begge mål (~0,16 s) → `cmp` mot committa-codegen-
  output (Linux-identitet) + `nc-wine`-køyring (exitkode/stdout).

## 3b. Status (2026-09-28)

* **W0 — ferdig.** `pe_emitter.no`: `bygg_importar` (importkatalog/ILT/IAT/namn, fleire DLL-ar),
  `bygg_pe` (samanhengande seksjonar, ImageBase 0x400000, RELOCS_STRIPPED, utan DYNAMIC_BASE),
  `beskriv_pe` og `bygg_hei_pe`. Ny `win64_asm.no` (assemblar med etikettar/rel32).
  Verifisert under wine: hello world via IAT, rett UTF-8-linje + exit 42. `tests/test_pe_emitter.no`.
* **W1 — grunnmur ferdig, codegen-innkopling står att.** `win64_os.no` emitterer heile
  OS-call-trampolina: OSTAB + rutinane write/exit/getrandom bak IAT med Linux-syscall-ABI,
  felles rbp-ankra ramme som self-alignar og bevarer rdi/rsi/rdx/r10/r8/r9. `emit_os_call`
  gjev 7 B (= «mov eax,nr; syscall»). Verifisert under wine (`bygg_os_demo_pe`: write+getrandom+exit
  via OSTAB → rett linje + exit 42). `tests/test_win64_os.no`.
  Att i W1: emittere OSTAB + rutinane inn i `native_codegen_v2` `.text`/`.idata`, PE-emisjonsvegen
  (§2.1), Windows-`_start` (argv/envp), heap som bss-seksjon, GC-vaktside via `VirtualProtect`,
  og rute alle 58 `syscall`-stader gjennom `emit_os_call` i Windows-modus. Alt gata på
  `NC_TARGET=windows-x86_64` så Linux-output held seg byte-lik (harness under).
* **Verifikasjonsharness (ferdig).** `scratch/windows/`: `wineimg/` (Docker `nc-wine`),
  `wrun.sh` (køyr exe i wine), `cgbuild.sh` (rebygg codegen-ELF frå worktree-kjelde — reproduserer
  committa `native_codegen_x86_64.elf` byte-eksakt på ~6 s), `linuxlik.sh` (Linux-output for
  seks fiksturar × GC 0/1 + full nc_main-NCB) og `likcheck.sh` (byte-likskaps-diff mot committa
  codegen). `prog/` har fiksturane (hei, tekst, unnatak, tidrand, gcstress, argenv).
  `ref/SHA256SUMS` er referansen frå committa codegen.

## 4. Bygg- og verifikasjonsveg

1. Lokalt (Mac): `NORSCODE_ROOT=$PWD … ./bin/nc run` for Norscode-verktøy; codegen-ELF-bygg i
   Docker `nc-x86tools` med `bootstrap/stage0/norscode-linux-x86_64`.
2. Windows-output: `NC_TARGET=windows-x86_64 NORSCODE_GC_ALLOC=1 NC_INPUT=… NC_OUTPUT=….exe <codegen>`;
   køyr i `nc-wine`.
3. CI (W5+): jobb `windows-seed` — Linux-steg byggjer PE-en med kryss-codegen og lastar opp
   artefakt; `windows-latest`-steg køyrer `version`/`selftest`/fiksturar og til slutt
   attestasjonen. Utløysing med `workflow_dispatch` på greina.
