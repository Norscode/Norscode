# Linux app-release

Linux app-release byggjer AppDir-arkivet gjennom Norscode. AppImage er ein
eksplisitt valfri adapter, ikkje ein del av normalflyten.

Køyr alltid lokal preflight før tag eller publisering:

```bash
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green --strict
```

Arbeidsflyten publiserer berre frå `v*`-taggar og lastar opp:

- `Norscode-linux-<versjon>-AppDir.tar.gz`
- `Norscode-linux-<versjon>-AppDir.tar.gz.sha256`
- valfritt `Norscode-linux-<versjon>.AppImage` og sidecar berre etter
  `./bin/nc package-linux-app --format appimage-adapter`

Normal release skal ikkje krevje C/Python som aktiv arbeidsveg.
