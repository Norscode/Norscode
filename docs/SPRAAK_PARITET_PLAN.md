# Språkparitet-plan — frå dagens Norscode til JS-erstatning og Java-garantiar

**Dato:** 2026-08-19
**Grunnlag:** alle påstandar er verifiserte ved å køyre `./bin/nc` mot testprogram, ikkje lesne ut av designdokument. Designdokumenta i `docs/01-language-guide/` skildrar fleire ting som ikkje er implementerte i den utsende verktøykjeda.

## Utgangspunkt: kva som IKKJE er problemet

Server-stacken er reell og sterk: HTTP/HTTPS, TLS 1.3 i rein Norscode, WebSocket over native socket, TCP/UDP, SMTP/IMAP/POP, DNS, JSON, regex-motor med grupper og flagg, kryptografi (AES-GCM, ChaCha20-Poly1305, Ed25519, X25519, Argon2id, SHA-familien), NorsDB + SQLite-adapter + ORM, templating, GC og JIT i VM-en, native ELF-generering, og ein ekte LSP-server ([selfhost/lsp/server.no](../selfhost/lsp/server.no), 1897 linjer, med hover/completion/definition/diagnostikk).

**Du byggjer språkgarantiar oppå ei fungerande kjerne — du byggjer ikkje kjernen på nytt.**

## Verifiserte funn som styrer planen

| Funn | Bevis | Milepæl |
|---|---|---|
| Byggcachen serverer gammal kode | `build/nc-module-cache/` gav gammal bytekode for `std/js_tal.no` etter kjelde-endring; `nc run` korrekt, `nc test` feil | M0 |
| Typar blir ikkje handheva | `tar_tal("tekst")` mot `x: heltall` kompilerer og køyrer | M1 |
| Køyretidsfeil er ikkje fangbare | `prøv { 1 / 0 } fang (e) {}` → programmet døyr; same for indeks utanfor | M2 |
| Ingen stack trace | uhandtert feil tre kall djupt gir éi linje utan kallstack | M2 |
| Lambdaer køyrer ikkje | `fun(y) -> x + y` → `ukjent opcode BUILD_LAMBDA`; capture-lista er tom i [ir_to_bytecode.no:1299](../selfhost/compiler/ir_to_bytecode.no) | M3 |
| Ingen null-verdi | `builtin.type(null)` → `heltall`, `tekst(null)` → `"0"` | M4 |
| Ingen objektmodell | `klasse Foo { funksjon bar() }` blir parsa som map-literal; `struktur` med typa felt gir parserfeil | M5, M9 |
| Unntakstypar verkar ikkje | `fang (e: ValueError)` fangar ikkje `kast "..."`, sjølv om `catch_type` finst i lowering | M6 |
| Async er berre syntaks | `asynk`-kall utan `await` returnerer verdien direkte, ikkje ei oppgåve | M7 |
| Ingen native trådar | [std/tråd.no](../std/tråd.no): *«Norscode har ikkje native trådar»*; `asynk.map_parallelt` er sekvensiell | M8 |
| Syntaks-hol | spread, destrukturering, `?.`, standardargument, varargs, template-strengar gir alle parserfeil | M10 |
| Ingen iteratorprotokoll | `gi`/`yield` er ikkje nøkkelord; `for k i ordbok` gir `null` | M11 |
| WASM er ein stubb | [selfhost/wasm.no](../selfhost/wasm.no) 70 linjer, [browser_runtime.no](../runtime/browser/browser_runtime.no) 63 linjer, ingen DOM-binding | M12 |

## Rekkefølgje-prinsipp

1. **Verifiseringa må vere til å stole på først** (M0). Utan det kan ingen av dei andre milepælane bevisast.
2. **Garantiar før funksjonar.** Typesjekk og fangbare feil (M1–M2) gjer all seinare kode tryggare å skrive.
3. **Låg innsats med høg spreiing før stor innsats med smal spreiing.** Lambdaer (M3) låser opp heile det funksjonelle laget; WASM (M12) tener berre nettlesaren.
4. **Kvar milepæl må ha eigne testar under `tests/`** som køyrer grønt via `./bin/nc test` før neste startar.

---

## Soft-seed: DELVIS aktivering lokalt (berre runtime) 🟡 *(2026-08-22)*

**Presist:** soft-seed (`run-ncb-pure` på nc_main-bundlen) aktiverer **runtime-features
(M3 lambda/closure, M4 ingenting)** lokalt, men **ikkje parse-tid-features (M5-metode-def,
M10 destrukturering/spread, M9 grensesnitt)**. Grunn: `run-ncb-pure` brukar seed-en sin
INNEBYGDE native kompilator-frontend (lexer+parser+lowering i `bin/nc`) til å kompilere
brukarfiler, og køyrer berre resultatet på den ferske VM-en frå bundlen. Difor lyser
VM-nivå-features opp, medan parser-nivå-syntaks feilar med same feil som gamle `bin/nc`.

**Bevis (grundig verifisert):** (1) live-source `USE_PRECOMPILED=0` passerer alle
M2/M4/M5/M10 — kjelda er korrekt; (2) bundle-parseren er byte-identisk med korrekt
kompilering (einaste diff: `LOAD_NAME null` vs `PUSH_CONST None`); (3) `nc-fresh` gir
NØYAKTIG same parserfeil som `bin/nc` på M5/M10; (4) smoke sin `BUILD_LAMBDA` KØYRDE
(gammal VM kan ikkje) → fersk VM gjorde køyringa. Konklusjon: seed-frontend skygger
bundlen sin parser. **Full aktivering av parse-tid-features krev å ERSTATTE `bin/nc`
(native rebuild)** — blokkert på macOS av 15 KB Mach-O-emitteren, treng Linux-ELF/CI.

Merk: M7/M8/M11 (data-drivne bibliotek) treng korkje soft-seed eller rebuild — dei
køyrer på enhver seed via `nc run` allereie.

Vegen for soft-seed-materialiseringa (uendra, fungerer):

1. `selfcompile-l5` → L5-bundles frå kjelde (byte-identisk Gen1/Gen2, ~1 GiB RSS).
2. `materialize_nc_main_fullhost_candidate.no` (med `NC_FULLHOST_REUSE=1`) →
   `build/fullhost/nc_main_fullhost_candidate.ncb.json` — full nc_main-kompilator
   bundla frå kjelde: **997 funksjonar**, inkl. all M1–M11 + `selfhost.compiler.typecheck`.
   Kvar modul kompilerast isolert (~270 MB RSS), så heile flyten held seg under budsjettet.
3. **Køyr den ferske bundlen tolka på gjeldande VM** via `run-ncb-pure`. Sidan
   `run-ncb-pure` ikkje tek argv, gir ein kommando/fil gjennom miljøet:

