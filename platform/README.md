# Platform

Denne mappa inneheld små plattformgrenser som Norscode ikkje kan erstatte fullt ut enno.

> **Sjølvstende:** Alt her er **opt-in framand tooling** for desktop-GUI-pakking på
> ein spesifikk plattform. Den sjølvhosta kjeda — kompilere, køyre, sjølvhoste og
> native ELF/Mach-O/PE-codegen — treng **null** av dette. Sjå
> [docs/BYGG_ALT_SELVSTENDIG_PLAN.md](../docs/BYGG_ALT_SELVSTENDIG_PLAN.md) Blokk A3/C.

Aktive filer her skal ha ein Norscode-eigar ved sida av seg eller ein Norscode-byggar som validerer og brukar dei. For macOS-window-hosten er `Main.swift` AppKit/WebKit-brua, medan `Main.no`, `app.no` og `tools/build-macos-window-host.no` eig kontrollen, malen og bygginga.

## Reglar

- Ny funksjonalitet skal skrivast i `.no` når det er mogleg.
- Plattformkode skal liggje under `platform/<os>/...`, ikkje blandast inn i `tools/`.
- Kvar aktiv `.sh`, `.ps1`, `.js` eller `.swift`-bru skal ha ein colocated `.no`-eigar eller ein dokumentert Norscode-byggar.
- `./bin/nc surface-ownership` skal halde denne regelen grøn.

## Aktiv plattformbru

- `macos/window-host/Main.swift`: AppKit/WebKit-vindauge for lokal macOS-app. Dette er OS-API-overflata; Norscode eig malen, kontrollfila og byggestyringa.
