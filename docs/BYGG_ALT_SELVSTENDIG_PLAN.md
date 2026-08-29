# Byggeplan: bygg *alt* i rein Norscode utan å bryte 100 % sjølvstende

**Mål:** produsere alle leveransar (kompilator/seed på fire plattformer + TLS/krypto
+ release-attestasjon + Docker/pakking) der **kvart** byggesteg går gjennom den reine
Norscode-stien — null nytt kall til gcc/clang/zig/`as`/`ld`/OpenSSL/SQLite/python/node.

Denne planen er **operasjonell** (bygg + portar). Djupt språk-/seed-arbeid er
kanonisert i:
- [MASTERPLAN_100_SELVSTENDIG.md](MASTERPLAN_100_SELVSTENDIG.md) — Omgang 0–19
- [SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md](SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md) — Fase 0–8
- [SELVSTENDIGHET_SLUTTPLAN.md](SELVSTENDIGHET_SLUTTPLAN.md)

Ei oppgåve blir **berre** avhuka når den levande porten er grøn (same regel som
arbeidsloggen). Dokumentasjon markerer aldri noko ferdig utan gatebevis.

---

## Definisjon: «ikkje brote sjølvstende»

Etter *kvart* byggesteg må desse haldast:

- [ ] **Aktiv-flate-port grøn:** `./bin/nc run tools/no_c_python_active_surface.no` → OK
      (0 `.c/.h/.py`/shell-python utanfor archive + allowlist)
- [ ] **Ingen legacy-C-VM:** `./bin/nc run tools/no_legacy_cvm.no` → OK
- [ ] **Overflate-eigarskap:** `./bin/nc run tools/verify_norscode_surface_ownership.no` → OK
- [ ] **Streng sjølvstende:** `./bin/nc run tools/verify_selvstendighet.no` → OK
- [ ] **Fixpunkt (ELF):** `./bin/nc selfcompile-stage0-elf` → Gen1 == Gen2 byte-identisk
- [ ] **Ingen ny framand prosess:** byggesteget kallar ikkje gcc/clang/zig/openssl/python/node
      (kryssjekk `prosess.køyr(...)`-argv i det nye verktøyet)

> Grunnlinje frå revisjon 2026-08-29: kjerneflata er alt rein. **Éin** levande
> framand avhengnad står att (Blokk B) + broten/forelda Dockerfile (Blokk A).

---

## Blokk 0 — Fastslå grunnlinje *(gjer først, ingen risiko)*

- [x] Køyr alle seks portane på `b2-independence` (macOS-host, 2026-08-29). Resultat:
      - **Gate 2** `no_legacy_cvm` → 🟢 GRØN
      - **Gate 1** `no_c_python_active_surface` → 🔴 (falsk-positiv arm64-attest + >10 min treg på host)
      - **Gate 3** `verify_norscode_surface_ownership` → 🔴 (ukommitterte `.sh` utan `.no`-eigar)
      - **Gate 4** `verify_selvstendighet` → 🔴 (11 committa `.bak_`, `.wrapper`, ukommitterte `.sh`,
        arm64-attest falsk-positiv, windows-zig-daudkode, legacy `exec_prosess`, CI shell-blokker)
      - **Gate 5** `selfcompile-stage0-elf` → 🟡 DELVIS (Gen1 ELF bygd; Gen1==Gen2 krev Linux x86-64)
      - Substans intakt: 🟢 ingen nye `.c/.h`, 🟢 ingen Python i `tools/`, 🟢 ingen `sh -c`/`find`/`tar`/`gzip`
- [x] **Fiks (2026-08-29):** la `arm64-native-attest.yml` på ref-allowlista i
      `tools/no_c_python_active_surface.no` → fjernar falsk-positiven frå gate 1 **og** 4
      (verifisert: ingen andre ikkje-allowlista ref-treff att)
