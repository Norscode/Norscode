/*
 * nc_runtime_mini.c — minimal Norscode C-runtime
 *
 * Brukt av generert C-kode frå selfhost/ncb_to_c.no.
 * Build: clang -O2 norscode_generated.c tools/nc_runtime_mini.c -o dist/norscode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <setjmp.h>
#include <errno.h>

/* ── Verditypar ── */
#define NC_NIL   0
#define NC_INT   1
#define NC_STR   2
#define NC_BOOL  3
#define NC_LIST  4
#define NC_MAP   5

typedef struct NcVal NcVal;
typedef struct { NcVal **items; int len, cap; } NcList;
typedef struct { char **keys; NcVal **vals; int len, cap; } NcMap;

struct NcVal {
    int type;
    union {
        long long i;
        double    f;
        char     *s;
        int       b;
        NcList   *list;
        NcMap    *map;
    };
};

/* ── Feilhandtering ── */
static jmp_buf g_err_jmp;
static char    g_err_msg[4096];

static void nc_panic(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(g_err_msg, sizeof(g_err_msg), fmt, ap);
    va_end(ap);
    longjmp(g_err_jmp, 1);
}

/* ── Konstruktørar ── */
static NcVal *nc_nil(void) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_NIL; return v;
}
static NcVal *nc_int(long long i) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_INT; v->i = i; return v;
}
static NcVal *nc_bool(int b) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_BOOL; v->b = b; return v;
}
static NcVal *nc_str(const char *s) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_STR; v->s = strdup(s ? s : ""); return v;
}
static NcVal *nc_str_own(char *s) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_STR; v->s = s; return v;
}
static NcVal *nc_list_new(void) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_LIST;
    v->list = calloc(1, sizeof(NcList)); return v;
}
static NcVal *nc_map_new(void) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_MAP;
    v->map = calloc(1, sizeof(NcMap)); return v;
}

/* ── Stack ── */
static void nc_push(int *sp, NcVal **stack, NcVal *v) {
    if (*sp >= 512) nc_panic("Stack overflow");
    stack[(*sp)++] = v ? v : nc_nil();
}
static NcVal *nc_pop(int *sp, NcVal **stack) {
    if (*sp <= 0) nc_panic("Stack underflow");
    return stack[--(*sp)];
}

/* ── Variabeloppslag ── */
static void nc_store(NcVal **vars, char **varnames, int *nvars, const char *name, NcVal *val) {
    for (int i = 0; i < *nvars; i++) {
        if (!strcmp(varnames[i], name)) { vars[i] = val; return; }
    }
    if (*nvars >= 2048) nc_panic("For mange variablar");
    varnames[*nvars] = strdup(name);
    vars[(*nvars)++] = val;
}
static NcVal *nc_load(NcVal **vars, char **varnames, int nvars, const char *name) {
    for (int i = 0; i < nvars; i++) {
        if (!strcmp(varnames[i], name)) return vars[i];
    }
    if (!strcmp(name, "null") || !strcmp(name, "ingenting")) return nc_nil();
    if (!strcmp(name, "true") || !strcmp(name, "sann"))  return nc_bool(1);
    if (!strcmp(name, "false") || !strcmp(name, "usann")) return nc_bool(0);
    nc_panic("Ukjent variabel: %s", name);
    return nc_nil();
}

/* ── Konvertering ── */
static int nc_truthy(NcVal *v) {
    if (!v || v->type == NC_NIL) return 0;
    if (v->type == NC_BOOL) return v->b;
    if (v->type == NC_INT)  return v->i != 0;
    if (v->type == NC_STR)  return v->s && v->s[0] != 0;
    if (v->type == NC_LIST) return v->list->len > 0;
    return 1;
}

static char *nc_to_str_raw(NcVal *v) {
    if (!v || v->type == NC_NIL) return strdup("ingenting");
    if (v->type == NC_STR)  return strdup(v->s);
    if (v->type == NC_BOOL) return strdup(v->b ? "sann" : "usann");
    if (v->type == NC_INT)  {
        char buf[32]; snprintf(buf, sizeof(buf), "%lld", v->i);
        return strdup(buf);
    }
    return strdup("[verdi]");
}

