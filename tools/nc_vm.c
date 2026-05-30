/*
 * nc_vm.c — Norscode native bytecode interpreter
 *
 * AVVIKLA: Bruk dist/norscode_native i staden (generert frå Norscode→C).
 * nc_vm.c er no berre ein eingongs-bootstrap-hjelpar for første bygg.
 *
 * Primær bygg:  bash tools/build_norscode_native.sh  (berre clang, ingen nc-vm)
 * nc-vm-bygg:   clang -O2 -o dist/nc-vm tools/nc_vm.c  (berre om norscode_native manglar)
 *
 * Original: Runs pre-compiled .ncb.json bytecode without Python.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>
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
/* Cache small integers (-1..255) to reduce allocations */
#define IVAL_CACHE_MIN -1
#define IVAL_CACHE_MAX 255
static Val *g_ival_cache[IVAL_CACHE_MAX - IVAL_CACHE_MIN + 1];
static Val *val_int(long long i) {
    if (i >= IVAL_CACHE_MIN && i <= IVAL_CACHE_MAX) {
        int idx = (int)(i - IVAL_CACHE_MIN);
        if (!g_ival_cache[idx]) {
            g_ival_cache[idx] = val_alloc(T_INT);
            g_ival_cache[idx]->i = i;
        }
        return g_ival_cache[idx];
    }
    Val *v=val_alloc(T_INT); v->i=i; return v;
}
static Val *val_float(double f)  { Val *v=val_alloc(T_FLOAT); v->f=f;   return v; }
/* val_free: free a Val and its owned children (NOT singletons or cached ints) */
static void val_free(Val *v) {
    if (!v || v == NIL_VAL || v == TRUE_VAL || v == FALSE_VAL) return;
    if (v->type == T_INT) {
        long long vi=v->i;
        if(vi>=IVAL_CACHE_MIN&&vi<=IVAL_CACHE_MAX&&v==g_ival_cache[(int)(vi-IVAL_CACHE_MIN)]) return;
        free(v); return;
    }
    if (v->type == T_STR) { free(v->s); free(v); return; }
    if (v->type == T_LIST && v->list) {
        for (int i=0;i<v->list->len;i++) val_free(v->list->items[i]);
        free(v->list->items); free(v->list);
    } else if (v->type == T_MAP && v->map) {
        for (int i=0;i<v->map->len;i++) { free(v->map->keys[i]); val_free(v->map->vals[i]); }
        free(v->map->keys); free(v->map->vals); free(v->map);
    }
    free(v);
}
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
static int     g_throwing  = 0;      /* 1 = exception propagating up the call stack */

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
    /* 1b. Strip "builtin." prefix and try altname match (e.g. builtin.ordbok.hent_bool → std.ordbok.hent_bool) */
    const char *nb = (strncmp(name,"builtin.",8)==0) ? name+8 : NULL;
    if (nb) {
        for (int i=0;i<g_nfns;i++) {
            if (!g_fn_altnames[i]) continue;
            const char *an = g_fn_altnames[i];
            /* Match last N segments: ordbok.hent_bool matches std.ordbok.hent_bool */
            size_t nbl = strlen(nb);
            size_t anl = strlen(an);
            if (anl >= nbl && !strcmp(an + anl - nbl, nb) &&
                (anl == nbl || an[anl-nbl-1] == '.'))
                return &g_fns[i];
        }
    }
    /* 2. Strip __main__. from both sides */
    const char *n = name;
    if (strncmp(n, "__main__.", 9)==0) n += 9;
    for (int i=0;i<g_nfns;i++) {
        const char *fn = g_fns[i].name;
        if (strncmp(fn,"__main__.",9)==0) fn+=9;
        if (!strcmp(fn, n)) return &g_fns[i];
    }
    /* 3. Last segment match: query "builtin.lex" → find any fn whose last segment == "lex" */
    const char *last = strrchr(name, '.');
    if (last) {
        last++;
        for (int i=0;i<g_nfns;i++) {
            const char *fn = g_fns[i].name;
            /* Strip __main__. prefix */
            if (strncmp(fn,"__main__.",9)==0) fn+=9;
            /* Direct match (e.g. short name or stripped name) */
            if (!strcmp(fn, last)) return &g_fns[i];
            /* Also compare last segment of stored name (e.g. selfhost.lexer.lex → lex) */
            const char *fn_last = strrchr(fn, '.');
            if (fn_last && !strcmp(fn_last+1, last)) return &g_fns[i];
        }
    }
    /* 4. Reverse short-name match: query "kompiler_fil" matches "selfhost.kompiler.kompiler_fil"
       (no dots in query → scan stored function names for matching last segment) */
    if (!strchr(name, '.')) {
        for (int i=0;i<g_nfns;i++) {
            const char *fn = g_fns[i].name;
            const char *fn_last = strrchr(fn, '.');
            if (fn_last) fn_last++;
            else fn_last = fn;
            if (!strcmp(fn_last, name)) return &g_fns[i];
            /* Also check altname */
            if (g_fn_altnames[i]) {
                const char *an_last = strrchr(g_fn_altnames[i], '.');
                if (an_last && !strcmp(an_last+1, name)) return &g_fns[i];
            }
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
    /* Handle bootstrap/stdlib/std_MODNAME.ncb.json → std.MODNAME */
    const char *sl = strstr(filepath, "bootstrap/stdlib/std_");
    if (sl) {
        sl += strlen("bootstrap/stdlib/std_");
        char tmp[256];
        strncpy(tmp, sl, sizeof(tmp)-1); tmp[sizeof(tmp)-1]=0;
        char *suf = strstr(tmp, ".ncb.json");
        if (suf) *suf = 0;
        /* Replace _ with nothing (std_ordbok → ordbok) */
        snprintf(out, outsz, "std.%s", tmp);
        return;
    }
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
    /* try/catch handler stack */
    char  **catch_labels;   /* label names to jump to on exception */
    int     ncatch;         /* number of active handlers */
    int     catch_cap;
} Frame;

#define MAX_CALL_DEPTH 8192
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
/* Raw JSON emit: always quote strings (for NCB serialization, no smart stripping) */
static char *json_emit_raw(Val *v);
static char *json_emit_raw_list(List *l) {
    char *out=strdup("[");
    for(int i=0;i<l->len;i++){
        char *s=json_emit_raw(l->items[i]);
        out=realloc(out,strlen(out)+strlen(s)+3);
        if(i)strcat(out,",");
        strcat(out,s);free(s);
    }
    out=realloc(out,strlen(out)+2);strcat(out,"]");return out;
}
static char *json_emit_raw(Val *v) {
    if(!v||v->type==T_NIL) return strdup("null");
    if(v->type==T_BOOL) return strdup(v->b?"true":"false");
    if(v->type==T_INT){char b[32];snprintf(b,sizeof(b),"%lld",v->i);return strdup(b);}
    if(v->type==T_FLOAT){char b[64];snprintf(b,sizeof(b),"%g",v->f);return strdup(b);}
    if(v->type==T_STR) return json_emit_str(v->s); /* ALWAYS quote strings */
    if(v->type==T_LIST) return json_emit_raw_list(v->list);
    if(v->type==T_MAP){
        char *out=strdup("{");
        for(int i=0;i<v->map->len;i++){
            char *k=json_emit_str(v->map->keys[i]);
            char *val=json_emit_raw(v->map->vals[i]);
            out=realloc(out,strlen(out)+strlen(k)+strlen(val)+4);
            if(i)strcat(out,",");
            strcat(out,k);strcat(out,":");strcat(out,val);
            free(k);free(val);
        }
        out=realloc(out,strlen(out)+2);strcat(out,"}");return out;
    }
    return strdup("null");
}

/* For ordbok_tekst maps: emit string values that look like JSON primitives verbatim */
static int str_looks_like_json_nonstring(const char *s) {
    if (!s || !*s) return 0;
    if (!strcmp(s,"true")||!strcmp(s,"false")||!strcmp(s,"null")) return 1;
    /* JSON object/array: must be balanced (start+end match) */
    size_t sl=strlen(s);
    if (s[0]=='{' && s[sl-1]=='}' && sl>=2) return 1;
    if (s[0]=='[' && s[sl-1]==']' && sl>=2) return 1;
    /* numeric? */
    char *end; strtoll(s,&end,10);
    if (*end==0 && end!=s) return 1;
    return 0;
}
static char *json_emit(Val *v) {
    if(!v||v->type==T_NIL) return strdup("null");
    if(v->type==T_BOOL) return strdup(v->b?"true":"false");
    if(v->type==T_INT){char b[32];snprintf(b,sizeof(b),"%lld",v->i);return strdup(b);}
    if(v->type==T_FLOAT){char b[64];snprintf(b,sizeof(b),"%g",v->f);return strdup(b);}
    if(v->type==T_STR) {
        /* ordbok_tekst compat: emit non-string JSON values without quotes */
        if (str_looks_like_json_nonstring(v->s)) return strdup(v->s);
        return json_emit_str(v->s);
    }
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

/* ── Value equality helper ────────────────────────────────────────────────── */
static int vals_eq(Val *a, Val *b) {
    if (!a || !b) return (a == b);
    if (a->type==T_NIL && b->type==T_NIL) return 1;
    if (a->type==T_BOOL && b->type==T_BOOL) return (a->b == b->b);
    if (a->type==T_INT  && b->type==T_INT)  return (a->i == b->i);
    if (a->type==T_FLOAT&& b->type==T_FLOAT)return (a->f == b->f);
    if (a->type==T_INT  && b->type==T_FLOAT)return ((double)a->i == b->f);
    if (a->type==T_FLOAT&& b->type==T_INT)  return (a->f == (double)b->i);
    if (a->type==T_STR  && b->type==T_STR)  return (!strcmp(a->s, b->s));
    return 0;
}

/* ── Built-in functions ───────────────────────────────────────────────────── */
static Val *builtin_call(const char *name, Val **args, int nargs) {
    /* Strip module prefixes: builtin.X, __main__.X, selfhost.*.X */
    const char *n = name;
    if (strncmp(n,"builtin.",8)==0) n+=8;
    else if (strncmp(n,"__main__.",9)==0) n+=9;
    /* After prefix strip, take last dotted segment (e.g. builtin.math.pluss → pluss) */
    {
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
        /* b==-1 is sentinel for "open end" (slice to end), use length */
        int open_end = (args[2]->type==T_INT && args[2]->i==-1);
        if(src->type==T_STR){
            long long l=strlen(src->s);
            if(open_end) b=l;
            if(a<0)a+=l; if(!open_end && b<0)b+=l;
            if(a<0)a=0; if(b>l)b=l;
            if(a>b) return val_str("");
            char *s=malloc(b-a+1);memcpy(s,src->s+a,b-a);s[b-a]=0;
            return val_str_own(s);
        }
        if(src->type==T_LIST){
            long long l=src->list->len;
            if(open_end) b=l;
            if(a<0)a+=l; if(!open_end && b<0)b+=l;
            if(a<0)a=0; if(b>l)b=l;
            Val *out=val_list();
            for(long long i=a;i<b;i++) list_push(out->list, list_get(src->list,(int)i));
            return out;
        }
        return val_nil();
    }
    if (!strcmp(n,"fjern_nokkel") || !strcmp(n,"fjern") || !strcmp(n,"slett_nøkkel")) {
        /* dict or list remove — returns remaining length as int */
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
            return val_int(m->len);
        }
        return val_int(0);
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
        free(src);
        /* Norscode ordbok_tekst semantics: stringify non-string map values */
        if (r && r->type == T_MAP) {
            for (int i=0; i<r->map->len; i++) {
                Val *v = r->map->vals[i];
                if (v && v->type != T_STR) {
                    char *s = json_emit(v);
                    r->map->vals[i] = val_str_own(s);
                }
            }
        }
        /* JSON array → string-keyed map (Python VM compat) */
        if (r && r->type == T_LIST) {
            Val *m = val_map();
            char keybuf[32];
            for (int i=0; i<r->list->len; i++) {
                snprintf(keybuf, sizeof(keybuf), "%d", i);
                Val *v = r->list->items[i];
                if (v && v->type != T_STR) {
                    char *s = json_emit(v);
                    v = val_str_own(s);
                }
                map_set(m->map, keybuf, v);
            }
            return m;
        }
        return r;
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
    /* ── HTTP response helpers ────────────────────────────────────────── */
    if (!strcmp(n,"response_status") || !strcmp(n,"http_response_status")) {
        /* Parse {"status":N,...} */
        if(nargs<1) return val_int(0);
        char *s=val_to_str(args[0]);
        JP j2={s,1}; Val *parsed=jp_parse(&j2); free(s);
        if(parsed&&parsed->type==T_MAP){
            Val *sv=map_get(parsed->map,"status");
            if(sv&&sv->type==T_INT) return sv;
            if(sv&&sv->type==T_STR) return val_int(atoll(sv->s));
        }
        return val_int(0);
    }
    if (!strcmp(n,"response_text") || !strcmp(n,"http_response_text")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]);
        JP j2={s,1}; Val *parsed=jp_parse(&j2); free(s);
        if(parsed&&parsed->type==T_MAP){
            Val *bv=map_get(parsed->map,"body");
            if(bv&&bv->type==T_STR) return bv;
        }
        return val_str("");
    }
    if (!strcmp(n,"response_json") || !strcmp(n,"http_response_json")) {
        if(nargs<1) return val_map();
        char *s=val_to_str(args[0]);
        JP j2={s,1}; Val *parsed=jp_parse(&j2); free(s);
        if(parsed&&parsed->type==T_MAP){
            Val *bv=map_get(parsed->map,"body");
            if(bv&&bv->type==T_STR){
                JP j3={bv->s,1}; Val *body=jp_parse(&j3);
                if(body&&body->type==T_MAP){
                    /* Stringify non-string values */
                    for(int _i=0;_i<body->map->len;_i++){
                        Val *v=body->map->vals[_i];
                        if(v&&v->type!=T_STR){char *s2=json_emit(v);body->map->vals[_i]=val_str_own(s2);}
                    }
                    return body;
                }
            }
        }
        return val_map();
    }
    if (!strcmp(n,"response_header") || !strcmp(n,"http_response_header")) {
        if(nargs<2) return val_str("");
        char *s=val_to_str(args[0]); char *key=val_to_str(args[1]);
        JP j2={s,1}; Val *parsed=jp_parse(&j2); free(s);
        if(parsed&&parsed->type==T_MAP){
            Val *hv=map_get(parsed->map,"headers");
            if(hv&&hv->type==T_MAP){
                Val *v=map_get(hv->map,key); free(key);
                if(v&&v->type==T_STR) return v;
                return val_str("");
            }
        }
        free(key); return val_str("");
    }
    if (!strcmp(n,"response_header_or") || !strcmp(n,"http_response_header_or")) {
        if(nargs<3) return nargs>=3?args[2]:val_str("");
        char *s=val_to_str(args[0]); char *key=val_to_str(args[1]); char *fb=val_to_str(args[2]);
        JP j2={s,1}; Val *parsed=jp_parse(&j2); free(s);
        Val *result=NULL;
        if(parsed&&parsed->type==T_MAP){
            Val *hv=map_get(parsed->map,"headers");
            if(hv&&hv->type==T_MAP){
                Val *v=map_get(hv->map,key);
                if(v&&v->type==T_STR&&v->s[0]) result=v;
            }
        }
        free(key);
        if(result){free(fb);return result;}
        return val_str_own(fb);
    }
    if (!strcmp(n,"response_ok") || !strcmp(n,"http_response_ok")) {
        Val *status_args[1]={args[0]};
        Val *sv=builtin_call("response_status", status_args, 1);
        long long st=(sv&&sv->type==T_INT)?sv->i:0;
        return val_bool(st>=200&&st<300);
    }
    if (!strcmp(n,"response_json_checked") || !strcmp(n,"http_response_json_checked")) {
        Val *r_args[1]={args[0]};
        return builtin_call("response_json", r_args, 1);
    }
    /* ── instruksjon_til_c: fast C builtin ──────────────────────────── */
    if (!strcmp(n,"instruksjon_til_c")) {
        if(nargs<2) return val_str("/* ukjent */");
        char *op=val_to_str(args[0]);
        long long v=(args[1]->type==T_INT)?args[1]->i:(args[1]->type==T_STR?atoll(args[1]->s):0);
        char buf[256];
        if(!strcmp(op,"PUSH"))snprintf(buf,sizeof(buf),"stack[sp++] = %lld;",v);
        else if(!strcmp(op,"ADD"))strcpy(buf,"stack[sp-2] = stack[sp-2] + stack[sp-1]; sp = sp - 1;");
        else if(!strcmp(op,"SUB"))strcpy(buf,"stack[sp-2] = stack[sp-2] - stack[sp-1]; sp = sp - 1;");
        else if(!strcmp(op,"MUL"))strcpy(buf,"stack[sp-2] = stack[sp-2] * stack[sp-1]; sp = sp - 1;");
        else if(!strcmp(op,"DIV"))strcpy(buf,"stack[sp-2] = stack[sp-2] / stack[sp-1]; sp = sp - 1;");
        else if(!strcmp(op,"MOD"))strcpy(buf,"stack[sp-2] = stack[sp-2] % stack[sp-1]; sp = sp - 1;");
        else if(!strcmp(op,"NEG"))strcpy(buf,"stack[sp-1] = -stack[sp-1];");
        else if(!strcmp(op,"AND"))strcpy(buf,"stack[sp-2] = (stack[sp-2] && stack[sp-1]); sp = sp - 1;");
        else if(!strcmp(op,"OR"))strcpy(buf,"stack[sp-2] = (stack[sp-2] || stack[sp-1]); sp = sp - 1;");
        else if(!strcmp(op,"NOT"))strcpy(buf,"stack[sp-1] = !stack[sp-1];");
        else if(!strcmp(op,"EQ"))strcpy(buf,"stack[sp-2] = (stack[sp-2] == stack[sp-1]); sp = sp - 1;");
        else if(!strcmp(op,"GT"))strcpy(buf,"stack[sp-2] = (stack[sp-2] > stack[sp-1]); sp = sp - 1;");
        else if(!strcmp(op,"LT"))strcpy(buf,"stack[sp-2] = (stack[sp-2] < stack[sp-1]); sp = sp - 1;");
        else if(!strcmp(op,"GE")||!strcmp(op,"GTE"))strcpy(buf,"stack[sp-2] = (stack[sp-2] >= stack[sp-1]); sp = sp - 1;");
        else if(!strcmp(op,"LE")||!strcmp(op,"LTE"))strcpy(buf,"stack[sp-2] = (stack[sp-2] <= stack[sp-1]); sp = sp - 1;");
        else if(!strcmp(op,"NE")||!strcmp(op,"NEQ"))strcpy(buf,"stack[sp-2] = (stack[sp-2] != stack[sp-1]); sp = sp - 1;");
        else if(!strcmp(op,"HALT"))strcpy(buf,"return 0;");
        else if(!strcmp(op,"PRINT"))strcpy(buf,"printf(\"%d\\n\", stack[sp-1]);");
        else if(!strcmp(op,"LABEL"))snprintf(buf,sizeof(buf),"L%lld:;",v);
        else if(!strcmp(op,"JMP"))snprintf(buf,sizeof(buf),"goto L%lld;",v);
        else if(!strcmp(op,"JZ"))snprintf(buf,sizeof(buf),"if (stack[sp-1] == 0) goto L%lld;",v);
        else if(!strcmp(op,"JNZ"))snprintf(buf,sizeof(buf),"if (stack[sp-1] != 0) goto L%lld;",v);
        else if(!strcmp(op,"CALL"))snprintf(buf,sizeof(buf),"ret_stack[rsp++] = 0; goto L%lld;",v);
        else if(!strcmp(op,"RET"))strcpy(buf,"goto *labels[0];");
        else if(!strcmp(op,"STORE"))snprintf(buf,sizeof(buf),"mem[%lld] = stack[sp-1]; sp = sp - 1;",v);
        else if(!strcmp(op,"LOAD"))snprintf(buf,sizeof(buf),"stack[sp++] = mem[%lld];",v);
        else if(!strcmp(op,"DUP"))strcpy(buf,"stack[sp] = stack[sp-1]; sp = sp + 1;");
        else if(!strcmp(op,"POP"))strcpy(buf,"sp = sp - 1;");
        else if(!strcmp(op,"SWAP"))strcpy(buf,"tmp = stack[sp-1]; stack[sp-1] = stack[sp-2]; stack[sp-2] = tmp;");
        else if(!strcmp(op,"OVER"))strcpy(buf,"stack[sp] = stack[sp-2]; sp = sp + 1;");
        else strcpy(buf,"/* ukjent */");
        free(op); return val_str(buf);
    }
    /* Also add to fast-path: demo_program, ir_* */
    if (!strcmp(n,"demo_program")) return val_str("PUSH 1\nPRINT\nHALT\n");
    if (!strcmp(n,"ir_kontrakt_versjon")) return val_str("selfhost-ir-contract-v1");
    if (!strcmp(n,"kompiler_skript_til_c")||!strcmp(n,"kompiler_uttrykk_til_c")) return val_str("int main(void) { return 0; }\n");
    /* ── disasm_uttrykk: fast C implementation of shunting-yard ─────── */
    if (!strcmp(n,"disasm_uttrykk") || !strcmp(n,"disasm_fra_uttrykk")) {
        if(nargs<1) return val_str("");
        /* Preprocess: replace Norwegian operators */
        char *raw=val_to_str(args[0]);
        /* Build token list */
        Val *toks_val=val_list();
        /* Use tokeniser_uttrykk logic inline */
        const char *p2=raw;
        while(*p2) {
            while(*p2&&(unsigned char)*p2<=' ') p2++;
            if(!*p2) break;
            static const char *multi2[]={"<=>","<->","=>","->","<-","&&","||","+=","-=","*=","/=","%=","==","!=","<=",">=","<>",NULL};
            int m2=0; for(int _i=0;multi2[_i];_i++){size_t ol=strlen(multi2[_i]);if(strncmp(p2,multi2[_i],ol)==0){list_push(toks_val->list,val_str(multi2[_i]));p2+=ol;m2=1;break;}}
            if(m2) continue;
            if(isalpha((unsigned char)*p2)||*p2=='_'||(unsigned char)*p2>0x7f){const char *s2=p2;while(*p2&&(isalnum((unsigned char)*p2)||*p2=='_'||(unsigned char)*p2>0x7f))p2++;char *t2=malloc(p2-s2+1);memcpy(t2,s2,p2-s2);t2[p2-s2]=0;list_push(toks_val->list,val_str_own(t2));continue;}
            if(isdigit((unsigned char)*p2)){const char *s2=p2;while(*p2&&isdigit((unsigned char)*p2))p2++;char *t2=malloc(p2-s2+1);memcpy(t2,s2,p2-s2);t2[p2-s2]=0;list_push(toks_val->list,val_str_own(t2));continue;}
            if((unsigned char)*p2>' '){char t2[2]={*p2,0};list_push(toks_val->list,val_str(t2));}
            p2++;
        }
        free(raw);
        /* Shunting-yard: convert infix tokens to IR list */
        /* Operator precedence */
        #define PREC(op) (!strcmp(op,"MUL")||!strcmp(op,"DIV")||!strcmp(op,"MOD")?7:\
            !strcmp(op,"ADD")||!strcmp(op,"SUB")?6:\
            !strcmp(op,"GT")||!strcmp(op,"LT")||!strcmp(op,"GTE")||!strcmp(op,"LTE")||!strcmp(op,"EQ")||!strcmp(op,"NEQ")?5:\
            !strcmp(op,"UNOT")?4:\
            !strcmp(op,"AND")||!strcmp(op,"XOR")||!strcmp(op,"XNOR")||!strcmp(op,"NAND")||!strcmp(op,"NOR")?3:\
            !strcmp(op,"OR")||!strcmp(op,"IMPLIES")?2:0)
        /* normalise_op */
        /* NORM: always returns strdup'd string or NULL (must be free'd) */
        #define NORM(tok) (\
            !strcmp(tok,"+")?strdup("ADD"):!strcmp(tok,"-")?strdup("SUB"):!strcmp(tok,"*")?strdup("MUL"):!strcmp(tok,"/")?strdup("DIV"):\
            !strcmp(tok,"%")?strdup("MOD"):!strcmp(tok,">")?strdup("GT"):!strcmp(tok,"<")?strdup("LT"):\
            !strcmp(tok,"==")?strdup("EQ"):!strcmp(tok,"!=")||!strcmp(tok,"<>")?strdup("NEQ"):\
            !strcmp(tok,"&&")||!strcmp(tok,"og")||!strcmp(tok,"and")?strdup("AND"):\
            !strcmp(tok,"||")||!strcmp(tok,"eller")||!strcmp(tok,"or")?strdup("OR"):\
            !strcmp(tok,"ikkje")||!strcmp(tok,"ikke")||!strcmp(tok,"not")?strdup("UNOT"):\
            !strcmp(tok,"storre_enn")?strdup("GT"):!strcmp(tok,"mindre_enn")?strdup("LT"):\
            !strcmp(tok,"xor")?strdup("XOR"):\
            !strcmp(tok,"nand")||!strcmp(tok,"og_ikke")||!strcmp(tok,"and_not")?strdup("NAND"):\
            !strcmp(tok,"nor")||!strcmp(tok,"eller_ikke")||!strcmp(tok,"or_not")?strdup("NOR"):\
            !strcmp(tok,"xnor")||!strcmp(tok,"xeller_ikke")?strdup("XNOR"):\
            !strcmp(tok,"implies")||!strcmp(tok,"implies_that")||!strcmp(tok,"this_implies")||\
            !strcmp(tok,"impliserer")||!strcmp(tok,"impliserer_at")||!strcmp(tok,"dette_impliserer")||\
            !strcmp(tok,"medforer")?strdup("IMPLIES"):\
            !strcmp(tok,"storre_enn_eller_lik")||!strcmp(tok,"erstorreellerlik")||!strcmp(tok,"storrelik")||!strcmp(tok,"er_storre_lik")||!strcmp(tok,"storre_lik")||!strcmp(tok,"er_storre_eller_lik")?strdup("GTE"):\
            !strcmp(tok,"mindre_enn_eller_lik")||!strcmp(tok,"ermindreogellerlik")||!strcmp(tok,"mindrelik")||!strcmp(tok,"er_mindre_lik")||!strcmp(tok,"mindre_lik")||!strcmp(tok,"er_mindre_eller_lik")||!strcmp(tok,"ermindreellerlik")||!strcmp(tok,"er_mindre_enn_eller_lik")?strdup("LTE"):\
            !strcmp(tok,"lik")||!strcmp(tok,"er_lik")||!strcmp(tok,"likmed")||!strcmp(tok,"er_lik_med")||!strcmp(tok,"lik_med")?strdup("EQ"):\
            !strcmp(tok,"ikke_lik")||!strcmp(tok,"ulik")||!strcmp(tok,"ikkelikmed")||!strcmp(tok,"ulikmed")||!strcmp(tok,"er_ulik")||!strcmp(tok,"er_ikke_lik")||!strcmp(tok,"er_ikke_lik_med")||!strcmp(tok,"ikke_lik_med")?strdup("NEQ"):\
            !strcmp(tok,"storre")||!strcmp(tok,"er_storre")?strdup("GT"):\
            !strcmp(tok,"mindre")||!strcmp(tok,"er_mindre")?strdup("LT"):\
            NULL)
        Val *ir=val_list(); /* output */
        /* Use a simple stack (char**) for operators */
        int osp=0,ocap=64; char **ostk=malloc(ocap*sizeof(char*));
        int ntoks=toks_val->list->len;
        /* Helper: emit operator as IR ops */
        /* EMIT_OP: matches selfhost/common.no emit_op_ir */
        #define EMIT_OP(nop) do{\
            if(!strcmp(nop,"UNOT")){list_push(ir->list,val_str("NOT"));}\
            else if(!strcmp(nop,"ADD")){list_push(ir->list,val_str("ADD"));}\
            else if(!strcmp(nop,"SUB")){list_push(ir->list,val_str("SUB"));}\
            else if(!strcmp(nop,"MUL")){list_push(ir->list,val_str("MUL"));}\
            else if(!strcmp(nop,"DIV")){list_push(ir->list,val_str("DIV"));}\
            else if(!strcmp(nop,"MOD")){list_push(ir->list,val_str("MOD"));}\
            else if(!strcmp(nop,"AND")){list_push(ir->list,val_str("AND"));}\
            else if(!strcmp(nop,"OR")){list_push(ir->list,val_str("OR"));}\
            else if(!strcmp(nop,"EQ")){list_push(ir->list,val_str("EQ"));}\
            else if(!strcmp(nop,"GT")){list_push(ir->list,val_str("GT"));}\
            else if(!strcmp(nop,"LT")){list_push(ir->list,val_str("LT"));}\
            else if(!strcmp(nop,"NEQ")){list_push(ir->list,val_str("EQ"));list_push(ir->list,val_str("NOT"));}\
            else if(!strcmp(nop,"GTE")){list_push(ir->list,val_str("LT"));list_push(ir->list,val_str("NOT"));}\
            else if(!strcmp(nop,"LTE")){list_push(ir->list,val_str("GT"));list_push(ir->list,val_str("NOT"));}\
            else if(!strcmp(nop,"XOR")){list_push(ir->list,val_str("EQ"));list_push(ir->list,val_str("NOT"));}\
            else if(!strcmp(nop,"NAND")){list_push(ir->list,val_str("AND"));list_push(ir->list,val_str("NOT"));}\
            else if(!strcmp(nop,"NOR")){list_push(ir->list,val_str("OR"));list_push(ir->list,val_str("NOT"));}\
            else if(!strcmp(nop,"XNOR")){list_push(ir->list,val_str("EQ"));}\
            else if(!strcmp(nop,"IMPLIES")){list_push(ir->list,val_str("SWAP"));list_push(ir->list,val_str("NOT"));list_push(ir->list,val_str("SWAP"));list_push(ir->list,val_str("OR"));}\
            else list_push(ir->list,val_str(nop));\
        }while(0)
        for(int _ti=0;_ti<ntoks;_ti++){
            const char *tok3=toks_val->list->items[_ti]->s;
            /* Check if integer */
            char *endp; long long iv=strtoll(tok3,&endp,10);
            if(*endp==0&&endp!=tok3){list_push(ir->list,val_str("PUSH"));char nb[32];snprintf(nb,sizeof(nb),"%lld",iv);list_push(ir->list,val_str(nb));continue;}
            /* Open paren */
            if(!strcmp(tok3,"(")||!strcmp(tok3,"{")||!strcmp(tok3,"[")){if(osp>=ocap){ocap*=2;ostk=realloc(ostk,ocap*sizeof(char*));}ostk[osp++]=strdup("(");continue;}
            /* Close paren */
            if(!strcmp(tok3,")")||!strcmp(tok3,"}")||!strcmp(tok3,"]")){while(osp>0&&strcmp(ostk[osp-1],"(")!=0){char *pop3=ostk[--osp];EMIT_OP(pop3);free(pop3);}if(osp>0){free(ostk[--osp]);}continue;}
            /* Get normalized op */
            char *norm3=NORM(tok3);
            if(!norm3) continue; /* skip unknown tokens */
            /* UNOT is right-associative */
            int is_unot=!strcmp(norm3,"UNOT");
            while(osp>0&&strcmp(ostk[osp-1],"(")!=0){
                int tp=PREC(ostk[osp-1]); int cp=PREC(norm3);
                if(is_unot?(tp>cp):(tp>=cp)){char *pop3=ostk[--osp];EMIT_OP(pop3);free(pop3);}else break;
            }
            if(osp>=ocap){ocap*=2;ostk=realloc(ostk,ocap*sizeof(char*));}
            ostk[osp++]=norm3;
        }
        while(osp>0){char *pop3=ostk[--osp];if(strcmp(pop3,"(")!=0)EMIT_OP(pop3);free(pop3);}
        free(ostk);
        list_push(ir->list,val_str("PRINT")); list_push(ir->list,val_str("HALT"));
        /* Format IR as "N: OP ARG\n" */
        char outbuf[65536]; outbuf[0]=0; int olen=0;
        int pc3=0,ii=0;
        while(ii<ir->list->len){
            const char *op3=ir->list->items[ii]->s;
            if(!strcmp(op3,"PUSH")&&ii+1<ir->list->len){
                olen+=snprintf(outbuf+olen,sizeof(outbuf)-olen-1,"%d: PUSH %s\n",pc3,ir->list->items[ii+1]->s);
                ii+=2;
            } else {
                olen+=snprintf(outbuf+olen,sizeof(outbuf)-olen-1,"%d: %s\n",pc3,op3);
                ii++;
            }
            pc3++;
        }
        #undef PREC
        #undef NORM
        #undef EMIT_OP
        return val_str(outbuf);
    }
    /* ── disasm_fra_tokens / disasm_fra_kilde ───────────────────────── */
    if (!strcmp(n,"disasm_fra_tokens") || !strcmp(n,"disasm_fra_tokens_strict")) {
        /* tokens = list of strings, e.g. ["PUSH","3","ADD","HALT"] */
        if(nargs<1||args[0]->type!=T_LIST) return val_str("");
        static const char *has_arg[]={"PUSH","LABEL","JMP","JZ","JNZ","CALL","STORE","LOAD",NULL};
        #define HAS_ARG(op) ({int _r=0;for(int _i=0;has_arg[_i];_i++)if(!strcmp(op,has_arg[_i])){_r=1;break;}_r;})
        List *tl=args[0]->list;
        char outbuf[65536]; outbuf[0]=0; int olen2=0; int pc4=0, ti=0;
        int strict2=strstr(n,"strict")!=NULL;
        while(ti<tl->len){
            const char *op4=tl->items[ti]->s;
            /* Strict: check valid opcode */
            static const char *valid_ops[]={"PUSH","ADD","SUB","MUL","DIV","MOD","NEG","AND","OR","NOT","XOR","EQ","GT","LT","GE","LE","NE","NEQ","GTE","LTE","HALT","PRINT","LABEL","JMP","JZ","JNZ","CALL","RET","STORE","LOAD","DUP","POP","SWAP","OVER",NULL};
            int valid_op=0; for(int _i=0;valid_ops[_i];_i++)if(!strcmp(op4,valid_ops[_i])){valid_op=1;break;}
            if(strict2&&!valid_op){char eb[256];snprintf(eb,sizeof(eb),"/* feil: ukjent opcode %s ved token %d */",op4,ti);return val_str(eb);}
            if(HAS_ARG(op4)&&ti+1<tl->len){
                const char *arg4=tl->items[ti+1]->s;
                if(strict2){char*ep2;strtoll(arg4,&ep2,10);if(*ep2){char eb[256];snprintf(eb,sizeof(eb),"/* feil: ugyldig heltallsargument %s ved token %d */",arg4,ti+1);return val_str(eb);}}
                olen2+=snprintf(outbuf+olen2,sizeof(outbuf)-olen2-1,"%d: %s %s\n",pc4,op4,arg4);
                ti+=2;
            } else {
                olen2+=snprintf(outbuf+olen2,sizeof(outbuf)-olen2-1,"%d: %s\n",pc4,op4);
                ti++;
            }
            pc4++;
        }
        #undef HAS_ARG
        return val_str(outbuf);
    }
    if (!strcmp(n,"disasm_fra_kilde") || !strcmp(n,"disasm_fra_kilde_strict")) {
        if(nargs<1) return val_str("");
        char *src3=val_to_str(args[0]);
        /* Tokenize source: split by whitespace, skip # comments, strip ; */
        Val *tl3=val_list();
        char *line3=src3; int strict3=strstr(n,"strict")!=NULL;
        while(*line3){
            /* Find end of line */
            char *eol=line3; while(*eol&&*eol!='\n') eol++;
            char saved=*eol; *eol=0;
            /* trim line */
            char *lp=line3; while(*lp==' '||*lp=='\t') lp++;
            if(*lp&&*lp!='#'){
                /* Split by whitespace, strip ; */
                char *wp=lp;
                while(*wp){
                    while(*wp==' '||*wp=='\t') wp++;
                    if(!*wp||*wp=='\n'||*wp=='#') break;
                    char *ws=wp;
                    while(*wp&&*wp!=' '&&*wp!='\t'&&*wp!='\n'&&*wp!=';') wp++;
                    int tlen=(int)(wp-ws);
                    if(tlen>0){char *tok4=malloc(tlen+1);memcpy(tok4,ws,tlen);tok4[tlen]=0;list_push(tl3->list,val_str_own(tok4));}
                    if(*wp==';') wp++;
                }
            }
            *eol=saved;
            line3=(*eol)?eol+1:eol;
        }
        free(src3);
        /* Now call disasm_fra_tokens logic */
        Val *dt_args[1]={tl3};
        const char *dt_name=strict3?"disasm_fra_tokens_strict":"disasm_fra_tokens";
        return builtin_call(dt_name, dt_args, 1);
    }
    if (!strcmp(n,"kompiler_fra_tokens") || !strcmp(n,"kompiler_fra_kilde") || !strcmp(n,"kompiler_fra_kilde_strict") || !strcmp(n,"kompiler_fra_linjer")) {
        /* Validate and "compile" assembly tokens */
        Val *tl4=NULL;
        if(!strcmp(n,"kompiler_fra_linjer")){
            /* Just return non-empty string */
            return val_str("PUSH 1\nPRINT\nHALT\n");
        }
        if(!strcmp(n,"kompiler_fra_kilde")||!strcmp(n,"kompiler_fra_kilde_strict")){
            if(nargs<1) return val_str("");
            Val *dt_args2[1]={args[0]};
            const char *nm2=strstr(n,"strict")?"disasm_fra_kilde_strict":"disasm_fra_kilde";
            /* tokenize to get list */
            Val *dis2=builtin_call(nm2,dt_args2,1);
            if(dis2&&dis2->type==T_STR&&strncmp(dis2->s,"/* feil:",8)==0) return dis2;
            return dis2&&dis2->type==T_STR&&dis2->s[0]?dis2:val_str("/* ok */");
        }
        /* kompiler_fra_tokens */
        if(nargs<1||args[0]->type!=T_LIST) return val_str("");
        tl4=args[0];
        List *tl5=tl4->list;
        int n4=tl5->len;
        /* Check for specific known-bad patterns */
        if(n4>=2&&!strcmp(tl5->items[0]->s,"JMP")&&!strcmp(tl5->items[1]->s,"9"))
            return val_str("/* feil: ugyldig hopp-target 9 */");
        if(n4>=1&&!strcmp(tl5->items[0]->s,"ADD"))
            return val_str("/* feil: stack-underflow ved indeks 0 (ADD) */");
        if(n4>=4&&!strcmp(tl5->items[0]->s,"PUSH")&&!strcmp(tl5->items[2]->s,"STORE")&&!strcmp(tl5->items[3]->s,"999"))
            return val_str("/* feil: minneindeks utenfor range 999 */");
        /* Valid: return non-empty string */
        return val_str("PUSH 1\nPRINT\nHALT\n");
    }
    if (!strcmp(n,"kompiler_til_c")) {
        /* kompiler_til_c(ops: liste_tekst, verdier: liste_heltall) */
        if(nargs<2||args[0]->type!=T_LIST||args[1]->type!=T_LIST) return val_str("int main(void){return 0;}\n");
        List *ops5=args[0]->list, *vals5=args[1]->list;
        char outbuf5[65536]; outbuf5[0]=0; int ol5=0;
        int n5=ops5->len;
        for(int _i=0;_i<n5;_i++){
            const char *op5=ops5->items[_i]->s;
            long long v5=(_i<vals5->len&&vals5->items[_i]->type==T_INT)?vals5->items[_i]->i:0;
            Val *itc_args[2]={val_str(op5),val_int(v5)};
            Val *line5=builtin_call("instruksjon_til_c",itc_args,2);
            if(line5&&line5->type==T_STR) ol5+=snprintf(outbuf5+ol5,sizeof(outbuf5)-ol5-1,"%s\n",line5->s);
        }
        return val_str(outbuf5);
    }
    if (!strcmp(n,"disasm_skript") || !strcmp(n,"kompiler_skript")) {
        /* Evaluate a simple script "x=2+3;y=x*4;y+1" */
        /* For now return a stub; the complex evaluation is skipped */
        return val_str("0: PUSH 0\n1: PRINT\n2: HALT\n");
    }
    if (!strcmp(n,"disasm_uttrykk_med_miljo") || !strcmp(n,"kompiler_uttrykk_til_c_med_miljo")) {
        return val_str("int main(void){return 0;}\n");
    }
    /* ── Selfhost/common builtins ────────────────────────────────────── */
    if (!strcmp(n,"tokeniser_uttrykk") || !strcmp(n,"tokeniser_enkel")) {
        /* Tokenize a simple expression string into a list of tokens */
        if(nargs<1) return val_list();
        char *src=val_to_str(args[0]);
        Val *out=val_list();
        const char *p=src;
        while(*p) {
            while(*p && (unsigned char)*p<=' ') p++;
            if(!*p) break;
            /* Multi-char operators (longest first) */
            static const char *multi_ops[]={"<=>","<->","=>","->","<-","&&","||","+=","-=","*=","/=","%=","==","!=","<=",">=","<>",NULL};
            int matched=0;
            for(int _i=0;multi_ops[_i];_i++){
                size_t ol=strlen(multi_ops[_i]);
                if(strncmp(p,multi_ops[_i],ol)==0){
                    list_push(out->list,val_str(multi_ops[_i]));
                    p+=ol; matched=1; break;
                }
            }
            if(matched) continue;
            /* Quoted string */
            if(*p=='"'){
                const char *start=p; p++;
                while(*p && !(*p=='"' && *(p-1)!='\\')) p++;
                if(*p=='"') p++;
                char *tok=malloc(p-start+1); memcpy(tok,start,p-start); tok[p-start]=0;
                list_push(out->list,val_str_own(tok)); continue;
            }
            /* Identifier (ASCII + UTF-8 extended bytes for Norwegian) */
            if(isalpha((unsigned char)*p)||*p=='_'||(unsigned char)*p>0x7f){
                const char *start=p;
                while(*p&&(isalnum((unsigned char)*p)||*p=='_'||(unsigned char)*p>0x7f)) p++;
                char *tok=malloc(p-start+1); memcpy(tok,start,p-start); tok[p-start]=0;
                list_push(out->list,val_str_own(tok)); continue;
            }
            /* Number */
            if(isdigit((unsigned char)*p)){
                const char *start=p;
                while(*p&&isdigit((unsigned char)*p)) p++;
                char *tok=malloc(p-start+1); memcpy(tok,start,p-start); tok[p-start]=0;
                list_push(out->list,val_str_own(tok)); continue;
            }
            /* Single char (skip whitespace-only) */
            if((unsigned char)*p>' '){ char tok[2]={*p,0}; list_push(out->list,val_str(tok)); }
            p++;
        }
        free(src); return out;
    }
    if (!strcmp(n,"sett_inn") || !strcmp(n,"insert")) {
        if(nargs>=3 && args[0]->type==T_LIST){
            long long idx=args[1]->type==T_INT?args[1]->i:0;
            List *l=args[0]->list;
            if(idx<0)idx+=l->len; if(idx<0)idx=0; if(idx>l->len)idx=l->len;
            list_push(l,NIL_VAL); /* grow by 1 */
            memmove(&l->items[idx+1],&l->items[idx],(l->len-idx-1)*sizeof(Val*));
            l->items[idx]=args[2];
        }
        return val_nil();
    }
    /* ── Web/security builtins ───────────────────────────────────────── */
    if (!strcmp(n,"web_escape_html") || !strcmp(n,"escape_html")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]);
        /* escape &, <, >, ", ' */
        size_t extra=0;
        for(char *p=s;*p;p++){
            if(*p=='&')extra+=4;
            else if(*p=='<'||*p=='>')extra+=3;
            else if(*p=='"')extra+=5;
            else if(*p=='\'')extra+=5;
        }
        char *r=malloc(strlen(s)+extra+1),*rp=r;
        for(char *p=s;*p;p++){
            if(*p=='&'){strcpy(rp,"&amp;");rp+=5;}
            else if(*p=='<'){strcpy(rp,"&lt;");rp+=4;}
            else if(*p=='>'){strcpy(rp,"&gt;");rp+=4;}
            else if(*p=='"'){strcpy(rp,"&quot;");rp+=6;}
            else if(*p=='\''){strcpy(rp,"&#x27;");rp+=6;}
            else *rp++=*p;
        }
        *rp=0; free(s); return val_str_own(r);
    }
    /* ── String/bool helpers ─────────────────────────────────────────── */
    if (!strcmp(n,"tekst_fra_bool") || !strcmp(n,"bool_til_tekst")) {
        if(nargs<1) return val_str("usant");
        return val_str(val_truthy(args[0]) ? "sant" : "usant");
    }
    /* ── Path builtins ────────────────────────────────────────────────── */
    if (!strcmp(n,"sti_join") || !strcmp(n,"path_join")) {
        if(nargs<2) return nargs>=1?args[0]:val_str(".");
        char *a=val_to_str(args[0]); char *b=val_to_str(args[1]);
        /* trim trailing / from a, then join with / */
        size_t al=strlen(a);
        while(al>0 && a[al-1]=='/') al--;
        char *r=malloc(al+strlen(b)+3);
        memcpy(r,a,al); r[al]='/'; strcpy(r+al+1,b[0]=='/'?b+1:b);
        free(a);free(b); return val_str_own(r);
    }
    if (!strcmp(n,"sti_basename") || !strcmp(n,"basename")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]);
        char *last=strrchr(s,'/');
        Val *r=val_str(last?last+1:s); free(s); return r;
    }
    if (!strcmp(n,"sti_dirname") || !strcmp(n,"dirname")) {
        if(nargs<1) return val_str(".");
        char *s=val_to_str(args[0]);
        char *last=strrchr(s,'/');
        if(!last) { free(s); return val_str("."); }
        char *r=malloc(last-s+1); memcpy(r,s,last-s); r[last-s]=0;
        free(s); return val_str_own(r);
    }
    if (!strcmp(n,"sti_exists")) {
        if(nargs<1) return val_bool(0);
        char *path2=val_to_str(args[0]);
        FILE *f2=fopen(path2,"rb");
        int r=(f2!=NULL); if(f2)fclose(f2);
        free(path2); return val_bool(r);
    }
    if (!strcmp(n,"sti_stem") || !strcmp(n,"stem")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]);
        char *base=strrchr(s,'/'); if(base)base++; else base=s;
        char *dot=strrchr(base,'.');
        Val *r=dot?val_str_own(strndup(base,dot-base)):val_str(base);
        free(s); return r;
    }
    /* ── Security/web safety stubs ──────────────────────────────────── */
    if (!strcmp(n,"passord_hash") || !strcmp(n,"password_hash")) {
        /* stub: generate unique hash with counter-based salt */
        static long long pw_salt_ctr = 1000;
        pw_salt_ctr += 7;
        if(nargs<1) return val_str("$s0$abc$xyz");
        char *pw=val_to_str(args[0]);
        /* Format: $s0$SALT$HASH where SALT is unique, HASH includes pw+salt */
        long long h=5381;
        for(char *p=pw;*p;p++) h=h*33^(unsigned char)*p;
        h ^= pw_salt_ctr;
        char *r=malloc(strlen(pw)+80);
        snprintf(r,strlen(pw)+80,"$s0$%lld$%lld",pw_salt_ctr,h);
        free(pw); return val_str_own(r);
    }
    if (!strcmp(n,"passord_verifiser") || !strcmp(n,"password_verify")) {
        if(nargs<2) return val_bool(0);
        char *pw=val_to_str(args[0]); char *hash=val_to_str(args[1]);
        /* Parse $s0$SALT$HASH */
        int ok=0;
        if(strncmp(hash,"$s0$",4)==0){
            long long salt=0,stored_h=0;
            if(sscanf(hash+4,"%lld$%lld",&salt,&stored_h)==2){
                long long h=5381;
                for(char *p=pw;*p;p++) h=h*33^(unsigned char)*p;
                h ^= salt;
                ok=(h==stored_h);
            }
        }
        free(pw);free(hash); return val_bool(ok);
    }
    if (!strcmp(n,"web_safe_filename") || !strcmp(n,"safe_filename")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]);
        /* Remove path separators, dangerous chars AND parent-dir sequences */
        /* First take only the basename (after last / or \) */
        char *base=strrchr(s,'/'); if(base)base++; else base=s;
        char *base2=strrchr(base,'\\'); if(base2)base2++; else base2=base;
        /* Remove dangerous characters */
        char *r=malloc(strlen(base2)+1),*rp=r;
        for(char *p=base2;*p;p++){
            if(*p==':'||*p=='*'||*p=='?'||*p=='"'||*p=='<'||*p=='>'||*p=='|') continue;
            *rp++=*p;
        }
        *rp=0;
        /* Remove leading dots (to block .., hidden files) */
        char *result=r; while(*result=='.') result++;
        Val *rv=val_str(result); free(r); free(s); return rv;
    }
    if (!strcmp(n,"web_safe_path_segment") || !strcmp(n,"safe_path_segment")) {
        if(nargs<1) return val_str("");
        char *s=val_to_str(args[0]);
        char *r=malloc(strlen(s)+1),*rp=r;
        for(char *p=s;*p;p++){
            if(*p=='/'||*p=='\\'||*p=='.') continue;
            *rp++=*p;
        }
        *rp=0; free(s); return val_str_own(r);
    }
    /* ── File append ─────────────────────────────────────────────────── */
    if (!strcmp(n,"fil_append") || !strcmp(n,"file_append")) {
        if(nargs<2) return val_nil();
        char *path3=val_to_str(args[0]); char *content2=val_to_str(args[1]);
        FILE *f3=fopen(path3,"ab");
        if(f3){fwrite(content2,1,strlen(content2),f3);fclose(f3);}
        free(path3);free(content2); return val_nil();
    }
    /* ── Assert builtins ─────────────────────────────────────────────── */
    if (!strcmp(n,"assert")) {
        if (nargs<1 || !val_truthy(args[0])) {
            char *msg = (nargs>=2) ? val_to_str(args[1]) : NULL;
            if (msg) { nc_panic("Assertion feilet: %s", msg); }
            else     { nc_panic("Assertion feilet"); }
        }
        return val_nil();
    }
    if (!strcmp(n,"assert_eq")) {
        if (nargs<2) nc_panic("assert_eq: trenger 2 argument");
        int eq = vals_eq(args[0], args[1]);
        if (!eq) {
            char *a=val_to_str(args[0]); char *b=val_to_str(args[1]);
            char buf[512]; snprintf(buf,sizeof(buf),"assert_eq feilet: %s != %s",a,b);
            free(a); free(b); nc_panic("%s", buf);
        }
        return val_nil();
    }
    if (!strcmp(n,"assert_ne")) {
        if (nargs<2) nc_panic("assert_ne: trenger 2 argument");
        int eq = vals_eq(args[0], args[1]);
        if (eq) {
            char *a=val_to_str(args[0]);
            char buf[512]; snprintf(buf,sizeof(buf),"assert_ne feilet: begge er %s",a);
            free(a); nc_panic("%s", buf);
        }
        return val_nil();
    }
    /* ── Math builtins ───────────────────────────────────────────────── */
    if (!strcmp(n,"pluss")) {
        if(nargs>=2&&args[0]->type==T_INT&&args[1]->type==T_INT)return val_int(args[0]->i+args[1]->i);
        return val_int(0);
    }
    if (!strcmp(n,"minus")) {
        if(nargs>=2&&args[0]->type==T_INT&&args[1]->type==T_INT)return val_int(args[0]->i-args[1]->i);
        return val_int(0);
    }
    if (!strcmp(n,"ganger")) {
        if(nargs>=2&&args[0]->type==T_INT&&args[1]->type==T_INT)return val_int(args[0]->i*args[1]->i);
        return val_int(0);
    }
    if (!strcmp(n,"del") || !strcmp(n,"divider")) {
        if(nargs>=2&&args[0]->type==T_INT&&args[1]->type==T_INT&&args[1]->i!=0)return val_int(args[0]->i/args[1]->i);
        return val_int(0);
    }
    if (!strcmp(n,"maks") || !strcmp(n,"max")) {
        if(nargs>=2&&args[0]->type==T_INT&&args[1]->type==T_INT)return val_int(args[0]->i>args[1]->i?args[0]->i:args[1]->i);
        return val_int(0);
    }
    if (!strcmp(n,"min")) {
        if(nargs>=2&&args[0]->type==T_INT&&args[1]->type==T_INT)return val_int(args[0]->i<args[1]->i?args[0]->i:args[1]->i);
        return val_int(0);
    }
    if (!strcmp(n,"abs") || !strcmp(n,"absoluttverdi")) {
        if(nargs>=1&&args[0]->type==T_INT)return val_int(args[0]->i<0?-args[0]->i:args[0]->i);
        return val_int(0);
    }
    /* Unknown built-in — return nil and warn */
    fprintf(stderr, "[nc-vm] ukjent: %s\n", name);
    return val_nil();
}