- [x] Stadfest at `linux-arm64-runtime-attestation` (i [ci.yml](../.github/workflows/ci.yml)) er
      den einaste push-jobben som kallar `/usr/bin/gcc` (grunnlinje-funn — **stadfesta**)
- [x] Stadfest x86_64-attesten er rein: `tools/build_linux_attestation_candidate.no` (**stadfesta rein**)
- [x] **Hygiene-opprydding (2026-08-29):** `git rm` 10 ureferert `selfhost/*.bak_*` (behaldt
      `bak_cookie_header_fix_v681` — kopla til `rollback_serve_runner_cookie_fix_v682.no` som
      `release_preflight` + 2 testar attesterer) + 2 ureferert `tools/*.wrapper` (shell-shim;
      `.no`-eigarane intakte). Fjernar backup- og `.sh`-flagg frå gate 3/4.

---

## Blokk A — Trygge oppryddingar NO *(ikkje blokkert av seed)*

### A1 — Fiks Dockerfile (broten + forelda) ✅ *delvis gjort 2026-08-29*
`Dockerfile` kunne ikkje byggje: duplisert `FROM … AS runtime`,
`FROM python:3.12-slim` (daud), `libsqlite3-0` (valfri — NorsDB har erstatta SQLite),
`COPY --from=build` mot eit stage som ikkje finst.

- [x] Skriv om til rein `build`→`runtime` fleirstegsstruktur (`debian:bookworm-slim`)
- [x] Fjern `FROM python:3.12-slim` heilt — **verifisert: `command -v python` → INGEN**
- [x] Fjern `libsqlite3-0` frå `apt-get` — **verifisert: binæren lenkar ikkje sqlite**
- [x] Port: `docker build --platform linux/amd64` grøn (exit 0); ingen python i biletet
- [ ] **BLOKKERT AV BLOKK B (nytt funn):** den committede `bootstrap/stage0/norscode-linux-x86_64`-seeden
      er sjølv dynamisk lenka mot `libssl.so.3`/`libcrypto.so.3` (`ldd` stadfesta). Difor
      dreg biletet framleis inn OpenSSL via `ca-certificates`. Fell vekk når den reine
      full-host Linux-binæren blir promotert (B3/B4).
- [ ] **BLOKKERT AV BLOKK B:** seeden er env-driven (NORSCODE_CMD/NORSCODE_FILE), køyrer
      berre selftest på rå argv → `ENTRYPOINT nc serve` blir først funksjonell etter
      full-CLI-binær (Omgang 15). Fram til då: driv serving via env (dokumentert i Dockerfile).

### A2 — Slett daudkode-kryssbygg (zig) ✅ *gjort 2026-08-29*
`tools/windows_runtime_cross_compile_gate.no` kalla `zig build-obj` mot `archive/legacy_c_backend`,
men **ingen** workflow refererte den (live Windows-release brukar `build_windows_stage0_candidate.no`).

- [x] Stadfest 0 levande referansar (berre docs nemnde den)
- [x] `git rm tools/windows_runtime_cross_compile_gate.no` (161 linjer)
- [x] Verifisert: einaste gjenverande `zig`-referansar i `tools/` er **attest av fråvær**
      (`release_preflight.no`) + byggartefakt-stiar (`platform_readiness_v3600.no`) — ingen zig-kall
- [x] Fjernar eitt reelt flagg frå gate 4 sin «legacy tekstprosess-kall»-liste

### A3 — Merk opt-in framand tooling eksplisitt ✅ *gjort 2026-08-29*
Desse er valfrie og matar ikkje kjeda, men bør vere tydeleg utanfor purity-porten.
Alle tre hadde alt `.no`-eigar/README; A3 la til eksplisitt **sjølvstende-framing**.

- [x] `platform/README.md`: la til «Sjølvstende»-notat — window-host ([Main.swift](../platform/macos/window-host/Main.swift))
      er opt-in GUI-pakking; kjeda treng null av det
