# Milepæl B – Rene native bygg på alle plattformer: køyrerunbok

Konkret, ordna kommandoliste for å fullføre Milepæl B i
[SELVSTENDIGHET_SLUTTPLAN.md](SELVSTENDIGHET_SLUTTPLAN.md). Mål: byte-identisk
Gen1/Gen2 og signert attestasjon på macOS ARM64, Linux x86-64, Linux ARM64 og
Windows x86-64 for **same** generasjon, utan GCC, Zig, C-bundlarar eller Docker
som byggeverktøy i normalflyten.

**Målast mot generasjon G-A** (`main@52593ad9`, jf.
[GENERATION_G-A_MANIFEST.md](GENERATION_G-A_MANIFEST.md)). Endrar kjelda seg, må
G-A oppdaterast og B køyrast på nytt (regelen om sannferdige portar).

## Kvifor kjernen ikkje kan køyrast frå Cowork-sky-økta

B krev native bygg og køyring på **fire** plattformer + signert attestasjon på
ekte host/CI. Sky-økta har berre den innsjekkede Linux ARM64-stage0-en (som køyrer
enkeltfiler via miljøkontrakt) og manglar macOS/Windows/x86-64-runnarar, GitHub CI
og signeringsinfrastruktur. Sky-økta kan likevel verifisere kjeldeflate-portane
under; B1–B4 sjølve må køyrast på host/CI.

## Sandkasse-verifisert utgangstilstand 2026-08-07

Køyrt direkte i sky-økta på committed Linux ARM64-stage0:

- **Active-surface-porten er grøn** (`tools/no_c_python_active_surface.no`):
  - ingen `.py`-filer utanfor `archive/`
  - ingen nye `.c/.h`-filer utanfor allowlist
  - ingen `ncb_to_c`-referansar
  - 0 shell-eksekverte run-blokker i aktiv CI; barneprosessar styrt via
    `NORSCODE_VM_CI_CHILD_CMD/FILE` med eksplisitt executable
  - ingen `sh -c`, `rm -rf`, `find`, `gzip`, `tar` som kommando i aktiv kjerneflate
- **Kjeldeflate-audit** (find, heile repo utanom `archive/`): **0** `.py`-filer;
  ingen `.c/.h/.zig` i `tools/ selfhost/ bootstrap/ bin/ cli/ compiler/`.
  (352 `.c`-filer finst, men alle under `build/` som **genererte artefaktar**,
  ikkje kjelde i byggbanen — sjå merknad under.)
- **Normal Linux x86-64-byggbane er GCC-fri:** `tools/release_preflight.no`
  handhevar at `tools/build_linux_openssl_candidate_v3604.no` ikkje inneheld
  `/usr/bin/gcc` («Linux full-host brukar ikkje GCC»).

## Konkrete B-restansar identifiserte i sky-økta

1. **B2 (ARM64 GCC-overgang står att):** `tools/build_linux_arm64_tls_attestation_candidate.no`
   kallar framleis `/usr/bin/gcc` (linje ~109). Dette er nett den historiske
   GCC/C/OpenSSL-TLS-overgangen B2 skal erstatte med same native codegen-bane som
   x86-64. Så lenge denne banen brukast for ARM64 TLS-attestasjon, er B2 ikkje
   lukka.
2. **Docker-bruk må klassifiserast:** `Dockerfile` og `Dockerfile.linux-build`
   finst. B tillèt Docker som **rein attestasjonssandkasse**, men ikkje som
   byggkrav. Stadfest på host at ingen normal byggbane krev `docker build`
   (ingen `docker build`/`buildx` vart funne i `.github/workflows/`).
3. **`build/`-genererte `.c`-artefaktar:** avklar på host om desse er levningar
   frå ein tidlegare C-codegen-veg eller blir regenererte av gjeldande native
   codegen. Normalkandidaten skal vere «source-only, native codegen, utan
   precompiled maskering» (B1).

## Empirisk stadfesta i sky-økta 2026-08-07 (kvifor B1–B4 ikkje kan lukkast her)

Køyrt direkte på committed Linux ARM64-stage0:

