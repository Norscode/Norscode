# Windows app-release

Windows app-release byggjer `norscode.exe` frå Norscode-kjelde på Linux og
køyrer runtime-testane på ekte Windows. Den same CRT-frie binæren blir
attestert, pakka i eit ZIP-arkiv og signert med GitHub-provenance.

Køyr alltid lokal preflight før tag eller publisering:

```bash
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green --strict
```

Arbeidsflyten publiserer berre frå `v*`-taggar og lastar opp:

- `norscode-windows-<versjon>.zip`
- `norscode-windows-<versjon>.zip.sha256`
- `windows-runtime-attestation.json`

Arbeidsflyten feilar dersom han ikkje finn ein ferdig Windows-native `norscode.exe` i dei dokumenterte release-stiane.
