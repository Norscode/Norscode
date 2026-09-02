# Generasjon G-A — frysemanifest

Dette er A7-artefakten frå [MILEPÆL_A_RUNBOK.md](MILEPÆL_A_RUNBOK.md): eit
komplett, integritetskontrollert hash-manifest for generasjon **G-A**.

**Status 2026-08-07: Milepæl A stengt.** Operatøren stadfestar alle A-portane
(A1–A6) grøne på host/CI, og Windows-attestasjonen er mottatt og verifisert
commit-bunden (sjå «A2-status»). Generasjonen er frosen på commit
`52593ad9a366ea9c6f9d2c7061a6aa94b1d85386`.

Hash-tabellen under vart rekna frå den innsjekkede arbeidskopien i Cowork-sky-
økta 2026-08-07. **Viktig:** dersom A3 promoterte ein ny Windows-stage0, er
`bootstrap/stage0/norscode-windows-x86_64.exe`-hashen under frå *før*
promoteringa — regenerer heile tabellen på host med
`shasum -a 256 dist/norscode_native bootstrap/stage0/*` og oppdater både dette
manifestet og `bootstrap/stage0/SHA256SUMS` slik at G-A er byte-nøyaktig.

## Kjeldecommit

- **Attestert commit (A2):** `52593ad9a366ea9c6f9d2c7061a6aa94b1d85386`
  (grein `main`) — henta frå den GitHub-signerte Windows-attestasjonen (sjå
  «A2-status» under). Dette er commiten Windows-attestasjonen faktisk køyrde på.
- **G-A-krav:** heile Milepæl A (A1–A6) skal målast mot **same** commit. Viss
  D2-arbeidet frå denne økta skal vere med i G-A, må A2 køyrast på nytt på
  commiten som inneheld D2; ellers er G-A = `52593ad9…` og D2 høyrer til ein
  seinare generasjon. Vel eksplisitt før frys.

## CI-run-referansar (G-A på `main@52593ad9`)

| Port | Workflow | Run | Varigheit |
|---|---|---|---|
| A2 Windows-attestasjon | Windows App Release #107 | [31219997969](https://github.com/Norscode/Norscode/actions/runs/31219997969) | 7m 55s |
| A6 full strict (inkl. release-preflight/local-green i CI) | CI #1312 | [31219995817](https://github.com/Norscode/Norscode/actions/runs/31219995817) | 5h 44m 58s |

Begge køyrde på `main`, manuelt trigga 2026-08-07 23:25 (GMT+2), stadfesta grøne
av operatøren. Att for full sjølv-verifisering: utdata frå
`gh attestation verify windows-runtime-attestation.json --repo Norscode/Norscode`.

## D2 ligg på eiga grein (seinare generasjon enn G-A)

