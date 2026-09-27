# Norscode OS — plan (x86-64 først, AArch64 seinare)

Status: utkast 2026-09-27. Grunnlag: kartlegging av kjerne-PoC, x86-codegen, bundle/stage0,
native_sys, bootformat, minne/GC, historikk og AArch64 (8 kart + kritikar, målte eksperiment i QEMU 11).

## Mål

Eit operativsystem der **kjernen er skriven i Norscode** og kompilert av Norscode sin eigen
codegen, og der **brukarprogram er vanlege Norscode-program** (uendra AOT-binærar). Sluttmål:
`nc` (stage0) køyrer som prosess i Norscode OS og kan kompilere og køyre Norscode-program der.

## Arkitekturval (og kvifor)

1. **Kjernen er eit vanleg Norscode AOT-program (linux-x86_64 ELF i GC-modus) i ring 0.**
   Ingen codegen-endring: målt «nivå 0» — ein uendra native_codegen_v2-ELF køyrde korrekt i
   ring 0 bak eit lite lastar-/syscall-shim (inkl. 85 GC-innsamlingar identiske med Linux).
   Konsekvens: null stage0-/pin-churn i dei første milepælane.
2. **Oppstart og shim er maskinkode emittert av Norscode** (som atoma i codegen-en), ikkje C/asm:
   `os/boot/*.no` byggjer trampolinen (32→64-bit, sidetabellar, GDT/IDT/TSS, LSTAR-dispatch,
   demand-zero-sidefeil) som bytes. Bootformat: **Multiboot1 med a.out-kludge** (målt: QEMU
   `-kernel` lastar han; føretrekt framfor PVH når begge finst) — PVH-notat kan leggjast til seinare.
3. **Demand-zero-paging** i trampolinen: GC-heapen (memsz 0x6AA80000 ≈ 1,7 GiB ved faste VA-ar)
   blir ikkje lasta eller nullstilt; sidefeil i heap-/stakk-området får ei nullstilt fysisk side frå
   ein ramme-pool. Då held `-m 256M`–`512M` for små kjernar (målt RAM-golv utan dette: ~1,7 GiB).
4. **Syscall-shim med Linux x86-64-ABI.** Kjerne-programmet sine `syscall`-instruksjonar (38 i
   runtime-preludet) går til LSTAR i ring 0 (retur via `jmp rcx`/r11, ikkje sysret): write→COM1,
   exit→isa-debug-exit, mprotect→ok, clock_gettime→TSC/CMOS, getrandom→RDRAND, nanosleep→hlt-løkke,
   resten −ENOSYS. Seinare: same dispatcher kallar tilbake i Norscode-kjernen (sjå 6).
5. **Avbrot køyrer aldri Norscode-kode.** Allokator og GC er ikkje reentrante og det kjem ein
   safepoint etter kvart CALL (målt: asynkrone ISR-ar i Norscode korrumperer heapen). ISR-stubbar
   (maskinkode) kvitterer, legg vektor/data i ein låsefri ringbuffer i rå minne og returnerer.
   Norscode-kjernen er éin synkron hendingsløkke som tømer bufferen («bottom half»).
6. **Brukarprosessar = uendra Norscode Linux-binærar i ring 3, eige adresserom (CR3).** Både kjerne
   og brukar er lenka til same faste låge VA-ar (0x400000…0x6B200000), difor eige CR3 per prosess;
   trampoline, IDT/GDT/TSS og fysisk minne-vindauge ligg på høge VA-ar mappa supervisor-only i alle
   CR3. `kjør_brukar(prosess)` er for Norscode eit vanleg kall (raw_call) som returnerer når
   prosessen trappar (syscall/avbrot/unnatak) med årsakskode og lagra registerramme — kjernen
   handsamar og vel neste prosess. Éin kjernestakk, ingen reentrans, GC ser berre kjernestakken.
7. **Linux-personlegdom** for brukarprosessar i nivå (kartlagt i native-sys-target):
   1) write/exit/mprotect (+SSE/FXSAVE per prosess), 2) filsystem-syscalls (open…getdents64,
   uname/getcwd/readlink), 3) prosessar (mmap, pipe2, fork(COW)/execve, wait4, dup2, kill, ppoll,
   fcntl, chdir), 4) sokkel (loopback TCP/UDP først, epoll). Brukarland treng ikkje endrast.
