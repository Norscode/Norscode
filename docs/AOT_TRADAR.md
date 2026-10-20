# Trådar i AOT (F37) — grøne trådar, fase 1 (x86-64) og plan for fase 2

Status: fase 1 implementert og verifisert for `linux-x86_64` (og bygd/køyrd for `norscodeos-x86_64`
under Linux). arm64/macOS: uendra, `NATIV-GAP` (sjå under). Krev ny codegen-ELF (`bootstrap/native_codegen_x86_64.elf`
+ `.srchash`/`.depshash`) og seed-reseed for `nc_main`.

## 1. Kontrakt (som VM-en, selfhost/vm.no vm_green_*)

- `builtin.thread_spawn({"fn": "__main__.f", "args": {…}, "stack_bytes": …})` → `{"status": "started", "handle":
  "green-thread:N", "thread_id": "<2^30+N>"}`. `fn` blir kalla med éin parameter, `args`-ordboka. Ukjend `fn` →
  synkron `status: "error"` («ukjend trådfunksjon»). `thread.*`-primitiv (clone + futex) er uendra (berre linux).
- `builtin.thread_join({"handle": h, "timeout_ms": …})` → `finished` (+ `value` som tekst), `error` (+ `error`-tekst,
  også for unntak og stakkoverflyt i tråden) eller `timeout` (`timeout_ms` 0 = poll, < 0 = utan frist). Handtaket er
  konsumert etter join.
- `builtin.thread_sync`: `mutex_*` og `condition_*` byter til kooperativ venting (poll + yield) i staden for futex
  etter fyrste grøne spawn (og alltid på norscodeos, der futex er `-ENOSYS`). Ny operasjon `yield`. `cancel` på ein
  grøn tråd er berre eit flagg (som i VM). `thread_current_id()` gjev `2^30+N` i ein grøn tråd.
- `vent.sov(ms)` går via `std.native_gap._gr_sov` (codegen-omruting når `std.native_gap` er i bunten): søv
  kooperativt når andre grøne trådar finst, elles vanleg `nanosleep`.
- `builtin.thread_pool` svarer berre på `std.samtid.backend()`-proben; resten er `NATIV-GAP` (pool-tasks finst berre
  i VM). Dermed verkar `std.samtid` (lås, monitor, semafor, nedteljar, barriere, `tråd_start`) og `std.kanal`
  (blokkerande kanal) oppå trådane.

## 2. Mekanisme

- Kvar grøn tråd har ein eigen stakk (mmap, ≥ 512 KiB, rekursjonsvakt 64 KiB over botnen) med ei ctx-blokk på toppen:
  `+0` lagra rsp, `+8` handler-teljar, `+16` stakkgrense, `+24` skann-topp, `+32` inngangsfunksjon.
- Nytt atom `green_switch(fra, til)` (native_codegen_v2): pushar callee-saved, lagrar rsp/handler-teljar/
  `STACK_LIMIT`/`stack_base` i «fra», lastar «til» og returnerer der. Ny tråd får ei krafta fyrste ramme som
  returnerer til `green_boot`, som kallar `ncb_kall(std.native_gap._gr_inngang)` på den nye stakken.
  `green_boot_va` og `green_setup` er hjelparar. Atoma blir berre emitterte når `std.native_gap._gr_inngang` er i
  programmet (`green_i_bruk`); program utan `std.native_gap` gjev byte-identisk ELF (verifisert mot committa ELF).
- Unntak: handler-stakken er global; kvar tråd får eit eige intervall på 128 rammer øvst i regionen (maks 64
  samtidige trådar; hovudtråden kan bruke dei ~22 500 nedste). Teljaren blir bytt i `green_switch`. `_gr_inngang`
  fangar alt, so ingen unnatak forlèt ein trådstakk.
- GC: `gc_mark_roots` skannar i tillegg `[lagra rsp, skann-topp)` for kvar suspendert stakk (ctx-tabell i
  `GREEN_TAB`, ved `HANDLER_COUNT_VA + 24`); tråden som køyrer blir skanna som før (`stack_base` blir bytt). Ferdige
  trådar fjernar seg frå tabellen; stakken blir frigjort (munmap) av ein annan tråd.
- Planleggjar: ring av levande trådar, runde-robin. Ventande tråd: `_gr_vent_steg` (yield; ei heil runde utan
  framgang → 1 ms nanosleep, so frist og `thread.*`-primitiv kan gå vidare).

## 3. Avgrensingar (ærleg)

- Ingen preemption: ei tråd som løkkar utan å kalle join/sov/lås/yield svelt dei andre. Ingen ekte parallellisme.
- Programslutt: når `start()` returnerer, avsluttar prosessen utan å vente på grøne trådar (VM-en ventar på
  ikkje-daemon). Bruk `thread_join`.
- Blokkerande OS-kall (socket-read, `process_*`-venting, DNS, TLS) stoppar alle grøne trådar.
- Maks 64 samtidige grøne trådar; `cancel` er eit flagg; `stack_bytes` blir rundt opp til 512 KiB.
- norscodeos-x86_64: bygd og køyrd som Linux-ELF (mmap 9/munmap 11/nanosleep 35 er alle i sys6-porten); IKKJE køyrt
  på ekte NorscodeOS-kjerne. Ingen GC der (bump-heap): grøne trådar deler bump-heapen.
- arm64/macOS: ingen `green_switch`-atom; `thread_spawn` av brukarfunksjon gjev `NATIV-GAP`-feil (`native_target()`-
  vakt i `_gr_spawn`), `thread_pool` gjev `null` som før. Windows: atoma er stubbar, same vakt.

## 4. Fase 2 (plan, ikkje bygd): ekte parallelle trådar på linux-x86_64

1. `clone(CLONE_VM|FS|FILES|SIGHAND|THREAD|SETTLS)` per tråd (finst som `thread_spawn_native`), eigen stakk og
   eigen TLS-blokk (`ctx`-peikar i `fs`-base via `arch_prctl`) i staden for dei globale `GREEN_CUR`/handler-teljar/
   `STACK_LIMIT`/`stack_base`-slottane (alle blir per-tråd).
2. Delt heap krev trådsikker allokering: per-tråd bump-bitar (TLAB) frå den globale bumpen med `lock xadd`, fri-liste
   bak lås eller per-tråd.
3. Stopp-verda-GC: safepoint-flagg + futex-barriere; alle trådar parkerer ved neste safepoint (kallstad i
   `gc_safepoint`), kollektoren skannar kvar stakk (same ctx-tabell som no) og vekkjer trådane. Registerinnhald blir
   lagra på stakken av safepoint-stubben (som i dag).
4. Handler-regionen og modulglobalar: handler-stakk per tråd (TLS), globale `la`-slottar er delt minne (programmet
   må låse; atom `atomic_cas/xadd` finst).
5. Runtime-skriveporter (`skriv`, `json`-bufferar, int-/bool-cache, `LAMBDA_CAPTURE`) må vere per-tråd eller låste;
   `LAMBDA_CAPTURE_VA` er ei global som må bli TLS.
6. norscodeos-x86_64 har ikkje `clone`/`futex`; fase 2 er berre linux.

## 5. Verifikasjon

`tests/test_x86_trad.no` (Linux x86-64): `aot_x86_trad_basis`, `aot_x86_trad_gc` (utan og med
`NORSCODE_GC_ALLOC=1`), `aot_x86_trad_sync` for linux-x86_64, og basis + sync for norscodeos-x86_64 (køyrt som
Linux-ELF). Testen feilar med codegen-ELF-en som er committa før F37 (segfault: atoma manglar).
