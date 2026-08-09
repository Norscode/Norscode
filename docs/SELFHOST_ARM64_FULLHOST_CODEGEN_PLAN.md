# ARM64 full-host native codegen – design og fasa køyreplan

Mål: bringe AArch64-kodegeneratoren opp til **full-host runtime-paritet** med
x86-64-backenden, slik at Milepæl B2 («rein Linux ARM64-bygg utan GCC/OpenSSL»)
kan lukkast med ein FERSK ARM64-binær bygd av Norscode sin eigen native codegen
frå levande kjelde – ikkje `/usr/bin/gcc`, ikkje ein gammal committed seed.

Denne planen er den autoritative kjelda for det fleirøkta arbeidet. Kvar fase
har ein konkret **køyr-og-verifiser-port** på ekte maskinkode.

## Kvifor dette er den ekte B2-blokkeringa

Analyse 2026-08-08 (grein `krypto-tls-primitiver`):

- `selfhost/native_execution/native_codegen_v2.no` (3077 linjer) er ein
  **x86-64-berre** kodegenerator: han handemitterer x86-64 maskinkode for ein
  komplett NcVal-runtime (`rt_hex_del0..6`), Linux syscall prosess-ABI
  (`rt_hex_process_spawn_linux_x86_64`) og set `e_machine = 62` (EM_X86_64).
  Dette er kjernen i den GCC-frie x86-64 full-host-bygginga
  (`tools/build_linux_openssl_candidate_v3604.no`).
- AArch64-backenden finst, men er berre ein **liten heiltals-AOT** utan runtime:
  - `selfhost/native_execution/macho_arm64_codegen.no` (~550 linjer):
    representerer alle verdiar som rå 32-bit heiltal i register (w8–w11 stakk,
    w12–w17/w1+ lokale), maks stakkdjupn 4, maks 13 lokale. Strengar/lister/kart
    finst **berre** via constant-folding på compile-time (`fold_konstant_runtime`)
    – dei eksisterer aldri i runtime. Funksjonskall blir inlinede (ingen ekte
    kallstakk, inga rekursjon).
  - `selfhost/native_execution/elf_arm64_emitter.no`: pakkar éin flat RX-blokk i
    eitt PT_LOAD på 0x400000; `_start: bl main; mov x8,#93; svc #0`. Ingen
    skrivbart datasegment, ingen heap.
  - `selfhost/native_execution/macho_arm64_emitter.no`: Mach-O-motstykke med
    ad-hoc kodesignatur (køyrer på Apple Silicon).
- Difor kan `nc bygg-native --target linux-arm64` berre kompilere små
  heiltalsprogram (t.d. `tests/fixtures/aot_common_integer_ops.no` → exit 11).
  Han kan **ikkje** byggje full-host `nc_main` (strengar, lister, kart, fil-I/O,
  sockets, TLS). Difor brukar ARM64 TLS-attestasjonskandidaten framleis gcc på
  `build/v3009/native_candidate_gc.c` (C-backend som ein native ARM64-gcc
  kompilerer til AArch64).

Å berre fjerne gcc og peike på `elf_arm64_codegen` gjer bygget raudt – det
reproduserer exit-127-klassen som vart reverta 2026-08-08. Den einaste ekte
løysinga er å skrive AArch64-runtimen.

## Lokal verifikasjonssløyfe (Apple Silicon)

Verten her er **macOS ARM64** med ein køyrande `dist/norscode_native`. Same
instruksjonsflate (`kompiler_arm64`) matar både Mach-O- og ELF-emitteren, så
AArch64-maskinkode kan byggjast **og køyrast** lokalt via Mach-O, og
Linux-ELF-en arvar same instruksjonsstraum. Kontrakt:

```bash
NORSCODE_ROOT="$PWD" \
NORSCODE_VM_CAPABILITIES="env.read,env.write,process.exec,disk.read,disk.write" \
NORSCODE_VM_DISK_ROOT="$PWD,.,/tmp,/private/tmp" \
./bin/nc bygg-native --target macos-arm64 <kjelde.no> /tmp/ut
/tmp/ut ; echo "exit=$?"
```

