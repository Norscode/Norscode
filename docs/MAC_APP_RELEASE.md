# macOS app-release

macOS app-release byggjer app-pakke og distribusjonsartefaktar frå Norscode-verktøya.

Køyr alltid lokal preflight før tag eller publisering:

```bash
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green --strict
```

Arbeidsflyten publiserer berre frå `v*`-taggar og lastar opp:

- `Norscode-macos-<versjon>.zip`
- `Norscode-macos-<versjon>.zip.sha256`
- `Norscode-macos-<versjon>.pkg`
- `Norscode-macos-<versjon>.pkg.sha256`
- `Norscode-macos-<versjon>.dmg`
- `Norscode-macos-<versjon>.dmg.sha256`

DMG kan mangla dersom miljøet ikkje støttar byggesteget i arbeidsflyten.
