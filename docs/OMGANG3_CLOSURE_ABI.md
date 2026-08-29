# Omgang 3 · oppgåve 1 — Closure-ABI i native codegen

**Mål:** definere korleis ein closure (lambda + fanga variablar) er representert i
heapen for AOT-native codegen, slik at `BUILD_LAMBDA` (oppgåve 2) og `CALL_VALUE`
(oppgåve 3) kan implementerast byte-verifiserbart mot tolk-orakelet.

Dette er **berre ABI-spesifikasjonen** — ingen emittarar er endra av denne
oppgåva. Emittering + fixpunkt-regenerering kjem i oppgåve 2–3.

---

## 1. Orakelet: korleis tolken representerer ein closure

`selfhost/vm.no` (M3, gjenoppretta i denne PR-en):

- **`BUILD_LAMBDA`** (`vm.no:4940`) — `instr[1]` = fullt lambda-funksjonsnamn,
  `instr[2]` = liste med namn på frie variablar. Byggjer eit fangst-kart
  `capture[cn] = ramme_les_var(ramme, cn)` for kvart namn, og ein closure-verdi:
  ```
  closure = { "__lambda__": lam_namn, "__capture__": capture }
  ```
  (ein `ordbok`), som blir dytta på operandstakken.
- **`CALL_VALUE`** (`vm.no:4963`) — `instr[1]` = argument-tal. Poppar closure-en
  frå toppen, hentar `__lambda__` (mål-namn) + `__capture__` (fangst-kart), og
  deler heile `CALL`-maskineriet. Fangsten blir bunden inn i callee-ramma med
  `vm_bind_lambda_capture(ramme, lambda_capture)` (`vm.no:5187`, `5283`) slik at
  lambda-kroppen les fanga variablar som om dei var lokale.

**Kontrakt for native:** ein native closure må vere *semantisk lik* denne — same
fanga verdiar (per namn), same returverdi ved kall. Differensial-selen
(`tools/differensial_sele.no`) er fasit: `test_lambda`/`test_lambda_closure` AOT
== tolk.

---

## 2. NcVal-grunnlaget (uendra)

Alle native verdiar er 16-byte heap-structar `{i64 type, i64 val}` (delt av
`macho_arm64_codegen.no` og `native_codegen_v2.no`). Eksisterande taggar:

| tagg | type | `val` |
|---|---|---|
| 1 | heiltal | 64-bit heiltal (sxtw ved boksing) |
| 2 | bool | 0/1 |
| 3 | streng | peikar til `[i64 lengd][bytes]` |
| 4 | liste | peikar til `[i64 count][elem0 ptr]…` |
| 5 | kart | peikar til `[i64 count][key0 ptr][val0 ptr]…` |

**Tagg 6 og 7 er ledige** (verifisert: ingen bruk i nokon backend). Denne ABI-en
tek **tagg 6 = closure**.

Boksing skjer via `emit_ncval_alloc_ptr(native, ptr_reg, dst_reg, type_imm)`
(`macho_arm64_codegen.no:221`): skriv `type_imm` til `[x28]`, peikaren til
`[x28+8]`, bump `x28 += 16`. Closure-boksing bruker `type_imm = 6`.

---

## 3. Closure-heap-objektet (tagg 6)

Ein closure er ein NcVal med `type = 6` og `val` = peikar til ei **closure-data-
blokk** på heapen:

```
NcVal closure:            closure-data (16 byte, x28-bump-allokert):
  [i64 type = 6]            [i64 fn_addr ]   ← absolutt runtime kode-adresse
  [i64 val   ]──────────▶   [i64 capture ]    ← NcVal* (tagg 5 kart) ELLER 0
```

