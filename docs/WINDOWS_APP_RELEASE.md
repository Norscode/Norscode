# Windows app-release

Windows app-release pakkar ein ferdig `norscode.exe` inn i ZIP-distribusjon.

Køyr alltid lokal preflight før tag eller publisering:

```bash
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green --strict
```

Arbeidsflyten publiserer berre frå `v*`-taggar og lastar opp:

- `norscode-windows-<versjon>.zip`
- `norscode-windows-<versjon>.zip.sha256`

Arbeidsflyten feilar dersom han ikkje finn ein ferdig Windows-native `norscode.exe` i dei dokumenterte release-stiane.
