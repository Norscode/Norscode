# Generert C (ikkje i git)

`norscode_generated.c` og `nc_dispatch.c` vert laga av `tools/regen_native.sh` frå `.no`-kjelda.

```bash
# Maintainer (krev seed i bootstrap/stage0/ eller release):
REGEN=1 bash tools/build_norscode_native.sh

# eller
bash tools/regen_native.sh --rebuild
```

Normal utvikling og CI bruker **ikkje** denne mappa — berre `bootstrap/stage0/norscode-*` eller GitHub Release.

Engangsmigrering frå committed C til stage0-seed (Linux):

```bash
bash tools/migrate_bootstrap_c_to_stage0.sh
git add bootstrap/stage0/norscode-linux-x86_64
git rm bootstrap/c/norscode_generated.c bootstrap/c/nc_dispatch.c
```
