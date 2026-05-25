# Selvstendig Norscode — resten av reisen

Mål:
Ferdigstille de siste praktiske delene av selvstendighet etter at core-selfhost og full språkparitet er på plass. Denne planen handler om å gjøre Norscode enklere å bygge, teste, distribuere, vedlikeholde og videreutvikle uten skjulte Python-avhengigheter.

Denne planen er bevisst kort og operativ. Den skal være lett å følge, lett å avkrysse og lett å knytte til konkrete leveranser.

## Kort status

- [x] Core selfhost-spor er etablert
- [x] Full språkparitet er låst i egen roadmap
- [x] Frontend/HTML er et separat, ferdig spor
- [x] Binary og distribusjon finnes allerede
- [x] Resten av bootstrap-flaten er helt slanket
- [x] Alle fallback-veier er eksplisitt dokumentert og minimale
- [x] Release, CI og vedlikehold er helt selvforklarende for nye bidragsytere

## Definisjon av ferdig

Resten av selvstendigheten regnes som ferdig når:

- `nc` kan bygges og verifiseres uten skjulte Python-steg i normalflyten
- fallback er eksplisitt, liten og lett å slå av/på
- releaseprosessen er repeterbar og fullstendig dokumentert
- CI fanger regresjoner i bootstrap, fallback og distribusjon tidlig
- nye bidragsytere kan forstå hvor Python fortsatt brukes uten å lese kildekoden først

## Hovedstrategi

Bygg de siste milepælene som små, avkryssbare omganger:

1. Kartlegg og reduser resten av bootstrap
2. Lås fallback-kontrakten og fjern implisitte veier
3. Bygg ut release/CI som selvstendig produktflyt
4. Hardenn vedlikehold, observability og testdekning
5. Rydd docs, eksempler og deprecations

## Omgangsoversikt

- [x] Omgang 1 — Gjenværende bootstrap-rydding
- [x] Omgang 2 — Eksplisitt og minimal fallback
- [x] Omgang 3 — Release- og installasjonsflyt
- [x] Omgang 4 — CI og regressionsvern
- [x] Omgang 5 — Vedlikehold, observability og feilsøking
- [x] Omgang 6 — Dokumentasjon, migrering og deprecations

## Omgang 1 — Gjenværende bootstrap-rydding

Mål:
Finne og redusere de siste stedene der bootstrap fortsatt bærer mer logikk enn nødvendig.

Leveranser:

- [x] Full inventarliste over gjenværende Python-burden i normalflyten
- [x] Tydelig skille mellom bootstrap, verktøy og produktlogikk
- [x] Oppdatert kart for wrapper-filer, startskript og hjelpekommandoer
- [x] Liste over filer som kan gjøres tynnere eller flyttes til Norscode-moduler

Ferdig når:

- [x] Ingen overraskende Python-kall skjer i vanlig brukerflyt
- [x] Alle gjenværende Python-steg er dokumentert med grunn og forventet levetid

Status:

Omgang 1 er fullført.

## Omgang 2 — Eksplisitt og minimal fallback

Mål:
Gjøre fallback til en bevisst nødvei, ikke en skjult standard.

Leveranser:

- [x] Én tydelig CLI-vei for eksplisitt fallback
- [x] Felles forklaring på hva fallback gjør og ikke gjør
- [x] Test som verifiserer at normalflyt ikke bruker fallback
- [x] Test som verifiserer at fallback bare slår inn når den er eksplisitt bedt om

Ferdig når:

- [x] Bruker kan forstå fallback uten å lese internimplementasjon
- [x] Fallback oppfører seg likt på tvers av støttede kommandoer

Status:

Omgang 2 er fullført.

## Omgang 3 — Release- og installasjonsflyt

Mål:
Gjøre publisering, installasjon og rollback helt forutsigbart.

Leveranser:

- [x] Release-sjekkliste som kan kjøres punkt for punkt
- [x] Verifiserbare artefakter med checksums/signaturer
- [x] Installasjon og oppgradering med tydelige feilmeldinger
- [x] Enkel rollback-prosedyre for feilfrigivelser

Ferdig når:

- [x] En release kan produseres og bekreftes mekanisk
- [x] En bruker kan installere og oppgradere uten utviklerassistanse

Status:

Omgang 3 er fullført.

## Omgang 4 — CI og regressionsvern

Mål:
Gjøre at regresjoner i bootstrap og fallback blir stoppet før de når brukere.

Leveranser:

- [x] CI-suiter for bootstrap og fallback-veier
- [x] Små, representative install-/build-/run-smoke tests
- [x] Fast terskel for når en endring regnes som regresjon
- [x] Rapportering som gjør feilsøking rask

Ferdig når:

- [x] CI fanger opp skjulte fallback-brudd
- [x] Reproduserbare testscenarier finnes for vanlige feilklasser

Status:

Omgang 4 er fullført.

## Omgang 5 — Vedlikehold, observability og feilsøking

Mål:
Gjøre prosjektet lett å forstå og drifte over tid.

Leveranser:

- [x] Tydelige diagnosekommandoer
- [x] Konsistent feilmeldingsformat i bootstrap og release-verktøy
- [x] Observability for hva som skjer i bygge-/testflyten
- [x] Hjelp og `--help` som beskriver normalen, ikke historikken

Ferdig når:

- [x] Feil er enkle å diagnostisere uten å gå inn i kildekoden
- [x] Vedlikeholdere kan se hva som skjedde i en mislykket pipeline raskt

Status:

Omgang 5 er fullført.

## Omgang 6 — Dokumentasjon, migrering og deprecations

Mål:
Rydde siste spor av gammel historie og gjøre støtteflaten tydelig.

Leveranser:

- [x] Oppdatert dokumentasjon for gjenværende brukerflyt
- [x] Deprecation-/migreringspolicy for gamle kommandoer og aliaser
- [x] Én leserekkefølge for nye bidragsytere
- [x] Kort oversikt over hva som fortsatt er bootstrap og hvorfor

Ferdig når:

- [x] Dokumentasjonen forklarer den faktiske produktflaten
- [x] Legacy er tydelig merket og lett å forstå som midlertidig

Status:

Omgang 6 er fullført.

## Anbefalt rekkefølge

- [x] Gjenværende bootstrap-rydding
- [x] Eksplisitt og minimal fallback
- [x] Release- og installasjonsflyt
- [x] CI og regressionsvern
- [x] Vedlikehold, observability og feilsøking
- [x] Dokumentasjon, migrering og deprecations

## Kortversjon

Det som gjenstår etter Omgang 10 er ferdig når:

- [x] bootstrap er minimal og ikke overrasker
- [x] fallback er eksplisitt og kontrollert
- [x] release og installasjon er repeterbar
- [x] CI stopper regresjoner tidlig
- [x] docs gjør resten av historien lett å forstå