Verifisert 2026-08-08: fixture over byggjer og køyrer med exit 11 (som forventa).
Linux-ELF-porten (`--target linux-arm64`) må til slutt køyrast på ekte Linux
ARM64-host/CI før promotering.

## Runtime-ABI som skal speglast (frå x86-64)

- **NcVal**: 16-byte heap-struct `{i64 type, i64 val}`. type-taggar identiske med
  x86-64 (0=nil, 1=int, 2=bool, 3=str, 4=list, 5=map, …). `val` er inline-heiltal
  for int/bool, elles peikar.
- **Heap**: eit skrivbart segment reservert med `mmap` (anonymt) ved
  `RT_INIT_HEAP`. Kanoniske cache for små heiltal (−32768..32767), bool og
  char (0..255) i starten av heapen, deretter monoton allokator. Same layout som
  `SMALL_INT_CACHE_BASE`/`BOOL_CACHE_BASE`/`CHAR_CACHE_BASE`.
- **Kallkonvensjon (AArch64)**: NcVal*-argument i x0..x7; retur i x0; stakk-VM
  held mellomverdiar på maskinstakken (sp) i staden for dei faste w8–w11.
  Runtime-hjelparane er posisjonsuavhengige og blir kalla med `bl` til faste
  offset i runtime-segmentet.
- **Syscalls (OS-spesifikt)** – einaste OS-avhengige delen:
  | op    | Linux aarch64 (`svc #0`, nr i x8) | macOS arm64 (`svc #0x80`, nr i x16) |
  |-------|-----------------------------------|-------------------------------------|
  | write | 64                                | 4                                   |
  | mmap  | 222                               | 197                                 |
  | exit  | 93                                | 1                                   |
  | read  | 63                                | 3                                   |
  | open  | 56 (openat)                       | 5                                   |
  | close | 57                                | 6                                   |
  Runtime-emitteren tek eit `os`-flagg og vel syscall-nummer/`svc`-immediat.

## Fasar (kvar med køyr-og-verifiser-port)

**Fase 0 – Grunnmur og verifikasjonssløyfe.** [FERDIG 2026-08-08]
Kartlagd backend, stadfesta lokal Mach-O build+run-sløyfe (exit 11).