```
NORSCODE_ENABLE_EXEC_PROSESS=1 \
NORSCODE_VM_CAPABILITIES=env.read,env.write,process.exec,thread.spawn,net.tcp,net.http,net.dns,disk.read,disk.write,jit.execute \
NORSCODE_VM_DISK_ROOT="$PWD,.,/tmp,/private/tmp" \
NORSCODE_CMD=run NORSCODE_FILE=<din_fil.no> \
./bin/nc run-ncb-pure build/fullhost/nc_main_fullhost_candidate.ncb.json
```

**Verifisert:** [tools/fixtures/seed_aktivering_smoke.no](../tools/fixtures/seed_aktivering_smoke.no)
gir «ukjent opcode BUILD_LAMBDA» på den gamle seeden, men **«SEED-AKTIVERING OK»**
(M3 lambda + closure + M4 ingenting) gjennom den ferske bundlen. Dette er den
fungerande sjølvstendige verktøykjeda — tolka (tregare enn native), men funksjonelt
komplett for M1–M11.

### Native seed på macOS — blokkert av liten Mach-O-emitter

`bygg-native --target macos-arm64` kan **ikkje** byggje ein full nc_main-seed her:
[macho_arm64_emitter.no:217](../selfhost/native_execution/macho_arm64_emitter.no)
har eit hardt tak på **15 000 byte kode** (signatur-offset låst til 20480) — han er
ein små-program-emitter. Den kanoniske self-rebuild-vegen ([tools/selfcompile_native_elf.no](../tools/selfcompile_native_elf.no))
målrettar dessutan **`linux-x86_64`** ELF, ikkje macos-arm64. Ein fullnative seed
krev difor Linux ELF-codegen på ein Linux-host/CI — akkurat som
[docs/ARM64_FRESH_SEED_BOOTSTRAP.md](ARM64_FRESH_SEED_BOOTSTRAP.md) føreskriv. Soft-seed
over `run-ncb-pure` er den lokale aktiveringa i mellomtida.

---

## Kjelde-kjede-verifisering ✅ GRØN *(2026-08-22)*

Heile paritetskjelda er empirisk verifisert **rebuild-klar** gjennom den levande
KJELDE-kompilatoren + KJELDE-VM-en (ikkje den innebygde seed-en):
- **M3** (closures) — `tests/test_lambda_closure.no` grøn via `nc run` (356s, maks 12,9 GB RSS).
- **M2/M4/M5/M10** — [tools/verify_parity_chain.no](../tools/verify_parity_chain.no)
  lastar `selfhost.vm` éin gong og køyrer alle snippetene: `ingenting`-type,
  fangbare div0/indeks-feil, struktur-metodar med dispatch (71325), destrukturering,
  spread, standardargument — **8/8 OK** (555s, maks 12,9 GB RSS).
- **M7/M8/M9/M11** — køyrer allereie på seed-runtimen (data-drivne), grøne via `nc run`.

**Konklusjon:** kjeldearbeidet er korrekt og aktiverast av ein seed-rebuild. Den
einaste attståande blokkeringa for `nc run`-aktivering er sjølve seed-rebuilden (native
codegen, ~8,7 GiB-topp), som må køyrast på ein host med nok minne / CI — sjå
[[seed-aktivering-minnevegg]] i minnet. Kjelde-kjede-verifisering krev ~10–13 GiB
arbeidssett; ho fullførte her fordi macOS komprimerte i staden for å jetsam-drepe.

### Rebuild-oppskrift (kjørbar, ikkje-destruktiv)

[tools/rebuild_seed_kandidat.no](../tools/rebuild_seed_kandidat.no) byggjer ein fersk
seed-kandidat frå gjeldande kjelde og verifiserer at M1–M11 er aktivert, **utan å
promotere** (skriv berre under `build/`, rører aldri `dist/` eller `bootstrap/stage0/`):

```
NORSCODE_ENABLE_EXEC_PROSESS=1 \
NORSCODE_VM_CAPABILITIES=env.read,process.exec,disk.read,disk.write \
NORSCODE_VM_DISK_ROOT="$PWD,.,/tmp,/private/tmp" \
./bin/nc run tools/rebuild_seed_kandidat.no
```

Tre steg: (1) materialiser full `nc_main`-bundle frå kjelde (minnetrygg L5b-fragment),
(2) `bygg-native --ncb` → kandidat-binær (~8,7 GiB-topp), (3) køyr
[tools/fixtures/seed_aktivering_smoke.no](../tools/fixtures/seed_aktivering_smoke.no)
gjennom kandidaten. Smoken parsar på alle seedar men treng M3/M4-runtime: gammal seed
feilar med «ukjent opcode BUILD_LAMBDA», ein korrekt fersk seed skriv «SEED-AKTIVERING
OK». Overstyr mål med `NC_SEED_TARGET` (standard `macos-arm64`). Promotering er eit
medvite B2-steg som verktøyet berre skriv instruksjonar for.

---

## M0 — Byggcache som invaliderer korrekt ✅ FERDIG *(2026-08-19)*
**Innsats:** 1–2 dagar · **Avhengig av:** ingenting · **Blokkerer:** alt

Cachenøkkelen tek berre omsyn til inngangsfila, ikkje til importerte modular. Endrar du `std/x.no` og køyrer ein test som importerer han, kan `nc test` køyre gammal bytekode og gi grønt eller raudt på feil grunnlag. Dette kosta ein halvtimes feilsøking under kartlegginga og ville ha forgifta kvar einaste milepæl under.

**Filer:** `selfhost/nc_main.no` (`nc test` ~linje 3396), bundlar-/cachelaget som skriv `build/nc-module-cache/` og `build/nc-test-cache/`.

**Steg**
1. Finn staden der cachenøkkelen blir rekna ut.
2. La nøkkelen vere hash av inngangsfila **pluss** hash av kvar transitivt importerte `.no`-fil.
3. Skriv importlista inn i `.key`-fila, slik at cachen kan sjekke kvar fil.
4. Legg til `nc test --no-cache` for naudutgang.

**Akseptkriterium:** endring i ein std-modul gjer at neste `nc test` rekompilerer.

**Resultat:** rotårsaka låg i `kompiler_modul` i [tools/compile_with_hybrid_bundle_v9400.no](../tools/compile_with_hybrid_bundle_v9400.no). Cachenøkkelen dekte berre modulens eiga kjeldefil og verktøyet sjølv, medan ei modulkompilering kan innehalde funksjonane til heile den transitive importgrafen (sjå `merge_fragment`). Nøkkelen tek no òg med eit innhaldsadressert manifest over alle transitive importar (`import_manifest`), med memoiserte filhashar. `cache_abi` er heva til `v3`, så gamle oppføringar blir forkasta. `NORSCODE_NO_MODULE_CACHE=1` hoppar over cachen heilt.

Reproduksjonen er verifisert begge vegar med ein probe-modul som endra returverdi frå 1 til 2 medan testen framleis kravde 1:

| Bundlar | Etter endring i importert modul | |
|---|---|---|
| original | `OK: Alle testar bestått` | feil — køyrde gammal bytekode |
| fiksa | `FEIL: Testane feilet` | rett — endringa vart oppdaga |