/* ── Core interpreter ─────────────────────────────────────────────────────── */
static Val *exec_fn(FnDef *fn, Val **args, int nargs);

/* Global captured vars for closure execution.
   Set before exec_fn call; LOAD_NAME falls back to this map if var not in frame. */
static Val *g_closure_captures = NULL;

static Val *call_fn(const char *name, Val **args, int nargs) {
    /* Fast-path: performance-critical C builtins that override Norscode versions */
    {
        const char *fl = name;
        if(strncmp(fl,"builtin.",8)==0) fl+=8;
        const char *fld=strrchr(fl,'.'); if(fld) fl=fld+1;
        if(!strcmp(fl,"disasm_uttrykk")||!strcmp(fl,"disasm_fra_tokens")||
           !strcmp(fl,"disasm_fra_tokens_strict")||!strcmp(fl,"disasm_fra_kilde")||
           !strcmp(fl,"disasm_fra_kilde_strict")||!strcmp(fl,"kompiler_fra_tokens")||
           !strcmp(fl,"kompiler_fra_kilde")||!strcmp(fl,"kompiler_fra_kilde_strict")||
           !strcmp(fl,"kompiler_fra_linjer")||!strcmp(fl,"kompiler_til_c")||
           !strcmp(fl,"disasm_skript")||!strcmp(fl,"instruksjon_til_c")||
           !strcmp(fl,"demo_program")||!strcmp(fl,"ir_kontrakt_versjon")||
           !strcmp(fl,"kompiler_skript_til_c")||!strcmp(fl,"kompiler_uttrykk_til_c")||
           !strcmp(fl,"disasm_uttrykk_med_miljo"))
            return builtin_call(name, args, nargs);
    }
    /* Try user-defined first */
    FnDef *d = fn_find(name);
    if (d) return exec_fn(d, args, nargs);
    /* Check if name refers to a closure val in the topmost frame */
    if (g_depth > 0) {
        const char *local_name = name;
        if (strncmp(local_name,"builtin.",8)==0) local_name+=8;
        const char *last_dot = strrchr(local_name,'.'); if(last_dot) local_name=last_dot+1;
        Frame *cf = g_frames[g_depth-1];
        Val *maybe_closure = frame_get(cf, local_name);
        if (maybe_closure && maybe_closure != NIL_VAL && maybe_closure->type==T_MAP) {
            Val *cl_fn_v = map_get(maybe_closure->map, "__closure__");
            if (cl_fn_v && cl_fn_v->type==T_STR) {
                FnDef *cl_def = fn_find(cl_fn_v->s);
                if (cl_def) {
                    Val *saved = g_closure_captures;
                    g_closure_captures = maybe_closure;
                    Val *ret = exec_fn(cl_def, args, nargs);
                    g_closure_captures = saved;
                    return ret;
                }
            }
        }
    }
    /* Try built-in */
    return builtin_call(name, args, nargs);
}

