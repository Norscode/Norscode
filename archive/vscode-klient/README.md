# Norscode VS Code-klient (generert)

Denne mappa er GENERERT av `tools/lag_vscode_klient.no`. Ikkje rediger ho for hand:
endre generatoren og køyr `nc run tools/lag_vscode_klient.no`. JavaScript høyrer ikkje
heime i aktiv Norscode-flate (AGENTS.md, `framandspråk`), og `archive/` er unnateke.

Innhald: `extension.js` (klient med `vscode-languageclient`), `package.json` (manifest =
`vscode-norscode/package.json` + main/avhengnad/innstillingar).

## Pakking og installasjon

```sh
mkdir -p /tmp/norscode-vsix && cp -R vscode-norscode/. /tmp/norscode-vsix/
cp archive/vscode-klient/extension.js archive/vscode-klient/package.json /tmp/norscode-vsix/
cd /tmp/norscode-vsix && npm install && npx vsce package
code --install-extension norscode-*.vsix
```

Innstillingar: `norscode.server.command` (standard `nc`), `norscode.server.args`
(standard `["lsp"]`), `norscode.server.env`. Sjå docs/LSP.md.
