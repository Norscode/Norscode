# Status per 2026-09-23 kveld

**Ny reseed-commit ddd3dba (2026-09-24):** native `acme_verify` (RS256/ES256) + gap-rute
i native_codegen_v2. Den endrar `.srchash`, så den førehandsbygde codegen-ELF-en må
byggjast på nytt. Utan han er `builtin.acme_verify` null-fallback, og
`test_dns_zone_validation` feilar på verifiseringa sjølv når signeringa er rask.

- PR-greina `promotering-fersk-seed` er på **a0eab40**. CI-status der:
  - «Native Linux (x86_64)» feilar på 8 testar. 7 av dei er dekte av reseed-greina eller
    er arkitektur (§6). Unntaket er multiprocessing, som krev ekte x86 (§5).
  - «Linux runtime attestasjon» skal vere fiksa test-side i a0eab40 (§3).
- **Hent:** `git fetch origin reseed-batch-2026-09-23`. Cherry-pick 7f02f4e og 36b4a1d
  (ikkje denne docs-commiten) før reseed. Regenerer `vm_executor.ncb` og Windows `vm.ncb`.

## VIKTIG (2026-09-23 21:xx): 6a02eda er revertert i da8bf31 på PR-greina

Regenereringa av `vm.ncb` braut to portar:
- **test_precompiled_vm_host_compat:** manglar `["INDEX_SET"],["POP"]` (kompilatoren
  emitterte ikkje POP etter INDEX_SET, som gav stack overflow i L5b) og
  `builtin.har_nokkel` (normaliser_ncb vart ikkje køyrd).
- **Windows runtime ABI:** exe-en byggjer inn `vm.ncb`, men vart ikkje regenerert.

Gjer det slik saman med reseed:
1. `nc regen-bootstrap` (compile + `normaliser_ncb`) med kompilatoren som emitterer
   INDEX_SET+POP. Sjekk etterpå at `grep -c '"INDEX_SET"\],\["POP"\]'` er ≥ 1.
2. Regenerer `selfhost/vm_executor.ncb.json`.
3. `tools/build_windows_stage0_candidate.no` → committ ny
   `bootstrap/stage0/norscode-windows-x86_64.exe` og oppdater `SHA256SUMS`.

# Brief til sky-sesjonen: Linux-diagnose av PR #200 (frå Mac-sesjonen, 2026-09-23)

Seed som er målt: `270f792a` (2fc58bc), køyrd under Docker `--platform linux/amd64`
på Apple Silicon. **Emuleringa er Rosetta, ikkje qemu** (`/run/rosetta/rosetta` i
`/proc/<pid>/exe`), så gjesteadressene er dei same som på ekte x86.

## 1. Allereie pusha på `promotering-fersk-seed` (ingen reseed nødvendig)

| commit | testar | rot |
|---|---|---|
| 6e7f126 | test_helpdesk, test_norsdb_migrering, test_web_request_response | `liste["0"]` gjev null på fersk VM (C tvinga tekst til indeks); `json_parse` (legacy) gjev nøsta objekt som tekst → `json_parse_raw`; lokal `la tekst = …` skuggar `builtin.tekst` etter closures (CALL_VALUE) |
| 7751465 | test_native_module_globals_gate_contract, test_native_import_direct_module_contract | står på `krev_ny_seed`-lista (vart aldri køyrde på committa seedar); strengane forsvann i #181 / Fase C.3 |
| c717e80 | test_embed_release_ncb_idempotent (139) | `bytes_to_list` er identitet (tag 7); `slice` på bytes → tekst → `bytes_from_list(tekst)` → SIGSEGV i `fil_skriv_binær` |

Alle er verifiserte grøne via `tools/nc_test.no` på både Linux-seeden og macOS.

## 2. Reseed-batch: greina `reseed-batch-2026-09-23` (7f02f4e, oppå c717e80)

Cherry-pick før neste fullhost-reseed. **vm.no er endra → regenerer
`selfhost/vm_executor.ncb.json` og Windows sin innebygde `vm.ncb`.**
`native_codegen_v2.no` og `ncb_serde.no` er urørte, så `.srchash` står.

