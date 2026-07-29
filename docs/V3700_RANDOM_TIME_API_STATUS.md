# v3700 Random and Time API Status

Status: random/tid-lina er delvis ferdig på runtime-nivå, men ikkje komplett på builtin-overflata.

## Grønt no

Desse builtin-kalla er verifiserte grøne i:

- aktiv macOS `dist/norscode_native`
- macOS `bootstrap/stage0/norscode-macos-arm64`
- Linux x86_64 `bootstrap/stage0/norscode-linux-x86_64`

```text
builtin.random_hex(n)
builtin.tid_ms()
builtin.tid_no()
builtin.now()
builtin.timestamp()
```

## Framleis ikkje ferdig som builtin-kontrakt

```text
builtin.random_bytes(n)
builtin.uuid()
```

## Praktisk status

- `std.krypto.token(...)` kan bruke `builtin.random_hex(...)`.
- `std.krypto.uuid4()` har fallback og fungerer utan `builtin.uuid()`.
- `std.tid.no()` og `std.tid_ms()` er grøne i aktiv runtime/stage0-lina.

## Produksjonsvurdering

Random/tid-delen er produksjonsklar for den delen som faktisk er implementert i native/stage0.

Det som framleis står att i denne fasen er å avgjere om:

- `builtin.random_bytes(n)` skal inn som rå builtin
- `builtin.uuid()` skal vere eigen builtin eller halde fram som std-fallback
