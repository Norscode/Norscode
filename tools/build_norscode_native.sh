#!/usr/bin/env bash
# tools/build_norscode_native.sh
# Bygger dist/norscode_native frå:
#   1. bootstrap/kompiler.ncb.json (selfhost-kompilator)
#   2. tools/nc_runtime_mini.c (C-runtime)
#   3. tools/nc_native_main.c (main + executor)
#
# Krev: dist/nc-vm (for å generere C), clang eller cc

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NC_VM="${ROOT}/dist/nc-vm"
OUT="${ROOT}/dist/norscode_native"
CC="${CC:-clang}"
if ! command -v "$CC" >/dev/null 2>&1; then CC=cc; fi

[ -x "$NC_VM" ] || { printf "Feil: dist/nc-vm ikkje funnen\n" >&2; exit 1; }

printf "Genererer C frå bootstrap/kompiler.ncb.json...\n"
NC_NCB_INPUT="${ROOT}/bootstrap/kompiler.ncb.json" \
NC_C_OUTPUT="${ROOT}/dist/norscode_generated.c" \
    "$NC_VM" --nc-run "${ROOT}/selfhost/ncb_to_c.no"

printf "Genererer dispatch-tabell...\n"
python3 - << 'PYEOF'
import json, sys, os
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
d = json.load(open(f"{root}/bootstrap/kompiler.ncb.json"))
fns = d['functions']
def san(n):
    return "nc_fn_" + "".join(c if (('a'<=c<='z') or ('A'<=c<='Z') or ('0'<=c<='9')) else '_' for b in n.encode('utf-8') for c in chr(b))
lines = ["typedef struct { const char *name; NcVal *(*fn)(NcVal **, int); } NcDispatch;",""]
for k in fns: lines.append(f"static NcVal *{san(k)}(NcVal **args, int nargs);")
lines.extend(["","static NcDispatch nc_dispatch[] = {"])
for k in fns: lines.append(f'  {{"{k}", {san(k)}}},')
lines.extend(["  {NULL, NULL}","};","",
    'static NcVal *nc_dispatch_call(const char *name, NcVal **args, int nargs) {',
    '    for(int i=0;nc_dispatch[i].name;i++) if(!strcmp(nc_dispatch[i].name,name)) return nc_dispatch[i].fn(args,nargs);',
    '    const char *last=strrchr(name,\'.\'); if(last) last++; else last=name;',
    '    for(int i=0;nc_dispatch[i].name;i++){const char *fl=strrchr(nc_dispatch[i].name,\'.\');fl=fl?fl+1:nc_dispatch[i].name;if(!strcmp(fl,last))return nc_dispatch[i].fn(args,nargs);}',
    '    if(!strncmp(name,"builtin.",8)) return nc_dispatch_call(name+8,args,nargs);',
    '    if(!strncmp(name,"__main__.",9)) return nc_dispatch_call(name+9,args,nargs);',
    '    {char s2[256];strncpy(s2,last,255);char *t=strstr(s2,"_token");if(t){*t=0;return nc_dispatch_call(s2,args,nargs);}}',
    '    return NULL;',
    '}',
    'static NcVal *nc_fn_builtin_neste_token(NcVal **args, int nargs) { return nc_dispatch_call("neste",args,nargs); }',
])
with open(f"{root}/dist/nc_dispatch.c", 'w') as f:
    f.write('\n'.join(lines))
print(f"Dispatch for {len(fns)} funksjonar")
PYEOF

printf "Kompilerer norscode_native...\n"
TMP=$(mktemp /tmp/nc_full_XXXXXX.c)
cat "${ROOT}/tools/nc_runtime_mini.c" > "$TMP"
cat "${ROOT}/dist/nc_dispatch.c" >> "$TMP"
grep -v '#include.*nc_runtime' "${ROOT}/dist/norscode_generated.c" | \
    sed 's/^int main/static int nc_gen_main/' >> "$TMP"
cat "${ROOT}/tools/nc_native_main.c" >> "$TMP"
"$CC" -O2 -Wno-everything -o "$OUT" "$TMP"
rm -f "$TMP"

printf "✓ dist/norscode_native bygget (%d KB)\n" "$(( $(wc -c < "$OUT") / 1024 ))"
