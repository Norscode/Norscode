# V9600 til 100% plan

Dette dokumentet samlar dei neste store omgangane for å ta Norscode frå dagens sterke, men framleis ujamne tilstand, til ein mykje nærare 100%-situasjon.

## Målbilete

Vi vil ha:
- grøn native/runtime på macOS og Linux
- stabil CLI for `compile`, `check`, `run`, `run-ncb`, `bundle`
- selfhost- og bootstrap-løype utan skjulte gamle spor
- ekte CMS/admin-flyt i NorscodeWeb
- AI-modul som kan planlegge og byggje trygt innanfor guard-rammer
- oppdaterte statusdokument som samsvarer med faktisk tilstand

## Omgang A: v9600-v9800

Mål:
- få statusdokumentasjon i sync med grøne native- og CLI-banar
- løfte CMS frå basisgenerator til arbeidsklar redaktørflyt

Sjekkliste:
- [x] ekte filbasert CMS-lesing
- [x] ekte filbasert CMS-lagring
- [x] generator for standardsider
- [x] CMS-status via JSON
- [ ] oppdatere gamle v-statusfiler til ny sann status
- [ ] lage rein regressjonsliste for CMS-rutene

## Omgang B: v9801-v10000

Mål:
- breiare regressjon for selfhost/compiler/bundler
- rydde vekk siste gamle embedded/stale baner der dei framleis finst

Sjekkliste:
- [x] smoke for Norscode-eigd compilerbane utan Node/JavaScript
- [ ] smoke for native candidate build + promotion path
- [x] regressjon for `INDEX_GET`, `BUILD_LIST`, `BUILD_MAP`
- [x] regressjon for `nc bundle` med fleire modular
- [x] dokumentere aktiv compiler-dispatch og eigarskapsgate

## Omgang C: v10001-v10400

Mål:
- gjere AI-klargjeringa meir ekte enn demo

Sjekkliste:
- [ ] tydeleg skilje mellom patch-plan, patch-forslag og faktisk utføring
- [ ] guard rundt kritiske filer og katalogar
- [ ] trygg arbeidsmappe for AI-genererte utkast
- [ ] enkel audit-logg for AI-endringar

## Omgang D: v10401-v10800

Mål:
- ta web/CMS frå DEV-demo til liten, men reell redigeringsplattform

Sjekkliste:
- [ ] betre render av CMS-innhald
- [ ] opprette nye sider/slugs frå admin
- [ ] slett/arkiver side med eksplisitt DEV-vern
- [ ] publiseringsstatus og sideoversikt
- [ ] eksport/import av CMS-data

## Omgang E: v10801-v11200

Mål:
- fullføre release- og produksjonsdisiplin

Sjekkliste:
- [ ] oppdatert backup-/rollback-dokumentasjon
- [ ] brei statusmatrise per plattform
- [ ] release-checklist som samsvarer med faktisk kode
- [ ] oppdatert production-ready-definisjon

## Det som mest truleg gir raskast prosentgevinst

1. Oppdatere utdaterte statusdokument
2. Køyre breiare selfhost-regresjon
3. Fullføre CMS med oppretting av nye sider
4. Rydde AI-flyt frå demo til trygg semiautonomi

## Arbeidsregel

Vi skal halde fast på:
- ikkje overskrive `dist/norscode_native` utan tydeleg grunn
- ikkje overskrive `bootstrap/stage0` utan kontrollert løype
- ikkje påstå 100% før status, regressjon og dokumentasjon stemmer med kvarandre