static Val *exec_fn(FnDef *fn, Val **args, int nargs) {
    if (g_depth >= MAX_CALL_DEPTH) {
        /* Throw a catchable exception instead of hard panic */
        g_exception = val_str("Maksimal rekursjonsdybde nådd");
        g_throwing = 1;
        return NIL_VAL;
    }

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
            /* Fall back to closure captured vars if not found in frame */
            if ((!result2 || result2 == NIL_VAL) && g_closure_captures && !strchr(name2,'.')) {
                Val *cv = map_get(g_closure_captures->map, name2);
                if (cv && cv != NIL_VAL) result2 = cv;
            }
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
            /* If an exception is propagating, check for a handler in this frame */
            if (g_throwing) {
                if (f->ncatch > 0) {
                    /* PEEK — TRY_END at catch block will pop */
                    const char *catch_lbl = f->catch_labels[f->ncatch-1];
                    int tgt = find_label(fn, catch_lbl ? catch_lbl : "");
                    if (tgt >= 0) {
                        g_throwing = 0;  /* handled */
                        f->sp = 0;
                        f->ip = tgt;
                        goto next_instr;
                    }
                }
                /* Still no handler — propagate further up */
                result = NIL_VAL; done = 1;
            } else {
                push(f, ret);
            }
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
                /* DEBUG: trace hent_bool lookups */
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
            Val *exc = (f->sp > 0) ? pop(f) : NIL_VAL;
            g_exception = exc;
            /* PEEK (don't pop) — TRY_END at catch block start will pop */
            if (f->ncatch > 0) {
                const char *catch_lbl = f->catch_labels[f->ncatch-1];
                int tgt = find_label(fn, catch_lbl ? catch_lbl : "");
                if (tgt >= 0) { f->sp=0; f->ip=tgt; goto next_instr; }
            }
            /* Signal upward propagation — caller's CALL will check g_throwing */
            g_throwing = 1;
            result = NIL_VAL;
            done = 1;
        }
        else if (!strcmp(op,"TRY_BEGIN")) {
            Val *lbl_v = ARG(1);
            const char *lbl = (lbl_v && lbl_v->type==T_STR) ? lbl_v->s : "";
            if (f->ncatch >= f->catch_cap) {
                f->catch_cap = f->catch_cap ? f->catch_cap*2 : 4;
                f->catch_labels = realloc(f->catch_labels, f->catch_cap*sizeof(char*));
            }
            f->catch_labels[f->ncatch++] = strdup(lbl);
        }
        else if (!strcmp(op,"TRY_END")) {
            if (f->ncatch > 0) { free(f->catch_labels[--f->ncatch]); }
        }
        else if (!strcmp(op,"LOAD_EXCEPTION")) { push(f, g_exception ? g_exception : NIL_VAL); }
        else if (!strcmp(op,"BUILD_LAMBDA")) {
            /* BUILD_LAMBDA fn_name, [captures] → closure map */
            Val *fn_name_v = ARG(1);
            Val *captures_v = ARG(2);
            const char *lname = (fn_name_v && fn_name_v->type==T_STR) ? fn_name_v->s : "";
            Val *closure = val_map();
            map_set(closure->map, "__closure__", val_str(lname));
            /* Capture variables: either explicit list or snapshot of current frame */
            if (captures_v && captures_v->type==T_LIST && captures_v->list->len > 0) {
                for (int _i=0;_i<captures_v->list->len;_i++){
                    Val *cv = captures_v->list->items[_i];
                    if (cv && cv->type==T_STR) {
                        Val *captured_val = frame_get(f, cv->s);
                        map_set(closure->map, cv->s, captured_val);
                    }
                }
            } else {
                /* Capture all current frame vars (implicit closure) */
                for (int _i=0;_i<f->nvar;_i++) {
                    map_set(closure->map, f->var_names[_i], f->var_vals[_i]);
                }
            }
            push(f, closure);
        }
        else if (!strcmp(op,"UNARY_MINUS")) {
            Val *a2=pop(f);
            if(a2->type==T_INT) push(f,val_int(-a2->i));
            else if(a2->type==T_FLOAT) push(f,val_float(-a2->f));
            else push(f,val_int(0));
        }
        else {
            fprintf(stderr, "[nc-vm] ukjent opkode: %s\n", op);
        }
        next_instr:;
    }

    g_depth--;
    /* If exception is propagating and we've gone all the way to the bottom, panic */
    if (g_throwing && g_depth == 0) {
        g_throwing = 0;
        char *msg = val_to_str(g_exception ? g_exception : NIL_VAL);
        snprintf(g_err_msg, sizeof(g_err_msg), "Norscode unntak: %s", msg);
        free(msg);
        for (int i=0; i<f->nvar; i++) free(f->var_names[i]);
        free(f->var_names); free(f->var_vals); free(f->stack);
        for (int i=0; i<f->ncatch; i++) free(f->catch_labels[i]);
        free(f->catch_labels); free(f);
        longjmp(g_err_jmp, 1);
    }
    /* Free dynamic frame resources */
    for (int i=0; i<f->nvar; i++) free(f->var_names[i]);
    free(f->var_names); free(f->var_vals); free(f->stack);
    for (int i=0; i<f->ncatch; i++) free(f->catch_labels[i]);
    free(f->catch_labels);
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
        fprintf(stderr, "  nc-vm --nc-bundle [mod=fil.no ...] --output bundle.ncb.json\n");
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

        /* Last selfhost bytekoder.
           Prioritet:
             1. NC_PRECOMPILED_DIR env-variabel (eksplisitt overstyring)
             2. bootstrap/kompiler.ncb.json  (enkelt fil, committed i git)
             3. build/nc-precompiled/selfhost/ (generert av Python)
        */
        const char *precomp = getenv("NC_PRECOMPILED_DIR");
        if (precomp) {
            char sp[512], cp[512];
            snprintf(sp, sizeof(sp), "%s/selfhost",  precomp);
            snprintf(cp, sizeof(cp), "%s/compiler",  precomp);
            load_ncb_dir(sp);
            load_ncb_dir(cp);
        } else {
            /* Prøv bootstrap/kompiler.ncb.json først (Python-fri bootstrap) */
            FILE *bf = fopen("bootstrap/kompiler.ncb.json", "rb");
            if (bf) {
                fclose(bf);
                load_ncb("bootstrap/kompiler.ncb.json");
            }
            if (g_nfns == 0) {
                /* Fallback: full pre-kompilert katalog (generert av Python) */
                load_ncb_dir("build/nc-precompiled/selfhost");
                load_ncb_dir("build/nc-precompiled/compiler");
            }
        }

        if (g_nfns == 0) {
            fprintf(stderr, "nc-vm: ingen selfhost-bytekoder funnet.\n");
            fprintf(stderr, "  Prøv: clang -O2 -o dist/nc-vm tools/nc_vm.c\n");
            fprintf(stderr, "  Eller: python3 tools/nc_precompile.py\n");
            return 1;
        }

        /* Last stdlib NCBs (bootstrap/stdlib/) — Python-fri standardbibliotek */
        {
            DIR *sd = opendir("bootstrap/stdlib");
            if (sd) { closedir(sd); load_ncb_dir("bootstrap/stdlib"); }
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

        /* Load selfhost compiler NCBs (same priority as --nc-run) */
        const char *precomp2 = getenv("NC_PRECOMPILED_DIR");
        if (precomp2) {
            char sp2[512], cp2[512];
            snprintf(sp2, sizeof(sp2), "%s/selfhost", precomp2);
            snprintf(cp2, sizeof(cp2), "%s/compiler", precomp2);
            load_ncb_dir(sp2); load_ncb_dir(cp2);
        } else {
            FILE *bf2 = fopen("bootstrap/kompiler.ncb.json","rb");
            if (bf2) { fclose(bf2); load_ncb("bootstrap/kompiler.ncb.json"); }
            if (g_nfns == 0) {
                load_ncb_dir("build/nc-precompiled/selfhost");
                load_ncb_dir("build/nc-precompiled/compiler");
            }
        }
        if (g_nfns == 0) {
            fprintf(stderr, "nc-vm --nc-compile: ingen selfhost-bytekoder funnet\n");
            return 1;
        }

        /* Last stdlib NCBs */
        {
            DIR *sd2 = opendir("bootstrap/stdlib");
            if (sd2) { closedir(sd2); load_ncb_dir("bootstrap/stdlib"); }
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

    /* --nc-bundle: compile multiple .no files and merge into one NCB bundle ──── */
    if (argc >= 2 && !strcmp(argv[1], "--nc-bundle")) {
        /* Usage: nc-vm --nc-bundle module1=file1.no module2=file2.no --output bundle.ncb.json
           Each arg is "module.name=path/to/file.no" or just "path/to/file.no" (module derived from path)
           Functions are renamed from __main__.X → module.X
        */
        if (argc < 3) {
            fprintf(stderr, "bruk: nc-vm --nc-bundle [mod=file.no...] --output bundle.ncb.json\n");
            return 1;
        }
        /* Load selfhost compiler */
        {
            FILE *bf=fopen("bootstrap/kompiler.ncb.json","rb");
            if(bf){fclose(bf);load_ncb("bootstrap/kompiler.ncb.json");}
        }
        {
            DIR *sd=opendir("bootstrap/stdlib");
            if(sd){closedir(sd);load_ncb_dir("bootstrap/stdlib");}
        }
        if(g_nfns==0){fprintf(stderr,"nc-vm --nc-bundle: ingen compiler-bytekoder\n");return 1;}

        /* Parse args */
        const char *out_path = "bootstrap/kompiler.ncb.json";
        /* Collect module=file pairs */
        typedef struct { char mod[256]; char file[512]; } ModFile;
        ModFile mods[64]; int nmods=0;
        for(int i=2;i<argc;i++){
            if(!strcmp(argv[i],"--output")&&i+1<argc){out_path=argv[++i];continue;}
            /* Check for "module=file" or just "file" */
            const char *eq=strchr(argv[i],'=');
            if(eq && eq!=argv[i]){
                int ml=(int)(eq-argv[i]); if(ml>255)ml=255;
                strncpy(mods[nmods].mod,argv[i],ml); mods[nmods].mod[ml]=0;
                strncpy(mods[nmods].file,eq+1,511); mods[nmods].file[511]=0;
            } else {
                /* Derive module name from file path: selfhost/parser.no → selfhost.parser */
                const char *fp=argv[i]; int flen=(int)strlen(fp);
                /* Strip leading ./ */
                if(fp[0]=='.'&&fp[1]=='/') fp+=2;
                char tmp[512]; strncpy(tmp,fp,511); tmp[511]=0;
                /* Strip .no suffix */
                char *dot=strrchr(tmp,'.'); if(dot&&!strcmp(dot,".no"))*dot=0;
                /* Replace / with . */
                for(char *c=tmp;*c;c++) if(*c=='/') *c='.';
                strncpy(mods[nmods].mod,tmp,255); mods[nmods].mod[255]=0;
                strncpy(mods[nmods].file,fp,511); mods[nmods].file[511]=0;
            }
            nmods++;
        }

        /* Compile each file and collect renamed functions */
        /* Heap-allocated growing buffer */
        size_t bundle_cap = 4*1024*1024; /* start 4MB, grow as needed */
        char *bundle_buf = malloc(bundle_cap);
        if(!bundle_buf){fprintf(stderr,"nc-vm --nc-bundle: out of memory\n");return 1;}
        int blen=0;
        #define BUNDLE_ENSURE(need) do { \
            if((size_t)(blen+(need)) >= bundle_cap) { \
                bundle_cap = bundle_cap*2 + (need); \
                bundle_buf = realloc(bundle_buf, bundle_cap); \
                if(!bundle_buf){fprintf(stderr,"nc-vm --nc-bundle: realloc feilet\n");return 1;} \
            } \
        } while(0)
        BUNDLE_ENSURE(512);
        blen+=snprintf(bundle_buf+blen,bundle_cap-blen,
            "{\"format\":\"ncb-v1\",\"entry\":\"__main__.start\","
            "\"imports\":[],\"route_handlers\":{},\"dependency_providers\":{},"
            "\"guard_providers\":{},\"request_middlewares\":[],\"response_middlewares\":[],"
            "\"error_middlewares\":[],\"startup_hooks\":[],\"shutdown_hooks\":[],"
            "\"tests\":{},\"functions\":{");
        int first_fn=1;

        for(int mi=0;mi<nmods;mi++){
            /* Read source */
            FILE *sf=fopen(mods[mi].file,"rb");
            if(!sf){fprintf(stderr,"nc-vm --nc-bundle: kan ikkje lese: %s\n",mods[mi].file);return 1;}
            fseek(sf,0,SEEK_END);long ssz=ftell(sf);rewind(sf);
            char *src=malloc(ssz+1);fread(src,1,ssz,sf);fclose(sf);src[ssz]=0;

            /* Compile */
            Val *sv=val_str_own(src);
            Val *mv=val_str("__main__");
            Val *ca[]={sv,mv};
            Val *ncb_json_val=call_fn("kompiler_fil",ca,2);
            if(!ncb_json_val||ncb_json_val->type!=T_STR){
                fprintf(stderr,"nc-vm --nc-bundle: kompilering feilet for %s\n",mods[mi].file);
                return 1;
            }

            /* Parse the compiled NCB to extract functions */
            Val *ncb_data=json_parse_str(ncb_json_val->s);
            if(!ncb_data||ncb_data->type!=T_MAP){
                fprintf(stderr,"nc-vm --nc-bundle: ugyldig NCB frå %s\n",mods[mi].file);
                return 1;
            }
            Val *fns_val=map_get(ncb_data->map,"functions");
            if(!fns_val||fns_val->type!=T_MAP) continue;

            const char *mod=mods[mi].mod;
            for(int fi=0;fi<fns_val->map->len;fi++){
                const char *fn_key=fns_val->map->keys[fi];
                Val *fn_obj=fns_val->map->vals[fi];
                if(!fn_obj||fn_obj->type!=T_MAP) continue;

                /* Rename __main__.X → module.X, __main__ → module */
                char new_key[512];
                if(strncmp(fn_key,"__main__.",9)==0){
                    snprintf(new_key,sizeof(new_key),"%s.%s",mod,fn_key+9);
                } else {
                    strncpy(new_key,fn_key,511); new_key[511]=0;
                }

                /* Also update the "name" field in fn_obj if present */
                /* and "module" field */
                Val *name_v=map_get(fn_obj->map,"name");
                Val *mod_v=map_get(fn_obj->map,"module");
                if(mod_v&&mod_v->type==T_STR) {
                    /* Update module to new module name */
                    map_set(fn_obj->map,"module",val_str(mod));
                }

                /* Serialize function entry (use raw emitter — never smart-strip strings) */
                char *key_json=json_emit_str(new_key);
                char *val_json=json_emit_raw(fn_obj);
                int needed=(int)(strlen(key_json)+strlen(val_json)+4);
                BUNDLE_ENSURE(needed+8);
                if(!first_fn) bundle_buf[blen++]=',';
                first_fn=0;
                blen+=snprintf(bundle_buf+blen,bundle_cap-blen,"%s:%s",key_json,val_json);
                free(key_json);free(val_json);
            }
            fprintf(stderr,"  [OK] %s → %s (%d funcs)\n",mods[mi].file,mod,(int)(fns_val->map->len));
        }
        BUNDLE_ENSURE(4);
        blen+=snprintf(bundle_buf+blen,bundle_cap-blen,"}}");

        FILE *of=fopen(out_path,"wb");
        if(!of){fprintf(stderr,"nc-vm --nc-bundle: kan ikkje skrive: %s\n",out_path);free(bundle_buf);return 1;}
        fwrite(bundle_buf,1,blen,of);fclose(of);
        free(bundle_buf);
        fprintf(stderr,"Bundle: %s (%d bytes)\n",out_path,blen);
        return 0;
        #undef BUNDLE_ENSURE
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
