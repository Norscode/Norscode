# ROADMAP PYTHON

Mål:
Gjøre Norscode til et språk som kan ta en stor del av de praktiske jobbene Python brukes til i dag, uten å miste enkelheten i Norscode.

Roadmapen handler ikke om å kopiere hele Python. Den peker på språk, runtime, standardbibliotek og utvikleropplevelse som gjør Norscode til et reelt alternativ i app-, web-, script- og backend-arbeid.

## Arbeidsform

- Bruk checkboxene som statusfelt.
- Knytt leveranser til konkrete `REAL_COMPILER_V*`-milepæler.
- Ikke start en ny stor språkfunksjon før parser, semantic, runtime, tester og selfhost-plan er definert.
- Marker en milepæl som ferdig først når akseptansekriteriene faktisk er verifisert.

## Milepæler

### `V44` Ordbok Grunnmur

- [x] parserstøtte for map-literals
- [x] AST-noder for map
- [x] semantic-regler for nøkkel/verdi
- [x] runtime-støtte for oppslag og mutasjon
- [x] første `std.ordbok`
- [x] tester i compiler, runtime og selfhost-suite

Ferdig når:
Vanlige key-value-strukturer kan brukes i Norscode-programmer uten fallback-mønstre.

### `V45` Structs Og Feltaksess

- [x] syntaks for structs eller records
- [x] feltoppslag (`obj.felt`) via map-objekt
- [x] konstruksjon med navngitte felt
- [x] nesting
- [x] feilmeldinger for manglende/ukjente felt
- [x] parity-plan for selfhost

Ferdig når:
Vanlige domenemodeller kan uttrykkes tydelig uten å falle tilbake til løse lister og tekstnøkler overalt.

### `V46` JSON Førsteklasse

- [x] `std.json.parse`
- [x] `std.json.stringify`
- [x] kobling mellom JSON, map, liste og struct
- [x] gode parse-feil
- [x] oppdatere webeksempler til å bruke ekte JSON-flyt

Ferdig når:
API-kode og payloadhåndtering kan skrives uten skjør manuell strengbygging.

### `V47` Feilmodell

- [x] `kast`
- [x] `prøv`
- [x] `fang`
- [x] rethrow i nested `fang` (inkludert testet caser)
- [x] standard basisfeiltyper
- [x] stackspor eller tilsvarende kontekst
- [x] teststøtte for forventede feil

Ferdig når:
Feil kan modelleres eksplisitt og testes på en måte som ligner normal Python-flyt.

### `V48` Fil, Path Og Env

- [x] `std.fil`
- [x] `std.path`
- [x] `std.env`
- [x] IO-feil koblet til feilmodellen
- [x] nok API til enkle CLI-verktøy

Ferdig når:
Enkle scripts og verktøy kan leses, skrives og konfigureres direkte i Norscode.

### `V49` Strenger Og Lister

- [x] rikere tekstfunksjoner
- [x] rikere listefunksjoner
- [x] vurdere slicing
- [x] sortering, filtrering og søk
- [x] dokumentasjon og eksempler

Ferdig når:
Vanlig transformasjonskode føles kort og naturlig, ikke som unødvendig boilerplate.

### `V50` Async Grunnmur

- [x] designvalg for async-modell
- [x] `async`-funksjoner
- [x] `await`
- [x] task/future-basert runtime
- [x] timeout/cancellation-basics

Ferdig når:
IO-tung kode kan skrives som samtidige flyter uten hjemmelagde kontrollmønstre.

### `V51` HTTP-Klient Og Integrasjoner

- [x] `std.http`
- [x] GET/POST
- [x] headers og query
- [x] JSON-hjelpere
- [x] timeout og nettverksfeil

Ferdig når:
Eksterne API-kall er en normal språkbrukssituasjon, ikke et spesialtilfelle.

### `V52` REPL Og Utviklerflyt

- [x] REPL
- [x] bedre test discovery
- [x] forbedret feilrapportering
- [x] formatter-plan
- [x] linter-plan

Ferdig når:
Den første utvikleropplevelsen i Norscode føles rask og utforskbar, ikke tung.

## Prinsipper

- Prioriter praktisk nytte før språk-ornamenter.
- Bygg først funksjoner som låser opp ekte apper og verktøy.
- Hold parity mellom hovedmotor og selfhost-banen når nye språkformer innføres.
- Gi hver milepæl tydelige akseptansekriterier.
- Unngå å bygge et stort, halvt ferdig språk; bygg små, komplette trinn.

## Målbilde

Når denne roadmapen er kommet langt nok, skal Norscode kunne:

