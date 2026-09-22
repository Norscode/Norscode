# Presise stakk-rot (precise stack roots) — designplan for å bryte reseed-blokkaren

Status: **DESIGN / scoping**. Skriven 2026-09-22. Gjeld greina `promotering-fersk-seed`
(PR #200). Følgjer opp rot-årsaka i `RESEED_BLOKKAR_ANALYSE.md` §6.

## 1. Kvifor (problemstilling)

Reseeden — og dermed heile «vekk med JSON»-målet og rask rein-VM-krypto — er blokkert av
**éin** ting: den emitterte GC-en si **konservative rot-skanning** pinnar søppel falskt, so
sweep ikkje kan attvinne det. Under allokeringstung last ratchetar bump-peikaren difor oppover
mot 1 GiB-taket → thrash (rc=124) eller OOM (rc=199).

Dette er empirisk fastslått (sjå RESEED-doc §5–§6):
- Codegen er **deterministisk** native vs interpretert (byte-identisk output) → ikkje ein
  prebuilt-miskompilering; svakheita ligg i **kjelda** `native_codegen_v2.no`.
- Allokatorane er **alt first-fit** (`gc_alloc16` :4358, `gc_alloc_var` :4459; vandringstak
  512, prøvd 65536 utan effekt). Attbruks-strategien er ikkje problemet — det er at det finst
  **for få hol** (objekt er falskt pinna, ikkje frigjorde).
- Billeg repro (`churn`, sjå RESEED-doc §5): ei lykkje som byggjer + forkastar ei liste per
  iterasjon fullfører på 15k iterasjonar men thrashar på 60k+ — super-lineær nedbremsing.

### Rot-mekanikk (kva som blir falskt pinna)
`gc_mark_roots` (`native_codegen_v2.no:7490–7503`) skannar HEILE den levande stakken
`[rsp+8, stack_base)` og reknar KVART 16-aligna ord i heap-området som ein levande peikar
(`gc_push`: range- + align- + mark-bit-dedup). Norscode-verdiar er alle boksa (peikar til
16 B-heap-slot), so ekte lokalvariablar ER peikarar og blir korrekt dekt. Dei FALSKE røtene
kjem frå alt ANNA på stakken:
- **Spilte mellomverdiar / uttrykks-temporærar** som ligg att på stakken.
- **Lagra caller-saved-register** rundt kall.
- **Skratsj-buffer på stakken** (t.d. streng-/syscall-buffer) med vilkårlege bytar som
  aliasar heap-adresser.
- **Daude slottar**: ein lokalslot for ein variabel som er ute av skop, men ikkje overskriven,
  held ein gammal peikar → held eit daudt objekt «levande» (dette er «stale stakkslot» frå
  kommentaren ved `gc_alloc16` :4358–4361).

## 2. Mål og ikkje-mål

**Mål**
- Redusere falsk pinning så sweep attvinn transient søppel → bump held seg lågt → ingen
  thrash/OOM på allokeringstung last (crypto via `køyr_ncb`, codegen-tid, `churn`).
- Behald **korrektheit** (aldri misse ein ekte levande peikar → aldri use-after-free) og
  **ELF-fikspunkt-determinisme**.

**Ikkje-mål (i denne runden)**
- Kompakterande/flyttande GC (fase 6-porten) — større, separat.
- Å heve 1 GiB-heaptaket / 64-bit-adressering — palliativ, løyser ikkje pinning (sjå §7).
- Endre allokatorane (dei er alt first-fit).

## 3. Kjerne-innsikt som gjer dette muleg

Codegen VEIT alt stakk-layoutet:
- Kvar funksjon har standard rbp-ramme (`prolog` :835: `push rbp; mov rbp,rsp`), og lokal-
  variablar er **rbp-relative 8 B-slottar adressert per namneindeks** (`rbp_off(slot) =
  -slot*8`, :896). `n_local` er kjend ved prolog-emittering.
- **Safepoint-ar** (der collect skjer) ligg berre INNE i slike ramma Norscode-funksjonar
  (safepoint-stub :7905–7923). Atom/rå-asm (t.d. `process_spawn`, felt-aritmetikk) har ingen
  safepoint inni seg → deira transiente stakk-skratsj er aldri levande VED ein collect.

Difor kan collect **gå ramme for ramme** via rbp-kjeda og skanne BERRE dei namngjevne
lokalslottane i kvar ramme — og hoppe over retur-adresse, lagra rbp, spilte temporærar og
skratsj. Det fjernar heile klassen av falske røter frå ikkje-slot-innhald.

## 4. Presisjons-spekter og tilrådd trinnvis veg

| Nivå | Kva | Fjernar | Kostnad |
|---|---|---|---|
| **0 (no)** | Konservativ full-stakk-skann | — | — |
| **1: ramme-presis** | Gå rbp-kjeda; skann berre `[rbp−8·n_local, rbp)` per ramme | spilte temporærar, register-lagringar, skratsj, retur-adr, padding | Moderat: uniform prolog + omskriven `gc_mark_roots` |
| **2: safepoint-presis** | Per-safepoint bitmap over LEVANDE slottar | i tillegg: daude-men-i-ramme slottar | Høg: liveness-analyse + kart-emisjon per safepoint |

**Tilråding: implementer Nivå 1 først, mål mot `churn`, gå til Nivå 2 berre om naudsynt.**
Hypotese (sterk): thrash-en kjem hovudsakleg frå spilte/skratsj-falske røter (nivå-1-klassen),
ikkje frå daude slottar — fordi `churn`-lykkja held `sum`/`l` LEVANDE i slottane sine, medan
det er dei frigjorde backing-arrays som blir pinna av spill/register-lagringar. Nivå 1
åleine skal difor gje det store utslaget. Mål det før noko meir.

## 5. Nivå 1 — detaljert design (ramme-presis rot-skann)

### 5a. Lagre `n_local` i ramma
`prolog` (:835) veit `n_local`. Legg til, rett etter `mov rbp,rsp`, ei uniform lagring av
`n_local` på ein **fast rammeslot** som collect kan lese. To variantar:
- **(A) Eigen slot:** `mov qword [rbp-8], n_local` og la lokalane starte på `rbp-16`
  (dvs. skift alle `rbp_off` +1 slot). Ryddig, men rører slot-nummereringa (påverkar `:852`
  «andre pass reknar slot frå namneindeks» → må justere `rbp_off`/`count_slot` konsekvent).
- **(B) Push før lokalane:** etter `push rbp; mov rbp,rsp`, `push imm(n_local)`; då ligg
  `n_local` på `[rbp-8]` og lokalane på `[rbp-16 …]`. Same skift som (A), men via push
  (enklare epilog: teljar-slotten forsvinn med `leave`).

Vel den som gjev minst forstyrring av eksisterande slot-rekning; begge treng at `rbp_off`
og alle slot-brukarar (prolog `count_slot` :858, variabel-emittering :866–899) skiftar
konsekvent. **Determinisme:** uniform for alle funksjonar → ELF-fikspunktet held.

Alternativ utan prolog-endring: **side-tabell retur-adresse → n_local**, bygd ved codegen
og skriven i .data; collect binærsøkjer. Meir kode i collect, men rører ikkje ramma. Vel
dette om prolog-skiftet viser seg for risikabelt for fikspunktet.

### 5b. Skriv om `gc_mark_roots` til ramme-vandring
Erstatt den lineære `[rsp+8, stack_base)`-skanninga (:7490–7503) med:
```
rbp_cur = <lagra rbp ved safepoint>        ; safepoint-stub må gje collect rbp (sjå 5c)
mens rbp_cur != SENTINEL og rbp_cur < stack_base:
    n = [rbp_cur - 8]                       ; n_local for denne ramma (5a)
    p = rbp_cur - 16                        ; fyrste lokalslot (etter n-slot)
    slutt = rbp_cur - 16 - 8*n
    mens p > slutt: gc_push([p]); p -= 8
    rbp_cur = [rbp_cur]                      ; neste ramme (lagra rbp)
```
- **Behald** HOST_ABI-globalskann (:7477–7489) uendra (modulglobalar + ENV_OVERLAY; lite,
  presist nok).
- **Behald** DFS-drain (:7505+) uendra.
- `gc_push` (range/align/dedup) står — den er den siste vakta mot ugyldige kandidatar.

### 5c. Ramme-kjede-terminering (SENTINEL)
Vandringa MÅ stoppe ved den ytste Norscode-ramma (under ligg C-runtime-entry / host-ABI-
oppsettet, som ikkje har vår ramme-form). Alternativ:
- Sett `rbp = 0` (eller ein kjend base) i entry-stubben FØR fyrste Norscode-kall, og stopp
  når `[rbp]==0`. Reint, men krev endring i entry-oppsettet.
- Klamp: stopp når `rbp_cur >= stack_base` eller `< STACK_LIMIT`. Robust nok som backstop.
Bruk begge (eksplisitt sentinel + klamp).

### 5d. Kva collect treng frå safepoint
Collect må starte vandringa frå rbp PÅ safepoint-punktet. Safepoint-stubben (:7905) kallar
`gc_collect_native` (:7805 → `gc_mark_roots`). Sidan stubben køyrer i den ramma funksjonen,
er `rbp` alt korrekt der. Verifiser at stubben ikkje endrar rbp før mark; om han gjer det,
lagre call-site-rbp i ein fast HEAP-slot som `gc_mark_roots` les.

## 6. Korrektheits-tryggleik (kritisk)

Ein FOR presis skann som MISSER ein ekte levande peikar → use-after-free → korrupsjon (verre
enn over-pinning). Rulle ut med nett:
- **Verifikasjonsmodus (`NORSCODE_GC_VERIFY_ROOTS=1`):** køyr BÅDE ramme-presis og konservativ
  full-skann; assertér at presis-settet ⊇ (alle konservative som er ekte boksar). Køyr heile
  test-porten + `churn` + crypto i denne modusen før presis blir standard. (Ny env-gate,
  codegen-time, som `NORSCODE_GC_ALLOC`.)
- **Invariant:** collect skjer BERRE ved safepoint (alt sant). Stadfest at ingen atom/rå-asm
  kallar allokator som kan trigge collect medan levande boksar berre finst i register (ikkje
  på stakken/slot). `gc_mark_roots`-kommentaren (:7455) seier verdiar ligg på stakken ved
  safepoint — dette må haldast: eventuelle boksar i register ved eit safepoint må vere spilte
  til ein SKANNA plass (lokalslot), ikkje berre til rå skratsj.
- **Spill-til-slot-regel:** om nivå 1 viser at nokre levande boksar berre finst som spill
  utanfor lokalslottane ved eit safepoint, må codegen spille dei til lokalslottar (eller
  markere dei) — dette er grensa mot Nivå 2.

## 7. Kvifor ikkje berre heve heaptaket (64-bit-adressering)

Å flytte GC-metadata til 64-bit-adressering og vekse heapen gjev meir headroom, men **løyser
ikkje** pinninga: falskt pinna objekt hopar seg framleis opp, berre seinare. Det er ein
palliativ (kan kjøpe tid for eit stort bygg), ikkje ein fiks. Prioriter §5. (Detaljar om
abs32-grensa: RESEED-doc §2 heap-layout-note.)

## 8. Integrasjonspunkt (eksakte stader)

- `prolog(buf, n_local, …)` — `native_codegen_v2.no:835` (n_local-lagring, 5a).
- `rbp_off` :896, `count_slot`/prolog-detalj :852–858, variabel-emittering :866–899 (slot-skift).
- `gc_mark_roots` :7457 (stakk-skann :7490–7503 → ramme-vandring; behald HOST_ABI :7477 +
  DFS :7505).
- `gc_push` (range/align/dedup) — sluttvakt, urørt.
- Safepoint-stub :7905–7923; `gc_collect_native` :7805.
- Entry-stub / SENTINEL-oppsett (5c) — finn entry-emitteringa (`start`-oppsett).
- GC-layout/kanon-cache :44–91 (uendra).

## 9. Valideringsplan

Validerings-løkka (RESEED-doc §5): `compile native_codegen_v2.no → NCB` (~180 s) →
`run-ncb-pure` codegen-NCB med `NC_INPUT=<test>.ncb.json` → køyr emittert ELF. Deterministisk,
so interpretert resultat == regenerert prebuilt.

1. **`churn` (billeg):** 200k-iter ELF skal no fullføre `rc=0` med bunden `ps -C churn.elf`
   RSS (ikkje 988 MB-thrash). Primær-signal.
2. **Verifikasjonsmodus** (§6) grøn over heile test-porten + `churn` + `test_ed25519`/`x25519`.
3. **Crypto:** `test_ed25519_rfc8032` via `køyr_ncb` (rein-VM) skal ikkje thrashe/OOM.
4. **Fikspunkt:** `selfcompile-stage0-elf` Gen1 ELF == Gen2 ELF (byte-paritet) held.
5. **Attestasjon:** nøsta probe-spawn (`tools/linux_runtime_attestation.no`).
6. **Regenerer** `bootstrap/native_codegen_x86_64.elf` + `.srchash` frå den fiksa kjelda
   (elles fell `ncb-to-elf` til den treige interpreterte vegen).
7. Fyrst DÅ: fullhost-reseed (materialize → codegen → valider → promoter seed + SHA256SUMS).

## 10. Risiko og mottiltak

| Risiko | Mottiltak |
|---|---|
| Presis skann missar ekte rot → use-after-free | Verifikasjonsmodus §6; behald konservativ som fallback til porten er grøn |
| Prolog-skift bryt ELF-fikspunkt | Uniform emittering; eller side-tabell-variant utan prolog-endring (5a) |
| Hand-emittert asm feil | Kvar endring testast via `churn`-løkka (§9.1) før vidare |
| Treg validerings-løkke (~10 min/iter) | Minimer iterasjonar: design ferdig på papir, verifiser encoding mot disassembler før bygg |
| Nivå 1 ikkje nok (daude slottar dominerer) | Gå til Nivå 2 (per-safepoint liveness-bitmap) |

## 11. Effekt-estimat og verdi

- **Nivå 1:** ~1–2 fokuserte dagar for ein som kan codegen-en (prolog + gc_mark_roots +
  sentinel + verifikasjonsmodus), pluss validerings-runder (trege pga. §9-løkka).
- **Låser opp:** korrekt fullhost-reseed → aktiverer `e1c4925` (__main__-drop) i CI, gjer
  `køyr_ncb` native (attestasjon), fjernar crypto-thrash, og — via reseed — gjer det trygt å
  flippe dei 78 `.ncb.json` til binær (fullfører «vekk med JSON»).
- Dette er den **einaste** kjende vegen som løyser rot-årsaka; alt anna (allokator-tuning,
  heap-tak) er uttømt eller palliativt.

## Relaterte filer
- `docs/05-development/RESEED_BLOKKAR_ANALYSE.md` — rot-årsak (§6) + repro (§5) + løkke.
- `docs/05-development/PLAN_SEED_PROMOTERING.md` — overordna reseed-status.
- `selfhost/native_execution/native_codegen_v2.no` — all codegen (integrasjonspunkt §8).
- `docs/05-development/AOT_GC_DESIGN.md` — eksisterande GC-designnotat.
