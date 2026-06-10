/*
 * nc_runtime_mini.c — minimal Norscode C-runtime for maintainer-lanen.
 *
 * Brukt av generert C-kode frå `archive/legacy_c_backend/ncb_to_c.no`.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <setjmp.h>
#include <errno.h>

/* ── Stage0-kjerne: verdi-layout og grunnstrukturar ── */
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
    size_t slen; /* cachet tekstlengd for NC_STR */
};

/* ── Stage0-kjerne: feilhandtering ── */
static jmp_buf g_err_jmp;
static char    g_err_msg[4096];

static void nc_panic(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(g_err_msg, sizeof(g_err_msg), fmt, ap);
    va_end(ap);
    longjmp(g_err_jmp, 1);
}

/* ── Stage0-kjerne: konstruktørar ── */
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
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_STR;
    v->s = strdup(s ? s : ""); v->slen = strlen(v->s); return v;
}
static NcVal *nc_str_own(char *s) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_STR;
    v->s = s; v->slen = s ? strlen(s) : 0; return v;
}
static NcVal *nc_list_new(void) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_LIST;
    v->list = calloc(1, sizeof(NcList)); return v;
}
static NcVal *nc_map_new(void) {
    NcVal *v = calloc(1, sizeof(NcVal)); v->type = NC_MAP;
    v->map = calloc(1, sizeof(NcMap)); return v;
}

/* ── Stage0-kjerne: stack ── */
static void nc_push(int *sp, NcVal **stack, NcVal *v) {
    if (*sp >= 512) nc_panic("Stack overflow");
    stack[(*sp)++] = v ? v : nc_nil();
}
static NcVal *nc_pop(int *sp, NcVal **stack) {
    if (*sp <= 0) nc_panic("Stack underflow");
    return stack[--(*sp)];
}

/* ── Stage0-kjerne: variabeloppslag ── */
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

/* ── Stage0-kjerne: konvertering/truthiness ── */
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

/* ── Stage0-kjerne: aritmetikk ── */
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

/* ── Stage0-kjerne: samanlikning ── */
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

/* ── Stage0-kjerne: liste/ordbok-bygging ── */
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
        /* veks kapasiteten ved behov */
        if (m->map->len >= m->map->cap) {
            m->map->cap = m->map->cap ? m->map->cap*2 : 4;
            m->map->keys = realloc(m->map->keys, m->map->cap*sizeof(char*));
            m->map->vals = realloc(m->map->vals, m->map->cap*sizeof(NcVal*));
        }
        m->map->keys[m->map->len] = ks;
        m->map->vals[m->map->len] = v;
        m->map->len++;
    }
    return m;
}

/* ── Stage0-kjerne: indeksering ── */
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
        long long slen = (long long)(obj->slen ? obj->slen : (obj->slen = strlen(obj->s)));
        if (idx < 0) idx = slen + idx;
        if (idx < 0 || idx >= slen) return nc_str("");
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
        if (obj->map->len >= obj->map->cap) {
            obj->map->cap = obj->map->cap ? obj->map->cap*2 : 4;
            obj->map->keys = realloc(obj->map->keys, obj->map->cap*sizeof(char*));
            obj->map->vals = realloc(obj->map->vals, obj->map->cap*sizeof(NcVal*));
        }
        obj->map->keys[obj->map->len] = ks;
        obj->map->vals[obj->map->len] = val;
        obj->map->len++;
    }
}

/* ── Stage0-kjerne: unntak ── */
static char g_throw_msg[4096];
static void nc_throw(const char *msg) {
    strncpy(g_throw_msg, msg ? msg : "ukjent feil", sizeof(g_throw_msg)-1);
    nc_panic("Norscode unntak: %s", g_throw_msg);
}

/*
 * Hjelpelag over stage0-kjerna.
 */