- [x] `tools/render-norscode-mark.swift`: header-kommentar «OPT-IN dev-/branding-hjelpar (ikkje runtime)»
- [x] `vscode-norscode/README.md`: «Self-sufficiency note» — opt-in editor-tooling utanfor kjeda
- [x] (Valfritt) ~~flytt til eige `tooling/`-tre~~ → **valde deklarasjon i staden**
      (2026-08-29): fysisk flytt hadde ~35 referansar (byggelane + CLI-dispatch +
      attest-testar) med reell CI-risiko for kosmetisk gevinst. I staden: ny
      maskin-sjekka deklarasjon i `tools/no_c_python_active_surface.no`
      (`collect_foreign_source_hits` + `deklarert_opt_in_kjelde`) som slår fast at
      **einaste framand programkjelde i aktiv flate er 4 deklarerte opt-in-filer**;
      ny fil feilar porten. Gjev «100 % `.no` utanom deklarert opt-in»-garanti, låg risiko.
      - Disk-skann (walk_active_files-eksklusjonar) stadfesta nøyaktig 4: `Main.swift`,
        `render-norscode-mark.swift`, `install.ps1` (Windows-installer, `.no`-eigar),
        `native_metal_gpu_smoke.m`.
      - **Slettekandidat:** `tools/native_metal_gpu_smoke.m` — Metal-GPU-gaten er alt fjerna
        frå CI (commit ded0be4); .m-fila + `test_native_metal_gate_structured_process.no`
        heng att som forelda par (som rollback-tilfellet). Eiga oppgåve å fjerne begge.

### A4 — Gate-herding mot regresjon ⏳ *item 2 gjort 2026-08-29; item 1 er repo-admin*
- [x] **Framand-verktøy-kall-allowlist (item 2):** ny port i `no_c_python_active_surface.no`
      (`collect_framand_verktoy_kall` + `framand_verktoy_kall_tillate` + `har_framand_verktoy_kall`)
      som feilar viss ei aktiv `.no` kallar gcc/clang/cc/zig/codesign/xcrun/security utanfor
      allowlista (11 deklarerte: gcc-attest Blokk B, macOS sign/verify/notarize/build, + attest/test
      som berre nemner stien). `nc check` grøn; fullstendig rot-søk stadfesta at settet er ⊆ allowlist.
      Når B4 slettar gcc-attesten → fjern den frå allowlista → **0 gcc-kall att**.
- [x] **Framand-programkjelde-deklarasjon (frå A3):** same gate slår fast 100 % `.no` utanom 4 opt-in.
- [ ] **Påkravde status-checks på `main` (item 1) — REPO-ADMIN, ikkje kode:** både gatene køyrer alt
      i CI via `selvstendighet.yml`, så CI går raud ved regresjon. Å BLOKKERE merge krev ei
      GitHub branch-protection-regel (Settings → Branches, eller `gh api`). **Vent til gatene er
      grøne** — dei er raude no på WIP-greina (py i `build/`, CI-shell-blokker, ukommitterte `.sh`);
      krav-check før grønt ville blokkere alle merges. Brukar-handling.

---

## Blokk B — Den einaste ekte blokkeraren: rein ARM64 full-host *(Fase 8 / Omgang 8)*

Rota: AArch64-codegen er berre ein liten heiltals-AOT utan runtime; berre x86-64 har
full NcVal-runtime. Difor byggjer `linux-arm64-runtime-attestation` framleis
`build/v3009/native_candidate_gc.c` med `/usr/bin/gcc -lssl -lcrypto`. Sjå
[SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md](SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md).

### B1 — Fullfør AArch64 full-host runtime (Fase 8)
- [ ] Kompiler heile levande `nc_main` til AArch64 via kjelde-codegen (ikkje gcc, ikkje gammal seed)
- [ ] Rein native TLS/krypto-flate over syscall-socket (D2) — ingen OpenSSL-lenking
- [ ] Port: `nc bygg-native --target linux-arm64 <nc_main>` → køyrande binær, selftest exit 0

