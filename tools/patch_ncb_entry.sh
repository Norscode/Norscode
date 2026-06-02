#!/usr/bin/env bash
# tools/patch_ncb_entry.sh — sett NCB entry-felt (Omgang 6b)
set -euo pipefail

NCB="${1:?bruk: patch_ncb_entry.sh fil.ncb.json entry.funksjon}"
ENTRY="${2:?}"

python3 - "$NCB" "$ENTRY" <<'PY'
import json, sys
path, entry = sys.argv[1], sys.argv[2]
with open(path, encoding="utf-8") as f:
    d = json.load(f)
fns = d.get("functions") or {}
if entry not in fns:
    print(f"Feil: entry {entry!r} finst ikkje i bundle ({len(fns)} funksjonar)", file=sys.stderr)
    sys.exit(1)
d["entry"] = entry
with open(path, "w", encoding="utf-8") as f:
    json.dump(d, f, ensure_ascii=False, separators=(",", ":"))
print(f"  entry → {entry}")
PY
