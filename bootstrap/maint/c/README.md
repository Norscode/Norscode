# Lokal maintainer-output (historisk vedlikehaldsbane)

`bootstrap/maint/c/` er ikkje brukt som aktiv normalflate, og bør normalt vere tomt.
Vedlikehald/regen-output går i staden til `build/maintainer_regen/` som standard.
`norscode_generated.c` er no normalt skrive til `REGEN_ROOT/maint/c/norscode_generated.c`.

```bash
# Maintainer (krev seed i bootstrap/stage0/ eller release):
NORSCODE_BOOTSTRAP_C=1 REGEN=1 bash tools/build_norscode_native.sh

# eller
bash tools/maint/regen_native.sh --rebuild
```

Normal utvikling og CI bruker **ikkje** denne mappa. Normalvegen er `bootstrap/stage0/norscode-*` eller GitHub Release.

Skulle du trenge fastlåste lokale snapshots i denne mappa (t.d. midlertidig):

```bash
REGEN_ROOT=bootstrap/maint/c NORSCODE_BOOTSTRAP_C=1 bash tools/maint/regen_native.sh --rebuild
```
