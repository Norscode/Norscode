#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RUNTIME = ROOT / 'archive/legacy_c_backend/nc_runtime_mini.c'


def sanitize(name: str) -> str:
    out = []
    for ch in name:
        if ch.isalnum():
            out.append(ch)
        else:
            out.append('_')
    return 'nc_fn_' + ''.join(out)


def c_escape(s: str) -> str:
    return s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n').replace('\t', '\\t').replace('\r', '\\r').replace('\0', '\\0')


def emit_push_const(v):
    if v is True:
        return '  nc_push(&sp, stack, nc_bool(1));\n'
    if v is False:
        return '  nc_push(&sp, stack, nc_bool(0));\n'
    if v is None:
        return '  nc_push(&sp, stack, nc_nil());\n'
    if isinstance(v, int):
        return f'  nc_push(&sp, stack, nc_int({v}LL));\n'
    if isinstance(v, str):
        return f'  nc_push(&sp, stack, nc_str("{c_escape(v)}"));\n'
    # fallback: stringify JSON as string
    return f'  nc_push(&sp, stack, nc_str("{c_escape(json.dumps(v, ensure_ascii=False))}"));\n'


def emit_call(instr, ip):
    callee = instr[1]
    nargs = int(instr[2]) if len(instr) > 2 else 0
    suf = str(ip)
    lines = ['  { ']
    for ai in range(nargs - 1, -1, -1):
        lines.append(f'NcVal *_a{suf}_{ai}=nc_pop(&sp,stack); ')
    if nargs > 0:
        args = ','.join(f'_a{suf}_{i}' for i in range(nargs))
        lines.append(f'NcVal *_args_{suf}[{nargs}]={{ {args} }}; ')
        lines.append(f'NcVal *_r_{suf}=nc_dispatch_call("{c_escape(callee)}", _args_{suf}, {nargs}); ')
    else:
        lines.append(f'NcVal *_r_{suf}=nc_dispatch_call("{c_escape(callee)}", NULL, 0); ')
    lines.append(f'nc_push(&sp,stack,_r_{suf}?_r_{suf}:nc_nil()); }}\n')
    return ''.join(lines)


def opcode_to_c(instr, ip):
    op = instr[0]
    if op == 'PUSH_CONST':
        return emit_push_const(instr[1] if len(instr) > 1 else None)
    if op == 'STORE_NAME':
        return f'  nc_store(vars, varnames, &nvars, "{c_escape(str(instr[1]))}", nc_pop(&sp, stack));\n'
    if op == 'LOAD_NAME':
        return f'  nc_push(&sp, stack, nc_load(vars, varnames, nvars, "{c_escape(str(instr[1]))}"));\n'
    if op == 'POP':
        return '  nc_pop(&sp, stack);\n'
    if op == 'RETURN':
        return '  { NcVal *_ret=nc_pop(&sp,stack); int _i; for(_i=0;_i<nvars;_i++) free(varnames[_i]); free(varnames); free(vars); free(stack); return _ret; }\n'
    if op == 'LABEL':
        return f'lbl_{sanitize(str(instr[1]))}:;\n'
    if op == 'JUMP':
        return f'  goto lbl_{sanitize(str(instr[1]))};\n'
    if op == 'JUMP_IF_FALSE':
        return f'  if (!nc_truthy(nc_pop(&sp, stack))) goto lbl_{sanitize(str(instr[1]))};\n'
    binops = {
        'BINARY_ADD': 'nc_add(a,b)', 'BINARY_SUB': 'nc_sub(a,b)', 'BINARY_MUL': 'nc_mul(a,b)',
        'BINARY_DIV': 'nc_div(a,b)', 'BINARY_MOD': 'nc_mod(a,b)', 'BINARY_AND': 'nc_int((a&&a->type==NC_INT?a->i:0)&(b&&b->type==NC_INT?b->i:0))',
        'BINARY_RSHIFT': 'nc_int((a&&a->type==NC_INT?a->i:0)>>(b&&b->type==NC_INT?b->i:0))'
    }
    if op in binops:
        expr = binops[op]
        return f'  {{ NcVal *b=nc_pop(&sp,stack),*a=nc_pop(&sp,stack); nc_push(&sp,stack,{expr}); }}\n'
    if op == 'UNARY_NEG':
        return '  { NcVal *a=nc_pop(&sp,stack); nc_push(&sp,stack,nc_neg(a)); }\n'
    if op == 'UNARY_NOT':
        return '  { NcVal *a=nc_pop(&sp,stack); nc_push(&sp,stack,nc_bool(!nc_truthy(a))); }\n'
    cmps = {
        'COMPARE_EQ': 'nc_bool(nc_eq(a,b))', 'COMPARE_NE': 'nc_bool(!nc_eq(a,b))',
        'COMPARE_LT': 'nc_cmp(a,b,-1)', 'COMPARE_GT': 'nc_cmp(a,b,1)',
        'COMPARE_LE': 'nc_cmp(a,b,-2)', 'COMPARE_GE': 'nc_cmp(a,b,2)'
    }
    if op in cmps:
        return f'  {{ NcVal *b=nc_pop(&sp,stack),*a=nc_pop(&sp,stack); nc_push(&sp,stack,{cmps[op]}); }}\n'
    if op == 'INDEX_SET':
        return '  { NcVal *v=nc_pop(&sp,stack),*k=nc_pop(&sp,stack),*o=nc_pop(&sp,stack); nc_index_set(o,k,v); nc_push(&sp,stack,o); }\n'
    if op == 'CALL':
        return emit_call(instr, ip)
    if op == 'TRY_BEGIN' or op == 'TRY_END':
        return f'  /* {op} unsupported in host C generator */\n'
    if op == 'LOAD_EXCEPTION':
        return '  nc_push(&sp,stack,nc_str("exception"));\n'
    if op == 'THROW':
        return '  { NcVal *e=nc_pop(&sp,stack); nc_throw(nc_to_str_raw(e)); }\n'
    return f'  /* TODO: {c_escape(op)} */\n'


