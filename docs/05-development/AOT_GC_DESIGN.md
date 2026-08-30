# AOT-runtime GC — design (native_codegen_v2)

**Mål:** gje den sjølvhosta AOT-runtimen ein reell garbage collector, slik at
allokeringstunge Norscode-program (pure crypto, kompilatoren på store kodebasar,
hosting-servarar, full testflate) ikkje sprenger heapen. Dette er rot-blokkaren
for å promotere ferske seedar og attvinne den grøne tilstanden frå PR #179 (som
var grøn fordi seedane då vart bygde med C-verktøykjeda sin GC).

## Kvifor no

`native_codegen_v2` sin runtime er ein rein **bump-allokator utan reclaim**.
Empirisk: ~10000 pure `hmac_sha256_bytes`-kall (kvar ~100–200KB Norscode-lister/
map) sprenger 2GB-heapen → SIGSEGV. Committa seed (C-GC) toler det. Utan GC kan
ingen allokeringstung sjølvhosta seed passere full testflate.

## Runtime-arkitektur (funn frå scoping 2026-08-30)

- **Heap** frå `HEAP_VA=0x600000`: 64B kontrollblokk (bump-ptr @0, envp @8,
  GC-flagg @32), så immutable cachar (small-int −32768..32767 @ +64, 2 bool,
  256 char), så dynamisk bump-region frå `HEAP_ALLOC_START` (~0x702060).
  ELF-programhovudet mapper heile 2GB (`HEAP_SZ`) som BSS (fysisk on-demand).
- **NcVal-boks = 16B**: `[type:8][payload:8]`. type 1=int, 2=streng, 3=liste,
  4=map, 5=bool. int/bool: payload = verdi. streng: payload → `[len:8][bytes]`.
  liste/map: payload → variabel struktur.
- **Allokering er DESENTRALISERT** og inline i den handassemblerte `rt_hex_del0..6`-
  bloben: kvar RT-rutine (RT_INT, RT_STR_RAW, RT_MAP_NEW, RT_LIST_APP, RT_CONCAT,
  RT_SLICE, …) gjer sin eigen bump: `mov rax,[0x600000]; lea rcx,[rax+N];
  mov [0x600000],rcx`. **Ingen objekt-header, inga størrelse-info.**
- **Append-mekanisme**: nye runtime-rutiner (t.d. `mkdir_p`, atomics, safepoint)
  vert emittert som maskinkode-byte-array ETTER bloben via `copy(bytes,[…])` +
  `atomics["namn"]=TEXT_VA()+lengde(bytes)`. Frå Norscode, men er maskinkode.
- **Safepoint** (`GC_SAFEPOINT_STUB_BYTES=24`): berre `mov rax,[HEAP+32];
  inc rax; mov [HEAP+32],rax; ret` — tel eit tal, aldri sjekka. GC hookar her.
  Kalla ved loop-back/CALL i codegen (linje ~2266/2291/2298).
- **Rot-settet er tolkaren sin objektgraf**: seeden KØYRER VM-en; heile VM-
  tilstanden (functions-map, frames, operand-stackar, alle tolka verdiar) er
  NcVal-objekt på AOT-heapen. GC må trace frå roter gjennom HEILE denne grafen.

## Arkitektur-avgjerd: høg-nivå Norscode-GC via små råminne-primitiv

Runtimen er maskinkode, men GC-**logikken** treng ikkje vere det. Plan:

1. **Små maskinkode-primitiv** (appenda, ~5–10B kvar, trivielle):
   - `raw_load64(addr) -> int`  (`mov rax,[rdi]; ret`, boksa via RT_INT)
   - `raw_store64(addr, val)`   (`mov [rdi],rsi; ret`)
   - `heap_bump() -> int`       (les `[0x600000]`)
   - `heap_alloc_start() -> int`
   - `stack_base()/stack_ptr()` for konservativ rot-skanning
   - `gc_mark_region_ptr()` — peikar til ein reservert mark-bitmap-region
     (utanfor bump-heapen, så GC ikkje allokerer medan han samlar).
2. **GC-logikken i HØG-NIVÅ NORSCODE** (`selfhost/native_execution/gc.no` e.l.),
   som brukar primitiva. Maintainable, portabel x86/arm64, eig runtimen i språket.
   GC-funksjonen må vere **allokeringsfri** (bruk råminne-buffer til mark-stack/
   bitmap), elles chicken-and-egg på heapen han samlar.

## KRITISK avgrensing: boksa verdi-modell vs allokeringsfri GC

Norscode-verdiar er **boksa NcVal-ar**. Å boksa ei heap-adresse (stor int >
32767) ALLOKERER ein 16B-boks. Ein GC skriven i rein høg-nivå Norscode ville
difor allokere på heapen han samlar → chicken-and-egg, veks heapen UNDER GC.
Konsekvens — eitt av:
- **(a) Hand-emittert maskinkode-GC** (jobbar med rå register/minne, ingen
  NcVal-boksing). Mest robust, allokeringsfri, men maskinkode × 2 arkitekturar.
- **(b) GC-scratch-arena**: dedikert bump-region (utanfor hovudheapen) som GC-
  interne allokeringar brukar medan GC køyrer, nullstilt per GC. Krev alloc-mode-
  bryter (GC-aktiv → alloker i scratch). Lèt GC-LOGIKKEN vere høg-nivå Norscode.
- **(c) Rå-int-type**: unboksa heiltals-handtak for adresser (utanfor NcVal-
  modellen). Større språk-/runtime-endring.

**Tilråding:** start med (a) for kjerne-mark/sweep-løkka (liten, allokeringsfri,
robust), eller (b) om ein vil halde logikken i Norscode. (a) er tryggast for
korrektheit; (b) er best for langsiktig vedlikehald/sjølvstende. Avgjer i F1.