### B2 — Fersk ARM64-seed frå gjeldande kjelde
Blokkeringa for [arm64-native-attest.yml](../.github/workflows/arm64-native-attest.yml):
den committede arm64-stage0-seeden er for gammal (manglar 2026-08-codegen-rettingar).

- [ ] Bygg fersk `norscode-linux-arm64`-seed på ekte `ubuntu-24.04-arm` frå kjelde
- [ ] Commit ny seed + SHA256SUMS i `bootstrap/stage0/`
- [ ] Fixpunkt-port: fersk seed byggjer seg sjølv Gen1 == Gen2 (arm64)

### B3 — Byt attest-lana til rein, fjern gcc-jobben
- [ ] Gjer `arm64-native-attest.yml` (kjelde-codegen, utan gcc/OpenSSL) grøn end-to-end på push
- [ ] Fjern `linux-arm64-runtime-attestation`-gcc-jobben frå [ci.yml](../.github/workflows/ci.yml)
- [ ] Slett `tools/build_linux_arm64_tls_attestation_candidate.no`

### B4 — Slett legacy C-backend (Omgang 10–11)
- [ ] Slett `-DNC_ENABLE_OPENSSL -DNC_DYNAMIC_SQLITE`-kandidatane + `build/v3009/*.c`
- [ ] `git rm -r archive/legacy_c_backend`
- [ ] Rydd attest-/kontrakttestane som *les* C-arkivet (dei blir overflødige)
- [ ] Port: aktiv-flate + `no_legacy_cvm` grøn utan C-arkivet på disk

---

## Blokk C — Bygg *alt* på fire plattformer via rein sti

Kvar plattform: bygg frå kjelde → køyr → signert build-provenance → reproduserbar.

- [ ] **Linux x86-64** — `build_linux_attestation_candidate.no` (alt rein) → attestert
- [ ] **Linux ARM64** — etter Blokk B → attestert utan gcc
- [ ] **macOS ARM64** — Mach-O via `macho_arm64_codegen`; signering (`codesign`/`xcrun`)
      er opt-in distribusjonssteg, ikkje ein byggeavhengnad
- [ ] **Windows x86-64** — PE-emitter i rein Norscode (Omgang 9) erstattar `zig cc`
- [ ] Vurder å trimme macOS-runtime-lenking (Security/CoreFoundation/libobjc → berre
      `libSystem`/`dyld`) så committed macOS-binær er nærare syscall-berre
- [ ] Port per plattform: byggjer frå kjelde + køyrer + `attest-build-provenance` grøn

---

## Blokk D — Lås inne (Omgang 19)

- [ ] Aktiv-flate-port: **0** `.c/.h/.py/.sh`/Zig i aktiv verktøykjede
- [ ] Ingen framand prosess i nokon push-CI-jobb (gcc/clang/zig/openssl/python/node)
- [ ] Reproduserbarheit: regenerer 2× → byte-identisk; seed byggjer seed (alle 4 plattformer)
- [ ] Signert attestasjon: macOS + Linux x86-64/ARM64 + Windows
- [ ] ELF/Mach-O/PE stage-0-fixpunkt grøn med full-paritet-seed
- [ ] Oppdater minne + [MASTERPLAN_100_SELVSTENDIG.md](MASTERPLAN_100_SELVSTENDIG.md) Omgang 19

---

## Avhengnadsgraf (rekkefølgje)

```
Blokk 0 ──▶ Blokk A  (parallelt, no)
              │
Blokk B1 ▶ B2 ▶ B3 ▶ B4   (seriell — seed-blokkert)
              │
              ▼
          Blokk C  (Linux-arm64 ventar på B; resten kan gå parallelt)
              │
              ▼
          Blokk D  (lås — når alt over er grønt)
```

**Kritisk sti:** B1 → B2 → B3 → B4. Alt i Blokk A + Linux-x86/macOS/Windows i Blokk C
kan gjerast no utan å vente på seed.
