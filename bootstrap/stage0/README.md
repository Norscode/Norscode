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

**Standard i CI:** `tools/build_norscode_native.sh` kompilerer frå `bootstrap/c/` + `tools/nc_native_main.c`
(clang, utan Python). Filene i denne mappa er valfrie forhåndsbygde kopiar for å spare byggetid.