/* ── Hjelpelag: innebygde funksjonar ── */
static NcVal *nc_builtin_lengde(NcVal *v) {
    if (!v) return nc_int(0);
    if (v->type == NC_STR)  return nc_int((long long)(v->slen ? v->slen : (v->slen=strlen(v->s))));
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
    long long len = (long long)(v->slen ? v->slen : (v->slen=strlen(v->s)));
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
/* Fil/miljø-bridge mot host-OS via `nc_host_*`. */
/* ── Hjelpelag: fil/miljø-host-bridge ── */
static int nc_host_file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    int ok = f != NULL;
    if (f) fclose(f);
    return ok;
}
static char *nc_host_read_text_path(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    return buf;
}
static int nc_host_write_text_path(const char *path, const char *data) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fputs(data, f);
    fclose(f);
    return 1;
}
static int nc_host_write_binary_path(const char *path, NcVal *list_v) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    if (list_v && list_v->type == NC_LIST) {
        for (int i = 0; i < list_v->list->len; i++) {
            NcVal *bv = list_v->list->items[i];
            unsigned char byte = (unsigned char)(bv && bv->type == NC_INT ? (int)bv->i : 0);
            fwrite(&byte, 1, 1, f);
        }
    }
    fclose(f);
    return 1;
}
static int nc_host_append_text_path(const char *path, const char *data) {
    FILE *f = fopen(path, "ab");
    if (!f) return 0;
    fputs(data, f);
    fclose(f);
    return 1;
}
static const char *nc_host_getenv_raw(const char *key, int *exists) {
    const char *v = getenv(key);
    if (exists) *exists = v != NULL;
    return v;
}
static char *nc_host_getenv_text(const char *key) {
    const char *v = nc_host_getenv_raw(key, NULL);
    return strdup(v ? v : "");
}
static NcVal *nc_env_get_text(const char *key) {
    return nc_str_own(nc_host_getenv_text(key));
}
static NcVal *nc_env_exists(const char *key) {
    int exists = 0;
    nc_host_getenv_raw(key, &exists);
    return nc_bool(exists);
}
static NcVal *nc_env_set_text(const char *key, const char *val) {
    setenv(key, val, 1);
    return nc_env_get_text(key);
}
static NcVal *nc_host_file_error_nil(const char *op, const char *path) {
    char msg[512];
    snprintf(msg, sizeof(msg), "Kan ikkje %s fil: %s", op, path);
    nc_throw(msg);
    return nc_nil();
}
static NcVal *nc_host_file_write_result(int ok, const char *op, const char *path) {
    return ok ? nc_nil() : nc_host_file_error_nil(op, path);
}
static NcVal *nc_file_read_text(const char *path) {
    char *buf = nc_host_read_text_path(path);
    return buf ? nc_str_own(buf) : nc_host_file_error_nil("opne", path);
}
static NcVal *nc_file_write_text(const char *path, const char *data) {
    return nc_host_file_write_result(nc_host_write_text_path(path, data), "skrive", path);
}
static NcVal *nc_file_write_binary(const char *path, NcVal *list_v) {
    return nc_host_file_write_result(nc_host_write_binary_path(path, list_v), "skrive", path);
}
static NcVal *nc_file_append_text(const char *path, const char *data) {
    return nc_host_file_write_result(nc_host_append_text_path(path, data), "append til", path);
}
static NcVal *nc_file_exists_value(const char *path) {
    return nc_bool(nc_host_file_exists(path));
}