## Eksakte objekt-layout (empirisk via obj_addr/raw_load64, 2026-08-30)

Alle heap-objekt er anten ein 16B NcVal-boks eller ein payload-blokk. Boksen er
`[type:8][payload:8]`. Verifisert layout per type:
- **int (1) / bool (5):** boks `[type][verdi]`, 16B, INGEN peikarar. Små int
  (−32768..32767) bur i immutable-cachen (SMALL_INT_CACHE_BASE) → aldri allokert,
  aldri samla. Store int allokerer boks.
- **streng (2):** boks `[2][payload→]` (16B) + payload `[len:8][bytes, padda]`.
  Peikarfelt: box+8. Payload har INGEN NcVal-peikarar (rå byte).
- **liste (3):** boks `[3][payload→]` (16B) + payload `[len:8][cap:8][elem_ptr…]`.
  Peikarar å trace: box+8 (→payload), og payload+16 .. +16+len·8 (elementa er
  NcVal-peikarar; små int → cache, større → heap).
- **map (4):** boks `[4][payload→]` (16B) + payload `[len:8][cap:8][key,val-par…]`.
  Peikarar: box+8, og payload+16 .. +16+len·16 (nøkkel- og verdi-peikarar).

Heapen er ein monoton bump-straum av desse allokeringane (payload ofte allokert
FØR sin boks). MARK-fasen har no full peikarfelt-kart. SWEEP treng framleis
objekt-grenser → header (sjå under), sidan `[len]`-ord (1–5) er tvetydig mot
box-type-tag (1–5) ved lineær walk.

## Objekt-header (påkravd for sweep)

Sweep må enumerere alle objekt. Utan header er lineær heap-walk tvetydig (boks
type-tag 1–5 vs payload-len 1–5 kolliderer). Val:
- **A (vald): re-pek dei allokerande RT_*-rutinene til nye appenda header-
  leggjande versjonar.** Kvar allokering: bump med `(8 + storleik)`, skriv
  `[size:8]` (+ mark-bit i høg-bit eller sidekart), returner boks etter headeren.
  All allokering går då gjennom våre rutiner; frosen rt_hex-bump vert forbigått.
  Meir arbeid, men NYE rutiner (ikkje redigering av hex).
- B (forkasta): sjølv-beskrivande walk — krev regulær, eintydig layout per type;
  boks/payload-interleaving gjer det tvetydig.

## Mark-sweep-algoritme (stop-the-world, konservativ rot)

1. **Trigger**: safepoint sjekkar `bump - alloc_start > terskel` → kall gc().
2. **Mark**: konservativ rot-skann (native stack `stack_ptr..stack_base` +
   global_vas). For kvar 8-byte-slot som ser ut som ein heap-peikar
   (`alloc_start ≤ v < bump`), push til mark-stack. Trace: for kvart markert
   objekt, les type-tag; for streng ingen barn; for liste/map følg element-/
   nøkkel-/verdi-peikarane (les layout via råminne). Sett mark-bit i sidekart.
   Immutable cachar (small-int/bool/char) er alltid levande → aldri samla.
3. **Sweep**: walk heapen via `[size]`-headrar frå alloc_start til bump. Umarkerte
   → legg på fri-liste (adresse+storleik), coalesce nabo-blokker. Nullstill
   mark-bits.
4. **Alloc etter GC**: header-allokatoren prøver fri-lista først (first/best-fit),
   elles bump. Konservativ GC kan IKKJE flytte objekt (kan ikkje trygt skrive om
   ein maybe-peikar) → mark-sweep, ikkje copying.

## Fasar (multi-sesjon)

- **F1 (foundation): LANDA** (commit 3862a46). råminne-primitiv raw_load64/
  raw_store64/heap_bump/heap_alloc_start (appenda). Norscode les heapen.
- **F2-investigering: LANDA** (commit f75336d). obj_addr-primitiv + eksakt
  objekt-layout kartlagt empirisk (sjå over).
- **F3 mark-LOGIKK: VALIDERT** (`selfhost/native_execution/gc.no`, gc_trace).
  Graf-traversering per layout verifisert: `[[1,2],[3,4],"hello"]`→7 (streng-
  literal-payload i .data korrekt ekskludert), `map{a,b}`→8. Retter største
  konseptuelle risiko (er layout-modellen rett?). Referanse-versjonen ALLOKERER
  (Norscode-ordbok som besøkt-sett) → må gjerast allokeringsfri (rå-bitmap) for
  produksjon. ATT i F3: konservativ rot-skann (stack+globals → treng stack-
  primitiv) + allokeringsfri besøkt-bitmap.
- **F2:** header-leggjande allokatorar (re-pek RT_INT/STR_RAW/MAP_NEW/LIST_APP/
  CONCAT/SLICE/…). Verifiser at seed framleis byggjer + køyrer basis.
- **F3:** konservativ mark i Norscode. Verifiser reachable-count på litmus.
- **F4:** sweep + fri-liste. Verifiser 10000+ hmac-litmus ikkje sprenger.
- **F5:** wiring i safepoint + terskel-trigger. Verifiser test_security + full
  Native Linux-flate på fersk seed.
- **F6:** ELF stage-0-paritet-sjekk + arm64-port (macho_arm64_codegen).

## Risiko

GC-bug korrupterer alt. Kvar fase må verifiserast isolert (litmus før full
flate). ELF stage-0-paritet må haldast etter kvar codegen-endring. Arm64 må
portast separat (eiga NcVal-emisjon). Realistisk fleir-sesjons maskinkode-nær
arbeid.
