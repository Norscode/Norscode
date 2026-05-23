/* Builtin implementations for minimal C VM (subset matching frozen bundle). */
#include "mv_builtins.h"
#include "mv_internal.h"
#include "mv_arena.h"
#include "mv_syscall.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if MV_USE_ARENA
#define MV_ALLOC(n) mv_arena_alloc(n)
#define MV_FREE(p) ((void)0)
#else
#define MV_ALLOC(n) malloc(n)
#define MV_FREE(p) free(p)
#endif

static int streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static const char *mv_builtin_short(const char *full) {
    const char *p = strrchr(full, '.');
    return p ? p + 1 : full;
}

static const char *mv_str_arg(const mv_value_t *v) {
    if (!v) return "";
    if (v->tag == MV_VAL_STR) return v->u.s ? v->u.s : "";
    return "";
}

static int64_t mv_int_arg(const mv_value_t *v, int64_t def) {
    if (v && v->tag == MV_VAL_INT) return v->u.i;
    return def;
}

static int mv_list_push_byte(mv_list_t *l, int64_t b) {
    mv_value_t v = MV_INT(b & 0xFF);
    return mv_list_append(l, &v);
}

static int mv_list_extend_bytes(mv_list_t *dest, const mv_list_t *src) {
    if (!dest || !src) return -1;
    for (size_t i = 0; i < src->len; i++) {
        if (src->items[i].tag != MV_VAL_INT) return -1;
        if (mv_list_push_byte(dest, src->items[i].u.i) != 0) return -1;
    }
    return 0;
}

static int mv_list_to_cstring(const mv_list_t *l, char **out, size_t *out_len) {
    size_t cap = 64;
    size_t len = 0;
    char *buf = (char *)MV_ALLOC(cap);
    if (!buf) return -1;
    if (l) {
        for (size_t i = 0; i < l->len; i++) {
            if (l->items[i].tag != MV_VAL_INT) {
                MV_FREE(buf);
                return -1;
            }
            unsigned char b = (unsigned char)(l->items[i].u.i & 0xFF);
            if (len + 1 >= cap) {
                cap *= 2;
                char *nb = (char *)MV_ALLOC(cap);
                if (nb && len) memcpy(nb, buf, len);
                if (!nb) {
                    MV_FREE(buf);
                    return -1;
                }
                buf = nb;
            }
            buf[len++] = (char)b;
        }
    }
    if (len + 1 >= cap) {
        char *nb = (char *)realloc(buf, len + 1);
        if (!nb) {
            MV_FREE(buf);
            return -1;
        }
        buf = nb;
    }
    buf[len] = 0;
    *out = buf;
    *out_len = len;
    return 0;
}

static int mv_mk_byte_list(mv_runtime_t *rt, const unsigned char *data, size_t n, mv_value_t *out) {
    mv_list_t *l = mv_list_new(rt, n);
    if (!l) return -1;
    for (size_t i = 0; i < n; i++) {
        if (mv_list_push_byte(l, data[i]) != 0) return -1;
    }
    out->tag = MV_VAL_LIST;
    out->u.list = l;
    return 0;
}

static int mv_mk_u_le_list(mv_runtime_t *rt, uint64_t n, int bytes, mv_value_t *out) {
    mv_list_t *l = mv_list_new(rt, (size_t)bytes);
    if (!l) return -1;
    for (int i = 0; i < bytes; i++) {
        if (mv_list_push_byte(l, (n >> (8 * i)) & 0xFF) != 0) return -1;
    }
    out->tag = MV_VAL_LIST;
    out->u.list = l;
    return 0;
}

static int mv_write_file_bytes(const char *path, const unsigned char *data, size_t n) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    if (n > 0 && fwrite(data, 1, n, f) != n) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

static int mv_read_file_text(const char *path, char **out) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return -1;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return -1;
    }
    if (sz > 0 && fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        MV_FREE(buf);
        fclose(f);
        return -1;
    }
    buf[sz] = 0;
    fclose(f);
    *out = buf;
    return 0;
}