static NcVal *nc_builtin_fil_les(NcVal *path_v) {
    char *path = nc_to_str_raw(path_v);
    NcVal *r = nc_file_read_text(path);
    free(path);
    return r;
}
static NcVal *nc_builtin_fil_skriv(NcVal *path_v, NcVal *data_v) {
    char *path = nc_to_str_raw(path_v), *data = nc_to_str_raw(data_v);
    NcVal *r = nc_file_write_text(path, data);
    free(path);
    free(data);
    return r;
}
static NcVal *nc_builtin_fil_skriv_binary(NcVal *path_v, NcVal *list_v) {
    char *path = nc_to_str_raw(path_v);
    NcVal *r = nc_file_write_binary(path, list_v);
    free(path);
    return r;
}
static NcVal *nc_builtin_fil_finnes(NcVal *path_v) {
    char *path = nc_to_str_raw(path_v);
    NcVal *r = nc_file_exists_value(path);
    free(path);
    return r;
}
static NcVal *nc_builtin_miljo_hent(NcVal *k_v) {
    char *k = nc_to_str_raw(k_v);
    NcVal *r = nc_env_get_text(k);
    free(k);
    return r;
}
static NcVal *nc_builtin_miljo_finnes(NcVal *k_v) {
    char *k = nc_to_str_raw(k_v);
    NcVal *r = nc_env_exists(k);
    free(k);
    return r;
}
/* ── Hjelpelag: samlingshjelparar over liste/map-kjerna ── */
static int nc_coll_find_key_index(NcVal *m, const char *k) {
    if (!m || m->type != NC_MAP) return -1;
    for (int i = 0; i < m->map->len; i++) {
        if (!strcmp(m->map->keys[i], k)) return i;
    }
    return -1;
}
static NcVal *nc_coll_collect_list(NcVal *m, int values) {
    NcVal *lst = nc_list_new();
    if (!m || m->type != NC_MAP) return lst;
    for (int i = 0; i < m->map->len; i++) {
        nc_builtin_legg_til(lst, values ? m->map->vals[i] : nc_str(m->map->keys[i]));
    }
    return lst;
}
static NcVal *nc_builtin_finnes_nokkel(NcVal *m, NcVal *k_v) {
    char *k = nc_to_str_raw(k_v);
    int ok = nc_coll_find_key_index(m, k) >= 0;
    free(k);
    return nc_bool(ok);
}
static NcVal *nc_builtin_nokler(NcVal *m) {
    return nc_coll_collect_list(m, 0);
}
static NcVal *nc_builtin_verdier(NcVal *m) {
    return nc_coll_collect_list(m, 1);
}
/* ── Hjelpelag: teksthjelparar over streng- og listekjerna ── */
static NcVal *nc_text_split_to_list(const char *s, const char *sep) {
    NcVal *lst = nc_list_new();
    size_t seplen = strlen(sep);
    if (seplen == 0) {
        for (size_t i = 0; s[i]; i++) {
            char buf[2] = {s[i], 0};
            nc_builtin_legg_til(lst, nc_str(buf));
        }
        return lst;
    }
    char *cur = (char *)s;
    char *found;
    while ((found = strstr(cur, sep)) != NULL) {
        size_t partlen = found - cur;
        char *part = malloc(partlen + 1);
        memcpy(part, cur, partlen);
        part[partlen] = 0;
        nc_builtin_legg_til(lst, nc_str_own(part));
        cur = found + seplen;
    }
    nc_builtin_legg_til(lst, nc_str(cur));
    return lst;
}
static char *nc_text_join_list(NcVal *lst, const char *sep) {
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
    free(parts);
    free(lens);
    return r;
}
static char *nc_text_replace_copy(const char *s, const char *old, const char *newstr) {
    size_t olen = strlen(old), nlen = strlen(newstr);
    if (olen == 0) return strdup(s);
    int cnt = 0;
    char *p = (char *)s;
    while ((p = strstr(p, old)) != NULL) { cnt++; p += olen; }
    char *r = malloc(strlen(s) + cnt * (nlen > olen ? nlen - olen : 0) + 1);
    char *wp = r;
    p = (char *)s;
    char *found;
    while ((found = strstr(p, old)) != NULL) {
        memcpy(wp, p, found - p); wp += found - p;
        memcpy(wp, newstr, nlen); wp += nlen;
        p = found + olen;
    }
    strcpy(wp, p);
    return r;
}
static int nc_text_index_of_raw(const char *s, const char *p) {
    char *found = strstr(s, p);
    return found ? (int)(found - s) : -1;
}
static int nc_text_starts_with_raw(const char *s, const char *p) {
    return strncmp(s, p, strlen(p)) == 0;
}
static int nc_text_ends_with_raw(const char *s, const char *p) {
    size_t sl = strlen(s), pl = strlen(p);
    return sl >= pl && strcmp(s + sl - pl, p) == 0;
}
static int nc_text_contains_raw(const char *s, const char *p) {
    return strstr(s, p) != NULL;
}
static NcVal *nc_builtin_starts_with(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    NcVal *r = nc_bool(nc_text_starts_with_raw(s, p)); free(s); free(p);
    return r;
}
static NcVal *nc_builtin_ends_with(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    NcVal *r = nc_bool(nc_text_ends_with_raw(s, p)); free(s); free(p);
    return r;
}
static NcVal *nc_builtin_contains(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    NcVal *r = nc_bool(nc_text_contains_raw(s, p)); free(s); free(p);
    return r;
}
static NcVal *nc_builtin_split(NcVal *s_v, NcVal *sep_v) {
    char *s = nc_to_str_raw(s_v), *sep = nc_to_str_raw(sep_v);
    NcVal *lst = nc_text_split_to_list(s, sep);
    free(s); free(sep);
    return lst;
}
static NcVal *nc_builtin_join(NcVal *lst, NcVal *sep_v) {
    if (!lst || lst->type != NC_LIST) return nc_str("");
    char *sep = nc_to_str_raw(sep_v);
    NcVal *r = nc_str_own(nc_text_join_list(lst, sep));
    free(sep);
    return r;
}
static NcVal *nc_builtin_trim(NcVal *s_v) {
    char *s = nc_to_str_raw(s_v);
    int a = 0, b = (int)strlen(s);
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    char *rtext = malloc(b - a + 1);
    memcpy(rtext, s + a, b - a);
    rtext[b - a] = 0;
    NcVal *r = nc_str_own(rtext);
    free(s); return r;
}
static NcVal *nc_builtin_replace(NcVal *s_v, NcVal *old_v, NcVal *new_v) {
    char *s = nc_to_str_raw(s_v), *old = nc_to_str_raw(old_v), *newstr = nc_to_str_raw(new_v);
    NcVal *r = nc_str_own(nc_text_replace_copy(s, old, newstr));
    free(s); free(old); free(newstr);
    return r;
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
static NcVal *nc_json_stringify_list(NcVal *v, int smart);
static NcVal *nc_json_stringify_map(NcVal *v, int smart);
static char *nc_json_escape_string_copy(const char *s);
static int nc_json_looks_like_nonstring(const char *s);
static NcVal *nc_json_stringify_primitive(NcVal *v) {
    if (!v || v->type == NC_NIL) return nc_str("null");
    if (v->type == NC_BOOL) return nc_str(v->b ? "true" : "false");
    if (v->type == NC_INT) { char buf[32]; snprintf(buf,sizeof(buf),"%lld",v->i); return nc_str(buf); }
    return NULL;
}
static NcVal *nc_json_stringify_compound(NcVal *v, int smart) {
    if (v->type == NC_LIST) {
        return nc_json_stringify_list(v, smart);
    }
    if (v->type == NC_MAP) {
        return nc_json_stringify_map(v, smart);
    }
    return nc_str("null");
}
static NcVal *nc_json_stringify_string(NcVal *v, int smart) {
    if (smart && nc_json_looks_like_nonstring(v->s)) return nc_str(v->s);
    return nc_str_own(nc_json_escape_string_copy(v->s));
}
static NcVal *nc_json_stringify_any(NcVal *v, int smart) {
    NcVal *prim = nc_json_stringify_primitive(v);
    if (prim) return prim;
    if (v->type == NC_STR) {
        return nc_json_stringify_string(v, smart);
    }
    return nc_json_stringify_compound(v, smart);
}

/* ── Hjelpelag: diverse convenience-builtins ── */
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
    NcVal *r = nc_int(nc_text_index_of_raw(s, p));
    free(s); free(p); return r;
}
static NcVal *nc_builtin_lower(NcVal *v) {
    char *s = nc_to_str_raw(v);
    char *rtext = strdup(s);
    for (int i = 0; rtext[i]; i++) rtext[i] = tolower((unsigned char)rtext[i]);
    NcVal *r = nc_str_own(rtext);
    free(s); return r;
}
static NcVal *nc_builtin_upper(NcVal *v) {
    char *s = nc_to_str_raw(v);
    char *rtext = strdup(s);
    for (int i = 0; rtext[i]; i++) rtext[i] = toupper((unsigned char)rtext[i]);
    NcVal *r = nc_str_own(rtext);
    free(s); return r;
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
    int i = nc_coll_find_key_index(m, k);
    if (i >= 0) {
        free(m->map->keys[i]);
        memmove(&m->map->keys[i],&m->map->keys[i+1],(m->map->len-i-1)*sizeof(char*));
        memmove(&m->map->vals[i],&m->map->vals[i+1],(m->map->len-i-1)*sizeof(NcVal*));
        m->map->len--; free(k); return nc_nil();
    }
    free(k); return nc_nil();
}
static NcVal *nc_builtin_fil_append(NcVal *path_v, NcVal *data_v) {
    char *path = nc_to_str_raw(path_v), *data = nc_to_str_raw(data_v);
    NcVal *r = nc_file_append_text(path, data);
    free(path);
    free(data); return r;
}
static NcVal *nc_builtin_exit(NcVal *code_v) {
    exit(code_v && code_v->type==NC_INT ? (int)code_v->i : 0);
    return nc_nil();
}

