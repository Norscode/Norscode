# Kompilatormål norscodeos-x86_64

Status: implementert i `native_codegen_v2.no` (spor X-A). Krev ombygd codegen-ELF og seed før
`nc bygg-native --target norscodeos-x86_64` verkar frå ein installert `nc` (sjå §8).

## 1. Kva målet er

Ein statisk ET_EXEC-ELF (x86-64, `.text` på 0x401000, verts-ABI-sida på 0x770000, `.data` på
0x700000, heap på 0x780000, `abs32`-adressering) som køyrer som ring-3-prosess på NorscodeOS
x86-64-kjerna (`~/OS-x86n`, X3). Same kodegen som `linux-x86_64`: same ELF-layout, same
runtime, same Linux-x86_64-syscall-ABI. Målet er ei **delmengd** av Linux-ABI-en pluss to private
syscall (4097 spawn, 4098 krypto). Vert for byggjing: Linux x86-64 (førehandsbygd codegen-ELF),
som for linux-x86_64.

## 2. Skilnad frå linux-x86_64

| Område | linux-x86_64 | norscodeos-x86_64 |
|---|---|---|
| Heap-segment (`p_memsz`) | 2 GiB (`HEAP_SZ`) | 64 MiB, `NC_HEAP_MIB=<4..2040>` ved bygging |
| GC-modus (`NORSCODE_GC_ALLOC=1`) | ja | nei (avvist ved bygging); bump-heap + `heap_merke`/`heap_sett` |
| Rekursjonsvakt | rsp − 7 MiB | rsp − 960 KiB, `NC_STAKK_KIB=<64..7168>`; fangbart «For djup rekursjon» |
| `builtin.sys6` | fri | berre i `std.*`-modular (kompileringsfeil i brukarkode) og gjennom ein nummer-port |
| `krypto_op(op, argp, iters)` | `NATIV-GAP` (fangbart unntak) | privat syscall 4098 |
| `heap_merke()`, `heap_sett(v)` | ja | ja (same på alle x86-mål) |
| `clone`, `futex` (trådar) | ekte syscall | `-ENOSYS` (`mov rax,-38`, 7 B) |
| `rename`, `symlink`, `lstat`, `uname`, `getcwd`, `readlink` | ekte syscall | `-ENOSYS`; `lstat` → `stat`; `system_info` gjev fast «NorscodeOS»/«x86_64»/«/» |
| `process_spawn_argv` | `pipe2`+`fork`+`execve`+`poll`+`wait4` | `pipe2`×3 + `spawn` (4097) + `poll`/`read` + `wait4` |

Linux-vegen er uendra: kvar endring ligg bak `ER_NOS()`/`HEAP_SZ_MÅL`, og `linux-x86_64`-ELF-ar for
fiksturane er byte-identiske med dei frå `origin/main` 079bfaf (verifisert, §7).

## 3. Syscall-mengd

Statiske syscall i `.text` (skanna med `tools/x86_64_syscall_skann.no`):

write 1, read 0, open 2, close 3, stat 4, lseek 8, access 21, unlink 87, rmdir 84, mkdir 83,
chmod 90, nanosleep 35, clock_gettime 228, getrandom 318, exit 60 — plus (i std-vegane)
socket 41, connect 42, accept 43, sendto 44, recvfrom 45, bind 49, listen 50, getsockname 51,
setsockopt 54, poll 7, ppoll 271, accept4 288, pipe2 293, wait4 61, kill 62, spawn 4097, krypto 4098,
og mmap 9 / munmap 11 (skratsj for spawn/socket/argon2 og straum-lesaren i `fil_les`; krev
MINNE-kapabilitet). Aldri `brk` 12, `clone` 56, `futex` 202, `fork` 57, `execve` 59.

Dei fleste av desse går ikkje som statiske `syscall` men gjennom **sys6-porten**, som er sist i
runtimen: `cmp rax,imm32; je .gjer` per tillate nummer, deretter `mov rax,-38; ret`, og
`.gjer: syscall; ret`. Porten er den einaste `syscall` med nummer frå register. Tillate nummer
(`nos_sys6_tillatne`): 0 1 2 3 4 5 7 8 9 11 21 22 24 35 39 41–55 60 61 62 83 84 87 90 110 228 231
271 288 293 318 4097. Utover lista over gjeld dette kjerne-delmengda frå OS-README (fstat 5,
pipe 22, sched_yield 24, getpid 39, getppid 110, exit_group 231) og dei resterande
sokkel-kalla (sendmsg/recvmsg/shutdown/getpeername/socketpair/getsockopt 46–48, 52, 53, 55).
4098 er **ikkje** i porten: `krypto_op` er einaste vegen til krypto-leafen (nøkkelisolasjon).

