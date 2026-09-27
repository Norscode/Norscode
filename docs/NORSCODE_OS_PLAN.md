# Norscode OS — plan (x86-64 først, AArch64 seinare)

Status: K0–K3 ferdige 2026-09-27 (sjå «Status K0/K1», «Status K2» og «Status K3» under). Grunnlag: kartlegging av kjerne-PoC, x86-codegen, bundle/stage0,
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

Opne punkt etter K1 (oppdatert etter K2):
- Alle ELF-sider er mappa RW (ingen W^X); literal-segmentet er skrivbart i motsetnad til Linux.
- #PF-poolen er ein bump-allokator utan frigjering (heap-sider blir aldri gjevne tilbake).
  ~~Multiboot-infoen blir overskriven av poolen~~ — løyst i K2 (sjå under).
- Ring 3 manglar (K3). SYSCALL-shimmen er ikkje reentrant (éin global lagra rsp), og ein
  syscall-buffer som ikkje er mappa enno går gjennom #PF på IST1.
- Byggjekjeda brukar den committa Linux-stage0 i Docker (stale mot kjelda for store program).

## Status K2 (2026-09-27) — ferdig, med målte resultat

Nye filer: `os/x86/info.no` (kontrakt: info-blokk, stubb-indeksar, ringlayout — delt av
trampolinen ved byggjetid og kjernen ved køyretid), `os/x86/maskin.no` (primitiv),
`os/boot/stubbar.no` (maskinkode: 30 raw_call-stubbar, PIC-oppsett, IRQ-stubbar, ring),
`os/drivarar/{hendingar,pic,tidtakar,serie,tastatur,rtc}.no`, `os/minne/{rammer,sidetabell}.no`,
`os/kjerne/k2_prøve.no`, `tests/test_os_k2_drivarar.no`. Selen: `os/boot/qemu.no`
`køyr_med_inndata` og `OS_SERIE_INN`/`OS_TASTAR` i `tools/os_qemu_test.no`.

```
OS_KJERNE=os/kjerne/k2_prøve.no OS_BILETE=build/os/k2.img ./bin/nc run tools/os_bygg.no
OS_BILETE=build/os/k2.img OS_VENTA="K2-prøve: ferdig" OS_SERIE_INN='Hei fra verten over COM1\n' \
  OS_TASTAR="shift-n o r s c o d e spc shift-o shift-s spc k 2 backspace 2 shift-1 ret" \
  ./bin/nc run tools/os_qemu_test.no
```

Arkitektur (som planlagt: avbrot køyrer aldri Norscode):
- **Info-blokk** i trampolinen sitt identitetskarta dataområde; adressa kjem til kjernen som
  `NORSCODE_OS_INFO=<desimal>` i envp (miljo_hent verkar i AOT under shimmen). Blokka har
  magi/versjon, stubbtabell, ring, tikk/tikk_hz, kopi av minnekartet (≤ 32 postar), #PF-pool-
  felt, direktekart-base, argumentblokk, heap-/stakkvindauge, HPET-periode, mbi-slutt.
- **Stubbar** (maskinkode frå Norscode, kalla med `builtin.raw_call(adr, arg)`): in/out 8/16/32,
  MMIO 16/32 (VA = direktekart + fysisk), cli/sti/hlt, `vent_hending` (cli; tom ring → sti;hlt —
  sti-skuggen gjer sjekk+søvn atomisk), CR0/2/3/4 les/skriv, rdmsr/wrmsr, invlpg, rdtsc, cpuid,
  rdrand, nullstill_side, pool_avgrens, rflags. Fleire argument går via argumentblokka.
- **Avbrot:** 8259 remappa til 32–47 og alt maskert ved oppstart (IF=0 til kjernen kallar sti).
  IRQ-stubbar på IST4 (rører aldri programstakken): IRQ0 aukar `tikk` (postar ikkje — 100/s ville
  fylt ringen), IRQ1 postar scancode frå 0x60, IRQ4 postar kvar byte medan LSR.DR, andre postar
  vektoren; falske IRQ7/15 blir filtrerte via ISR. EOI til slave/master. Ringen: SPSC, 256 × 32 B
  {vektor, data, tsc, sekvens}, monotone indeksar, `tapte`-teljar ved full ring.
- **Drivarane er Norscode** og opnar sine eigne IRQ-ar: PIT kanal 0 (modus 3, 100 Hz) set
  `tikk_hz`; seriell (IER.RDA, MCR.OUT2, FCR RX-terskel 1); PS/2 sett 1 (i8042-omsetjing frå BIOS)
  → ASCII med shift/caps/Enter/Backspace; CMOS-RTC med UIP-venting, BCD/12 t og dobbel avlesing.
