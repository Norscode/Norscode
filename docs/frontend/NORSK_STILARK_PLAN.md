# Norsk stilarkplan

Dette dokumentet samlar arbeidet med eit norsk alternativ til CSS i Norscode.
Målet er ikkje berre å oversette ord, men å byggje ei meir strukturert og trygg
stilarkflate som kan brukast i vanleg utvikling.

## Retning

- skal kunne generere vanleg CSS
- skal bruke norske funksjonsnamn
- skal vere lett å teste i Norscode-kode
- skal kunne vekse frå enkel tekstbygging til meir strukturert modell
- skal ikkje bryte sjølvstendig utviklingsløype utan Python eller C

## Omgang 1: Grunnmur

Mål: få ein enkel, norsk stilarkbygjar som kan skrive vanleg CSS-tekst.

- [x] Lage modul for norsk stilarkgenerering i `std/stil.no`
- [x] Støtte grunnleggjande byggesteinar: `regel`, `eigenskap`, `variabel`, `media`
- [x] Støtte hjelparar for selectorar som `klasse`, `id`, `pseudo`, `barn`
- [x] Leggje på ei enkel testfil som låser forventa output
- [x] Dokumentere modulen i frontend-dokumentasjonen
- [x] Bruke modulen i minst eitt reelt frontend-eksempel

Klar når:

- stilark kan skrivast frå Norscode utan handskriven CSS-tekst
- output er vanleg CSS som nettlesaren forstår

## Omgang 2: Selector-modell

Mål: gjere selectorar til eksplisitte byggesteinar i staden for reine strengar.

- [x] Leggje til gruppeselktorar
- [x] Leggje til attributtselectorar
- [x] Leggje til pseudo-element
- [x] Leggje til støtte for kombinasjonar som `>`, `+` og `~`
- [x] Leggje til nesting mellom reglar

Klar når:

- ein kan byggje komplekse selectorar utan strengmanipulasjon
- testane fangar vanlege selector-mønster

## Omgang 3: At-reglar

Mål: støtte dei viktigaste CSS-reglane som startar med `@`.

- [x] Leggje til `@font-face`
- [x] Leggje til `@supports`
- [x] Leggje til `@container`
- [x] Leggje til `@keyframes`
- [x] Leggje til `@layer`
- [x] Leggje til import-støtte der det passar

Klar når:

- vanleg moderne CSS kan uttrykkast utan å falle tilbake til manuell tekst

## Omgang 4: Verdi-system

Mål: få betre kontroll på tal, einingar og verditypar.

- [x] Lage hjelparar for `px`, `rem`, `em`, `%`, `vw` og `vh`
- [x] Lage hjelparar for `calc`, `clamp`, `min` og `max`
- [x] Lage fargehjelparar for vanlege fargeformat
- [x] Lage struktur for gradientar, transformasjonar og filter
- [x] Gjere typefeil lettare å oppdage før output blir generert

Klar når:

- stilark kan byggjast med færre rå strengar og meir kontroll

## Omgang 5: Tokens og tema

Mål: gjere stil systematisk og gjenbrukbart.

- [x] Modellere farge-, spacing- og radius-tokens
- [x] Leggje til tema-variantar
- [x] Støtte lys og mørk modus som første-klasses konsept
- [x] Gjere komponentar i stand til å lese frå same token-kjelde
- [x] Dokumentere korleis tokens skal brukast i frontend

Klar når:

- fleire komponentar kan dele same visuelle språk utan å duplisere verdiar

## Omgang 6: Kvalitet og tryggleik

Mål: gjere språket robust nok til vanleg bruk i større prosjekt.

- [x] Leggje til validering av eigenskapar og verdier
- [x] Gi tydelege feilmeldingar med kontekst
- [x] Leggje til formattering eller normalisering av output
- [x] Leggje til fleire regresjonstestar
- [x] Måle om modulen er lett å bruke i reelle sider

Klar når:

- planlagde stilark kan byggjast, testast og vedlikehaldast utan ekstra friksjon

## Omgang 7: Avanserte CSS-kontraktar

Mål: fylle inn nokre av dei meir spesialiserte CSS-byggjeklossane som ofte dukkar opp i større prosjekt.

- [x] Leggje til `@property`
- [x] Leggje til `@scope`
- [x] Leggje til `url(...)` som verdi-hjelpar
- [x] Dokumentere korleis avanserte reglar bør brukast i frontend
- [ ] Vurdere fleire komponentdøme som nyttar dei nye hjelparane

Klar når:

- stilmodulen kan uttrykkje meir av moderne CSS utan å falle tilbake til rå strengar

## Arbeidsregel

Vi tek éi omgang om gongen.
Når ei omgang er ferdig, oppdaterer vi dokumentet med avkryssing, kort status og eventuelle nye funn før vi går vidare.