- **`fn_addr`** — den absolutte runtime-adressa til lambda-funksjonen, rekna ut på
  BUILD_LAMBDA-staden med ein `adr x9,#0` (x9 = adressa til adr-instruksjonen)
  pluss eit **rein byte-avstand** til lambdaen: `movz/movk x10, #delta; add x9,x9,x10`.
  `delta = fn_start[lam] − adr_pos` er vaddr-**uavhengig** (rein avstand i imaget)
  og alltid positiv (lambdaen kjem etter build-staden i `order`), patcha i
  link-passet (`lamaddr`-patch, §4). Vald framfor ein fn-indeks + tabell fordi
  det droppar all tabell-materialisering og eitt runtime-oppslag, og adr+delta
  er allereie PC-relativt (PIE-trygt) utan å tråkle inn tekst-vaddr.
- **`capture`** — NcVal-peikar til eit **tagg-5 kart** med *identisk* innhald som
  tolken sitt `__capture__` (nøkkel = fritt-variabel-namn, verdi = boksa NcVal
  fanga på BUILD_LAMBDA-tidspunktet). `0` (null-peikar) når lambdaen ikkje fangar
  noko — callee-en sjekkar `cbz` (jf. Omgang 1 null-vaktene) og hoppar over
  binding.

Blokka er **immutabel** etter bygging (fangst er by-value på byggjetidspunktet,
som tolken). Ingen GC enno (Fase 6-porten open) → inga rewind. Reachability:
`ncval_reachable` følgjer no BUILD_LAMBDA-målet eksplisitt (`:3618`-blokka), elles
blir lambdaen aldri lagd i `order`/`fn_start` og `lamaddr`-patchen feilar.

---

## 4. `lamaddr`-patch: closure → fn-adresse (vaddr-uavhengig)

`CALL` i dag er ein **direkte** `bl` med namn-patch (`bl_patchar` →
`all_patchar`, resolvert via `fn_start[namn]`). `CALL_VALUE` har **ikkje** målet
på byggjetidspunktet — det kjem frå closure-verdien. BUILD_LAMBDA legg difor
adressa inn i closure-blokka via ein ny patch-type `lamaddr`.

BUILD_LAMBDA emitterer (4 instr):
```
adr  x9, #0                 ; x9 = adressa til denne instruksjonen (base+adr_pos)
movz x10, #delta_lo16        ; ← patch
movk x10, #delta_hi16, lsl16 ; ← patch
add  x9, x9, x10             ; x9 = base + adr_pos + delta = fn_addr
```
og registrerer `[movz_pos, lam_namn, "lamaddr"]` i `bl_patchar`. Sidan
emitterbufferen er **byte-indeksert**, resolverer link-passet:
```
delta = fn_start[lam_namn] − (movz_pos − 4)      // −4: adr står rett før movz
movz-imm = delta & 0xFFFF ; movk-imm = (delta>>16) & 0xFFFF
```
`delta` er rein byte-avstand i imaget → **uavhengig av tekst-vaddr** (PIE-trygt),
og alltid positiv (unik build-stad, lambdaen kjem etter i `order`); resolveren
feilar eksplisitt ved negativ delta. 32-bit rekkjevidd (>4 GiB image utenkjeleg).

`CALL_VALUE`: last `fn_addr = [closure_data+0]`, `capture = [closure_data+8]`,
sett opp argument + capture (§5), `blr fn_addr`.

> **Alternativ (forkasta):** ein fn-indeks + `__fn_addr_table__` (indeks→adresse)
> materialisert med `adrp+add`. Meir indireksjon (tabell-oppslag) og `adrp`
> treng absolutt tekst-vaddr for sideberekninga — `adr`+delta unngår begge.

---

## 5. Kallekonvensjon for `CALL_VALUE`

Vanleg `CALL` (`macho_arm64_codegen.no` §kallekonvensjon, `:2489`, `:3415`):
argument i `x0–x7` (maks 8 — «NcVal kall støttar høgst 8 argument»), resultat i
`x0`, callee lagrar `x19–x26 + x30`, lokale som `NcVal*` ved `[x26 + i*8]`.

`CALL_VALUE` legg til **fangst-peikaren** som eit skjult *etterfølgjande*
argument:

- Deklarerte argument i `x0 … x[argc-1]` (som vanleg).
- **`capture`-peikaren i `x[argc]`** (registeret rett etter siste deklarerte
  arg). Gjenbruker arg-passeringsvegen (som alt er callee-preservert inn i
  prologen), så ingen nytt reservert register trengst.
