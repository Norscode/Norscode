# Selvstendig Norscode Roadmap

Mål:
Gjøre Norscode til en selvbærende språkplattform der Norscode kan bygge, teste og videreutvikle Norscode uten at Python er hovedmotoren.

Python skal på sikt bare være:

- bootstrap-verktøy
- sammenligningsmotor
- nød-/debug-fallback
- utviklerhjelp i overgangsfasen

## Definisjon av selvstendig

Norscode regnes som selvstendig når:

- `nc` kan bygges til en kjørbar binær
- Norscode-compiler skrevet i Norscode kan kompilere Norscode-kode
- Norscode-compiler kan bygge en ny versjon av seg selv
- selfhost-output kan sammenlignes mot eksisterende Python-motor
- testpakken kan kjøres uten Python som primær compiler
- standardbiblioteket har nok IO, tekst, lister, maps, feilmodell og CLI-støtte til å bære compilerarbeidet

## Hovedstrategi

Ikke skriv om alt på én gang.

Bygg én smal, verifiserbar selfhost-kjede:

1. Lexer i Norscode
2. Parser for liten kjerne i Norscode
3. AST-representasjon i Norscode
4. Semantic-sjekk for kjernen
5. IR/bytecode-output
6. Sammenligning mot Python-motor
7. Utvidelse av språkflaten
8. Full compiler-parity
9. Binær distribusjon

## Fase 0 — Fastslå nåværende avhengigheter

Mål:
Få oversikt over hva som fortsatt eies av Python.

Oppgaver:

- [ ] Liste alle Python-filer i `compiler/`
- [ ] Dele Python-motoren i moduler: lexer, parser, semantic, runtime, codegen, CLI, test
- [ ] Markere hvilke deler som allerede har selfhost-ekvivalent
- [ ] Markere hvilke deler som bare finnes i Python
- [ ] Lage avhengighetskart for `main.py`
- [ ] Lage minimumskjerne for første selfhost-compiler

Exit-kriterier:

- Det finnes en dokumentert oversikt over hva som må flyttes først
- Det finnes en tydelig liste over hva selfhost-v1 skal støtte

## Fase 1 — Selfhost Lexer

Mål:
Lage en lexer i Norscode som kan lese `.no`-kode og produsere tokens.

Første fil:

- `selfhost/lexer.no`

Må støtte først:

- identifikatorer
- norske nøkkelord
- heltall
- tekststrenger
- operatorer
- parenteser
- klammer
- komma
- kolon
- linjeskift/whitespace
- kommentarer

Oppgaver:

- [ ] Definere token-format som map/struct
- [ ] Lage `tokeniser(kilde: tekst)`
- [ ] Lage hjelpefunksjoner for bokstav, tall og whitespace
- [ ] Lage feilmeldinger med posisjon
- [ ] Lage tester mot enkle `.no`-snutter
- [ ] Sammenligne tokens mot Python-lexer for samme input

Exit-kriterier:

- Selfhost-lexer kan tokenisere enkle funksjoner
- Token-output er stabil og testbar
- Lexer kan brukes av neste parserfase

## Fase 2 — Minimal Parser

Mål:
Lage parser i Norscode for en liten, men nyttig språk-kjerne.

Første fil:

- `selfhost/parser.no`

Støtt først:

- funksjonserklæring
- parameterliste
- returtype
- blokk
- `la` variabel
- `returner`
- `hvis` / `ellers`
- `mens`
- funksjonskall
- binære uttrykk
- tekst, heltall, bool
- lister og maps når lexer og AST tåler det

Oppgaver:

- [ ] Definere AST-format
- [ ] Lage parser-state
- [ ] Lage `parse_program(tokens)`
- [ ] Lage uttrykksparser med operatorpresedens
- [ ] Lage parserfeil med linje/kolonne
- [ ] Lage snapshot-tester

Exit-kriterier:

- Parser kan lese `selfhost/compiler.no`-lignende kode i redusert form
- AST kan serialiseres til tekst/JSON-lignende output for sammenligning

## Fase 3 — AST og Semantic Core

Mål:
Gi selfhost-kompilatoren nok forståelse til å avvise åpenbare feil.

Filer:

- `selfhost/ast.no`
- `selfhost/semantic.no`

Oppgaver:

- [ ] Representere program, funksjon, blokk og uttrykk
- [ ] Bygge symboltabell
- [ ] Sjekke dupliserte navn
- [ ] Sjekke ukjente variabler
- [ ] Sjekke enkel returflyt
- [ ] Sjekke grunnleggende typer: tekst, heltall, bool, lister
- [ ] Integrere med eksisterende feilmodell

Exit-kriterier:

- Enkle programmer får deterministiske semantic-feil
- Gyldige programmer går videre til IR-output

## Fase 4 — IR/Bytecode Backend i Norscode

Mål:
La selfhost-kompilatoren produsere kjørbar mellomkode.

Filer:

- `selfhost/ir.no`
- `selfhost/bytecode_backend.no`