/* ── Hjelpelag: JSON-parsing/-stringify-lag ── */
/* Enkel JSON-parser for direkte parse-entrypoint. */
typedef struct { const char *p; } JP2;
static NcVal *jp2_parse(JP2 *j);
static NcVal *nc_json_parse_string(JP2 *j);
static NcVal *nc_json_parse_object(JP2 *j);
static NcVal *nc_json_parse_array(JP2 *j);
static NcVal *nc_json_parse_true(JP2 *j);
static NcVal *nc_json_parse_false(JP2 *j);
static NcVal *nc_json_parse_null(JP2 *j);
static NcVal *nc_json_parse_number(JP2 *j);
static NcVal *nc_json_parse_unknown(JP2 *j);
static NcVal *nc_json_parse_primitive(JP2 *j);
static int nc_json_is_token_boundary(char c);
static int nc_json_consume_keyword(JP2 *j, const char *kw, int len);
static void nc_json_unescape_append(char *buf, size_t *bi, char c) {
    switch (c) {
        case '"': buf[(*bi)++] = '"'; break;
        case '\\': buf[(*bi)++] = '\\'; break;
        case 'n': buf[(*bi)++] = '\n'; break;
        case 'r': buf[(*bi)++] = '\r'; break;
        case 't': buf[(*bi)++] = '\t'; break;
        default:
            buf[(*bi)++] = '\\';
            buf[(*bi)++] = c;
            break;
    }
}
static void nc_json_skip_ws(JP2 *j) { while (*j->p==' '||*j->p=='\t'||*j->p=='\n'||*j->p=='\r') j->p++; }
static char *jp2_str(JP2 *j) {
    j->p++;
    /* brukar heapbuffer for store JSON-strengar */
    size_t cap = 4096, bi = 0;
    char *buf = malloc(cap);
    while (*j->p && *j->p!='"') {
        if (bi + 4 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        if (*j->p=='\\') { j->p++;
            nc_json_unescape_append(buf, &bi, *j->p);
        } else buf[bi++]=*j->p;
        j->p++;
    }
    if (*j->p == '"') {
        j->p++;
    } else {
        free(buf);
        return NULL;
    }
    buf[bi]=0;
    char *result = strdup(buf);
    free(buf);
    return result;
}
static NcVal *nc_json_parse_from_ncval(NcVal *v) {
    char *s = nc_to_str_raw(v);
    JP2 j = {s};
    NcVal *r = jp2_parse(&j);
    nc_json_skip_ws(&j);
    if (r && *j.p != '\0') r = nc_nil();
    free(s);
    return r ? r : nc_nil();
}
static NcVal *nc_json_parse_object(JP2 *j) {
    j->p++;
    NcVal *m = nc_map_new();
    nc_json_skip_ws(j);
    if (*j->p == '}') { j->p++; return m; }
    while (*j->p) {
        if (*j->p == ',') { return nc_nil(); }
        if (*j->p != '"') { return nc_nil(); }
        char *k = jp2_str(j);
        if (!k) { return nc_nil(); }
        nc_json_skip_ws(j);
        if (*j->p != ':') { free(k); return nc_nil(); }
        j->p++;
        nc_json_skip_ws(j);
        NcVal *v = jp2_parse(j);
        if (!v) { free(k); return nc_nil(); }
        nc_json_skip_ws(j);
        nc_index_set(m, nc_str_own(k), v);
        if (*j->p == ',') {
            j->p++;
            nc_json_skip_ws(j);
            if (*j->p == '}' || *j->p == ',') return nc_nil();
            if (*j->p == '\0') return nc_nil();
            continue;
        }
        if (*j->p == '}') { j->p++; return m; }
        return nc_nil();
    }
    return nc_nil();
}
static NcVal *nc_json_parse_array(JP2 *j) {
    j->p++;
    NcVal *lst = nc_list_new();
    nc_json_skip_ws(j);
    if (*j->p == ']') { j->p++; return lst; }
    while (*j->p) {
        if (*j->p == ',') { return nc_nil(); }
        nc_builtin_legg_til(lst, jp2_parse(j));
        nc_json_skip_ws(j);
        if (*j->p == ',') {
            j->p++;
            nc_json_skip_ws(j);
            if (*j->p == ']' || *j->p == ',') return nc_nil();
            continue;
        }
        if (*j->p == ']') { j->p++; return lst; }
        return nc_nil();
    }
    return nc_nil();
}
static NcVal *nc_json_parse_string(JP2 *j) {
    char *s = jp2_str(j);
    if (!s) return nc_nil();
    return nc_str_own(s);
}
static NcVal *nc_json_parse_true(JP2 *j) {
    (void)j;
    return nc_bool(1);
}
static NcVal *nc_json_parse_false(JP2 *j) {
    (void)j;
    return nc_bool(0);
}
static NcVal *nc_json_parse_null(JP2 *j) {
    (void)j;
    return nc_nil();
}
static int nc_json_is_token_boundary(char c) {
    return c == '\0' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',' || c == '}' || c == ']' || c == ':';
}
static int nc_json_consume_keyword(JP2 *j, const char *kw, int len) {
    if (strncmp(j->p, kw, len) != 0) return 0;
    if (!nc_json_is_token_boundary(j->p[len])) return 0;
    j->p += len;
    return 1;
}
static NcVal *nc_json_parse_number(JP2 *j) {
    const char *start = j->p;
    const char *cursor = start;
    if (*cursor == '-') cursor++;
    if (*cursor == '+') return NULL;
    if (!isdigit((unsigned char)*cursor)) return NULL;
    if (*cursor == '0' && isdigit((unsigned char)cursor[1])) return NULL;

    char *end;
    long long iv = strtoll(j->p, &end, 10);
    if (end == j->p) return NULL;
    if (!nc_json_is_token_boundary(*end)) return NULL;
    j->p = end;
    return nc_int(iv);
}
static NcVal *nc_json_parse_unknown(JP2 *j) {
    while (*j->p && !nc_json_is_token_boundary(*j->p)) {
        j->p++;
    }
    return nc_nil();
}
static NcVal *nc_json_parse_primitive(JP2 *j) {
    if (nc_json_consume_keyword(j, "true", 4)) return nc_json_parse_true(j);
    if (nc_json_consume_keyword(j, "false", 5)) return nc_json_parse_false(j);
    if (nc_json_consume_keyword(j, "null", 4)) return nc_json_parse_null(j);
    NcVal *number = nc_json_parse_number(j);
    if (number) return number;
    return nc_json_parse_unknown(j);
}
static NcVal *jp2_parse(JP2 *j) {
    nc_json_skip_ws(j);
    if (*j->p=='"') return nc_json_parse_string(j);
    if (*j->p=='{') return nc_json_parse_object(j);
    if (*j->p=='[') return nc_json_parse_array(j);
    return nc_json_parse_primitive(j);
}
static NcVal *nc_to_str(NcVal *v) { return nc_str_own(nc_to_str_raw(v)); }
/* Host-/generated hook: levert utanfrå denne runtimefila. */
NcVal *nc_fn_builtin_neste_token(NcVal **args, int nargs);

/* Smart JSON-stringify for verdiar som allereie kan likne rå JSON. */
static int nc_json_looks_like_nonstring(const char *s) {
    if (!s || !*s) return 0;
    if (!strcmp(s,"true")||!strcmp(s,"false")||!strcmp(s,"null")) return 1;
    size_t sl = strlen(s);
    if (s[0]=='{' && s[sl-1]=='}' && sl>=2) return 1;
    if (s[0]=='[' && s[sl-1]==']' && sl>=2) return 1;
    char *end; strtoll(s,&end,10);
    if (*end==0 && end!=s) return 1;
    return 0;
}
static NcVal *nc_json_stringify_list(NcVal *v, int smart) {
    char *r = strdup("["); int first = 1;
    for (int i = 0; i < v->list->len; i++) {
        NcVal *item_json = smart
            ? nc_json_stringify_any(v->list->items[i], 1)
            : nc_json_stringify_any(v->list->items[i], 0);
        size_t rl = strlen(r), jl = strlen(item_json->s);
        r = realloc(r, rl + jl + 3);
        if (!first) strcat(r, ",");
        strcat(r, item_json->s); first = 0;
    }
    r = realloc(r, strlen(r)+2); strcat(r, "]");
    return nc_str_own(r);
}
static int nc_json_map_is_dense_array_keys(NcVal *v) {
    if (!v || v->type != NC_MAP) return 0;
    for (int i = 0; i < v->map->len; i++) {
        char keybuf[32];
        snprintf(keybuf, sizeof(keybuf), "%d", i);
        if (strcmp(v->map->keys[i], keybuf) != 0) return 0;
    }
    return 1;
}
static NcVal *nc_json_stringify_map(NcVal *v, int smart) {
    int arrayish = nc_json_map_is_dense_array_keys(v);
    char *r = strdup("{"); int first = 1;
    for (int i = 0; i < v->map->len; i++) {
        NcVal *kj = nc_json_stringify_any(nc_str(v->map->keys[i]), 0);
        int value_smart = smart;
        if (arrayish && v->map->vals[i] && v->map->vals[i]->type == NC_STR) {
            value_smart = 0;
        }
        NcVal *vj = value_smart
            ? nc_json_stringify_any(v->map->vals[i], 1)
            : nc_json_stringify_any(v->map->vals[i], 0);
        size_t rl = strlen(r), kl = strlen(kj->s), vl = strlen(vj->s);
        r = realloc(r, rl + kl + vl + 4);
        if (!first) strcat(r, ",");
        strcat(r, kj->s); strcat(r, ":"); strcat(r, vj->s); first = 0;
    }
    r = realloc(r, strlen(r)+2); strcat(r, "}");
    return nc_str_own(r);
}
static char *nc_json_escape_string_copy(const char *s) {
    size_t len = strlen(s);
    char *r = malloc(len * 2 + 3);
    char *wp = r;
    *wp++ = '"';
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"') { *wp++='\\'; *wp++='"'; }
        else if (c == '\\') { *wp++='\\'; *wp++='\\'; }
        else if (c == '\n') { *wp++='\\'; *wp++='n'; }
        else if (c == '\r') { *wp++='\\'; *wp++='r'; }
        else if (c == '\t') { *wp++='\\'; *wp++='t'; }
        else *wp++ = c;
    }
    *wp++ = '"';
    *wp = 0;
    return r;
}
static void nc_json_store_stringified_value(NcVal *m, NcVal *key, NcVal *v) {
    nc_index_set(m, key, (v && v->type != NC_STR) ? nc_json_stringify_any(v, 1) : v);
}
static void nc_json_stringify_map_values_in_place(NcVal *m) {
    for (int i=0; i<m->map->len; i++) {
        nc_json_store_stringified_value(m, nc_str(m->map->keys[i]), m->map->vals[i]);
    }
}
static NcVal *nc_json_stringify_list_as_map(NcVal *lst) {
    NcVal *m = nc_map_new();
    for (int i=0; i<lst->list->len; i++) {
        char keybuf[32]; snprintf(keybuf, sizeof(keybuf), "%d", i);
        nc_json_store_stringified_value(m, nc_str(keybuf), lst->list->items[i]);
    }
    return m;
}
/* Norscode-variant av json.parse med tekstifisering av ikkje-streng-verdiar. */
static NcVal *nc_builtin_json_parse_norscode(NcVal *v) {
    NcVal *r = nc_json_parse_from_ncval(v);
    /* MAP: stringify non-string values */
    if (r->type == NC_MAP) {
        nc_json_stringify_map_values_in_place(r);
        return r;
    }
    /* LIST: konverter til string-keyed map */
    if (r->type == NC_LIST) {
        return nc_json_stringify_list_as_map(r);
    }
    /* Scalars keep std.json semantics: non-string values become their text form. */
    if (r->type != NC_STR) {
        return nc_json_stringify_any(r, 1);
    }
    return r;
}
static NcVal *nc_builtin_json_parse_str(NcVal *v) {
    return nc_json_parse_from_ncval(v);
}
static NcVal *nc_builtin_json_parse_raw(NcVal *v) {
    return nc_json_parse_from_ncval(v);
}
static NcVal *nc_builtin_json_stringify(NcVal *v) {
    return nc_json_stringify_any(v, 0);
}
static NcVal *nc_builtin_json_stringify_smart(NcVal *v) {
    return nc_json_stringify_any(v, 1);
}