def gen_function(name, fn_def):
    params = fn_def.get('params', [])
    code = fn_def.get('code', [])
    c_name = sanitize(name)
    out = []
    out.append(f'static NcVal *{c_name}(NcVal **args, int nargs) {{\n')
    out.append('  NcVal **stack = calloc(4096,sizeof(NcVal*)); int sp=0;\n')
    out.append('  NcVal **vars = calloc(4096,sizeof(NcVal*)); char **varnames = calloc(4096,sizeof(char*)); int nvars=0;\n')
    for i, p in enumerate(params):
        out.append(f'  nc_store(vars,varnames,&nvars,"{c_escape(str(p))}",nargs>{i}?args[{i}]:nc_nil());\n')
    out.append('\n')
    for ip, instr in enumerate(code):
        out.append(opcode_to_c(instr, ip))
    out.append('  { int _i; for(_i=0;_i<nvars;_i++) free(varnames[_i]); free(varnames); free(vars); free(stack); return nc_nil(); }\n')
    out.append('}\n\n')
    return ''.join(out)


def generate(ncb):
    fns = ncb['functions']
    names = list(fns.keys())
    parts = []
    parts.append('/* Generated by tools/ncb_to_c_host_v9300.py */\n')
    parts.append(RUNTIME.read_text())
    parts.append('\n/* forward decls */\n')
    parts.append('struct NcVal; typedef struct NcVal NcVal;\n')
    parts.append('static NcVal *nc_dispatch_call(const char *n, NcVal **a, int na);\n')
    for nm in names:
        parts.append(f'static NcVal *{sanitize(nm)}(NcVal **args, int nargs);\n')
    parts.append('\n')
    for nm in names:
        parts.append(gen_function(nm, fns[nm]))
    parts.append('typedef struct { const char *name; NcVal *(*fn)(NcVal **, int); } NcDispatch;\n')
    parts.append('static NcDispatch nc_dispatch[] = {\n')
    for nm in names:
        parts.append(f'  {{"{c_escape(nm)}", {sanitize(nm)}}},\n')
    parts.append('  {NULL, NULL}\n};\n')
    return ''.join(parts)


def main():
    if len(sys.argv) != 3:
        print('bruk: ncb_to_c_host_v9300.py input.ncb.json output.c', file=sys.stderr)
        return 1
    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    ncb = json.loads(src.read_text())
    dst.write_text(generate(ncb))
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
