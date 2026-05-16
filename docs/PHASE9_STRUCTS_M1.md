# Fase 9 M1 — Structs

Mål: legge inn første stabile selfhost-støtte for `struktur`, felt, struct-literal og feltaksess.

## Syntax

```norscode
struktur Bruker
    navn: tekst
    alder: heltall
slutt

la bruker = Bruker { navn: "Ola", alder: 42 }
returner bruker.navn
```

## Lexer

Må støtte token:

- `STRUKTUR`
- `IDENT`
- `COLON`
- `LBRACE`
- `RBRACE`
- `COMMA`
- `DOT`
- `SLUTT`

## AST

Nye node-typer:

```text
StructDef(navn)
  Felt(navn)
    Type(type)

StructLiteral(navn)
  FeltVerdi(navn)
    Expr

Felt(navn)
  Expr
```

## Parser

Parser må kunne lese:

- strukturdefinisjon på toppnivå
- feltliste med navn og type
- struct-literal som uttrykk
- feltaksess med punktum

## Semantic

Regler:

- duplikat struct-navn feiler
- duplikat feltnavn feiler
- ukjent struct-type feiler
- ukjent felt feiler
- struct-literal må ikke ha ukjente felt
- manglende påkrevde felt skal rapporteres
- feltverdi skal typekontrolleres mot felt-type når typen er kjent

## IR/backend

Nye IR-ops:

```text
BUILD_STRUCT <navn> <felt_antall>
GET_FIELD <feltnavn>
SET_FIELD <feltnavn>
```

For literal:

```norscode
Bruker { navn: "Ola", alder: 42 }
```

Forventet stack-lowering:

```text
PUSH_TEXT Ola
PUSH_TEXT navn
PUSH_INT 42
PUSH_TEXT alder
BUILD_STRUCT Bruker 2
```

For feltaksess:

```norscode
bruker.navn
```

Forventet:

```text
LOAD bruker
GET_FIELD navn
```

## Parity fixture

Første testcase:

```norscode
struktur Bruker
    navn: tekst
    alder: heltall
slutt

funksjon main() -> tekst {
    la bruker = Bruker { navn: "Ola", alder: 42 }
    returner bruker.navn
}
```

Skal gi:

- AST snapshot
- semantic OK
- IR snapshot
- output hash

## Exit-kriterier

Structs M1 er ferdig når:

- parser lager `StructDef`, `StructLiteral` og `Felt`
- semantic validerer navn og felt
- backend lager `BUILD_STRUCT` og `GET_FIELD`
- parity fixture kjører deterministisk
