# Kandidater for representative GitHub Linguist-samples

Dette er **ekte kildefiler i Norscode-repositoriet**, ikke oppdiktede «Hello world»-eksempler. Filene er foreløpig bare kandidatgrunnlag; ingenting er kopiert til Linguist.

| Fil | Hvorfor kandidat | Syntaks |
| --- | --- | --- |
| [std/ordbok.no](../../std/ordbok.no) | Ordbokoperasjoner i standardbiblioteket | typed funksjoner, ordbøker, indeksering, `returner` |
| [std/tomllib.no](../../std/tomllib.no) | Ekte standardbibliotek / parsing | kommentarer, funksjon, typed verdier, `mens`, `hvis` |
| [std/ghash.no](../../std/ghash.no) | Kryptografisk standardbibliotek | lister, bitoperasjoner, `mens`, indeksering |
| [std/tekst.no](../../std/tekst.no) | Tekstbehandling i standardbiblioteket | funksjoner, tekstverdier, løkker og betingelser |

## Kontrollpunkter før upstream-PR

1. Alle fire sample-kandidatene er hentet fra `std/`, som beholder **Apache License 2.0** etter lisensdelingen. Lisensen ligger i [`LICENSE-APACHE`](../../LICENSE-APACHE); se [omfanget](../../LICENSING.md). Eksportøren kopierer denne til sample-områdets `LICENSE`. Før innsending må eventuelle særskilte filoverskrifter/NOTICE og korrekt attribusjon kontrolleres. Dette er ikke det samme som godkjent innsendelse.
2. Vurder syntaksvariasjon (blokker med `{}` kontra `slutt`) og at grammatikken faktisk dekker eksemplene.
3. Test TextMate-grammatikken og importer den med Linguists `script/add-grammar`; GitHub krever en grammatikkilde med kompatibel lisens.
4. Gjennomfør GitHub-kodesøk etter faktisk `.no`-bruk. Dokumenter antallet indekserte filer fra siste år, uten forks, og fordeling mellom uavhengige eiere/repositorier. Ikke oppgi et anslått tall som verifisert.
5. `.no` kan brukes til andre formål, så vurder om språkdeteksjonen trenger heuristikk og tilstrekkelige negative tester.
6. Kontroller Linguists gjeldende `CONTRIBUTING.md`, fyll ut PR-malen og inkluder originalkildelenker og tydelig lisensinformasjon.

**Kilde:** https://github.com/github-linguist/linguist/blob/main/CONTRIBUTING.md

**Status:** Eksportkandidatene er begrenset til standardbibliotekets Apache-2.0-omfang. Lisensvalget for kompilatoren endrer ikke disse filene. Filspesifikke rettigheter, brukstelling og upstream-test må fortsatt kontrolleres før innsending.

## Konkrete søk for bruk i GitHub

- [GitHub-kodesøk: Norscode-filer](https://github.com/search?q=path%3A*.no+%22funksjon%22+%22returner%22&type=code)
- [GitHub-kodesøk: andre eiere enn Norscode](https://github.com/search?q=path%3A*.no+%22funksjon%22+%22returner%22+-user%3ANorscode&type=code)

**Viktig:** Søkene er bare innganger til manuell kontroll, ikke verifiserte tall. Tellekravet gjelder indekserte filer siste år uten forks, og distribusjon på uavhengige brukere. Skillet mellom dato for filindeksering og sist oppdatert repository må vurderes etter GitHubs egen metodikk.