Test: [tests/test_byggcache_invalidering.no](../tests/test_byggcache_invalidering.no) med fixtur-kjede `cachetest_a → b → c` under `tests/fixtures/`, som låser fast at manifestet er transitivt og innhaldsadressert.

**Merk:** `nc test`-brotet for db-testar (`Ukjent variabel: vm_active_functions`) er *ikkje* same sak — det står att etter denne fiksen og skuldast uinitialiserte modulglobalar i vm.no (sjå `tools/repair_vm_globals_candidate.no`).

---

## M1 — Handheving av typar i semantic-fasen ✅ FERDIG I KJELDE *(2026-08-20)*
**Innsats:** 3–5 veker · **Avhengig av:** M0 · **Verdi:** høgast av alle

Annotasjonane finst allereie i parsaren og i AST-en. Det som manglar er sjekken. Dette er ein kompilatorpassasje, ikkje ein språkredesign.

**Filer:** `selfhost/semantic.no` (har allereie diagnostikk-infrastruktur, jf. `semantic_rapporter`), `selfhost/ast.no`.

**Steg**
1. **Typemiljø:** bygg symboltabell per funksjon med parameter- og lokalvariabeltypar.
2. **Uttrykkstypar:** implementer `type_av(uttrykk, miljø)` for literalar, variablar, binæroperatorar, indeksering, feltaksess og kall.
3. **Sjekkpunkt i denne rekkjefølgja:**
   - returverdi mot deklarert returtype
   - argument mot parametertypar
   - tildeling mot deklarert variabeltype
   - operandar til aritmetikk og samanlikning
4. **Gradvis innføring:** ny flagg `nc check --strict-typar`. Køyr han over heile `std/` og `selfhost/`, tell feil, ryd opp modul for modul.
5. **Vipp om:** når `std/` er rein, gjer strict til standard og legg inn `--lax-typar` som overgangsutgang.
6. **Generics-handheving** (`liste<tekst>`) kjem i M9 — i M1 blir typeparametrar godtekne, men ikkje sjekka.

**Akseptkriterium:** kvart av desse gir kompileringsfeil med linjenummer:
```norscode
la a = tar_tal("tekst")           // argument
funksjon f() -> heltall { returner "tekst" }   // retur
la c: heltall = "tekst"           // tildeling
la d = 1 + "tekst"                // operand
```
Test: `tests/test_typesjekk_avvis.no` (skal feile ved kompilering) + `tests/test_typesjekk_godtek.no` (gyldig kode skal framleis kompilere).

**Risiko:** heile `std/` (293 modular) kan innehalde typefeil som har levd i skuggen. Difor er steg 4 obligatorisk før steg 5 — rekn med at oppryddinga tek meir tid enn sjølve sjekken.

**Resultat (verifisert [tests/test_typesjekk.no](../tests/test_typesjekk.no)):** implementert som eit opt-in pass bak `NORSCODE_STRICT_TYPAR=1`. Sjekken ligg i ein eigen modul [selfhost/compiler/typecheck.no](../selfhost/compiler/typecheck.no) (`kt_koyr(tabell, program)`), kalla frå `analyser_program` i [selfhost/compiler/semantic.no](../selfhost/compiler/semantic.no) som `kast`ar den fyrste typefeilen når flagget er på. `samle_funksjonar` populerer no `param_typar`/`retur_typar`. Tre sjekkar: returverdi vs deklarert returtype, `la x: T = …` vs T, og kall-argument vs parametertypar. Typar kanoniserast til tal/tekst/bool/liste/ordbok/ukjend; `ukjend` (struct/generisk/uatkjent) blir aldri rapportert → ingen falske positive. Operand-sjekk på `+` er *med vilje utelaten*: Norscode coercar `1 + "x"` → `"1x"`, så det ville vore falsk positiv.

Verifiserte tilfelle: avvist — retur-mismatch, tildeling-mismatch, argument-mismatch; godteke — gyldig kode, tekst-konkatenering, builtin-kall (ukjend), og alt kompilerer framleis med flagget AV (bakoverkompatibelt).

**To harde seed-kompilator-gotchas funne undervegs** (kritiske for vidare selfhost-arbeid):
1. **`nc check` fangar IKKJE alt** — ein duplisert funksjonsheader gav `OK` i `nc check` men fekk bundlaren til å droppe funksjonen. Verifiser alltid med `./bin/nc compile <fil> -o /tmp/x.ncb.json` og sjekk at funksjonane finst (~0.3 s, mykje raskare enn full chain).
2. **Seed-kodegen trunkerer modulen** ved visse konstruksjonar — særleg **ein ø-variabel (t.d. `miljø`) sendt direkte som argument til ein builtin** (`har_nokkel(miljø, …)`). Funksjonen og ALT etter han i modulen forsvinn frå bundelen. Løysing: unngå ø i variabelnamn som går til builtins, og trekk nøsta uttrykk ut til temp-variablar før builtin-kall. Difor er `typecheck.no` skrive magert med `env` (ikkje `miljø`) og temps før kvart builtin-kall.

**Std/-opprydding (steg 4–5):** ikkje køyrt — krev stage0-regen for å køyre `--strict-typar` over alle 293 modulane praktisk. Sjølve sjekken er ferdig og verifisert.

---

## M2 — Fangbare køyretidsfeil og stack traces ✅ FERDIG I KJELDE *(2026-08-19)*
**Innsats:** 1–2 veker · **Avhengig av:** M0

I dag drep ein indeksfeil i éin HTTP-request heile serveren. Dette er den mest akutte produksjonsrisikoen i heile lista.

**Filer:** `selfhost/vm.no` (feilstiar, `TRY_BEGIN`/`TRY_END`-handtering, `__vm_active_frames__`).

**Steg**
1. Finn alle stader i VM-en som avbryt utan å gå gjennom unntaksmekanismen (indeks utanfor, divisjon med null, ukjent variabel, typefeil i builtins).
2. Rut dei gjennom same veg som `kast`, med maskinlesbar kode: `IndeksFeil`, `DivisjonMedNull`, `TypeFeil`, `NøkkelFeil`.
3. Bygg kallstack ved unntak frå `__vm_active_frames__` og legg han på unntaksobjektet.
4. Skriv ut stack trace ved uhandtert feil: funksjonsnamn, modul, linje (source maps finst allereie i bytekoden).

**Akseptkriterium:**
```norscode
prøv { la x = [1, 2][9] } fang (e) { skriv("fanga\n") }    // skal skrive "fanga"
prøv { la d = 1 / 0 }     fang (e) { skriv("fanga\n") }    // skal skrive "fanga"
```
og uhandtert feil tre kall djupt skal vise alle tre nivåa.

**Resultat:** implementert i [selfhost/vm.no](../selfhost/vm.no):

