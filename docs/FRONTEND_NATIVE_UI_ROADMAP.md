# Frontend Native UI Roadmap

Dette dokumentet er en avkrysningsliste for å gjøre Native UI til en ferdig, selvstendig del av Norscode.
Målet er at UI skal være bygget direkte i Norscode, med Python kun som bootstrap og verktøy der det trengs.

## Målbildet

- [x] Native UI er en kanonisk del av språk- og frontend-kontrakten.
- [x] `std/nativeui.no` er den primære veien for rendering og struktur.
- [x] Python er ikke lenger selve UI-motoren.
- [x] Dokumentasjon, tester og eksempler viser samme modell som koden.

## Omgang 1: Stabiliser grunnmuren

- [x] rydde opp i `std/nativeui.no`
- [x] gjøre blokk- og helpernavn konsekvente
- [x] sikre at layoutoppførsel er forutsigbar
- [x] samle små avvik før vi utvider videre
- [x] dele store deler av `std/nativeui.no` i tydeligere seksjoner
- [x] standardisere navngiving for helpers og blokk-renderere
- [x] rette små layoutavvik i `hero`, `tabs`, `accordion`, `panel`, `card` og `footer`
- [x] legge inn negative tester for feil innrykk, ukjent blokk og ugyldig barnestruktur
- [x] gjøre output mer stabil og enklere å snapshot-teste

Ferdig når:

- [x] samme input gir samme HTML-output
- [x] feil i blokkstruktur blir fanget tydelig
- [x] `std/nativeui.no` er lesbar nok til å utvides trygt

## Omgang 2: Fullfør statisk UI-bibliotek

- [x] dekke vanlige layout- og feedbackmønstre
- [x] gjøre Native UI nyttig for hele sider, ikke bare demoer
- [x] legge til blokker som `grid`, `kolonner`, `sidebar`
- [x] finjustere eksisterende feedback-blokker som `alert`, `toast`, `dialog` og `avatar`
- [x] finjustere skjema-mønstre og tilgjengelighet for forms
- [x] standardisere hvordan nye blokker får HTML, test og dokumentasjon
- [x] teste at vanlige web-UI-er kan bygges uten rå HTML
- [x] finpusse `switch`, `file`, `search`, `submit` og `reset` der det trengs
- [x] standardisere visuell styling for `toast`, `dialog`, `avatar` og `chip`

Ferdig når:

- [x] de fleste typiske web-sider kan skrives i Native UI uten å falle tilbake til manuelt HTML-arbeid

## Omgang 3: Komponent- og slot-modell

- [x] definere en tydelig komponentmodell for Norscode UI
- [x] standardisere slot- eller barn-komposisjon
- [x] gjøre det lett å skrive egne komponenter som brukes på flere sider
- [x] samle `partial_*`-mønstre i en mer eksplisitt standard
- [x] dokumentere hva som er lavnivå renderer og hva som er anbefalt komponentmønster
- [x] legge til en anbefalt komponentflate i `std/frontend.no`
- [x] gjøre layout og sammensatte komponenter lett gjenbrukbare
- [x] vise modellen i et konkret eksempelprosjekt
- [x] lage testdekning for komponent- og slot-flaten

Ferdig når:

- [x] en side kan bygges av små, gjenbrukbare UI-biter uten unødvendig duplisering

## Omgang 4: Reaktivitet

- [x] innføre en enkel state-modell
- [x] definere event-håndtering og bindings
- [x] legge til betinget rendering
- [x] bygge en liten delt state-/store-flate
- [x] skille tydelig mellom server-rendering og klientoppdatering
- [x] avklare hva som er reaktive widgets og hva som er ren statisk rendering

Ferdig når:

- [ ] UI kan oppdatere seg uten full reload der det faktisk gir verdi

## Omgang 5: Inn i egen pipeline

- [x] flytte Native UI-parsing nærmere eller inn i den ordinære compiler-pipelinen
- [x] gjøre `ui-render` til en ren Norscode-kommando
- [x] fjerne gjenværende Python-logikk fra selve rendringen
- [x] sikre at samme input gir samme output gjennom hele kjeden
- [x] la Python være oppstart og verktøy, ikke UI-implementasjon

Ferdig når:

- [x] Native UI rendres uten at Python er en del av motorveien

## Omgang 6: Dokumentasjon og eksempler

- [x] oppdatere de viktigste frontend-dokumentene så de matcher koden
- [x] ha et eksempel som viser hele Native UI-flaten
- [x] ha render-tester for viktige UI-eksempler
- [x] lage 1-2 gull-eksempler som viser hele sider i mer lesbar form
- [x] rydde bort gamle eller overlappende beskrivelser
- [x] gjøre det lett for nye bidragsytere å finne riktig mønster

Ferdig når:

- [x] dokumentasjonen forteller samme historie som implementasjonen

## Omgang 7: Polering og kvalitet

- [x] standardisere CSS-klassene som Native UI genererer
- [x] forbedre feilmeldinger og diagnoser
- [x] utvide visuell og strukturell testdekning
- [x] sikre at output holder seg stabil over tid
- [x] gjøre vedlikehold og videreutvikling enklere

Ferdig når:

- [x] UI-laget føles som en naturlig del av Norscode, ikke som et sideprosjekt

## Anbefalt rekkefølge

1. Stabiliser grunnmuren
2. Fullfør statisk UI-bibliotek
3. Komponent- og slot-modell
4. Reaktivitet
5. Inn i egen pipeline
6. Dokumentasjon og eksempler
7. Polering og kvalitet

## Kortversjon

Native UI er ferdig når:

- [x] du kan bygge hele sider i Norscode
- [x] komponenter kan gjenbrukes ryddig
- [x] reaktivitet er et eget lag, ikke en hack
- [x] Python ikke lenger er nødvendig for selve UI-motoren
- [x] dokumentasjon, tester og implementasjon er i sync
