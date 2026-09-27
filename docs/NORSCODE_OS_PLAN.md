# Norscode OS — plan (x86-64 først, AArch64 seinare)

Status: K0 og K1 ferdige 2026-09-27 (sjå «Status K0/K1» under). Grunnlag: kartlegging av kjerne-PoC, x86-codegen, bundle/stage0,
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
| K2 | Maskinvarebibliotek og drivarar | `os/x86` (stubbar), 16550-seriell inn/ut, PIC-remap + PIT/LAPIC-timer via hendingsbuffer, PS/2-tastatur, CMOS-klokke, RDRAND, bootinfo (Multiboot mmap) → fysisk rammeallokator med frigjering, sidetabell-API (unnatak 0–31 → diagnose er alt gjort i K1) |
| K3 | Prosessar i ring 3 | ELF64-lastar, eige CR3, `kjør_brukar`-trampoline, Linux-syscall nivå 1, round-robin med timer-preemptering; to uendra Norscode hello-binærar køyrer samstundes |
| K4 | Filsystem | initramfs (arkiv bygt av tools/os_bygg.no) + VFS + nivå 2-syscalls; uendra Norscode-program les/skriv filer |
| K5 | Prosess-API og skal | fork/execve/wait4/pipe2/dup2/…; `os/brukar/init.no` + enkel Norscode-skal; **stage0 `nc run hello.no` inne i Norscode OS** |
| K6 | Lagring og nett | virtio-blk + vedvarande FS; virtio-net + TCP/IP i Norscode + nivå 4-sokkel; `nc`-HTTP-tenar svarar frå OS-et |
| K7 | CI | QEMU-boottestar i GitHub Actions (Linux-runner) via Norscode CI-adapteren |
| K8 | AArch64 | same kjerne på qemu-system-aarch64 -M virt (PL011, GIC, generic timer) etter at AArch64-GC er inne |

## Status K0/K1 (2026-09-27) — ferdige, med målte resultat

Filer: `os/boot/x86.no` (maskinkode-emittar: etikettar, rel/abs-fiksar, delsett av x86-64),
`os/boot/elf.no` (PT_LOAD-lesar), `os/boot/trampoline.no` (heile oppstartsbiletet),
`os/boot/byggjar.no` + `tools/os_bygg.no` (kjede), `tools/os_pakk.no` (ELF → bilete),
`os/boot/qemu.no` + `tools/os_qemu_test.no` (sele), `os/kjerne/{hei,k1_prøve,k1_feil}.no`,
`tests/test_os_k0_boot.no`, `tests/test_os_k1_ring0.no` (begge hoppar reint utan QEMU).

Bruk:

```
./bin/nc run tools/os_bygg.no                  # os/kjerne/hei.no → build/os/norscode_os.img (demand)
./bin/nc run tools/os_qemu_test.no             # -m 256M, krev «Hei frå Norscode OS» + exit 0
OS_KJERNE=os/kjerne/k1_prøve.no OS_BILETE=build/os/k1.img ./bin/nc run tools/os_bygg.no
OS_BILETE=build/os/k1.img OS_VENTA="K1-prøve: ferdig" ./bin/nc run tools/os_qemu_test.no
```

Målt (macOS arm64-vert, QEMU 11 TCG, `-cpu max`):
- K0: hei-ELF (155 KB) → bilete 237 KB; bootar på ~0,17 s; COM1 «Hei frå Norscode OS», QEMU-status 1.
  Ivrig modus (`OS_MODUS=ivrig`, identitetskart med 2 MiB-sider) krev `-m 2G`; demand held med 256M
  (3 sidefeil, 6 rammer).
- K1: k1_prøve-ELF (2,06 MB, med std.native_gap) bootar med `-m 256M` på ~8,7 s; seriell-utdata
  (546 B) er **byte-identiske** med same ELF i Docker linux/amd64, inkl. 7 GC-innsamlingar over
  >300 MiB søppel. 16 725 rammer (≈ 65 MiB) rørt → `-m 96M` held òg; `-m 48M` gjev
  «[kjerne] tomt for fysiske sider» og debug-exit 126. Null-peikar (k1_feil) gjev
  `[kjerne] unnatak v=0e … cr2=8` og debug-exit 125, ikkje trippelfeil.
