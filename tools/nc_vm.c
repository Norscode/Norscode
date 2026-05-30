/*
 * nc_vm.c — Norscode native bytecode interpreter
 *
 * Runs pre-compiled .ncb.json bytecode without Python.
 * Supports all 30 opcodes and the core built-in functions.
 *
 * Build: clang -O2 -o dist/nc-vm tools/nc_vm.c
 * Usage: dist/nc-vm <file.ncb.json> [<extra.ncb.json>...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <setjmp.h>
#include <errno.h>
#include <dirent.h>

/* ── Forward declarations ─────────────────────────────────────────────────── */
typedef struct Val Val;
typedef struct Frame Frame;
typedef struct FnDef FnDef;

/* ── Value types ──────────────────────────────────────────────────────────── */
#define T_NIL   0
#define T_BOOL  1
#define T_INT   2
#define T_FLOAT 3
#define T_STR   4
#define T_LIST  5
#define T_MAP   6

typedef struct { Val **items; int len, cap; } List;
typedef struct { char **keys; Val **vals; int len, cap; } Map;

struct Val {
    int type;
    union {
        int        b;
        long long  i;
        double     f;
        char      *s;
        List      *list;
        Map       *map;
    };
};

/* ── Memory helpers (no GC — arena allocations for bootstrap) ─────────────── */
static Val *NIL_VAL;
static Val *TRUE_VAL;
static Val *FALSE_VAL;

static Val *val_alloc(int type) {
    Val *v = calloc(1, sizeof(Val));
    v->type = type;
    return v;
}
static Val *val_nil(void)        { return NIL_VAL; }
static Val *val_bool(int b)      { return b ? TRUE_VAL : FALSE_VAL; }
static Val *val_int(long long i) { Val *v=val_alloc(T_INT);   v->i=i;   return v; }
static Val *val_float(double f)  { Val *v=val_alloc(T_FLOAT); v->f=f;   return v; }
static Val *val_str(const char *s) {
    Val *v=val_alloc(T_STR); v->s=strdup(s); return v;
}
static Val *val_str_own(char *s) { /* takes ownership */
    Val *v=val_alloc(T_STR); v->s=s; return v;
}
static Val *val_list(void) {
    Val *v=val_alloc(T_LIST);
    v->list=calloc(1,sizeof(List)); return v;
}
static Val *val_map(void) {
    Val *v=val_alloc(T_MAP);
    v->map=calloc(1,sizeof(Map)); return v;
}
static int val_truthy(Val *v) {
    if (!v || v->type==T_NIL)  return 0;
    if (v->type==T_BOOL)       return v->b;
    if (v->type==T_INT)        return v->i != 0;
    if (v->type==T_FLOAT)      return v->f != 0.0;
    if (v->type==T_STR)        return v->s && v->s[0];
    if (v->type==T_LIST)       return v->list->len > 0;
    if (v->type==T_MAP)        return v->map->len > 0;
    return 1;
}

/* ── List helpers ─────────────────────────────────────────────────────────── */
static void list_push(List *l, Val *v) {
    if (l->len >= l->cap) {
        l->cap = l->cap ? l->cap*2 : 8;
        l->items = realloc(l->items, l->cap * sizeof(Val*));
    }
    l->items[l->len++] = v;
}
static Val *list_get(List *l, int i) {
    if (i<0) i += l->len;
    if (i<0 || i>=l->len) return NIL_VAL;
    return l->items[i];
}
static void list_set(List *l, int i, Val *v) {
    if (i<0) i += l->len;
    if (i<0 || i>=l->len) return;
    l->items[i] = v;
}

/* ── Map helpers ──────────────────────────────────────────────────────────── */
static Val *map_get(Map *m, const char *k) {
    for (int i=0;i<m->len;i++)
        if (!strcmp(m->keys[i], k)) return m->vals[i];
    return NIL_VAL;
}
static void map_set(Map *m, const char *k, Val *v) {
    for (int i=0;i<m->len;i++)
        if (!strcmp(m->keys[i], k)) { m->vals[i]=v; return; }
    if (m->len >= m->cap) {
        m->cap = m->cap ? m->cap*2 : 8;
        m->keys = realloc(m->keys, m->cap * sizeof(char*));
        m->vals = realloc(m->vals, m->cap * sizeof(Val*));
    }
    m->keys[m->len] = strdup(k);
    m->vals[m->len] = v;
    m->len++;
}
static int map_has(Map *m, const char *k) {
    for (int i=0;i<m->len;i++)
        if (!strcmp(m->keys[i], k)) return 1;
    return 0;
}

/* ── Value to string ──────────────────────────────────────────────────────── */
static char *val_to_str(Val *v) {
    if (!v || v->type==T_NIL)  return strdup("ingenting");
    if (v->type==T_BOOL)       return strdup(v->b ? "sant" : "usant");
    if (v->type==T_STR)        return strdup(v->s);
    if (v->type==T_INT) {
        char buf[64]; snprintf(buf,sizeof(buf),"%lld",v->i); return strdup(buf);
    }
    if (v->type==T_FLOAT) {
        char buf[64]; snprintf(buf,sizeof(buf),"%g",v->f); return strdup(buf);
    }
    if (v->type==T_LIST) {
        /* simple repr */
        char *out = strdup("[");
        for (int i=0;i<v->list->len;i++) {
            char *s = val_to_str(v->list->items[i]);
            size_t nl = strlen(out)+strlen(s)+3;
            out = realloc(out, nl);
            if (i) strcat(out, ", ");
            strcat(out, s); free(s);
        }
        out = realloc(out, strlen(out)+2);
        strcat(out, "]"); return out;
    }
    if (v->type==T_MAP) return strdup("{...}");
    return strdup("?");
}

/* ── Error handling ───────────────────────────────────────────────────────── */
static jmp_buf g_err_jmp;
static char    g_err_msg[4096];
static Val    *g_exception = NULL;   /* current thrown value */

static void nc_panic(const char *fmt, ...) __attribute__((noreturn));
static void nc_panic(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(g_err_msg, sizeof(g_err_msg), fmt, ap);
    va_end(ap);
    longjmp(g_err_jmp, 1);
}

/* ── Minimal JSON parser ──────────────────────────────────────────────────── */
typedef struct { const char *p; int line; } JP;

static void jp_skip_ws(JP *j) {
    while (*j->p==' '||*j->p=='\t'||*j->p=='\r'||*j->p=='\n') {
        if (*j->p=='\n') j->line++;
        j->p++;
    }
}
static Val *jp_parse(JP *j);

static char *jp_str(JP *j) {
    if (*j->p != '"') nc_panic("JSON: forventet '\"' linje %d", j->line);
    j->p++;
    char buf[65536]; int bi=0;
    while (*j->p && *j->p!='"') {
        if (*j->p=='\\') {
            j->p++;
            switch(*j->p) {
                case '"': buf[bi++]='"'; break;
                case '\\':buf[bi++]='\\';break;
                case '/': buf[bi++]='/'; break;
                case 'n': buf[bi++]='\n';break;
                case 'r': buf[bi++]='\r';break;
                case 't': buf[bi++]='\t';break;
                case 'u': {
                    /* skip 4 hex digits */
                    unsigned cp=0;
                    for(int k=0;k<4;k++){j->p++;cp=cp*16+((*j->p>='0'&&*j->p<='9')?*j->p-'0':(*j->p>='a'&&*j->p<='f')?*j->p-'a'+10:*j->p-'A'+10);}
                    /* encode as UTF-8 */
                    if(cp<0x80)buf[bi++]=cp;
                    else if(cp<0x800){buf[bi++]=0xC0|(cp>>6);buf[bi++]=0x80|(cp&0x3F);}
                    else{buf[bi++]=0xE0|(cp>>12);buf[bi++]=0x80|((cp>>6)&0x3F);buf[bi++]=0x80|(cp&0x3F);}
                    break;
                }
                default: buf[bi++]='\\'; buf[bi++]=*j->p;
            }
        } else {
            if (*j->p=='\n') j->line++;
            buf[bi++]=*j->p;
        }
        j->p++;
        if (bi >= (int)sizeof(buf)-4) nc_panic("JSON: streng for lang linje %d", j->line);
    }
    if (*j->p=='"') j->p++;
    buf[bi]=0;
    return strdup(buf);
}