- **Shimmen:** nanosleep søv no verkeleg (frist = tikk + ceil(ns·hz/1e9), sti;hlt;cli-løkke)
  når `tikk_hz` ≠ 0, elles 0 straks som før. Monoton klokke: HPET → tikk → TSC.
- **Fysisk minne:** `pool_init` kopierer minnekartet inn i info-blokka og startar #PF-poolen etter
  både biletet+bss og alle Multiboot-strukturar (mbi, mmap, cmdline, lastarnamn; QEMU legg strengane
  rett etter bss_end) → poolen skriv aldri over Multiboot-data. Delinga med Norscode-allokatoren:
  `rammer.init(n)` kallar `pool_avgrens` éin gong og tek toppen av pool-regionen (stubben endrar
  berre `pool_slutt`, og kan ikkje sidefeile, så han er atomisk mot #PF-stubben som eig
  `pool_neste`). Bitmap (1 bit/ramme, 0 … høgaste brukande < 4 GiB) i dei første overtekne
  rammene, via direktekartet. alloc/alloc_null/fri med dobbel-fri-vern.
- **Sidetabell-API** (4 KiB) over CR3 via direktekartet, vindauge [2 GiB, 0x8000000000) — under
  2 GiB eig trampolinen identitets-/ELF-kart og demand-vindauga. Mellomtabellar frå `rammer`.
  `oversett` kjenner 2 MiB/1 GiB-sider.

Målt (QEMU 11 TCG, `-m 256M`, ~0,8 s utan GC-delen, ~11 s med):
`sov(100 ms): 10 tikk, 98 ms`; `rtc: 2026-09-27 20:44:08` (= vertsdato UTC); `rammer: 4094 frie
(15 MiB) frå 0xefe2000, pool_slutt=0xefe0000`; sidetabell på 0x7000000000 laga 2 mellomtabellar,
skriv via VA = les via direktekart; `serie: ekko «Hei fra verten over COM1»`;
`tastatur: «Norscode OS k2!»` (shift, backspace); `hendingar: tapte=0`; K1-søppellasta med
PIT på: `tot=102889` (same som utan avbrot), 8 GC-innsamlingar, 988 IRQ0 midt i Norscode-kode.
8/8 gjentekne køyringar grøne. K0/K1-testane er framleis grøne (K1 byte-lik Linux).

Avvik / val i K2:
- **Inndata frå selen:** `-serial mon:stdio`, stdin = seriell-tekst + Ctrl-A c + `sendkey …`-linjer.
  QEMU sin mux les stdin berre når 16550-en kan ta imot, men han har eit eige 32 B-buffer: målt
  gjekk linjeskiftet tapt i 2 av 4 køyringar (bytar i mux-bufferet blir ikkje leverte etter
  fokusbytet). Løysing: 64 «.» + LF før og 48 «.» (utan LF) etter teksten; gjesten ignorerer
  punktum. Kjernen opnar RX/tastatur og tier medan han ventar, så monitor-ekkoet (ANSI) ligg
  mellom linjene og blir filtrert bort (`serie`), rått i `stdout`.
- **Seriell mottak er ASCII** (andre bytar → «?»); tastaturet har amerikansk oppsett.
- **LAPIC/IOAPIC** er ikkje tekne i bruk (8259 held for éin CPU); **framebuffer** finst ikkje
  (feltet er 0). MMIO via direktekartet brukar WB-caching (ingen PAT/MTRR-oppsett).
- Kjernen køyrer med IF=1 etter `sti`; stubbane er usynlege for Norscode-koden (eigen IST-stakk).

Opne punkt etter K2:
- Tidtakaren postar ikkje i ringen; ei hendingsløkke må lese `tikk` sjølv (vent_hending vaknar på
  kvart tikk). Ingen one-shot-tidtakar/LAPIC-timer.
- `rammer` tek ein fast del av #PF-poolen ved init; heapen kan ikkje låne tilbake (OOM 126 om
  delen er for stor). Rammer ≥ 4 GiB blir ikkje brukte (direktekartet dekkjer 0–4 GiB).
- Sidetabell-API-et frigjer ikkje mellomtabellar, og har ingen TLB-shootdown (éin CPU).
- Ringen har ingen tidsstempel-kalibrering (TSC-verdiar er rå).

