# ARM64-stage0: kontrollert promotering 2026-11-12

## Status

Kandidatane under er bygde frå kjelde, uavhengig reproduserte og grøne gjennom
alle seks seed-testskår på ekte Linux ARM64 og macOS ARM64. Dei er promoterte
atomisk med hashverifiserte rollback-kopiar. På kjeldekøyringa før promotering
var committed stage0 ulik kandidatane, og dei to obligatoriske byte-paritetsjobbane
feila korrekt. Endeleg PR-status krev ei ny køyring på promoteringscommiten.
Ingen testport, bytekontroll eller unntaksgrense er svekt.

Promoteringa er deretter verifisert på PR-head
[`5a8b175ace9fd4fbdad19aa875a2cde56c4a8030`](https://github.com/Norscode/Norscode/commit/5a8b175ace9fd4fbdad19aa875a2cde56c4a8030):
[ARM64-køyring 43278692411](https://github.com/Norscode/Norscode/actions/runs/43278692411)
fullførte med alle ni jobbar grøne. Dette omfattar kryssbygget, begge uendra
stage0-byteparitetsportane og alle seks seed-testskår på ekte Linux ARM64 og
macOS ARM64. Desse resultata verifiserer ARM64-promoteringa; samla PR-status
må framleis kontrollerast på den endelege PR-headen.

## Kjeldebinding

Beviset kjem frå [ARM64-køyring 43267262599](https://github.com/Norscode/Norscode/actions/runs/43267262599)
for PR-head `19ee05039d6b72a0a9986d1afc31a963d67b504c`. GitHub sjekka ut
testmerge `672d7272852857c0551eab09f9e11aa667d86264`. Den felles
`ci-runtime/build.json`-rapporten i artefakten `norscode-macos-arm64-feilsok`
oppgjev:

| Felt | Verdi |
| --- | --- |
| Skjema / status | `norscode-ci-runtime-v1` / `ok` |
| Fullhost-kjeldehash, 60 filer | `b5aa5e0a797610a01e1f7c77e9af17b1fd35d8c74fa431cd3ad2e0443433a543` |
| Fullhost-NCB SHA-256 | `271f461a4c61b44911318fa870db847f2a0886548fd372cd22f04a9d350706db` |
| Fullhost-NCB storleik | 5 755 425 byte |
| L5 byte-paritet | `true` |
| Uløyste / uventa builtins | `0` / `0` |
| x86-byggjekandidat selftest | `passed` |
| Automatisk runtime-promotering | `false` |

Kjeldehashen er òg rekna lokalt med
`codegen_prebuilt.hash_filer(root, codegen_prebuilt.lukking(root, "selfhost/nc_main.no"))`.
`build.json.candidate_sha256` identifiserer x86-byggjekandidaten; ARM64-hashane
under er rekna frå dei eigne ARM64-artefaktane.

## Dei konkrete ARM64-kandidatane

| Mål | Artefakt / fil | Byte | SHA-256 | Git-blob |
| --- | --- | ---: | --- | --- |
| Linux ARM64 | `norscode-linux-arm64-kandidat` / `nc_main_arm64` | 28 753 524 | `fc7dfd79dbfb824e100d0bc8b6a0c74d3881211e73b94f3418774a72226c0038` | `cd75948d2ad471c319b34fc7f5bb149fdcc216c3` |
| macOS ARM64 | `norscode-macos-arm64-kandidat` / `nc_main_macos_arm64` | 29 015 833 | `a5d7ac91fd51559ff3c6dd926d66f6aac55e6aac60bb667ee77e72565916a978` | `51e30c3f60d9a60b0b26951631294d6ac42d9386` |

Artefakt-ID-ane er `13966976213` (Linux), `13966143591` (macOS) og
`13965918843` (felles NCB/byggjerapport). SHA-256 for dei nedlasta ZIP-filene
vart kontrollert mot GitHub-metadata før utpakking:

| Artefakt | ZIP SHA-256 |
| --- | --- |
| Linux-kandidat | `dd04ef6f4d89a46065cfdb8c9470ebdb41433c1c51f65913b50ff0414be78790` |
| macOS-kandidat | `482f8f5e6a6be47f6e67f9b3766fee0fefa8d9d3fdc61532c0914706e94cbd46` |
| NCB/byggjerapport | `d596a49bf3f590aba008670622e4c209cd61867e07cf8b7d6105ecc70e322309` |

Begge kandidatane vart deretter bygde på nytt frå akkurat denne NCB-en med
`tools/build_arm64_seed.no`, `NORSCODE_SEED_NCB` sett til den nedlasta NCB-en og
`NC_ARM64_OS` sett til høvesvis `linux` og `macos`. Dei pinna Norscode-codegenane
vart validerte mot kjelde og avhengigheiter. Begge nye binærar var byte-identiske
med CI-artefaktane ved full filsamanlikning (`cmp`), og SHA-256/Git-blob-hashane
var identiske med tabellen over.

Kryss-codegen rapporterte null uløyste builtins og dei fem eksisterande stubba
builtinane `argon2id/6`, `kompiler_fil/2`, `thread_spawn/1`, `thread_join/1` og
`green_switch/2`. Dette er ikkje ein påstand om null stubbar. Kandidatane måtte
òg passere den separate runtime-gap-porten på kvar av dei ekte plattformjobbane.

## Faktiske kandidatportar

[Kryssbygg-jobb 131354072289](https://github.com/Norscode/Norscode/actions/runs/43267262599/job/131354072289)
fullførte grønt. Alle jobbane under fullførte grønt, med både
`Kandidat-selftest`, `Kandidatens runtime-gap-port før stage0-promotering` og
den reelle `tools/seed_gate_tests.txt`-køyringa som eigne vellykka steg.
Loggane inneheld òg den eksplisitte meldinga om grøn runtime-gap-port.

| Plattform / skår | Jobb | Bestått | Feila | Eksisterande plattformhopp |
| --- | --- | ---: | ---: | ---: |
| Linux ARM64 / 0 | [131355331948](https://github.com/Norscode/Norscode/actions/runs/43267262599/job/131355331948) | 43 | 0 | 3 |
| Linux ARM64 / 1 | [131355332598](https://github.com/Norscode/Norscode/actions/runs/43267262599/job/131355332598) | 43 | 0 | 1 |
| Linux ARM64 / 2 | [131355332236](https://github.com/Norscode/Norscode/actions/runs/43267262599/job/131355332236) | 43 | 0 | 0 |
| macOS ARM64 / 0 | [131355332115](https://github.com/Norscode/Norscode/actions/runs/43267262599/job/131355332115) | 45 | 0 | 1 |
| macOS ARM64 / 1 | [131355332277](https://github.com/Norscode/Norscode/actions/runs/43267262599/job/131355332277) | 43 | 0 | 1 |
| macOS ARM64 / 2 | [131355333570](https://github.com/Norscode/Norscode/actions/runs/43267262599/job/131355333570) | 43 | 0 | 0 |
| **Sum** | | **260** | **0** | **6** |

Dei seks hoppa er dei eksisterande `native-unsupported`-vala:
`test_install_macos_app_native_live` og `test_native_cli_builtin_gate` på Linux,
`test_stdlib_source_cache_live` på begge plattformer og
`test_tls13_public_trust` på begge plattformer. macOS-køyringa av
`test_native_cli_builtin_gate` er no grøn etter den kjeldebundne fornyinga av
macOS-host-codegenane.

## Promoteringsgrense og rollback

Promoteringa brukte `tools/runtime_promotion_core.no` med eksplisitte kandidat-,
mål- og rollback-stiar. Drivaren verifiserte same fullhost-kjeldehash, NCB-hash,
begge kandidat-hashane og begge gamle mål-hashane før målfilene vart erstatta.
Kjernen skreiv atomisk, bevarte førre binær og kontrollerte rollback-hashen.

| Mål før promotering | Førre SHA-256 | Eksisterande Git-blob |
| --- | --- | --- |
| `bootstrap/stage0/norscode-linux-arm64` | `378f848af3bd74cf6d29fd7553f193beebb5595e276323a2bf61f09770cf3a6b` | `0d6ecf09b6f18224aa6c2384f61f5f0c08521eb1` |
| `bootstrap/stage0/norscode-macos-arm64` | `3f427047dff6b5061a000922f757203b743925c3407c47e4b5034718e0846e56` | `9018b8061f263c47178deb18ec2afbcdec82274f` |

`bootstrap/stage0/SHA256SUMS` og dei to godkjende binærpinnane er oppdaterte.
Rollback-kopiane har eigne SHA-256-bundne `godkjend binær`-oppføringar;
talgrensene i active-surface-allowlista er uendra. Rollback-filene ligg under
`bootstrap/stage0/rollback/` med namna `norscode-<plattform>.<førre-sha256>.rollback`.
Dei to `norscode-<plattform>.promotion.manifest`-filene i same katalog bind
kandidat, gammalt mål, nytt mål og rollback til kvarandre.

Etter den atomiske utskiftinga vart desse lokale kontrollane grøne:

- Alle fire oppføringane i `bootstrap/stage0/SHA256SUMS` vart verifiserte.
- Begge promoterte filer var framleis byte-identiske med dei immutable
  CI-artefaktane, og begge rollback-filene hadde sine gamle SHA-256-hashar.
- Dei to målfilene og begge rollback-filene hadde køyrbar modus `0755`.
- Den uendra CI-eigaren `tools/verify_identical_files.no`, køyrd med committed
  Linux x86-64-stage0, godtok begge dei fullstendige ARM64-filpara.
- `./bin/nc feature-check tools/runtime_promotion_core.no tools/update_stage0_manifest.no`
  bestod active-surface, seed/runtime og syntakskontroll av begge Norscode-eigarane.
- `git diff --check` var rein.

Endeleg PR-status blir først avgjord av nye kontrollar på den publiserte
promoteringscommiten, inkludert begge uendra byte-paritetsjobbane. Dersom andre
endringar blir integrerte før publisering, må fullhost-kjeldehashen framleis
vere identisk med tabellen over; eit avvik krev nytt kjeldebygg og nye
kandidatportar før promoteringa kan publiserast.