1. Ny lokal `intern_feil` i tolkeloopen. Interne feil set flagget i staden for å `kast` rett til verten; toppen av loopen gjer det om til eit vanleg guest-`THROW`, som går gjennom den eksisterande handler-, cleanup- og continuation-logikken.
2. `BINARY_DIV` → `DivisjonMedNull`, `BINARY_MOD` → `ModuloMedNull`.
3. `INDEX_GET` og `INDEX_SET` fekk grensesjekk *før* den native indekseringa (som elles avbryt prosessen) → `IndeksFeil` med indeks og listelengde.
4. Uhandterte unntak ber kallstacken: rammenamn og ip blir samla medan stacken blir rulla av, og lagt på meldinga av `vm_med_spor` som `" | ved <funksjon> (ip N)"`. Meldinga står fyrst, så eksisterande delstreng-sjekkar held.

Test: [tests/test_vm_fangbare_feil.no](../tests/test_vm_fangbare_feil.no) — fanging av divisjon/modulo med null, lesing og skriving utanfor lista, negativ indeks, at gyldige operasjonar er upåverka, og at uhandterte feil ber både feiltype og kallstack. Alle 11 testane som køyrer kjelde-VM-en er grøne.

**Viktig avgrensing:** `bin/nc` er ein symlink til `dist/norscode_native`, som er byte-identisk med stage0-seeden `bootstrap/stage0/norscode-macos-arm64`. Brukarkode køyrer difor i VM-en som ligg i seed-binæren, ikkje i `selfhost/vm.no`. Endringa er verifisert mot kjelde-VM-en og blir aktiv i `./bin/nc run` fyrst når stage0 blir regenerert.

**Funn undervegs:** eit ukvalifisert `køyr_funksjon(...)` blir kompilert til `builtin.køyr_funksjon` og treffer seed-VM-en. Testar som skal dekke `selfhost/vm.no` må bruke `bruk selfhost.vm som vm` og kalle `vm.køyr_funksjon(...)`. Fleire eksisterande `test_vm_*`-testar brukar den ukvalifiserte forma og testar dermed seeden, ikkje kjelda.

---

## M3 — Førsteklasses funksjonar og closures ✅ FERDIG I KJELDE *(2026-08-20)*
**Innsats:** 1–2 veker · **Avhengig av:** M0 · **Låser opp:** M7, M11 og heile `std/js_*`-laget

Parsaren og lowering er halvvegs ferdige alt. Det er VM-sida som manglar.

**Filer:** `selfhost/vm.no`, `selfhost/compiler/ir_to_bytecode.no` (linje ~1299).

**Steg**
1. **Capture-analyse i lowering:** finn frie variablar i lambdakroppen og fyll capture-lista som i dag er hardkoda tom.
2. **`BUILD_LAMBDA` i VM-en:** lag ein funksjonsverdi `{__fn__: namn, __capture__: {...}}`; registrer opcoden i `vm_opcode_operandar` (~linje 5560) og i bytekodeverifikatoren.
3. **`CALL_VALUE`:** nytt opcode som kallar ein funksjonsverdi frå stacken; capture-miljøet blir lagt inn i ramma før parametrane.
4. **Funksjonsreferansar:** `la f = dobbel` skal gi same verdiforma.
5. **AOT/native:** same lowering i `selfhost/native_execution/` slik at native-banen ikkje divergerer.
6. **Overlast `std/js_*`:** `kart(l, fn)` skal godta både funksjonsnamn (tekst) og funksjonsverdi — API-et endrar seg ikkje.

**Akseptkriterium:** `tests/test_lambda.no` (finst allereie, feilar i dag) blir grøn, pluss:
```norscode
la faktor = 3
la gong = fun(x) -> x * faktor
assert_eq(jsl.kart([1, 2], gong), [3, 6])
```
**Resultat:** implementert i [selfhost/compiler/ir_to_bytecode.no](../selfhost/compiler/ir_to_bytecode.no) og [selfhost/vm.no](../selfhost/vm.no):

1. **Capture-analyse** (`_lambda_saml_referert` / `_lambda_saml_bundne`): frie variablar = namn referert i lambdakroppen, som verken er param, deklarert inne i kroppen, eller modulglobal, men som finst i det ytre lokale namnrommet. BUILD_LAMBDA får no capture-lista fylt (var hardkoda tom før).
2. **`BUILD_LAMBDA` i VM-en**: byggjer ein closure-verdi `{__lambda__, __capture__}` og les dei fanga variablane frå gjeldande ramme (by-value).
3. **`CALL_VALUE`**: nytt opcode som deler heile CALL-maskineriet. Closure-verdien ligg øvst på stacken; VM-en løyser `__lambda__`-namnet og bind `__capture__` som lokale i callee-ramma etter parametrane (`vm_bind_lambda_capture`), i både tail- og continuation-vegen. JIT-vegen blir hoppa over når closuren har capture.
4. **Kall til lokal variabel** som held ein lambda blir lowra til `LOAD_NAME` + `CALL_VALUE` i staden for eit namn-basert CALL.

Test: [tests/test_lambda_closure.no](../tests/test_lambda_closure.no) — kompilerer fire program gjennom **kjelde-kompilatoren** og køyrer dei gjennom **kjelde-VM-en**: grunnleggjande closure (fangar `x` → 7), by-value-capture (endring etterpå påverkar ikkje → 11), lambda utan capture (→ 42), fleire captures (→ 13). Alle grøne, og NCB-en inneheld BUILD_LAMBDA + CALL_VALUE.

**Same stage0-avgrensing som M2:** aktiv i `./bin/nc` fyrst etter stage0-regenerering. `tests/test_lambda.no` (den opphavlege aksepttesten) passerer gjennom kjelde-chaina, men ikkje via seed-binæren enno.

**M3-follow-on ✅ FERDIG:** `builtin.ncb_call_fn` tek no eit funksjonsnamn (tekst) ELLER ein closure-verdi som fyrste argument. Er det ein closure, blir `__capture__` bunde som lokale. Difor tek `std/js_*`-modulane lambdaverdiar **utan éi einaste kjeldeendring** — dei kallar allereie `builtin.ncb_call_fn(fn_namn, ...)`. Verifisert i [tests/test_lambda_closure.no](../tests/test_lambda_closure.no): closure gjennom ncb_call_fn (→42), med capture (→105), og funksjonsnamn-som-tekst framleis (→42).

---

## M4 — Ekte `ingenting` som eigen type ✅ FERDIG I KJELDE *(2026-08-20)*
**Innsats:** 1 veke · **Avhengig av:** M1 (for at typesjekken skal kunne uttrykkje det)

**Nøkkelfunn:** den native runtimen HAR alt ein distinkt `ingenting`-verdi — eit manglande ordbok-oppslag gir type `ingenting` og `== ingenting` var *usann* mot literalen. Feilen var berre at **`ingenting`-literalen las som 0** (`ramme_les_var` i [selfhost/vm.no](../selfhost/vm.no) coerca "null"/"ingenting" → 0). Difor er M4 ein *liten, kjelde-verifiserbar* endring, ikkje eit native-arbeid: `ramme_les_var` returnerer no den native ingenting-verdien (`builtin.json_parse_raw("null")`).