Oppgaver:

- [ ] Definere IR-instruksjoner i Norscode
- [ ] Senke uttrykk til stack-ops
- [ ] Senke variabler, if, while og kall
- [ ] Gjenbruke eksisterende opcode-regler fra `selfhost.common`
- [ ] Lage disassembler-output for sammenligning
- [ ] Lage parity-tester mot Python backend

Exit-kriterier:

- Samme enkle program gir samme IR/disasm i Python og Norscode
- Selfhost-kompilatoren kan produsere output uten Python-parser

## Fase 5 — Bootstrap Chain

Mål:
Bygge en kontrollert kjede der Norscode bygger Norscode.

Kjede:

1. Python-motor bygger selfhost compiler
2. Selfhost compiler kompilerer testprogrammer
3. Selfhost compiler kompilerer ny selfhost compiler
4. Output sammenlignes
5. Avvik rapporteres tydelig

Oppgaver:

- [ ] Lage `selfhost/bootstrap.no`
- [ ] Lage `nc selfhost-build`
- [ ] Lage `nc selfhost-check`
- [ ] Lage output-hash for compiler-resultat
- [ ] Lage golden snapshots
- [ ] Integrere i CI

Exit-kriterier:

- Bootstrap-kjeden kjører deterministisk
- Endringer i compiler gir synlig diff
- Python er bare første trinn, ikke eneste compiler

## Fase 6 — Standardbibliotek for compilerarbeid

Mål:
Sikre at Norscode har nok stdlib til å bære compiler og verktøy.

Må være stabilt:

- `std.tekst`
- `std.liste`
- `std.ordbok`
- `std.fil`
- `std.path`
- `std.env`
- `std.json`
- `std.cli`
- feilmodell med `kast`, `prøv`, `fang`

Oppgaver:

- [ ] Lage compiler-brukstester for hver stdlib-modul
- [ ] Unngå skjulte Python-hjelpere i kritisk selfhost-kode
- [ ] Dokumentere hva som fortsatt er native/runtime-avhengig

Exit-kriterier:

- Selfhost-kompilatoren bruker primært Norscode stdlib
- Manglende funksjoner er synlige som åpne oppgaver

## Fase 7 — Binær og distribusjon

Mål:
Gjøre Norscode praktisk å installere uten Python-miljø.

Oppgaver:

- [ ] Bygge `nc` som Linux-binær
- [ ] Bygge `nc` som macOS-binær
- [ ] Lage release-pakke med stdlib
- [ ] Lage install-script
- [ ] Lage `nc --version`
- [ ] Lage `nc doctor`
- [ ] Lage smoke-test for installert release

Exit-kriterier:

- Bruker kan installere Norscode og kjøre `.no` uten å sette opp Python-prosjekt manuelt

## Fase 8 — Python nedgraderes

Mål:
Flytte Python fra hovedmotor til fallback.

Oppgaver:

- [ ] Dokumentere offisiell selfhost-kompileringsflyt
- [ ] Gjøre selfhost som standard for støttet kjerne
- [ ] Beholde Python som `--python-fallback`
- [ ] Varsle når Python-fallback brukes
- [ ] Måle selfhost-parity i CI

Exit-kriterier:

- Ny kode kan bygges med selfhost som primærbane
- Python er ikke nødvendig i vanlig brukerflyt for støttede features

## Fase 9 — Full språkparitet

Mål:
Alle viktige språkfeatures støttes i selfhost.

Oppgaver:

- [ ] Structs
- [ ] Maps
- [ ] JSON-flyt
- [ ] Try/catch
- [ ] Async/await
- [ ] Generics
- [ ] Lambdaer
- [ ] Pattern matching
- [ ] Web/server-kjerne
- [ ] Test runner

Exit-kriterier:

- Selfhost kan bygge store deler av repoet
- Python og Norscode compiler gir likt resultat for definerte parity-suiter

## Første konkrete sprint

Sprintnavn:
`SELFHOST-V1 Lexer`

Mål:
Bygge første ekte `selfhost/lexer.no`.

Oppgaver:

- [ ] Lage token-format
- [ ] Lage tokeniser-funksjon
- [ ] Støtte norske nøkkelord
- [ ] Støtte tekst, tall og operatorer
- [ ] Lage tester
- [ ] Koble til sammenligning mot Python-lexer

Ferdig når:

- `selfhost/lexer.no` finnes
- minst 5 små kodeeksempler kan tokeniseres
- lexerfeil viser posisjon
- output kan brukes videre av parser

## Viktig prinsipp

Ikke prøv å lage perfekt compiler først.

Lag en liten compiler som faktisk virker, bygg tester rundt den, og utvid steg for steg.

## Status

- [x] Eksisterende selfhost-spor finnes
- [x] Roadmap for selvstendig Norscode opprettet
- [ ] Selfhost lexer startet
- [ ] Selfhost parser startet
- [ ] Selfhost backend startet
- [ ] Bootstrap-kjede komplett
- [ ] Python nedgradert til fallback
