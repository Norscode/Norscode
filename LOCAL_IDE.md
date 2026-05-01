# Lokal Norscode IDE (VS Code)

Du kan nå bruke en ekte lokal IDE ved å installere Norscode VS Code-utvidelsen.

## Installer på ett trinn

```bash
cd /Users/jansteinar/Projects/language_handoff/projects/language
bash scripts/install-vscode-ide.sh
```

Skriptet gjør dette:

- Bygger en `.vsix` fra `vscode-norscode/`
- Installerer den via `code --install-extension`

## Forutsetninger

- Visual Studio Code installert
- `code` i PATH (`Command Palette → Shell Command: Install 'code' command in PATH`)
- `node` installert (for `vsce`/`npx` under bygging av pakken)

## Hva du får etter installasjon

- Språkstøtte for `.no` filer
- Syntax highlighting
- Snippets
- Kommander:
  - `Norscode: Run Current File` (`Ctrl/Cmd + Alt + R`)
  - `Norscode: Check Current File` (`Ctrl/Cmd + Alt + Shift + C`)

Hvis du heller vil installere uten npm-pakken, kan du åpne mappen `projects/language/vscode-norscode/` direkte i VS Code og pakke utvidelsen med `vsce` manuelt.

## Lokal desktop-IDE (installasjon)

Hvis du vil ha en egen desktop-app på maskinen:

```bash
cd /Users/jansteinar/Projects/language_handoff/projects/language
bash scripts/package-desktop-ide.sh app
bash scripts/install-desktop-ide.command
```

Dette bygger en separat Tkinter-basert Norscode IDE og installerer en `.app`/`.dmg` (macOS).