static Val *jp_parse(JP *j) {
    jp_skip_ws(j);
    if (*j->p=='"') {
        char *s = jp_str(j);
        return val_str_own(s);
    }
    if (*j->p=='{') {
        j->p++; Val *v=val_map(); jp_skip_ws(j);
        while (*j->p && *j->p!='}') {
            if (*j->p==',') { j->p++; jp_skip_ws(j); continue; }
            char *k = jp_str(j); jp_skip_ws(j);
            if (*j->p!=':') nc_panic("JSON: forventet ':' linje %d", j->line);
            j->p++; jp_skip_ws(j);
            Val *val = jp_parse(j); jp_skip_ws(j);
            map_set(v->map, k, val); free(k);
        }
        if (*j->p=='}') j->p++;
        return v;
    }
    if (*j->p=='[') {
        j->p++; Val *v=val_list(); jp_skip_ws(j);
        while (*j->p && *j->p!=']') {
            if (*j->p==',') { j->p++; jp_skip_ws(j); continue; }
            list_push(v->list, jp_parse(j)); jp_skip_ws(j);
        }
        if (*j->p==']') j->p++;
        return v;
    }
    if (strncmp(j->p,"true",4)==0)  { j->p+=4; return val_bool(1); }
    if (strncmp(j->p,"false",5)==0) { j->p+=5; return val_bool(0); }
    if (strncmp(j->p,"null",4)==0)  { j->p+=4; return val_nil(); }
    /* number */
    char *end; long long iv = strtoll(j->p, &end, 10);
    if (end != j->p) {
        if (*end=='.' || *end=='e' || *end=='E') {
            double fv = strtod(j->p, &end);
            j->p = end; return val_float(fv);
        }
        j->p = end; return val_int(iv);
    }
    nc_panic("JSON: uventet tegn '%c' linje %d", *j->p, j->line);
}

static Val *json_load_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) nc_panic("Kan ikke åpne: %s (%s)", path, strerror(errno));
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz+1);
    fread(buf, 1, sz, f); fclose(f); buf[sz]=0;
    JP j = {buf, 1};
    Val *v = jp_parse(&j);
    free(buf);
    return v;
}

/* ── Function table ───────────────────────────────────────────────────────── */
#define MAX_FNS 8192
typedef struct FnDef {
    char  *name;
    Val  **params;      /* array of str vals */
    int    nparam;
    Val  **code;        /* array of instruction vals (each a list) */
    int    ncode;
} FnDef;

static FnDef  g_fns[MAX_FNS];
static int    g_nfns = 0;

/* Alternate names for cross-module lookup */
static char  *g_fn_altnames[MAX_FNS];  /* module-qualified alias */

static FnDef *fn_find(const char *name) {
    /* 1. Direct match (including module-qualified names stored as altnames) */
    for (int i=0;i<g_nfns;i++) {
        if (!strcmp(g_fns[i].name, name)) return &g_fns[i];
        if (g_fn_altnames[i] && !strcmp(g_fn_altnames[i], name)) return &g_fns[i];
    }
    /* 2. Strip __main__. from both sides */
    const char *n = name;
    if (strncmp(n, "__main__.", 9)==0) n += 9;
    for (int i=0;i<g_nfns;i++) {
        const char *fn = g_fns[i].name;
        if (strncmp(fn,"__main__.",9)==0) fn+=9;
        if (!strcmp(fn, n)) return &g_fns[i];
    }
    /* 3. Last segment match: selfhost.vm.køyr_ncb → look for any fn named køyr_ncb */
    const char *last = strrchr(name, '.');
    if (last) {
        last++;
        for (int i=0;i<g_nfns;i++) {
            const char *fn = g_fns[i].name;
            if (strncmp(fn,"__main__.",9)==0) fn+=9;
            if (!strcmp(fn, last)) return &g_fns[i];
        }
    }
    return NULL;
}

static void fn_register(const char *name, Val *params_arr, Val *code_arr) {
    if (g_nfns >= MAX_FNS) nc_panic("For mange funksjoner");
    FnDef *d = &g_fns[g_nfns];
    g_fn_altnames[g_nfns] = NULL;
    g_nfns++;
    d->name = strdup(name);
    /* params */
    d->nparam = params_arr ? params_arr->list->len : 0;
    d->params = calloc(d->nparam, sizeof(Val*));
    for (int i=0;i<d->nparam;i++)
        d->params[i] = params_arr->list->items[i];
    /* code */
    d->ncode = code_arr ? code_arr->list->len : 0;
    d->code = calloc(d->ncode, sizeof(Val*));
    for (int i=0;i<d->ncode;i++)
        d->code[i] = code_arr->list->items[i];
}

/* Derive module path from file path, e.g.:
   "build/selfhost-whole/selfhost/vm.ncb.json" → "selfhost.vm"
   "build/selfhost-whole/selfhost/compiler/ir.ncb.json" → "selfhost.compiler.ir"
*/
static void derive_module_path(const char *filepath, char *out, int outsz) {
    /* Find "selfhost/" in the path */
    const char *p = strstr(filepath, "selfhost/");
    if (!p) { out[0]=0; return; }
    /* Copy from "selfhost/" to end, replacing / with . and stripping .ncb.json */
    char tmp[512];
    strncpy(tmp, p, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1]=0;
    /* strip .ncb.json suffix */
    char *suf = strstr(tmp, ".ncb.json");
    if (suf) *suf=0;
    /* replace / with . */
    for (char *c=tmp; *c; c++) if(*c=='/') *c='.';
    strncpy(out, tmp, outsz-1);
    out[outsz-1]=0;
}

static void load_ncb(const char *path) {
    Val *data = json_load_file(path);
    if (data->type != T_MAP) nc_panic("ncb-fil har ingen map: %s", path);
    Val *fns_val = map_get(data->map, "functions");
    if (!fns_val || fns_val->type != T_MAP) return;

    /* Derive module path for cross-module lookup */
    char modpath[512];
    derive_module_path(path, modpath, sizeof(modpath));

    Map *fns = fns_val->map;
    for (int i=0;i<fns->len;i++) {
        Val *fn = fns->vals[i];
        if (!fn || fn->type != T_MAP) continue;
        Val *params = map_get(fn->map, "params");
        Val *code   = map_get(fn->map, "code");
        int idx_before = g_nfns;
        fn_register(fns->keys[i], params, code);
        /* Register module-qualified alias */
        if (modpath[0]) {
            const char *bare = fns->keys[i];
            if (strncmp(bare,"__main__.",9)==0) bare+=9;
            char altname[512];
            snprintf(altname,sizeof(altname),"%s.%s",modpath,bare);
            g_fn_altnames[idx_before] = strdup(altname);
        }
    }
}

/* Load NCB from an in-memory Val (already parsed JSON), with given module path */
static void load_ncb_val(Val *data, const char *modpath) {
    if (!data || data->type != T_MAP) return;
    Val *fns_val = map_get(data->map, "functions");
    if (!fns_val || fns_val->type != T_MAP) return;
    Map *fns = fns_val->map;
    for (int i = 0; i < fns->len; i++) {
        Val *fn = fns->vals[i];
        if (!fn || fn->type != T_MAP) continue;
        Val *params = map_get(fn->map, "params");
        Val *code   = map_get(fn->map, "code");
        int idx_before = g_nfns;
        fn_register(fns->keys[i], params, code);
        if (modpath && modpath[0]) {
            const char *bare = fns->keys[i];
            if (strncmp(bare,"__main__.",9)==0) bare+=9;
            char altname[512];
            snprintf(altname, sizeof(altname), "%s.%s", modpath, bare);
            g_fn_altnames[idx_before] = strdup(altname);
        }
    }
}

