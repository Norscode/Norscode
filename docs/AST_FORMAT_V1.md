# Norscode AST Format v1

Dette dokumentet definerer den stabile AST-kontrakten for Norscode.

Målet er at flere compiler-fronter kan produsere samme format:

- dagens historisk parser
- selfhost-parser
- fremtidig Norscode-parser
- eventuelle IDE-/tooling-parserere

Bytecode-generatoren og compiler core skal kunne forholde seg til dette formatet uten å vite hvilken parser som produserte det.

## Formatnavn

```json
{
  "format": "norscode-ast-v1"
}
```

Alle AST-dokumenter må inneholde `format`.

## Toppnivå

```json
{
  "format": "norscode-ast-v1",
  "alias_map": {},
  "imports": [],
  "functions": []
}
```

### Felt

- `format`: alltid `norscode-ast-v1`
- `alias_map`: map fra importalias til modulnavn
- `imports`: liste med importerte moduler
- `functions`: liste med funksjoner

## Import

```json
{
  "module_name": "std.tekst",
  "alias": "tekst"
}
```

### Felt

- `module_name`: fullt modulnavn
- `alias`: valgfritt alias, eller `null`

## Function

```json
{
  "type": "function",
  "name": "start",
  "module_name": "__main__",
  "params": [],
  "return_type": "heltall",
  "body": {
    "type": "block",
    "statements": []
  }
}
```

### Felt

- `type`: `function`
- `name`: funksjonsnavn
- `module_name`: modulnavn, vanligvis `__main__`
- `params`: liste med parametere
- `return_type`: returtype
- `body`: block-node

## Parameter

```json
{
  "name": "x",
  "type": "heltall"
}
```

## Block

```json
{
  "type": "block",
  "statements": []
}
```

## Minimum statements for v1

Følgende statements er del av minimumskontrakten:

- `var_declare`
- `var_set`
- `expr_stmt`
- `print`
- `return`
- `if`
- `while`
- `for`
- `for_each`
- `break`
- `continue`
- `throw`
- `try_catch`

## Minimum expressions for v1

Følgende expressions er del av minimumskontrakten:

- `number`
- `string`
- `bool`
- `var_access`
- `bin_op`
- `unary_op`
- `call`
- `module_call`
- `list_literal`
- `map_literal`
- `index`
- `field_access`
- `if_expr`
- `lambda`

## Type names

Standard type-navn i v1:

- `heltall`
- `tekst`
- `bool`
- `liste_heltall`
- `liste_tekst`
- `ordbok_heltall`
- `ordbok_tekst`
- `ordbok_bool`

## Stabilitetsregler

1. Nye felter kan legges til, men eksisterende felter skal ikke endre betydning.
2. Nye node-typer kan legges til uten å bryte v1.
3. Bytecode-generatoren skal ignorere ukjente metadatafelter når mulig.
4. Parser og selfhost-parser skal testes mot samme AST fixtures.
5. Breaking changes krever nytt formatnavn, for eksempel `norscode-ast-v2`.

## Selfhost-krav

For at Norscode-parseren skal regnes som kompatibel må den kunne:

- produsere `norscode-ast-v1`
- bevare `module_name`
- bevare import/alias-informasjon
- produsere samme funksjonsstruktur som historisk parseren for kjerneprogrammer
- passere parser parity-fixtures

## Neste steg

- Lage AST validator for `norscode-ast-v1`
- Koble `ast_service` til validering
- Bruke samme validator i CI
- Bruke formatet som kontrakt for Norscode-native parser
