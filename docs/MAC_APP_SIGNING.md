# macOS app-signering

macOS app-signering skjer i release-arbeidsflyten med Developer ID når hemmelegheiter er sette, og med ad-hoc fallback elles.

Lokal kontroll før release:

```bash
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green --strict
./bin/nc verify-macos-app path/to/Norscode.app
```

Kontroller alltid publiserte artefaktar mot tilhøyrande `.sha256` før installasjon eller vidare distribusjon.

```bash
shasum -a 256 -c Norscode-macos-<versjon>.zip.sha256
```

Notarisering køyrer berre når `APPLE_ID`, `APPLE_TEAM_ID`, `APPLE_APP_PASSWORD` og `MACOS_CODESIGN_IDENTITY` er tilgjengelege i arbeidsflyten.

Release skal framleis vere byggbar utan å gjere C/Python til aktiv normalveg.
