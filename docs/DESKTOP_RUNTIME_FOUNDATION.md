# Norscode Desktop Runtime — første leveranse

Arbeidsgren: `feature/desktop-runtime-foundation-20261027`.

## Implementert
- `std.desktop`: backend-uavhengig app/vindu-/komponentmodell, begrenset tastatur- og pekerkontrakt, fokus, tilgjengelighetstre og headless HTML via `std.runtime_gui`.
- `std.desktop_text`: tekstbuffer, avgrenset erstatt, revisjon, dirty-status og linjetelling.
- `tests/test_desktop_foundation.no`: grenseverditester og røykprøve.

## Ikke implementert
Ingen reell macOS AppKit/Cocoa-binding, native skjermtegning, GPU, fontforming, IME, PTY, filovervåking, systemdialoger, signering eller Swift-fri Studio-app. Denne endringen skal **ikke** markere `gui_runtime` som komplett native desktop-støtte eller oppjustere 19/22-runtime-status.

## Verifisering
```sh
./bin/nc check std/desktop.no
./bin/nc check std/desktop_text.no
./bin/nc check tests/test_desktop_foundation.no
./bin/nc run tests/test_desktop_foundation.no
```
Kommandoene må kjøres i Norscode-miljø før testresultatet kan regnes som bekreftet.

## Neste steg
Implementer et eksplisitt OS-vindu-/event-backend med macOS ARM64 først, og test i faktisk GUI-økt. Flytt først deretter editorens native rendering fra Swift til Norscode.
