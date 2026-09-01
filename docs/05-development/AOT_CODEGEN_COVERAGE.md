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

## Sekvens vidare

GC-veggen OG unnatak-hullet er klarert. Codegen ser no funksjonelt komplett ut for
kompilator-like kode (verifisert med mini-tolk). Neste: prøv å AOT-kompilere ein EKTE
kompilator-modul / heile kompilatoren via native_codegen_v2 og iterer på evt. gjenverande
hull; gjer GC standard av flagget; byt seed-bygging C→sjølvhosta (Fase 4: fjern gjenverande C).
Valfritt seinare: finally + float for full brukarprogram-dekning.
