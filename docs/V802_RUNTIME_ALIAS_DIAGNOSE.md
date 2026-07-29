# v802 runtime alias diagnose

Dette steget er reint diagnose-/normaliseringsarbeid.

Det implementerer ikkje nye runtime-builtins, og det endrar ikkje `dist/norscode_native` eller `bootstrap/stage0`.

## Funn

- `builtin.random_hex` manglar framleis i runtime.
- `builtin.exec_prosess` manglar framleis i runtime.
- `builtin.tid`, `builtin.now` og `builtin.timestamp` manglar framleis i runtime.
- `builtin.socket_listen` manglar framleis som direkte builtin.
- Test-NCB kan framleis innehalde aliaset `builtin.builtin.finnes_nøkkel`.

## Tiltak

`tools/ncb_normalize_builtin_aliases_v802.no` kopierer ein NCB-fil til ny output-fil og normaliserer berre test-callar:

- `builtin.builtin.finnes_nøkkel` -> `builtin.har_nokkel`
- `builtin.finnes_nøkkel` -> `builtin.har_nokkel`
- `builtin.builtin.har_nokkel` -> `builtin.har_nokkel`
- `builtin.builtin.*` -> `builtin.*` for CALL-instruksjonar

Dette er ein test-hygiene-fiks for VM/NCB-diagnose. Det skal ikkje tolkast som at `finnes_nøkkel` eller andre manglande builtins er ferdig i stage0/native.

## Bruk

```sh
cd "/Users/jansteinar/Norscode AI/prosjekter/Norscode"
./bin/nc compile runtime_key_v800.no build/v802_key_test.raw.ncb.json
NORSCODE_NCB_NORMALIZE_IN=build/v802_key_test.raw.ncb.json NORSCODE_NCB_NORMALIZE_OUT=build/v802_key_test.ncb.json ./bin/nc run tools/ncb_normalize_builtin_aliases_v802.no
./bin/nc run-ncb build/v802_key_test.ncb.json
```