/* Parse a JSON string (from a Val) and return the Val tree */
static Val *json_parse_str(const char *s) {
    JP j = {s, 1};
    return jp_parse(&j);
}

/* Load all *.ncb.json from a directory */
/* Comparison helper for qsort */
static int _cmp_str(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

static void load_ncb_dir(const char *dirpath) {
    DIR *d = opendir(dirpath);
    if (!d) return;
    /* Collect all .ncb.json filenames, then sort alphabetically */
    char **names = NULL;
    int nnames = 0, cap = 0;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (!strstr(e->d_name, ".ncb.json")) continue;
        if (nnames >= cap) {
            cap = cap ? cap*2 : 64;
            names = realloc(names, cap*sizeof(char*));
        }
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", dirpath, e->d_name);
        names[nnames++] = strdup(full);
    }
    closedir(d);
    if (!names) return;
    qsort(names, nnames, sizeof(char*), _cmp_str);
    for (int i=0; i<nnames; i++) {
        load_ncb(names[i]);
        free(names[i]);
    }
    free(names);
}

/* ── Execution stack & frame ──────────────────────────────────────────────── */
typedef struct Frame {
    FnDef  *fn;
    int     ip;
    /* variable bindings (dynamisk) */
    char  **var_names;
    Val   **var_vals;
    int     nvar, var_cap;
    /* value stack (dynamisk) */
    Val   **stack;
    int     sp, stack_cap;
    /* try/catch */
    jmp_buf try_jmp;
    int     try_catch_label;
    int     in_try;
} Frame;

#define MAX_CALL_DEPTH 32768
static Frame *g_frames[MAX_CALL_DEPTH];
static int    g_depth = 0;

static Val *frame_get(Frame *f, const char *k) {
    for (int i=f->nvar-1;i>=0;i--)
        if (!strcmp(f->var_names[i], k)) return f->var_vals[i];
    return NIL_VAL;
}
static void frame_set(Frame *f, const char *k, Val *v) {
    for (int i=f->nvar-1;i>=0;i--)
        if (!strcmp(f->var_names[i], k)) { f->var_vals[i]=v; return; }
    if (f->nvar >= f->var_cap) {
        f->var_cap = f->var_cap ? f->var_cap*2 : 16;
        f->var_names = realloc(f->var_names, f->var_cap*sizeof(char*));
        f->var_vals  = realloc(f->var_vals,  f->var_cap*sizeof(Val*));
    }
    f->var_names[f->nvar] = strdup(k);
    f->var_vals [f->nvar] = v;
    f->nvar++;
}
static void push(Frame *f, Val *v) {
    if (f->sp >= f->stack_cap) {
        f->stack_cap = f->stack_cap ? f->stack_cap*2 : 64;
        f->stack = realloc(f->stack, f->stack_cap*sizeof(Val*));
    }
    f->stack[f->sp++] = v;
}
static Val *pop(Frame *f) {
    if (f->sp <= 0) nc_panic("Stack underflow");
    return f->stack[--f->sp];
}
static Val *peek(Frame *f) {
    if (f->sp <= 0) nc_panic("Stack peek underflow");
    return f->stack[f->sp-1];
}

/* ── Jump label resolution ────────────────────────────────────────────────── */
static int find_label(FnDef *fn, const char *label) {
    for (int i=0;i<fn->ncode;i++) {
        Val *instr = fn->code[i];
        if (!instr || instr->type!=T_LIST || instr->list->len<1) continue;
        Val *op = instr->list->items[0];
        if (op->type==T_STR && !strcmp(op->s,"LABEL")) {
            if (instr->list->len>=2) {
                Val *lbl = instr->list->items[1];
                if (lbl->type==T_STR && !strcmp(lbl->s, label))
                    return i;
            }
        }
    }
    return -1;
}

/* ── Built-in JSON emit ───────────────────────────────────────────────────── */
static char *json_emit(Val *v);
static char *json_emit_list(List *l) {
    char *out=strdup("[");
    for(int i=0;i<l->len;i++){
        char *s=json_emit(l->items[i]);
        out=realloc(out,strlen(out)+strlen(s)+3);
        if(i)strcat(out,",");
        strcat(out,s);free(s);
    }
    out=realloc(out,strlen(out)+2);strcat(out,"]");return out;
}
static char *json_emit_str(const char *s) {
    char *out=malloc(strlen(s)*6+4);
    char *p=out; *p++='"';
    while(*s){
        if(*s=='"'||*s=='\\'){*p++='\\';*p++=*s;}
        else if(*s=='\n'){*p++='\\';*p++='n';}
        else if(*s=='\r'){*p++='\\';*p++='r';}
        else if(*s=='\t'){*p++='\\';*p++='t';}
        else *p++=*s;
        s++;
    }
    *p++='"';*p=0;return out;
}
static char *json_emit(Val *v) {
    if(!v||v->type==T_NIL) return strdup("null");
    if(v->type==T_BOOL) return strdup(v->b?"true":"false");
    if(v->type==T_INT){char b[32];snprintf(b,sizeof(b),"%lld",v->i);return strdup(b);}
    if(v->type==T_FLOAT){char b[64];snprintf(b,sizeof(b),"%g",v->f);return strdup(b);}
    if(v->type==T_STR) return json_emit_str(v->s);
    if(v->type==T_LIST) return json_emit_list(v->list);
    if(v->type==T_MAP){
        char *out=strdup("{");
        for(int i=0;i<v->map->len;i++){
            char *k=json_emit_str(v->map->keys[i]);
            char *val=json_emit(v->map->vals[i]);
            out=realloc(out,strlen(out)+strlen(k)+strlen(val)+4);
            if(i)strcat(out,",");
            strcat(out,k);strcat(out,":");strcat(out,val);
            free(k);free(val);
        }
        out=realloc(out,strlen(out)+2);strcat(out,"}");return out;
    }
    return strdup("null");
}

/* ── Forward: call_fn ─────────────────────────────────────────────────────── */
static Val *call_fn(const char *name, Val **args, int nargs);

