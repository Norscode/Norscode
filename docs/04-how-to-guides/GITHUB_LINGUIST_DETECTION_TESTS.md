# Norscode .no – eksempler for språkgjenkjenning

Dette er testgrunnlag for framtidig GitHub Linguist-integrasjon, **ikke** en integrasjon med Linguist sin klassifiserer. Kjør positive og negative tilfeller mot faktiske Linguist-heuristikker etter upstream-implementasjon.

## Positive (.no-kildekode)

`compiler/compiler.no`: `pakke compiler`, `bruk std.runtime`, `struktur CompilerRuntimeState {`, `funksjon init_runtime() -> CompilerRuntimeState {`, `returner CompilerRuntimeState {`.

`std/tomllib.no`: `funksjon _trim(s: tekst) -> tekst {`, `la teikn = builtin.split(s, "")`, `mens (start < n og ...)`.

`std/ghash.no`: `funksjon _rshift1(v: liste_heiltall) -> liste_heiltall {`, `la carry = 0`, `returner ut`.

`toolchain/norspkg/cache.no`: `funksjon cache_lag()`, `returner {`, `slutt`.

Alle disse er faktisk observert i de oppgitte kildefilene. Ingen kildekode er kopiert inn som egen `.no`-fixture.

## Negative (vanlige .no-tekstfiler, forslag til testvektorer)

- Norske tekstlinjer: `Dette er en vanlig tekstfil på norsk.`
- Overskrifter og lenker: `# Kontaktinformasjon` og `https://eksempel.no`
- Prosa med isolert ord: `En funksjon brukes for å beskrive formål.`
- Prosa med ordet returner: `Vennligst returner varen innen fristen.`
- Tekst som inneholder `pakke` eller `bruk` uten kodekonstruksjoner.

## Akseptansekriterier

1. Ingen klassifisering som Norscode på negative tilfeller.
2. Positive eksempler klassifiseres som Norscode.
3. Enkeltstående norske ord er ikke tilstrekkelige; krev kombinasjoner av kildekode-signaler som `funksjon <identifikator>(`, `returner` og blokksyntaks.
4. Ingen endring i eksisterende `Text`-deteksjon for vanlige `.no`-filer.
5. Upstream Linguist-testene må passere, og testdekningen må vise både positive og negative tilfeller.

## Lokal prototype

`tests/test_github_linguist_detector_prototype.no` sjekker fire ekte positive kildefiler og to kompakte positive kodeeksempler, samt elleve negative tekst-/kodefragmenter. Den sjekker linjer og ignorerer linjer som starter med kommentarer eller anførselstegn. Den krever funksjonsdeklarasjon med parenteser, kodeblokk og retursetning på en kodelinje. Metoden er bevisst konservativ og kan overse gyldig Norscode som ikke bruker `returner`. Den er integrert som eget obligatorisk steg i `format-lint-check` i `.github/workflows/ci.yml` og kjøres med `./bin/nc run tests/test_github_linguist_detector_prototype.no`.

**Avgrensning:** Dette er en lokal Norscode-prototype og kun et første datasett. Den kan gi både falske positive og falske negative for andre kilder, støtter ikke Linguists faktiske heuristikk-format og endrer ikke GitHubs klassifisering. Flere ekte uavhengige positive og negative kilder og upstream-kryssvalidering trengs.

**Verifisering:** CI #2051 og Krypto-smoke #750 fullførte med `success` på commit `9bc01a4`. Metadata-, regex- og deteksjonsprototypetesten bestod.

## Direkte test av foreslått Linguist-regex

- `tests/test_github_linguist_heuristic_regex.no` kontrollerer de **samme mønstrene som ligger i** `linguist-proposal/norscode-heuristics.yml` (strenglikhet mot YAML-innholdet).
- Den kjører hver regexp med `std.regex` og multiline-flagg mot fem positive kodeeksempler og elleve negative eksempler. Resultatet blir en reell feilreturkode ved regresjon, ikke bare `assert`.
- Testen er koblet til GitHub Actions `format-lint-check` via `./bin/nc run tests/test_github_linguist_heuristic_regex.no`.
- Mønstrene benytter bare ASCII-klasser for å kunne testes i både Norscodes regex-motor og Ruby-regex. Navn som bare kan beskrives med andre Unicode-bokstaver er en kjent falsk-negativ risiko.
- Uavhengig lokal Ruby-regex-kontroll av tilsvarende mønster med tre positive og seks negative eksempler ble gjennomført. **Dette er ikke** `github-linguist/linguist` sin testpakke eller distribuerte GitHub-detektering.

