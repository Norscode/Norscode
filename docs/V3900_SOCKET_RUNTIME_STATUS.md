# V3900 Socket Runtime Status

Dato: 2026-06-25
Fase: v3900 runtime phase 3

## Status

Socket-builtinane er no grøne i aktiv macOS `dist/norscode_native` og i Linux x86_64 `bootstrap/stage0/norscode-linux-x86_64`.

Verifisert roundtrip:
- `socket_listen`
- `socket_settimeout`
- `socket_accept`
- `socket_read`
- `socket_write`
- `socket_close`

Forventa svar er verifisert på begge plattformspor:
- `pong:ping`

## Fiks som måtte inn

1. Native dispatch i `archive/legacy_c_backend/nc_native_main.c` mangla først socket-dispatch heilt.
2. Deretter mangla aliasa `socket_read` og `socket_write`, sjølv om `socket_recv`/`socket_send` fanst.
3. Linux stage0 måtte oppdaterast med fersk kandidatkopi etter rebuild, slik at aktiv `bootstrap/stage0/norscode-linux-x86_64` fekk same socket-fiks som kandidaten.

## Resultat

`socket_status=green`

Dette lukkar socket-delen av fase 3 for:
- macOS arm64 aktiv dist
- Linux x86_64 stage0