/* ── Built-in functions ───────────────────────────────────────────────────── */
static Val *builtin_call(const char *name, Val **args, int nargs) {
    /* Strip module prefixes: builtin.X, __main__.X, selfhost.*.X */
    const char *n = name;
    if (strncmp(n,"builtin.",8)==0) n+=8;
    else if (strncmp(n,"__main__.",9)==0) n+=9;
    else {
        /* selfhost.vm.legg_til → legg_til (take last segment) */
        const char *last = strrchr(n, '.');
        if (last) n = last+1;
    }

    if (!strcmp(n,"skriv") || !strcmp(n,"print")) {
        for(int i=0;i<nargs;i++){
            char *s=val_to_str(args[i]);
            fwrite(s,1,strlen(s),stdout); free(s);
            if(i<nargs-1) fwrite(" ",1,1,stdout);
        }
        fwrite("\n",1,1,stdout); fflush(stdout); return val_nil();
    }
    if (!strcmp(n,"lengde") || !strcmp(n,"len")) {
        if(nargs<1) return val_int(0);
        Val *v=args[0];
        if(v->type==T_STR)  return val_int(strlen(v->s));
        if(v->type==T_LIST) return val_int(v->list->len);
        if(v->type==T_MAP)  return val_int(v->map->len);
        return val_int(0);
    }
    if (!strcmp(n,"legg_til") || !strcmp(n,"append")) {
        if(nargs>=2 && args[0]->type==T_LIST)
            list_push(args[0]->list, args[1]);
        return val_nil();
    }
    if (!strcmp(n,"fjern_siste") || !strcmp(n,"pop")) {
        if(nargs>=1 && args[0]->type==T_LIST && args[0]->list->len>0)
            return args[0]->list->items[--args[0]->list->len];
        return val_nil();
    }
    if (!strcmp(n,"har_nokkel") || !strcmp(n,"finnes_nøkkel") || !strcmp(n,"has_key")) {
        if(nargs>=2 && args[0]->type==T_MAP){
            char *k=val_to_str(args[1]);
            int r=map_has(args[0]->map,k);free(k);
            return val_bool(r);
        }
        return val_bool(0);
    }
    if (!strcmp(n,"nøkler") || !strcmp(n,"keys")) {
        Val *out=val_list();
        if(nargs>=1 && args[0]->type==T_MAP)
            for(int i=0;i<args[0]->map->len;i++)
                list_push(out->list, val_str(args[0]->map->keys[i]));
        return out;
    }
    if (!strcmp(n,"verdier") || !strcmp(n,"values")) {
        Val *out=val_list();
        if(nargs>=1 && args[0]->type==T_MAP)
            for(int i=0;i<args[0]->map->len;i++)
                list_push(out->list, args[0]->map->vals[i]);
        return out;
    }
    if (!strcmp(n,"tekst_fra_heltall") || !strcmp(n,"str") || !strcmp(n,"til_tekst") || !strcmp(n,"tekst")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]); Val *r=val_str(s); free(s); return r;
    }
    if (!strcmp(n,"heltall") || !strcmp(n,"heltall_fra_tekst") || !strcmp(n,"int")) {
        if(nargs<1) return val_int(0);
        if(args[0]->type==T_INT) return args[0];
        if(args[0]->type==T_STR) return val_int(atoll(args[0]->s));
        if(args[0]->type==T_FLOAT) return val_int((long long)args[0]->f);
        return val_int(0);
    }
    if (!strcmp(n,"bool")) {
        if(nargs<1) return val_bool(0);
        return val_bool(val_truthy(args[0]));
    }
    if (!strcmp(n,"type_av") || !strcmp(n,"type")) {
        if(nargs<1) return val_str("ingenting");
        const char *t[]= {"ingenting","boolsk","heltall","desimal","tekst","liste","ordbok"};
        return val_str(args[0]->type<=6?t[args[0]->type]:"ukjent");
    }
    if (!strcmp(n,"slice")) {
        if(nargs<3) return val_nil();
        Val *src=args[0]; long long a=0,b=0;
        if(args[1]->type==T_INT) a=args[1]->i;
        if(args[2]->type==T_INT) b=args[2]->i;
        if(src->type==T_STR){
            long long l=strlen(src->s);
            if(a<0)a+=l; if(b<0)b+=l;
            if(a<0)a=0; if(b>l)b=l;
            if(a>b) return val_str("");
            char *s=malloc(b-a+1);memcpy(s,src->s+a,b-a);s[b-a]=0;
            return val_str_own(s);
        }
        if(src->type==T_LIST){
            long long l=src->list->len;
            if(a<0)a+=l; if(b<0)b+=l;
            if(a<0)a=0; if(b>l)b=l;
            Val *out=val_list();
            for(long long i=a;i<b;i++) list_push(out->list, list_get(src->list,(int)i));
            return out;
        }
        return val_nil();
    }
    if (!strcmp(n,"fjern") || !strcmp(n,"slett_nøkkel")) {
        /* dict or list remove */
        if(nargs>=2 && args[0]->type==T_MAP){
            char *k=val_to_str(args[1]);
            Map *m=args[0]->map;
            for(int i=0;i<m->len;i++)
                if(!strcmp(m->keys[i],k)){
                    free(m->keys[i]);
                    memmove(&m->keys[i],&m->keys[i+1],(m->len-i-1)*sizeof(char*));
                    memmove(&m->vals[i],&m->vals[i+1],(m->len-i-1)*sizeof(Val*));
                    m->len--; break;
                }
            free(k);
        }
        return val_nil();
    }
    if (!strcmp(n,"fjern_indeks")) {
        if(nargs>=2 && args[0]->type==T_LIST){
            long long idx=args[1]->type==T_INT?args[1]->i:0;
            List *l=args[0]->list;
            if(idx<0)idx+=l->len;
            if(idx>=0&&idx<l->len){
                memmove(&l->items[idx],&l->items[idx+1],(l->len-idx-1)*sizeof(Val*));
                l->len--;
            }
        }
        return val_nil();
    }
    if (!strcmp(n,"pop_siste")) {
        if(nargs>=1 && args[0]->type==T_LIST && args[0]->list->len>0)
            return args[0]->list->items[--args[0]->list->len];
        return val_nil();
    }
    if (!strcmp(n,"finnes")) {
        if(nargs>=2){
            if(args[0]->type==T_MAP){
                char *k=val_to_str(args[1]);
                int r=map_has(args[0]->map,k);free(k);return val_bool(r);
            }
            if(args[0]->type==T_LIST){
                for(int i=0;i<args[0]->list->len;i++){
                    Val *item=args[0]->list->items[i];
                    if(item->type==T_STR&&args[1]->type==T_STR&&!strcmp(item->s,args[1]->s))return val_bool(1);
                    if(item->type==T_INT&&args[1]->type==T_INT&&item->i==args[1]->i)return val_bool(1);
                }
                return val_bool(0);
            }
        }
        return val_bool(0);
    }
    if (!strcmp(n,"inneholder")) {
        /* string contains or list contains */
        if(nargs>=2&&args[0]->type==T_STR&&args[1]->type==T_STR)
            return val_bool(strstr(args[0]->s,args[1]->s)!=NULL);
        return builtin_call("finnes", args, nargs);
    }
    if (!strcmp(n,"delstreng")) {
        if(nargs>=3) return builtin_call("slice",args,nargs);
        return val_nil();
    }
    if (!strcmp(n,"json_stringify")) {
        if(nargs<1) return val_str("null");
        char *s=json_emit(args[0]); Val *r=val_str(s); free(s); return r;
    }
    if (!strcmp(n,"json_parse")) {
        if(nargs<1) return val_nil();
        char *src=val_to_str(args[0]);
        JP j={src,1};
        Val *r = jp_parse(&j);
        free(src); return r;
    }
    if (!strcmp(n,"fil_les")) {
        if(nargs<1) return val_nil();
        char *path=val_to_str(args[0]);
        FILE *f=fopen(path,"rb");free(path);
        if(!f) return val_nil();
        fseek(f,0,SEEK_END);long sz=ftell(f);rewind(f);
        char *buf=malloc(sz+1);fread(buf,1,sz,f);fclose(f);buf[sz]=0;
        return val_str_own(buf);
    }
    if (!strcmp(n,"fil_skriv")) {
        if(nargs<2) return val_nil();
        char *path=val_to_str(args[0]);
        char *content=val_to_str(args[1]);
        FILE *f=fopen(path,"wb");
        if(f){fwrite(content,1,strlen(content),f);fclose(f);}
        free(path);free(content);
        return val_nil();
    }
    if (!strcmp(n,"fil_finnes")) {
        if(nargs<1) return val_bool(0);
        char *path=val_to_str(args[0]);
        FILE *f=fopen(path,"rb");
        int r=(f!=NULL);if(f)fclose(f);
        free(path); return val_bool(r);
    }
    if (!strcmp(n,"miljo_hent")) {
        if(nargs<1) return val_str("");
        char *k=val_to_str(args[0]);
        char *v=getenv(k);free(k);
        return v?val_str(v):val_str("");
    }
    if (!strcmp(n,"miljo_finnes")) {
        if(nargs<1) return val_bool(0);
        char *k=val_to_str(args[0]);
        int r=(getenv(k)!=NULL);free(k);
        return val_bool(r);
    }
    if (!strcmp(n,"stopp") || !strcmp(n,"exit")) {
        int code = (nargs>=1 && args[0]->type==T_INT) ? (int)args[0]->i : 0;
        exit(code);
    }
    if (!strcmp(n,"feil") || !strcmp(n,"panic")) {
        if(nargs>=1){char *s=val_to_str(args[0]);nc_panic("Norscode feil: %s",s);}
        nc_panic("Norscode feil (ukjent)");
    }
    if (!strcmp(n,"skriv_linje")) {
        for(int i=0;i<nargs;i++){char *s=val_to_str(args[i]);printf("%s",s);free(s);}
        printf("\n"); return val_nil();
    }
    if (!strcmp(n,"kjør") || !strcmp(n,"run")) {
        /* Call a function by name stored in args[0] */
        if(nargs>=1 && args[0]->type==T_STR)
            return call_fn(args[0]->s, args+1, nargs-1);
        return val_nil();
    }
    /* Struct constructor: if name starts with uppercase, return empty map */
    if (n[0] >= 'A' && n[0] <= 'Z') {
        return val_map();  /* struct instance = empty map */
    }
    /* Constructor pattern: ny_* returns empty map */
    if (strncmp(n, "ny_", 3)==0) {
        return val_map();
    }
    /* Validator/void pattern: analyser_*, valider_*, sjekk_* return nil */
    if (strncmp(n,"analyser_",9)==0 || strncmp(n,"valider_",8)==0 ||
        strncmp(n,"sjekk_",6)==0) {
        return val_nil();
    }
    /* forvent(), feil(), stopp_*() — common selfhost helpers */
    if (!strcmp(n,"forvent")) {
        /* forvent(ctx, token_type, msg) — returns token, simplified: return second arg */
        return (nargs>=2) ? args[1] : val_nil();
    }
    if (!strcmp(n,"runtime_panic") || !strcmp(n,"runtime_feil")) {
        if(nargs>=1){char *s=val_to_str(args[0]);nc_panic("Runtime feil: %s",s);}
        nc_panic("Runtime feil");
    }
    if (!strcmp(n,"runtime_banner") || !strcmp(n,"runtime_execute") ||
        !strcmp(n,"runtime_execute_check") || !strcmp(n,"skriv_linje_logg")) {
        return val_nil();
    }
    /* ── Streng-verktøy ──────────────────────────────────────────────── */
    if (!strcmp(n,"tekst_trim") || !strcmp(n,"trim")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]);
        int a=0,b=(int)strlen(s);
        while(a<b && (s[a]==' '||s[a]=='\t'||s[a]=='\n'||s[a]=='\r')) a++;
        while(b>a && (s[b-1]==' '||s[b-1]=='\t'||s[b-1]=='\n'||s[b-1]=='\r')) b--;
        char *r=malloc(b-a+1); memcpy(r,s+a,b-a); r[b-a]=0;
        free(s); return val_str_own(r);
    }
    if (!strcmp(n,"tekst_starter_med") || !strcmp(n,"starts_with") || !strcmp(n,"startsWith")) {
        if(nargs<2) return val_bool(0);
        char *s=val_to_str(args[0]); char *p=val_to_str(args[1]);
        int r=strncmp(s,p,strlen(p))==0; free(s);free(p); return val_bool(r);
    }
    if (!strcmp(n,"tekst_slutter_med") || !strcmp(n,"ends_with") || !strcmp(n,"endsWith")) {
        if(nargs<2) return val_bool(0);
        char *s=val_to_str(args[0]); char *p=val_to_str(args[1]);
        size_t sl=strlen(s),pl=strlen(p);
        int r=(sl>=pl && strcmp(s+sl-pl,p)==0); free(s);free(p); return val_bool(r);
    }
    if (!strcmp(n,"tekst_del") || !strcmp(n,"substring") || !strcmp(n,"tekst_utdrag")) {
        /* tekst_del(s, start[, end]) */
        if(nargs<2) return val_str("");
        char *s=val_to_str(args[0]); long long l=strlen(s);
        long long a=args[1]->type==T_INT?args[1]->i:0;
        long long b=(nargs>=3&&args[2]->type==T_INT)?args[2]->i:l;
        if(a<0)a+=l; if(b<0)b+=l;
        if(a<0)a=0; if(b>l)b=l; if(a>b)a=b;
        char *r=malloc(b-a+1); memcpy(r,s+a,b-a); r[b-a]=0;
        free(s); return val_str_own(r);
    }
    if (!strcmp(n,"tekst_splitt") || !strcmp(n,"split")) {
        Val *out=val_list();
        if(nargs<2){if(nargs>=1){list_push(out->list,args[0]);}return out;}
        char *s=val_to_str(args[0]); char *sep=val_to_str(args[1]);
        size_t seplen=strlen(sep); char *cur=s;
        if(seplen==0){for(size_t i=0;i<strlen(s);i++){char c[2]={s[i],0};list_push(out->list,val_str(c));}free(s);free(sep);return out;}
        char *pos;
        while((pos=strstr(cur,sep))!=NULL){
            int n2=(int)(pos-cur); char *part=malloc(n2+1); memcpy(part,cur,n2); part[n2]=0;
            list_push(out->list,val_str_own(part)); cur=pos+seplen;
        }
        list_push(out->list,val_str(cur));
        free(s);free(sep); return out;
    }
    if (!strcmp(n,"tekst_erstatt") || !strcmp(n,"replace")) {
        if(nargs<3) return nargs>=1?args[0]:val_str("");
        char *s=val_to_str(args[0]); char *old=val_to_str(args[1]); char *new2=val_to_str(args[2]);
        size_t olen=strlen(old),nlen=strlen(new2);
        if(olen==0){free(old);free(new2);free(s);return val_str(s);}
        /* Count occurrences */
        int cnt=0; char *p=s;
        while((p=strstr(p,old))!=NULL){cnt++;p+=olen;}
        size_t rlen=strlen(s)+(nlen-olen)*cnt;
        char *r=malloc(rlen+1),*rp=r; p=s;
        char *q;
        while((q=strstr(p,old))!=NULL){
            size_t pre=q-p; memcpy(rp,p,pre);rp+=pre;
            memcpy(rp,new2,nlen);rp+=nlen; p=q+olen;
        }
        strcpy(rp,p); free(s);free(old);free(new2); return val_str_own(r);
    }
    if (!strcmp(n,"tekst_indeks") || !strcmp(n,"index_of")) {
        if(nargs<2) return val_int(-1);
        char *s=val_to_str(args[0]); char *p=val_to_str(args[1]);
        char *pos=strstr(s,p);
        long long r=pos?(long long)(pos-s):-1;
        free(s);free(p); return val_int(r);
    }
    if (!strcmp(n,"tekst_til_store") || !strcmp(n,"to_upper") || !strcmp(n,"upper")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]);
        for(char *p=s;*p;p++) if(*p>='a'&&*p<='z')*p-=32;
        return val_str_own(s);
    }
    if (!strcmp(n,"tekst_til_små") || !strcmp(n,"to_lower") || !strcmp(n,"lower")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]);
        for(char *p=s;*p;p++) if(*p>='A'&&*p<='Z')*p+=32;
        return val_str_own(s);
    }
    if (!strcmp(n,"tekst_join") || !strcmp(n,"join")) {
        if(nargs<2||args[0]->type!=T_LIST) return val_str("");
        char *sep=val_to_str(args[1]);
        size_t total=0; int len=args[0]->list->len;
        char **parts=malloc(len*sizeof(char*));
        for(int i=0;i<len;i++){parts[i]=val_to_str(args[0]->list->items[i]);total+=strlen(parts[i]);}
        if(len>1) total+=strlen(sep)*(len-1);
        char *r=malloc(total+1),*rp=r;
        for(int i=0;i<len;i++){strcpy(rp,parts[i]);rp+=strlen(parts[i]);if(i<len-1){strcpy(rp,sep);rp+=strlen(sep);}free(parts[i]);}
        *rp=0; free(parts);free(sep); return val_str_own(r);
    }
    if (!strcmp(n,"tekst_inneholder") || !strcmp(n,"contains")) {
        if(nargs<2) return val_bool(0);
        char *s=val_to_str(args[0]); char *p=val_to_str(args[1]);
        int r=strstr(s,p)!=NULL; free(s);free(p); return val_bool(r);
    }
    /* Unknown built-in — return nil and warn */
    fprintf(stderr, "[nc-vm] ukjent: %s\n", name);
    return val_nil();
}