int mv_call_builtin(
    mv_runtime_t *rt,
    const char *bname,
    mv_value_t *args,
    size_t argc,
    mv_value_t *out) {
    const char *name = mv_builtin_short(bname);

    if (streq(name, "lengde")) {
        if (argc < 1) return MV_ERR_BUILTIN;
        if (args[0].tag == MV_VAL_STR) {
            *out = MV_INT((int64_t)strlen(args[0].u.s ? args[0].u.s : ""));
            return 0;
        }
        if (args[0].tag == MV_VAL_LIST) {
            *out = MV_INT((int64_t)(args[0].u.list ? args[0].u.list->len : 0));
            return 0;
        }
        if (args[0].tag == MV_VAL_MAP) {
            *out = MV_INT((int64_t)(args[0].u.map ? args[0].u.map->len : 0));
            return 0;
        }
        return MV_ERR_TYPE;
    }
    if (streq(name, "legg_til")) {
        if (argc < 2 || args[0].tag != MV_VAL_LIST || !args[0].u.list) return MV_ERR_TYPE;
        return mv_list_append(args[0].u.list, &args[1]) == 0 ? (*out = MV_NIL(), 0) : MV_ERR_BUILTIN;
    }
    if (streq(name, "bytes_append")) {
        if (argc < 2 || args[0].tag != MV_VAL_LIST || !args[0].u.list) return MV_ERR_TYPE;
        if (args[1].tag != MV_VAL_LIST || !args[1].u.list) return MV_ERR_TYPE;
        if (mv_list_extend_bytes(args[0].u.list, args[1].u.list) != 0) return MV_ERR_BUILTIN;
        *out = MV_NIL();
        return 0;
    }
    if (streq(name, "skriv")) {
        for (size_t i = 0; i < argc; i++) {
            if (i) fputc(' ', stderr);
            if (args[i].tag == MV_VAL_STR) fputs(mv_str_arg(&args[i]), stderr);
            else if (args[i].tag == MV_VAL_INT)
                fprintf(stderr, "%lld", (long long)args[i].u.i);
            else if (args[i].tag == MV_VAL_BOOL) fputs(args[i].u.b ? "sann" : "usann", stderr);
            else fputs("?", stderr);
        }
        fputc('\n', stderr);
        *out = MV_NIL();
        return 0;
    }
    if (streq(name, "slice")) {
        if (argc < 3) return MV_ERR_BUILTIN;
        int64_t start = mv_int_arg(&args[1], 0);
        int64_t end = mv_int_arg(&args[2], 0);
        if (args[0].tag == MV_VAL_STR) {
            const char *s = mv_str_arg(&args[0]);
            size_t slen = strlen(s);
            if (start < 0) start = 0;
            if (end < 0 || (size_t)end > slen) end = (int64_t)slen;
            if (start > end) start = end;
            size_t n = (size_t)(end - start);
            char *buf = (char *)mv_track(rt, malloc(n + 1));
            if (!buf) return MV_ERR_BUILTIN;
            memcpy(buf, s + start, n);
            buf[n] = 0;
            out->tag = MV_VAL_STR;
            out->u.s = buf;
            return 0;
        }
        if (args[0].tag == MV_VAL_LIST && args[0].u.list) {
            mv_list_t *src = args[0].u.list;
            if (start < 0) start = 0;
            if (end < 0 || (size_t)end > src->len) end = (int64_t)src->len;
            if (start > end) start = end;
            mv_list_t *dst = mv_list_new(rt, (size_t)(end - start));
            if (!dst) return MV_ERR_BUILTIN;
            for (int64_t i = start; i < end; i++) {
                if (mv_list_append(dst, &src->items[i]) != 0) return MV_ERR_BUILTIN;
            }
            out->tag = MV_VAL_LIST;
            out->u.list = dst;
            return 0;
        }
        return MV_ERR_TYPE;
    }
    if (streq(name, "tekst_til_bytes")) {
        const char *s = (argc > 0) ? mv_str_arg(&args[0]) : "";
        return mv_mk_byte_list(rt, (const unsigned char *)s, strlen(s), out);
    }
    if (streq(name, "bytes_til_tekst")) {
        if (argc < 1 || args[0].tag != MV_VAL_LIST) return MV_ERR_TYPE;
        char *buf = NULL;
        size_t n = 0;
        if (mv_list_to_cstring(args[0].u.list, &buf, &n) != 0) return MV_ERR_BUILTIN;
        char *owned = (char *)mv_track(rt, buf);
        if (!owned) {
            MV_FREE(buf);
            return MV_ERR_BUILTIN;
        }
        out->tag = MV_VAL_STR;
        out->u.s = owned;
        return 0;
    }
    if (streq(name, "tekst_fra_heltall")) {
        int64_t v = mv_int_arg(&args[0], 0);
        char buf[32];
        snprintf(buf, sizeof(buf), "%lld", (long long)v);
        out->tag = MV_VAL_STR;
        out->u.s = mv_strdup(rt, buf);
        return out->u.s ? 0 : MV_ERR_BUILTIN;
    }
    if (streq(name, "tekst_fra_bool")) {
        int truth = argc > 0 && mv_value_truthy(&args[0]);
        out->tag = MV_VAL_STR;
        out->u.s = mv_strdup(rt, truth ? "sann" : "usann");
        return out->u.s ? 0 : MV_ERR_BUILTIN;
    }
    if (streq(name, "heltall_fra_tekst")) {
        const char *s = mv_str_arg(&args[0]);
        *out = MV_INT((int64_t)strtoll(s, NULL, 10));
        return 0;
    }
    if (streq(name, "tekst_starter_med")) {
        const char *a = argc > 0 ? mv_str_arg(&args[0]) : "";
        const char *b = argc > 1 ? mv_str_arg(&args[1]) : "";
        *out = MV_BOOL(strncmp(a, b, strlen(b)) == 0 && strlen(b) <= strlen(a));
        return 0;
    }
    if (streq(name, "tekst_slutter_med")) {
        const char *a = argc > 0 ? mv_str_arg(&args[0]) : "";
        const char *b = argc > 1 ? mv_str_arg(&args[1]) : "";
        size_t la = strlen(a), lb = strlen(b);
        *out = MV_BOOL(lb <= la && strcmp(a + la - lb, b) == 0);
        return 0;
    }
    if (streq(name, "tekst_inneholder")) {
        const char *a = argc > 0 ? mv_str_arg(&args[0]) : "";
        const char *b = argc > 1 ? mv_str_arg(&args[1]) : "";
        *out = MV_BOOL(strstr(a, b) != NULL);
        return 0;
    }
    if (streq(name, "tekst_trim")) {
        const char *s = mv_str_arg(&args[0]);
        while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
        size_t n = strlen(s);
        while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\n' || s[n - 1] == '\r')) n--;
        char *buf = (char *)mv_track(rt, malloc(n + 1));
        if (!buf) return MV_ERR_BUILTIN;
        memcpy(buf, s, n);
        buf[n] = 0;
        out->tag = MV_VAL_STR;
        out->u.s = buf;
        return 0;
    }
    if (streq(name, "heltall_til_u8")) {
        int64_t n = mv_int_arg(&args[0], 0);
        n = ((n % 256) + 256) % 256;
        *out = MV_INT(n);
        return 0;
    }
    if (streq(name, "heltall_til_u16_le")) return mv_mk_u_le_list(rt, (uint64_t)mv_int_arg(&args[0], 0), 2, out);
    if (streq(name, "heltall_til_u32_le")) return mv_mk_u_le_list(rt, (uint64_t)mv_int_arg(&args[0], 0), 4, out);
    if (streq(name, "heltall_til_u64_le")) return mv_mk_u_le_list(rt, (uint64_t)mv_int_arg(&args[0], 0), 8, out);
    if (streq(name, "miljo_hent")) {
        const char *key = mv_str_arg(&args[0]);
        if (mv_sy_available()) {
            char ebuf[4096];
            if (mv_sy_environ_get(key, ebuf, sizeof(ebuf)) != 0) return MV_ERR_BUILTIN;
            out->tag = MV_VAL_STR;
            out->u.s = mv_strdup(rt, ebuf);
            return out->u.s ? 0 : MV_ERR_BUILTIN;
        }
        const char *val = getenv(key);
        out->tag = MV_VAL_STR;
        out->u.s = val ? val : "";
        return 0;
    }
    if (streq(name, "miljo_finnes")) {
        const char *key = mv_str_arg(&args[0]);
        if (mv_sy_available()) {
            char ebuf[4096];
            mv_sy_environ_get(key, ebuf, sizeof(ebuf));
            *out = MV_BOOL(ebuf[0] != 0);
            return 0;
        }
        *out = MV_BOOL(getenv(key) != NULL);
        return 0;
    }
    if (streq(name, "fil_les")) {
        const char *path = mv_str_arg(&args[0]);
        char *text = NULL;
        size_t n = 0;
        if (mv_sy_available()) {
            if (mv_sy_read_file(path, &text, &n) != 0) return MV_ERR_BUILTIN;
        } else {
            if (mv_read_file_text(path, &text) != 0) return MV_ERR_BUILTIN;
        }
        char *owned = (char *)mv_track(rt, text);
        if (!owned) {
            MV_FREE(text);
            return MV_ERR_BUILTIN;
        }
        out->tag = MV_VAL_STR;
        out->u.s = owned;
        return 0;
    }
    if (streq(name, "fil_skriv_bytes")) {
        if (argc < 2) return MV_ERR_BUILTIN;
        const char *path = mv_str_arg(&args[0]);
        if (args[1].tag != MV_VAL_LIST) return MV_ERR_TYPE;
        char *buf = NULL;
        size_t n = 0;
        if (mv_list_to_cstring(args[1].u.list, &buf, &n) != 0) return MV_ERR_BUILTIN;
        int w;
        if (mv_sy_available()) {
            w = mv_sy_write_file(path, buf, n);
        } else {
            w = mv_write_file_bytes(path, (const unsigned char *)buf, n);
        }
        MV_FREE(buf);
        if (w < 0) return MV_ERR_BUILTIN;
        *out = MV_INT((int64_t)n);
        return 0;
    }
    if (streq(name, "har_nokkel")) {
        if (argc < 2 || args[0].tag != MV_VAL_MAP || !args[0].u.map) return MV_ERR_TYPE;
        const char *key = mv_str_arg(&args[1]);
        mv_map_t *m = args[0].u.map;
        for (size_t i = 0; i < m->len; i++) {
            if (m->entries[i].key.tag == MV_VAL_STR && streq(mv_str_arg(&m->entries[i].key), key)) {
                *out = MV_BOOL(1);
                return 0;
            }
        }
        *out = MV_BOOL(0);
        return 0;
    }
    return MV_ERR_BUILTIN;
}
