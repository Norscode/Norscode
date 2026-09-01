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

## ❌ Codegen-hull (blokkerer seed-bygging — kompilatoren brukar begge)

1. **Unnatak — `prøv`/`fang`/`kast` (try/catch/throw).** `native_codegen_v2` handterer
   `THROW` (→ RT_THROW som skriv+avsluttar) men IKKJE `TRY_BEGIN`/`TRY_END`/`LOAD_EXCEPTION`
   → unnatak vert ALDRI fanga, dei propagerer til topp og avsluttar. Krev: global handler-
   stakk i mappa scratch-minne (utvid ELF-mmap/HEAP_SZ), TRY_BEGIN push {catch_addr, rsp,
   rbp}, TRY_END pop, RT_THROW → unwind (restore rsp/rbp, hopp til catch, sett last_exception),
   LOAD_EXCEPTION push. Pluss FINALLY_PUSH/RUN/END + type-matching for full VM-paritet
   (sjå `selfhost/vm.no` linje ~6298 for kanon-semantikk). STOR, kryssfunksjon-unwinding.

2. **Flyttal — `desimaltall` (float).** INGEN SSE/float-støtte i codegen (ingen movsd/xmm/
   cvtsi). Float-aritmetikk gir feil resultat. Krev: NcVal-float-representasjon, float-literal
   (PUSH_CONST), SSE +-*/ og samanlikningar, int↔float-konvertering. STOR, frå grunnen.

## Sekvens vidare

GC-veggen er klarert. Neste: implementer (1) unnatak og (2) flyttal (kvar sitt fokuserte
delprosjekt, verifiser mot VM-paritet), deretter prøv å AOT-kompilere ein ekte kompilator-
modul og iterer på gjenverande hull, gjer GC standard av flagget, og byt seed-bygging
C→sjølvhosta (Fase 4: fjern gjenverande C).