- skrive vanlige scripts og CLI-verktøy
- bygge web- og API-tjenester uten skjør tekstbygging
- håndtere JSON, filer, miljøvariabler og nettverk som førsteklasses oppgaver
- modellere data tydelig med strukturer i stedet for løse lister
- ha en feilmodell som er trygg nok for større apper
- gi utviklere en flyt som ligner mer på Python: rask testing, god feilsøking og lav friksjon

## Fase 1: Python For Praktiske Apper

Fokus:
Bygg den minste flaten som gjør Norscode sterkt til API-er, scripts og vanlig backend-logikk.

### 1. Ordbok / map / key-value-type

Hvorfor:
Python er sterk i praksis fordi dict er overalt. Uten dette blir JSON, config, headers, payloads og app-state unødvendig tunge.

Leveranse:

- språkstøtte for key-value-verdier
- oppslag på tekstnøkler
- mutasjon, sletting, iterasjon
- typekontroll for map-verdier
- `std.ordbok`

Akseptansekriterier:

- maps fungerer i parser, semantic, codegen og runtime
- maps kan brukes i tester, webkode og standardbibliotek
- selfhost-banen kan lese og senke samme konstruksjoner

Status:

- [x] design av literal-syntaks
- [x] AST-støtte
- [x] semantic-støtte
- [x] runtime-støtte
- [x] stdlib-støtte
- [x] selfhost-plan
- [x] dokumentasjon

### 2. Structs / records / navngitte datafelt

Hvorfor:
Python-kode blir lesbar når data kan samles i tydelige modeller. Norscode trenger dette for å unngå lange parameterlister og løse samlinger.

Leveranse:

- syntaks for enkle datatyper med navngitte felt
- konstruksjon og feltoppslag
- nesting av structs
- tydelige feilmeldinger ved manglende eller feil type på felt

Akseptansekriterier:

- structs kan brukes som parametere og returverdier
- feltfeil fanges i semantic-fasen når mulig
- runtime-feil er lesbare når felt mangler eller er ugyldige

Status:

- [x] syntaksvalg
- [x] AST-støtte
- [x] semantic-støtte
- [x] runtime-støtte
- [x] selfhost-plan
- [x] eksempler og docs

### 3. JSON som ekte datatype og verktøy

Hvorfor:
I praksis er JSON en av de viktigste broene mellom språk og verden rundt.

Leveranse:

- `std.json.parse`
- `std.json.stringify`
- trygg feltlesing
- bro mellom JSON, maps, lister, tekst, tall og bool

Akseptansekriterier:

- roundtrip fungerer for vanlige payloads
- ugyldig JSON gir god feil
- webeksempler kan bygge respons uten skjør manuell strengsammensetting

Status:

- [x] parse
- [x] stringify
- [x] kobling mot map/struct-literals
- [x] typed access helpers
- [x] integrasjon med listehjelpere
- [x] webdocs oppdatert

### 4. Feilmodell med `kast`, `prøv`, `fang`

Hvorfor:
Python er praktisk fordi feil kan modelleres og fanges kontrollert. Norscode trenger mer enn rå feilmeldinger.

Leveranse:

- språkstøtte for å kaste og fange feil
- standard basisfeiltyper
- stackspor eller tilsvarende feilkontekst
- bedre runtime-diagnostikk

Akseptansekriterier:

- feil kan propagere gjennom flere funksjonskall
- en fanget feil kan brukes videre i kode
- testene kan verifisere forventede feilbaner

Status:

- [x] språkdesign
- [x] runtime propagation
- [x] rethrow i nested catch blokker
- [x] basisfeiltyper
- [x] stackkontekst
- [x] teststøtte

### 5. Filsystem, path og miljøvariabler

Hvorfor:
Python brukes mye fordi det er enkelt å lese filer, skrive filer og styre miljø.

Leveranse:

- `std.fil`
- `std.path`
- `std.env`
- les, skriv, append, finnes, mkdir, join, basename, dirname

Akseptansekriterier:

- nok funksjonalitet til å skrive ekte CLI-verktøy i Norscode
- IO-feil kobles til den nye feilmodellen

Status:

- [x] `std.fil`
- [x] `std.path`
- [x] `std.env`
- [x] feilintegrasjon
- [x] CLI-eksempler

### 6. Rikere streng- og listebibliotek

Hvorfor:
Mye av Python-følelsen ligger i samlings- og tekstoperasjoner som bare finnes og virker.

Leveranse:

- `split`, `join`, `trim`, `replace`
- `starts_med`, `slutter_med`, `inneholder`
- slicing for tekst og lister, hvis det passer språkretningen
- flere listeoperasjoner som søk, filtrering, sortering og transformasjon