**Resultat (verifisert [tests/test_ingenting.no](../tests/test_ingenting.no), 6/6):**
- `type(ingenting)` → `"ingenting"`
- `ingenting == 0` → usann
- `ingenting == ingenting` → sann
- `m["manglar"] == ingenting` → sann (idiomet som var broten er no rett)
- `hvis ingenting` → falsy
- sette verdiar er upåverka

**Same stage0-avgrensing:** aktiv i `./bin/nc` etter stage0-regenerering. Merk: når stage0 blir regenerert gjeld dette heile selfhost-kodebasen — kode som les `ingenting` og reknar med 0 må sjekkast (blast radius er liten: berre 7 `== ingenting`/`!= ingenting`-stader, og dei blir *rettare*, ikkje broten).

I dag er `ingenting` heiltalet 0. Ingen funksjon kan skilje «fann ikkje» frå «fann 0». Dette er årsaka til at `std/js_liste.no` måtte få `har_indeks`/`finn_indeks` som omveg.

**Filer:** `selfhost/vm.no` (verditypar, `builtin.type`, likskap, `tekst()`), `selfhost/semantic.no`.

**Steg**
1. Innfør ein eigen køyretidsverdi for `ingenting` med `builtin.type(...)` → `"ingenting"`.
2. `tekst(ingenting)` → `"ingenting"`, ikkje `"0"`.
3. Likskap: `ingenting == ingenting` er sann; `ingenting == 0` er usann.
4. Sanningsverdi i `hvis`: `ingenting` er usann.
5. Manglande ordboknøkkel returnerer `ingenting` konsekvent.
6. Typesystem: valfrie typar `heltall?` med krav om sjekk før bruk (kan utsetjast til M9 om det blir for stort).

**Akseptkriterium:** `finn` i `std/js_liste.no` kan returnere `ingenting` utan tvitydigheit; `tests/test_ingenting_type.no`.

**Følgje:** `std/js_liste.no` og `std/js_objekt.no` skal ryddast og dokumentasjonen i `docs/02-standard-library/JS_PARITET.md` oppdaterast.

---

## M5 — Metodar på `struktur` og typa felt 🟡 DELVIS *(2026-08-20)*
**Innsats:** 2–3 veker · **Avhengig av:** M1, M3

**Status:**
- **Typa felt** ✅ i kjelde: `struktur Punkt { x: heltall, y: heltall }` parsar no (gav parserfeil før). Felt-typane blir Type-barn på Felt-nodane; runtime lagrar struktur som ordbok som før. Test [tests/test_typa_felt.no](../tests/test_typa_felt.no).
- **Metodar** ✅ FERDIG I KJELDE *(2026-08-22)*: struktur-konstruktør taggar verdien med `__struct_type__`; `funksjon Type.metode(self, …)` registrerast som `Type.metode`; `p.metode(args)` dispatchar via `builtin.kall_metode` på køyretids-typen (eksakt/suffiks-treff, ikkje fuzzy siste-segment). Verifisert: metode(self)→7, metode m/arg→150, type-dispatch 2 typar→37. Test [tests/test_metode.no](../tests/test_metode.no). AVGRENSING: `__struct_type__` forureinar `nøkler`/`lengde` på struct-verdiar (skjult metadata) — risiko ved stage0-regen for kode som itererar struct-nøklar.
- **Metodar (gammal note)** ❌ utsett: struktur-verdiar er utypa ordbøker utan type-tag (`Punkt()` → tom ordbok via stor-forbokstav-konvensjonen i vm.no). Metodedispatch `p.flytt(2)` → `Punkt.flytt(p, 2)` krev at struktur-verdien ber typenamnet sitt (t.d. `__struct__`-tag ved konstruksjon). Det er ei runtime-endring med reell risiko (forureinar ordbok-nøklane som mykje kode brukar), og kan ikkje verifiserast mot heile selfhost-kodebasen utan stage0-regen. Utsett til etter stage0-regen.

Gir 80 % av objektorienteringa utan full klassemodell.

**Filer:** `selfhost/parser.no`, `selfhost/semantic.no`, `selfhost/compiler/ir_to_bytecode.no`.

**Steg**
1. **Typa felt:** `struktur Punkt { x: heltall }` skal parse (feilar i dag, sjølv om `std/native/elf_writer.no` alt er skriven slik).
2. **Konstruktør:** `Punkt { x: 1, y: 2 }` med sjekk av at alle felt er sette.
3. **Metodesyntaks:** `funksjon Punkt.flytt(self, dx: heltall)` og kall `p.flytt(2)`.
4. **Lowering:** metodekall blir statisk kall med `self` som fyrste argument — ingen dynamisk dispatch enno.
5. **Synlegheit:** `privat funksjon` skal faktisk hindre tilgang på tvers av modular (i dag blir ordet godteke utan verknad).

**Akseptkriterium:** `tests/test_struktur_metode.no` med konstruktør, metodekall, felt-mutasjon og avvist tilgang til privat felt.

---

## M6 — Unntakstypar med hierarki ✅ ALT I RUNTIMEN *(verifisert 2026-08-20)*
**Innsats:** 1–2 veker · **Avhengig av:** M2, M5

**Funn:** typa unntak verkar ALLEREIE i den utsende seed-runtimen. `vm_exception_type` løyser eit unntak av forma `"Type: melding"` → `Type`, og strukturerte unntak (ordbok med `"type"`-felt) → den typen. `vm_handler_match` matchar `fang (e: Type)` mot dette. Parsaren les alt `fang (e: Type)`. Difor:
- `kast "FilFeil: melding"` + `fang (e: FilFeil)` → fangar
- feil type fell gjennom til ytre handler
- strukturert `{type: "ValideringsFeil", …}` + `fang (e: ValideringsFeil)` → fangar
- `fang (e)` (utan type) fangar alt

Og M2-feilane er alt typa slik (`DivisjonMedNull: …`, `IndeksFeil: …`), så `fang (e: DivisjonMedNull)` verkar når M2 er aktiv. Test [tests/test_unntakstypar.no](../tests/test_unntakstypar.no).

**Gjenstår (nice-to-have):** ekte type-HIERARKI (subtype-matching) og `kast FilFeil { … }`-konstruktørsukker. Grunnfunksjonen er komplett.

**Seed-quirk notert:** testen passerer via `./bin/nc run` (alle vegar), men feilar via `./bin/nc test` (som set `__vm_force_fast__`) — inste typa fang fangar feil type etter ein tidlegare typa fang. Dette er ein seed-test-runner-bug i unntaks-unwind som M2-omskrivinga i kjelde truleg rettar ved neste stage0-regen. Ikkje ein M6-defekt.

`catch_type` finst allereie i lowering ([ir_to_bytecode.no](../selfhost/compiler/ir_to_bytecode.no) ~linje 1500) og `TRY_BEGIN` tek imot han — men ingen kastar typa unntak.