/* ── Core interpreter ─────────────────────────────────────────────────────── */
static Val *exec_fn(FnDef *fn, Val **args, int nargs);

static Val *call_fn(const char *name, Val **args, int nargs) {
    /* Try user-defined first */
    FnDef *d = fn_find(name);
    if (d) return exec_fn(d, args, nargs);
    /* Try built-in */
    return builtin_call(name, args, nargs);
}

static Val *exec_fn(FnDef *fn, Val **args, int nargs) {
    if (g_depth >= MAX_CALL_DEPTH)
        nc_panic("Maksimal rekursjonsdybde nådd (%d)", MAX_CALL_DEPTH);

    Frame *f = calloc(1, sizeof(Frame));
    f->fn = fn;
    f->ip = 0;
    g_frames[g_depth++] = f;

    /* Bind parameters */
    for (int i=0; i<fn->nparam && i<nargs; i++) {
        const char *pname = fn->params[i]->type==T_STR ? fn->params[i]->s : "?";
        frame_set(f, pname, args[i]);
    }

    Val *result = NIL_VAL;
    int done = 0;

    while (!done && f->ip < fn->ncode) {
        Val *instr = fn->code[f->ip];
        f->ip++;

        if (!instr || instr->type != T_LIST || instr->list->len < 1)
            continue;

        Val *op_val = instr->list->items[0];
        if (!op_val || op_val->type != T_STR) continue;
        const char *op = op_val->s;

        #define ARG(n) (instr->list->len > (n) ? instr->list->items[n] : NIL_VAL)

        if (!strcmp(op,"LABEL"))        { /* no-op */ }
        else if (!strcmp(op,"PUSH_CONST")) { push(f, ARG(1)); }
        else if (!strcmp(op,"POP"))        { pop(f); }
        else if (!strcmp(op,"STORE_NAME")) {
            Val *v = pop(f);
            Val *name_v = ARG(1);
            const char *name2 = (name_v->type==T_STR) ? name_v->s : "?";
            /* Handle "obj.field" notation: set field on map object */
            const char *dot = strchr(name2, '.');
            if (dot && dot != name2) {
                char obj_name[256];
                int n2 = (int)(dot - name2);
                if (n2 < (int)sizeof(obj_name)) {
                    memcpy(obj_name, name2, n2); obj_name[n2] = 0;
                    Val *obj = frame_get(f, obj_name);
                    if (obj && obj->type == T_MAP) {
                        map_set(obj->map, dot+1, v);
                        /* Don't store as flat variable — field is in the map */
                        goto next_instr;
                    }
                }
            }
            frame_set(f, name2, v);
        }
        else if (!strcmp(op,"LOAD_NAME")) {
            Val *name_v = ARG(1);
            const char *name2 = (name_v->type==T_STR) ? name_v->s : "?";
            Val *result2 = frame_get(f, name2);
            /* Handle "obj.field" notation if flat lookup gave NIL */
            if ((!result2 || result2 == NIL_VAL) && strchr(name2, '.')) {
                const char *dot = strchr(name2, '.');
                char obj_name[256];
                int n2 = (int)(dot - name2);
                if (n2 < (int)sizeof(obj_name)) {
                    memcpy(obj_name, name2, n2); obj_name[n2] = 0;
                    Val *obj = frame_get(f, obj_name);
                    if (obj && obj->type == T_MAP) {
                        result2 = map_get(obj->map, dot+1);
                    }
                }
            }
            push(f, result2 ? result2 : NIL_VAL);
        }
        else if (!strcmp(op,"JUMP")) {
            Val *lbl_v = ARG(1);
            if (lbl_v->type==T_STR) {
                int tgt = find_label(fn, lbl_v->s);
                if (tgt < 0) nc_panic("Ukjent label: %s", lbl_v->s);
                f->ip = tgt;
            }
        }
        else if (!strcmp(op,"JUMP_IF_FALSE")) {
            Val *cond = pop(f);
            Val *lbl_v = ARG(1);
            if (!val_truthy(cond) && lbl_v->type==T_STR) {
                int tgt = find_label(fn, lbl_v->s);
                if (tgt < 0) nc_panic("Ukjent label: %s", lbl_v->s);
                f->ip = tgt;
            }
        }
        else if (!strcmp(op,"RETURN")) {
            result = pop(f);
            done = 1;
        }
        else if (!strcmp(op,"CALL")) {
            Val *name_v = ARG(1);
            Val *nargs_v = ARG(2);
            int n = (nargs_v && nargs_v->type==T_INT) ? (int)nargs_v->i : 0;
            const char *fn_name = (name_v->type==T_STR) ? name_v->s : "?";
            /* collect args from stack (last arg on top) */
            Val **call_args = calloc(n, sizeof(Val*));
            for (int i=n-1; i>=0; i--) call_args[i] = pop(f);
            Val *ret = call_fn(fn_name, call_args, n);
            free(call_args);
            push(f, ret);
        }
        else if (!strcmp(op,"BINARY_ADD")) {
            Val *b2 = pop(f), *a2 = pop(f);
            if (a2->type==T_STR || b2->type==T_STR) {
                char *sa=val_to_str(a2), *sb=val_to_str(b2);
                char *res=malloc(strlen(sa)+strlen(sb)+1);
                strcpy(res,sa);strcat(res,sb);free(sa);free(sb);
                push(f, val_str_own(res));
            } else if (a2->type==T_INT && b2->type==T_INT) {
                push(f, val_int(a2->i + b2->i));
            } else if (a2->type==T_LIST && b2->type==T_LIST) {
                Val *out=val_list();
                for(int i=0;i<a2->list->len;i++) list_push(out->list,a2->list->items[i]);
                for(int i=0;i<b2->list->len;i++) list_push(out->list,b2->list->items[i]);
                push(f,out);
            } else {
                double fa=a2->type==T_INT?(double)a2->i:a2->f;
                double fb=b2->type==T_INT?(double)b2->i:b2->f;
                push(f, val_float(fa+fb));
            }
        }
        else if (!strcmp(op,"BINARY_SUB")) {
            Val *b2=pop(f),*a2=pop(f);
            if(a2->type==T_INT&&b2->type==T_INT) push(f,val_int(a2->i-b2->i));
            else{double fa=a2->type==T_INT?(double)a2->i:a2->f;double fb=b2->type==T_INT?(double)b2->i:b2->f;push(f,val_float(fa-fb));}
        }
        else if (!strcmp(op,"BINARY_MUL")) {
            Val *b2=pop(f),*a2=pop(f);
            if(a2->type==T_INT&&b2->type==T_INT) push(f,val_int(a2->i*b2->i));
            else{double fa=a2->type==T_INT?(double)a2->i:a2->f;double fb=b2->type==T_INT?(double)b2->i:b2->f;push(f,val_float(fa*fb));}
        }
        else if (!strcmp(op,"BINARY_DIV")) {
            Val *b2=pop(f),*a2=pop(f);
            if(b2->type==T_INT&&b2->i==0) nc_panic("Divisjon med null");
            if(a2->type==T_INT&&b2->type==T_INT) push(f,val_int(a2->i/b2->i));
            else{double fa=a2->type==T_INT?(double)a2->i:a2->f;double fb=b2->type==T_INT?(double)b2->i:b2->f;push(f,val_float(fa/fb));}
        }
        else if (!strcmp(op,"BINARY_MOD")) {
            Val *b2=pop(f),*a2=pop(f);
            if(a2->type==T_INT&&b2->type==T_INT) push(f,val_int(a2->i%b2->i));
            else push(f,val_int(0));
        }
        else if (!strcmp(op,"UNARY_NEG")) {
            Val *a2=pop(f);
            if(a2->type==T_INT) push(f,val_int(-a2->i));
            else push(f,val_float(-a2->f));
        }
        else if (!strcmp(op,"UNARY_NOT")) {
            Val *a2=pop(f); push(f,val_bool(!val_truthy(a2)));
        }
        else if (!strcmp(op,"COMPARE_EQ")) {
            Val *b2=pop(f),*a2=pop(f);
            int r=0;
            if(a2->type==T_NIL&&b2->type==T_NIL) r=1;
            else if(a2->type==T_BOOL&&b2->type==T_BOOL) r=(a2->b==b2->b);
            else if(a2->type==T_INT&&b2->type==T_INT) r=(a2->i==b2->i);
            else if(a2->type==T_FLOAT&&b2->type==T_FLOAT) r=(a2->f==b2->f);
            else if(a2->type==T_INT&&b2->type==T_FLOAT) r=((double)a2->i==b2->f);
            else if(a2->type==T_FLOAT&&b2->type==T_INT) r=(a2->f==(double)b2->i);
            else if(a2->type==T_STR&&b2->type==T_STR) r=(!strcmp(a2->s,b2->s));
            push(f,val_bool(r));
        }
        else if (!strcmp(op,"COMPARE_NE")) {
            Val *b2=pop(f),*a2=pop(f);
            /* NE = logical NOT of EQ; reuse same equality logic */
            int eq=0;
            if(a2->type==T_NIL&&b2->type==T_NIL) eq=1;
            else if(a2->type==T_BOOL&&b2->type==T_BOOL) eq=(a2->b==b2->b);
            else if(a2->type==T_INT&&b2->type==T_INT) eq=(a2->i==b2->i);
            else if(a2->type==T_FLOAT&&b2->type==T_FLOAT) eq=(a2->f==b2->f);
            else if(a2->type==T_INT&&b2->type==T_FLOAT) eq=((double)a2->i==b2->f);
            else if(a2->type==T_FLOAT&&b2->type==T_INT) eq=(a2->f==(double)b2->i);
            else if(a2->type==T_STR&&b2->type==T_STR) eq=(!strcmp(a2->s,b2->s));
            /* Different types (other than float/int mix): not equal → NE=1 */
            push(f,val_bool(!eq));
        }
        else if (!strcmp(op,"COMPARE_LT")||!strcmp(op,"COMPARE_GT")||
                 !strcmp(op,"COMPARE_LE")||!strcmp(op,"COMPARE_GE")) {
            Val *b2=pop(f),*a2=pop(f);
            double fa=a2->type==T_INT?(double)a2->i:a2->f;
            double fb=b2->type==T_INT?(double)b2->i:b2->f;
            int r;
            if(!strcmp(op,"COMPARE_LT")) r=fa<fb;
            else if(!strcmp(op,"COMPARE_GT")) r=fa>fb;
            else if(!strcmp(op,"COMPARE_LE")) r=fa<=fb;
            else r=fa>=fb;
            if(a2->type==T_STR&&b2->type==T_STR){
                int c=strcmp(a2->s,b2->s);
                if(!strcmp(op,"COMPARE_LT")) r=c<0;
                else if(!strcmp(op,"COMPARE_GT")) r=c>0;
                else if(!strcmp(op,"COMPARE_LE")) r=c<=0;
                else r=c>=0;
            }
            push(f,val_bool(r));
        }
        else if (!strcmp(op,"BUILD_LIST")) {
            Val *n_v=ARG(1);
            int n2=(n_v&&n_v->type==T_INT)?(int)n_v->i:0;
            Val *out=val_list();
            /* Pop n items (last on top) */
            Val **tmp=calloc(n2,sizeof(Val*));
            for(int i=n2-1;i>=0;i--) tmp[i]=pop(f);
            for(int i=0;i<n2;i++) list_push(out->list,tmp[i]);
            free(tmp); push(f,out);
        }
        else if (!strcmp(op,"BUILD_MAP")) {
            Val *n_v=ARG(1);
            int n2=(n_v&&n_v->type==T_INT)?(int)n_v->i:0;
            Val *out=val_map();
            for(int i=0;i<n2;i++){
                Val *v2=pop(f),*k2=pop(f);
                char *ks=val_to_str(k2);
                map_set(out->map,ks,v2);free(ks);
            }
            push(f,out);
        }
        else if (!strcmp(op,"INDEX_GET")) {
            Val *idx=pop(f),*obj=pop(f);
            if(obj->type==T_LIST){
                long long i2=idx->type==T_INT?idx->i:0;
                push(f, list_get(obj->list,(int)i2));
            } else if(obj->type==T_MAP){
                char *k=val_to_str(idx);
                Val *mv=map_get(obj->map,k);
                push(f, mv ? mv : NIL_VAL);free(k);
            } else if(obj->type==T_STR){
                long long i2=idx->type==T_INT?idx->i:0;
                if(i2<0)i2+=strlen(obj->s);
                if(i2>=0&&i2<(long long)strlen(obj->s)){char c[2]={obj->s[i2],0};push(f,val_str(c));}
                else push(f,val_str(""));
            } else push(f,NIL_VAL);
        }
        else if (!strcmp(op,"INDEX_SET")) {
            Val *val3=pop(f),*idx=pop(f),*obj=pop(f);
            if(obj->type==T_LIST){
                long long i2=idx->type==T_INT?idx->i:0;
                list_set(obj->list,(int)i2,val3);
            } else if(obj->type==T_MAP){
                char *k=val_to_str(idx);
                map_set(obj->map,k,val3);free(k);
            }
            push(f,obj); /* leave object on stack */
        }
        else if (!strcmp(op,"THROW")) {
            Val *exc=pop(f);
            g_exception=exc;
            char *msg=val_to_str(exc);
            snprintf(g_err_msg,sizeof(g_err_msg),"Norscode unntak: %s",msg);
            free(msg);
            g_depth--;
            for (int _i=0;_i<f->nvar;_i++) free(f->var_names[_i]);
            free(f->var_names); free(f->var_vals); free(f->stack); free(f);
            longjmp(g_err_jmp,1);
        }
        else if (!strcmp(op,"TRY_BEGIN")) { /* simplified: no real catch support yet */ }
        else if (!strcmp(op,"TRY_END"))   { /* no-op */ }
        else if (!strcmp(op,"LOAD_EXCEPTION")) { push(f, g_exception ? g_exception : NIL_VAL); }
        else {
            fprintf(stderr, "[nc-vm] ukjent opkode: %s\n", op);
        }
        next_instr:;
    }

    g_depth--;
    /* Free dynamic frame resources */
    for (int i=0; i<f->nvar; i++) free(f->var_names[i]);
    free(f->var_names); free(f->var_vals); free(f->stack);
    free(f);
    return result;
}

