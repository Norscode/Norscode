#!/usr/bin/env python3
"""Generer nc_dispatch.c frå bootstrap/kompiler.ncb.json"""
import json, sys, os

ncb_path = sys.argv[1] if len(sys.argv) > 1 else "bootstrap/kompiler.ncb.json"
d = json.load(open(ncb_path))
fns = d['functions']

def nc_san(n):
    return "nc_fn_" + "".join(
        c if (('a'<=c<='z') or ('A'<=c<='Z') or ('0'<=c<='9')) else '_'
        for b in n.encode('utf-8') for c in chr(b)
    )

lines = ["/* Auto-generert av tools/gen_dispatch.py — IKKJE REDIGER */",
         "typedef struct { const char *name; NcVal *(*fn)(NcVal **, int); } NcDispatch;",""]
for k in fns:
    lines.append(f"static NcVal *{nc_san(k)}(NcVal **args, int nargs);")
lines.extend(["","static NcDispatch nc_dispatch[] = {"])
for k in fns:
    lines.append(f'  {{"{k}", {nc_san(k)}}},')
lines.extend(["  {NULL, NULL}","};","",
    "static NcVal *nc_dispatch_call(const char *n, NcVal **a, int na) {",
    "    for(int i=0;nc_dispatch[i].name;i++) if(!strcmp(nc_dispatch[i].name,n)) return nc_dispatch[i].fn(a,na);",
    "    const char *l=strrchr(n,'.');if(l)l++;else l=n;",
    "    for(int i=0;nc_dispatch[i].name;i++){const char *fl=strrchr(nc_dispatch[i].name,'.');fl=fl?fl+1:nc_dispatch[i].name;if(!strcmp(fl,l))return nc_dispatch[i].fn(a,na);}",
    '    if(!strncmp(n,"builtin.",8)) return nc_dispatch_call(n+8,a,na);',
    '    if(!strncmp(n,"__main__.",9)) return nc_dispatch_call(n+9,a,na);',
    '    {char s2[256];strncpy(s2,l,255);char *t=strstr(s2,"_token");if(t){*t=0;return nc_dispatch_call(s2,a,na);}}',
    "    return NULL;",
    "}",
    "static NcVal *nc_fn_builtin_neste_token(NcVal **a, int na) { return nc_dispatch_call(\"neste\",a,na); }",
])
print('\n'.join(lines))
