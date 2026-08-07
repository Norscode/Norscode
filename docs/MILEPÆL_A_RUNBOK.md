# Milepæl A – Stabiliser generasjonen: køyrerunbok

Dette er den konkrete, ordna kommandolista for å fullføre Milepæl A i
[SELVSTENDIGHET_SLUTTPLAN.md](SELVSTENDIGHET_SLUTTPLAN.md). Målet er éin frosen,
fullt attestert kandidatgenerasjon som alle seinare milepælar målast mot.

**Kvifor dette ikkje kan køyrast frå Cowork-sky-økta:** Milepæl A krev
eksekvering av macOS-tillitsankeret (`dist/norscode_native`, ein Mach-O ARM64
som ikkje køyrer på Linux), ekte Windows-host for attestasjon, og GitHub Actions
for signert provenance. Sky-økta har berre den innsjekkede Linux ARM64-stage0-en,
som køyrer enkeltfiler via miljøkontrakt. Køyr difor stega under på **Mac-en din**
og i **GitHub CI**. Sky-økta har verifisert utgangstilstanden (under) og kan
byggje/verifisere reine `.no`-modular, men ikkje kryssplattform-attestasjon.

## Verifisert utgangstilstand 2026-08-07 (frå maskina di)

| Artefakt | Storleik | SHA-256 |
|---|---|---|
| `dist/norscode_native` (macOS tillitsanker) | — | `5f2a626ae1859df81c1adac97aacce3427841a6702c279c471ec1bab8c6c323a` |
| `bootstrap/stage0/norscode-macos-arm64` | — | `97483d04c3a433297d152b2788176828e04d4e98c4d7f9b41a6b3597fa269284` |
| `bootstrap/stage0/norscode-windows-x86_64.exe` | 11 604 280 | `534917fff7781c1c55c4aca322edb5f30295618683766a771205470ea20c00b0` |

Blokkeringa iflg. arbeidsloggen: siste generasjonsskifte (fixed15, kandidat
`9cc436c…be1af`) gjorde at `release-preflight` endar `FEIL (1)` på Windows
stage0/kandidat-avvik. Windows-stage0 over vart byte-identisk med kandidaten i
CI-køyring `30830409254` for ein *tidlegare* generasjon; Milepæl A re-attesterer
**gjeldande** generasjon.

## Steg A0 – Frys kjeldegenerasjonen

```bash
cd <repo>
git status            # arbeidskopi skal vere rein før frys
git rev-parse HEAD    # noter commit som "G-A" i arbeidsloggen
```

Alle stega under skal køyre på **same commit**. Endrar du kjelda undervegs, må
G-A oppdaterast og A2–A6 køyrast på nytt (regelen om sannferdige portar).

## Steg A1 – Bygg Windows-kandidat A/B byte-identisk

```bash
./bin/nc run tools/build_windows_stage0_candidate.no
# køyr ein gong til i eit reint arbeidsområde og samanlikn hash
```

Krav: to uavhengige bygg gir **byte-identisk** PE. Noter kandidat-SHA-256.
Dette er reproduserbarheit, ikkje endå stage0-paritet.

## Steg A2 – Attester på ekte Windows i CI

Windows-attestasjon køyrer i `.github/workflows/windows-app-release.yml`
(`tools/windows_runtime_attestation.no`). Trigg via tag eller manuelt:

```bash
gh workflow run "Windows App Release"           # workflow_dispatch
# eller: git tag vX.Y.Z-rcN && git push origin vX.Y.Z-rcN
gh run watch                                    # følg køyringa
```

Porten skal attestere SChannel TLS 1.3, AppContainer, IOCP, filesystem, prosess
og Argon2id, og GitHub-signere provenance bunde til G-A-commit og kandidat-SHA-256.

## Steg A3 – Promoter Windows-stage0 kontrollert

```bash
./bin/nc run tools/promote_attested_windows_stage0.no
```

Krav: verktøyet krev signert attestasjon + commit-kontroll + kandidat-SHA-256 før
det publiserer atomisk, oppdaterer `bootstrap/stage0/SHA256SUMS` og bevarer førre
generasjon som hashverifisert rollback under `bootstrap/stage0/rollback/`.

## Steg A4 – Release-preflight på 0 feil

```bash
./bin/nc release-preflight --strict
```

Krav: **0 feil**. Etter A3 skal det einaste dokumenterte avviket (Windows
stage0/kandidat-storleik) vere borte. Er det framleis raudt, er A1–A3 ikkje
fullførte for gjeldande generasjon.

## Steg A5 – Komplett local-green --strict på Mac

```bash
./bin/nc local-green --strict
```

Køyrer streng release-preflight, active-surface, fase 0, bootstrap A+B/C,
L5 Gen1/Gen2, L5b (cache på og av), isolert seed, single-binary, full test og
slow-lane. Nye tidsgrenser: fire timar ytre L5b-kaldbygg, seks timar i CI.
Krav: grøn utan uklassifiserte hopp.

## Steg A6 – Full strict i Linux-CI med ekte testtotal

```bash
gh workflow run "CI"          # eller push til hovudgrein
gh run watch
```

Krav: full strict køyrer i Linux-CI med ekte samla testtotal (ikkje selftest,
ikkje tom testliste — fail-closed-krava frå køyringane `30790807053` /
`30791877369` gjeld). `local-green --strict` skal vere grøn **både** lokalt og
i CI på G-A.

## Steg A7 – Frys og dokumenter generasjon G-A

Noter i `docs/NORSCODE_SELFSTENDIGHET_PLAN.md`:

- G-A kjeldecommit
- `dist/norscode_native`-SHA-256 og alle fire stage0-SHA-256
- CI-run-ID-ane for Windows-attestasjon og full strict

Kryss av desse punkta i «Endeleg godkjenning» først når portane er grøne:
`release-preflight`, `local-green --strict`, `selvstendighet --strict`, og
byte-identisk Gen1/Gen2 per plattform.

## Port for heile Milepæl A

`local-green --strict` grøn lokalt **og** i CI på G-A, med `release-preflight`
på 0 feil og signert Windows-attestasjon bunde til G-A-commit. Ingen seinare
milepæl (B–F) skal målast mot ein annan generasjon utan å oppdatere G-A eksplisitt.

## Kva sky-økta bidrog med

- Verifiserte utgangstilstanden (hashane over) direkte frå maskina di.
- Kan ikkje køyre A1–A6 (kryssplattform-attestasjon/promotering krev
  Mac + Windows-host + GitHub CI).
- Held fram med reine `.no`-krypto-modular for Milepæl D parallelt (SHA-512,
  HKDF, X25519 er alt inne og RFC-verifiserte mot ARM64-stage0).