Akseptansekriterier:

- vanlige dataflyter i web- og scriptkode kan skrives uten unødvendig boilerplate
- standardbiblioteket dekker minst 80 prosent av enkle Python-lignende tekst- og listebehov

Status:

- [x] tekstfunksjoner
- [x] listefunksjoner
- [x] slicing-vurdering
- [x] docs og eksempler

## Fase 2: Python For Plattformarbeid

Fokus:
Gjør Norscode sterkt nok for integrasjoner, samtidighet og produksjonsnære tjenester.

### 7. `async` / `await`

Hvorfor:
Et moderne språk for API-er og integrasjoner trenger en tydelig samtidighetsmodell.

Leveranse:

- async-funksjoner
- await-uttrykk
- task- eller future-modell
- timeout og cancellation-mønstre

Akseptansekriterier:

- samtidige IO-oppgaver kan kjøres uten tung manuell styring
- feil i async-kjeder forblir lesbare

Status:

- [x] concurrency-design
- [x] parser/AST
- [x] `async`-funksjoner
- [x] `await`
- [x] runtime
- [x] timeout/cancel
- [x] testdekning

### 8. HTTP-klient

Hvorfor:
Python brukes konstant til å kalle eksterne API-er. Norscode må kunne gjøre det enkelt.

Leveranse:

- `std.http`
- GET, POST, headers, query, body
- tekst- og JSON-responser
- tidsavbrudd og feilhåndtering

Akseptansekriterier:

- en enkel integrasjon mot ekstern API kan skrives uten uoffisielle hjelpeveier

Status:

- [x] request API
- [x] response API
- [x] JSON helpers
- [x] nettverksfeil
- [x] integrasjonseksempel

### 9. Lagring og databasekontrakter

Hvorfor:
Mange ekte apper stanser hvis språket ikke har en tydelig historie for lagring.

Leveranse:

- enkel databaseadapter eller lagringskontrakt
- minimum én robust standardbane, for eksempel SQLite eller tilsvarende enkel lokal lagring
- klare grenser mellom domene, data og transport

Akseptansekriterier:

- små og mellomstore apper kan lagre og hente strukturerte data uten hjemmelagde tekstprotokoller

Status:

- [x] lagringsmodell valgt
- [x] første adapter implementert
- [x] feilmodell integrert
- [x] dokumentasjon

### 10. Mer moden webflate

Hvorfor:
Repoet viser allerede web/API-retning. Nå må den bli trygg og mindre tekstorientert.

Leveranse:

- sterkere request/response-hjelpere
- JSON-respons som standardbane
- typed input-validering
- tydeligere kontrakter for ruter og feilsvar

Akseptansekriterier:

- webkode blir kortere, tryggere og mer konsistent enn dagens manuelle strengbygging

Status:

- [x] request helpers
- [x] response helpers
- [x] JSON standardbane
- [x] typed validation
- [x] docs migrert

## Fase 3: Python For Utviklerflyt

Fokus:
Gi Norscode den lave friksjonen som gjør Python behagelig å jobbe i.

### 11. REPL

Hvorfor:
Rask utforsking, læring og debugging er en stor del av Python-opplevelsen.

Leveranse:

- interaktiv Norscode-shell
- multiline-støtte
- enkel import av moduler
- inspektiv output for grunnverdier og strukturer

Status:

- [x] kjørbar REPL
- [x] multiline
- [x] modulimport
- [x] visning av verdier

### 12. Formatter

Hvorfor:
Et språk blir raskt mer profesjonelt når stilvalg ikke må forhandles manuelt.

Leveranse:

- én offisiell formatter
- stabil output
- enkel CLI-integrasjon

Status:

- [x] stilregler valgt
- [x] formatter implementert
- [x] testet på eksempelkode

### 13. Linter og statiske kvalitetssjekker

Hvorfor:
Tidlig tilbakemelding holder kodebasen ryddig og senker review-friksjon.

Leveranse:

- enkle, nyttige regler først
- dødkode, skyggelegging, uklare navn, ubrukte imports, uoppnåelig kode

Status:

- [x] regelsett definert
- [x] første linter
- [x] CLI-integrasjon

### 14. Bedre testopplevelse

Hvorfor:
Python vinner mye på rask testflyt. Norscode bør ha samme følelse.

Leveranse:

- bedre testdiscovery
- rikere assertions
- tydelig feilrapportering
- maskinlesbar output
- grunnlag for coverage senere

Status:

- [x] discovery
- [x] assertions
- [x] rapportering
- [x] maskinlesbar output

## Fase 4: Python-Nær Språkmodenhet

