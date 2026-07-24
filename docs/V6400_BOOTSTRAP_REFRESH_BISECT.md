# V6400 Bootstrap refresh bisect

## Formål
Dette steget snevrar inn kvar i bundle-løypa `Stack underflow` først dukkar opp.

## Verktøy
- `tools/bootstrap_refresh_bisect_v6400.no`

Skriptet byggjer bundle stegvis:
1. lexer
2. parser
3. semantic
4. ir_to_bytecode
5. kompiler
6. json
7. vm
8. bundler
9. nc_main

For kvart steg blir det skrive:
- `build/v6400/bisect/<step>.ncb.json` ved suksess
- `build/v6400/bisect/<step>.log` alltid med output/logg

## Tolkning
Det første raude steget er den smalaste kjende reproduksjonen av bootstrap-refresh-feilen.