- **Grense:** lambda-aritet ≤ 7 (så `x[argc]` finst i `x0–x7`). Kompilatoren
  assertar dette ved BUILD_LAMBDA; nc_main sine lambdaer er langt under.

Callee-prolog for ein **lambda-funksjon** (kompilatoren veit på byggjetid at
`lam_namn` er ein lambda med frie namn `cap_namn`):

1. Stash `x[argc]` (capture-peikaren) i ein reservert lokal-slot *før* nokon
   scratch-klobb (alloc bruker `x13/x14`, concat `x9–x16`).
2. `cbz` capture → hopp over binding (lambda utan fangst).
3. For kvar fri variabel: verdien blir lesen frå fangst-kartet via
   `emit_map_get(capture, name)` i staden for eit lokal-slot-oppslag. Kompilatoren
   rutar fritt-variabel-referansar i lambda-kroppen hit (namna er kjende frå
   `BUILD_LAMBDA instr[2]`). Dette er den native motparten til
   `vm_bind_lambda_capture`.

Resultat i `x0` som vanleg. Ikkje-closure på `CALL_VALUE`-staden (tolken sin
«mål_namn tom → ikkje-funne»-veg): native sjekkar `type != 6` → kast
`NATIV-GAP`/ikkje-kallbar (fail-closed, som `ncval_gap_stub`).

---

## 6. Stack-effekt (alt på plass i tolken)

`vm.no:4010–4011` (denne PR-en):
```
BUILD_LAMBDA → [2, 2]     (konsumerer namn+capture-liste-materiale, dytter closure)
CALL_VALUE   → [1, 1]     (poppar closure + argc, dytter resultat — som CALL)
```
Native operand-djupn-føranalyse (`ncval_maxdjupn`) må telje `BUILD_LAMBDA`/
`CALL_VALUE` likt, elles feil spilling. (Oppgåve 2–3 legg dei inn i føranalysen.)

---

## 7. Differensial-kontrakt (fasit for oppgåve 2–3)

Ein native closure er **korrekt** når, for alle testar i lambda-familien:

1. `BUILD_LAMBDA` byggjer ein tagg-6 NcVal der `capture`-kartet har *same
   nøklar/verdiar* som tolken sitt `__capture__` (fanga by-value på same punkt).
2. `CALL_VALUE` gjev *same returverdi* som tolken (`test_lambda`,
   `test_lambda_closure`, nøsta/rekursive closures, closure-i-liste/kart).
3. `NORSCODE_DIFF_STRICT=1 tools/differensial_sele.no` grøn for kvar lambda-fixtur
   på ARM64-Linux **og** x86-64.

Backend-nøytralitet: §3–4 (heap-layout, tagg 6, indeks-tabell) er delt; §5
(register `x[argc]`, prolog-stash) er ARM64-konkret — x86-64 (`native_codegen_v2.no`)
speglar med sin eigen arg-register-sekvens og eit tilsvarande capture-slot, same
closure-layout og same tabell-modell.

---

## 8. Status

- [x] **Oppgåve 1 — ABI-dok** (dette dokumentet): tagg 6 closure = `{fn_index,
      capture_ptr}`, funksjons-adressetabell (indeks→adresse, `adrp+add`),
      capture som etterfølgjande arg `x[argc]`, prolog-binding via `map_get`,
      differensial-kontrakt.
- [ ] Oppgåve 2 — `BUILD_LAMBDA` i native codegen (bygg tagg-6 blokk + fangst-kart).
- [ ] Oppgåve 3 — `CALL_VALUE` (adressetabell + `blr` + capture-binding) +
      fixpunkt-regenerering av seed; differensial grøn.

**Verifiseringsveg:** all maskinkode i oppgåve 2–3 er byte-usjekka frå macOS —
ARM64-Linux Docker/CI-loopen (jf. B2-minnet) er fasit, med differensial-selen som
port.
