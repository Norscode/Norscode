# C-kode-avvikling

## Status: tools/nc_vm.c er FJERNA

`tools/nc_vm.c` (2520 linjar C, bytekode-tolkar) er sletta 2026-05-31.

### Erstatning

All funksjonalitet er erstatta av Norscode-native pipeline:

```
selfhost/*.no (Norscode-kjelda)
    ↓  selfhost/ncb_to_c.no (transpiler)
bootstrap/c/norscode_generated.c  (generert C)
    ↓  clang -O2
dist/norscode_native  (native binary, 692KB)
```

### Bootstrap-kjede no

```
bootstrap/c/norscode_generated.c  (committed i git)
bootstrap/c/nc_dispatch.c          (committed i git)
tools/nc_runtime_mini.c            (500-linje C-runtime)
tools/nc_native_main.c             (main + executor)
    ↓  clang (eingong, 4 sekund)
dist/norscode_native               (native binary)
```

### Ytelse vs nc_vm.c

| Operasjon | nc_vm.c | norscode_native |
|-----------|---------|-----------------|
| nc test (48/48) | 4s | 1s |
| bootstrap-gate | 13s | 1.2s |
| build | 0ms (pre-built) | 4s (clang) |
| run program | 2.1s | 12ms |

### Resterande C-filer

| Fil | Linjar | Rolle |
|-----|--------|-------|
| `tools/nc_runtime_mini.c` | 700 | Norscode C-runtime (vals, strenger, JSON) |
| `tools/nc_native_main.c` | 500 | Executor + main() |
| `bootstrap/c/norscode_generated.c` | 16895 | Auto-generert frå Norscode |
| `bootstrap/c/nc_dispatch.c` | 342 | Auto-generert dispatch-tabell |

Desse tre siste vert regenererte av `bin/nc run selfhost/ncb_to_c.no` og
`python3 tools/gen_dispatch.py` og er i git for å unngå bootstrap-problem.

### Neste steg mot C-fri kjøring

1. Erstatt `tools/nc_runtime_mini.c` med Norscode-kode (std-bibliotek)
2. Erstatt `tools/nc_native_main.c` med ein Norscode-main
3. Produser native binær via Norscode ELF-backend (selfhost/compiler/elf_backend.no)
4. Heilt C-fri bootstrap