Fokus:
Utvid språket når plattformgrunnmuren er sterk nok.

### 15. Generics

Hvorfor:
Når maps, lister og structs blir vanlige, trengs bedre typeuttrykk for å holde koden sterk.

Leveranse:

- generiske containere
- tydelige typefeil

Status:

- [x] design valgt
- [x] parser/semantic
- [x] runtime/codegen

### 16. Lambdaer og closures

Hvorfor:
Nyttig sammen med rikere samlings-API-er og transformasjonsflyt.

Leveranse:

- korte anonyme funksjoner
- lukking over ytre variabler

Status:

- [x] syntaks valgt
- [x] AST/semantic
- [x] runtime-støtte

### 17. Comprehensions eller tilsvarende kompakt datasyntaks

Hvorfor:
Dette er ikke nødvendig først, men gir høy uttrykkskraft når samlingsmodellen er moden.

Status:

- [x] design utredet
- [x] valgt syntaks
- [x] implementert

### 18. Pattern matching

Hvorfor:
Særlig nyttig når språkets datamodell blir rikere.

Status:

- [x] modell vurdert
- [x] syntaks valgt
- [x] implementert

## Fase 5: Python-Uavhengig Norscode

Fokus:
Flytt språket fra Python-støttet utviklingsmodus til en fullverdig selvbærende plattform.

### 19. Full selfhost-parity som gate

Hvorfor:
Nye språkformer må ikke bare finnes i Python-motoren.

Leveranse:

- alle store nye konstruksjoner støttes i selfhost-banen
- parity-suiter utvides sammen med språkflaten
- CI gate for nye Python-roadmap-funksjoner

Status:

- [x] gate definert
- [x] parity-suiter
- [x] CI-integrasjon

### 20. Python-fallback blir sekundær, ikke primær

Hvorfor:
Roadmapen er først virkelig fullført når Norscode kan bære sin egen hovedflyt.

Leveranse:

- mindre avhengighet av Python i kritisk utviklerbane
- tydelig definisjon av hva som fortsatt er bootstrap og hva som er produksjonsklart i Norscode

Status:

- [x] bootstrapgrense definert
- [x] primærflyt i Norscode
- [x] Python-fallback nedgradert

## Anbefalt Byggerekkefølge

1. map
2. structs
3. JSON
4. feilmodell
5. fil/path/env
6. streng- og listeforbedringer
7. async/await
8. HTTP-klient
9. lagring/database
10. REPL
11. formatter/linter/test-DX
12. generics/lambdaer/pattern matching
13. full selfhost-parity på hele flaten

## Første Konkrete Sprint

Anbefalt første sprint er `V44`.

Mål:
Få inn den første virkelige Python-lignende datamodellen i språket gjennom maps, med minst mulig spredning og med tydelig parity-plan.

### Sprintoppgaver For `V44`

- [x] definere map-literal-syntaks
- [x] legge til AST-node for map
- [x] utvide semantic-sjekk for map-verdier
- [x] legge til runtime-operasjoner for oppslag, innsetting og `har`
- [x] lage første `std.ordbok`
- [x] skrive compiler-tester
- [x] skrive runtime-tester
- [x] skrive selfhost-parsercases
- [x] skrive et lite web- eller JSON-eksempel som bruker maps

### Sprint Exit Criteria For `V44`

- [x] `norcode test` er grønn
- [x] minst ett brukseksempel i docs eller samples er på plass
- [x] parity-plan for selfhost er dokumentert
- [x] map-literal og map-mutasjon er nå støttet i selfhost-parity for relevante cases

## Ikke Prioriter Først

Disse tingene er interessante, men bør ikke stjele fokus tidlig:

- dekoratører
- tung metaprogrammering
- avansert reflection
- stor OOP-modell hvis structs dekker behovet
- språkpynt som ikke låser opp ekte apparbeid

## Definisjon Av Suksess

Roadmapen er vellykket når en utvikler kan velge Norscode i stedet for Python for:

- små og mellomstore backend-tjenester
- vanlige API-integrasjoner
- CLI-verktøy og scripts
- fil- og JSON-behandling
- validerings- og forretningslogikk

Roadmapen er svært vellykket når dette også skjer uten at Python-motoren er den bærende standardveien.

## Sluttstatus

Roadmapen er fullført slik den nå er skrevet:

- alle milepæler er avkrysset
- selfhost-parity og CI-gate er på plass
- Python er nedgradert til bootstrap-fallback i utviklerflyten
- Python brukes bare som utviklerverktøy der det fortsatt trengs

Videre arbeid bør nå styres av konkrete nye behov, ikke av å lukke gamle roadmap-punkter.