| fil | endring | test(ar) | korleis verifisert |
|---|---|---|---|
| vm.no | `builtin.sh.X` → `selfhost.common.X` i `finn_funksjon` | test_ir_snapshot_cases | tidlegare sesjon |
| vm.no | `vm_jit_fjern_daudkode` (alle numeric_kind) | test_vm_jit_integration (konstant_gren) | einingsprobe på begge plattformer |
| native_gap | JIT-backend avviser uoppnåelege ops att (revert deb2956) | test_native_jit_float (invalid_branch) | ikkje mogleg utan reseed (`sys6` finst berre i AOT) |
| vm.no | coop-`thread_spawn`: ukjend funksjon → `error` | test_thread_sync_lifecycle L6 | resten av testen er grøn når L6–8 blir hoppa over |
| native_gap | `_tls_handshake`: `shutdown(fd,2)` ved feil, **ikkje close** | Linux-attestasjon, test_native_tls_event_loop | sjå §3 |
| native_gap | `_tls_listen` fail-closed på manglande cert/nøkkel | test_tls_http (`lytt_secure == 1`) | kodegjennomgang |
| nc_main | `toolchain/` + rot-fallback i `_resolve_import_sti` | test_norspkg | root-compile reproduserte «tests/toolchain/... ikkje funne» |
| nc_main | `bygg-native` linux-x86_64 brukar hash-gata prebuilt codegen-ELF | test_linux_x86_64_aot_cli (124) | tolka: 11,5 min per fixture; prebuilt: 0,1 s; alle 5 fixtures gav exit 49/10/11/25/77 |
| runtime_filesystem_native | `event_backend` = epoll/kqueue/poll | test_native_filesystem_handles L19 | direkte import på begge |
| runtime_filesystem_native | `symlink_replace` avviser mål som går over rota | test_native_filesystem_symlink_live L28 | direkte import på begge |

## 3. TLS-badhost-stallen (attestasjonen er raud sidan 327b034): rota er pinna

Same probe på begge seedane (`build/tlstime.no`: eitt godt par og eitt `wrong.example`-par):

- 37cee6b: GOOD 25,6 s; BAD **14 s**, der tenaren får `tls peer closed`.
- 270f792a: GOOD 24,5 s; BAD **121 s**, der tenaren berre fell på `TimeoutFeil`.

37cee6b var bygd med close-on-fail (4ec1394). 86f31e6 reverterte det på grunn av
ein «uforklart hang». Den hangen var truleg dobbel close: `close(fd)` medan
handle-slot-en framleis eigde fd-nummeret, og så lukka `native_lukk` same nummer
etter at det var gjenbrukt. Fiksen i batchen brukar `shutdown`, så fd-en blir ståande
til `native_lukk`. Krypto har ikkje regressert (GOOD-para er like raske).

**Tillegg:**
- **Stadfesta dobbel close:** `_tls_lukk` fjerna aldri `_nw_slots[h]`, så eit andre
  `native_lukk` lukka same fd-nummer ein gong til. Fiksa i 36b4a1d på reseed-greina.
- **Test-sida (pusha a0eab40, verkar utan reseed):** `handshake_pair` køyrer planleggaren
  i 1 s-bitar og shutdown-ar sokkelen til den parten som feila. Heile
  `test_native_tls_event_loop` er då grøn på seed 270f792a på 1:59.

## 4. Høgast gevinst att: GC-bump-ratchet → collect-storm (krev codegen)

Målt ved å lese GC-hovudet frå `/proc/<pid>/mem` (skript: `gcpeek.sh` i patch-mappa). Ordlayout @0x780000:
bump, ?, port(=64 MiB), last, counter, fri-liste-hovud, ?, live.

- Isolert RS256-sign (`builtin.acme_sign`) tek 12 s, og bump veks likevel **~34 MB/s**:
  480 → 683 → 886 MB på 6 s, med `last == bump` etter kvar collect. Proben blir ferdig
  like under SOFT_LIMIT (966 MB).
- `test_dns_zone_validation` gjer akkurat same kall (L24 `dnssec_sign_rrset`), men startar
  med meir heap. Han kryssar SOFT_LIMIT og stod i >15 min (stoppa manuelt).
- Truleg same rot for timeoutane i dns_zone_validation, tls_acme_contract,
  https_front_live_multisite, selfhost_part_15, chunk_tail_part_15 og release_preflight
  (compile). Mål dette med gcpeek på ekte x86.

Forslag:
- (a) Rut dei gjenverande rå bump-allokatorane via `gc_alloc_var`: RT_STR_RAW/concat
  (sjå kommentaren på linje 48 i native_codegen_v2) og listevekst. Dette er det som
  ratchetar bump medan fri-lista står ubrukt.
- (b) Soft-limit-greina i safepoint-stubben (0x4069ad) skal krevje `bump - last ≥ budsjett`
  eller counter ≥ N. I dag samlar ho ved kvar safepoint.
- (c) Vurder storleiksklassar: first-fit er avgrensa til 512 blokkar.