## 4. ABI for dei private syscall

**spawn (4097)**: `rdi` = peikar til NUL-terminert namn på ei initrd-/manifestoppføring (maks 63
teikn, utan `/initrd/`-prefikset), `rsi` = peikar til tre qword `{fd0, fd1, fd2}` i forelderen →
`rax` = pid eller −errno. Barnet får fd 0/1/2 frå kartet; forelderen lukkar barne-endane etterpå.
`wait4(pid, &status, 0, 0)` berre på eigne born; status er Linux-kodinga (`exit(k)` → `k<<8`,
drepen → signalnummer). Krev SPAWN-kapabilitet.

**krypto (4098)**: `mov rax,4098; mov rdi,op; mov rsi,argp; mov rdx,iters; syscall`. `rax` er
returverdien som heiltal (negativ = −errno). Kallstaden er (handskrive i
`tests/test_norscodeos_x86_64_encoder.no`): `31 D2 41 5B 4D 85 DB 74 04 49 8B 53 08` (rdx), same for
rsi (`…73 08`) og rdi (`…7B 08`), `48 C7 C0 02 10 00 00`, `0F 05`, `48 89 C7`, `call RT_INT`,
`50`. Klokketenestene (opcode 64–79) går same vegen. Mønsteret frå `~/OS-x86n/tls_ecdsa.no`
(«sann berre viss rv == 1 og out[1..7] == 0») verkar uendra, sidan retur og argument er likt arm64
(svc 402).

**Synkron spawn** (`std.native_gap._nos_spawn_synkron`, vald av `process_spawn_argv` når
`builtin.native_target() == "norscodeos-x86_64"`): `pipe2`×3, `spawn(namn, fd-kart)`, skriv stdin,
`poll` over stdout/stderr (fell tilbake til sekvensiell lesing om `poll` gjev −ENOSYS), `wait4`.
Avgrensingar: `args`, `cwd`, `environment` blir ikkje sende (spawn-ABI-en har dei ikkje);
`timeout_ms` blir ikkje handheva (ingen kill); `max_output_bytes` kuttar berre utdata; krev MINNE
(éin 16 KiB `mmap`-skratsj, gjenbrukt). Resultatet er `norscode-process-spawn-v1`-forma.

## 5. args() og miljo()

`builtin.argv_list_v1()` (liste av argv) og `builtin.argv_operation({"abi":"norscode-argv-v1",
"operation":"get"})` finst alt og les `argc/argv` frå verts-ABI-sida; `miljo_hent`/`miljo_finnes`
les `envp` ([0x780008]). Ingen nye namn er innførde; verifisert på målet (fiksturen
`spawn_args`). Ei oppramsing av heile miljøet finst ikkje.

## 6. Heap- og stakk-parametrar, sys6-val

- `NC_HEAP_MIB` (4..2040, standard 64): `p_memsz` til heap-segmentet. Handler-regionen ligg i
  øvste MiB. Manifestet bør seie kor mykje programmet treng; kjerna tildeler lat.
- `NC_STAKK_KIB` (64..7168, standard 960): rekursjonsvakta kastar «For djup rekursjon» (fangbart)
  før kjerna sin sidekvote/stakkregion tek prosessen.
- Heapen er ein bump-heap utan GC. Hald han flat med `heap_merke()`/`heap_sett(m)`: alt allokert
  etter merket er ugyldig etter `heap_sett`, so berre verdiar allokerte FØR merket kan overleve
  (legg arbeidet i ei eiga funksjon, sjå `tests/fixtures/nos_x86/heap_krypto.no`). Utan reset
  fyller ei lang løkke heapen (verifisert: exit 139).
- sys6: gjort til std-intern (ikkje fjerna), fordi `std.native_gap` (sokkel, spawn, fil-straum)
  treng han. Brukarkode får kompileringsfeil; porten avgrensar nummera ved køyring.

### Ingen kollektor på norscodeos-x86_64 utan NORSCODE_GC_ALLOC

Paritet-safepointet (GC-modus av) tel safepoint og kallar `gc_collect_native` når teljaren når
terskelen (2^31) og heapen har vakse minst éin port sidan sist. Kollektoren treng GC-metadata på
faste adresser (bitmap `0x40600000`, live-map, mark-stakk) som ligg langt utanfor det 64 MiB store
heap-segmentet. Kjerna mappar berre `PT_LOAD`, so første collect gav ring-3-#PF (cr2 `0x40600000`;
sett i ei HTTP-økt etter ~239 tunge økter). På norscodeos-x86_64 utan `NORSCODE_GC_ALLOC=1` er
safepoint-stubben difor ein no-op (`jmp .skip`); `NORSCODE_GC_ALLOC=1` er framleis avvist for målet.
Linux-x86_64 (2 GiB-segment) er uendra og samlar som før.