/* ─── Sti-hjelpere ─────────────────────────────────────────────────────────── */
#include <sys/stat.h>
static char *nc_path_join_copy(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *r = malloc(la + lb + 2);
    strcpy(r, a);
    if (la > 0 && a[la-1] != '/' && lb > 0) strcat(r, "/");
    strcat(r, b);
    return r;
}
static char *nc_path_basename_ptr(char *path) {
    char *last = strrchr(path, '/');
    return last ? last + 1 : path;
}
static char *nc_path_dirname_copy(char *path) {
    char *last = strrchr(path, '/');
    if (!last) return strdup(".");
    if (last == path) return strdup("/");
    *last = 0;
    return strdup(path);
}
static int nc_path_exists_raw(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}
static char *nc_path_stem_copy(char *path) {
    char *base = nc_path_basename_ptr(path);
    char *dot = strrchr(base, '.');
    if (dot && dot != base) {
        char tmp = *dot;
        *dot = 0;
        char *r = strdup(base);
        *dot = tmp;
        return r;
    }
    return strdup(base);
}
static NcVal *nc_builtin_sti_join(NcVal *a, NcVal *b) {
    char *sa = nc_to_str_raw(a), *sb = nc_to_str_raw(b);
    char *r = nc_path_join_copy(sa, sb);
    free(sa); free(sb);
    return nc_str_own(r);
}
static NcVal *nc_builtin_sti_basename(NcVal *v) {
    char *s = nc_to_str_raw(v);
    NcVal *r = nc_str(nc_path_basename_ptr(s));
    free(s); return r;
}
static NcVal *nc_builtin_sti_dirname(NcVal *v) {
    char *s = nc_to_str_raw(v);
    NcVal *r = nc_str_own(nc_path_dirname_copy(s));
    free(s); return r;
}
static NcVal *nc_builtin_sti_exists(NcVal *v) {
    char *s = nc_to_str_raw(v);
    int ok = nc_path_exists_raw(s);
    free(s); return nc_bool(ok);
}
static NcVal *nc_builtin_sti_stem(NcVal *v) {
    char *s = nc_to_str_raw(v);
    NcVal *r = nc_str_own(nc_path_stem_copy(s));
    free(s); return r;
}

