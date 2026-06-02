# Stage-0 binærer (`norscode_native`)

CI og `./bin/nc` trenger én ferdig `norscode_native` per plattform. Normal rekkefølge:

1. `dist/norscode_native` finst lokalt
2. Nedlasting frå siste [GitHub Release](https://github.com/Norscode/Norscode/releases)
3. Kopiering frå denne mappa: `bootstrap/stage0/norscode-<plattform>`

## Filnamn

| Plattform | Fil |
|-----------|-----|
| macOS arm64 | `norscode-macos-arm64` |
| macOS x86_64 | `norscode-macos-x86_64` |
| Linux x86_64 | `norscode-linux-x86_64` |
| Linux arm64 | `norscode-linux-arm64` |

## Legg inn binærer (første gang / når CI er rød)

```sh
# Bygg eller finn dist/norscode_native lokalt (må støtte NORSCODE_CMD=run)
cp dist/norscode_native bootstrap/stage0/norscode-macos-arm64
chmod +x bootstrap/stage0/norscode-macos-arm64

# Publiser til GitHub Release (valfritt, men anbefalt)
gh release upload v0.1.0 bootstrap/stage0/norscode-macos-arm64 --clobber
```

Etter commit av filene i `bootstrap/stage0/` vil `tools/build_norscode_native.sh` fungere utan nettverk.

**Linux-seed (migrering frå bootstrap/c):**

```sh
# På Linux, eller GitHub Actions → «Export stage0 Linux seed»
bash tools/migrate_bootstrap_c_to_stage0.sh
git add bootstrap/stage0/norscode-linux-x86_64
bash tools/finish_6b4.sh   # git rm bootstrap/c/*.c
```

**Standard:** `tools/build_norscode_native.sh` brukar seed herifrå (eller release) og krev ikkje clang.

**Maintainer / regen:** `REGEN=1 bash tools/build_norscode_native.sh` køyrer
`tools/regen_native.sh` for å lage `bootstrap/c/`, og kompilerer med clang.
Ingen committed `.c` i repo.

**Opt-in .no-host:** `NORSCODE_USE_NC_MAIN=1` delegerer `run`/`compile`/`selftest` til
`selfhost.nc_main.start` (krev regen-bundle med `nc_main.no`). Smoke: `bash tools/verify_nc_main_host.sh`.
