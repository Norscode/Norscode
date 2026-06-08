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

## Kontrollpanel

`SHA256SUMS` inneheld SHA256-samandraging for stage-0-fila i denne mappa.

```sh
shasum -a 256 -c SHA256SUMS
```

Oppdater manifestet med:

```sh
bash tools/update_stage0_manifest.sh
```

## Legg inn binærer (første gang / når CI er rød)

```sh
# Bygg eller finn dist/norscode_native lokalt (må støtte NORSCODE_CMD=run)
cp dist/norscode_native bootstrap/stage0/norscode-macos-arm64
chmod +x bootstrap/stage0/norscode-macos-arm64

# Publiser til GitHub Release (valfritt, men anbefalt)
gh release upload v0.1.0 bootstrap/stage0/norscode-macos-arm64 --clobber
```

Etter commit av filene i `bootstrap/stage0/` vil `tools/build_norscode_native.sh` fungere utan nettverk.

**Linux-seed (migrering frå bootstrap/maint/c):**

```sh
# På Linux, eller GitHub Actions → «Export stage0 Linux seed»
bash tools/maint/migrate_bootstrap_c_to_stage0.sh
git add bootstrap/stage0/norscode-linux-x86_64
bash tools/maint/finish_6b4.sh   # git rm bootstrap/maint/c/*.c
```

**Standard:** `tools/build_norscode_native.sh` brukar seed herifrå (eller release) og krev ikkje clang.

## Kva som er produksjonsløpet for ny seed

Ny `bootstrap/stage0/norscode-<plattform>` produseres frå same normale pipeline som bygg `dist/norscode_native`:

1. `./bin/nc`/`dist/norscode_native` må vere tilgjengeleg.
2. `tools/build_norscode_native.sh` byggjer/oppdaterer `dist/norscode_native` frå seed.
3. `publish.yml` (Linux/OS X byggesteg) kopierer `dist/norscode_native` til plattformartefakt.
4. Ved vedlikehald kan ein også bruke `bash tools/maint/migrate_bootstrap_c_to_stage0.sh` i `export-stage0-linux.yml` for éin-gong migrering til `bootstrap/stage0/`.

Når ny seed er verifisert, legg vi berre inn fila i git og køyrer evt. `bash tools/update_stage0_manifest.sh`.

**Maintainer / regen:** `REGEN=1 bash tools/build_norscode_native.sh` køyrer
`tools/maint/regen_native.sh` for å lage `bootstrap/maint/c/`, og kompilerer med clang.
Ingen committed `.c` i repo.

**Opt-in .no-host:** `NORSCODE_USE_NC_MAIN=1` delegerer `run`/`compile`/`selftest` til
`selfhost.nc_main.start` (krev regen-bundle med `nc_main.no`). Smoke: `bash tools/verify_nc_main_host.sh`.
