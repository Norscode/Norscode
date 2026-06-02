# Legacy: NCBB / C minimal VM

**Status (2026-06):** Fysisk `tools/c_minimal_vm/` og `tools/build_norscode_native_from_source.sh` er fjerna frå repoet. Denne mappa er berre dokumentasjon.

## Normalvei i dag

| Treng du | Bruk |
|----------|------|
| Køyre / teste Norscode | `bash tools/build_norscode_native.sh` → `dist/norscode_native`, `./bin/nc` |
| Selvstendighet L1–L6 | `bash tools/verify_selvstendighet.sh` |
| Regenerere uttrykk-fraser i `common.no` | `python3 scripts/gen_expr_fraser.py` (dev, utanfor `tools/`) |

## Historikk

| Tidlegare | Rolle |
|-----------|--------|
| `tools/c_minimal_vm/*.c` | Eldre C-VM / NCBB-loader |
| `tools/build_norscode_native_from_source.sh` | Clang-bygg av c_minimal_vm |
| `Dockerfile.linux-build` | Docker-bygg av same (nå retired stub) |

Kjeldekopiar kan finnast i eldre git-revisjonar om du treng å sammenligne.
