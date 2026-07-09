# Linux app-release

Linux app-release byggjer AppDir-arkiv og AppImage-artefakt når miljøet støttar det.

Køyr alltid lokal preflight før tag eller publisering:

```bash
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green --strict
```

Arbeidsflyten publiserer berre frå `v*`-taggar og lastar opp:

- `Norscode-linux-<versjon>-AppDir.tar.gz`
- `Norscode-linux-<versjon>-AppDir.tar.gz.sha256`
- `Norscode-linux-<versjon>.AppImage`
- `Norscode-linux-<versjon>.AppImage.sha256`

Normal release skal ikkje krevje C/Python som aktiv arbeidsveg.
