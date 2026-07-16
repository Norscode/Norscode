# Linux app-installasjon

Installer Linux app-artefakt frå ein verifisert release.

Før installasjon:

```bash
sha256sum -c Norscode-linux-<versjon>-AppDir.tar.gz.sha256
```

Pakk ut AppDir-arkivet og køyr den medfølgjande Norscode-launcheren. Dersom AppImage finst for releasen, kan den brukast direkte etter tilsvarande `.sha256`-sjekk.

Etter installasjon:

```bash
./bin/nc --version
./bin/nc doctor
```
