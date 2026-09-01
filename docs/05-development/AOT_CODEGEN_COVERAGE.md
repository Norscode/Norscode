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

## Sekvens vidare

GC-veggen OG unnatak-hullet er klarert; kompilator-like kode (mini-tolk) verkar. NESTE
konkrete blokkar er dei frosne JSON-rutinane (over) — reimplementer json_stringify+json_parse
for list/map. Deretter: AOT-kompiler heile kompilatoren, gjer GC standard av flagget, byt
seed-bygging C→sjølvhosta (Fase 4). Valfritt seinare: finally + float for brukarprogram.