/* ── Aritmetikk ── */
static NcVal *nc_add(NcVal *a, NcVal *b) {
    if (a->type == NC_STR || b->type == NC_STR) {
        char *sa = nc_to_str_raw(a), *sb = nc_to_str_raw(b);
        char *r = malloc(strlen(sa)+strlen(sb)+1);
        strcpy(r, sa); strcat(r, sb);
        free(sa); free(sb);
        return nc_str_own(r);
    }
    if (a->type == NC_INT && b->type == NC_INT) return nc_int(a->i + b->i);
    return nc_nil();
}
static NcVal *nc_sub(NcVal *a, NcVal *b) {
    if (a->type == NC_INT && b->type == NC_INT) return nc_int(a->i - b->i);
    return nc_nil();
}
static NcVal *nc_mul(NcVal *a, NcVal *b) {
    if (a->type == NC_INT && b->type == NC_INT) return nc_int(a->i * b->i);
    return nc_nil();
}
static NcVal *nc_div(NcVal *a, NcVal *b) {
    if (a->type == NC_INT && b->type == NC_INT) {
        if (b->i == 0) nc_panic("Divisjon med null");
        return nc_int(a->i / b->i);
    }
    return nc_nil();
}
static NcVal *nc_mod(NcVal *a, NcVal *b) {
    if (a->type == NC_INT && b->type == NC_INT) {
        if (b->i == 0) nc_panic("Modulo med null");
        return nc_int(a->i % b->i);
    }
    return nc_nil();
}
static NcVal *nc_lshift(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av<<bv);
}
static NcVal *nc_rshift(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av>>bv);
}
static NcVal *nc_band(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av&bv);
}
static NcVal *nc_bor(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av|bv);
}
static NcVal *nc_bxor(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av^bv);
}
static NcVal *nc_neg(NcVal *a) {
    if (a->type == NC_INT) return nc_int(-a->i);
    return nc_nil();
}

/* ── Samanlikning ── */
static int nc_eq(NcVal *a, NcVal *b) {
    if (!a || !b) return a == b;
    if (a->type == NC_NIL && b->type == NC_NIL) return 1;
    if (a->type == NC_BOOL && b->type == NC_BOOL) return a->b == b->b;
    if (a->type == NC_INT  && b->type == NC_INT)  return a->i == b->i;
    if (a->type == NC_STR  && b->type == NC_STR)  return !strcmp(a->s, b->s);
    return 0;
}
/* mode: -1=LT, 1=GT, -2=LE, 2=GE */
static NcVal *nc_cmp(NcVal *a, NcVal *b, int mode) {
    int r = 0;
    if (a->type == NC_INT && b->type == NC_INT) {
        r = (a->i < b->i) ? -1 : (a->i > b->i) ? 1 : 0;
    } else if (a->type == NC_STR && b->type == NC_STR) {
        r = strcmp(a->s, b->s);
    }
    int ok = (mode == -1) ? r < 0 : (mode == 1) ? r > 0 : (mode == -2) ? r <= 0 : r >= 0;
    return nc_bool(ok);
}

/* ── Liste/ordbok bygg ── */
static NcVal *nc_build_list(int *sp, NcVal **stack, int n) {
    NcVal *lst = nc_list_new();
    lst->list->items = calloc(n, sizeof(NcVal*));
    lst->list->cap = n;
    NcVal **tmp = malloc(n * sizeof(NcVal*));
    for (int i = n-1; i >= 0; i--) tmp[i] = nc_pop(sp, stack);
    for (int i = 0; i < n; i++) {
        lst->list->items[lst->list->len++] = tmp[i];
    }
    free(tmp);
    return lst;
}
static NcVal *nc_build_map(int *sp, NcVal **stack, int n) {
    NcVal *m = nc_map_new();
    for (int i = 0; i < n; i++) {
        NcVal *v = nc_pop(sp, stack), *k = nc_pop(sp, stack);
        char *ks = nc_to_str_raw(k);
        /* resize */
        m->map->keys = realloc(m->map->keys, (m->map->len+1)*sizeof(char*));
        m->map->vals = realloc(m->map->vals, (m->map->len+1)*sizeof(NcVal*));
        m->map->keys[m->map->len] = ks;
        m->map->vals[m->map->len] = v;
        m->map->len++;
    }
    return m;
}

