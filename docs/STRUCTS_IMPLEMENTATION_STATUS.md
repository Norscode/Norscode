# Structs — Implementasjonsstatus

## Ferdig AST-støtte

Selfhost AST-systemet støtter nå:

- StructDef
- StructLiteral
- FeltVerdi
- Felt

Normalisering:

- STRUCT
- STRUKTUR
- STRUCT_LITERAL
- FIELD_VALUE

StructLiteral regnes som:

- literal
- uttrykk
- snapshot-stabil node

StructDef regnes som:

- toppnivå-setning
- semantic-analyserbar node

## Semantic-plan

Semantic-laget skal:

1. registrere struct-typer
2. validere felt
3. validere struct-literals
4. validere feltaksess
5. validere type-match

Planlagte semantic-feil:

- ukjent struct
- ukjent felt
- duplikat felt
- manglende felt
- type mismatch

Planlagte semantic-strukturer:

```text
StructInfo
  navn
  felt
```

## IR-plan

Planlagte IR-opcodes:

```text
BUILD_STRUCT
GET_FIELD
SET_FIELD
```

Planlagt lowering:

```text
PUSH_TEXT Ola
PUSH_TEXT navn
PUSH_INT 42
PUSH_TEXT alder
BUILD_STRUCT Bruker 2
```

Feltaksess:

```text
LOAD bruker
GET_FIELD navn
```

## Runtime-plan

Første runtime-format:

```text
{
  "__type__": "Bruker",
  "navn": "Ola",
  "alder": 42
}
```

## Exit-kriterier

Structs er ferdig når:

- parser leser struct-definisjoner
- semantic validerer structs
- backend lower structs til IR
- runtime kan lagre structs
- parity fixtures er stabile