/* ─── Hjelpelag: sti- og miljøoperasjonar ─────────────────────────────────── */
static NcVal *nc_builtin_miljo_sett(NcVal *key, NcVal *val) {
    char *k = nc_to_str_raw(key), *v = nc_to_str_raw(val);
    NcVal *r = nc_env_set_text(k, v);
    free(k); free(v); return r;
}

/* Små compat-restar frå eldre generated namn. */
/* ─── Compat-wrapperar for eldre builtin-symbol ─────────────────────────────── */
NcVal *nc_fn_builtin_tekst_erstatt(NcVal **a, int na) { return nc_builtin_replace(na>0?a[0]:nc_nil(),na>1?a[1]:nc_nil(),na>2?a[2]:nc_nil()); }
NcVal *nc_fn_builtin_tekst_starter_med(NcVal **a, int na) { return nc_builtin_starts_with(na>0?a[0]:nc_nil(),na>1?a[1]:nc_nil()); }
NcVal *nc_fn_builtin_sett_inn(NcVal **a, int na) {
    if (na < 3) return nc_nil();
    nc_index_set(a[0], a[1], a[2]);
    return nc_nil();
}
NcVal *nc_fn_builtin_miljo_sett(NcVal **a, int na) {
    return nc_builtin_miljo_sett(na > 0 ? a[0] : nc_nil(), na > 1 ? a[1] : nc_nil());
}
NcVal *nc_fn_builtin__rt_env_finnes(NcVal **a, int na) {
    return nc_builtin_miljo_finnes(na > 0 ? a[0] : nc_nil());
}
