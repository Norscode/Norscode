# AOT-codegen feature-dekning (native_codegen_v2, x86-64)

Status for `selfhost/native_execution/native_codegen_v2.no` — self-hosta AOT-codegen som
byggjer native ELF-ar. Målet (Fase 3) er at han skal kunne kompilere sjølve KOMPILATOREN
→ seed, slik at seed-bygging kan byttast C→sjølvhosta.

Verifisert ~2026-09-02 ved å AOT-kompilere små feature-probar (`nc compile` → `nc ncb-to-elf`)
og køyre ELF-en under Docker `linux/amd64` (flagg AV = paritet, flagg PÅ = `NORSCODE_GC_ALLOC=1`).

## ✅ Verkar (grønt, flagg AV+PÅ)

- **GC / minne:** self-hosta mark-sweep-GC, TYPE-KOMPLETT for alle 5 heap-typar (int, streng,
  liste, map, bool). 10k-hmac-litmus grøn (gated i CI). Nøsta strukturar + djup rekursjon OK.
- **Kontrollflyt:** `hvis/ellers`, `mens`, `for kvar` (foreach), `match`/`case`/`_`, rekursjon.
- **Heiltal:** aritmetikk (+ - * / %), negative tal, bitops (& | ^ << >>), samanlikningar.
- **Boolsk:** `og`, `eller`, `ikkje`, `sann`/`usann`.
- **Strengar:** konkat, `tekst(heiltall)` (fiksa denne økta), split/upper/lower/slice/index_of.
- **Lister:** literal, `legg_til` (grow), indeksering, lengde.
- **Maps (ordbok):** literal, `map_set`/`map_get` (grow), streng- og liste-verdiar.
- **Importar (`bruk`):** verkar — 10k-litmusen importerer `std.sha256`.
- **Unnatak — `prøv`/`fang`/`kast` (LANDA denne økta):** full kryssfunksjon-handtering
  (handler-stakk i utvida mmap, TRY_BEGIN/TRY_END/LOAD_EXCEPTION + throw_unwind, setjmp/
  longjmp-stil). Verifisert: 400 kryssfunksjon-kast fanga, nøsta try, unntaksverdi, rethrow,
  alt under GC. **Catch-all** (typa `fang (e: Type)` er IKKJE brukt av kompilatoren).
- **Integrasjon:** mini-uttrykkstolk (rekursiv-nedstig parse + symboltabell-map + unnatak,
  2000 evalueringar under GC) kompilerer og køyrer korrekt — kompilator-like mønster verkar.

## ⚠️ Ikkje implementert — men IKKJE påkravd for å kompilere kompilatoren

1. **`endeleg` (finally).** Ikkje implementert (FINALLY_PUSH/RUN/END). Men kompilatoren
   brukar det IKKJE (einaste `endeleg` i kjelda er inne i ein feilmeldingsstreng, parser.no:1037).
2. **Flyttal-aritmetikk — `desimaltall` (float).** Ingen SSE/float-støtte. Men kompilatoren
   gjer INGEN float-aritmetikk: `desimaltall` er berre eit typenamn (semantic.no) + JSON-
   float-parsing (json.no) som ikkje vert køyrt under .no-kompilering. Trengst berre om ein
   vil køyre float-tunge brukarprogram native.

## ✅ JSON-serialisering LANDA (var seed-bygging-blokkar)

`builtin.json_stringify` (json_skriv) og `builtin.json_parse_raw` verkar no korrekt på
LISTER og MAP (inkl. djupt nøsta NCB-like `{"namn":..,"kode":[["PUSH_CONST",42],..]}`),
flagg AV+PÅ. Fiksa: (1) reimplementerte json_list_case/json_map_case som emitterte rekursive
atom; (2) LATENT pat_rcx-alloc-bug — patch_var_allocators gav buffer i rax for BÅDE pat_rax
og pat_rcx, men pat_rcx-sites (json_string_case) brukar rcx=buffer + rax=kjelde → eigen
repl_rcx (mov rcx,rax + mov rax,[rdi+8]). Same fiks løyste OGSÅ json_parse_raw under GC.
Dette låser opp NCB-serialiseringa i seed-pipeline (nc_main json_skriv/json_parse).

## ✅ JSON-escaping i AOT FIKSA (var rota til ELF stage-0-fixpunkt-divergens)