- `NC_SAFEPOINT_TERSKEL` (1..2147483647, standard 2147483647): overstyrer terskelen i paritet-
  safepointet ved bygging. Utan env er alle ELF-ar byte-identiske med før. Brukt av
  `tests/test_x86_safepoint_nos.no` (terskel 1000) for å utløyse kollektor-vegen utan 2^31 safepoint.

## 7. Verifikasjon (08.10.2026, Docker linux/amd64)

- Fiksturar (`tests/fixtures/nos_x86`, NCB frose ved 079bfaf utanom nye) bygde for begge mål og
  køyrde i Docker: `filio`, `kart`, `unntak` (exit 42), `heiltal` (11), `rekursjon` (fanga, exit 7),
  `spawn_args` (exit 42; spawn feilar reint med −38 under Linux fordi 4097 ikkje finst der),
  `heap_krypto` (arena-løkke på ~200 MiB i 64 MiB heap OK; `krypto_op` = −38 under Linux-kjerne).
- linux-x86_64-ELF-ar av fiksturane er byte-identiske med dei frå committa
  `bootstrap/native_codegen_x86_64.elf`.
- `tests/test_norscodeos_x86_64_encoder.no`: byte-eksakt (sys6-port, −ENOSYS, heap_bump/heap_set_bump,
  krypto_op-, heap_merke- og heap_sett-kallstad).
- `tests/test_linux_x86_64_norscodeos_syscall_skann.no`: syscall-skann, heap `p_memsz`, stakkvakt,
  parametrar, sys6-avvising. Hoppar over (tydeleg melding) når codegen-ELF-en ikkje kjenner målet.
- Ikkje verifisert: ekte kjerne (spawn-ABI, krypto 4098, poll/read/wait4-løkka, stakkvakt mot
  sidekvote). Det krev køyring på ~/OS-x86n.

## 8. Kva som krev reseed / ombygging

1. **Codegen-ELF** (`bootstrap/native_codegen_x86_64.elf` + `.srchash`/`.depshash` + pinne i
   `tools/active_surface_allowlist.txt`): `native_codegen_v2.no`, `heap_layout.no` endra. Bygg:
   `nc bundle` av `native_codegen_v2.no` → NCB, køyr gamal ELF med `NC_INPUT`/`NC_OUTPUT` → `cg1`,
   og `cg1` på same NCB → `cg2` (fikspunkt: sha like). Lokal sha256:
   `09b25595b34c1d65169bbafa2764baca544c4e4aba67c756c8138b2070855d48` (før rA-os; med rA-os sjå §9 og rapporten). Krev godkjenning før
   installasjon. `test_native_codegen_srchash` er raud til dette er gjort.
2. **Seed** (`nc_main.no`-lukkinga): `std/native_gap.no`, `std/native_sys.no`,
   `selfhost/native_execution/codegen_prebuilt.no` endra → nc_main-NCB endra → seedane må
   reseedast for at `./bin/nc bygg-native --target norscodeos-x86_64` skal kjenne målet. Utan
   reseed fungerer målet med `NC_TARGET=norscodeos-x86_64` rett på codegen-ELF-en.
3. `windows-x86_64` og `linux-x86_64`: ingen funksjonell endring, men NCB-hash for
   nc_main endrar seg (ny `std.native_gap`-kode), so fikspunktportane må følgje seed-regen.

## 9. rA-os: klokkefeil, ukjende kall, mono_ms, socket_prøv_*, prosess_start (12.10.2026)

**Punkt 6 — klokkefeil.** `tid_ms()` (inline) og `now_ms()` (atom) las `timespec` på stakken utan å
sjå på `rax` frå `clock_gettime`; utan KLOKKE-kapabilitet (seccomp/kjerne nektar) vart svaret
uinitialisert stakk (t.d. 140737488289536004). No: `rax < 0` → svaret er `−errno` (heiltal < 0),
same feilkontrakt som `random_byte()`. Epoke-millisekund er alltid ≥ 0, so `< 0` er eintydig feil.
Kartlegging av andre innebygde syscall-atom: `random_byte` (sjekka alt), `fil_finnes`/`fil_les` (`stat`
sjekkar `rax`), `system_info` (`uname`/`getcwd`/`readlink` skriv til nullfylt mmap-buffer, `readlink`
sjekkar `rax`; på norscodeos faste svar) — ingen andre las ein resultatbuffer utan `rax`-sjekk.