/* ── Indeksering ── */
static NcVal *nc_index_get(NcVal *obj, NcVal *key) {
    if (!obj) return nc_nil();
    if (obj->type == NC_LIST) {
        long long idx = (key->type == NC_INT) ? key->i : atoll(key->s);
        if (idx < 0) idx = obj->list->len + idx;
        if (idx < 0 || idx >= obj->list->len) nc_panic("Indeks utanfor: %lld", idx);
        return obj->list->items[idx];
    }
    if (obj->type == NC_MAP) {
        char *ks = nc_to_str_raw(key);
        for (int i = 0; i < obj->map->len; i++) {
            if (!strcmp(obj->map->keys[i], ks)) { free(ks); return obj->map->vals[i]; }
        }
        free(ks); return nc_nil();
    }
    if (obj->type == NC_STR) {
        long long idx = (key->type == NC_INT) ? key->i : atoll(key->s);
        if (idx < 0) idx = (long long)strlen(obj->s) + idx;
        if (idx < 0 || idx >= (long long)strlen(obj->s)) return nc_str("");
        char buf[2] = {obj->s[idx], 0};
        return nc_str(buf);
    }
    return nc_nil();
}
static void nc_index_set(NcVal *obj, NcVal *key, NcVal *val) {
    if (!obj) return;
    if (obj->type == NC_LIST) {
        long long idx = (key->type == NC_INT) ? key->i : atoll(key->s);
        if (idx < 0) idx = obj->list->len + idx;
        if (idx >= obj->list->len) {
            while (obj->list->len <= idx) {
                if (obj->list->len >= obj->list->cap) {
                    obj->list->cap = obj->list->cap ? obj->list->cap*2 : 8;
                    obj->list->items = realloc(obj->list->items, obj->list->cap*sizeof(NcVal*));
                }
                obj->list->items[obj->list->len++] = nc_nil();
            }
        }
        obj->list->items[idx] = val;
        return;
    }
    if (obj->type == NC_MAP) {
        char *ks = nc_to_str_raw(key);
        for (int i = 0; i < obj->map->len; i++) {
            if (!strcmp(obj->map->keys[i], ks)) { obj->map->vals[i] = val; free(ks); return; }
        }
        obj->map->keys = realloc(obj->map->keys, (obj->map->len+1)*sizeof(char*));
        obj->map->vals = realloc(obj->map->vals, (obj->map->len+1)*sizeof(NcVal*));
        obj->map->keys[obj->map->len] = ks;
        obj->map->vals[obj->map->len] = val;
        obj->map->len++;
    }
}

/* ── Unntak ── */
static char g_throw_msg[4096];
static void nc_throw(const char *msg) {
    strncpy(g_throw_msg, msg ? msg : "ukjent feil", sizeof(g_throw_msg)-1);
    nc_panic("Norscode unntak: %s", g_throw_msg);
}