- **Native gap-/readiness-portane krev host-eksekvering.** `native_runtime_gap_gate_v3001`
  stoppar på `manglar capability process.exec` (porten vil *køyre* den native
  binæren), og `native_runtime_gap_pure_vm_gate_v3002` på `disk.write outside
  scope`. `dist/norscode_native` er dessutan ein **Mach-O ARM64** (macOS) som
  ikkje kan køyre på Linux. Portane er difor konstruerte for å køyre på det native
  vertsanker-oppsettet, ikkje i sky-økta. Dette er sjølve grunnen B1–B4 må gå på
  host/CI.
- **Docker er klassifisert (ikkje eit byggkrav):**
  - `Dockerfile.linux-build` er **RETIRED** — han `exit 1` med melding om å bruke
    `tools/build_norscode_native.no`. Ingen byggfunksjon.
  - `Dockerfile` byggjer via `./bin/nc run tools/build-bootstrap-binary.no`
    (Norscode-eigd), ikkje GCC/Zig. Det siste steget brukar `python:3.12-slim`
    berre som *køyre*-basisimage for `nc serve` (pakking), og trekkjer inn
    `libsqlite3-0` — SQLite-avhengigheita som D3 skal gjere valfri. Ingen
    `docker build`/`buildx` i `.github/workflows/`.
- **B2-restansen er presist lokalisert:** `tools/build_linux_arm64_tls_attestation_candidate.no`
  kompilerer `build/v3009/native_candidate_gc.c` med
  `/usr/bin/gcc -O2 -DNC_ENABLE_OPENSSL -DNC_DYNAMIC_SQLITE -Iarchive/legacy_c_backend
  … -lssl -lcrypto -lpthread -ldl -lm`. Dette er heile GCC + OpenSSL + C-backend-
  overgangen for ARM64. `build/*.c`-artefaktane (352 filer) er nett denne
  C-codegen-backenden. Med D2 (rein Norscode TLS: X25519/Ed25519/ChaCha20-
  Poly1305/HKDF + RSA/ECDSA) finst no krypto-alternativet som trengst for å bytte
  bort `-lssl -lcrypto`; sjølve ombygginga til native ARM64-codegen må likevel
  byggjast og verifiserast på ekte ARM64-host.

## Steg B0 – Frys generasjonen (same som G-A)

```bash
cd <repo>
git status && git rev-parse HEAD    # skal vere G-A: 52593ad9…
```

## Steg B1 – Linux x86-64 source-only-kandidat + byte-identisk Gen1/Gen2

```bash
./bin/nc run tools/build_linux_native_candidate_v3005.no      # source-only, native codegen
./bin/nc run tools/native_runtime_gap_gate_v3001.no           # runtime-gap grøn på kandidaten
# Gen1/Gen2-paritet: bygg to gonger (cache på, så av) og samanlikn hash
```

Krav: 947-funksjons source-only-kandidat blir normal byggveg i CI på **ekte
Linux x86-64-runner utan Docker som byggverktøy**; `builtin.process_spawn_argv`
og heile prosess-ABI-en grøn på L5/L5b (ikkje berre testfila); byte-identisk
Gen1/Gen2 med cache på og av.

## B2-avhengigheit oppdaga 2026-08-08 (viktig rekkefølgje)

Nærare analyse i sky-økta viser at B2 **ikkje** kan lukkast ved berre å fjerne
GCC/OpenSSL frå `build_linux_arm64_tls_attestation_candidate.no`:

- Den normale native-binæren byggjast utan kompilator — `build_norscode_native.no`
  materialiserer den committede stage0-seeden og røyktestar (ingen GCC). Dei
  arkiverte `v3005`/`v3002`-verktøya returnerer `exit 2`.
- Den committede ARM64-stage0 **støttar** `run-ncb` (bekrefta i binæren og i
  `selfhost/nc_main.no` §3229), så seed-materialisering er teknisk mogleg.
- **Men** ARM64 TLS-attestasjonsprøva (`NORSCODE_ARM64_PROBE_SECTION: tls`) køyrer
  med `net.tls`-capability — native-socket-TLS. Det var OpenSSL-drive. Fjernar ein
  `-lssl -lcrypto`, må prøva tilfredsstillast av **rein Norscode-TLS over det
  native socket-laget**.
- Rein Norscode-TLS er ferdig og CI-grøn som std-modular i minnet (D2-kjernen,
  Krypto-smoke run 31245505530), men er **ikkje kopla inn i det native socket-
  laget** — det er D2 «socket-integrasjon», som framleis står open.

