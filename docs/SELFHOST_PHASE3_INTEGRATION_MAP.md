# Selfhost Fase 3 - Integrasjonsmap

Dette dokumentet viser korleis dei tre første fase-3-komponentane heng saman.

## Kjeda

1. `selfhost/ast.no`
2. `selfhost/semantic.no`
3. `selfhost/compiler/ir_to_bytecode.no`

## Rollefordeling

- AST definerer stabil struktur og minimumsfelt
- Semantic validerer og beriker AST med scope- og symbolreglar
- IR-til-bytecode gjer den endelege emissiona til NCB JSON

## Praktisk kontrakt

- AST skal vere lett å validere og trygg å lese
- Semantic skal vere deterministisk og gi posisjonsbaserte feil
- IR-løypa skal vere stabil og bruke NCB JSON som output

## Kva som er kopla i fase 3 no

- [x] AST-kontrakt v1
- [x] Semantic-kjerne v1
- [x] IR-til-bytecode v1
- [x] Smoke
- [x] Regresjon

## Kva som står att for vidare arbeid

- Eventuell utviding av backend-kopling mot semantic-resultat
- Sterkare end-to-end-grep for nye språkfunksjonar
