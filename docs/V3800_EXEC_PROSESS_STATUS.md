# v3800 exec_prosess status

Status: `exec_prosess` er implementert, men med vilje avgrensa.

## Grønt no

- aktiv macOS `dist/norscode_native`
- macOS `bootstrap/stage0/norscode-macos-arm64`
- Linux x86_64 `bootstrap/stage0/norscode-linux-x86_64`

Runtime-gap-proben viser:

```text
exec_prosess OK
```

## Gjeldande tryggingsmodell

- Default: av
- Aktivering: `NORSCODE_ENABLE_EXEC_PROSESS=1`
- Formål no: DEV, probe, lokal verifisering og avgrensa verktøylag

## Framleis att

- whitelist for produksjon
- timeout i sjølve exec-laget
- maks output-grense i sjølve exec-laget
- strukturert resultatkontrakt utover dagens enkle retur
- logging/policy-notat for blokkeringar

## Konklusjon

`exec_prosess` er ikkje lenger runtime-gap. Han er no ein DEV-gated funksjon som framleis treng produksjonspolicy før vidare oppgradering.
