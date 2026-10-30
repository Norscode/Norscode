# GitHub Linguist: få Norscode gjenkjent

## Status
GitHub bruker [github-linguist](https://github.com/github-linguist/linguist) for å identifisere programmeringsspråk, språkstatistikk og syntaksfarger. Norscode kan ikke registreres som eget GitHub-språk kun via `.gitattributes`. Derfor skal vi **ikke** feilmerke `*.no` som et annet språk.

## Eksisterende grunnlag
- Offisiell kildefilendelse: `.no`. Tidligere `.nors` omtales som eldre alternativ, ikke som anbefalt eller offisiell kildefilendelse. **Bekreftet:** GitHub Linguist har allerede `.no` under språket `Text` (`languages.yml`, Text-seksjonen). Dette krever eksplisitt disambiguering ved upstream-registrering; `.no` skal likevel være offisiell Norscode-filendelse.
- Norscode-syntaks: `vscode-norscode/syntaxes/norscode.tmLanguage.json`.
- VS Code-språkdefinisjon: `vscode-norscode/package.json`.
- Eksisterende `.gitattributes`: linjeslutt og filtrering av genererte artefakter. Behold dette.

## Vei til offisiell støtte
1. Vedlikehold en offentlig grammatikk med stabil URL, tydelig lisens og `source.norscode`-scope. Undersøk om eksisterende TextMate-grammatikk kan brukes direkte.
2. Velg ekte, representative Norscode-kildeeksempler (ikke genererte filer) for Linguists `samples/`.
3. Tell antall **indekserte, offentlige** filer med `.no` fra uavhengige GitHub-repositorier og dokumenter hva som faktisk er Norscode. Ikke anta at antallet repo-filer tilsvarer GitHubs indekserte antall.
4. Sjekk gjeldende krav i [Linguist CONTRIBUTING](https://github.com/github-linguist/linguist/blob/main/CONTRIBUTING.md). Per oktober 2026 beskriver retningslinjene minst 2000 indekserte filer for en gjentakende filendelse i løpet av ett år (uten forks); kontroller kravet igjen før innsending.
5. Når kravene er oppfylt: opprett separat PR mot `github-linguist/linguist` med språkdefinisjon i `languages.yml`, grammatikk, samples og bruksbevis. GitHub vedtar selv om språket tas inn.

## Verifisering
- På GitHub: kontroller at Linguist faktisk har akseptert og publisert Norscode i `languages.yml`.
- Etter at GitHub har rullet ut oppdateringen: sjekk at `.no` vises med riktig språk og farger i et offentlig Norscode-repo.
- Kontroller at språkstatistikken er oppdatert etter en normal endring på standardgrenen.
- Ikke rapporter «GitHub støtter Norscode» før dette er bekreftet.

## Kilder
- https://github.com/github-linguist/linguist/blob/main/CONTRIBUTING.md
- https://github.com/github-linguist/linguist/blob/main/docs/overrides.md
- https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-repository-languages

## Bekreftet konflikt med GitHub Linguist

Per 27. oktober 2026 er `.no` registrert som `Text` i [Linguists languages.yml](https://github.com/github-linguist/linguist/blob/main/lib/linguist/languages.yml). `Norscode` finnes ikke i dette registeret. Før upstream-PR må vi derfor ta høyde for både eksisterende tekstfiler og nye Norscode-kildefiler. Linguists retningslinjer ber om minst to representanter for hvert språk som deler endelsen og eventuelt en heuristikk når innholdet kan skilles. Norscode-kjennetegn kan være `funksjon`, `returner`, `pakke`, `bruk`, `la`, `heiltall`, `slutt`; én enkelt forekomst bør ikke automatisk gi positiv klassifisering. Ingen heuristikk er skrevet eller testet mot upstream ennå.


## Konkrete upstream-forslag i denne PR-en

- [norscode-language.yml](linguist-proposal/norscode-language.yml): foreslått `Norscode`-oppføring til `lib/linguist/languages.yml`; `language_id` skal opprettes med Linguists `script/update-ids`.
- [norscode-heuristics.yml](linguist-proposal/norscode-heuristics.yml): forslag til `.no`-regel i `lib/linguist/heuristics.yml`. Finner funksjonssignatur, innrykket `returner` og avslutning med `}` eller `slutt`; beholder `Text` som reserveutfall.
- Reglene er prøvd mot representative positive og negative utdrag med en lokal YAML/regex-kontroll, **ikke** mot Linguists offisielle testpakke eller GitHub.com.
- GitHub Linguist sin [lisensliste](https://github.com/github-linguist/linguist/blob/main/vendor/licenses/config.yml) har `apache-2.0` som tillatt grammatikklisens. Rotlisensen til Norscode er Apache-2.0, men `script/add-grammar` og eventuelle særskilte filnotiser må fortsatt verifiseres.
- Verken språkregisteret, heuristikkene eller grammatikken er oppdatert hos github-linguist ennå. **Ikke send upstream PR før bevis for GitHub-bruk på uavhengige kontoer og kildegrunnlag er kontrollert.**

Når bruken er dokumentert, må fragmentene flettes inn i deres faktiske YAML-filer i korrekt alfabetisk rekkefølge, grammatikken legges inn via `script/add-grammar`, `script/update-ids` kjøres, det legges inn representative samples og minst to eksempler for hvert språk som deler `.no`, og deres egne `test/test_blob.rb`-/heuristikk- og kryssvalideringstester kjøres. Dette er krav til *ekstern Linguist-integrasjon*, ikke nye runtime-avhengigheter for Norscode.

## Generert grammatikkpakke utan JSON-ratchet-brot (30. oktober 2026)

GitHub Linguist sitt `script/add-grammar` importerer eit **eige Git-repositorium som submodul**, ikkje berre ei undermappe frå Norscode. Opphavleg staging under `linguist-proposal/grammar-repo` la til to nye JSON-filer som fekk Norscode sitt F0.1-gate til å feile: **60 spora JSON-filer mot taket 58** i CI #2044, jobb «Driftsvakt». Dette var ein faktisk gate-feil, ikkje ein feil i Linguist-regex-testane.

Løysinga er **ikkje å auke ratchet-taket**. Begge dei dupliserte JSON-filene og den dupliserte lisensen er fjerna frå Git-indeksen. I staden køyrer ein:

```sh
./bin/nc run tools/eksporter_linguist_grammatikk.no
```

Dette Norscode-programmet lagar ein fullstendig, separat eksport i den genererte katalogen `build/norscode-linguist-grammar/` med `package.json`, `syntaxes/norscode.tmLanguage.json`, `LICENSE` og `README.md`. Det les den **kanoniske** VS Code-grammatikken og rotlisensen, og validerer ved å lese eksporten tilbake. `.json`-filene i `build/` er genererte, ikkje nye spora kjelder.

- [Eksportinstruksjonar](linguist-proposal/grammar-repo/README.md)
- `tests/test_github_linguist_metadata.no` kontrollerer at eksportprogrammet og kjeldefilene har rett identitet, medan eksportprogrammet sjølv kontrollerer faktisk utdata.
- Eige steg i `.github/workflows/ci.yml` køyrer eksportprogrammet utan Python/C/shell for sjølve eksporten.

**Status:** Rettelsen er committta, men nyaste CI/Krypto må stadfestast separat. Det er ikkje oppretta noko eksternt grammatikklager. Oppfyll uavhengige brukskrav og køyr Linguist sine eigne testar før upstream-PR.