**Tillegg (harness-målingar på seed 270f792a):**
- `test_dns_zone_validation` startar køyringa med ~395 MB bump og står fast på L24
  (`dnssec_sign_rrset` → RS256 i native_gap). Éi RS256-signering aukar bump med
  ~400 MB, fordi `std/bigint` lagar nye lister i kvar `mul`/`modulo`.
- Truleg same mekanisme for dei andre køyretids-timeoutane:
  - `tls_acme_contract` og `https_front_live_multisite` (RSA `localhost.key`);
  - `selfhost_part_15` og `chunk_tail_part_15` (tunge VM-køyringar).
- Utan GC-fiks hjelper det å redusere allokeringa i `std/bigint.modexp`: gjenbruk
  buffer, Montgomery og CRT. Modulen er baka inn via native_gap, så det krev òg reseed.

## 5. Må verifiserast på ekte x86

`test_multiprocessing_native` og `test_linux_arm64_runtime_attestation_probe` får begge
exit 126 med sandbox `no-network`: `seccomp()` returnerer ≠0 i barnet. Med
`none`-sandbox går det bra.

**Oppdatert etter CI-køyringa på c717e80:** `test_multiprocessing_native` er framleis raud
i «Native Linux» på **ekte x86**, så dette er *ikkje* berre ein Rosetta-artefakt.

Kva som er utelukka:
- BPF-programmet er dumpa byte for byte frå AOT-kode (`build/bpfdump.no`: kallar
  `ng._pg_bpf(scratch, "no-network")`, bygd med den førehandsbygde codegen-ELF-en).
  - 11 instruksjonar: `ld [4]`; `jeq 0xC000003E jt=1`; `ret 0x80000000`; `ld [0]`;
    `jeq 41 jf=4`; `ld [16]`; `jeq 1 jt=1`; `ret 0x00050001`; `ld [0]`;
    `ret 0x7fff0000`; `ret 0x00050001`.
  - `sock_fprog`: len=11 @+0, peikar @+8.
  - Alt er korrekt.
- `_pg_spawn` flyttar `fri` forbi filteret før røyr, og barnet gjer
  `prctl(38,1)` og så `seccomp(1,0,prog)`. Rekkjefølgja er rett.

Neste steg (krev ekte x86):
- Køyr `probe-multiprocessing-seccomp.no` under `strace -f -e trace=prctl,seccomp,execve`
  og sjå kva `seccomp` faktisk returnerer (EINVAL/EACCES/EFAULT?).
- Test òg om `!= 0`-samanlikninga av sys6-resultatet i barnet (etter fork) oppfører seg
  rett. Jf. minnet om boksa int-0 som ikkje var lik 0.

## 5b. VM-feil: modulglobal i feil modul etter kryssmodul-`prøv`

På seed 581bd059: i `tools/platform_readiness_v3600.no` feila
`_fil_info[sti] = info` med «Ukjent global variabel:
std.runtime_filesystem._fil_info» når førre setning var
`prøv { native_fs.native_stat_sti(sti) } fang (e0) {}`. Same tilgang fungerte når
nokre andre kall kom imellom. Truleg blir modulkonteksten for LOAD_GLOBAL/STORE_GLOBAL
ståande på modulen til kallet inne i `prøv` etter handler-regionen. Omveg i 66563e1:
bind den globale lokalt før kallet. Ekte fiks høyrer til i vm.no (modulkontekst ved
try-exit).

## 5c. Motstridande kontraktar for `vm.ncb` (open avgjerd)

`test_precompiled_vm_host_compat` krev `["INDEX_SET"],["POP"]` og
`["CALL","builtin.har_nokkel",2]` i `bootstrap/precompiled/vm.ncb.json`. Men den
offisielle regenereringa (`nc_regen_bootstrap` → `normaliser_ncb` →
`ncb_normalize_builtin_aliases_v802.no`, som begge er kontrakttesta) fjernar nettopp
`POP` etter `INDEX_SET` («eldre host-VM») og byter `builtin.har_nokkel` →
`selfhost.vm.vm_map_har_nokkel`. Begge kan ikkje vere grøne via det offisielle
verktøyet.

- Historikk: 35d111c (utan POP) → d728347 (vanleg compile *utan* normalisering, med
  POP) → 6a02eda (utan) → da8bf31 (med) → d3a0901 (utan). Det flip-floppar.
- Med normalisert form (d3a0901) er Windows ABI, Native macOS, Steg C bootstrap-self og
  alle andre Linux-testar grøne. Berre host-compat-testen er raud.
- Semantikk: `vm.no` sin INDEX_SET pushar objektet att, og kompilatoren emitterer POP.
  Utan POP lek stakken per tilordning (d728347: stack overflow i djup L5b); med POP på
  ein host som ikkje pushar, underflow.
