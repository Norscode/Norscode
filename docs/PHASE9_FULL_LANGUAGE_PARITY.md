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
| Structs | TODO | TODO | TODO | TODO | TODO | TODO |
| Maps | Delvis | Delvis | Delvis | Delvis | Delvis | TODO |
| JSON-flyt | TODO | TODO | TODO | TODO | TODO | TODO |
| Try/catch | TODO | TODO | TODO | TODO | TODO | TODO |
| Async/await | TODO | TODO | TODO | TODO | TODO | TODO |
| Generics | TODO | TODO | TODO | TODO | TODO | TODO |
| Lambdaer | TODO | TODO | TODO | TODO | TODO | TODO |
| Pattern matching | TODO | TODO | TODO | TODO | TODO | TODO |
| Web/server-kjerne | TODO | TODO | TODO | TODO | TODO | TODO |
| Test runner | TODO | TODO | TODO | TODO | TODO | TODO |

## Prioritert rekkefølge

### 1. Structs

Structs bør komme først fordi compiler, AST og runtime trenger strukturerte data.

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

### 4. Lambdaer

Må støtte:

```norscode
la dobbel = (x) => x * 2
```

Semantic:

- closure scope
- parameterinferens eller eksplisitt type
- kallbar type

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

### 6. Pattern matching

Kan bygges etter structs og enums/variant-typer.

Må støtte:

```norscode
match verdi
    når 1 -> "en"
    ellers -> "annet"
slutt
```

### 7. Web/server-kjerne

Målet er at Norscode kan bygge egne web/API-verktøy uten Python som hovedmotor.

Minimum:

- HTTP request/response typer
- router
- query/path params
- JSON body
- statuskoder
- middleware hook

### 8. Test runner

Selfhost må kunne kjøre egne tester.

Minimum:

```norscode
test "summerer" {
    assert_eq(1 + 2, 3)
}
```

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