To uavhengige codegen-feil gjorde at NCB-ar skrivne av ein Gen1-ELF (og konstantar baka
inn frå seed-runtime) vart korrupte for strengar med bakstrek — Gen2-bundlen i fixpunktet
inneheldt `["PUSH_CONST","\"]` (ugyldig JSON) → «Source-only NCB differ»:

1. **`json_string_case`** var ein trampoline til den frosne legacy-rutinen @0x4028d2 som
   IKKJE escapa bakstrek (`"a\b"`→`"a"`, einsleg `\` → rått). Reimplementert som emittert
   atom (kropp etter `gc_alloc_var`, framover-jmp frå atomics-adressa; scratch via
   `gc_alloc_var`, resultat via `RT_STR_RAW`; escapar `" \ LF CR TAB`, rå kopi elles).
2. **Konstant-innbaking dobbeltdekoda** på stage-0-seed-runtime, der `json_parse_raw(fil_les)`
   ALT dekodar escapes (runtime-avhengig!) → `a\b` vart a+backspace. No SJØLVKALIBRERT: parse
   `["a\nb"]` éin gong → 3 teikn ⇒ parseren dekodar ⇒ hopp over eigen dekoding.

Sjølvsjekkande vakter i gc-litmus.yml: `gc_jsonesc_probe` (serialisering) og `gc_bs_probe`
(konstant-round-trip + runtime-bakstrek). Lærdom: alt som kryssar JSON-grensa i AOT-en
(NCB-skriving/-lesing) må verifiserast byte-eksakt mot VM-referansen — VM-en escapa rett
heile tida (`["a\\b","q\"r","n\nl","\\"]`).

## ✅ ELF stage-0-fixpunkt: committa fragment var MISKOMPILERTE (presedens `&` vs `+`)

Etter JSON-escaping-fiksen var Gen2 gyldig, men Gen1 ≠ Gen2 (988283 vs 1042140 B). Diff av
CI-artefaktet `norscode-linux-x86_64-source-only-fragments` mot `bootstrap/precompiled_fragments*`
(223 funksjonar): ALLE skil seg berre i `source_lines`/`source_columns` (metadata som blir baka
inn i ELF-en) — pluss ÉIN reell kodeforskjell: `ir_to_bytecode.json_escape_tegn` sitt
`(code & 15) + 1` var i dei committa fragmenta kompilert som `code & (15 + 1)` (PUSH 1, ADD, AND).
Den self-hosta parseren (Gen1) kompilerer parentesen rett (AND, PUSH 1, ADD). Dvs. kompilatoren som
laga dei committa fragmenta hadde ein presedensfeil; fixpunktet konvergerer ved å committe Gen2-
fragmenta (dei er sjølvkonsistente: Gen1' frå dei gir same Gen2). **Lærdom:** ved fixpunkt-drift,
diff fragmenta funksjon-for-funksjon FØR ein leitar i codegen — metadata-drift og éin miskompilering
såg ut som «strukturell divergens».

## ✅ `vent.sov` søv faktisk (var no-op i seed-runtime → test_vm_vent_sleep raud)

x86-64 v2 mangla `builtin.vent.sov` heilt (ukjend builtin → returnerte straks); ARM64-emitteren
var eksplisitt no-op på macOS. No: x86-64 → `nanosleep`(#35) med timespec på stakken (ms/1000,
(ms%1000)·1e6; negativt → 0; returnerer int 0); macOS-ARM64 → `poll(NULL, 0, ms)` (BSD #230), Linux-
ARM64 hadde alt nanosleep(#101). Vakter: `gc_sov_probe` (gc-litmus, Docker-verifisert 69 ms av 60)
og `arm64_sov`-fixture i test_arm64_ncval_machine. **Committed stage-0-seed søv framleis ikkje** før
han er regenerert — harnessen klassifiserer test_vm_vent_sleep som `native-unsupported` via ein
LEVANDE måling (sov(15) < 5 ms ⇒ hopp), så testen kjem automatisk tilbake med ein seed som søv.

## ✅ Fersk self-hosta x86-64-seed (2026-09-03): frå SIGSEGV til null krasj

`tools/build_x86_seed.no` (Docker linux/amd64, ~4 min) gjev ein STATISK ELF (3.7 MB; den
committa 10 MB-seeden lenkar libssl3). Full nc_main + heile VM-en (vm.no) køyrer no natively.
Defektar funne og fiksa i denne runden (kvar med sjølvsjekkande vakt i gc-litmus):

| # | Symptom i fersk seed | Rot | Fiks |
|---|---|---|---|
| 1 | SIGSEGV @0x4010ab i parser-feilmelding | `"b" + 7` / `any + any` gjekk rått til RT_CONCAT/RT_ADD | to_text på operandar (statisk + dynamisk) |
| 2 | RT_MAP_KEYS(0) SIGSEGV | `builtin.system_info` mangla | nativt uname/getcwd/readlink → map |
| 3 | RT_FEIL → RT_STR_RAW(0x401050) SIGSEGV | frosen RT_FEIL/RT_THROW brotne (literal overskriven av RT_BOOL-trampolinen) | INDEX_GET-grensesjekk-atom, feil-atom, emittert ufanga-kast-veg |
| 4 | TRY_BEGIN count=garbage / ret til stakkadresse | handler-region @0x10600000 overskriven av 2 GiB-bump-heapen (non-GC) | gc-gata region (0x7FF00000 non-GC) |
| 5 | stille tidleg retur / syscall m/ tilfeldig rax | safepoint-VA = «siste 93 byte av runtimeen» — feil når trampolinar er appenda | `atomics["gc_safepoint"]` registrert ved emittering |
| 6 | ret til stakkadresse i VM-init | `returner` inne i prøv {} lekte global handler-ramme | count lagra i botnslottet, restaurert (rcx) i epilog |
| 7 | policy nekta alt («manglar capability env.read») | ingen miljo_sett → NORSCODE_VM_TARGET_* tomme | env_set/env_get-overlay (@HEAP_VA+56) |
| 8 | fil_finnes sann for alt | frosen RT_FIL_FINNES + RT_FIL_LES returnerte i staden for å kaste | access(2)-atom + fil_les_safe som kastar |
| 9 | SIGSEGV på uendeleg rekursjon | inga djupnevakt | rsp-grense (initiell rsp − 7 MiB) i prologen → fangbart unntak |
| 10 | «path outside disk scope: /» | vm.no sjekka berre root av rot+relativ | effektiv sti (vm.no) |
| 11 | tekst(sann) == "" | to_text mangla bool | "sann"/"usann" |

Diagnose-metoden som verka: 30-linjers probe → `gc_probe_run.sh` + Docker; krasj-PC via
`qemu-x86_64 -g 1234` + `gdb-multiarch`, mappa med `.symbols`-sidecar (NC_NATIVE_SYMBOL_MAP);
break på throw_unwind-atomet (bytemønster) for å sjå kva som blir kasta og kvar det unwindar.
Attståande før promotering: harness-subset grønt på fersk seed, så committ som stage0 +
SHA256SUMS; deretter macOS-seed (Mach-O-emitter-tak) og binær NCB (JSON bort).

## ⚠️ Kjende separate frosne-runtime-bugs (IKKJE seed-blokkar)

Etter hovudblokkarane er tetta dukkar det opp smale frosne-runtime-edge-cases når codegen
kompilerer meir variert kode. Kjende:

1. **`builtin.slice(liste)` — FIKSA.** Rota var IKKJE RT_LIST_SLICE, men at slice-atomet
   dispatcha ALLTID til streng-slice → slice(liste) gav STRENG (type=tekst) → legg_til krasja.
   Fiks: type-sjekk ved slice-inngangen (cmp [rdi],3 → tail-call RT_LIST_SLICE @0x401f10).
   Verifisert flagg AV+PÅ. Løyste OGSÅ json_les-KRASJEN.
2. **`selfhost.json.json_les` fleir-element-liste** (låg prioritet, json.no-spesifikk, IKKJE
   seed-blokkar): json_les('42')→heltall OK, json_les('[42]')→[heltall] OK, MEN json_les(
   '[42,43]')→ FYRSTE element kjem ut `boolsk` (tomt), andre rett. Isolert: IKKJE generelt
   (enkel bool-så-int-i-liste-repro verkar); spesifikt for json_verdi si REKURSIVE parser
   med delt parser-state (map `t`) — fyrste list-element vert korrupt medan seinare element
   vert parsa. heltall(str), slice, legg_til verkar alle isolert. Seed brukar builtin
   json_parse_raw (verkar), ikkje json_les.

Mønster: dei fleste språktrekk verkar isolert, men spesifikke KOMBINASJONAR (slice→append,
json list/map) treff akkumulerte C-avleidde frosne-runtime-bugs. Full seed-bygging krev å
tette denne halen etter kvart som kompilator-modulane treff dei.

## (historikk) opphavleg frosen JSON-bug

`builtin.json_stringify` (json_skriv) og `json_parse`/`json_parse_raw` krasjar i native
codegen når input er ei LISTE eller MAP (skalar-verdiar verkar). Rota: dei frosne C-avleidde
JSON-rutinane (`json_list_case`→0x4029c3, `json_map_case`→0x4026ad) har ein lengd-bug —
rekursjonen reknar feil strenglengd → kopi-løkke med rax≈16 KB → segfault (krasj @0x4010ab
`mov (%r12,%rax),%cl`). Òg kjent i kjelda: nc_main.no:1613 «json_skriv mister data i komplekse
nøsta lister».

Dette BLOKKERER seed-bygging: NCB-serialisering brukar json_skriv/json_parse pervasivt
(nc_main.no:1614-1615, bundler.no:357/67), og sjølv binær-serde-ruta går via
`serde.serialiser(json_parse_raw(...))` (nc_main.no:1659) → treng json_parse.

**Fiks:** reimplementer `json_stringify` (list+map) og `json_parse` for lister/map — anten
som nye emitterte atom (rekursiv streng-bygging via RT_CONCAT + sjølv-rekursjon) eller ved å
rute til ein Norscode-implementasjon (native_codegen_v2 kompilerer alt rekursjon+map+streng
korrekt, jf. mini-tolk). STOR, men veldefinert.

## ⚠️ Main-flettings-gotcha (LØYST): cache-optimalisering vs GC-patchgate

Ved fletting av `main` (språkparitet #182) inn i GC-greina segfaulta 10k-litmusen (og hmac
for N > ~300) under GC, sjølv om paritet (flagg AV) var grøn. Rot: main sin cache-optimalisering
byter RT_INT/RT_BOOL/RT_STR_RAW sine INLINE bump-allokatorar med trampolinar (`movabs rax,
cached_va; jmp rax`) til nye runtime-blokkar APPENDA forbi `frozen_len`. Cache-FALLBACK-ane
(int utanfor [-32768,32767], fleir-byte str_raw) bump-allokerer rått via `movabs rcx,&bump` —
eit mønster `patch_bump_prologer`/`patch_var_allocators` IKKJE fangar, OG dei ligg forbi
frozen_len → dei blir ALDRI ruta til `gc_alloc16`. sha256 sine 32-bit-ord treff int-fallbacken
pervasivt → kvar allokering BUMPAR (ingen fri-liste-gjenbruk) → bump klatrar monotont (257MB @
3000 hmac, berre 125 levande) INN i GC-scratchen (0x10300000+) → mark/sweep skriv oppå levande
objekt → segfault. Diagnose: patch-teljar 9 (fletta) vs 11 (før). Dei 2 tapte = RT_INT@32 +
RT_BOOL@64. **Fiks (begge GC-gata, paritet BYTE-urørt):** (1) installer cache-trampolinane berre
når `gc_mode != "1"` → under GC held RT_INT/RT_BOOL/RT_STR_RAW dei patchbare inline-prologane;
(2) `HEAP_SZ` gc-gata (269484032 = 256MB+handler, scratch på toppen med naturleg bump-tak) under
GC, main sin 2GiB for paritet. **Lærdom:** GC-patchgaten krev at ALL allokering ligg i den
frosne, mønster-matchbare regionen; nye allokatorar (frå framtidige main-fletting) må anten
emitte den kanoniske bump-prologen ELLER gc-gatast av.

## Sekvens vidare (oppdatert)

Klarert denne økta: GC (type-komplett, gated), unnatak (prøv/fang/kast), tekst(heiltall),
**JSON-serialisering** (json_stringify + json_parse_raw for list/map — seed-blokkaren), og
slice(liste)-dispatch. Kompilator-like kode (mini-tolk) verkar. **main (#181–#184) er no fletta
inn** (PR #186) og cache-vs-GC-regresjonen over er løyst — heile GC-suiten + 10k-litmus grøn på
det fletta resultatet, paritet flagg-AV urørt.

NESTE: AOT-kompiler ein EKTE kompilator-modul / heile kompilatoren og iterér på gjenverande
smale frosne-runtime-edge-cases etter kvart som dei treffast (jf. json_les fleir-element over
— json.no-spesifikk, låg prioritet). Deretter: gjer GC standard av flagget → byt seed-bygging
C→sjølvhosta (Fase 4). Valfritt seinare: finally + float for brukarprogram.
