# V6200 Bootstrap refresh execution

## Resultat
Bootstrap-refreshen vart køyrd, men stoppa i første steg.

Køyrd kommando:
- `NORSCODE_V6000_MODE=--apply ./bin/nc run tools/refresh_bootstrap_compiler_v6000.no`

Feil:
- `NORSCODE_REGEN_BOOTSTRAP_FULL=1 ./bin/nc run tools/nc_regen_bootstrap.no` stoppa med `norscode: Stack underflow`

## Tolkning
Dette er eit viktig funn:
- refresh-pipelineen er riktig definert
- men den gamle bundlede/bootstrap compile-løypa klarer framleis ikkje å regenerere seg sjølv frå dagens kjelder
- blokkeringa er dermed flytta frå "uklar codegen-feil" til ein presis og reproduserbar bootstrap-refresh-feil

## Konsekvens
Vi kan enno ikkje påstå at bootstrap-compileren er oppfriska eller at `BUILD_LIST` / `BUILD_MAP` / `INDEX_GET` er komne inn i aktiv compile-løype via refresh.

## Neste rette steg
Neste steg bør vere ein diagnosefase som snevrar inn kvar i `--full`-bundle-løypa stack-underflow faktisk skjer:
1. bundle av mindre modulsett
2. finne første modul/kallkjede som tippar stacken
3. isolere om feilen sit i bundler, compiler bridge eller sjølve compile-driveren
