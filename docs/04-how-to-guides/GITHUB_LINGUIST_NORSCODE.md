# GitHub Linguist: få Norscode gjenkjent

## Status
GitHub bruker [github-linguist](https://github.com/github-linguist/linguist) for å identifisere programmeringsspråk, språkstatistikk og syntaksfarger. Norscode kan ikke registreres som eget GitHub-språk kun via `.gitattributes`. Derfor skal vi **ikke** feilmerke `*.no` som et annet språk.

## Eksisterende grunnlag
- Utvidelser: `.no` og `.nors`. Merk at `.no` kan overlappe andre bruksområder.
- Norscode-syntaks: `vscode-norscode/syntaxes/norscode.tmLanguage.json`.
- VS Code-språkdefinisjon: `vscode-norscode/package.json`.
- Eksisterende `.gitattributes`: linjeslutt og filtrering av genererte artefakter. Behold dette.

## Vei til offisiell støtte
1. Vedlikehold en offentlig grammatikk med stabil URL, tydelig lisens og `source.norscode`-scope. Undersøk om eksisterende TextMate-grammatikk kan brukes direkte.
2. Velg ekte, representative Norscode-kildeeksempler (ikke genererte filer) for Linguists `samples/`.
3. Tell antall **indekserte, offentlige** filer med `.no` og `.nors` fra uavhengige GitHub-repositorier og dokumenter hva som faktisk er Norscode. Ikke anta at antallet repo-filer tilsvarer GitHubs indekserte antall.
4. Sjekk gjeldende krav i [Linguist CONTRIBUTING](https://github.com/github-linguist/linguist/blob/main/CONTRIBUTING.md). Per oktober 2026 beskriver retningslinjene minst 2000 indekserte filer for en gjentakende filendelse i løpet av ett år (uten forks); kontroller kravet igjen før innsending.
5. Når kravene er oppfylt: opprett separat PR mot `github-linguist/linguist` med språkdefinisjon i `languages.yml`, grammatikk, samples og bruksbevis. GitHub vedtar selv om språket tas inn.

## Verifisering
- På GitHub: kontroller at Linguist faktisk har akseptert og publisert Norscode i `languages.yml`.
- Etter at GitHub har rullet ut oppdateringen: sjekk at `.no` og `.nors` vises med riktig språk og farger i et offentlig Norscode-repo.
- Kontroller at språkstatistikken er oppdatert etter en normal endring på standardgrenen.
- Ikke rapporter «GitHub støtter Norscode» før dette er bekreftet.

## Kilder
- https://github.com/github-linguist/linguist/blob/main/CONTRIBUTING.md
- https://github.com/github-linguist/linguist/blob/main/docs/overrides.md
- https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-repository-languages
