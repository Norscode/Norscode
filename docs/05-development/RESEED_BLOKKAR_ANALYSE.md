# Reseed-blokkar: rot-årsak og hand-off (Linux x86_64 stage0)

Sist oppdatert: **2026-09-22**. Skriven som presis hand-off for den som køyrer
seed-bygg-pipelinen. Gjeld greina `promotering-fersk-seed` (PR #200).

## Kort samandrag

- **Kompilatorfiksen `__main__`-drop er ferdig og korrekt** (commit `e1c4925`).
  Han er **ikkje aktiv i CI** før stage0-seeden blir bygd på nytt (CI kopierer
  den committa `bootstrap/stage0/norscode-linux-x86_64` til `dist/norscode_native`).
- **Reseed er blokkert av eit chicken-and-egg GC/heap-problem** i sjølve
  seed-bygg-vegen — ikkje av fiksen. Detaljane under.
- Eit reseed-forsøk med rask codegen (`4cf2c27`) vart **reversert (`4734316`)**
  fordi den seeden regresserte attestasjon (sjå under).

## 1. Kompilatorbuggen som er fiksa (`e1c4925`)

`compile` av ei fil som importerer ein modul dropp entry-modulen (`__main__`)
sine eigne funksjonar. `nc_bundle_ncb` byggjer ein **korrekt** bundle (gyldig
JSON, alle funksjonar; `run` køyrer han), men `nc_compile` køyrde
`normaliser_json_kontrollteikn` **ein gong til** over heile ~154 KB-bundlen —
som alt er `json_skriv`-eskapa (0 rå kontrollteikn) — og under compile-heap-
presset avkorta reserialiseringa strengen til ~10.7 KB (samanhengande prefiks;
`__main__` blir slått inn sist og forsvann).

Fiks: `normaliser_json_kontrollteikn` (`selfhost/nc_main.no`) er skriven om til
**eitt pass**: skann etter det fyrste rå kontrollteiknet inne i ein streng;
finst ingen → returner input uendra (ingen reserialisering); finst eitt → skriv
det reine prefikset som éin slice og reserialiser berre resten. Byte-identisk
med det gamle no-op-passet på reine data → fikspunktet urørt; toppminne ≤ det
gamle eitt-pass-bygget.

Verifisert (native x86-64, på ein kandidat-seed): `test_web_handle_request_fallback`
→ 154157 B / 2 `__main__` (var 10769 / 0); 40+ testar (native krypto,
`admin_relations` 1.58 MB norsdb, web-klynga) grøne, 0 OOM.

## 2. Reseed-blokkaren — presis rot-årsak

Målet: byggje ein fullhost stage0-seed (nc_main + selfhost.vm innebygd, so
`selfhost.vm.køyr_ncb` er native) frå den **fiksa** kjelda. To codegen-vegar,
begge blokkerte:

### (a) Rask prebygd codegen-ELF → seed med ØYDELAGD GC-attvinning
`tools/ncb_to_elf.no` sin raske veg (`bootstrap/native_codegen_x86_64.elf`)
emitterer seedar der GC-en **ikkje vinn attende** transient søppel. Målt:

- Committed seed (den arbeidande) *grind* `test_ed25519_rfc8032` via `run` i
  280 s **utan OOM** (rc=124 timeout) — GC-en attvinn, berre treg (rein-VM ECC).
- Seed bygd av den prebygde codegen-ELF-en frå **committed seed sin EIGEN
  NCB** (byte for byte same NCB, 3 020 794 B) → **OOM (rc=199,
  «GC-heap full, 1 GiB levande etter collect»)** på same testen.
- Dette gjeld **både** gjeldande codegen-ELF (`sha 41329163…`) og den frå før
  `37cee6b` (`2543470 B`). So det er **ikkje ein versjonsregresjon** — den
  prebygde codegen-ELF-en miskompilerer GC-emisjonen som sådan.

Difor: den raske vegen gjev alltid ein seed som OOM-ar på minnetung rein-VM-
last (krypto). Ein slik seed regresserte òg attestasjon (nøsta probe-spawn).

### (b) Interpretert codegen → korrekt GC, men GC-*trash* mot 1 GiB-taket
`native_codegen_v2.no` køyrd interpretert (via committed seed) emitterer
**korrekte** seedar (slik den arbeidande committed seeden vart laga). MEN å
byggje fullhost-seeden (~3 MB NCB) interpretert **GC-trashar mot 1 GiB-
heaptaket**: målt 3,5 t CPU, RSS fast ~988 MB (~1 GiB), null framgang. Det
levande settet under codegen-bygget nærmar seg taket → collect gjer lite → i
praksis fast.

### (c) Chicken-and-egg
Å heve heaptaket krev at seeden som *køyrer* codegen har større heap — men å
byggje ein seed med større heap krev ein codegen. Rask codegen (a) gjev
øydelagd GC; interpretert (b) trashar. Sirkelen må brytast.

### Heap-layout-grense (kvifor ein ikkje berre hevar taket)
GC-metadata skalerer med heapstorleik (bitmap ∝ H, mark-stakk = H/2, live-map =
H/8), og **handler-regionen må liggje under `0x80000000`** pga. abs32-
adressering (`mov rax,[abs32]`). Noverande layout (`selfhost/native_execution/
native_codegen_v2.no`, GC-layout v3, ~linje 44–67): heap 1 GiB
`[0x600000, 0x40600000)` + bitmap + live-map (0x43000000) + mark-stakk 512 MiB
(0x4B000000..0x6B000000) ≈ 1,79 GiB totalt. Berre ~256 MiB slark under
2 GiB-taket. Å vekse heapen monaleg krev at GC-metadata flyttar til 64-bit-
adressering (`movabs`) — ei større codegen-endring.

## 3. Kva som må til for ein rein reseed

Bryt sirkelen slik (den arbeidande committed seeden vart truleg laga på ein av
desse måtane — venteleg via ein bootstrap-seed med heva heap / GC-fiks som køyrde
den interpreterte codegen utan å trashe):

1. **Anten** hev heap-taket (og flytt GC-metadata til 64-bit-adressering der det
   trengst) so den interpreterte fullhost-codegenen fullfører utan trash, og
   byggj seeden interpretert (korrekt GC).
2. **Eller** rot-årsak kvifor den prebygde codegen-ELF-en miskompilerer GC-
   emisjonen, og regenerer ein korrekt prebygd codegen-ELF (rask veg attende).
3. Fôr den fiksa fullhost-NCB-en (materialisert av
   `tools/materialize_nc_main_fullhost_candidate.no`, som gjev rett
   `global_names`/`LOAD_GLOBAL` og unngår OOM ved per-modul-barneprosessar)
   til den korrekte codegenen → seed.
4. Valider: `test_ed25519`/`x25519` (ingen rc=199), web-klynga
   (`test_web_handle_request_fallback` = 2 `__main__`), `selfhost-bootstrap-gate`,
   og — kritisk — **Linux runtime-attestasjon** (driveren spawnar ein nøsta
   probe; ein seed med øydelagd nøsta køyring avsluttar utan rapport).
5. Promoter: `bootstrap/stage0/norscode-linux-x86_64` + oppdater
   `bootstrap/stage0/SHA256SUMS` (x86_64-lina).

## 4. Reproduksjon (kommandoar)

```sh
# __main__-drop (på committed seed, før reseed): 0 __main__, avkorta
NORSCODE_CMD=compile NORSCODE_FILE=tests/test_web_handle_request_fallback.no \
  NORSCODE_OUTPUT=/tmp/x.json NORSCODE_MODULE=__main__ \
  NORSCODE_ROOT=$PWD NORSCODE_IMPORT_BASE=$PWD \
  ./bootstrap/stage0/norscode-linux-x86_64
grep -ao '"module":"__main__"' /tmp/x.json | wc -l   # → 0 (buggy) / 2 (fiksa seed)

# Rask-codegen GC-brest: bygg seed frå committed seed sin eigen NCB, køyr ed25519
#   → rc=199 (OOM), medan committed seed grind (rc=124) på same test.
```

## 5. Empirisk oppdatering 2026-09-22 (ny undersøking) — presisert diagnose

Ei ny undersøking (billeg repro, ikkje fullhost-bygg) korrigerer og skjerpar §2:

1. **Prebygd codegen-ELF er IKKJE stale.** `bootstrap/native_codegen_x86_64.elf.srchash`
   (`4e7ae236…`) == `sha256(native_codegen_v2.no + ncb_serde.no)` no. `native_codegen_v2.no`
   er dessutan uendra sidan `37cee6b` — same commit som bygde BÅDE den committa seeden OG
   den prebygde codegen-ELF-en. Så «rask veg = stale» er ikkje forklaringa.

2. **Prebygd codegen gjev KORREKT rask krypto når native atomar dekker rekninga.**
   `tests/test_ed25519_rfc8032` kompilert → ELF via prebygd codegen (GC på) → `rc=0`,
   ingen OOM (native fe_mul/_A/_Z-atomar unngår heap-allokering). rc=199-påstanden i §2(a)
   reproduserer altså IKKJE for direkte-native krypto.

3. **Den verkelege bresten er ein alvorleg yte-/GC-klippe på LISTE-/allokeringstung
   native kode** — det som i §2 heiter «grind»/«GC-trash». Billeg repro (sekund å byggje):
   ein `churn`-lykkje som byggjer + forkastar ei 2000-elements liste per iterasjon:
   - 15 000 iter → `rc=0`, RSS ~1,8 MB, ferdig på sekund (KORREKT reclaim, rett svar).
   - 60 000 / 120 000 / 200 000 iter → `rc=124` (timeout), fullfører ikkje → **super-lineær
     nedbremsing** (ikkje ei lineær minnelekkasje: RSS held seg ~1,8 MB ved GC på;
     ved GC av (2 GiB bump) same nedbremsing). Ein O(1)-per-iter-lykkje skal ikkje bremse slik.
   - SAME `churn` køyrd via den committa seeden sin VM (`run-ncb-pure`, GC på) held RSS
     flatt på 1,8 MB — VM-GC-en reclaimar reint.

4. **Hovudmistenkt: den emitterte fri-liste-allokatoren `gc_alloc`
   (`native_codegen_v2.no:4077`) er «head-fit»** — han gjenbrukar berre HOVUD-blokka i
   fri-lista om ho er stor nok, elles bump. Blokkar som ikkje matchar hovudet blir aldri
   gjenbrukte → fragmentering/bump-vekst og/eller aukande collect-kostnad → thrash når
   allokeringane hopar seg opp. Dette råkar BÅDE (a) codegen-tida (interpretert fullhost-
   codegen som byggjer den store grafen) OG (b) seed-runtime på rein-VM-krypto (ed25519
   via `køyr_ncb` allokerer tungt). Å betre reclaim (ekte fri-liste-søk / storleiksklassar
   / koalescering, eller kompakterande sweep) er difor eit felles, høgverdig mål.

**Konsekvens for reseed-strategien:** den skarpaste låsen er ikkje «stale prebuilt» og
ikkje ein rein heap-tak-storleik, men at den emitterte GC-allokatoren skalerer dårleg på
allokeringstung last. To reelle vegar står att: (i) betre `gc_alloc`/sweep-reclaim i
`native_codegen_v2` (hand-emittert x86-64 — delikat, men avgrensa; testbar med `churn`-
reproen under før noko seed-bygg), eller (ii) den større 64-bit-adresserings-/heap-tak-
endringa frå §2. Begge er substansielle; ingen er ein rask patch.

### Repro (billeg, ingen fullhost-bygg)
```sh
SCR=/tmp/nc-churn; mkdir -p $SCR
cat > $SCR/churn.no <<'NO'
funksjon lag_liste(n: heiltall) -> liste { la ut: liste = [] la j = 0 mens j < n { legg_til(ut, j) j = j + 1 } returner ut }
funksjon start() -> heiltall { la i = 0 la sum = 0 mens i < 200000 { la l = lag_liste(2000) sum = sum + lengde(l) i = i + 1 } skriv("churn done\n") returner 0 }
NO
NORSCODE_CMD=compile NORSCODE_FILE=$SCR/churn.no NORSCODE_OUTPUT=$SCR/churn.ncb.json \
  NORSCODE_MODULE=__main__ NORSCODE_ROOT=$PWD NORSCODE_IMPORT_BASE=$PWD ./dist/norscode_native
NORSCODE_GC_ALLOC=1 NORSCODE_ROOT=$PWD ./bin/nc ncb-to-elf $SCR/churn.ncb.json $SCR/churn.elf
NORSCODE_GC_ALLOC=1 $SCR/churn.elf                      # native: thrash/timeout (rc=124)
NORSCODE_GC_ALLOC=1 NORSCODE_CMD=run-ncb-pure NORSCODE_FILE=$SCR/churn.ncb.json \
  NORSCODE_ROOT=$PWD ./dist/norscode_native            # committed VM: reclaimar, RSS ~1,8 MB
# Reduser 200000→15000 i kjelda → native ELF fullfører rc=0 (reclaim OK i det små).
```

## Relaterte filer
- `selfhost/nc_main.no` — `normaliser_json_kontrollteikn` (fiksen)
- `selfhost/native_execution/native_codegen_v2.no` — GC-layout v3 (linje ~44–67),
  CALL-emisjon (bruker-funksjon FØR atom, ~linje 9449), ECC-atom (fe_mul/fe_add/
  fe_sub, ~linje 2861–2980, suffix-match ~8683)
- `tools/ncb_to_elf.no` — rask prebygd codegen-veg (srchash-porta)
- `tools/build_x86_seed.no` — interpretert codegen-veg (30-min spawn-tak)
- `tools/materialize_nc_main_fullhost_candidate.no` — fullhost-NCB per-modul
- `docs/05-development/PLAN_SEED_PROMOTERING.md` — overordna statusplan
