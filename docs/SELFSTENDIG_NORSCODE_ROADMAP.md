# Selvstendig Norscode Roadmap

Mål:
Gjøre Norscode til en selvbærende språkplattform der Norscode kan bygge, teste og videreutvikle Norscode uten at Python er hovedmotoren.

Planen under er delt i omganger for å gjøre fremdriften lett å følge.

Python skal på sikt bare være:

- bootstrap-verktøy
- sammenligningsmotor
- nød-/debug-fallback
- utviklerhjelp i overgangsfasen

## Kort status

- [x] Målet om selvstendig Norscode er formulert
- [x] Python er allerede tydelig definert som overgangs- og fallback-verktøy
- [x] Det finnes et etablert selfhost-spor i repoet
- [x] Frontend/HTML er allerede flyttet ut av bootstrap som eget ferdig spor
- [x] Omgang 1 — Fastslå nåværende avhengigheter er fullført
- [x] Omgang 2 — Selfhost Lexer er fullført
- [x] Omgang 3 — Minimal Parser er fullført
- [ ] Resten av bootstrap-flaten er slank nok til at den bare er oppstart og verktøy
- [x] Selfhost-kompilatoren er primærbane for støttet kjerne
- [x] Python brukes bare eksplisitt som fallback for støttede gap

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

## Omgangsoversikt

- [x] Omgang 1 — Fastslå nåværende avhengigheter
- [x] Omgang 2 — Selfhost Lexer
- [ ] Omgang 3 — Minimal Parser
- [x] Omgang 4 — AST og Semantic Core
- [x] Omgang 5 — IR/Bytecode Backend i Norscode
- [x] Omgang 6 — Bootstrap Chain
- [x] Omgang 7 — Standardbibliotek for compilerarbeid
- [x] Omgang 8 — Binær og distribusjon
- [x] Omgang 9 — Python nedgraderes
- [ ] Omgang 10 — Full språkparitet

## Omgang 1 — Fastslå nåværende avhengigheter

Mål:
Få oversikt over hva som fortsatt eies av Python.

Leveranse:

- [`docs/SELFHOST_DEPENDENCY_MAP.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_DEPENDENCY_MAP.md)

Oppgaver:

- [x] Liste alle Python-filer i `compiler/`
- [x] Dele Python-motoren i moduler: lexer, parser, semantic, runtime, codegen, CLI, test
- [x] Markere hvilke deler som allerede har selfhost-ekvivalent
- [x] Markere hvilke deler som bare finnes i Python
- [x] Lage avhengighetskart for `main.py`
- [x] Lage minimumskjerne for første selfhost-compiler

Exit-kriterier:

- [x] Det finnes en dokumentert oversikt over hva som må flyttes først
- [x] Det finnes en tydelig liste over hva selfhost-v1 skal støtte
- [x] Eksisterende selfhost-spor er dokumentert i repoet
- [x] Avhengighetskart for `main.py` er komplett
- [x] Minimumskjerne for første selfhost-compiler er låst

Status:

Omgang 1 er fullført.

## Omgang 2 — Selfhost Lexer

Mål:
Lage en lexer i Norscode som kan lese `.no`-kode og produsere tokens.

Første fil:

- `selfhost/lexer.no`

Status:

- [x] `selfhost/lexer.no` finnes
- [x] `selfhost/lexer/lexer_m1.no` finnes
- [x] Lexer-runbook finnes i [`docs/SELFHOST_LEXER_PHASE1_RUNBOOK.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_LEXER_PHASE1_RUNBOOK.md)
- [x] Token-formatet er dokumentert i [`docs/TOKEN_FORMAT_V1.md`](/Users/jansteinar/Projects/Norscode/docs/TOKEN_FORMAT_V1.md)
- [x] Omgang 2-parity er grønn i praksis
- [x] Selfhost lexer kan tokenisere representative `.no`-filer stabilt
- [x] Python- og selfhost-tokenstrøm matcher på samme input

Byggeplan:

- [x] etablere lexer-skjelett og token-typer
- [x] støtte identifikatorer og norske nøkkelord i første versjon
- [x] støtte heltall, tekststrenger, operatorer og punctuation
- [x] støtte whitespace og kommentarer
- [x] legge inn presise posisjonsfeil for alle lexer-feil
- [x] stabilisere EOF- og linje/kolonne-kontrakt
- [x] koble lexer til parity-fixtures mot Python
- [x] gjøre `selfhost-lexer-suite` til grønn gate i CI
- [x] dokumentere bruk av lexer i resten av selfhost-pipelinen

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

- [x] Definere token-format som map/struct
- [x] Lage `tokeniser(kilde: tekst)`
- [x] Lage hjelpefunksjoner for bokstav, tall og whitespace
- [x] Lage feilmeldinger med posisjon
- [x] Lage tester mot enkle `.no`-snutter
- [x] Sammenligne tokens mot Python-lexer for samme input

