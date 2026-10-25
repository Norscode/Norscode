# Generatorar: `gi` (F45)

Status: implementert i VM-vegen (kjelde-kjeda `.no` -> NCB -> `selfhost/vm.no`).
AOT (x86/arm64) støttar ikkje generatorar, sjå "AOT" nedst.

## Syntaks

Ein funksjon som inneheld `gi uttrykk` er ein generator. Å kalle han køyrer
IKKJE kroppen; han gjev ein iterator-verdi:

```
funksjon teller(n: heiltall) {
    la i = 0
    mens i < n {
        gi i            # gjev verdien og pausar
        i = i + 1
    }
}

for x i teller(3) { skriv(tekst(x)) }   # 0 1 2

la g = teller(2)
neste(g)        # 0
g.neste()       # 1  (metodesyntaks, same som neste(g))
neste(g)        # ingenting (ferdig)
g.ferdig()      # sann
```

- `gi uttrykk` er ei setning (ikkje eit uttrykk) og gjev `uttrykk` til den som
  ba om neste verdi.
- `gi frå uttrykk` delegerer: gjev alle element frå ein annan generator (eller
  ei liste/tekst/ordbok) ein etter ein. Parseren senkar det til
  `for t i uttrykk { gi t }`. Returverdien til den delegerte generatoren
  blir ikkje sendt tilbake.
- `returner` (med eller utan verdi) avsluttar generatoren; verdien blir
  forkasta. Å kome til slutten av kroppen avsluttar òg.
- `neste(gen)` / `gen.neste()` gjev neste verdi, eller `ingenting` når
  generatoren er ferdig (ingen `SluttFeil`). Sidan `gi ingenting` er lov, kan ein
  skilje "ferdig" frå ein gjeven `ingenting` med `gen.ferdig()` (`sann` når det
  ikkje finst fleire element; pausar kroppen til neste element ved behov).
  `for`-lykkja har ikkje dette problemet.
- Unntak i generatoren propagerer ut av `neste()` (eller ut av den `for`-lykkja
  som dreiv han) og kan fangast der. Etter eit unntak er generatoren ferdig
  (`neste` gjev `ingenting`).
- Lat evaluering: det første `neste()` (eller første lykkjesteg) køyrer fram til
  fyrste `gi`. Uendelege generatorar er fine saman med `bryt`.
- `gi` fungerer inne i `prøv`/`fang`/`endeleg`, `for`, `mens`, `hvis`.
- Kvar generator har eigne lokale variablar; fleire generatorar frå same
  funksjon er uavhengige. Ein generator kan kalle vanlege funksjonar og
  andre generatorar (`neste(inner)`).
- `neste()` på ein generator som allereie køyrer (frå seg sjølv) kastar
  `GeneratorFeil`.
- Ikkje støtta: `gi` i ein lambda (kompileringsfeil), `await`/suspensjon inne i
  ein generator (`GeneratorFeil`), sending av verdiar inn i generatoren,
  `gi` som uttrykk.

`gi` er kontekstuelt: eksisterande kode som brukar `gi` som variabelnamn
(`la gi = 5`, `gi = gi + 1`, `gi(x)`, `gi[0]`, `gi.felt`) er uendra. Ei setning
er eit yield berre når `gi` står først og eit uttrykk fylgjer på same linje
(`gi x`, `gi "a"`, `gi 3`, `gi [1, 2]`, `gi (a + b)`); for `gi (x)` og `gi [x]`
må det vere mellomrom etter `gi`.

## Implementering

- Parser: `Gi`-node (`parse_gi` i `selfhost/parser.no`).
- Bytekode: ny opcode `GI` (ingen operand; poppar verdien). Ein funksjon med
  `GI` får `"generator": true` i NCB-funksjonsdefinisjonen. Kode utan `gi` gjev
  uendra NCB (verifisert byte-identisk på fleire fiksturar).
- VM: eit kall til ein generator-funksjon (CALL, `ncb_call_fn`, `køyr_funksjon`)
  gjev ei ordbok `{"__generator__": fn, args, state, resume, ...}`. `neste` køyrer
  kroppen i ein nøsta `køyr_funksjon`-aktivering. `GI` parkerer rammen (pc, stakk,
  lokale, try-stakk) i resume-slotten med `reason = "gen"` (same mekanisme som
  await/grøne trådar) og returnerer verdien; neste `neste` tek rammen opp att.
- `for x i gen()` brukar den uendra for-bytekoden: `type(gen)` er `"generator"`,
  `lengde(gen)` er `0` (ferdig) eller ei stor verdi (eit element står klart;
  køyrer generatoren fram til neste `gi`), og `INDEX_GET` på generatoren gjev
  neste element. Lengda er altså ikkje meiningsfull for brukarkode; bruk `for`,
  `neste` eller `ferdig()`.
- Frosne samlingar (F46) kan delast mellom generatorar og trådar, sjå
  FROSNE_SAMLINGAR.md.

## AOT

`gi` har ingen AOT-semantikk enno. x86 (`native_codegen_v2.no`) og arm64
(`macho_arm64_codegen.no`) gjev kompileringsfeil `NATIV-GAP: gi (generator) er
ikkje støtta i AOT`, og `neste`/`gen_neste`/`gen_ferdig` er fail-closed
(`NATIV-GAP:` unntak ved køyring). Dette verkar først i den pinna kodegen-ELF-en
etter regenerering (reseed); før det gjev ein AOT-bygd generator-program stille
feil resultat. WASM har ingen støtte.
