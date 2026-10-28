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

**Status per CI #2022 (commit `09325cf`):** både metadata-test og den første prototypetesten er bekreftet grønne. Prototypen er siden utvidet med ekstra negative tilfeller og linjebasert deteksjon; denne nye versjonen har ikke verifisert CI-resultat ennå.

## Direkte test av foreslått Linguist-regex

- `tests/test_github_linguist_heuristic_regex.no` kontrollerer de **samme tre mønstrene som ligger i** `linguist-proposal/norscode-heuristics.yml` (strenglikhet mot YAML-innholdet).
- Den kjører hver regexp med `std.regex` og multiline-flagg mot tre positive kodeeksempler og åtte negative eksempler. Resultatet blir en reell feilreturkode ved regresjon, ikke bare `assert`.
- Testen er koblet til GitHub Actions `format-lint-check` via `./bin/nc run tests/test_github_linguist_heuristic_regex.no`.
- Mønstrene benytter bare ASCII-klasser for å kunne testes i både Norscodes regex-motor og Ruby-regex. Navn som bare kan beskrives med andre Unicode-bokstaver er en kjent falsk-negativ risiko.
- Uavhengig lokal Ruby-regex-kontroll av tilsvarende mønster med tre positive og seks negative eksempler ble gjennomført. **Dette er ikke** `github-linguist/linguist` sin testpakke eller distribuerte GitHub-detektering.

**Obs:** `std.regex` støtter `(?m)` eksplisitt for flerlinjeankre. Ruby bruker sine egne ankerregler. Full upstream `script/check-regex-compatibility`, samples, deteksjon og kryssvalidering gjenstår. 
