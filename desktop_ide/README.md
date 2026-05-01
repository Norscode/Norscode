# Norscode Studio (Desktop)

Norscode Studio er en lokal, installasjonsvennlig desktop-app (Tkinter),  
ikke en nettleser-IDE.

Studio kjører lokal kode via Norscode CLI og gir AI-assistanse via en ren Norscode-driver:
- `desktop_ide/studio_engine.no`
- Codex (`CODEX_API_KEY` / `OPENAI_API_KEY`)
- Gemini (`GEMINI_API_KEY`)

## Oppsett

- `NORSCODE_CLI` (standard: `norscode`)  
  Kommando for lokal kjøring av `.no`-filer.
- `CODEX_API_KEY` eller `OPENAI_API_KEY`  
  Nøkkel for Codex/Chat Completion API.
- `CODEX_API_URL` (standard: `https://api.openai.com/v1/chat/completions`)
- `CODEX_MODEL` (standard: `gpt-4o-mini`)
- `GEMINI_API_KEY`
- `GEMINI_MODEL` (standard: `gemini-1.5-flash`)

## Kjøre lokalt

```bash
cd /Users/jansteinar/Projects/language_handoff/projects/language
python3 desktop_ide/main.py
```

Tekstmodus (hvis GUI ikke kan startes):

```bash
cd /Users/jansteinar/Projects/language_handoff/projects/language
python3 desktop_ide/main.py --no-gui
```


## Bygg (macOS)

```bash
cd /Users/jansteinar/Projects/language_handoff/projects/language
bash scripts/package-desktop-ide.sh app
```

Resultat:
- `projects/language/dist_desktop_ide/Norscode IDE.app`
- `projects/language/dist_desktop_ide/Norscode-IDE.dmg`

## Studio-funksjoner

- **Kjør kode**: kjørt mot valgt modus (`run`, `test`, `fmt`, `lint`) via local CLI.
- **Spør AI**: sender prompt + nåværende kildekode til valgt leverandør.
- **Refaktorer**: ber om kodeforbedring og erstatter editor hvis AI returnerer kode.

## Hvorfor kan appen krasje?

Crash-rapportene vi ser peker mot `Tk()`-lagring av GUI-laget (`TkpOpenDisplay`), ofte i headless- eller sterkt begrensede miljøer.
På en vanlig Mac-brukersesjon uten slike begrensninger bør appen åpne normalt.
Hvis appen likevel krasjer ved oppstart, vil den forsøke tekstmodus, og du kan kjøre med:

```bash
python3 desktop_ide/main.py --no-gui
```

og åpne appen på nytt. Når du starter den som applikasjon, vil GUI fortsatt være standard.
