# Parser Maturity Checklist

Mål: fullføre selfhost parser-kjernen til et nivå hvor parity, recovery og moderne språkfeatures er stabile.

## Struct Parser

- [ ] parse_struct_def
- [x] parse_struct_literal
- [ ] parse_field_value
- [x] nested structs
- [x] struct parity fixtures
- [x] semantic struct validation
- [x] BUILD_STRUCT lowering
- [ ] GET_FIELD lowering
- [ ] struct runtime objects

## Try/Catch Parser

- [ ] parse_try
- [ ] parse_catch
- [ ] parse_throw
- [ ] nested try/catch
- [ ] catch scopes
- [ ] THROW lowering
- [ ] TRY_BEGIN lowering
- [ ] exception parity fixtures

## Generics Parser

- [ ] parse_generic_params
- [ ] parse_generic_type
- [ ] generic struct support
- [ ] generic function support
- [ ] nested generics
- [ ] generic semantic scopes
- [ ] generic parity fixtures

## Parser Recovery

- [ ] sync after syntax error
- [ ] collect multiple errors
- [ ] line/column diagnostics
- [ ] expected token diagnostics
- [ ] recovery hooks for blocks
- [ ] recovery hooks for expressions
- [ ] IDE diagnostic format

## Full Parity Suite

- [ ] AST snapshots
- [ ] semantic snapshots
- [ ] IR snapshots
- [ ] deterministic hashes
- [ ] golden fixtures
- [ ] CI parity validation
- [ ] selfhost parity reports

## Exit Criteria

Parser maturity er ferdig når:

- selfhost parser støtter structs, exceptions og generics
- parser recovery fungerer stabilt
- parity fixtures er deterministiske
- CI kan validere snapshots automatisk
- selfhost compiler kan parse store deler av repoet stabilt
