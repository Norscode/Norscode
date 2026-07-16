# v821-v830 random_hex design

Dette er eit designnotat, ikkje ein runtime-implementasjon.

`random_hex` skal ikkje reknast som produksjonsklar før ny stage0/native runtime har ekte kryptografisk sikker random-kjelde.

## Mål

- `builtin.random_hex(32)` skal returnere 64 hex-teikn frå 32 sikre bytes.
- `builtin.random_bytes(n)` skal returnere `n` sikre bytes i ein runtime-representasjon som Norscode kan bruke stabilt.
- `builtin.uuid()` skal returnere UUID v4 basert på sikker random.

## API

```no
la token = builtin.random_hex(32)
la bytes = builtin.random_bytes(32)
la id = builtin.uuid()
```

## Krav til runtime

- Må bruke OS-krypto-random, til dømes `getrandom`, `/dev/urandom`, `arc4random_buf` eller plattform-ekvivalent.
- Må feile hardt dersom trygg random ikkje er tilgjengeleg.
- Må ikkje falle tilbake til tid, PID, teller eller pseudo-random utan sikker seed.
- Må ha same semantikk på macOS og Linux.
- Må dekkast av stage0-test før web-login kan kallast produksjonssikker.

## Produksjonssperre

Så lenge `random_hex` manglar i stage0/native skal web-status vere:

```json
{
  "production_ready": false,
  "reason": "stage0_mangler_random_hex"
}
```

## Ikkje-gjort i denne v-runden

- Ingen endring i `dist/norscode_native`.
- Ingen overskriving av `bootstrap/stage0`.
- Ingen DEV-login oppgradering til produksjonslogin.