## Status K3 (2026-09-27) — ferdig, med målte resultat

Nye filer: `os/prosess/adresserom.no`, `os/prosess/prosess.no`, `os/brukar/*.no` (seks uendra
brukarprogram), `os/kjerne/k3_prøve.no`, `tests/test_os_k3_prosessar.no`. Maskinkode i
`os/boot/stubbar.no` (kjor_brukar, til_kjerne, brukar_felle, brukar_syscall) og
`os/boot/trampoline.no` (brukarsegment 0x2B/0x33 i GDT, EFER.NXE, CPL-sjekk i shim, #PF,
unnatak og IRQ0, Multiboot-modular). Byggjar: `bygg_brukar(kjelde, elf, gc)`. Sele:
`køyr_full(…, modular, …)` og `OS_MODULAR` i `tools/os_qemu_test.no`.

Korleis det verkar:
- Brukarprogram er Multiboot-modular (`-initrd "a.elf,b.elf"`). `pool_init` kopierer
  modultabellen til info-blokka og startar #PF-poolen etter moduldata og -strengar.
- Kvar prosess har eigen PML4. Delt og supervisor-only: PT0/PT1 (trampoline-området
  0x1000–0x3FFFFF, U=0 på PD-nivå) og PML4[1] (direktekartet). ELF-segment: W^X etter
  PT_LOAD-flagga (RX / R+NX / RW+NX). Heap- og stakkvindauge er demand-zero, mappa av
  Norscode-kjernen med rammer frå `os/minne/rammer`. Stakken er 8 MiB under 0x7F00000000.
- `kjor_brukar(pcb)` lagrar kjernekonteksten og FXSAVE64, byter CR3 og går til ring 3 med
  iretq. Fellene lagrar registra og FPU/SSE i PCB-en og returnerer til kjernen med ein grunn:
  syscall frå CPL3, unnatak (inkl. #PF) eller IRQ0 (føregriping, kvantum 1 tikk). Andre IRQ-ar
  under ring 3 blir posta i ringen, og så held prosessen fram.
- Syscall nivå 1 i Norscode: write(1/2), exit/exit_group, mprotect (alltid 0),
  clock_gettime, getrandom (RDRAND) og nanosleep (blokkerer, andre prosessar køyrer).
  Andre syscalls blir logga éin gong og gjev -ENOSYS. Brukarpeikarar blir validerte via
  prosessen sine sidetabellar (til stades, U, RW ved skriving) og lesne via direktekartet.
- Feil i ein prosess (null-peikar, vernebrot, supervisor-side, NX) drep berre prosessen:
  diagnose, exit 128 + signal, og rammene blir frigjorde.

Målt (QEMU 11 TCG, `-m 256M`, 6 modular, ~5,5 s): skrivar (ikkje-GC) exit 3, soppel (GC)
exit 5, og linjene deira er like same ELF i Docker. 7–8 skrivar-linjer kjem mellom soppel
sine framdriftslinjer, med ~430 føregripingar per køyring. nullpeikar, vern, kjerneles og nx
blir drepne med exit 139 og feilkodane 0x6, 0x7, 0x5 og 0x15. vern sin
`write(kjerneadresse)` gjev -14, som på Linux. soppel hadde 16 657 demand-sidefeil.
Ukjende syscalls: 0. Alle rammer var frie etter køyringa. K0–K3-testane er grøne (4/4).

Avvik frå planen:
- Det delte kjerneområdet i brukar-CR3 er trampoline-området på LÅGE adresser (under
  0x400000, der ET_EXEC-brukarprogram aldri ligg) pluss direktekartet. Dette er ikkje eit
  høgt område som planen skisserte.
- Retur til ring 3 skjer alltid med iretq, aldri sysret.
- Demand-zero for brukarprosessar går via kjernen i Norscode, ikkje via maskinkode.
- mprotect er ein konsekvent no-op, så GC-vaktsida blir ikkje handheva.
- NX-proben kan ikkje samanliknast med Docker: emuleringa der handhevar ikkje NX, og
  programmet heng. Proben blir berre sjekka i QEMU.

Opne punkt etter K3:
- Kjernen er ikkje-føregripande under syscall-handsaming. Det finst ingen prioritetar og
  ingen signal, og nanosleep skriv ikkje `rem`.
- Norscode-rammeallokatoren tek ein fast del på 128 MiB.
- Brukarprosessar får tomt miljø og berre AT_NULL i auxv.
- fork/execve, filer og røyr kjem i K4/K5.

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
