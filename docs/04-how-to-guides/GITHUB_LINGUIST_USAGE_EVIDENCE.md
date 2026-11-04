# GitHub Linguist – bevis for Norscode-bruk

**Kontrolldato:** 28. oktober 2026  
**Status:** Foreløpig **ikke** dokumentert tilstrekkelig uavhengig bruk for innsending.

## Hva GitHub Linguist krever

For en gjentakende kildefilendelse som `.no` krever Linguists gjeldende [CONTRIBUTING.md](https://github.com/github-linguist/linguist/blob/main/CONTRIBUTING.md#language-extension-and-filename-usage-requirements) normalt **minst 2 000 indekserte filer per filendelse siste år, uten forks**, og en rimelig fordeling over *uavhengige eiere og repositorier*. Filene må også faktisk inneholde Norscode, fordi `.no` er registrert som Text og brukes av andre tekstformater. Ikke bruk antall resultater for en løs nøkkelordspørring som dette tallet.

## Foreløpige, reproduserbare observasjoner

Jeg undersøkte kode via den tilknyttede GitHub-kodesøk-funksjonen:

| Søk | Avgrenset observasjon | Hva det **ikke** beviser |
| --- | --- | --- |
| `bruk std.tekst som`, de første 100 returnerte kodetreffene | 100/100 treff lå under eieren `Norscode`: `NorscodeOS` 50, `Gateway-v2` 29, `Norscode` 12, `norscode.no` 6, `NorscodeAI-Gen3` 2, `Gateway` 1 | Ikke antall indekserte filer siste år; søket er begrenset til de første 100 treffene og ett kodeuttrykk |
| Repositoriosøk `Norscode`, første resultatside (100 etterspurt) | 14 returnerte repositorier, alle under eieren `Norscode` | Ikke fullstendig antall repositorier som inneholder Norscode-kode under andre navn |
| Globalt kodetekstsøk `Norscode` | Det finnes også irrelevante treff med ordstrengen på andre kontoer | Søk på språkets navn alene gir ikke bevis for uavhengig programkode |

**Konklusjon:** Disse søkene dokumenterer faktisk bruk **i flere repositorier under samme eier**, men viser ikke en tilstrekkelig spredning på uavhengige eiere. Ingen global indeks-telling er bekreftet. Dette er **ikke** grunnlag for å hevde at GitHub Linguists terskel er oppfylt.

## Etterprøvbar søkeprosedyre

Søkene under er startpunkter i GitHub Code Search, ikke verifiserte søketall:

1. [Norscode-kildesøk med `.no` og typiske nøkkelord](https://github.com/search?q=path%3A*.no+%22funksjon%22+%22returner%22&type=code)
2. [Samme søk, men utelat eieren Norscode](https://github.com/search?q=path%3A*.no+%22funksjon%22+%22returner%22+-user%3ANorscode&type=code)
3. [Norscode-kilde med `bruk std.tekst som`](https://github.com/search?q=path%3A*.no+%22bruk+std.tekst+som%22&type=code)

Ved kontroll i GitHub-webgrensesnittet:

- Sett begrensning til `.no`, søk etter flere ulike **kode**kjennetegn, og verifiser at treffene virkelig er kildekode.
- Fjern forks og noter hvordan indekseringsvindu og GitHubs søkegrensesnitt påvirker resultatet.
- Filtrer bort `Norscode`-eieren, og tell distinkte, uavhengige `owner/repository`-kombinasjoner. Legg ved kildelenker.
- Registrer antall som GitHubs søkegrensesnitt faktisk rapporterer. Ikke summer søketreff fra ulike spørringer (samme fil kan matche flere).
- Kontroller forveksling med vanlig tekst (`.no`), kopier av kilder, genererte filer og lisens per sample.
- Oppdater dette dokumentet med datert, etterprøvbar oppsummering. Opprett *ikke* upstream PR hvis kriteriene fortsatt ikke kan dokumenteres.

GitHub Code Search kan benytte `path:*.no` som filstifilter. Se [GitHub Docs: Understanding GitHub Code Search syntax](https://docs.github.com/en/search-github/github-code-search/understanding-github-code-search-syntax) for oppdaterte søkeregler.

## Neste avgjørelsespunkt

Hvis faktisk, uavhengig bruk er tilstrekkelig dokumentert, kan en **separat** PR til `github-linguist/linguist` lages med grammatikk, samples, `languages.yml`, heuristikk og deres valideringstester.

Hvis den ikke er det, bør utviklingen prioritere at flere **ekte, uavhengige** prosjekter tar Norscode i bruk. Ikke opprett kunstige repositorier eller dupliser filer for å påvirke statistikken.