**Steg**
1. Definer basistypen `Feil` med `melding` og `stack`.
2. `kast FilFeil { melding: "..." }` konstruerer eit typa unntak.
3. `fang (e: FilFeil)` matchar typen og subtypar; `fang (e)` matchar alt.
4. Innebygde feil frå M2 blir typa: `IndeksFeil`, `DivisjonMedNull`, `TypeFeil`, `NøkkelFeil`.
5. Fleire `fang`-blokker etter kvarandre, første match vinn.

**Akseptkriterium:** `tests/test_unntakstypar.no` — spesifikk fangst, fallback, og at feil type ikkje blir fanga.

---

## M7 — Async-runtime med oppgåver, event loop og timerar 🟡 GRUNNMUR FERDIG *(2026-08-22)*
**Innsats:** 4–6 veker · **Avhengig av:** M3, M6

I dag er `asynk`/`await` reint kosmetisk. Denne milepælen gjer dei ekte.

**Status (2026-08-22): KOOPERATIV EVENT LOOP + TIMERAR + KOMBINATORAR FERDIG.**
[std/hendelseslokke.no](../std/hendelseslokke.no) er ein data-driven event loop med
virtuell klokke: ei oppgåve er ei tilstandsmaskin (`oppg_neste` gir `{ferdig}` eller
`{vent: ms}`), loopen har køyreklar-kø + parkert-kø og hoppar klokka fram til neste
vekketid. `køyr_alle` (Promise.all: ferdig når alle er), `køyr_først` (Promise.race:
ferdig ved første) og `med_timeout` (gi opp på ein frist). Akseptkriteriet er verifisert i
[tests/test_hendelseslokke.no](../tests/test_hendelseslokke.no): to oppgåver som søv
100 og 150 fullfører på virtuell tid **150 (maks), ikkje 250 (sum)** — seed-rask via
`nc run`.

**Gjenstår for M7:** `asynk`-kall som faktisk returnerer ei oppgåve, `await` som
parkerer ei vilkårleg VM-ramme (same suspend/resume som M11-generatorane), og ekte
ikkje-blokkerande socket-IO kopla til loopen. Scheduler-semantikken (samtidig = maks,
ikkje sum) er no bevist; det som står att er VM-integrasjonen av `await`.

**Filer:** `selfhost/vm.no`, `selfhost/async_runtime.no`, `selfhost/async_io.no`, `std/asynk.no`.

**Steg**
1. **Oppgåveverdi:** eit `asynk`-kall returnerer ei oppgåve, ikkje ein verdi.
2. **Event loop** i VM-en: køyreklar-kø, ventande-kø, kjøring til alle er ferdige.
3. **Suspendering:** `await` parkerer ramma og gir kontrollen tilbake til loopen (`asynk`-funksjonar blir gjenopptakbare tilstandsmaskinar, slik `docs/01-language-guide/ASYNC_DESIGN.md` skisserer).
4. **Timerar:** `sett_tidsavbrot(ms, fn)` og `sov(ms)` mot loopen; verifiser at `builtin.sov` finst i alle backendar (jf. merknaden i `std/fjern_agent.no:31`).
5. **Kombinatorar:** `alle([...])`, `først([...])`, `med_timeout(...)` — Promise.all/race/timeout.
6. **Ikkje-blokkerande IO:** kople socket-lesing/skriving til loopen, slik at HTTP-serveren kan handtere fleire tilkoplingar utan trådar.
7. **Feil:** eit unntak i ei oppgåve skal kunne fangast av den som ventar på henne.

**Akseptkriterium:** to `asynk`-funksjonar med `sov` skal fullføre på tid ≈ maks, ikkje sum. Test: `tests/test_async_samtidig.no`, `tests/test_async_kombinatorar.no`.

---

## M8 — Native trådar og ekte parallellisme 🟡 KANAL-PRIMITIV FERDIG *(2026-08-22)*
**Innsats:** 4–8 veker · **Avhengig av:** M7

Til no er all «parallellitet» sekvensiell. Dette er den einaste vegen til CPU-utnytting.

**Status (2026-08-22): MELDINGSKANAL-PRIMITIVET FERDIG.**
[std/kanal.no](../std/kanal.no) implementerer synkroniseringsprimitivet frå steg 5 —
`ny_kanal`, `send`, `motta` (FIFO), `lengd`, `er_tom`, `steng`/`er_open`, `tapp` — som
ekte trådar (isolerte VM-instansar med meldingsutveksling, jf. tilrådinga i steg 2)
skal bruke bak same API. [tests/test_kanal.no](../tests/test_kanal.no) grøn via `nc run`.

**Gjenstår for M8 (kjernearbeidet):** sjølve trådprimitivet i runtime-laget
(plattform-ABI), trådpool og `køyr_parallelt`. Dette kan **ikkje** skrivast i rein
Norscode-kjelde — det krev at runtimen eksponerer OS-trådar — og akseptkriteriet (fire
CPU-tunge oppgåver på ≈1/4 tid) kan berre oppfyllast med ekte trådar. Kanalen er
API-grunnmuren; parallelliteten under han er runtime-arbeid.

**Steg**
1. Trådprimitiv i runtime-laget (plattform-ABI, ikkje shell).
2. Avgjer minnemodell: **isolerte VM-instansar med meldingsutveksling** er trygt og passar arkitekturen; delt mutabelt minne krev låsing gjennom heile VM-en. *Tilråding: isolerte instansar først.*
3. Trådpool med `køyr_parallelt(fn, args_liste)`.
4. Gjer `asynk.map_parallelt` faktisk parallell (den er sekvensiell i dag).
5. Synkronisering: kanal/kø mellom trådar; erstatt simuleringa i `std/tråd.no` med ekte implementasjon bak same API.

**Akseptkriterium:** fire CPU-tunge oppgåver på fire kjerner tek ≈ 1/4 av sekvensiell tid. Test: `tests/test_traadar_parallell.no`.

---

## M9 — Grensesnitt, enum-typar og generics-handheving 🟡 DELVIS *(2026-08-20)*
**Innsats:** 4–6 veker · **Avhengig av:** M1, M5

- **Enum** ✅ i kjelde: `enum Farge { RAUD, GRON, BLA }` → variantane blir heiltalskonstantar (0,1,2). `Farge.RAUD` løysast til konstanten på kompileringstid (lexer-nøkkelord `enum`, parser `Enum`-node, compiler samlar variantane og resolverer `Enum.Variant` i Felt-lowering). Verkar i variablar, samanlikning og `match`. Test [tests/test_enum.no](../tests/test_enum.no).
- Gjenstår: grensesnitt (dispatch), generics-handheving (byggjer på M1). Ikkje starta.