**Fase 1 – Runtime `skriv` av heiltal + write-syscall.** [FERDIG 2026-08-08]
La til ein OS-parametrisert write-syscall (`emit_skriv_int`, macOS x16=4/svc #0x80,
Linux x8=64/svc #0) og desimalformatering, og `skriv(<heiltalsuttrykk>)` som ein
ekte runtime-operasjon (ikkje constant-fold). Verifisert på host: `42`, `7`,
`12345`, `-8`, `0` matchar VM-tolken byte-for-byte. Regresjon:
`tests/test_arm64_skriv_int_aot.no` (stdout `-123450`, exit 0).

**Fase 2 – mmap-heap + NcVal for heiltal/bool + kanoniske cache.**
[GRUNNMUR FERDIG 2026-08-08] `emit_heap_init` reserverer ein anonym mmap-heap
(macOS x16=197, Linux x8=222) med bump-peikar i callee-saved x28; NcVal er ei
16-byte {i64 type, i64 val}-struct (tagg 1=int). `emit_ncval_alloc_int` allokerer,
og NcVal-aritmetikk (last val → add → alloker resultat → pakk ut) er verifisert:
`phase2_ncval_probe` gjev exit 42 (20+22 over heap-NcVal) på ekte AArch64.
Regresjon: `tests/test_arm64_ncval_heap.no`.
**Stakk-VM-migreringa FERDIG 2026-08-08:** `kompiler_arm64_ncval` er den fulle
NcVal-maskina — operand-stakk i x19–x25, locals som NcVal* i heapen ved [x26+i*8],
bump i x28. PUSH_CONST/BINARY_*/UNARY_*/COMPARE_*/LOAD/STORE/LABEL/JUMP/
JUMP_IF_FALSE/RETURN + `skriv` (via `emit_skriv_ncval`) går alle over heap-NcVal.
Aritmetikk skjer i 32-bit w-register og blir pakka i ny NcVal, så exit-kodene er
identiske med den registerbaserte maskina. Verifisert på ekte AArch64 gjennom
`kompiler_ncb_ncval`: `aot_common_integer_ops`=11, `macos_aot_std_math`=49,
`macos_aot_std_math_control`=10, `macos_aot_control_flow`=17,
`macos_aot_function_call`=12, og `arm64_skriv_int`→`-123450`. Ingen fold: fold var
ei krykke for den runtime-lause maskina (og kalla `builtin.heiltall`, fråverande i
committed stage0). Dict-oppslag brukar `finnes_nøkkel` (committed stage0 gjev
ustabile ikkje-null-verdiar på absent-key `[key]`). Regresjon:
`tests/test_arm64_ncval_machine.no` (+ `test_arm64_ncval_heap.no`).

Att i Fase 2: kanonisk small-int/bool-cache. **Driver-byte er gata på Fase 3/4:**
`macos_aot_common_runtime`-fixturen brukar streng/liste/kart-konstantar som den
gamle maskina foldar på compile-time; å byte `bygg-native`-standarden til NcVal
før streng- (Fase 3) og liste/kart-runtime (Fase 4) finst, ville gjere den raud.
NcVal-vegen er difor forover-vegen (opt-in via `kompiler_ncb_ncval`) til Fase 3/4
gjer han til eit supersett av den gamle maskina.

**Fase 3 – Strengruntime.** [GRUNNMUR FERDIG 2026-08-08] NcVal-streng er
`{type=3, val→[i64 lengd][bytes]}` på heapen. `emit_ncval_str_const` materialiserer
strengkonstantar (movz/strb per byte, ASCII foreløpig), `emit_ncval_alloc_ptr`
boksar strdata-peikaren, og `emit_skriv_ncval` dispatchar no på tagg: type=3 →
skriv råbytes, elles desimalformater heiltal. Verifisert på ekte AArch64:
`skriv("Hei fra Norscode") skriv(42) skriv("!")` → `Hei fra Norscode42!`,
identisk med VM-tolken. Regresjon: `tests/fixtures/arm64_str_skriv.no` i
`test_arm64_ncval_machine`.
**Utvida 2026-08-08:** streng-`+`-konkatenering (`emit_str_concat`, runtime
type-dispatch: `ldr`-tagg på venstre operand → concat hvis type=3, elles
heiltals-add) og `lengde(streng)` er inne og verifiserte på ekte AArch64:
`skriv("Hei " + "verden") skriv("!") returner lengde("abcde") + (3+4)` →
`Hei verden!`, exit 12; heiltals-`+` er uregressert (`aot_common_integer_ops`=11).
Builtins som NcVal-maskina handterer (`skriv`, `lengde`) må også sleppast gjennom
`inline_kall` (elles gjer stage0-dict-quirken `funksjonar[absent]≠null` at dei blir
tolka som brukarfunksjonar). Regresjon: `tests/fixtures/arm64_str_concat.no`.

**Fase 3 FERDIG 2026-08-08:** heile strengruntimen er inne og verifisert på ekte
AArch64 mot VM-tolken. Emitterar: `emit_char_code`, `emit_chr`, `emit_slice`,
`emit_case` (lower/upper), `emit_str_eq` (streng-`==` via COMPARE_EQ-dispatch),
`emit_str_find` (index_of/contains), `emit_str_matchat` (starts_with/ends_with),
`emit_trim`, `emit_replace`, `emit_tekst_int` (int→str), `emit_heltall_str`
(str→int), delt `emit_alloc_str_copy`. Samla litmus (`arm64_str_ops.no`,
speglar strengdelane av `macos_aot_common_runtime`) gjev exit 16 = VM.
UTF-8 er dekt: VM-strengar er byte-baserte (`lengde("å")=2`,
`char_code(slice("å",0,1))=195`), så `emit_ncval_str_const` sin byte-for-byte-
ekstraksjon er allereie korrekt for ikkje-ASCII. Regresjon:
`tests/fixtures/arm64_str_ops.no` i `test_arm64_ncval_machine`.

**To subtile buggar fanga med otool-disassemblering:** (1) 32-bit `ldr w,[X,#8]`
må ha imm12=2 (`0xB9400800`), ikkje 1 (les offset 4 = øvre null-byte av type);
(2) `emit_ncval_alloc` klobbar x13/x14 som scratch, så resultat må flyttast til x9
før boksing (find/str_eq/matchat gav elles type-verdien i staden for resultatet).

Att i Fase 3: berre lister/kart (`macos_aot_common_runtime` brukar `[..]`/`{..}`)
høyrer til Fase 4 før den fixturen kan byggjast over NcVal og standardvegen byte.

**Fase 4 – Liste/kart-runtime.** [FERDIG 2026-08-08] NcVal-liste (type=4) =
`{val → [i64 count][elem ptrs]}`; NcVal-kart (type=5) = `{val → [count][key,val
ptrs]}` med lineært oppslag. Emitterar `emit_build_list`, `emit_build_map`,
`emit_map_get` (INDEX_GET-kart + finnes_nøkkel/har_nokkel), `emit_index_get`
(type-dispatch liste int-indeks / kart streng-nøkkel); `lengde` på liste/kart les
count-headeren gratis. Verifisert på ekte AArch64: `macos_aot_common_runtime`
(lister + kart + heile strengruntimen) → exit **25 = VM = gamal registermaskin**.
Regresjon: `macos_aot_common_runtime` i `test_arm64_ncval_machine`.

**Standardvegen bytt 2026-08-08:** `bygg-native --target macos-arm64`
(`macho_arm64_codegen.start` → `kompiler_ncb_ncval`) og `--target linux-arm64`
(`elf_arm64_codegen` → `kompiler_arm64_ncval`) brukar no den fulle NcVal-maskina.
Alle seks macOS-AOT-fixturane byggjer over NcVal med VM-rette exit-koder
(11/49/10/17/12/25), så dei eksisterande macho-AOT-testane held. Den runtime-lause
registermaskina (`kompiler_ncb`/`kompiler_arm64`/`kompiler_instruksjoner`) blir
berre halden for `test_macho_arm64_codegen`.

Att i Fase 4 (ikkje kravd av common_runtime): muterbar `legg_til`/INDEX_SET (treng
kapasitet/GC — fase-6-minneport), og INDEX_GET på streng (`s[i]`).

**Fase 5 – Ekte kall/rekursjon.** [FERDIG 2026-08-08] Inlining er erstatta av
ekte AArch64-kall. `ncval_compile_program` legg ut entry + alle nåbare
brukarfunksjonar (`ncval_reachable`, BFS) i eitt image og lenkar `bl`-kalla.
Kvar funksjon får `emit_fn_prologue`/`emit_fn_epilogue`: callee-saved x19–x26
(operand-stakk + locals-base) + x30 (lr) på maskinstakken, locals i heapen per
kall (x28 global), argument i x0–x7, resultat i x0 (entry pakkar ut til w0-exit).
RETURN kan stå kvar som helst (tidleg retur). Verifisert på ekte AArch64:
`arm64_factorial` (rekursiv `fakultet(5)`) → **120**; `macos_aot_function_call`
(`legg_saman(7,5)`) → 12; `aot_common_integer_ops` (bland/forkast som ekte kall)
→ 11. Ingen regresjon: alle seks macOS-AOT-fixturane + strengane held over den
nye multi-funksjons-standardvegen (49/10/17/25/16/12). `inline_kall` er berre
igjen for den gamle registermaskina (`test_macho_arm64_codegen`). Regresjon:
`arm64_factorial` + `macos_aot_function_call` i `test_arm64_ncval_machine`.

**Fase 6 – Fil-I/O + JSON + miljø.** [KOMPLETT 2026-08-08 — fil-I/O + miljø + full JSON]
`emit_fil_skriv`/`emit_fil_les`/`emit_fil_finnes` (OS-parametriserte syscalls:
macOS open/read/write/close/access 5/3/4/6/33 via svc #0x80; Linux openat/read/
write/close/faccessat 56/63/64/57/48 via svc #0; path null-termineres på
maskinstakken via `emit_cstr_to_sp`), og `emit_miljo_hent` (skannar envp i den
globale x27 — fanga frå x2 i entry-prologen på macOS). Verifisert på ekte AArch64:
fil-rundtur (skriv "Norscode fase 6", les tilbake, samanlikn, finnes ja/nei) →
`arm64_fileio` = 18; `miljo_hent(sett)`→len, `miljo_hent(usett)`→"". Retta ein
x27-klobb i `emit_heap_init` (gamal reserve som øydela envp). Regresjon:
`arm64_fileio` i `test_arm64_ncval_machine`. Ingen regresjon (integer_ops=11,
factorial=120 held).

**JSON (skalarar) FERDIG 2026-08-08:** `emit_json_stringify` gjer runtime
type-dispatch: int→desimal (`emit_tekst_int`), streng→sitatpakka `"…"`,
bool→`"true"`/`"false"`, elles (liste/kart/nil)→`""` som trygg reserve. For at
bool-literalar skulle gje `true`/`false` blei `PUSH_CONST boolsk` no boksa med
tagg=2 (same tagg som `COMPARE_*`-resultat) i staden for tagg=1 — heile
regresjonen held (integer_ops=11, common_runtime=25, str_ops=16, factorial=120,
fileio=18). Litmus `arm64_json_scalar` → stdout `42|"hei"|true|false`, exit 16.

**JSON (containerar + parse) FERDIG 2026-08-08 — FASE 6 KOMPLETT.**
`json_stringify` av liste/kart er no ei ekte rekursiv runtime-rutine
`__json_stringify` (bl-kalla, sjølv-rekursiv; sparar x19–x24+x30; x19=akkumulator,
x20=&element, x21=tal, x22=indeks; konkatenerer barn med `emit_str_concat` og
`emit_char_str`-skiljeteikn). `json_parse` er ei rekursiv-descent-rutine
`__json_parse` (peikar-kursor x19=cur/x20=end; skalar inline; liste/kart samlar
element-peikarar på ein 512-byte maskinstakk-buffer og kopierer til heap etterpå).
Begge blir append-a i `ncval_compile_program` berre om dei vert kalla, og deler
`fn_start`/`all_patchar`-bl-lenkinga; sjølv-bl-ane patcha lokalt (mål=offset 0).
`tekst()` blei gjort type-bevisst (`emit_tekst_ncval`: streng→identitet, elles
int→desimal) sidan `json_parse` gjev strengar. Litmus: `arm64_json_container` →
`[1,2,3]|{"a":1}|[true,false]|[[1,2],[3]]` (exit 37); `arm64_json_parse` →
strengar/bool/tal/nøsta liste+kart + stringify→parse-rundtur (exit 142). Heile
regresjonen held (11/49/25/120/12/18 + json 16/37/142, str_ops 16).

Kjende avgrensingar (eiga inkrement seinare om trong): `json_stringify`-strengar
escapar ikkje `"`/`\`; `json_parse` handterar ikkje escape-sekvensar og har
maks 64 slots per liste/kart-nivå; `null`→int 0. Dekkjer NCB-forma i praksis.

**Fase 7 – Prosess-/socket-ABI.** [SOCKET-ABI KOMPLETT 2026-08-08]
Åtte socket-builtins som OS-parametriserte syscall-emittarar: `socket_new`
(socket), `socket_bind` (setsockopt SO_REUSEADDR + bind), `socket_connect`
(connect), `socket_listen`, `socket_accept`, `socket_send` (write), `socket_recv`
(read → NcVal-streng på heapen), `socket_close`. Felles `emit_syscall(num, os)`
(macOS x16+svc#0x80 / Linux x8+svc#0) og `emit_sockaddr_in` (byggjer sockaddr_in
på maskinstakken: OS-spesifikk sin_len/family, port i big-endian, vert parsa som
punkt-desimal → 4 addr-byte). BSD-syscallnummer for macOS (socket 97, connect 98,
bind 104, listen 106, accept 30, read 3, write 4, close 6, setsockopt 105);
Linux-AArch64-nummer parametriserte (198/203/200/201/202/63/64/57/208). Verifisert
på ekte macOS-ARM64: `arm64_socket_loopback` — TCP-loopback i éin prosess
(bind+lytt, connect fullførast inn i lytte-køen, accept, send "hello!", recv) →
6 byte; SO_REUSEADDR gjer at umiddelbar re-køyring på same port også gjev 6 (ingen
TIME_WAIT-flakiness). D2/TLS byggjer på desse (std.socket → same builtins).

Att i Fase 7: **prosess-spawn** (fork/exec/wait) står att — ikkje kravd av
socket-/TLS-vegen mot B2; blir teke som eiga inkrement. Feilhandtering via
carry-flagget (macOS svc set carry ved feil) er ikkje implementert; happy-path
(fd/status ≥ 0) er dekt.

**Fase 8 – Full-host nc_main + TLS.** Kompiler heile levande `nc_main` til
AArch64-ELF; køyr `selftest` på ekte Linux ARM64. Koplar mot D2 socket-
integrasjon for rein TLS. Port: byte-identisk Gen1/Gen2 + selftest grøn på ekte
ARM64-host. **Dette lukkar B2.**

[PÅGÅR — måld gap-analyse 2026-08-08] Fase 8 er eit fleir-inkrement-steg: full
`nc_main` treng fleire opcodes utover Fase 0–7, og den endelege drop-gcc-
attestasjonen krev ekte Linux-ARM64-CI (denne verten er macOS-ARM64). Måld gap
(prøve `kompiler_ncb_ncval` mot representative program):
- **INDEX_SET** [FERDIG 2026-08-08]: `emit_index_set` — liste-mutasjon på plass +
  kart oppdater/vekst (realloc av mapdata med count+1 par). Litmus
  `arm64_index_set` = 241 (VM = AOT), inkl. nye kart-nøklar.
- **`legg_til`** (liste-append) [FERDIG 2026-08-08]: `emit_legg_til` — same
  realloc som kart-veksten (1 peikar per element). Litmus `arm64_legg_til` = 35
  (tom liste vaksen til 5 element i ei løkke, VM = AOT).
- **`TRY_BEGIN`/`TRY_END`/`THROW`/`LOAD_EXCEPTION`** (prøv/fang) [FERDIG
  2026-08-08]: setjmp/longjmp-stil handler-stakk (256 record × 80 byte, reservert
  framfor heapen; topp i den skjulte globalen `__exc_top__`, unnataksverdi i
  `__exc_val__`). TRY_BEGIN lagrar {catch-adresse (ADR), sp, x19–x24, x26}; THROW
  lagrar unnataket, les gjeldande handler (utan å poppe — catch-blokka sin TRY_END
  popper), gjenopprettar sp/x19–x24/x26 og `br` til catch. x28-heapen blir ikkje
  rulla (unnatak/allokeringar overlever). Litmus `arm64_try_catch` = 53 (djup
  rekursiv utrulling over 5 rammer + nøsta try + re-kast, VM = AOT).
- **`LOAD_GLOBAL`/`STORE_GLOBAL`** (modul-globalar) [FERDIG 2026-08-08]: x25
  reservert globalt som globals-base (operandstakken avgrensa til x19–x24, maks
  djupn 6). Entry-prologen set x25 = heap-start og reserverer N·8 byte (mmap-
  nullstilt) framfor heapen; slots tildelt i `ncval_compile_program` (forhandsskann
  av alle nåbare funksjonar). `ncb["module_initializers"]` blir kompilert og kalla
  (`bl`) frå entry rett etter prologen, så toppnivå-`STORE_GLOBAL` køyrer før
  entry-kroppen. `PUSH_CONST null` (type "ingenting") → int 0. Litmus
  `arm64_globals` = 109 (global teller/sum mutert på tvers av kall, VM = AOT).
**Alle fire kjende opcode-gap er no lukka** (INDEX_SET, `legg_til`, modul-globalar,
prøv/fang) — alle verifiserte på ekte macOS-ARM64. MEN prøvekøyring av ekte NCB-ar
gjennom codegen viser at det står att ein **builtin-hale**: `ncval_compile_program`
kjem gjennom alle opcodes, men stoppar på builtins som ikkje er implementerte enno
(t.d. `builtin.system_info` i auth_mfa_diag; helpdesk stoppa berre på eit
u-bundla modulkall `std.lagring.last`). nc_main brukar truleg mange fleire
builtins (random_hex, split, system_info, …) som må portast. Att i Fase 8:
- **Builtin-hale** [PÅBEGYNT 2026-08-08]: målt frekvens over 385 førbygde NCB-ar
  (327 distinkte u-implementerte builtins, tungt front-lasta). Fyrste batch
  lukka + verifisert (`arm64_builtins_tail` = 26, VM = AOT): `tekst_fra_heltall`/
  `tekst_fra_heiltall`/`til_tekst` (→ emit_tekst_ncval), `heltall_fra_tekst`/
  `heiltall_fra_tekst` (→ emit_heltall_str), `builtin.type` (køyretids typenamn;
  kart heiter **"ordbok"**, ikkje "kart"; liste er generisk "liste"), `nøkler`
  (kart→liste). Dette er dei mest brukte (≈44k+28k+7k+4.5k treff). Batch 2
  (2026-08-08): `verdier` (kart→verdiar) + `fjern_siste` (pop siste, muter lista)
  — litmus `arm64_builtins_tail2` = 96 (VM = AOT). Att: split, join, fjern_nokkel,
  miljo_sett, tid_ms, json_parse_raw, random_hex, sha256, system_info,
  process_spawn_argv, network_operation m.fl. **`split` FERDIG** (2026-08-09):
  runtime-rutine `__split` (bl-kalla; x19–x24 for løkke-tilstand, delstreng-
  peikarar på 512-byte maskinstakk-buffer, generell fleirteikns-delim via
  byte-samanlikning; kopiert til heap-listdata) — litmus `arm64_split` = 12
  (VM = AOT), både éin- og fleirteikns-delim. **`join` FERDIG** (2026-08-09): runtime-rutine `__join` (akkumulator x19 +
  `emit_str_concat` i løkke) — litmus `arm64_join` = 13 (VM = AOT). **`fjern_nokkel`/`json_parse_raw` FERDIG** (2026-08-09,
  arm64_fjern_nokkel=30): fjern_nokkel = skann+skyv par ned+dekrementer count;
  json_parse_raw = alias til `__json_parse`. `miljo_sett`/`tid_ms` m.fl. står att.

  **STOR ARKITEKTUR-GRENSE (funne 2026-08-09):** operand-stakken er REGISTER-basert
  (x19–x24, maks djupn 6 sidan x25 er globals-base). Difor feilar **kart-literalar
  >3 par (treng 8 operandar) og liste-literalar >6 element** å kompilere
  («operand-stakk full»). Full `nc_main` brukar store literalar, så dette treng
  **operand-spilling til minne** (spill til stakk-/heap-slot når djupn>N) — eit
  substansielt eige steg, truleg den største attståande blokkeringa (større enn
  einskild-builtins). NB: mislukka bygg-native gjev ingen binær → bash exit 127.
- **Køyr full `nc_main` gjennom codegen** for å avdekkje restgap (LOAD_FIELD/
  STORE_FIELD, SWAP/DUP/OVER, fleire builtins nc_main brukar). NB: å kompilere
  heile `nc_main` (3458 liner) til NCB på denne verten blei SIGKILL-a (compile=137,
  ressurspress) — steget bør køyrast på ein maskin med meir minne / i CI.
- **Linux-ARM64-CI**: den endelege `selftest`-attestasjonen som slepper
  `/usr/bin/gcc` KAN IKKJE køyrast på denne macOS-ARM64-verten — han krev ekte
  ARM64-Linux-CI. Dette er det ytre steget som formelt lukkar B2.
Codegen-arbeidet (alle opcodes) er gjort; det som står att er full-`nc_main`-
gjennomkøyring (ressurskrevjande) og den eksterne Linux-CI-attestasjonen.

## Kopling til Milepæl B / D2

- B2 kan først lukkast etter Fase 8. Fram til då står den verifiserte GCC/C-
  overgangen i `tools/build_linux_arm64_tls_attestation_candidate.no` som
  dokumentert mellombels bru.
- Fase 7–8 føreset D2 socket-integrasjon (rein Norscode-TLS over native
  socket-ABI). Sjå [SELVSTENDIGHET_SLUTTPLAN.md](SELVSTENDIGHET_SLUTTPLAN.md) D2.