**Punkt 9 — ukjende kall.** På `norscodeos-x86_64` er eit innebygd kall utan rute i bunten
(`builtin.xyz` som endar i null-fallbacken) **kompileringsfeil** frå brukarkode:
`native codegen (norscodeos-x86_64): ukjent kall builtin.xyz/1 i funksjonen modul.funksjon (modul «…»)
har ingen rute i bunten …`. Kjeldelinje finst ikkje i NCB-en codegenen får; funksjonsnamn og modul gjer
det. `NC_UKJENT_KALL=null` gjev gammal åtferd (åtvaring + null-fallback); `NC_UKJENT_KALL=strikt` gjer
òg kall i `std.*`-modular til feil. Kall i `std.*` (i dag berre `builtin.desimaltall` i
`std.native_gap._jpr_tal`, som berre har rute når `std.desimaltall_tekst` er i bunten) held fram som
åtvaring + null, elles hadde kvart program med `std.native_gap` slutta å byggje. Dei andre måla
(`linux-x86_64`, `windows-x86_64`) er uendra: for 5 representative program er ELF-en byte-identisk
med ein codegen utan punkt 9 (skilnaden frå committa ELF er berre punkt 6, som endrar `now_ms`-atomet).

**Punkt 7 — `mono_ms()`** (`std.native_gap`, krev `bruk std.native_gap`): `clock_gettime(CLOCK_MONOTONIC=1)`
over sys6 i millisekund (linux-x86_64, norscodeos-x86_64, linux-arm64). Feil → `−errno`. Fell tilbake på
`tid_ms()` på VM (ingen sys6) og macOS (ingen syscall) — då veggklokke, ikkje strengt monoton. Berre
differansar har meining.

**Punkt 10 — `socket_prøv_recv(fd, maks)` / `socket_prøv_send(fd, data)` / `socket_prøv_aksepter(lyttar)`**
gjev ordbok `{"feil": 0|−errno, "data"|"n"|"fd": …}` i staden for å kaste. EOF = `feil 0`, `data ""`;
tidsavbrot = `feil −EAGAIN` (11). `socket_prøv_send` tek tekst eller liste av byte; `n` er byte sendt før
feilen. Dei gamle `socket_recv/send/accept` er uendra (kastar ved −errno).

**Punkt 8 — `prosess_start(namn, fd_kart)`** (pid eller −errno), **`prosess_vent_ikkje_blokkerande(pid)`**
(`wait4(WNOHANG)` → `{"ferdig": bool, "status": heiltal}`; status = exit-kode, eller 128+signal; wait4-feil
(t.d. ECHILD) → `ferdig=sann, status=−1, feil=−errno`), **`prosess_drep(pid)`** (`kill(pid, SIGKILL)`,
syscall 62; 0 eller −errno). `kill` (62) er lagt til i `nos_sys6_tillatne` (kjerna legg inn kill i X6a).
Skilnad mellom måla: på **norscodeos-x86_64** går `prosess_start` rett på `spawn` (4097) med namn på ei
initrd-oppføring (maks 63 teikn) og `fd_kart` som `{fd0, fd1, fd2}`; på **linux-x86_64** (og andre
fork-mål) er det `fork` + `dup2` + `execve(namn, [namn], arva envp)` — `namn` er ei filsti utan PATH-oppslag,
og den eksisterande `process_spawn`-motoren (`_pg_spawn`) er ikkje brukt fordi han lagar eigne røyr og
ikkje tek fd-kart. Ingen args/cwd/miljø (som spawn-ABI-en). Kallaren eig fd-ane. Berre AOT (sys6).

Verifikasjon: `tests/test_x86_ra_os.no` (Linux x86-64), `tests/test_r4_os_mono_ms.no` (VM),
`tests/test_norscodeos_x86_64_encoder.no` (byte-eksakt `now_ms`-feilsjekk, port med 62). Ikkje verifisert:
nekta `clock_gettime` med ekte seccomp (Docker på Apple Silicon køyrer amd64 under emulering, som ikkje
handhevar seccomp) — feilstia er i staden verifisert ved å byte-patche syscall-nummeret til eit ugyldig
(→ `−38`) i både `tid_ms` og `now_ms`; og `wait4(WNOHANG)`/`kill` mot ekte NorscodeOS-kjerne (berre
kompilering + port-innhald er sjekka).