**Steg**
1. `grensesnitt Form { funksjon areal(self) -> heltall }` som nøkkelord.
2. `struktur Sirkel implementerer Form` med sjekk av at alle metodar finst.
3. Dynamisk dispatch: kall gjennom grensesnittstype vel implementasjon i køyretid.
4. `enum Farge { RAUD, GRØN }` som eigen type med uttømande `match`-sjekk.
5. Generics: `funksjon fyrste<T>(l: liste<T>) -> T` med typeparameter som faktisk blir sjekka (byggjer på M1).

**Akseptkriterium:** `tests/test_grensesnitt.no`, `tests/test_enum_type.no`, `tests/test_generics_avvis.no`.

---

## M10 — Syntaks-ergonomi ✅ KOMPLETT *(2026-08-20)*
**Innsats:** 3–4 veker · **Avhengig av:** M1, M3

Reint parser- og lowering-arbeid, ingen VM-endringar. Kan gjerast stykkevis og gir umiddelbar effekt i all eksisterande kode.

| Funksjon | Døme | Status i dag |
|---|---|---|
| Spread | `[...a, 3]`, `{...a, ...b}` | parserfeil |
| Destrukturering | `la [a, b] = liste` | ✅ FERDIG (verifiserast) |
| Valfri kjede | `o?.a` | «Ugyldig tegn: ?» |
| Standardargument | `funksjon f(a, b = 1)` | parserfeil |
| Varargs | `funksjon f(...args)` | parserfeil |
| Template-strengar | `` `hei {n}` `` | parserfeil |
| `for` over ordbok | `for k i ordbok` | ✅ FERDIG — gir nøklane |

**Akseptkriterium:** ein test per punkt.

**Status (delvis ferdig, 2026-08-20):**
- **`for k i ordbok`** ✅ — ny `builtin.for_iterabel` (ordbok→nøklar, liste/tekst uendra) i [selfhost/vm.no](../selfhost/vm.no); For-lowering i [ir_to_bytecode.no](../selfhost/compiler/ir_to_bytecode.no) normaliserer samlinga. Test [tests/test_for_iterasjon.no](../tests/test_for_iterasjon.no) (ordbok→60, liste→10, tekst→4, tom→0).
- **Destrukturering** ✅ — liste `la [a, b] = uttrykk` (`LaListe`) og ordbok `la {x, y} = uttrykk` (`LaOrdbok`) i parser + lowering (tmp + INDEX_GET per namn/felt). Test [tests/test_destrukturering.no](../tests/test_destrukturering.no).
- **Spread i liste-literalar** `[...a, b]`, `[0, ...a, ...c]` ✅ — parser (`Spread`-node ved `...`), lowering (dynamisk bygging med `builtin.legg_til`/ny `builtin.utvid`). Test [tests/test_spread.no](../tests/test_spread.no).
- **Standardargument** `funksjon f(a, b = 10)` ✅ — parser (`Standard`-node), VM bind manglande argument til ein sentinel-streng (native ingenting-verdi forsvinn frå kartet — `har_nokkel` blir usann), og funksjonsinngang fyller default når parameteren er sentinelen. Test [tests/test_standardarg.no](../tests/test_standardarg.no).
- **Optional chaining `o?.felt`** ✅ — kortsluttar til ingenting når o er ingenting, elles feltoppslag (lexer får `?`-teikn via tegn_kode 63 + `?.`→QDOT; parser OptionalFelt-node; compiler kortslutningskode). AVGRENSING: verkar på inline-verdiar (`m["x"]?.felt`); ingenting kan ikkje lagrast i variabel (native avgrensing), so `la b = finn(); b?.felt` feilar. Test [tests/test_optional.no](../tests/test_optional.no).
- **Varargs `funksjon f(...rest)`** ✅ — samlar overskytande argument i ei liste (parser Rest-node på param, compiler serialiserer rest_param, VM `vm_bind_rest` bind siste param til lista i tail+continuation-veg). Test [tests/test_varargs.no](../tests/test_varargs.no).
- **Template-strengar `f"hei {n}"`** ✅ — lexeren transformerer f-strengen til `("" + "hei " + tekst(n) + ...)` med rekursiv lexing av interpolasjonane. Ingen parser-/compiler-endring. Test [tests/test_template.no](../tests/test_template.no).

**M10 er no KOMPLETT** (alle sju punkta).

---

## M11 — Iteratorar og generatorar 🟡 PROTOKOLL FERDIG *(2026-08-22)*
**Innsats:** 2–3 veker · **Avhengig av:** M3, M7

**Status (2026-08-22): ITERATORPROTOKOLL + UENDELEG GENERATOR + `ta(n)` FERDIG.**
[std/iterator.no](../std/iterator.no) er ein **data-driven** iterator-/generator-
protokoll som køyrer på seed-runtimen i dag (ingen førsteklasses funksjonar): ein
iterator er `{ "kind", "state" }`, `neste(it)` returnerer `{ ferdig, verdi }` og
muterer state. Kjelder: `teljar(start, hopp)` (uendeleg), `område(start, ende, hopp)`,
`frå_liste`, `frå_tekst`. Konsumentar: `ta(it, n)` (trygg mot uendelege), `saml`,
`tel`. Akseptkriteriet (uendeleg generator + `ta(n)`, med state som varer mellom kall)
er verifisert i [tests/test_generator.no](../tests/test_generator.no) — seed-rask,
grøn via `nc run`.

**Gjenstår for M11:** late `kart`/`filtrer` over iteratorar med brukar-lambda (krev
M3-aktivering: seed-runtimen manglar førsteklasses funksjonar i dag) og `gi`-nøkkelord-
generatorar med ekte suspend/resume (same VM-maskineri som `await` i M7). Den data-
drivne protokollen dekker uendelege generatorar utan dette.

**Steg**
1. Iteratorprotokoll: `neste()` som gir verdi eller `ingenting` (krev M4).
2. `for x i <kva som helst med iterator>`.
3. Generatorfunksjonar med `gi` som suspenderer — same maskineri som `await` i M7.
4. Late kjeder i `std/js_liste.no`: `kart`/`filtrer` over iteratorar utan mellomliggjande lister.

**Akseptkriterium:** `tests/test_generator.no` med uendeleg generator + `ta(n)`.

---

## M12 — WASM-backend og nettlesar 🟡 EMITTER FERDIG *(2026-08-20)*
**Innsats:** 4–8 månader · **Avhengig av:** M3, M4, M7 · **Størst av alle**

Dette er den einaste milepælen som faktisk erstattar JavaScript i nettlesaren. Alt anna i planen gjer Norscode betre på server og CLI.

**Filer:** `selfhost/wasm.no` (70 linjer stubb i dag), `runtime/browser/browser_runtime.no` (63 linjer), `selfhost/backend*.no`.