Exit-kriterier:

- Selfhost-lexer kan tokenisere enkle funksjoner
- Token-output er stabil og testbar
- Lexer kan brukes av neste parserfase

Status:

Omgang 2 er fullført.

## Omgang 3 — Minimal Parser

Mål:
Lage parser i Norscode for en liten, men nyttig språk-kjerne.

Første fil:

- `selfhost/parser.no`

Runbook:

- [`docs/SELFHOST_PARSER_PHASE1_RUNBOOK.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_PARSER_PHASE1_RUNBOOK.md)

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

- [x] Definere AST-format
- [x] Lage parser-state
- [x] Lage `parse_program(tokens)`
- [x] Lage uttrykksparser med operatorpresedens
- [x] Lage parserfeil med linje/kolonne
- [x] Lage snapshot-tester

Exit-kriterier:

- [x] Parser kan lese `selfhost/compiler.no`-lignende kode i redusert form
- [x] AST kan serialiseres til tekst/JSON-lignende output for sammenligning

Status:

Omgang 3 er fullført.

## Omgang 4 — AST og Semantic Core

Mål:
Gi selfhost-kompilatoren nok forståelse til å avvise åpenbare feil.

Filer:

- `selfhost/ast.no`
- `selfhost/semantic.no`

Oppgaver:

- [x] Representere program, funksjon, blokk og uttrykk
- [x] Bygge symboltabell
- [x] Sjekke dupliserte navn
- [x] Sjekke ukjente variabler
- [x] Sjekke enkel returflyt
- [x] Sjekke grunnleggende typer: tekst, heltall, bool, lister
- [x] Integrere med eksisterende feilmodell

Exit-kriterier:

- [x] Enkle programmer får deterministiske semantic-feil
- [x] Gyldige programmer går videre til IR-output

Status:

Omgang 4 er fullført.

## Omgang 5 — IR/Bytecode Backend i Norscode

Mål:
La selfhost-kompilatoren produsere kjørbar mellomkode.

Filer:

- `selfhost/ir.no`
- `selfhost/bytecode_backend.no`
- [`docs/SELFHOST_BYTECODE_PHASE1_RUNBOOK.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_BYTECODE_PHASE1_RUNBOOK.md)

Oppgaver:

- [x] Definere IR-instruksjoner i Norscode
- [x] Senke uttrykk til stack-ops
- [x] Senke variabler, if, while og kall
- [x] Gjenbruke eksisterende opcode-regler fra `selfhost.common`
- [x] Lage disassembler-output for sammenligning
- [x] Lage parity-tester mot Python backend

Exit-kriterier:

- [x] Samme enkle program gir samme IR/disasm i Python og Norscode
- [x] Selfhost-kompilatoren kan produsere output uten Python-parser

Status:

Omgang 5 er fullført.

## Omgang 6 — Bootstrap Chain

Mål:
Bygge en kontrollert kjede der Norscode bygger Norscode.

Kjede:

1. Python-motor bygger selfhost compiler
2. Selfhost compiler kompilerer testprogrammer
3. Selfhost compiler kompilerer ny selfhost compiler
4. Output sammenlignes
5. Avvik rapporteres tydelig

Filer:

- `selfhost/bootstrap.no`
- `selfhost/bootstrap.hash`
- [`docs/SELFHOST_BOOTSTRAP_PHASE1_RUNBOOK.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_BOOTSTRAP_PHASE1_RUNBOOK.md)

Oppgaver:

- [x] Lage `selfhost/bootstrap.no`
- [x] Lage `nc selfhost-build`
- [x] Lage `nc selfhost-check`
- [x] Lage output-hash for compiler-resultat
- [x] Lage golden snapshots
- [x] Integrere i CI

Exit-kriterier:

- [x] Bootstrap-kjeden kjører deterministisk
- [x] Endringer i compiler gir synlig diff
- [x] Python er bare første trinn, ikke eneste compiler

Status:

Omgang 6 er fullført.

## Omgang 7 — Standardbibliotek for compilerarbeid

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

- [x] Lage compiler-brukstester for hver stdlib-modul
- [x] Unngå skjulte Python-hjelpere i kritisk selfhost-kode
- [x] Dokumentere hva som fortsatt er native/runtime-avhengig

Exit-kriterier:

- [x] Selfhost-kompilatoren bruker primært Norscode stdlib
- [x] Manglende funksjoner er synlige som åpne oppgaver

Filer:

- [`docs/SELFHOST_STDLIB_PHASE1_RUNBOOK.md`](/Users/jansteinar/Projects/Norscode/docs/SELFHOST_STDLIB_PHASE1_RUNBOOK.md)
- [`selfhost/tests/stdlib_compiler_tests.no`](/Users/jansteinar/Projects/Norscode/selfhost/tests/stdlib_compiler_tests.no)
- [`std/cli.no`](/Users/jansteinar/Projects/Norscode/std/cli.no)
- [`selfhost/bootstrap_compiler/compiler_entrypoint.no`](/Users/jansteinar/Projects/Norscode/selfhost/bootstrap_compiler/compiler_entrypoint.no)

Status:

Omgang 7 er fullført.

## Omgang 8 — Binær og distribusjon

Mål:
Gjøre Norscode praktisk å installere uten Python-miljø.

Oppgaver:

- [x] Bygge `nc` som Linux-binær
- [x] Bygge `nc` som macOS-binær
- [x] Lage release-pakke med stdlib
- [x] Lage install-script
- [x] Lage `nc --version`
- [x] Lage `nc doctor`
- [x] Lage smoke-test for installert release

Exit-kriterier:

- [x] Bruker kan installere Norscode og kjøre `.no` uten å sette opp Python-prosjekt manuelt

Filer:

- [`package-release.sh`](/Users/jansteinar/Projects/Norscode/package-release.sh)
- [`tools/install-release.sh`](/Users/jansteinar/Projects/Norscode/tools/install-release.sh)
- [`norcode/commands/doctor.py`](/Users/jansteinar/Projects/Norscode/norcode/commands/doctor.py)
- [`tests/test_distribution_commands.py`](/Users/jansteinar/Projects/Norscode/tests/test_distribution_commands.py)
- [`docs/PHASE7_BINARY_DISTRIBUTION.md`](/Users/jansteinar/Projects/Norscode/docs/PHASE7_BINARY_DISTRIBUTION.md)

Status:

Omgang 8 er fullført.

## Omgang 9 — Python nedgraderes

Mål:
Flytte Python fra hovedmotor til fallback.

Oppgaver:

- [x] Dokumentere offisiell selfhost-kompileringsflyt
- [x] Gjøre selfhost som standard for støttet kjerne
- [x] Beholde Python som `--python-fallback`
- [x] Varsle når Python-fallback brukes
- [x] Måle selfhost-parity i CI

Exit-kriterier:

- [x] Ny kode kan bygges med selfhost som primærbane
- [x] Python er ikke nødvendig i vanlig brukerflyt for støttede features

Gjennomførte punkter:

- [x] Python-fallback-prinsippet er dokumentert
- [x] `nc run` er selfhost-first for støttet kjerne
- [x] `--python-fallback` er eneste vei til Python-bane
- [x] CI håndhever fallback-regelen

Status:

Omgang 9 er fullført.

## Omgang 10 — Full språkparitet

Mål:
Alle viktige språkfeatures støttes i selfhost.

Oppgaver:

- [x] Structs (M1 parity)
- [x] Maps (M1 parity)
- [x] JSON-flyt (M1 parity)
- [x] Try/catch (M1 parity)
- [x] Async/await
- [x] Generics (M1 parity)
- [x] Lambdaer (M1 parity)
- [x] Pattern matching
- [x] Web/server-kjerne
- [x] Test runner

Exit-kriterier:

- [x] Selfhost kan bygge store deler av repoet
- [x] Python og Norscode compiler gir likt resultat for definerte parity-suiter

Status:

Omgang 10 er fullført.

## Første konkrete sprint

Sprintnavn:
`SELFHOST-V1 Lexer`

Mål:
Bygge første ekte `selfhost/lexer.no`.

Oppgaver:

- [x] Lage token-format
- [x] Lage tokeniser-funksjon
- [x] Støtte norske nøkkelord
- [x] Støtte tekst, tall og operatorer
- [ ] Lage tester
- [ ] Koble til sammenligning mot Python-lexer

Ferdig når:

- `selfhost/lexer.no` finnes
- minst 5 små kodeeksempler kan tokeniseres
- lexerfeil viser posisjon
- output kan brukes videre av parser

Status:

- [x] Lexer-filer finnes allerede
- [ ] Minst 5 små kodeeksempler kan tokeniseres stabilt
- [ ] Lexerfeil viser posisjon stabilt
- [ ] Output kan brukes videre av parser uten workaround

## Viktig prinsipp

Ikke prøv å lage perfekt compiler først.

Lag en liten compiler som faktisk virker, bygg tester rundt den, og utvid steg for steg.

## Status

- [x] Eksisterende selfhost-spor finnes
- [x] Roadmap for selvstendig Norscode opprettet
- [ ] Omgang 2 — Selfhost Lexer er stabil
- [ ] Omgang 3 — Minimal Parser er stabil
- [x] Omgang 4 — AST og Semantic Core er stabil
- [x] Omgang 5 — IR/Bytecode Backend er stabil
- [x] Omgang 6 — Bootstrap Chain er stabil
- [x] Omgang 7 — Standardbibliotek for compilerarbeid er stabilt
- [x] Omgang 8 — Binær og distribusjon er stabil
- [x] Omgang 9 — Python nedgraderes
- [ ] Omgang 10 — Full språkparitet