/* ── Innebygde funksjonar ── */
static NcVal *nc_builtin_lengde(NcVal *v) {
    if (!v) return nc_int(0);
    if (v->type == NC_STR)  return nc_int((long long)strlen(v->s));
    if (v->type == NC_LIST) return nc_int(v->list->len);
    if (v->type == NC_MAP)  return nc_int(v->map->len);
    return nc_int(0);
}
static NcVal *nc_builtin_legg_til(NcVal *lst, NcVal *v) {
    if (!lst || lst->type != NC_LIST) return nc_nil();
    if (lst->list->len >= lst->list->cap) {
        lst->list->cap = lst->list->cap ? lst->list->cap*2 : 8;
        lst->list->items = realloc(lst->list->items, lst->list->cap*sizeof(NcVal*));
    }
    lst->list->items[lst->list->len++] = v;
    return nc_nil();
}
static NcVal *nc_builtin_fjern_siste(NcVal *lst) {
    if (!lst || lst->type != NC_LIST || lst->list->len == 0) return nc_nil();
    return lst->list->items[--lst->list->len];
}
static NcVal *nc_builtin_fjern(NcVal *lst, NcVal *idx_v) {
    if (!lst || lst->type != NC_LIST) return nc_nil();
    long long idx = idx_v->type == NC_INT ? idx_v->i : 0;
    if (idx < 0 || idx >= lst->list->len) return nc_nil();
    NcVal *r = lst->list->items[idx];
    memmove(&lst->list->items[idx], &lst->list->items[idx+1], (lst->list->len-idx-1)*sizeof(NcVal*));
    lst->list->len--;
    return r;
}
static NcVal *nc_builtin_slice(NcVal *v, NcVal *a_v, NcVal *b_v) {
    if (!v) return nc_str("");
    long long a = (a_v && a_v->type==NC_INT) ? a_v->i : 0;
    if (v->type == NC_LIST) {
        long long len = v->list->len;
        long long b = (b_v && b_v->type==NC_INT && b_v->i != -1) ? b_v->i : len;
        if (a < 0) a = 0; if (b < 0 || b > len) b = len; if (b < a) b = a;
        NcVal *r = nc_list_new();
        for (long long i=a; i<b; i++) nc_builtin_legg_til(r, v->list->items[i]);
        return r;
    }
    if (v->type != NC_STR) return nc_str("");
    long long len = (long long)strlen(v->s);
    long long b = (b_v && b_v->type==NC_INT && b_v->i != -1) ? b_v->i : len;
    if (a < 0) a = 0; if (a > len) a = len;
    if (b < a) b = a; if (b > len) b = len;
    char *r = malloc(b-a+1);
    memcpy(r, v->s+a, b-a); r[b-a] = 0;
    return nc_str_own(r);
}
static NcVal *nc_builtin_skriv(NcVal *v) {
    char *s = nc_to_str_raw(v); fputs(s, stdout); free(s); fflush(stdout);
    return nc_nil();
}
static NcVal *nc_builtin_fil_les(NcVal *path_v) {
    char *path = nc_to_str_raw(path_v);
    FILE *f = fopen(path, "rb"); free(path);
    if (!f) nc_panic("Kan ikkje opne fil: %s", path_v->s);
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz+1); fread(buf, 1, sz, f); fclose(f); buf[sz] = 0;
    return nc_str_own(buf);
}
static NcVal *nc_builtin_fil_skriv(NcVal *path_v, NcVal *data_v) {
    char *path = nc_to_str_raw(path_v), *data = nc_to_str_raw(data_v);
    FILE *f = fopen(path, "wb"); free(path);
    if (!f) { free(data); return nc_nil(); }
    fputs(data, f); fclose(f); free(data);
    return nc_nil();
}
static NcVal *nc_builtin_fil_skriv_binary(NcVal *path_v, NcVal *list_v) {
    char *path = nc_to_str_raw(path_v);
    FILE *f = fopen(path, "wb"); free(path);
    if (!f) return nc_nil();
    if (list_v && list_v->type == NC_LIST) {
        for (int i = 0; i < list_v->list->len; i++) {
            NcVal *bv = list_v->list->items[i];
            unsigned char byte = (unsigned char)(bv && bv->type == NC_INT ? (int)bv->i : 0);
            fwrite(&byte, 1, 1, f);
        }
    }
    fclose(f);
    return nc_nil();
}
static NcVal *nc_builtin_fil_finnes(NcVal *path_v) {
    char *path = nc_to_str_raw(path_v);
    FILE *f = fopen(path, "rb"); free(path);
    int r = f != NULL; if (f) fclose(f);
    return nc_bool(r);
}
static NcVal *nc_builtin_miljo_hent(NcVal *k_v) {
    char *k = nc_to_str_raw(k_v); char *v = getenv(k); free(k);
    return nc_str(v ? v : "");
}
static NcVal *nc_builtin_miljo_finnes(NcVal *k_v) {
    char *k = nc_to_str_raw(k_v); int r = getenv(k) != NULL; free(k);
    return nc_bool(r);
}
static NcVal *nc_builtin_finnes_nokkel(NcVal *m, NcVal *k_v) {
    if (!m || m->type != NC_MAP) return nc_bool(0);
    char *k = nc_to_str_raw(k_v);
    for (int i = 0; i < m->map->len; i++) {
        if (!strcmp(m->map->keys[i], k)) { free(k); return nc_bool(1); }
    }
    free(k); return nc_bool(0);
}
static NcVal *nc_builtin_nokler(NcVal *m) {
    NcVal *lst = nc_list_new();
    if (!m || m->type != NC_MAP) return lst;
    for (int i = 0; i < m->map->len; i++) nc_builtin_legg_til(lst, nc_str(m->map->keys[i]));
    return lst;
}
static NcVal *nc_builtin_verdier(NcVal *m) {
    NcVal *lst = nc_list_new();
    if (!m || m->type != NC_MAP) return lst;
    for (int i = 0; i < m->map->len; i++) nc_builtin_legg_til(lst, m->map->vals[i]);
    return lst;
}
static NcVal *nc_builtin_starts_with(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    int r = strncmp(s, p, strlen(p)) == 0; free(s); free(p);
    return nc_bool(r);
}
static NcVal *nc_builtin_ends_with(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    size_t sl = strlen(s), pl = strlen(p);
    int r = sl >= pl && strcmp(s+sl-pl, p) == 0; free(s); free(p);
    return nc_bool(r);
}
static NcVal *nc_builtin_contains(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    int r = strstr(s, p) != NULL; free(s); free(p);
    return nc_bool(r);
}
static NcVal *nc_builtin_split(NcVal *s_v, NcVal *sep_v) {
    char *s = nc_to_str_raw(s_v), *sep = nc_to_str_raw(sep_v);
    NcVal *lst = nc_list_new();
    size_t seplen = strlen(sep);
    if (seplen == 0) {
        for (size_t i = 0; s[i]; i++) {
            char buf[2] = {s[i], 0};
            nc_builtin_legg_til(lst, nc_str(buf));
        }
    } else {
        char *cur = s, *found;
        while ((found = strstr(cur, sep)) != NULL) {
            size_t partlen = found - cur;
            char *part = malloc(partlen+1); memcpy(part, cur, partlen); part[partlen] = 0;
            nc_builtin_legg_til(lst, nc_str_own(part));
            cur = found + seplen;
        }
        nc_builtin_legg_til(lst, nc_str(cur));
    }
    free(s); free(sep);
    return lst;
}
static NcVal *nc_builtin_join(NcVal *lst, NcVal *sep_v) {
    if (!lst || lst->type != NC_LIST) return nc_str("");
    char *sep = nc_to_str_raw(sep_v);
    size_t seplen = strlen(sep);
    int n = lst->list->len;
    size_t total = 0;
    char **parts = malloc(n * sizeof(char*));
    size_t *lens = malloc(n * sizeof(size_t));
    for (int i = 0; i < n; i++) {
        parts[i] = nc_to_str_raw(lst->list->items[i]);
        lens[i] = strlen(parts[i]);
        total += lens[i];
    }
    if (n > 1) total += seplen * (n - 1);
    char *r = malloc(total + 1);
    char *wp = r;
    for (int i = 0; i < n; i++) {
        if (i > 0) { memcpy(wp, sep, seplen); wp += seplen; }
        memcpy(wp, parts[i], lens[i]); wp += lens[i];
        free(parts[i]);
    }
    *wp = 0;
    free(parts); free(lens); free(sep);
    return nc_str_own(r);
}
static NcVal *nc_builtin_trim(NcVal *s_v) {
    char *s = nc_to_str_raw(s_v);
    int a = 0, b = (int)strlen(s);
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b-1])) b--;
    char *r = malloc(b-a+1); memcpy(r, s+a, b-a); r[b-a] = 0;
    free(s); return nc_str_own(r);
}
static NcVal *nc_builtin_replace(NcVal *s_v, NcVal *old_v, NcVal *new_v) {
    char *s = nc_to_str_raw(s_v), *old = nc_to_str_raw(old_v), *newstr = nc_to_str_raw(new_v);
    size_t olen = strlen(old), nlen = strlen(newstr);
    if (olen == 0) { free(old); free(newstr); return nc_str_own(s); }
    /* count */
    int cnt = 0; char *p = s;
    while ((p = strstr(p, old)) != NULL) { cnt++; p += olen; }
    char *r = malloc(strlen(s) + cnt * (nlen > olen ? nlen - olen : 0) + 1);
    char *wp = r; p = s;
    char *found;
    while ((found = strstr(p, old)) != NULL) {
        memcpy(wp, p, found-p); wp += found-p;
        memcpy(wp, newstr, nlen); wp += nlen;
        p = found + olen;
    }
    strcpy(wp, p);
    free(s); free(old); free(newstr);
    return nc_str_own(r);
}
static NcVal *nc_builtin_chr(NcVal *v) {
    int code = v && v->type == NC_INT ? (int)v->i : 0;
    char buf[2] = {(char)(unsigned char)code, 0};
    return nc_str(buf);
}
static NcVal *nc_builtin_char_code(NcVal *v) {
    if (!v || v->type != NC_STR || !v->s || !v->s[0]) return nc_int(0);
    return nc_int((unsigned char)v->s[0]);
}
static NcVal *nc_builtin_tekst_fra_heltall(NcVal *v) {
    char buf[32];
    if (v->type == NC_INT) snprintf(buf, sizeof(buf), "%lld", v->i);
    else snprintf(buf, sizeof(buf), "%s", nc_to_str_raw(v));
    return nc_str(buf);
}
static NcVal *nc_builtin_heltall(NcVal *v) {
    if (v->type == NC_INT) return v;
    if (v->type == NC_STR) return nc_int(strtoll(v->s, NULL, 0));
    if (v->type == NC_BOOL) return nc_int(v->b);
    return nc_int(0);
}
static NcVal *nc_builtin_json_stringify(NcVal *v);
static NcVal *nc_builtin_json_stringify(NcVal *v) {
    if (!v || v->type == NC_NIL) return nc_str("null");
    if (v->type == NC_BOOL) return nc_str(v->b ? "true" : "false");
    if (v->type == NC_INT) { char buf[32]; snprintf(buf,sizeof(buf),"%lld",v->i); return nc_str(buf); }
    if (v->type == NC_STR) {
        size_t len = strlen(v->s);
        char *r = malloc(len*2+3); char *wp = r; *wp++ = '"';
        for (size_t i = 0; i < len; i++) {
            unsigned char c = (unsigned char)v->s[i];
            if (c == '"') { *wp++='\\'; *wp++='"'; }
            else if (c == '\\') { *wp++='\\'; *wp++='\\'; }
            else if (c == '\n') { *wp++='\\'; *wp++='n'; }
            else if (c == '\r') { *wp++='\\'; *wp++='r'; }
            else if (c == '\t') { *wp++='\\'; *wp++='t'; }
            else *wp++ = c;
        }
        *wp++ = '"'; *wp = 0;
        return nc_str_own(r);
    }
    if (v->type == NC_LIST) {
        char *r = strdup("["); int first = 1;
        for (int i = 0; i < v->list->len; i++) {
            NcVal *item_json = nc_builtin_json_stringify(v->list->items[i]);
            size_t rl = strlen(r), jl = strlen(item_json->s);
            r = realloc(r, rl + jl + 3);
            if (!first) strcat(r, ",");
            strcat(r, item_json->s); first = 0;
        }
        r = realloc(r, strlen(r)+2); strcat(r, "]");
        return nc_str_own(r);
    }
    if (v->type == NC_MAP) {
        char *r = strdup("{"); int first = 1;
        for (int i = 0; i < v->map->len; i++) {
            NcVal *kj = nc_builtin_json_stringify(nc_str(v->map->keys[i]));
            NcVal *vj = nc_builtin_json_stringify(v->map->vals[i]);
            size_t rl = strlen(r), kl = strlen(kj->s), vl = strlen(vj->s);
            r = realloc(r, rl + kl + vl + 4);
            if (!first) strcat(r, ",");
            strcat(r, kj->s); strcat(r, ":"); strcat(r, vj->s); first = 0;
        }
        r = realloc(r, strlen(r)+2); strcat(r, "}");
        return nc_str_own(r);
    }
    return nc_str("null");
}

