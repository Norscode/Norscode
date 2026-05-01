# Norscode Language Support

Denne mappen inneholder en enkel VS Code-språkpakke for Norscode.

Den gir:

- språkregistrering for `.no`
- syntaksfarging for nøkkelord, typer, strenger, tall og kommentarer
- grunnleggende bracket- og kommentarstøtte
- snippets for vanlige mønstre som funksjoner og løkker
- kommandoen `Norscode: Run Current File`
- kommandoen `Norscode: Check Current File`
- tastatursnarvei `Ctrl+Alt+R` / `Cmd+Alt+R` for å kjøre aktiv fil
- tastatursnarvei `Ctrl+Alt+Shift+C` / `Cmd+Alt+Shift+C` for å sjekke aktiv fil

Selve kjørelogikken ligger nå i `bridge.no`, så utvidelsen er bare en tynn
VS Code-adapter rundt Norscode-koden.

## Bruk

Åpne `vscode-norscode/` i VS Code for å jobbe med utvidelsen som lokalt prosjekt.

Hvis du vil pakke den som en `.vsix`, kan du bruke `vsce` fra samme mappe.

Kjør-kommandoen bruker `norscode` fra PATH som standard. Hvis du har en annen binærsti, sett
`norscode.norscodePath` i VS Code-innstillingene.

`Run Current File` og `Check Current File` går via `bridge.no`, som igjen
starter `norscode run` eller `norscode check` for den aktive fila.

Snarveien for kjøring er aktiv bare i Norscode-filer.
Snarveien for sjekk er også aktiv bare i Norscode-filer.

## Videre steg

Hvis du vil, kan vi også legge til:

- kommando for å kjøre aktuell fil med `norscode`
- feildiagnostikk via språkserver eller `norscode check`
- debugging-oppsett for Norscode i VS Code
