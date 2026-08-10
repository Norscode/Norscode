# Fersk ARM64-seed → lukk B2-attestasjonen (operatør-/CI-runbok)

**Formål:** Det einaste attståande steget for formell Milepæl B2-attestasjon på
Linux ARM64. Alt codegen-arbeidet er gjort og verifisert; denne runboka skildrar
korleis operatøren produserer ein **fersk arm64-stage0-seed** som ber
2026-08-codegen-rettingane, slik at den GitHub-signerte attestasjonen blir grøn.

Sjå òg: [SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md](SELFHOST_ARM64_FULLHOST_CODEGEN_PLAN.md),
[MILEPÆL_B_RUNBOK.md](MILEPÆL_B_RUNBOK.md).

## Kvifor dette steget trengst

Den reine krypto/TLS-stakken køyrer **native på ekte Linux ARM64, utan OpenSSL**
(verifisert 2026-08-09 i `linux/arm64`-sandkasse): `test_sha512_nist`,
`test_x25519_rfc7748`, `test_ed25519_rfc8032`, `test_chacha20_poly1305_rfc8439` og
**`test_tls13_handshake_flow`** — alle grøne, VM = native. Fem codegen-feil vart
funne og retta (operand-spilling, 64-bit-aritmetikk, kontrollflyt-merge-djupn,
`slice`-type-dispatch, `COMPARE_NE`-type-dispatch), kvar med regresjonsfixture i
`tests/test_arm64_ncval_machine.no`.

Workflow-en `.github/workflows/arm64-native-attest.yml` køyrer **end-to-end** på ein
ekte `ubuntu-24.04-arm`-runner: checkout, materialisering, NCB-kompilering og
codegen-bygg av attestasjonsbinæren er grøne. Det **einaste** som feilar er
run-steget, fordi:

> Den **committede** `bootstrap/stage0/norscode-linux-arm64` (frå juli 2026) brukar
> sin **innebygde, gamle codegen** — sjølv med `NORSCODE_USE_PRECOMPILED_SELFHOST=0`
> (jf. `tools/ci_shell_runner.no` L174: «den historiske arm64-stage0-en arvar ikkje
> miljøkartet ved prosess-spawn»). Difor får den bygde ELF-en **ikkje**
> 2026-08-rettingane, og feilar ved køyring.

Dette er ikkje ein codegen-feil (kjelde-codegen er rett), men den kjende
«committed seed er for gammal»-avhengigheita — same som B2 uansett krev: *ein fersk
ARM64-binær bygd frå levande kjelde på ekte ARM64-host*.

## Kva ein fersk seed løyser

Ein seed bygd frå gjeldande kjelde ber den **retta codegen innebygd**. Då byggjer
CI-jobben attestasjonsbinæren med rettingane → binæren gjev exit 0
(«GROEN (null OpenSSL)») → `actions/attest-build-provenance` signerer → **B2 lukka**.

## Steg (må køyrast på ein ekte ARM64-host / CI — ikkje sky-økta)

Føresetnad: ein current Linux ARM64-host (t.d. `ubuntu-24.04-arm`) med gjeldande
kjelde utsjekka på grein `krypto-tls-primitiver` (eller etterfølgjaren etter merge).

1. **Bygg ein fersk linux-arm64 `nc` frå levande kjelde.** Bruk den ordinære
   sjølvhost-byggvegen (L5/L5b-fragmentmateraliseringa for å halde RSS nede — sjå
   B2-restansen om 8,7 GiB-toppen), *ikkje* gcc og *ikkje* den committede seeden som
   codegen-kjelde. Resultat: `dist/norscode_native` (Mach-O-motstykket på macOS;
   her ELF64/AArch64).
   - Sanity: `file dist/norscode_native` → «ELF 64-bit LSB executable, ARM aarch64».
   - Sanity: `NORSCODE_USE_PRECOMPILED_SELFHOST=0 ./bin/nc run tools/build_arm64_krypto_attest.no`
     (med `ATTEST_NCB`/`ATTEST_ELF`) skal byggje ein ELF som gjev exit 0.

2. **Verifiser kandidaten mot krypto/TLS-flata (kandidat-smoke).** Bygg og køyr desse
   native ELF-ane via den ferske `nc` og stadfest exit 0 / rett resultat:
   `tests/fixtures/arm64_krypto_attest.no` (samla), pluss dei fem
   `test_{sha512_nist,x25519_rfc7748,ed25519_rfc8032,chacha20_poly1305_rfc8439,
   tls13_handshake_flow}`. Køyr òg heile `tests/test_arm64_ncval_machine.no`
   (regresjon: operand-spilling, 64-bit, merge, slice, COMPARE_NE m.m.).

3. **Kontroller miljøarv.** Den ferske seeden må arve environment-kartet ved
   prosess-spawn (så `NORSCODE_USE_PRECOMPILED_SELFHOST=0` verkar i CI). Stadfest at
   `ci_shell_runner.no`-omvegen L174 (`NORSCODE_VM_CI_DIRECT_OWNER=
   linux-arm64-tls-attestation`) **ikkje** lenger trengst for den nye seeden; fjern
   omvegen om han er unødvendig.

4. **Frys og promoter seeden kontrollert** (promoteringsregelen: kandidat → levande
   smoke → full kandidatmatrise → atomisk promotering med rollback):
   - `cp` den ferske binæren til `bootstrap/stage0/norscode-linux-arm64`, bevar førre
     som `bootstrap/stage0/rollback/norscode-linux-arm64`.
   - `shasum -a 256 dist/norscode_native bootstrap/stage0/*` → oppdater
     `bootstrap/stage0/SHA256SUMS`.
   - Noter kjeldecommit + hash i arbeidsloggen.

5. **Køyr attestasjonen.** Med fersk seed på plass, trigg
   `arm64-native-attest.yml` (gjer han til `push`/`pull_request`-trigga att, eller
   `gh workflow run` når greina er på `main`). Forventa:
   - Bygg native ARM64-ELF via codegen: **grøn** (no med rettingane).
   - Køyr binæren: **exit 0**, «ARM64 KRYPTO/TLS ATTESTASJON: GROEN (null OpenSSL)».
   - `attest-build-provenance`: **GitHub-signert provenance** bunden til
     kjeldecommit + binær-SHA-256.

6. **B2-restansane** (uavhengig av dette, men same milepæl): byte-identisk Gen1/Gen2
   på Linux ARM64, og at `tools/build_linux_arm64_tls_attestation_candidate.no`
   sluttar å bruke `/usr/bin/gcc` + OpenSSL (byt til den ferske native codegen-vegen
   + rein TLS over D2 socket-ABI). ABI-restansane (B3) og fire-plattform-beviset (B4)
   står framleis som eigne punkt.

## Kva som alt er på plass (ingen ny kode trengst for sjølve attestasjonen)

- `.github/workflows/arm64-native-attest.yml` — jobben (manuell trigger inntil fersk
  seed finst).
- `tools/build_arm64_krypto_attest.no` — byggjedrivar (NCB → ELF via
  `elf_arm64_codegen`).
- `tests/fixtures/arm64_krypto_attest.no` — attestasjonsprogrammet.
- Retta kjelde-codegen i `selfhost/native_execution/macho_arm64_codegen.no` +
  regresjonsfixturane.

Alt dette er pusha på `krypto-tls-primitiver`. Den einaste manglande brikka er den
ferske arm64-seeden i steg 1–5.