/* ── Tilleggsbuiltins ── */
static NcVal *nc_builtin_bool(NcVal *v) { return nc_bool(nc_truthy(v)); }
static NcVal *nc_builtin_feil(NcVal *v) { char *s = nc_to_str_raw(v); nc_throw(s); free(s); return nc_nil(); }
static NcVal *nc_builtin_n(NcVal *v) { return v ? v : nc_nil(); }
static NcVal *nc_builtin_desimaltall(NcVal *v) {
    if (!v) return nc_int(0);
    if (v->type == NC_INT) return v;
    if (v->type == NC_STR) { NcVal *r = calloc(1,sizeof(NcVal)); r->type=NC_INT; r->i=(long long)atof(v->s); return r; }
    return nc_int(0);
}
static NcVal *nc_builtin_index_of(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    char *found = strstr(s, p); int r = found ? (int)(found-s) : -1;
    free(s); free(p); return nc_int(r);
}
static NcVal *nc_builtin_lower(NcVal *v) {
    char *s = nc_to_str_raw(v), *r = strdup(s);
    for (int i=0;r[i];i++) r[i]=tolower((unsigned char)r[i]);
    free(s); return nc_str_own(r);
}
static NcVal *nc_builtin_upper(NcVal *v) {
    char *s = nc_to_str_raw(v), *r = strdup(s);
    for (int i=0;r[i];i++) r[i]=toupper((unsigned char)r[i]);
    free(s); return nc_str_own(r);
}
static NcVal *nc_builtin_type(NcVal *v) {
    if (!v || v->type==NC_NIL) return nc_str("ingenting");
    if (v->type==NC_INT)  return nc_str("heltall");
    if (v->type==NC_STR)  return nc_str("tekst");
    if (v->type==NC_BOOL) return nc_str("boolsk");
    if (v->type==NC_LIST) return nc_str("liste");
    if (v->type==NC_MAP)  return nc_str("ordbok");
    return nc_str("ukjent");
}
static NcVal *nc_builtin_fjern_nokkel(NcVal *m, NcVal *k_v) {
    if (!m || m->type!=NC_MAP) return nc_nil();
    char *k = nc_to_str_raw(k_v);
    for (int i=0;i<m->map->len;i++) {
        if (!strcmp(m->map->keys[i],k)) {
            free(m->map->keys[i]);
            memmove(&m->map->keys[i],&m->map->keys[i+1],(m->map->len-i-1)*sizeof(char*));
            memmove(&m->map->vals[i],&m->map->vals[i+1],(m->map->len-i-1)*sizeof(NcVal*));
            m->map->len--; free(k); return nc_nil();
        }
    }
    free(k); return nc_nil();
}
static NcVal *nc_builtin_fil_append(NcVal *path_v, NcVal *data_v) {
    char *path = nc_to_str_raw(path_v), *data = nc_to_str_raw(data_v);
    FILE *f = fopen(path,"ab"); free(path);
    if (f) { fputs(data,f); fclose(f); }
    free(data); return nc_nil();
}
static NcVal *nc_builtin_exit(NcVal *code_v) {
    exit(code_v && code_v->type==NC_INT ? (int)code_v->i : 0);
    return nc_nil();
}