**Konklusjon:** B2 er blokkert på D2 socket-integrasjon. Rett rekkefølgje er:
(1) kople rein Norscode-TLS inn på native socket-ABI (D2 socket-integrasjon,
verifisert på host), deretter (2) bytt ARM64-attestasjonskandidaten frå
GCC/OpenSSL til den native seed-en + rein TLS. Å fjerne OpenSSL før (1) gjer
TLS-prøva raud.

**Djupare blokkering oppdaga 2026-08-08 (endrar rekkefølgja):** sjølv før
D2-socket-spørsmålet manglar AArch64-kodegeneratoren ein **full-host runtime**
heilt. `native_codegen_v2.no` (x86-64) har ein handemittert NcVal-runtime;
AArch64-sida (`macho_arm64_codegen.no` + `elf_arm64_emitter.no`) er berre ein
liten heiltals-AOT (ingen heap/strengar/lister/kart/sockets/TLS). Ein rein ARM64
full-host er difor ikkje produserbar før den runtimen er skriven. Den fasa
køyreplanen for å byggje han (med lokal Apple Silicon-verifikasjon per fase) ligg
i [SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md](SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md).
B2 lukkast i Fase 8 der; D2 socket-integrasjon kjem inn i Fase 7–8.

## Steg B2 – Linux ARM64 utan GCC-overgang

1. Erstatt `/usr/bin/gcc`-kallet i `tools/build_linux_arm64_tls_attestation_candidate.no`
   med same native codegen-bane som x86-64 (restanse 1 over).
2. Løys 8,7 GiB-RSS-toppen ved isolert/strøymt modulmaterialisering (same
   fragmentmønster som L5b brukar), slik at ARM64-kandidaten byggjast utan
   precompiled maskering.
3. Byte-identisk Gen1/Gen2 på Linux ARM64 + signert attestasjon på **ekte
   ARM64-host**.

## Steg B3 – ABI-restansar i tillitsankeret

```bash
./bin/nc run tools/promote_native_stage0_v3001.no    # kandidat → smoke → matrise → atomisk promotering
```

- Promoter ein stage0 som eksponerer binær range-read/slice, `chmod`/filmodus og
  `rmdir` (fjernar `/bin/chmod`-reservane + Windows-tempopprydding-åtvaringa).
- Fullfør Windows-thread-backend + freestanding ARM64/Windows-atomics
  (`norscode-native-thread-v1`/`norscode-atomic-v1`).
- Attverande FS/nett/tryggleik-ABI: Windows HANDLE, hard Darwin-minnegrense,
  seccomp/AppContainer-detaljar.
- Kvar ny ABI følgjer promoteringsregelen: kandidat → levande smoke → full
  matrise → kontrollert atomisk promotering med hashverifisert rollback.

## Steg B4 – Samla plattformbevis

```bash
gh workflow run "CI"                     # samlar attestasjonar frå alle fire plattformer
./bin/nc run tools/platform_readiness_v3600.no    # fail-closed samla rapportport
```

Krav: éin CI-køyring samlar signerte attestasjonar frå macOS ARM64, Linux
x86-64, Linux ARM64 og ekte Windows, bundne til **same** G-A-commit og
kandidatgenerasjon, og skriv `production_ready_all_platforms=true` for gjeldande
generasjon (mønsteret frå køyring `30830409254`, no utan overgangsbaner).

## Port for heile Milepæl B

`tools/platform_readiness_v3600.no` grøn på gjeldande generasjon; ingen
GCC/Zig/Docker-byggverktøy i nokon aktiv byggbane (active-surface-porten er alt
grøn i sky-økta — restansen er B2-GCC-kallet og Docker-klassifiseringa over);
byte-identisk Gen1/Gen2 dokumentert per plattform i arbeidsloggen og manifestet.

## Kva sky-økta bidrog med

- Køyrde active-surface-porten grøn og verifiserte kjeldeflate-audit (0 Python,
  ingen C/Zig i aktiv kjeldeflate).
- Lokaliserte B2-restansen (`build_linux_arm64_tls_attestation_candidate.no`
  brukar `/usr/bin/gcc`) og Docker-klassifiseringspunktet.
- Kan **ikkje** køyre B1–B4 (native fire-plattform-bygg + attestasjon krev
  host/CI + signeringsinfrastruktur).