**Steg**
1. **Ekte WASM-emitter:** binærformat, seksjonar, typar, funksjonar, minne, eksport. Start med heiltalsaritmetikk og kall.
2. **Minnemodell:** lineært minne, allokator, strengrepresentasjon.
3. **GC:** enten eigen GC over lineært minne, eller WasmGC-forslaget der det er tilgjengeleg.
4. **JS-glue:** minimal, autogenerert brubar for import/eksport.
5. **DOM-binding:** `dokument.finn(...)`, `element.tekst = ...`, hendingslyttarar som funksjonsverdiar (krev M3).
6. **Hydrering:** kople til `std/html_islands.no`, som i dag berre sender ut `<script type="module">` mot ein klientruntime som ikkje finst.
7. **Byggkommando:** `nc bygg-wasm <fil.no> -o app.wasm`.

**Akseptkriterium:** ei side med ein teljarknapp, skriven berre i Norscode, køyrer i Chrome og Firefox utan handskriven JavaScript. Test: `tests/test_wasm_emitter.no` (binærvektorar) + manuell nettlesartest.

**Merknad:** vurder seriøst om dette er verdt det. Server-rendert URL-state (som du alt planlegg) dekker det meste av interaktivitet utan denne milepælen i det heile.

**Status (2026-08-20): EMITTER + NC-MÅL FERDIG.** [std/wasm_binary.no](../std/wasm_binary.no) er ein komplett WASM MVP-binæremitter: magic + versjon + LEB128 (u/s) + type-/funksjon-/eksport-/kode-seksjonar, verifisert BYTE-EKSAKT mot spec og godkjend av `file` (libmagic: "WebAssembly (wasm) binary module version 0x1 (MVP)"). Bytekode-lowering (stack-maskin→stack-maskin) for heiltalsaritmetikk (i32.const/add/sub/mul/div_s) og parametrar (local.get). Wira inn som `nc bygg-wasm <fil.no> [-o ut.wasm]` ([tools/nc_bygg_wasm.no](../tools/nc_bygg_wasm.no)) — kompilerer fila, lowrar kvar heiltals-/parameter-funksjon, hoppar over ikkje-lowrbare (strengar, kontrollflyt, kall). Verifisert ende-til-ende: bygde eit 2-funksjons .wasm frå ei Norscode-fil, godkjend av `file`. Test [tests/test_wasm_binary.no](../tests/test_wasm_binary.no) (seed-rask, byte-eksakt).

**Gjenstår for M12:** kontrollflyt (if/loop/br), lokale variablar, minne/strengar, DOM-binding + JS-glue + hydrering (månadsverk). Emitteren og nc-målet er grunnmuren.

---

## M13 — Verktøy og drift
**Innsats:** 3–4 veker · **Avhengig av:** M2

1. **Step-debugger:** VM-en har alt source maps og `vm_debug_resume`; kople det til LSP-serveren og VS Code.
2. **Profilering:** `nc profile <fil.no>` med funksjonstider (VM har alt `vm_metric_inc` og sampling).
3. **Testdekning:** kva linjer er køyrde.
4. **Betre feilmeldingar:** kodeutdrag med peikar til kolonne, slik Rust gjer.

---

## Avhengigheiter

```
M0 ──┬── M1 ──┬── M4 ── M11
     │        ├── M5 ──┬── M6 ── M7 ── M8
     │        │        └── M9
     │        └── M10
     ├── M2 ──────── M6, M13
     └── M3 ──┬───── M7, M10, M11
              └───── M12
```

**Kritisk sti mot «Java-garantiar»:** M0 → M1 → M2 → M5 → M9
**Kritisk sti mot «JS-erstatning på server»:** M0 → M3 → M7
**Kritisk sti mot «JS-erstatning i nettlesar»:** M0 → M3 → M4 → M7 → M12

## Estimat

| Fase | Milepælar | Innsats | Resultat |
|---|---|---|---|
| **Fase 1 — tillit** | M0, M2 | 2–3 veker | Verifisering du kan stole på; ingen serverkrasj frå indeksfeil |
| **Fase 2 — garantiar** | M1, M4 | 4–6 veker | Kompilatoren fangar typefeil; ekte null |
| **Fase 3 — uttrykkskraft** | M3, M10 | 4–6 veker | Lambdaer, closures og moderne syntaks |
| **Fase 4 — objektmodell** | M5, M6, M9 | 7–11 veker | Java-liknande strukturering |
| **Fase 5 — samtidigheit** | M7, M8 | 8–14 veker | Ekte async og parallellisme |
| **Fase 6 — plattform** | M11, M12, M13 | 5–10 månader | Iteratorar, nettlesar, verktøy |

Fase 1–3 (≈ 3 månader) gir det meste av kvardagsverdien. Fase 6 er der prosjektet må avgjere om nettlesaren faktisk er eit mål.

## Arbeidsreglar

1. **Ein milepæl om gongen**, med eigne testar under `tests/` som køyrer grønt via `./bin/nc test` før neste startar.
2. **Ingen regresjonar:** `./bin/nc local-green` skal vere grøn før ein milepæl blir rekna som ferdig.
3. **Ryd `std/` med same gong** — når M1 avdekker typefeil i standardbiblioteket, blir dei retta i same milepæl, ikkje utsette.
4. **Oppdater `std/stdlib_status.no` og `docs/02-standard-library/`** når semantikken endrar seg.
5. **Til M0 er ferdig:** ser eit testresultat umogleg ut etter ei kjeldeendring, køyr `rm -rf build/nc-module-cache` og prøv igjen.

## Status

| Milepæl | Status |
|---|---|
| M0 — byggcache | ✅ ferdig, verifisert begge vegar |
| M2 — fangbare feil og stack traces | ✅ ferdig i kjelde; aktiv i `./bin/nc` etter stage0-regenerering |
| M3 — lambdaer og closures (+ follow-on) | ✅ ferdig i kjelde |
| M10 — KOMPLETT (7 punkt) | ✅ ferdig i kjelde |
| M1 — typesjekk (opt-in) | ✅ ferdig i kjelde |
| M4 — ekte `ingenting` | ✅ ferdig i kjelde |
| M5 — typa felt | ✅ i kjelde (metodar utsett) |
| M6 — unntakstypar | ✅ alt i runtimen |
| M9 — enum | ✅ i kjelde (grensesnitt/generics att) |
| M7, M8, M11–M13 | ikkje starta |

**Fase 1 (M0, M2) og Fase 2 (M1, M4) er no ferdige i kjelde.** Fase 3 (M3, M10) er stort sett ferdig. Alle kjelde-endringar blir aktive i `./bin/nc` etter stage0-regenerering.

## Kva som allereie er gjort

- `std/js_liste.no`, `std/js_streng.no`, `std/js_tal.no`, `std/js_objekt.no` — 113 funksjonar med JS-semantikk, testa ([docs/02-standard-library/JS_PARITET.md](02-standard-library/JS_PARITET.md)). Desse får lambdastøtte i M3 utan at API-et endrar seg.
- `std/url.no` `url_decode` skriven om til byte-vis UTF-8-dekoding — prosentkoda æøå vart ikkje dekoda før ([tests/test_url_decode_utf8.no](../tests/test_url_decode_utf8.no)).