8. **Maskinvareprimitiv via raw_call-stubbar** (målt: fungerer frå uendra codegen): port-I/O
   8/16/32, MMIO 16/32, cli/sti/hlt, lidt/lgdt/ltr, rdmsr/wrmsr, CR0/2/3/4, invlpg, rdtsc,
   rdrand, cpuid. Stubbane blir skrivne inn i trampolinen si kodeside ved oppstart.
9. **Kode-plassering:** `os/` (kjerne, boot, brukarland, bibliotek) og `tools/os_*.no` (bygg,
   QEMU-test). Ingenting her blir importert av nc_main eller codegen-lukkingane → ingen reseed.
   Kjerne-bilete blir bygde under `build/os/` (ikkje committa).

## Milepælar

| # | Namn | Akseptanse (automatisk, QEMU headless, seriell utdata) |
|---|------|------|
| K0 | Byggjekjede + testsele | `nc run tools/os_bygg.no` lagar `build/os/norscode_os.img` frå ein Norscode-kjerne; `nc run tools/os_qemu_test.no` bootar i qemu-system-x86_64, les COM1, avsluttar via isa-debug-exit, med tidsgrense og rydding; «Hei frå Norscode OS» over seriell |
| K1 | Ring-0-kjerne med syscall-shim + demand-zero | Kjerne-program med strengar, lister, kart, prøv/fang, rekursjon og >100 MiB søppel (GC-innsamlingar) gjev same utdata som på Linux; `-m 256M` held |
| K2 | Maskinvarebibliotek og drivarar | `os/x86` (stubbar), 16550-seriell inn/ut, PIC-remap + PIT/LAPIC-timer via hendingsbuffer, PS/2-tastatur, CMOS-klokke, RDRAND, bootinfo (Multiboot mmap) → fysisk rammeallokator, sidetabell-API; unnatak 0–31 gjev diagnose over seriell i staden for trippelfeil |
| K3 | Prosessar i ring 3 | ELF64-lastar, eige CR3, `kjør_brukar`-trampoline, Linux-syscall nivå 1, round-robin med timer-preemptering; to uendra Norscode hello-binærar køyrer samstundes |
| K4 | Filsystem | initramfs (arkiv bygt av tools/os_bygg.no) + VFS + nivå 2-syscalls; uendra Norscode-program les/skriv filer |
| K5 | Prosess-API og skal | fork/execve/wait4/pipe2/dup2/…; `os/brukar/init.no` + enkel Norscode-skal; **stage0 `nc run hello.no` inne i Norscode OS** |
| K6 | Lagring og nett | virtio-blk + vedvarande FS; virtio-net + TCP/IP i Norscode + nivå 4-sokkel; `nc`-HTTP-tenar svarar frå OS-et |
| K7 | CI | QEMU-boottestar i GitHub Actions (Linux-runner) via Norscode CI-adapteren |
| K8 | AArch64 | same kjerne på qemu-system-aarch64 -M virt (PL011, GIC, generic timer) etter at AArch64-GC er inne |

## Kjende hol og risikoar (frå kartlegginga)

- GC: over 8 388 608 live-map-oppføringar blir levande objekt frigjorde stille; heap-tomt mellom
  safepoints treffer guard-sida (SIGSEGV) i staden for gc_oom; OOM-sjekk etter innsamling er
  bump-basert (falsk OOM). Må rettast før kjernen stolar på GC over lang tid.
- Ingen safepoints på bakoverhopp: kall-frie løkker kan tømme heapen utan innsamling.
- `>>` er aritmetisk, ingen usignert aritmetikk: sidetabell-kode må maskere eksplisitt.
- Heiltal utanfor ±32767 allokerer (boksing): kjernekode i varme stiar skal ikkje vere i ISR.
- SSE brukt av desimaltal: CR4.OSFXSR/OSXMMEXCPT må vere på; FXSAVE per prosess.
- x86-stage0 er stale mot kjelda (ikkje OS-relevant, men noter).
- Ingen QEMU i CI i dag; CI-steg må vere argv-linjer via Norscode CI-adapteren.
- Feilsøking: QEMU gdbstub fungerer utan symbol; bruk NC_NATIVE_SYMBOL_MAP-liknande kart for x86.

## Reglar

AGENTS.md: ingen Python/C/asm-filer i aktiv flate (maskinkode blir emittert av Norscode);
`./bin/nc feature-check` på nye .no-filer; commit-meldingar på nynorsk utan AI-attribusjon.
QEMU er godteke plattformunntak (som docker/Pebble).