D2-kryptoen (rein Norscode TLS 1.3) vart pusha til grein
**`krypto-tls-primitiver`** som commit
[`d942c58`](https://github.com/Norscode/Norscode/commit/d942c583408aea4be2696af0ae255d8681a5212c),
og **Krypto-smoke #2**
([31219986381](https://github.com/Norscode/Norscode/actions/runs/31219986381),
3m 36s) er grøn på den commiten. D2 er difor **ikkje** del av G-A
(`main@52593ad9`); han høyrer til ein seinare generasjon. Skal D2 inn i ein
frosen generasjon, må A-portane (særleg A2 + full strict) køyrast på nytt på ein
commit som merger `krypto-tls-primitiver` inn i `main`.

## Tillitsanker og stage0 (verifiserte hashar)

| Artefakt | Storleik (byte) | SHA-256 |
|---|---:|---|
| `dist/norscode_native` (macOS ARM64 tillitsanker) | 6 307 632 | `5f2a626ae1859df81c1adac97aacce3427841a6702c279c471ec1bab8c6c323a` |
| `bootstrap/stage0/norscode-macos-arm64` | 11 707 440 | `97483d04c3a433297d152b2788176828e04d4e98c4d7f9b41a6b3597fa269284` |
| `bootstrap/stage0/norscode-linux-arm64` | 36 021 952 | `4ce15d11dc61b993e0f1c7d2553dcb46a7addb4612cffb9951ad4f706c30dabe` |
| `bootstrap/stage0/norscode-linux-x86_64` | 10 257 339 | `040c20b20d2071d5445d247da5d672a38136aa08748acdd1b911f68fc40c79d3` |
| `bootstrap/stage0/norscode-windows-x86_64.exe` | 11 604 280 | `534917fff7781c1c55c4aca322edb5f30295618683766a771205470ea20c00b0` |

**Integritetskontroll:** alle fire stage0-hashane er byte-for-byte like med
`bootstrap/stage0/SHA256SUMS`. ✔

## A1-status: Windows-kandidat kan ikkje bli byte-identisk med stage0 for denne generasjonen

`tools/build_windows_stage0_candidate.no` har ein «kanonisk fast-path» som berre
binærkopierer den committede Windows-stage0-en (og dermed gir paritet) når dei
tre NCB-payloadane matchar innebygde konstantar. Ein direkte hash-samanlikning
viser at **to av tre payloadar har drifta** frå konstantane:

| Payload | Noverande SHA-256 | Kanonisk konstant | Match |
|---|---|---|---|
| `bootstrap/kompiler.ncb.json` | `912afec5b06ca84e6a0c70178f1a1d3316ab8826e9132aebdb52935c2be4d4ef` | `183c413d867ddfbd3d1939924c03d7db3d1a4816cde65592422565629629d3f2` | ✗ **nei** |
| `bootstrap/precompiled/vm.ncb.json` | `c99500e9b1c5e9466cc9d4546519c07dc620a431e2ac1df8f3317c675868dfe6` | `f7d925869c78cd5ce2d7c3c14f59504c053e4e3f2c5a2b8979fb6f830296b1c2` | ✗ **nei** |
| `selfhost/vm_executor.ncb.json` | `eafe683e708a46c40ca7af1f54d53fa416b667b791073d8963fc2097f2a9c04f` | `eafe683e708a46c40ca7af1f54d53fa416b667b791073d8963fc2097f2a9c04f` | ✔ ja |

Konsekvens: for gjeldande generasjon vil verktøyet gå den NCB-embeddande vegen,
ikkje fast-path-en, og kandidaten blir **ikkje** byte-identisk med den committede
Windows-stage0-en (`windows_stage0_parity=false`). Dette er nett det
release-preflight-avviket runbok-en omtalar — no lokalisert til at
kompilator- og VM-payloadane har endra seg sidan Windows-stage0-en sist vart
promotert, medan vm-executor-payloaden framleis er i takt.

**Handling for å lukke A1:** anten (a) regenerer/promoter ein ny Windows-stage0
frå gjeldande payloadar via A2→A3 og oppdater konstantane, eller (b) frys ein
generasjon der alle tre payloadane igjen matchar ein attestert Windows-stage0.
Begge krev Windows-host + CI (A2) og kan ikkje gjerast i sky-økta.

## Kvifor A1-verktøyet ikkje kan køyrast i sky-økta

Den committede `bootstrap/stage0/norscode-linux-arm64` (tolken sky-økta har)
støttar **ikkje `0x`-heksadesimale literalar** — dei evaluerer til `ingenting`.
`tools/build_windows_stage0_candidate.no` (og fleire byggeverktøy) brukar `0x..`
gjennomgåande (t.d. PE-magic-kontrollen `magic == 0x20B`), så verktøyet gir
falsk `[FAIL] committed Windows-stage0 er ikkje PE32+ x86_64` her sjølv om PE-en
er gyldig (verifisert uavhengig: `e_lfanew=120`, `machine=0x8664`,
`magic=0x20B`, PE-signatur `50 45 00 00`). Dette er «committed stage0-alder»-
risikoen i sluttplanen. På det nyare macOS-tillitsankeret finst hex-literal-
støtta, så A1 må køyrast der (som runbok-en allereie føreskriv).

## A2-status: GitHub-signert Windows-attestasjon mottatt (delvis verifisert)

Ei sigstore-attestasjon er levert og dekoda (arkivert i repoet som
[`docs/attestations/windows-runtime-attestation-run31219997969.sigstore.json`](attestations/windows-runtime-attestation-run31219997969.sigstore.json),
GitHub-run 31219997969). Innhaldet er ekte SLSA-provenance:

| Felt | Verdi |
|---|---|
| Attestert artefakt | `windows-runtime-attestation.json` |
| Artefakt-SHA-256 | `8200cfe335e421557b4754b3a3eab93526417e989669e77ae16bcb3df282fe36` |
| predicateType | `https://slsa.dev/provenance/v1` |
| Kjeldecommit (i signeringssertifikatet) | `52593ad9a366ea9c6f9d2c7061a6aa94b1d85386` |
| Grein | `refs/heads/main` |
| Workflow | `.github/workflows/windows-app-release.yml` |
| Trigger | `workflow_dispatch`, `github-hosted` runner |
| Run | `https://github.com/Norscode/Norscode/actions/runs/31219997969/attempts/1` |
| OIDC-utstedar | `https://token.actions.githubusercontent.com` |
| Fulcio-sertifikat | utstedar `sigstore.dev` / `sigstore-intermediate`, gyldig 10 min (2026-08-07 21:33–21:43Z) |
| Rekor-transparenslogg | `logIndex 2373140531`, `integratedTime 1786138406`, DSSE v0.0.1 |

Både `BuildSignerDigest` og `SourceRepoDigest` i Fulcio-sertifikatet er bundne til
commit `52593ad9…`, så commit-bindinga er forankra i sjølve signeringsidentiteten,
ikkje berre i nyttelasten. ✔

**Kva som er verifisert her:** strukturen (gyldig DSSE/in-toto SLSA v1),
signeringsidentiteten (GitHub Actions OIDC → Fulcio), commit-/workflow-/runner-
bindinga og at det finst ei Rekor-loggoppføring.

**Restansar før A2 kan hakast av:**
1. **Full kryptografisk verifikasjon** (signatur mot Fulcio-rot + Rekor-
   inklusjonsprov + sertifikatkjede). Kan ikkje gjerast i sky-økta (krev
   sigstore-trust-root/online Rekor). Køyr på host:
   ```bash
   gh attestation verify windows-runtime-attestation.json \
     --repo Norscode/Norscode
   ```
2. **Substansen**: sjølve `windows-runtime-attestation.json` (berre hashen finst
   her) må vise at SChannel TLS 1.3, AppContainer, IOCP, filesystem, prosess og
   Argon2id alle attesterte grønt, og kva **kandidat-SHA-256** rapporten bind seg
   til (A2 krev binding til Windows-kandidaten, ikkje berre til rapporten).
   Last opp rapporten, så kryssjekkar eg mot payload-drifta og manifestet.

## A-infrastruktur som finst i repoet (verifisert)

- `tools/build_windows_stage0_candidate.no` (A1)
- `tools/windows_runtime_attestation.no` (A2)
- `tools/promote_attested_windows_stage0.no` (A3)
- `tools/release_preflight.no` (A4)
- `.github/workflows/{windows-app-release,ci}.yml` (A2/A6)
- `bootstrap/stage0/rollback/` finst med fire hash-namngjevne historiske
  Windows-binærar (rollback-krav i A3 er strukturelt på plass).

## Restansar for å fullføre Milepæl A (krev host/CI)

1. **A1** på Mac: to uavhengige bygg → byte-identisk PE; løys payload-drifta over.
2. **A2** i CI på ekte Windows: SChannel/AppContainer/IOCP/filesystem/prosess/
   Argon2id + GitHub-signert provenance bunde til G-A-commit og kandidat-SHA-256.
3. **A3**: kontrollert atomisk stage0-promotering med hashverifisert rollback.
4. **A4**: `./bin/nc release-preflight --strict` = 0 feil.
5. **A5**: `./bin/nc local-green --strict` grøn på Mac.
6. **A6**: full strict i Linux-CI med ekte testtotal.
7. **A7**: fyll inn G-A-commit + CI-run-ID-ane her og i
   `docs/NORSCODE_SELFSTENDIGHET_PLAN.md`.

Port for heile Milepæl A: `local-green --strict` grøn lokalt **og** i CI på G-A,
`release-preflight` på 0 feil, og signert Windows-attestasjon bunde til G-A.