- Avgjerd trengst: anten (a) vanleg compile utan normalisering + byggj Windows-exe på nytt
  (lokalt stormar verktøyet på 8,8 MB-exe-en, så gjer det i CI eller på ekte x86), eller
  (b) oppdater host-compat-testen til det normaliserte formatet og ta bort POP-kravet.

## 5d. macOS: førehandsbygd macho-codegen manglar

`bygg-native --target macos-arm64` tolkar heile `macho_arm64_codegen.no` frå kjelde:
588 s lokalt (42 % CPU) og over 600 s på CI-macOS. `test_arm64_skriv_int_aot` har fått
1100 s (ad7222f → neste commit) som mildning. Rett fiks er ein førehandsbygd macho-codegen
med kjeldehash-port, som `bootstrap/native_codegen_x86_64.elf` og
`nc_bygg_native_prebuilt_x86`.

## 6. Arkitektur (ikkje patchar)

- **thread_pool** og **fjern_agent_loopback** krev at VM-funksjonar køyrer *samstundes*
  med hovudtråden. `pool_blokker` spinn medan main pollar «running», og agenten må
  akseptere medan main er klient. Coop-spawn køyrer synkront og kan ikkje gjere dette.
  `builtin.thread_pool` finst ikkje nokon stad.
- **tensor_operation** manglar på fersk seed (null-fallback → tom map). C-kontrakten
  frå 8343f3e:`nc_native_main.c` gjeld:
  - `status` → `backend` `simd-x86_64-sse4.1` (CPUID.1:ECX bit 19), elles `scalar`;
    `vector_lanes` 4/1; `metal_available`.
  - `matmul_i32`: grenser rows/inner/cols ≤ 4096, r·c ≤ 1e6, r·i·c ≤ 2e7; verdiar i
    int16-området (`[40000]` → feil); int64-akkumulering.
  - Kan skrivast i native_gap med `builtin.raw_call` og ein SSE4.1-kjerne
    (pmulld + pmovsxdq + paddq), slik JIT-en alt gjer. Ikkje rapporter `simd-` for ein
    skalar veg.
- **bytes**: `bytes_to_list` = identitets-atom, og RT-`slice` behandlar tag 7 som tekst.
  c717e80 går rundt det i embed-verktøyet. Den ekte fiksen er at atomet lagar ei
  tag-3-kopi og at `slice` handterer tag 7.
- **RSA**: 12 s per 2048-bit-signering under Rosetta (`std/bigint` base 2^16, ny liste
  per steg, ingen CRT). Montgomery med 30-bits limbar + CRT, eller eit native
  modexp-atom.

## 7. Lokale feller (til eiga diagnose)

- `build/nc-test-cache`-nøkkelen er plattformuavhengig. Deler du worktree mellom macOS
  og Linux-Docker, køyrer Linux ein mac-kompilert NCB. Tøm cachen når du byter.
- `bin/nc run` på macOS gjev 404 på web-ruter (route-metadata på direkte-vegen), men
  harnessen er grøn. Bruk harnessen.

## Vedlegg: gcpeek (les GC-hovudet @0x780000 frå /proc/<pid>/mem i Docker)

```bash
#!/bin/bash
# usage: gcpeek.sh <file.no> <delay_first> <interval> <count>
SP=${SP:-.}
cd $SP/macos-wt2
docker run --rm --platform linux/amd64 --cap-add SYS_PTRACE --security-opt seccomp=unconfined -v "$PWD":/repo -v "$SP/gcdiag/dist-linux2":/repo/dist -w /repo -e F="$1" -e D="$2" -e I="$3" -e N="$4" ubuntu:24.04 bash -c '
NORSCODE_ROOT=/repo NORSCODE_VM_CAPABILITIES=env.read,disk.read,disk.write,net.dns,net.tcp,net.connect NORSCODE_VM_DISK_ROOT=/repo,.,/tmp ./bin/nc run $F > /tmp/o.txt 2>&1 &
sleep $D
P=$(for p in $(ls /proc | grep -E "^[0-9]+$"); do tr "\\0" " " < /proc/$p/cmdline 2>/dev/null | grep -q "nc run $F" && [ "$(awk "/VmRSS/{print \$2}" /proc/$p/status)" -gt 100000 ] && echo $p; done | head -1)
k=0; while [ $k -lt $N ] && [ -e /proc/$P ]; do
  echo "t+$((D + k*I))s: $(dd if=/proc/$P/mem bs=8 skip=$((0x780000/8)) count=8 2>/dev/null | od -A n -t x8 | tr -s " " | tr "\n" " ")"
  k=$((k+1)); sleep $I
done
kill %1 2>/dev/null; head -2 /tmp/o.txt'
```
