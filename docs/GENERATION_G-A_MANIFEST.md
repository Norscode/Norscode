# Generasjon G-A — frysemanifest (pre-attestasjon)

Dette er A7-artefakten frå [MILEPÆL_A_RUNBOK.md](MILEPÆL_A_RUNBOK.md): eit
komplett, integritetskontrollert hash-manifest for gjeldande kandidat-
generasjon. Manifestet er merkt **pre-attestasjon** fordi A1–A6-portane (byte-
identisk Windows-kandidat, Windows-attestasjon, `release-preflight --strict`,
`local-green --strict`, full strict i CI) endå ikkje er grøne — dei krev
Mac-tillitsankeret, ekte Windows-host og GitHub CI, jf. runbok-en.

Produsert frå den innsjekkede arbeidskopien i Cowork-sky-økta 2026-08-07.

## Kjeldecommit

- **G-A commit:** _fyll inn frå host_ (`git rev-parse HEAD`) — Git-objekta ligg
  utanfor sandkasse-mountet, så commit-SHA må hentast på maskina di. Arbeidskopien
  var rein (ingen ukommiterte endringar utover D2-arbeidet i denne økta) då
  hashane under vart rekna.

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