- Byggjetid: compile ~1 s, codegen i Docker ~3–6 s, pakking ~1 s.

Avvik frå planen / val tekne under K0–K1 (og kvifor):
- **Biletet er ikkje «segment på sine VA-ar».** Codegen-ELF-en har segmenta side-justert og tett
  i fila, så heile ELF-fila blir lagd uendra inn i biletet, og sidetabellane (bygde av Norscode ved
  pakketid) mappar VA → fysisk. Berre siste, delvis fylte side per segment blir kopiert (4 sider),
  for resten av den sida må vere null (NCB-tilhengjet ligg rett etter heap-kontrollblokka i fila).
  Gjev 2,1 MB bilete for k1 mot ~7,5 MB flatt, og ingen store liste-kopiar i VM-en.
- **Adresserom:** 0x1000–0x3FFFFF identitet (side 0 umappa), 0x400000–0x7FFFFF ELF (4 KiB-sider),
  heap `[0x780000, 0x6B200000)` og programstakk `[0x6B800000, 0x6C000000)` demand-zero,
  direktekart av 0–4 GiB på 0x8000000000 (2 MiB-sider) for sidetabellar, rammer, mbi og HPET.
  Trampoline-stakkane (boot, shim, IST1–3) ligg i bss og blir nådde via direktekartet.
- **Unnatak 0–31 → diagnose** er alt gjort i K1 (IST-stakkar: #PF=IST1, #DF=IST2, resten IST3),
  ikkje K2. Diagnose og statistikk: COM1 for feil, port 0xE9 (`-debugcon`) for
  «[kjerne] exit=… rammer=… sidefeil=…» slik at COM1 er lik Linux-utdata.
- **Klokke:** monoton = HPET (periode frå GCAP_ID; TSC som reserve), realtid = CMOS ved boot
  (days_from_civil i maskinkode) + monoton; målt lik vertsklokka. `nanosleep` returnerer 0 straks
  (ingen timer før K2). `getrandom` = RDRAND, fail-closed (ENOSYS utan RDRAND → QEMU `-cpu max`,
  qemu64 manglar RDRAND).
- **Codegen på macOS går via Docker** (`nc-x86tools`, committa Linux-stage0 kopiert til
  `build/os/linux-dist` og montert som `/work/dist`), NCB-en blir kompilert lokalt. Utan Docker:
  lokal tolka codegen (rett, ~20 s for små program). På Linux x86-64: `bin/nc bygg-native` direkte.
- **Pakkinga køyrer i eigen prosess med `NORSCODE_VM_FAST=1`** (`tools/os_pakk.no`): i
  `tools/os_bygg.no`-prosessen (mange importerte modular, utan fast-modus) tok same arbeid 74–83 s,
  i barneprosessen ~1 s.
- Heapen må liggje under 2 GiB (GC-modus); ein ikkje-GC-ELF (2 GiB heap) blir avvist ved pakking
  med ei tydeleg melding.

Opne punkt etter K1:
- Alle ELF-sider er mappa RW (ingen W^X); literal-segmentet er skrivbart i motsetnad til Linux.
- Rammepoolen er ein bump-allokator frå den RAM-regionen som inneheld `pool_start` (avgrensa til
  4 GiB); ingen frigjering. Multiboot-infoen ligg i poolen og blir overskriven etter at `pool_init`
  har lese minnekartet (K2 må kopiere mbi først om ho trengst seinare).
- `raw_call`-stubbar, IRQ-ar, timer og ring 3 manglar (K2/K3). SYSCALL-shimmen er ikkje reentrant
  (éin global lagra rsp), og ein syscall-buffer som ikkje er mappa enno går gjennom #PF på IST1.
- Byggjekjeda brukar den committa Linux-stage0 i Docker (stale mot kjelda for store program).

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