**Obs:** `std.regex` støtter `(?m)` eksplisitt for flerlinjeankre. Ruby bruker sine egne ankerregler. Full upstream `script/check-regex-compatibility`, samples, deteksjon og kryssvalidering gjenstår. 

## Utvida Linguist-heuristikk og tverrprosjektkontroll (30. oktober 2026)

Heuristikkforslaget i `linguist-proposal/norscode-heuristics.yml` har no **to Norscode-reglar**: ei for kompakte funksjonar på éi line, og ei for funksjonssignatur + innrykka `returner` + blokkslutt. Begge krev at filinnhaldet ikkje har ei line som startar med `\`\`\`` (kodegjerde, ofte brukt i dokumentasjon), og `Text` står sist som reserveutfall.

`tests/test_github_linguist_heuristic_regex.no` kontrollerer no **5 positive og 11 negative kode-/teksttilfelle**, inkludert kompakte funksjonar og `.no`-tekst med inngjerda kode. Den verifiserer òg at desse regex-strengane finst i YAML-utkastet. Ein separat kontroll av dei 16 tilfella mot JavaScript-regex ga ingen avvik; **CI på den nyaste endringa må verifiserast separat**.

Eg kontrollerte òg funksjons-, retur- og blokksluttmønstra mot 10 faktiske, offentlege Norscode-kildefiler fordelt over fire andre prosjekt under **same eigar**:

- `Norscode/NorscodeOS`: `m1.no`, `vev.no`, `sntp.no`
- `Norscode/NorscodeStudio-v39`: `studio-core/studio_core.no`
- `Norscode/NorscodeAI-Gen3`: `app.no`, `src/chat.no`, `src/nett.no`
- `Norscode/Delekatalog-Norscode`: `kjor.no`, `publiser.no`, `verktoy/importer.no`

Alle ti hadde treff på dei tre fleirlinjemønstra. Dette er **intern tverrprosjektdekning, ikkje uavhengig adopsjon**; filene er heller ikkje kopierte inn i prøvesamlinga. Å kontrollere positive og negative eksempel mot Ruby/Linguist sine eigen testar står framleis att.

**Kjende avgrensingar:** Eit Norscode-program utan `returner` eller berre med uttrykksbasert kontrollflyt kan bli feilklassifisert som tekst. Eit Norscode-program med kodegjerde i ein streng eller kommentar kan bli feilklassifisert som tekst. Desse tiltaka prioriterer å unngå falske positive, men krev vidare validering før upstream-innsending.

## Heuristics YAML-kontrakt (30. oktober 2026)

GitHub Linguist si [`Heuristics.load`-implementering](https://github.com/github-linguist/linguist/blob/main/lib/linguist/heuristics.rb) les `data['named_patterns'].map` før ho går gjennom `disambiguations`. Eit isolert YAML-forslag utan `named_patterns` er derfor ikkje ein komplett, lastbar heuristikk-konfigurasjon. Utkastet inneheld no den tomme mappa `named_patterns: {}`.

`tests/test_github_linguist_yaml_contract.no` er ein ny **obligatorisk Norscode CI-test** som sjekkar:

- `named_patterns: {}` og `disambiguations` i korrekt rekkefølgje
- Nøyaktig éi `.no`-gruppe, to Norscode-reglar med `and`, og éin avsluttande `Text`-fallback
- Fire positive regex-mønster og to `negative_pattern`-reglar, ein for kvar Norscode-regel

Testen er medvite avgrensa til den kjende forslagsstrukturen og er **ikkje** ein full YAML-parser. `tests/test_github_linguist_heuristic_regex.no` kontrollerer i tillegg dei konkrete mønsterstrengane og positive/negative innhaldseksempel. Ekte `github-linguist/linguist`-testar, mellom anna `test/test_heuristics.rb`, er ikkje køyrde mot ein upstream-endring. Ingen Github-språkregistrering er publisert.