/* ── Main ─────────────────────────────────────────────────────────────────── */
int main(int argc, char **argv) {
    /* Init singletons */
    NIL_VAL   = val_alloc(T_NIL);
    TRUE_VAL  = val_alloc(T_BOOL); TRUE_VAL->b  = 1;
    FALSE_VAL = val_alloc(T_BOOL); FALSE_VAL->b = 0;

    if (argc < 2) {
        fprintf(stderr, "Bruk:\n");
        fprintf(stderr, "  nc-vm <fil.ncb.json> [<ekstra.ncb.json>...]\n");
        fprintf(stderr, "  nc-vm --nc-run <kilde.no>\n");
        fprintf(stderr, "  nc-vm --nc-compile <kilde.no> [utdata.ncb.json]\n");
        return 1;
    }

    if (setjmp(g_err_jmp)) {
        fprintf(stderr, "\nnc-vm feil: %s\n", g_err_msg);
        return 1;
    }

    /* --nc-run mode: compile + run source file using selfhost compiler */
    if (!strcmp(argv[1], "--nc-run")) {
        if (argc < 3) { fprintf(stderr, "nc-vm --nc-run: mangler kildefil\n"); return 1; }
        const char *src_path = argv[2];

        /* Load selfhost bytecodes + std library */
        const char *precomp = getenv("NC_PRECOMPILED_DIR");
        if (!precomp) precomp = "build/nc-precompiled";
        char selfhost_path[512], std_path[512], compiler_path[512];
        snprintf(selfhost_path,  sizeof(selfhost_path),  "%s/selfhost",  precomp);
        snprintf(std_path,       sizeof(std_path),       "%s/std",       precomp);
        snprintf(compiler_path,  sizeof(compiler_path),  "%s/compiler",  precomp);
        /* Last kun selfhost — kompiler.no trenger ikke std-biblioteket.
           Std-biblioteket definerer __main__.feil etc. som kolliderer med
           selfhost sine innebygde funksjoner. */
        load_ncb_dir(selfhost_path);
        load_ncb_dir(compiler_path);

        if (g_nfns == 0) {
            fprintf(stderr, "nc-vm: ingen selfhost-bytekoder i: %s\n", precomp);
            return 1;
        }

        /* Read source file */
        FILE *sf = fopen(src_path, "rb");
        if (!sf) { fprintf(stderr, "nc-vm: kan ikke åpne: %s\n", src_path); return 1; }
        fseek(sf, 0, SEEK_END); long ssz = ftell(sf); rewind(sf);
        char *src_text = malloc(ssz+1); fread(src_text, 1, ssz, sf); fclose(sf); src_text[ssz]=0;
        Val *src_val    = val_str_own(src_text);
        Val *module_val = val_str("__main__");

        /* Step 1: Call kompiler_fil(source_text, "__main__") → NCB JSON string */
        Val *compile_args[] = { src_val, module_val };
        Val *ncb_json_val = call_fn("kompiler_fil", compile_args, 2);

        if (!ncb_json_val || ncb_json_val->type != T_STR) {
            fprintf(stderr, "nc-vm: kompiler_fil returnerte ikke JSON-tekst");
            if (ncb_json_val) {
                char *s = json_emit(ncb_json_val);
                fprintf(stderr, " (type=%d val=%.80s)\n", ncb_json_val->type, s); free(s);
            } else fprintf(stderr, "\n");
            return 1;
        }

        /* Step 2: Parse the NCB JSON string */
        Val *ncb_data = json_parse_str(ncb_json_val->s);
        if (!ncb_data || ncb_data->type != T_MAP) {
            fprintf(stderr, "nc-vm: ugyldig NCB JSON fra kompiler_fil\n");
            return 1;
        }

        /* Step 3: Load compiled functions — remember where they start */
        int compiled_start = g_nfns;
        load_ncb_val(ncb_data, NULL);

        /* Step 4: Run the compiled entry point.
           Search from compiled_start onwards so compiled functions take priority
           over any earlier-loaded selfhost functions with the same name. */
        Val *entry_val = map_get(ncb_data->map, "entry");
        const char *entry_name = (entry_val && entry_val->type == T_STR)
                                  ? entry_val->s : "__main__.start";
        /* Find the compiled entry specifically */
        FnDef *compiled_entry = NULL;
        for (int i = compiled_start; i < g_nfns; i++) {
            if (!strcmp(g_fns[i].name, entry_name)) {
                compiled_entry = &g_fns[i]; break;
            }
        }
        if (!compiled_entry) compiled_entry = fn_find(entry_name);
        if (!compiled_entry) {
            fprintf(stderr, "nc-vm: inngangsport '%s' ikke funnet\n", entry_name);
            return 1;
        }
        exec_fn(compiled_entry, NULL, 0);
        return 0;
    }

    /* --nc-compile mode: compile source → write .ncb.json, do NOT run */
    if (!strcmp(argv[1], "--nc-compile")) {
        if (argc < 3) { fprintf(stderr, "nc-vm --nc-compile: mangler kildefil\n"); return 1; }
        const char *src_path = argv[2];
        const char *out_path = (argc >= 4) ? argv[3] : NULL;

        /* Default output: replace .no with .ncb.json */
        char out_buf[1024];
        if (!out_path) {
            strncpy(out_buf, src_path, sizeof(out_buf)-20);
            out_buf[sizeof(out_buf)-20] = 0;
            char *dot = strrchr(out_buf, '.');
            if (dot && !strcmp(dot, ".no")) strcpy(dot, ".ncb.json");
            else strcat(out_buf, ".ncb.json");
            out_path = out_buf;
        }

        /* Load selfhost compiler NCBs */
        const char *precomp = getenv("NC_PRECOMPILED_DIR");
        if (!precomp) precomp = "build/nc-precompiled";
        char sp[512], cp[512];
        snprintf(sp, sizeof(sp), "%s/selfhost",  precomp);
        snprintf(cp, sizeof(cp), "%s/compiler",  precomp);
        load_ncb_dir(sp);
        load_ncb_dir(cp);
        if (g_nfns == 0) {
            fprintf(stderr, "nc-vm: ingen selfhost-bytekoder i: %s\n", precomp);
            return 1;
        }

        /* Read source */
        FILE *sf = fopen(src_path, "rb");
        if (!sf) { fprintf(stderr, "nc-vm: kan ikke åpne: %s\n", src_path); return 1; }
        fseek(sf, 0, SEEK_END); long ssz = ftell(sf); rewind(sf);
        char *src_text = malloc(ssz+1); fread(src_text, 1, ssz, sf); fclose(sf); src_text[ssz]=0;
        Val *sv = val_str_own(src_text);
        Val *mv = val_str("__main__");

        /* Compile */
        Val *ca[] = { sv, mv };
        Val *ncb_json = call_fn("kompiler_fil", ca, 2);
        if (!ncb_json || ncb_json->type != T_STR) {
            fprintf(stderr, "nc-vm --nc-compile: kompilering feilet\n");
            return 1;
        }

        /* Write to file */
        FILE *of = fopen(out_path, "wb");
        if (!of) { fprintf(stderr, "nc-vm: kan ikke skrive: %s\n", out_path); return 1; }
        fwrite(ncb_json->s, 1, strlen(ncb_json->s), of);
        fclose(of);
        fprintf(stdout, "%s\n", out_path);   /* print output path */
        return 0;
    }

    /* Load all bytecode files; remember entry from first file */
    const char *explicit_entry = NULL;
    for (int i=1; i<argc; i++) {
        if (argv[i][0]=='-') continue;  /* skip flags */
        if (!explicit_entry) {
            /* Read entry field from first NCB file */
            Val *first = json_load_file(argv[i]);
            if (first && first->type == T_MAP) {
                Val *e = map_get(first->map, "entry");
                if (e && e->type == T_STR && e->s[0])
                    explicit_entry = strdup(e->s);
            }
        }
        load_ncb(argv[i]);
    }

    if (g_nfns == 0) {
        fprintf(stderr, "nc-vm: ingen funksjoner lastet\n");
        return 1;
    }

    /* Find entry point: use explicit_entry from NCB, else scan candidates */
    FnDef *entry = NULL;
    if (explicit_entry) {
        entry = fn_find(explicit_entry);
    }
    if (!entry) {
        const char *entry_candidates[] = {
            "start", "__main__.start", "hoved", "__main__.hoved",
            "main",  "__main__.main",  NULL
        };
        for (int i=0; entry_candidates[i]; i++) {
            entry = fn_find(entry_candidates[i]);
            if (entry) break;
        }
    }
    if (!entry) {
        fprintf(stderr, "nc-vm: ingen inngangsport funnet (start/hoved/main)\n");
        fprintf(stderr, "Tilgjengelige funksjoner:\n");
        for (int i=0; i<g_nfns && i<20; i++)
            fprintf(stderr, "  %s\n", g_fns[i].name);
        return 1;
    }

    Val *result = exec_fn(entry, NULL, 0);
    (void)result;
    return 0;
}