/* Minimal JSON parser for nc_builtin_json_parse_str */
typedef struct { const char *p; } JP2;
static NcVal *jp2_parse(JP2 *j);
static void jp2_ws(JP2 *j) { while (*j->p==' '||*j->p=='\t'||*j->p=='\n'||*j->p=='\r') j->p++; }
static char *jp2_str(JP2 *j) {
    j->p++;
    /* Bruk heap i staden for stack for å unngå stack overflow på store NCB-ar */
    size_t cap = 4096, bi = 0;
    char *buf = malloc(cap);
    while (*j->p && *j->p!='"') {
        if (bi + 4 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        if (*j->p=='\\') { j->p++;
            switch(*j->p) {
                case '"': buf[bi++]='"'; break; case '\\': buf[bi++]='\\'; break;
                case 'n': buf[bi++]='\n'; break; case 'r': buf[bi++]='\r'; break;
                case 't': buf[bi++]='\t'; break;
                default: buf[bi++]='\\'; buf[bi++]=*j->p; break;
            }
        } else buf[bi++]=*j->p;
        j->p++;
    }
    if (*j->p=='"') j->p++;
    buf[bi]=0;
    char *result = strdup(buf);
    free(buf);
    return result;
}
static NcVal *jp2_parse(JP2 *j) {
    jp2_ws(j);
    if (*j->p=='"') { char *s=jp2_str(j); return nc_str_own(s); }
    if (*j->p=='{') {
        j->p++; NcVal *m=nc_map_new(); jp2_ws(j);
        while (*j->p&&*j->p!='}') {
            if (*j->p==','){j->p++;jp2_ws(j);continue;}
            char *k=jp2_str(j); jp2_ws(j); j->p++; jp2_ws(j);
            NcVal *v=jp2_parse(j); jp2_ws(j);
            nc_index_set(m,nc_str_own(k),v);
        }
        if (*j->p=='}') j->p++;
        return m;
    }
    if (*j->p=='[') {
        j->p++; NcVal *lst=nc_list_new(); jp2_ws(j);
        while (*j->p&&*j->p!=']') {
            if (*j->p==','){j->p++;jp2_ws(j);continue;}
            nc_builtin_legg_til(lst,jp2_parse(j)); jp2_ws(j);
        }
        if (*j->p==']') j->p++;
        return lst;
    }
    if (strncmp(j->p,"true",4)==0){j->p+=4;return nc_bool(1);}
    if (strncmp(j->p,"false",5)==0){j->p+=5;return nc_bool(0);}
    if (strncmp(j->p,"null",4)==0){j->p+=4;return nc_nil();}
    char *end; long long iv=strtoll(j->p,&end,10);
    if (end!=j->p){j->p=end;return nc_int(iv);}
    return nc_nil();
}
static NcVal *nc_builtin_json_parse_str(NcVal *v) {
    char *s = nc_to_str_raw(v); JP2 j={s}; NcVal *r=jp2_parse(&j); free(s); return r;
}
static NcVal *nc_to_str(NcVal *v) { return nc_str_own(nc_to_str_raw(v)); }
/* Forward: definert i nc_dispatch.c etter runtime */
NcVal *nc_fn_builtin_neste_token(NcVal **args, int nargs);


/* old json_parse removed */

/* Smart json_stringify (same semantikk som nc_vm.c json_emit) */
static int nc_str_looks_like_json_nonstring(const char *s) {
    if (!s || !*s) return 0;
    if (!strcmp(s,"true")||!strcmp(s,"false")||!strcmp(s,"null")) return 1;
    size_t sl = strlen(s);
    if (s[0]=='{' && s[sl-1]=='}' && sl>=2) return 1;
    if (s[0]=='[' && s[sl-1]==']' && sl>=2) return 1;
    char *end; strtoll(s,&end,10);
    if (*end==0 && end!=s) return 1;
    return 0;
}
static NcVal *nc_builtin_json_stringify_smart(NcVal *v);
static NcVal *nc_builtin_json_stringify_smart(NcVal *v) {
    if (!v || v->type==NC_NIL) return nc_str("null");
    if (v->type==NC_BOOL) return nc_str(v->b?"true":"false");
    if (v->type==NC_INT) { char b[32]; snprintf(b,sizeof(b),"%lld",v->i); return nc_str(b); }
    if (v->type==NC_STR) {
        if (nc_str_looks_like_json_nonstring(v->s)) return nc_str(v->s);
        return nc_builtin_json_stringify(v);  /* normal string quoting */
    }
    if (v->type==NC_LIST) {
        char *r = strdup("["); int first=1;
        for (int i=0; i<v->list->len; i++) {
            NcVal *item_json = nc_builtin_json_stringify_smart(v->list->items[i]);
            r = realloc(r, strlen(r)+strlen(item_json->s)+3);
            if (!first) strcat(r,",");
            strcat(r, item_json->s); first=0;
        }
        r = realloc(r, strlen(r)+2); strcat(r,"]");
        return nc_str_own(r);
    }
    if (v->type==NC_MAP) {
        char *r = strdup("{"); int first=1;
        for (int i=0; i<v->map->len; i++) {
            NcVal *kj = nc_builtin_json_stringify(nc_str(v->map->keys[i]));
            NcVal *vj = nc_builtin_json_stringify_smart(v->map->vals[i]);
            r = realloc(r, strlen(r)+strlen(kj->s)+strlen(vj->s)+4);
            if (!first) strcat(r,",");
            strcat(r, kj->s); strcat(r,":"); strcat(r, vj->s); first=0;
        }
        r = realloc(r, strlen(r)+2); strcat(r,"}");
        return nc_str_own(r);
    }
    return nc_str("null");
}

/* nc_builtin_json_parse_norscode — full nc-vm-kompatibel json.parse */
static NcVal *nc_builtin_json_parse_norscode(NcVal *v) {
    char *s = nc_to_str_raw(v); JP2 j={s};
    NcVal *r = jp2_parse(&j); free(s);
    if (!r) return nc_nil();
    /* MAP: stringify non-string values */
    if (r->type == NC_MAP) {
        for (int i=0; i<r->map->len; i++) {
            NcVal *mv = r->map->vals[i];
            if (mv && mv->type != NC_STR) {
                r->map->vals[i] = nc_builtin_json_stringify_smart(mv);
            }
        }
        return r;
    }
    /* LIST: konverter til string-keyed map */
    if (r->type == NC_LIST) {
        NcVal *m = nc_map_new();
        for (int i=0; i<r->list->len; i++) {
            char keybuf[32]; snprintf(keybuf, sizeof(keybuf), "%d", i);
            NcVal *mv = r->list->items[i];
            if (mv && mv->type != NC_STR) mv = nc_builtin_json_stringify_smart(mv);
            nc_index_set(m, nc_str(keybuf), mv);
        }
        return m;
    }
    return r;
}

/* nc_builtin_json_parse_raw — JSON-parse utan string-konvertering
 * Returnerer native NcVal-ordbøker og lister (ikkje stringifiserte verdiar).
 * Brukt av ncb_to_c.no for å parse bootstrap/kompiler.ncb.json. */
static NcVal *nc_builtin_json_parse_raw(NcVal *v) {
    char *s = nc_to_str_raw(v); JP2 j={s};
    NcVal *r = jp2_parse(&j); free(s);
    return r ? r : nc_nil();
}

static NcVal *nc_fn_builtin_json_parse_raw(NcVal **a, int na) { return nc_builtin_json_parse_raw(na>0?a[0]:nc_nil()); }
