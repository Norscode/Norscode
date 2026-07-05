# Platform

Denne mappa inneheld små plattformgrenser som Norscode ikkje kan erstatte fullt ut enno.

Aktive filer her skal ha ein Norscode-eigar ved sida av seg eller ein Norscode-byggar som validerer og brukar dei. For macOS-window-hosten er `Main.swift` AppKit/WebKit-brua, medan `Main.no`, `app.no` og `tools/build-macos-window-host.no` eig kontrollen, malen og bygginga.

## Reglar

- Ny funksjonalitet skal skrivast i `.no` når det er mogleg.
- Plattformkode skal liggje under `platform/<os>/...`, ikkje blandast inn i `tools/`.
- Kvar aktiv `.swift`, `.js` eller `.sh`-bru skal ha ein colocated `.no`-eigar eller ein dokumentert Norscode-byggar.
- `tools/verify_norscode_surface_ownership.sh` skal halde denne regelen grøn.

## Aktiv plattformbru

- `macos/window-host/Main.swift`: AppKit/WebKit-vindauge for lokal macOS-app. Dette er OS-API-overflata; Norscode eig malen, kontrollfila og byggestyringa.
