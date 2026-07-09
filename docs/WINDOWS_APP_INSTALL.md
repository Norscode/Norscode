# Windows app-installasjon

Installer Windows app-artefakt frå ein verifisert release.

Før installasjon:

```powershell
$hash = (Get-FileHash .\norscode-windows-<versjon>.zip -Algorithm SHA256).Hash.ToLower()
$want = (Get-Content .\norscode-windows-<versjon>.zip.sha256).Split(" ")[0].ToLower()
if ($hash -ne $want) { throw "SHA256 mismatch" }
```

På system med `sha256sum`:

```bash
sha256sum -c norscode-windows-<versjon>.zip.sha256
```

Pakk ut ZIP-arkivet og køyr `norscode.exe` frå den utpakka mappa.

Etter installasjon:

```bash
norscode.exe --version
```
