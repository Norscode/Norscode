# Samla PR-kø – 11. november 2026

## Føremål og revisjonar

PR [#276](https://github.com/Norscode/Norscode/pull/276) er den samla arbeids-PR-en
etter konsolideringa. Utgangspunktet er
`a0a33089e33362e7336bc8e84fe19489bda28376`. Ingen innfletting i `main` eller
promotering av stage0 er del av denne konsolideringa.

Innlemmingscommitten bevarer dei uavhengige PR-tippane som foreldre i Git-historikken.
#270 er alt ein stamfar til #276. Dermed er også historiske binærar, byggbevis og
tidlegare rettingar tilgjengelege sjølv om eit gamalt generert bygg ikkje blir valt
som gjeldande fil. PR-greinene blir ikkje sletta.

| PR | Kontrollert tip | Kva den samla PR-en tek vare på |
|---|---|---|
| [#251](https://github.com/Norscode/Norscode/pull/251) | `a429ef1f71c62870b849c7a6c71c83e6de6ec29c` | NCB-versjonskontroll og regresjon finst frå før; `docs/NCB_VERSION_CANDIDATE.md` blir teke inn som historisk byggbevis. |
| [#260](https://github.com/Norscode/Norscode/pull/260) | `e1f26f2ee0be531c0115b1b95b81b910b1b5f34c` | Vedvarande tensorvekter, handtak, frigjeving og regresjon; handtakstesten blir lagd til i seed-porten. |
| [#269](https://github.com/Norscode/Norscode/pull/269) | `8ed48f3bd631768e27079ebcfa5121bd6ba9b582` | Windows-miljøblokk/UTF-16, direkte attestasjon, NCB-trailerkontroll, miljø-smoke og eigen streng attest-jobb. |
| [#270](https://github.com/Norscode/Norscode/pull/270) | `9d2bdd1ed0441137360402be18b034dbff4a17bb` | Heile selfhost-stabelen er alt ein stamfar til #276. |
| [#271](https://github.com/Norscode/Norscode/pull/271) | `2e494e13daa2b4574b7f8f8832004d4fa044c3db` | HTTP-lukke-/408-fristar er alt innarbeidde i `8ed985b11fd29c45f55ed519e202fa4f72e5d4fe`; testfila er byte-identisk. |
| [#272](https://github.com/Norscode/Norscode/pull/272) | `623cf55474f41c606ac54fa418f2ceed6fd49b5c` | Test-NCB blir førebudd i kortlevd barn; tilsvarande retting `179d0c95fe9ce577bd3290a180106bc04de1db20` og regresjon finst alt. |
| [#273](https://github.com/Norscode/Norscode/pull/273) | `03df69a9b0167050de9b107047f5cc6c81e6311f` | Desktop-, editor-, layout-, tekst-, render- og adaptergrunnmur med fem testar og dokumentasjon. |
| [#274](https://github.com/Norscode/Norscode/pull/274) | `70a810e9014dcbcfa1bd2f29bd96a1283c7766bc` | VS Code-grammatikk, Linguist-forslag, eksport/verifikasjon, fire testar og CI-steg. |
| [#275](https://github.com/Norscode/Norscode/pull/275) | `714cdf4d71bce55b7d4c9ec1e15e062de9a9c1a2` | Den sterke Windows-sandboxvakta og negativ test finst alt; ARM64-testbeviset blir teke inn som historisk dokument. |

## Samansetjing og avgrensingar

`std/native_gap.no` blir fletta på funksjonsnivå. Nyare allocator-, TLS- og
prosessrettingar frå #276 blir bevarte. Tensor- og Windows-endringane erstattar
ikkje den eldre PR-en si heile fil over den nyare kjelda.

Den eksisterande Windows-vakta krev eit eksplisitt `sandbox`-felt med verdien
`"none"` før prosessen kan starte. Manglande profil og alle isolasjonsprofilar blir
avviste. Den positive AppContainer-/Job Object-testen krev framleis faktisk
isolasjon og timeout. Den skal ikkje bli grøn berre fordi negative testar består.

Windows-jobbane bruker Norscode-adapteren og eksplisitt kommandobinding. Den
historiske PowerShell-diagnosen omtalt i #269 er ikkje med i den kontrollerte
PR-tippen. Full attestasjon krev eksplisitt testdispatch, vellykka prosess og den
forventa sluttmarkøren; hjelp eller tom testdiscovery med exit 0 er ikkje nok.

CI-adapteren får både `NORSCODE_LINGUIST_SOURCE_SHA` og
`NORSCODE_WINDOWS_FREESTANDING_ATTESTATION`, med regresjon som kontrollerer
miljøet i eit ekte barn. Eksisterande ARM-, GC-, strict-differensial- og verbose-felt
blir bevarte. Linguist-eksporten bruker same fullstendige commit-SHA som checkout.

Desktop-arbeidet er ein headless grunnmur, ikkje ferdige native OS-vindauge.
Linguist-arbeidet er eit verifiserbart forslag/eksport; konsolideringa inneber ikkje
at GitHub Linguist har godkjent Norscode oppstraums. Ingen nye spora JSON-filer
blir lagde til; berre den eksisterande VS Code-grammatikken blir oppdatert.

## Genererte binærar og historisk bevis

Gamle binærbytte blir ikkje blanda med den nye kjeldekombinasjonen:

- #251 sine codegens og tilhøyrande metadata må byggjast på nytt frå samla kjelde.
- #260 sine tre stage0-bytebytte, manifest og pinnar blir ikkje promoterte.
- #269 sine eldre codegens, kjeldehashar, avhengnadshashar og kryss-proveniens blir
  ikkje kopierte over #276.
- #275 sine tidlegare testa ARM64-seedar blir bevarte i historikken og
  `docs/WINDOWS_GUARD_ARM64_SEEDS_2026-10-31.md`, men blir ikkje promoterte som
  bygg av den nye kjelda.

`docs/NCB_VERSION_CANDIDATE.md`,
`docs/WINDOWS_CODEGEN_REFRESH_2026-10-31.md` og
`docs/WINDOWS_GUARD_ARM64_SEEDS_2026-10-31.md` er historiske rapportar for sine
oppgitte revisjonar. Dei er ikkje attestasjon av denne innlemminga.

Både Windows-codegen-kjelda og fullruntime-kjelda er endra. Nye faktiske vert- og
kryssbygg, fixed point, korrekt proveniens og kandidat-testar må difor fullførast
før nye pinnar eller stage0 blir godkjende. Kildehashar skal ikkje omskrivast for
å godkjenne gamle binærar. Byteparitet og positive tryggleiksportar står ved lag.

## Verifikasjon av konsolideringa

Kontrollane under bruker den verifiserte Linux x86_64-bootstrapen med SHA-256
`1f7b588917c47e5c68dce3000df6f9bc7d8cde8e5e90e669a5fb1c7a0f62de48`.
Dette er ein bootstrap for kjeldekontroll, ikkje ein nybygd runtime med tensor- og
Windows-endringane.

- Alle 28 endra/nye `.no`-filer består syntakskontroll gjennom
  `tools/ci_check_source.no`.
- Fem desktop-testar og fire Linguist-testar består: 9/9 med exit 0 og eigne
  sluttmarkørar. Regex-testen består uendra etter ein innleiande kort
  diagnostikk-timeout; ingen testgrense i prosjektet er endra.
- Begge Windows-kjeldekontraktane består: NCB-trailervalidering og direkte
  attest-dispatch, 2/2 med exit 0 og sluttmarkørar. Dette køyrer ikkje Windows-ABI-en.
- Heile `test_ci_shell_runner.no` består, inkludert faktisk miljøoverføring til
  barn og avvising av ukjend miljøvariabel. Samla er 12 fokuserte testfiler grøne.
- Linguist-eksporten består gjennom CI-adapteren med `NC_CI=1` og
  `NORSCODE_LINGUIST_SOURCE_SHA=a0a33089e33362e7336bc8e84fe19489bda28376`.
  `SAMPLE_ORIGINS.md` bruker denne revisjonen; alle fire eksempelkjelder og
  grammatikken er byte-identiske med originalane.
- `test_tensor_handtak.no` er faktisk køyrd på bootstrapen og rapporterer at
  vedvarande vekter manglar. Dette er SKIP, ikkje bestått.
- `git diff --cached --check` er rein for alle 44 endra/nye filer. 35 direkte
  importfiler matcher PR-blobene nøyaktig; to dokument har berre normalisert
  whitespace med bevart Markdown-linjeskift. Dei andre sju filene er samanfletta
  kjelde/workflow, miljøregresjon, seed-testliste og denne oversikta.
- Uavhengig gjennomgang rekonstruerte heile `native_gap.no` frå a0 og PR-hunkene
  og fekk byte-identisk resultat. Windows-workflowen har åtte run-steg, åtte
  samsvarande kommandobindingar og Norscode-shell i alle tre jobbar.

Ingen av desse kontrollane godkjenner full feature-check/active-surface frå den
delvis gjenoppretta checkouten, eller erstattar ferske runtimebygg, ekte
plattformtestar, positiv Windows-attestasjon, proveniens eller byteparitet.

## Opne punkt som følgjer med til #276

1. Bygg codegens og fullruntime frå den endelege samla kjelda. Køyr Linux fixed
   point, ARM-kryssbygg og dei råka Mac-hostede x86-kryssbygga på ekte Mac.
   Tensor-handtak må rapportere `persistent_weights == sann` og fullføre med
   `test_tensor_handtak OK`. Skip på gammal bootstrap er ikkje testbevis.
2. Køyr den kjeldebygde CRT-frie Windows-kandidaten med eksplisitt og arva miljø,
   negativ sandbox-test og full positiv attestasjon. Reell AppContainer/Job Object,
   Windows-nettverk/TLS og timeout er ikkje erklærte ferdige.
3. ARM-differensialen på a0 stoppar med `NATIV-GAP:kompiler_fil`. Bruk og test den
   same kandidaten sin native compile-veg til NCB, VM og AOT utan stub-fallback.
4. Den forbetra ARM-loggen peikar på `CRL-tenarpolicy vart akseptert: rc=0`.
   Loggen oppgir committed stage0 som executable. Følg faktisk kandidatval gjennom
   adapter, testløpar og barn; ikkje rekn ein fersk byggartefakt som bevis på at
   rett executable blei testa. Den strenge CRL-avvisninga skal bestå.
5. Tidlegare upublisert lokalarbeid (`691112b3d75ea53df62561d1008e28589ad78ec0`,
   med seinare differensialendringar) er ikkje del av det publiserte treet og var
   ikkje tilgjengeleg i det gjenoppretta arbeidsmiljøet. GC-retting for store
   bytes/lister, kjeldebygd CI-runtime og resten av differensialrettinga må
   gjenskapast og verifiserast før dei kan reknast som leverte. Den flate
   gjentekne store-fil-prøven treng framleis ei avgrensa retting utan høgare
   heapgrense.
6. Køyr alle kandidat-/seed-portar på faktisk plattform og godkjenn byteparitet
   før promotering. Full CI er framleis eit krav før eventuell innfletting.

Siste undersøkte a0-køyringar gav grøn
[codegen-refresh](https://github.com/Norscode/Norscode/actions/runs/43104465131),
[Krypto-smoke](https://github.com/Norscode/Norscode/actions/runs/43104465151) og
[Windows-smoke](https://github.com/Norscode/Norscode/actions/runs/43104465094).
[Hovud-CI](https://github.com/Norscode/Norscode/actions/runs/43104466045) var raud,
og [ARM64](https://github.com/Norscode/Norscode/actions/runs/43104465073) hadde
differensial-/TLS-/paritetsfeil. Dette er status for a0, ikkje for den nye samla
tippen. #276 skal stå som draft til dei attståande portane er løyste.
