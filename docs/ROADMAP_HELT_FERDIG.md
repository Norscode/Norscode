# ROADMAP HELT FERDIG

Mål:
Gjøre Norscode helt ferdig som produkt. Det betyr at normal bruk er binary-first, Python bare er et eksplisitt bootstrap-verktøy, og release-, installasjons- og vedlikeholdsflaten er så tydelig at prosjektet kan brukes uten å kjenne historikken.

Dette er ikke en videreføring av den gamle språk-roadmapen. Den er allerede fullført. Denne planen beskriver det som gjenstår før Norscode kan regnes som helt selvstendig i praksis.

## Hva "helt ferdig" betyr

- [x] normal kjøring trenger ikke Python
- [x] installasjon og oppgradering kan gjøres uten utviklerkunnskap om bootstrap
- [x] dokumentasjon, hjelpekommandoer og eksempler peker samme vei
- [x] releaseprosessen er repeterbar og verifiserbar
- [x] kvaliteten er god nok til at regressjoner oppdages før brukere gjør det

## Etappe 1: Fjern implisitt Python-fallback

Hvorfor:
Norscode er først virkelig selvstendig når den vanlige CLI-flyten aldri bruker Python automatisk.

Leveranse:

- [x] `bin/nc`, `bin/nor` og `bin/nl` bruker bare ferdig binary i normalflyten
- [x] `main.py` er kun en eksplisitt bootstrap-/utviklervariant
- [x] en egen kommando eller tydelig flaggsti for bootstrap hvis den trengs
- [x] README og help-tekst beskriver den nye regelen
- [x] CI sjekker at wrapperne ikke faller tilbake til Python i det skjulte

Ferdig når:

- [x] ingen standard kommandoer kjører Python uten at brukeren har bedt om det
- [x] bootstrap er tydelig, eksplisitt og dokumentert
- [x] nye brukere møter binary-first som eneste normale vei

## Etappe 2: Selvstendig installasjon og oppgradering

Hvorfor:
Et produkt er ikke helt ferdig før det kan installeres, oppgraderes og rulles tilbake uten spesialkunnskap.

Leveranse:

- [x] releasepakker inneholder ferdig binary og nødvendige wrapper-filer
- [x] installasjonsflyt for lokal maskin er enkel og deterministisk
- [x] checksums eller signaturer for release-artifacts
- [x] tydelig versjons- og oppgraderingsflyt
- [x] rollback-dokumentasjon for feilfrigivelser

Ferdig når:

- [x] en ny bruker kan installere og oppgradere uten å lese intern utviklerdokumentasjon
- [x] release-artifactene kan verifiseres mekanisk
- [x] oppgradering og nedgradering er forutsigbar

## Etappe 3: Lås produktkontrakten

Hvorfor:
Når kjernen er stabil, må grensesnittet bli rolig, forutsigbart og lett å støtte over tid.

Leveranse:

- [x] stabil CLI-kontrakt for de viktigste kommandoene
- [x] plan for deprecering av legacy-navn og gamle aliaser
- [x] stabile exit-koder og lesbare feilmeldinger
- [x] dokumentert migreringshistorikk for eventuelle brudd
- [x] kommandooversikt generert eller kontrollert fra samme kilde som implementasjonen

Ferdig når:

- [x] vanlige brukere ikke blir overrasket av navnebytter eller skjulte fallback-regler
- [x] dokumentasjonen matcher faktisk oppførsel
- [x] det finnes en klar policy for hva som er støtteflaten og hva som er legacy

## Etappe 4: Driftklar kvalitet

Hvorfor:
Helt ferdig betyr også at prosjektet tåler vekst, regresjoner og større bruksmengder.

Leveranse:

- [x] benchmark-suite med konkrete ytelsesmål
- [x] negative tester og fuzz-lignende dekning for parser og runtime
- [x] CI på tvers av støttede plattformer og byggemoduser
- [x] smoke tests for fresh install og fresh release
- [x] tydelige terskler for hva som regnes som ytelses- eller stabilitetsregresjon

Ferdig når:

- [x] regresjoner stoppes før de når brukerne
- [x] ytelse måles mot faste mål, ikke bare magefølelse
- [x] det finnes en reell release-sjekkliste som kan kjøres hver gang

## Etappe 5: Full produktmodenhet

Hvorfor:
Det siste steget er ikke språkfunksjoner, men overleverbarhet, dokumentasjon og friksjonsfri bruk.

Leveranse:

- [x] en komplett "kom i gang"-flyt for nye brukere
- [x] cookbook for CLI, API, JSON, filer og skript
- [x] representative eksempelapper som viser de viktigste bruksmønstrene
- [x] tydelig vedlikeholds- og deprecationspolicy
- [x] ferdig definert release-kadens og støttevinduer

Ferdig når:

- [x] en ny utvikler kan bruke Norscode uten å spørre om interne detaljer
- [x] prosjektet kan vedlikeholdes over tid uten at bootstrap er en skjult svakhet
- [x] det som gjenstår er normalt produktvedlikehold, ikke store grunnmursprosjekter

## Anbefalt rekkefølge

- [x] Fjern implisitt Python-fallback
- [x] Gjør installasjon og oppgradering selvstendig
- [x] Lås produktkontrakten
- [x] Bygg driftklar kvalitet
- [x] Fullfør produktmodenhet

## Kortversjon

Norscode er ferdig når:

- [x] binary-first er den eneste normale veien
- [x] Python bare brukes eksplisitt som bootstrap
- [x] release og installasjon er selvforklarende
- [x] kvalitet og dokumentasjon holder nivået til et stabilt 1.0-produkt
