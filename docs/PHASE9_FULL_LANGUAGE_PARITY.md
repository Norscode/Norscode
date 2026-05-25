# Fase 9 — Full språkparitet

Mål: selfhost-kompilatoren skal støtte de viktigste språkfeature-ene slik at Python-compiler og Norscode-compiler kan gi samme resultat for definerte parity-suiter.

## Prinsipp

Fase 9 skal ikke bygges som én stor omskriving. Hver feature skal innføres gjennom samme kjede:

```text
lexer
  -> parser
  -> AST snapshot
  -> semantic
  -> IR/backend
  -> parity-test
```

Ingen feature regnes som ferdig før den har snapshot/parity-test.

## Feature-matrise

| Feature | Lexer | Parser | AST | Semantic | IR/backend | Parity |
| --- | --- | --- | --- | --- | --- | --- |
| Structs | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |
| Maps | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |
| JSON-flyt | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |
| Try/catch | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |
| Async/await | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |
| Generics | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |
| Lambdaer | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |
| Pattern matching | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |
| Web/server-kjerne | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |
| Test runner | Delvis | Delvis | Delvis | Delvis | Delvis | Delvis |

## Prioritert rekkefølge

### 1. Structs

Structs bør komme først fordi compiler, AST og runtime trenger strukturerte data.

M1-status:

- brace-baserte struct-literals med ident-felt er nå støttet i selfhost
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en enkel `returner { navn: "Ada", by: "Oslo" }`-flyt

Må støtte:

```norscode
struktur Bruker
    navn: tekst
    alder: heltall
slutt
```

Semantic-regler:

- duplikate feltnavn skal feile
- ukjent struct-type skal feile
- feltaksess skal sjekkes
- struct-literal må matche felt

IR/backend:

- `BUILD_STRUCT`
- `GET_FIELD`
- `SET_FIELD`

### 2. Try/catch

Try/catch trengs for robust compilerfeil og runtimefeil.

Må støtte:

```norscode
prøv
    kast "feil"
fang feil
    skriv(feil)
slutt
```

IR/backend:

- `TRY_BEGIN`
- `TRY_END`
- `CATCH`
- `THROW`

### 3. Generics

Generics må komme etter structs/type-systemet.

Må støtte:

```norscode
funksjon første<T>(liste: liste<T>) -> T
```

Semantic-regler:

- typeparameter må være deklarert
- generic instansiering må være deterministisk
- feil typeargument skal feile

M1-status:

- generiske containere som `liste<heltall>`, `liste<tekst>` og `ordbok<tekst, heltall>` er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker en liten `summer(xs: liste<heltall>)`-flyt og container-bruk i `start`

### 4. Lambdaer

Må støtte:

```norscode
la dobbel = (x) => x * 2
```

Semantic:

- closure scope
- parameterinferens eller eksplisitt type
- kallbar type

M1-status:

- en enkel lambda som blir bundet i en variabel er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en liten `la x = fun(a) -> a + 1`-flyt i `start`

### 5. JSON-flyt

- En enkel `json_stringify({"navn": "Norscode", "alder": 2, "aktiv": sann})`-flyt er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en enkel JSON-serialisering av en map med tekst, tall og bool via builtin

### 5. Async/await

Må bygges etter funksjons- og runtime-kall er stabile.

Må støtte:

```norscode
async funksjon hent() -> tekst
la svar = await hent()
```

IR/backend:

- `ASYNC_FN`
- `AWAIT`
- promise/task runtime

M1-status:

- En enkel `async funksjon` og `await`-flyt er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en liten `hent()` + `await hent()`-flyt med async-retur

### 6. Pattern matching

Kan bygges etter structs og enums/variant-typer.

Må støtte:

```norscode
match verdi
    når 1 -> "en"
    ellers -> "annet"
slutt
```

M1-status:

- En enkel `match`-flyt med heltall, tekst og bool er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en liten `match`-flyt med eksplisitte case, wildcard og ellers

### 7. Web/server-kjerne

Målet er at Norscode kan bygge egne web/API-verktøy uten Python som hovedmotor.

Minimum:

- HTTP request/response typer
- router
- query/path params
- JSON body
- statuskoder
- middleware hook

M1-status:

- En enkel `web.route("GET /status")`-flyt er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en liten route-annotasjon med full bytecode-metadata, inkludert `route_handlers`

### 8. Test runner

Selfhost må kunne kjøre egne tester.

Minimum:

```norscode
test "summerer" {
    assert_eq(1 + 2, 3)
}
```

M1-status:

- en enkel `test "..." { ... }`-flyt er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en liten test-blokk og en `tests`-metadata-array i bytecode

## M1-status

### Structs

- Brace-baserte struct-literals med ident-felt er nå støttet i selfhost
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en enkel `returner { navn: "Ada", by: "Oslo" }`-flyt

### Maps

- Brace-baserte map-literals med tekstnøkler og homogene tekstverdier er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en enkel `returner {"en": "1", "to": "2"}`-flyt

### Try/catch

- En enkel `prøv` / `fang (feil)` / `kast("boom")`-flyt er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en enkel `returner feil`-flyt etter exception-unwind

### Generics

- Generiske containere som `liste<heltall>`, `liste<tekst>` og `ordbok<tekst, heltall>` er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en liten `summer(xs: liste<heltall>)`-flyt og container-bruk i `start`

### JSON-flyt

- En enkel `json_stringify({"navn": "Norscode", "alder": 2, "aktiv": sann})`-flyt er nå låst i parity
- selfhost parser, semantic og bytecode matcher Python-kontrakten for denne M1-formen
- parity-testen dekker nå en enkel JSON-serialisering av en map med tekst, tall og bool via builtin

## Parity-suiter

Fase 9 bør ha separate testmapper:

```text
tests/parity/structs/
tests/parity/maps/
tests/parity/errors/
tests/parity/generics/
tests/parity/lambda/
tests/parity/async/
tests/parity/patterns/
tests/parity/web/
tests/parity/test_runner/
```

Hver suite bør ha:

- `.no` input
- expected AST snapshot
- expected semantic result
- expected IR/disasm
- expected output hash

## Exit-kriterier

Fase 9 er ferdig når:

- selfhost kan bygge store deler av repoet
- Python og Norscode compiler gir likt resultat for definerte parity-suiter
- CI kjører parity automatisk
- nye språkfeatures krever selfhost-test før merge
