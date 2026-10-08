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
setsockopt 54, poll 7, ppoll 271, accept4 288, pipe2 293, wait4 61, spawn 4097, krypto 4098,
og mmap 9 / munmap 11 (skratsj for spawn/socket/argon2 og straum-lesaren i `fil_les`; krev
MINNE-kapabilitet). Aldri `brk` 12, `clone` 56, `futex` 202, `fork` 57, `execve` 59.

Dei fleste av desse går ikkje som statiske `syscall` men gjennom **sys6-porten**, som er sist i
runtimen: `cmp rax,imm32; je .gjer` per tillate nummer, deretter `mov rax,-38; ret`, og
`.gjer: syscall; ret`. Porten er den einaste `syscall` med nummer frå register. Tillate nummer
(`nos_sys6_tillatne`): 0 1 2 3 4 5 7 8 9 11 21 22 24 35 39 41–55 60 61 83 84 87 90 110 228 231
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
   `09b25595b34c1d65169bbafa2764baca544c4e4aba67c756c8138b2070855d48`. Krev godkjenning før
   installasjon. `test_native_codegen_srchash` er raud til dette er gjort.
2. **Seed** (`nc_main.no`-lukkinga): `std/native_gap.no`, `std/native_sys.no`,
   `selfhost/native_execution/codegen_prebuilt.no` endra → nc_main-NCB endra → seedane må
   reseedast for at `./bin/nc bygg-native --target norscodeos-x86_64` skal kjenne målet. Utan
   reseed fungerer målet med `NC_TARGET=norscodeos-x86_64` rett på codegen-ELF-en.
3. `windows-x86_64` og `linux-x86_64`: ingen funksjonell endring, men NCB-hash for
   nc_main endrar seg (ny `std.native_gap`-kode), so fikspunktportane må følgje seed-regen.
