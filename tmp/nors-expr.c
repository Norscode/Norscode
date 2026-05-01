#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <setjmp.h>

typedef struct { int *data; int len; int cap; } nl_list_int;
typedef struct { char **data; int len; int cap; } nl_list_text;
typedef struct { char **keys; int *values; int len; int cap; } nl_map_int;
typedef struct { char **keys; char **values; int len; int cap; } nl_map_text;
typedef struct { char **keys; int *values; int len; int cap; } nl_map_bool;
typedef struct { char **keys; void **values; int len; int cap; int value_type; } nl_map_any;
#define NL_MAP_ANY_INT 1
#define NL_MAP_ANY_TEXT 2
#define NL_MAP_ANY_BOOL 3
#define NL_MAP_ANY_MAP 4

static nl_list_int *nl_list_int_new(void) {
    nl_list_int *l = (nl_list_int *)malloc(sizeof(nl_list_int));
    if (!l) { perror("malloc"); exit(1); }
    l->len = 0;
    l->cap = 8;
    l->data = (int *)malloc(sizeof(int) * l->cap);
    if (!l->data) { perror("malloc"); exit(1); }
    return l;
}

static nl_list_text *nl_list_text_new(void) {
    nl_list_text *l = (nl_list_text *)malloc(sizeof(nl_list_text));
    if (!l) { perror("malloc"); exit(1); }
    l->len = 0;
    l->cap = 8;
    l->data = (char **)malloc(sizeof(char *) * l->cap);
    if (!l->data) { perror("malloc"); exit(1); }
    return l;
}

static nl_map_int *nl_map_int_new(void) {
    nl_map_int *m = (nl_map_int *)malloc(sizeof(nl_map_int));
    if (!m) { perror("malloc"); exit(1); }
    m->len = 0;
    m->cap = 8;
    m->keys = (char **)malloc(sizeof(char *) * m->cap);
    m->values = (int *)malloc(sizeof(int) * m->cap);
    if (!m->keys || !m->values) { perror("malloc"); exit(1); }
    return m;
}

static nl_map_text *nl_map_text_new(void) {
    nl_map_text *m = (nl_map_text *)malloc(sizeof(nl_map_text));
    if (!m) { perror("malloc"); exit(1); }
    m->len = 0;
    m->cap = 8;
    m->keys = (char **)malloc(sizeof(char *) * m->cap);
    m->values = (char **)malloc(sizeof(char *) * m->cap);
    if (!m->keys || !m->values) { perror("malloc"); exit(1); }
    return m;
}

static nl_map_bool *nl_map_bool_new(void) {
    nl_map_bool *m = (nl_map_bool *)malloc(sizeof(nl_map_bool));
    if (!m) { perror("malloc"); exit(1); }
    m->len = 0;
    m->cap = 8;
    m->keys = (char **)malloc(sizeof(char *) * m->cap);
    m->values = (int *)malloc(sizeof(int) * m->cap);
    if (!m->keys || !m->values) { perror("malloc"); exit(1); }
    return m;
}

static nl_map_any *nl_map_any_new(int value_type) {
    nl_map_any *m = (nl_map_any *)malloc(sizeof(nl_map_any));
    if (!m) { perror("malloc"); exit(1); }
    m->len = 0;
    m->cap = 8;
    m->value_type = value_type;
    m->keys = (char **)malloc(sizeof(char *) * m->cap);
    m->values = (void **)malloc(sizeof(void *) * m->cap);
    if (!m->keys || !m->values) { perror("malloc"); exit(1); }
    return m;
}

static void nl_list_int_ensure(nl_list_int *l, int need) {
    if (need <= l->cap) { return; }
    while (l->cap < need) { l->cap *= 2; }
    l->data = (int *)realloc(l->data, sizeof(int) * l->cap);
    if (!l->data) { perror("realloc"); exit(1); }
}

static void nl_list_text_ensure(nl_list_text *l, int need) {
    if (need <= l->cap) { return; }
    while (l->cap < need) { l->cap *= 2; }
    l->data = (char **)realloc(l->data, sizeof(char *) * l->cap);
    if (!l->data) { perror("realloc"); exit(1); }
}

static void nl_map_int_ensure(nl_map_int *m, int need) {
    if (need <= m->cap) { return; }
    while (m->cap < need) { m->cap *= 2; }
    m->keys = (char **)realloc(m->keys, sizeof(char *) * m->cap);
    m->values = (int *)realloc(m->values, sizeof(int) * m->cap);
    if (!m->keys || !m->values) { perror("realloc"); exit(1); }
}

static void nl_map_text_ensure(nl_map_text *m, int need) {
    if (need <= m->cap) { return; }
    while (m->cap < need) { m->cap *= 2; }
    m->keys = (char **)realloc(m->keys, sizeof(char *) * m->cap);
    m->values = (char **)realloc(m->values, sizeof(char *) * m->cap);
    if (!m->keys || !m->values) { perror("realloc"); exit(1); }
}

static void nl_map_bool_ensure(nl_map_bool *m, int need) {
    if (need <= m->cap) { return; }
    while (m->cap < need) { m->cap *= 2; }
    m->keys = (char **)realloc(m->keys, sizeof(char *) * m->cap);
    m->values = (int *)realloc(m->values, sizeof(int) * m->cap);
    if (!m->keys || !m->values) { perror("realloc"); exit(1); }
}

static void nl_map_any_ensure(nl_map_any *m, int need) {
    if (need <= m->cap) { return; }
    while (m->cap < need) { m->cap *= 2; }
    m->keys = (char **)realloc(m->keys, sizeof(char *) * m->cap);
    m->values = (void **)realloc(m->values, sizeof(void *) * m->cap);
    if (!m->keys || !m->values) { perror("realloc"); exit(1); }
}

static int nl_list_int_len(nl_list_int *l) { return l ? l->len : 0; }
static int nl_list_text_len(nl_list_text *l) { return l ? l->len : 0; }

static int nl_list_int_push(nl_list_int *l, int v) {
    nl_list_int_ensure(l, l->len + 1);
    l->data[l->len++] = v;
    return l->len;
}

static int nl_list_text_push(nl_list_text *l, char *v) {
    nl_list_text_ensure(l, l->len + 1);
    l->data[l->len++] = v;
    return l->len;
}

static int nl_list_int_pop(nl_list_int *l) {
    if (!l || l->len == 0) { return 0; }
    int v = l->data[l->len - 1];
    l->len -= 1;
    return v;
}

static char *nl_list_text_pop(nl_list_text *l) {
    if (!l || l->len == 0) { return ""; }
    char *v = l->data[l->len - 1];
    l->len -= 1;
    return v;
}

static int nl_list_int_remove(nl_list_int *l, int idx) {
    if (!l || idx < 0 || idx >= l->len) { return nl_list_int_len(l); }
    for (int i = idx; i < l->len - 1; i++) { l->data[i] = l->data[i + 1]; }
    l->len -= 1;
    return l->len;
}

static int nl_list_text_remove(nl_list_text *l, int idx) {
    if (!l || idx < 0 || idx >= l->len) { return nl_list_text_len(l); }
    for (int i = idx; i < l->len - 1; i++) { l->data[i] = l->data[i + 1]; }
    l->len -= 1;
    return l->len;
}

static int nl_list_int_set(nl_list_int *l, int idx, int v) {
    if (!l || idx < 0 || idx >= l->len) { return nl_list_int_len(l); }
    l->data[idx] = v;
    return l->len;
}

static int nl_list_text_set(nl_list_text *l, int idx, char *v) {
    if (!l || idx < 0 || idx >= l->len) { return nl_list_text_len(l); }
    l->data[idx] = v;
    return l->len;
}

static char *nl_strdup(const char *s) {
    if (!s) { s = ""; }
    size_t n = strlen(s) + 1;
    char *out = (char *)malloc(n);
    if (!out) { perror("malloc"); exit(1); }
    memcpy(out, s, n);
    return out;
}

static int nl_streq(const char *a, const char *b) {
    if (!a) { a = ""; }
    if (!b) { b = ""; }
    return strcmp(a, b) == 0;
}

static int nl_map_int_len(nl_map_int *m) { return m ? m->len : 0; }
static int nl_map_text_len(nl_map_text *m) { return m ? m->len : 0; }
static int nl_map_bool_len(nl_map_bool *m) { return m ? m->len : 0; }
static int nl_map_any_len(nl_map_any *m) { return m ? m->len : 0; }

static int nl_map_int_find(nl_map_int *m, const char *key) {
    if (!m) { return -1; }
    for (int i = 0; i < m->len; i++) {
        if (nl_streq(m->keys[i], key)) { return i; }
    }
    return -1;
}

static int nl_map_text_find(nl_map_text *m, const char *key) {
    if (!m) { return -1; }
    for (int i = 0; i < m->len; i++) {
        if (nl_streq(m->keys[i], key)) { return i; }
    }
    return -1;
}

static int nl_map_bool_find(nl_map_bool *m, const char *key) {
    if (!m) { return -1; }
    for (int i = 0; i < m->len; i++) {
        if (nl_streq(m->keys[i], key)) { return i; }
    }
    return -1;
}

static int nl_map_any_find(nl_map_any *m, const char *key) {
    if (!m) { return -1; }
    for (int i = 0; i < m->len; i++) {
        if (nl_streq(m->keys[i], key)) { return i; }
    }
    return -1;
}

static int nl_map_int_set(nl_map_int *m, const char *key, int value) {
    if (!m) { return 0; }
    int idx = nl_map_int_find(m, key);
    if (idx >= 0) { m->values[idx] = value; return m->len; }
    nl_map_int_ensure(m, m->len + 1);
    m->keys[m->len] = nl_strdup(key ? key : "");
    m->values[m->len] = value;
    m->len += 1;
    return m->len;
}

static int nl_map_text_set(nl_map_text *m, const char *key, char *value) {
    if (!m) { return 0; }
    int idx = nl_map_text_find(m, key);
    if (idx >= 0) { m->values[idx] = value; return m->len; }
    nl_map_text_ensure(m, m->len + 1);
    m->keys[m->len] = nl_strdup(key ? key : "");
    m->values[m->len] = value;
    m->len += 1;
    return m->len;
}

static int nl_map_bool_set(nl_map_bool *m, const char *key, int value) {
    if (!m) { return 0; }
    int idx = nl_map_bool_find(m, key);
    if (idx >= 0) { m->values[idx] = value; return m->len; }
    nl_map_bool_ensure(m, m->len + 1);
    m->keys[m->len] = nl_strdup(key ? key : "");
    m->values[m->len] = value;
    m->len += 1;
    return m->len;
}

static int nl_map_any_set_int(nl_map_any *m, const char *key, int value) {
    if (!m) { return 0; }
    int idx = nl_map_any_find(m, key);
    if (idx >= 0) { m->values[idx] = (void *)(intptr_t)value; return m->len; }
    nl_map_any_ensure(m, m->len + 1);
    m->keys[m->len] = nl_strdup(key ? key : "");
    m->values[m->len] = (void *)(intptr_t)value;
    m->value_type = NL_MAP_ANY_INT;
    m->len += 1;
    return m->len;
}

static int nl_map_any_set_text(nl_map_any *m, const char *key, char *value) {
    if (!m) { return 0; }
    int idx = nl_map_any_find(m, key);
    if (idx >= 0) { m->values[idx] = (void *)value; return m->len; }
    nl_map_any_ensure(m, m->len + 1);
    m->keys[m->len] = nl_strdup(key ? key : "");
    m->values[m->len] = (void *)value;
    m->value_type = NL_MAP_ANY_TEXT;
    m->len += 1;
    return m->len;
}

static int nl_map_any_set_bool(nl_map_any *m, const char *key, int value) {
    if (!m) { return 0; }
    int idx = nl_map_any_find(m, key);
    if (idx >= 0) { m->values[idx] = (void *)(intptr_t)value; return m->len; }
    nl_map_any_ensure(m, m->len + 1);
    m->keys[m->len] = nl_strdup(key ? key : "");
    m->values[m->len] = (void *)(intptr_t)value;
    m->value_type = NL_MAP_ANY_BOOL;
    m->len += 1;
    return m->len;
}

static int nl_map_any_set_map(nl_map_any *m, const char *key, void *value) {
    if (!m) { return 0; }
    int idx = nl_map_any_find(m, key);
    if (idx >= 0) { m->values[idx] = value; return m->len; }
    nl_map_any_ensure(m, m->len + 1);
    m->keys[m->len] = nl_strdup(key ? key : "");
    m->values[m->len] = value;
    m->value_type = NL_MAP_ANY_MAP;
    m->len += 1;
    return m->len;
}

static int nl_map_int_get(nl_map_int *m, const char *key) {
    int idx = nl_map_int_find(m, key);
    return idx >= 0 ? m->values[idx] : 0;
}

static int nl_map_int_get_required(nl_map_int *m, const char *key) {
    int idx = nl_map_int_find(m, key);
    if (idx < 0) {
        fprintf(stderr, "Ukjent felt: %s\n", key ? key : "");
        exit(1);
    }
    return m->values[idx];
}

static char *nl_map_text_get(nl_map_text *m, const char *key) {
    int idx = nl_map_text_find(m, key);
    return idx >= 0 ? m->values[idx] : "";
}

static char *nl_map_text_get_required(nl_map_text *m, const char *key) {
    int idx = nl_map_text_find(m, key);
    if (idx < 0) {
        fprintf(stderr, "Ukjent felt: %s\n", key ? key : "");
        exit(1);
    }
    return m->values[idx];
}

static int nl_map_bool_get(nl_map_bool *m, const char *key) {
    int idx = nl_map_bool_find(m, key);
    return idx >= 0 ? m->values[idx] : 0;
}

static int nl_map_bool_get_required(nl_map_bool *m, const char *key) {
    int idx = nl_map_bool_find(m, key);
    if (idx < 0) {
        fprintf(stderr, "Ukjent felt: %s\n", key ? key : "");
        exit(1);
    }
    return m->values[idx];
}

static int nl_map_any_get_int(nl_map_any *m, const char *key) {
    if (!m || m->value_type != NL_MAP_ANY_INT) { return 0; }
    int idx = nl_map_any_find(m, key);
    return idx >= 0 ? (int)(intptr_t)m->values[idx] : 0;
}

static int nl_map_any_get_required_int(nl_map_any *m, const char *key) {
    if (!m || m->value_type != NL_MAP_ANY_INT) {
        fprintf(stderr, "Ukjent eller ugyldig felttype for %s\n", key ? key : "");
        exit(1);
    }
    int idx = nl_map_any_find(m, key);
    if (idx < 0) {
        fprintf(stderr, "Ukjent felt: %s\n", key ? key : "");
        exit(1);
    }
    return (int)(intptr_t)m->values[idx];
}

static char *nl_map_any_get_text(nl_map_any *m, const char *key) {
    if (!m || m->value_type != NL_MAP_ANY_TEXT) { return ""; }
    int idx = nl_map_any_find(m, key);
    return idx >= 0 ? (char *)m->values[idx] : "";
}

static char *nl_map_any_get_required_text(nl_map_any *m, const char *key) {
    if (!m || m->value_type != NL_MAP_ANY_TEXT) {
        fprintf(stderr, "Ukjent eller ugyldig felttype for %s\n", key ? key : "");
        exit(1);
    }
    int idx = nl_map_any_find(m, key);
    if (idx < 0) {
        fprintf(stderr, "Ukjent felt: %s\n", key ? key : "");
        exit(1);
    }
    return (char *)m->values[idx];
}

static int nl_map_any_get_bool(nl_map_any *m, const char *key) {
    if (!m || m->value_type != NL_MAP_ANY_BOOL) { return 0; }
    int idx = nl_map_any_find(m, key);
    return idx >= 0 ? (int)(intptr_t)m->values[idx] : 0;
}

static int nl_map_any_get_required_bool(nl_map_any *m, const char *key) {
    if (!m || m->value_type != NL_MAP_ANY_BOOL) {
        fprintf(stderr, "Ukjent eller ugyldig felttype for %s\n", key ? key : "");
        exit(1);
    }
    int idx = nl_map_any_find(m, key);
    if (idx < 0) {
        fprintf(stderr, "Ukjent felt: %s\n", key ? key : "");
        exit(1);
    }
    return (int)(intptr_t)m->values[idx];
}

static void *nl_map_any_get_map(nl_map_any *m, const char *key) {
    if (!m || m->value_type != NL_MAP_ANY_MAP) { return NULL; }
    int idx = nl_map_any_find(m, key);
    return idx >= 0 ? m->values[idx] : NULL;
}

static void *nl_map_any_get_required_map(nl_map_any *m, const char *key) {
    if (!m || m->value_type != NL_MAP_ANY_MAP) {
        fprintf(stderr, "Ukjent eller ugyldig felttype for %s\n", key ? key : "");
        exit(1);
    }
    int idx = nl_map_any_find(m, key);
    if (idx < 0) {
        fprintf(stderr, "Ukjent felt: %s\n", key ? key : "");
        exit(1);
    }
    return m->values[idx];
}

static char *nl_concat(const char *a, const char *b) {
    if (!a) { a = ""; }
    if (!b) { b = ""; }
    size_t al = strlen(a);
    size_t bl = strlen(b);
    char *out = (char *)malloc(al + bl + 1);
    if (!out) { perror("malloc"); exit(1); }
    memcpy(out, a, al);
    memcpy(out + al, b, bl);
    out[al + bl] = '\0';
    return out;
}

static char *nl_int_to_text(int value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return nl_strdup(buffer);
}

static nl_list_text *nl_split_words(const char *s) {
    nl_list_text *out = nl_list_text_new();
    char *copy = nl_strdup(s ? s : "");
    char *tok = strtok(copy, " \t\r\n");
    while (tok) {
        nl_list_text_push(out, nl_strdup(tok));
        tok = strtok(NULL, " \t\r\n");
    }
    return out;
}

static nl_list_text *nl_tokenize_simple(const char *s) {
    nl_list_text *out = nl_list_text_new();
    if (!s) { return out; }
    char token[256];
    int tlen = 0;
    int in_comment = 0;
    for (const char *p = s; ; ++p) {
        char c = *p;
        if (c == '\0') {
            if (tlen > 0) { token[tlen] = '\0'; nl_list_text_push(out, nl_strdup(token)); }
            break;
        }
        if (in_comment) {
            if (c == '\n') { in_comment = 0; }
            continue;
        }
        if (c == '#') {
            if (tlen > 0) { token[tlen] = '\0'; nl_list_text_push(out, nl_strdup(token)); tlen = 0; }
            in_comment = 1;
            continue;
        }
        if (isalnum((unsigned char)c) || c == '_' || c == '-' || (unsigned char)c >= 128) {
            if (tlen < 255) { token[tlen++] = c; }
            continue;
        }
        if (tlen > 0) {
            token[tlen] = '\0';
            nl_list_text_push(out, nl_strdup(token));
            tlen = 0;
        }
    }
    return out;
}

static nl_list_text *nl_tokenize_expression(const char *s) {
    nl_list_text *out = nl_list_text_new();
    if (!s) { return out; }
    int in_comment = 0;
    for (const char *p = s; *p; ) {
        char c = *p;
        if (in_comment) {
            if (c == '\n') { in_comment = 0; }
            p++;
            continue;
        }
        if (c == '#') { in_comment = 1; p++; continue; }
        if (isspace((unsigned char)c)) { p++; continue; }
        if (isdigit((unsigned char)c)) {
            char token[256];
            int tlen = 0;
            while (*p && isdigit((unsigned char)*p)) {
                if (tlen < 255) { token[tlen++] = *p; }
                p++;
            }
            token[tlen] = '\0';
            nl_list_text_push(out, nl_strdup(token));
            continue;
        }
        if (isalpha((unsigned char)c) || c == '_' || (unsigned char)c >= 128) {
            char token[256];
            int tlen = 0;
            while (*p && (isalnum((unsigned char)*p) || *p == '_' || (unsigned char)*p >= 128)) {
                if (tlen < 255) { token[tlen++] = *p; }
                p++;
            }
            token[tlen] = '\0';
            nl_list_text_push(out, nl_strdup(token));
            continue;
        }
        if ((c == '&' && p[1] == '&') || (c == '|' && p[1] == '|') || (c == '=' && p[1] == '=') ||
            (c == '!' && p[1] == '=') || (c == '<' && p[1] == '=') || (c == '>' && p[1] == '=') ||
            (c == '+' && p[1] == '=') || (c == '-' && p[1] == '=') || (c == '*' && p[1] == '=') ||
            (c == '/' && p[1] == '=') || (c == '%' && p[1] == '=')) {
            char token[3];
            token[0] = c;
            token[1] = p[1];
            token[2] = '\0';
            nl_list_text_push(out, nl_strdup(token));
            p += 2;
            continue;
        }
        if (c == '(' || c == ')' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '<' || c == '>' || c == '=' || c == ';') {
            char token[2];
            token[0] = c;
            token[1] = '\0';
            nl_list_text_push(out, nl_strdup(token));
            p++;
            continue;
        }
        char unknown[2];
        unknown[0] = c;
        unknown[1] = '\0';
        nl_list_text_push(out, nl_strdup(unknown));
        p++;
    }
    return out;
}

static char *nl_bool_to_text(int value) {
    return nl_strdup(value ? "sann" : "usann");
}

static int nl_text_to_int(const char *s) {
    if (!s) { return 0; }
    return (int)strtol(s, NULL, 10);
}

typedef struct {
    char *data;
    int len;
    int cap;
} nl_json_builder;

static void nl_json_builder_init(nl_json_builder *b, int initial_cap) {
    b->cap = initial_cap > 0 ? initial_cap : 16;
    b->len = 0;
    b->data = (char *)malloc((size_t)b->cap);
    if (!b->data) { perror("malloc"); exit(1); }
    b->data[0] = '\0';
}

static void nl_json_builder_ensure(nl_json_builder *b, int need) {
    if (b->len + need + 1 < b->cap) { return; }
    while (b->len + need + 1 >= b->cap) { b->cap *= 2; }
    b->data = (char *)realloc(b->data, (size_t)b->cap);
    if (!b->data) { perror("realloc"); exit(1); }
}

static void nl_json_builder_append_char(nl_json_builder *b, char ch) {
    nl_json_builder_ensure(b, 1);
    b->data[b->len++] = ch;
    b->data[b->len] = '\0';
}

static void nl_json_builder_append_text(nl_json_builder *b, const char *text) {
    if (!text) { return; }
    size_t n = strlen(text);
    nl_json_builder_ensure(b, (int)n);
    memcpy(b->data + b->len, text, n + 1);
    b->len += (int)n;
}

static char *nl_json_strdup_range(const char *start, int n) {
    char *out = (char *)malloc((size_t)n + 1);
    if (!out) { perror("malloc"); exit(1); }
    memcpy(out, start, (size_t)n);
    out[n] = '\0';
    return out;
}

static void nl_json_skip_ws(const char *s, int *idx, int len) {
    while (*idx < len) {
        if (s[*idx] == ' ' || s[*idx] == '\n' || s[*idx] == '\t' || s[*idx] == '\r') {
            (*idx)++;
        } else {
            break;
        }
    }
}

static int nl_json_is_digit(char c) {
    return c >= '0' && c <= '9';
}

static char *nl_json_parse_string(const char *s, int *idx, int len) {
    if (*idx >= len || s[*idx] != '"') { return NULL; }
    (*idx)++;
    nl_json_builder out;
    nl_json_builder_init(&out, 32);
    while (*idx < len) {
        char c = s[*idx];
        if (c == '"') {
            (*idx)++;
            return out.data;
        }
        if (c == '\\') {
            (*idx)++;
            if (*idx >= len) { free(out.data); return NULL; }
            char e = s[*idx];
            if (e == '"' || e == '\\' || e == '/') {
                nl_json_builder_append_char(&out, e);
            } else if (e == 'b') { nl_json_builder_append_char(&out, '\b'); }
            else if (e == 'f') { nl_json_builder_append_char(&out, '\f'); }
            else if (e == 'n') { nl_json_builder_append_char(&out, '\n'); }
            else if (e == 'r') { nl_json_builder_append_char(&out, '\r'); }
            else if (e == 't') { nl_json_builder_append_char(&out, '\t'); }
            else if (e == 'u') {
                if (*idx + 4 >= len) { free(out.data); return NULL; }
                (*idx)++;
                char *end = NULL;
                char code_text[5];
                code_text[0] = s[*idx];
                code_text[1] = s[*idx + 1];
                code_text[2] = s[*idx + 2];
                code_text[3] = s[*idx + 3];
                code_text[4] = '\0';
                long code = strtol(code_text, &end, 16);
                if (end && *end == '\0') {
                    nl_json_builder_append_char(&out, (char)code);
                }
                else { free(out.data); return NULL; }
                *idx += 3;
            }
            else { free(out.data); return NULL; }
            (*idx)++;
        } else {
            nl_json_builder_append_char(&out, c);
            (*idx)++;
        }
    }
    free(out.data);
    return NULL;
}

static char *nl_json_quote(const char *value) {
    if (!value) { return nl_strdup("\"\""); }
    nl_json_builder out;
    nl_json_builder_init(&out, 16);
    nl_json_builder_append_char(&out, '"');
    for (const char *p = value; *p; ++p) {
        if (*p == '"' || *p == '\\') {
            nl_json_builder_append_char(&out, '\\');
            nl_json_builder_append_char(&out, *p);
        } else if (*p == '\n') {
            nl_json_builder_append_text(&out, "\\n");
        } else if (*p == '\r') {
            nl_json_builder_append_text(&out, "\\r");
        } else if (*p == '\t') {
            nl_json_builder_append_text(&out, "\\t");
        } else {
            nl_json_builder_append_char(&out, *p);
        }
    }
    nl_json_builder_append_char(&out, '"');
    return out.data;
}

static int nl_json_is_keyword_true(const char *s, int *idx, int len) {
    if (*idx + 3 >= len) { return 0; }
    if (strncmp(&s[*idx], "true", 4) != 0) { return 0; }
    (*idx) += 4;
    return 1;
}

static int nl_json_is_keyword_false(const char *s, int *idx, int len) {
    if (*idx + 4 >= len) { return 0; }
    if (strncmp(&s[*idx], "false", 5) != 0) { return 0; }
    (*idx) += 5;
    return 1;
}

static int nl_json_is_keyword_null(const char *s, int *idx, int len) {
    if (*idx + 3 >= len) { return 0; }
    if (strncmp(&s[*idx], "null", 4) != 0) { return 0; }
    (*idx) += 4;
    return 1;
}

static int nl_json_parse_value_raw(const char *s, int *idx, int len, char **raw);
static void nl_json_report_error(const char *text, int idx, int len, const char *message) {
    int line = 1;
    int column = 1;
    for (int i = 0; i < len && i < idx; i++) {
        if (text[i] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }
    int preview_len = (idx < len) ? 24 : 0;
    if (preview_len > 0 && idx + preview_len > len) {
        preview_len = len - idx;
    }
    char preview[64] = {0};
    if (idx >= 0 && idx < len && preview_len > 0) {
        int copy_len = preview_len > 60 ? 60 : preview_len;
        for (int i = 0; i < copy_len; i++) {
            char c = text[idx + i];
            preview[i] = (c == '\n' || c == '\r' || c == '\t') ? ' ' : c;
        }
        preview[copy_len] = '\0';
    }
    fprintf(stderr, "json_parse feilet: %s (linje %d, kolonne %d, posisjon %d)", message, line, column, idx);
    if (idx >= 0 && idx < len) {
        fprintf(stderr, ", tegn: '%c', kontekst: '%s'", text[idx], preview);
    }
    fprintf(stderr, "\n");
}

static int nl_json_skip_object_raw(const char *s, int *idx, int len) {
    if (*idx >= len || s[*idx] != '{') { return 0; }
    (*idx)++;
    nl_json_skip_ws(s, idx, len);
    if (*idx < len && s[*idx] == '}') { (*idx)++; return 1; }
    while (*idx < len) {
        if (s[*idx] != '"') { return 0; }
        char *key = nl_json_parse_string(s, idx, len);
        if (!key) { return 0; }
        free(key);
        nl_json_skip_ws(s, idx, len);
        if (*idx >= len || s[*idx] != ':') { return 0; }
        (*idx)++;
        nl_json_skip_ws(s, idx, len);
        if (!nl_json_parse_value_raw(s, idx, len, NULL)) { return 0; }
        nl_json_skip_ws(s, idx, len);
        if (*idx >= len) { return 0; }
        if (s[*idx] == ',') {
            (*idx)++;
            nl_json_skip_ws(s, idx, len);
        } else if (s[*idx] == '}') {
            (*idx)++;
            return 1;
        } else { return 0; }
        }
    return 0;
    }
    
    static int nl_json_skip_array_raw(const char *s, int *idx, int len) {
        if (*idx >= len || s[*idx] != '[') { return 0; }
        (*idx)++;
        nl_json_skip_ws(s, idx, len);
        if (*idx < len && s[*idx] == ']') { (*idx)++; return 1; }
        while (*idx < len) {
            if (!nl_json_parse_value_raw(s, idx, len, NULL)) { return 0; }
            nl_json_skip_ws(s, idx, len);
            if (*idx >= len) { return 0; }
            if (s[*idx] == ',') {
                (*idx)++;
                nl_json_skip_ws(s, idx, len);
            } else if (s[*idx] == ']') {
                (*idx)++;
                return 1;
            } else { return 0; }
            }
        return 0;
        }
        
        static int nl_json_parse_value_raw(const char *s, int *idx, int len, char **raw) {
            nl_json_skip_ws(s, idx, len);
            if (*idx >= len) { return 0; }
            char c = s[*idx];
            int start = *idx;
            if (c == '"') {
                char *value = nl_json_parse_string(s, idx, len);
                if (!value) { return 0; }
                if (raw) { *raw = value; } else { free(value); }
                return 1;
            }
            if (c == '{' || c == '[') {
                int i = *idx;
                int ok = (c == '{') ? nl_json_skip_object_raw(s, idx, len) : nl_json_skip_array_raw(s, idx, len);
                if (!ok) { return 0; }
                if (raw) { *raw = nl_json_strdup_range(s + i, *idx - i); }
                return 1;
            }
            if (c == '-' || nl_json_is_digit(c)) {
                while (*idx < len && (nl_json_is_digit(s[*idx]) || s[*idx] == '-' || s[*idx] == '+' || s[*idx] == '.' || s[*idx] == 'e' || s[*idx] == 'E')) {
                    (*idx)++;
                }
                if (raw) { *raw = nl_json_strdup_range(s + start, *idx - start); }
                return 1;
            }
            if (c == 't' && nl_json_is_keyword_true(s, idx, len)) {
                if (raw) { *raw = nl_strdup("true"); }
                return 1;
            }
            if (c == 'f' && nl_json_is_keyword_false(s, idx, len)) {
                if (raw) { *raw = nl_strdup("false"); }
                return 1;
            }
            if (c == 'n' && nl_json_is_keyword_null(s, idx, len)) {
                if (raw) { *raw = nl_strdup(""); }
                return 1;
            }
            return 0;
        }
        
        static int nl_json_is_number_like(const char *value) {
            if (!value || !*value) { return 0; }
            char *end = NULL;
            strtod(value, &end);
            return end && *end == '\0';
        }
        
        static char *nl_json_serialize_value(const char *raw) {
            if (!raw) { return nl_strdup("null"); }
            if (raw[0] == '[' || raw[0] == '{' || nl_streq(raw, "true") || nl_streq(raw, "false") ||
                nl_streq(raw, "null") || nl_json_is_number_like(raw)) {
                return nl_strdup(raw);
            }
            return nl_json_quote(raw);
        }
        
        static nl_map_text *json_parse(const char *text) {
            if (!text) { return nl_map_text_new(); }
            int len = (int)strlen(text);
            int idx = 0;
            nl_json_skip_ws(text, &idx, len);
            if (idx >= len) { return nl_map_text_new(); }
            nl_map_text *result = nl_map_text_new();
            if (text[idx] == '{') {
                idx++;
                nl_json_skip_ws(text, &idx, len);
                if (idx < len && text[idx] == '}') {
                    return result;
                }
                while (idx < len) {
                    if (text[idx] != '"') {
                        nl_json_report_error(text, idx, len, "ugyldig nøkkel");
                        exit(1);
                    }
                    char *raw_key = nl_json_parse_string(text, &idx, len);
                    if (!raw_key) {
                        nl_json_report_error(text, idx, len, "ugyldig nøkkel");
                        exit(1);
                    }
                    nl_json_skip_ws(text, &idx, len);
                    if (idx >= len || text[idx] != ':') {
                        free(raw_key);
                        nl_json_report_error(text, idx, len, "mangler ':' etter nøkkel");
                        exit(1);
                    }
                    idx++;
                    nl_json_skip_ws(text, &idx, len);
                    char *raw_value = NULL;
                    if (!nl_json_parse_value_raw(text, &idx, len, &raw_value)) {
                        free(raw_key);
                        nl_json_report_error(text, idx, len, "ugyldig verdi");
                        exit(1);
                    }
                    nl_map_text_set(result, raw_key, raw_value ? raw_value : nl_strdup(""));
                    free(raw_key);
                    nl_json_skip_ws(text, &idx, len);
                    if (idx >= len) {
                        nl_json_report_error(text, idx, len, "mangler avsluttende '}'");
                        exit(1);
                    }
                    if (text[idx] == ',') {
                        idx++;
                        nl_json_skip_ws(text, &idx, len);
                    continue;
                    }
                    if (text[idx] == '}') {
                        idx++;
                        break;
                    }
                    nl_json_report_error(text, idx, len, "ugyldig objekt-separator");
                    exit(1);
                }
                return result;
            }
            if (text[idx] == '[') {
                idx++;
                nl_json_skip_ws(text, &idx, len);
                if (idx < len && text[idx] == ']') {
                    return result;
                }
                int index = 0;
                while (idx < len) {
                    char *raw_value = NULL;
                    if (!nl_json_parse_value_raw(text, &idx, len, &raw_value)) {
                        nl_json_report_error(text, idx, len, "ugyldig listeverdi");
                        exit(1);
                    }
                    char *raw_key = nl_int_to_text(index);
                    nl_map_text_set(result, raw_key, raw_value ? raw_value : nl_strdup(""));
                    index += 1;
                    nl_json_skip_ws(text, &idx, len);
                    if (idx >= len) {
                        nl_json_report_error(text, idx, len, "mangler avsluttende ']'");
                        exit(1);
                    }
                    if (text[idx] == ',') {
                        idx++;
                        nl_json_skip_ws(text, &idx, len);
                    continue;
                    }
                    if (text[idx] == ']') {
                        idx++;
                        return result;
                    }
                    nl_json_report_error(text, idx, len, "ugyldig liste-separator");
                    exit(1);
                }
                nl_json_report_error(text, idx, len, "ugyldig liste");
                exit(1);
                return result;
            }
            nl_json_report_error(text, idx, len, "forventer objekt eller liste");
            exit(1);
            return result;
        }
        
        static char *json_stringify(nl_map_text *value) {
            nl_json_builder out;
            nl_json_builder_init(&out, 32);
            nl_json_builder_append_char(&out, '{');
            for (int i = 0; i < value->len; i++) {
                if (i > 0) {
                    nl_json_builder_append_char(&out, ',');
                }
                char *key = nl_json_quote(value->keys[i]);
                nl_json_builder_append_text(&out, key);
                free(key);
                nl_json_builder_append_char(&out, ':');
                char *serialized = nl_json_serialize_value(value->values[i]);
                nl_json_builder_append_text(&out, serialized);
                free(serialized);
            }
            nl_json_builder_append_char(&out, '}');
            return out.data;
        }
        
        static int nl_assert(int cond) {
            if (!cond) {
                printf("assert failed\n");
                exit(1);
            }
            return 0;
        }
        
        static int nl_assert_eq_int(int a, int b) {
            if (a != b) {
                printf("assert_eq failed: %d != %d\n", a, b);
                exit(1);
            }
            return 0;
        }
        
        static int nl_assert_ne_int(int a, int b) {
            if (a == b) {
                printf("assert_ne failed: %d == %d\n", a, b);
                exit(1);
            }
            return 0;
        }
        
        static int nl_assert_eq_text(const char *a, const char *b) {
            if (!nl_streq(a, b)) {
                printf("assert_eq failed: \"%s\" != \"%s\"\n", a ? a : "", b ? b : "");
                exit(1);
            }
            return 0;
        }
        
        static int nl_assert_ne_text(const char *a, const char *b) {
            if (nl_streq(a, b)) {
                printf("assert_ne failed: \"%s\" == \"%s\"\n", a ? a : "", b ? b : "");
                exit(1);
            }
            return 0;
        }
        
        static void nl_print_text(const char *s) {
            printf("%s\n", s ? s : "");
        }
        
        typedef struct nl_try_frame nl_try_frame;
        struct nl_try_frame {
            nl_try_frame *previous;
            jmp_buf jump_point;
            char *error_message;
        };
        
        static nl_try_frame *nl_try_stack = NULL;
        
        static void nl_push_try_frame(nl_try_frame *frame) {
            frame->previous = nl_try_stack;
            frame->error_message = NULL;
            nl_try_stack = frame;
        }
        
        static void nl_pop_try_frame(void) {
            if (nl_try_stack) {
                nl_try_stack = nl_try_stack->previous;
            }
        }
        
        static void nl_throw(const char *message) {
            if (!nl_try_stack) {
                fprintf(stderr, "Ubehandlet feil: %s\n", message ? message : "");
                exit(1);
            }
            nl_try_stack->error_message = nl_strdup(message ? message : "");
            longjmp(nl_try_stack->jump_point, 1);
        }
        
        char * selfhost__compiler__append_linje(char * kilde, char * linje) {
            return nl_concat(nl_concat(kilde, linje), "\n");
            return "";
        }
        
        char * selfhost__compiler__instruksjon_til_tekst(char * op, int verdi) {
            if ((((((nl_streq(op, "PUSH") || nl_streq(op, "LABEL")) || nl_streq(op, "JMP")) || nl_streq(op, "JZ")) || nl_streq(op, "CALL")) || nl_streq(op, "STORE")) || nl_streq(op, "LOAD")) {
                return nl_concat(nl_concat(op, " "), nl_int_to_text(verdi));
            }
            return op;
            return "";
        }
        
        char * selfhost__compiler__disasm_program(nl_list_text* ops, nl_list_int* verdier) {
            char * out = "";
            int i = 0;
            if (nl_list_text_len(ops) != nl_list_int_len(verdier)) {
                return "/* feil: ops og verdier må ha samme lengde */";
            }
            while (i < nl_list_text_len(ops)) {
                out = selfhost__compiler__append_linje(out, nl_concat(nl_concat(nl_int_to_text(i), ": "), selfhost__compiler__instruksjon_til_tekst(ops->data[i], verdier->data[i])));
                i = (i + 1);
            }
            return out;
            return "";
        }
        
        int selfhost__compiler__op_krever_arg(char * op) {
            if ((((((nl_streq(op, "PUSH") || nl_streq(op, "LABEL")) || nl_streq(op, "JMP")) || nl_streq(op, "JZ")) || nl_streq(op, "CALL")) || nl_streq(op, "STORE")) || nl_streq(op, "LOAD")) {
                return 1;
            }
            return 0;
            return 0;
        }
        
        int selfhost__compiler__op_kjent(char * op) {
            if (selfhost__compiler__op_krever_arg(op)) {
                return 1;
            }
            if ((((nl_streq(op, "ADD") || nl_streq(op, "SUB")) || nl_streq(op, "MUL")) || nl_streq(op, "DIV")) || nl_streq(op, "MOD")) {
                return 1;
            }
            if ((((nl_streq(op, "EQ") || nl_streq(op, "GT")) || nl_streq(op, "LT")) || nl_streq(op, "AND")) || nl_streq(op, "OR")) {
                return 1;
            }
            if ((((nl_streq(op, "NOT") || nl_streq(op, "DUP")) || nl_streq(op, "POP")) || nl_streq(op, "SWAP")) || nl_streq(op, "OVER")) {
                return 1;
            }
            if ((nl_streq(op, "PRINT") || nl_streq(op, "HALT")) || nl_streq(op, "RET")) {
                return 1;
            }
            return 0;
            return 0;
        }
        
        int selfhost__compiler__er_heltall_token(char * tok) {
            int n = nl_text_to_int(tok);
            return nl_streq(nl_int_to_text(n), tok);
            return 0;
        }
        
        char * selfhost__compiler__normaliser_norsk_token(char * tok) {
            if (nl_streq(tok, "let")) {
                return "la";
            }
            if (nl_streq(tok, "const")) {
                return "la";
            }
            if (nl_streq(tok, "var")) {
                return "la";
            }
            if (nl_streq(tok, "declare")) {
                return "la";
            }
            if (nl_streq(tok, "set")) {
                return "sett";
            }
            if (nl_streq(tok, "assign")) {
                return "sett";
            }
            if (nl_streq(tok, "return")) {
                return "returner";
            }
            if (nl_streq(tok, "if")) {
                return "hvis";
            }
            if (nl_streq(tok, "then")) {
                return "da";
            }
            if (nl_streq(tok, "else")) {
                return "ellers";
            }
            if (nl_streq(tok, "elseif") || nl_streq(tok, "elsif")) {
                return "ellers_hvis";
            }
            if (nl_streq(tok, "and")) {
                return "og";
            }
            if (nl_streq(tok, "or")) {
                return "eller";
            }
            if (nl_streq(tok, "not")) {
                return "ikke";
            }
            if (nl_streq(tok, "and_not") || nl_streq(tok, "andnot")) {
                return "nand";
            }
            if (nl_streq(tok, "or_not") || nl_streq(tok, "ornot")) {
                return "nor";
            }
            if (nl_streq(tok, "implies")) {
                return "impliserer";
            }
            if (((nl_streq(tok, "this_implies") || nl_streq(tok, "thisimplies")) || nl_streq(tok, "dette_impliserer")) || nl_streq(tok, "detteimpliserer")) {
                return "impliserer";
            }
            if (((nl_streq(tok, "implies_that") || nl_streq(tok, "impliesthat")) || nl_streq(tok, "impliserer_at")) || nl_streq(tok, "implisererat")) {
                return "impliserer";
            }
            if (nl_streq(tok, "medforer")) {
                return "impliserer";
            }
            if (nl_streq(tok, "impl") || nl_streq(tok, "impliser")) {
                return "impliserer";
            }
            if ((nl_streq(tok, "follows") || nl_streq(tok, "folger_av")) || nl_streq(tok, "folgerav")) {
                return "impliserer";
            }
            if (((nl_streq(tok, "it_follows_that") || nl_streq(tok, "itfollowsthat")) || nl_streq(tok, "det_folger_at")) || nl_streq(tok, "detfolgerat")) {
                return "impliserer";
            }
            if (nl_streq(tok, "therefore") || nl_streq(tok, "derfor")) {
                return "impliserer";
            }
            if (nl_streq(tok, "hence") || nl_streq(tok, "altsa")) {
                return "impliserer";
            }
            if (nl_streq(tok, "thus") || nl_streq(tok, "dermed")) {
                return "impliserer";
            }
            if (nl_streq(tok, "consequently") || nl_streq(tok, "folgelig")) {
                return "impliserer";
            }
            if (nl_streq(tok, "thereby") || nl_streq(tok, "derved")) {
                return "impliserer";
            }
            if (nl_streq(tok, "thereupon") || nl_streq(tok, "derpa")) {
                return "impliserer";
            }
            if (nl_streq(tok, "ergo") || nl_streq(tok, "saledes")) {
                return "impliserer";
            }
            if (nl_streq(tok, "infer") || nl_streq(tok, "derav")) {
                return "impliserer";
            }
            if (((nl_streq(tok, "as_consequence") || nl_streq(tok, "asconsequence")) || nl_streq(tok, "som_konsekvens")) || nl_streq(tok, "somkonsekvens")) {
                return "impliserer";
            }
            if (((nl_streq(tok, "as_a_result") || nl_streq(tok, "asaresult")) || nl_streq(tok, "som_resultat")) || nl_streq(tok, "somresultat")) {
                return "impliserer";
            }
            if (((nl_streq(tok, "only_if") || nl_streq(tok, "onlyif")) || nl_streq(tok, "kun_hvis")) || nl_streq(tok, "kunhvis")) {
                return "impliserer";
            }
            if (nl_streq(tok, "requires") || nl_streq(tok, "krever")) {
                return "impliserer";
            }
            if (nl_streq(tok, "implied_by") || nl_streq(tok, "impliedby")) {
                return "impliseres_av";
            }
            if (nl_streq(tok, "given") || nl_streq(tok, "gitt")) {
                return "impliseres_av";
            }
            if (nl_streq(tok, "provided") || nl_streq(tok, "forutsatt")) {
                return "impliseres_av";
            }
            if (nl_streq(tok, "assuming") || nl_streq(tok, "antar")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "assuming_that") || nl_streq(tok, "assumingthat")) || nl_streq(tok, "antar_at")) || nl_streq(tok, "antarat")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "when_assuming") || nl_streq(tok, "whenassuming")) || nl_streq(tok, "nar_antatt")) || nl_streq(tok, "narantatt")) {
                return "impliseres_av";
            }
            if (nl_streq(tok, "presuming") || nl_streq(tok, "forutsattvis")) {
                return "impliseres_av";
            }
            if (nl_streq(tok, "since") || nl_streq(tok, "siden")) {
                return "impliseres_av";
            }
            if (nl_streq(tok, "because") || nl_streq(tok, "fordi")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "given_that") || nl_streq(tok, "giventhat")) || nl_streq(tok, "gitt_at")) || nl_streq(tok, "gittat")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "provided_that") || nl_streq(tok, "providedthat")) || nl_streq(tok, "forutsatt_at")) || nl_streq(tok, "forutsattat")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "follows_if") || nl_streq(tok, "followsif")) || nl_streq(tok, "folger_hvis")) || nl_streq(tok, "folgerhvis")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "if_given") || nl_streq(tok, "ifgiven")) || nl_streq(tok, "hvis_gitt")) || nl_streq(tok, "hvisgitt")) {
                return "impliseres_av";
            }
            if (nl_streq(tok, "as") || nl_streq(tok, "ettersom")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "inasmuch_as") || nl_streq(tok, "inasmuchas")) || nl_streq(tok, "i_og_med_at")) || nl_streq(tok, "iogmedat")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "on_condition_that") || nl_streq(tok, "onconditionthat")) || nl_streq(tok, "pa_vilkar_av_at")) || nl_streq(tok, "pavilkaravat")) {
                return "impliseres_av";
            }
            if ((nl_streq(tok, "granted") || nl_streq(tok, "gitt_dette")) || nl_streq(tok, "gittdette")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "with_premise") || nl_streq(tok, "withpremise")) || nl_streq(tok, "med_premiss")) || nl_streq(tok, "medpremiss")) {
                return "impliseres_av";
            }
            if (((((((((((((((nl_streq(tok, "given_premise") || nl_streq(tok, "givenpremise")) || nl_streq(tok, "gitt_premiss")) || nl_streq(tok, "gittpremiss")) || nl_streq(tok, "premise_given")) || nl_streq(tok, "premisegiven")) || nl_streq(tok, "premiss_gitt")) || nl_streq(tok, "premissgitt")) || nl_streq(tok, "premise_assumed")) || nl_streq(tok, "premiseassumed")) || nl_streq(tok, "premiss_antatt")) || nl_streq(tok, "premissantatt")) || nl_streq(tok, "premise_condition")) || nl_streq(tok, "premisecondition")) || nl_streq(tok, "premiss_vilkar")) || nl_streq(tok, "premissvilkar")) {
                return "impliseres_av";
            }
            if (((nl_streq(tok, "follows_from") || nl_streq(tok, "followsfrom")) || nl_streq(tok, "folger_fra")) || nl_streq(tok, "folgerfra")) {
                return "impliseres_av";
            }
            if (nl_streq(tok, "iff")) {
                return "xnor";
            }
            if (nl_streq(tok, "equiv") || nl_streq(tok, "ekvivalent")) {
                return "xnor";
            }
            if (((nl_streq(tok, "if_and_only_if") || nl_streq(tok, "ifandonlyif")) || nl_streq(tok, "hvis_og_bare_hvis")) || nl_streq(tok, "hvisogbarehvis")) {
                return "xnor";
            }
            if (nl_streq(tok, "og_ikke") || nl_streq(tok, "ogikke")) {
                return "nand";
            }
            if (nl_streq(tok, "eller_ikke") || nl_streq(tok, "ellerikke")) {
                return "nor";
            }
            if (nl_streq(tok, "xeller")) {
                return "xor";
            }
            if (nl_streq(tok, "xeller_ikke") || nl_streq(tok, "xellerikke")) {
                return "xnor";
            }
            if (nl_streq(tok, "is")) {
                return "er";
            }
            if (nl_streq(tok, "eq")) {
                return "er";
            }
            if (nl_streq(tok, "neq")) {
                return "ikke_er";
            }
            if (nl_streq(tok, "<>")) {
                return "ikke_er";
            }
            if ((nl_streq(tok, "is_not") || nl_streq(tok, "isnot")) || nl_streq(tok, "isnt")) {
                return "ikke_er";
            }
            if ((((((((((((((((nl_streq(tok, "not_equal") || nl_streq(tok, "not_equals")) || nl_streq(tok, "not_equal_to")) || nl_streq(tok, "notequal")) || nl_streq(tok, "notequals")) || nl_streq(tok, "is_not_equal")) || nl_streq(tok, "is_not_equals")) || nl_streq(tok, "is_not_equal_to")) || nl_streq(tok, "isnotequal")) || nl_streq(tok, "isnotequals")) || nl_streq(tok, "isnotequalto")) || nl_streq(tok, "isnt_equal")) || nl_streq(tok, "isnt_equals")) || nl_streq(tok, "isnt_equal_to")) || nl_streq(tok, "isntequal")) || nl_streq(tok, "isntequals")) || nl_streq(tok, "isntequalto")) {
                return "ikke_er";
            }
            if ((((((((nl_streq(tok, "equals") || nl_streq(tok, "equal")) || nl_streq(tok, "equal_to")) || nl_streq(tok, "is_equal")) || nl_streq(tok, "is_equals")) || nl_streq(tok, "is_equal_to")) || nl_streq(tok, "isequal")) || nl_streq(tok, "isequals")) || nl_streq(tok, "isequalto")) {
                return "er";
            }
            if (nl_streq(tok, "less_than") || nl_streq(tok, "lessthan")) {
                return "mindre_enn";
            }
            if ((nl_streq(tok, "is_less_than") || nl_streq(tok, "is_lessthan")) || nl_streq(tok, "islessthan")) {
                return "mindre_enn";
            }
            if (nl_streq(tok, "greater_than") || nl_streq(tok, "greaterthan")) {
                return "storre_enn";
            }
            if ((nl_streq(tok, "is_greater_than") || nl_streq(tok, "is_greaterthan")) || nl_streq(tok, "isgreaterthan")) {
                return "storre_enn";
            }
            if (nl_streq(tok, "less_equal") || nl_streq(tok, "lessequal")) {
                return "mindre_eller_lik";
            }
            if ((nl_streq(tok, "is_less_equal") || nl_streq(tok, "is_lessequal")) || nl_streq(tok, "islessequal")) {
                return "mindre_eller_lik";
            }
            if (nl_streq(tok, "greater_equal") || nl_streq(tok, "greaterequal")) {
                return "storre_eller_lik";
            }
            if ((nl_streq(tok, "is_greater_equal") || nl_streq(tok, "is_greaterequal")) || nl_streq(tok, "isgreaterequal")) {
                return "storre_eller_lik";
            }
            if (((nl_streq(tok, "less_or_equal") || nl_streq(tok, "less_or_equals")) || nl_streq(tok, "lessorequal")) || nl_streq(tok, "lessorequals")) {
                return "mindre_eller_lik";
            }
            if (((nl_streq(tok, "is_less_or_equal") || nl_streq(tok, "is_less_or_equals")) || nl_streq(tok, "islessorequal")) || nl_streq(tok, "islessorequals")) {
                return "mindre_eller_lik";
            }
            if (((nl_streq(tok, "is_less_or_equal_to") || nl_streq(tok, "is_less_or_equals_to")) || nl_streq(tok, "islessorequalto")) || nl_streq(tok, "islessorequalsto")) {
                return "mindre_eller_lik";
            }
            if (((((((nl_streq(tok, "is_less_than_or_equal") || nl_streq(tok, "is_less_than_or_equals")) || nl_streq(tok, "islessthanorequal")) || nl_streq(tok, "islessthanorequals")) || nl_streq(tok, "is_less_than_or_equal_to")) || nl_streq(tok, "is_less_than_or_equals_to")) || nl_streq(tok, "islessthanorequalto")) || nl_streq(tok, "islessthanorequalsto")) {
                return "mindre_eller_lik";
            }
            if (((nl_streq(tok, "greater_or_equal") || nl_streq(tok, "greater_or_equals")) || nl_streq(tok, "greaterorequal")) || nl_streq(tok, "greaterorequals")) {
                return "storre_eller_lik";
            }
            if (((nl_streq(tok, "is_greater_or_equal") || nl_streq(tok, "is_greater_or_equals")) || nl_streq(tok, "isgreaterorequal")) || nl_streq(tok, "isgreaterorequals")) {
                return "storre_eller_lik";
            }
            if (((nl_streq(tok, "is_greater_or_equal_to") || nl_streq(tok, "is_greater_or_equals_to")) || nl_streq(tok, "isgreaterorequalto")) || nl_streq(tok, "isgreaterorequalsto")) {
                return "storre_eller_lik";
            }
            if (((((((nl_streq(tok, "is_greater_than_or_equal") || nl_streq(tok, "is_greater_than_or_equals")) || nl_streq(tok, "isgreaterthanorequal")) || nl_streq(tok, "isgreaterthanorequals")) || nl_streq(tok, "is_greater_than_or_equal_to")) || nl_streq(tok, "is_greater_than_or_equals_to")) || nl_streq(tok, "isgreaterthanorequalto")) || nl_streq(tok, "isgreaterthanorequalsto")) {
                return "storre_eller_lik";
            }
            if (((nl_streq(tok, "times") || nl_streq(tok, "multiply")) || nl_streq(tok, "multiplies")) || nl_streq(tok, "multiplied")) {
                return "*";
            }
            if ((nl_streq(tok, "add") || nl_streq(tok, "adds")) || nl_streq(tok, "added")) {
                return "+";
            }
            if ((nl_streq(tok, "subtract") || nl_streq(tok, "subtracts")) || nl_streq(tok, "subtracted")) {
                return "-";
            }
            if (nl_streq(tok, "divide") || nl_streq(tok, "divides")) {
                return "/";
            }
            if (((nl_streq(tok, "mod_of") || nl_streq(tok, "modulo_of")) || nl_streq(tok, "modof")) || nl_streq(tok, "moduloof")) {
                return "%";
            }
            if ((nl_streq(tok, "remainder") || nl_streq(tok, "remainder_of")) || nl_streq(tok, "remainderof")) {
                return "%";
            }
            if (((nl_streq(tok, "multiplied_by") || nl_streq(tok, "multipliedby")) || nl_streq(tok, "multiply_by")) || nl_streq(tok, "multiplyby")) {
                return "*";
            }
            if (((nl_streq(tok, "divided_by") || nl_streq(tok, "dividedby")) || nl_streq(tok, "divide_by")) || nl_streq(tok, "divideby")) {
                return "/";
            }
            if ((((nl_streq(tok, "elif") || nl_streq(tok, "else_if")) || nl_streq(tok, "else_hvis")) || nl_streq(tok, "elsehvis")) || nl_streq(tok, "else-hvis")) {
                return "ellers_hvis";
            }
            if (nl_streq(tok, "ellershvis")) {
                return "ellers_hvis";
            }
            if (nl_streq(tok, "på")) {
                return "pa";
            }
            if (nl_streq(tok, "delt_på")) {
                return "delt_pa";
            }
            if (nl_streq(tok, "delt_paa")) {
                return "delt_pa";
            }
            if (nl_streq(tok, "dele_på") || nl_streq(tok, "dele_paa")) {
                return "dele_pa";
            }
            if (nl_streq(tok, "del_på") || nl_streq(tok, "del_paa")) {
                return "del_pa";
            }
            if (nl_streq(tok, "deles_på") || nl_streq(tok, "deles_paa")) {
                return "deles_pa";
            }
            if (nl_streq(tok, "deler_på") || nl_streq(tok, "deler_paa")) {
                return "deler_pa";
            }
            if (nl_streq(tok, "dele_seg_på") || nl_streq(tok, "dele_seg_paa")) {
                return "dele_seg_pa";
            }
            if (nl_streq(tok, "deler_seg_på") || nl_streq(tok, "deler_seg_paa")) {
                return "deler_seg_pa";
            }
            if (nl_streq(tok, "divider_seg_på") || nl_streq(tok, "divider_seg_paa")) {
                return "divider_seg_pa";
            }
            if (nl_streq(tok, "dividere_seg_på") || nl_streq(tok, "dividere_seg_paa")) {
                return "dividere_seg_pa";
            }
            if (nl_streq(tok, "dividerer_seg_på") || nl_streq(tok, "dividerer_seg_paa")) {
                return "dividerer_seg_pa";
            }
            if (nl_streq(tok, "divider_på") || nl_streq(tok, "divider_paa")) {
                return "divider_pa";
            }
            if (nl_streq(tok, "dividere_på") || nl_streq(tok, "dividere_paa")) {
                return "dividere_pa";
            }
            if (nl_streq(tok, "dividerer_på") || nl_streq(tok, "dividerer_paa")) {
                return "dividerer_pa";
            }
            if (nl_streq(tok, "dividert_på") || nl_streq(tok, "dividert_paa")) {
                return "dividert_pa";
            }
            if (nl_streq(tok, "divideres_på") || nl_streq(tok, "divideres_paa")) {
                return "divideres_pa";
            }
            if (nl_streq(tok, "større")) {
                return "storre";
            }
            if ((nl_streq(tok, "større_enn") || nl_streq(tok, "størreenn")) || nl_streq(tok, "storreenn")) {
                return "storre_enn";
            }
            if (((nl_streq(tok, "er_større_enn") || nl_streq(tok, "er_storre_enn")) || nl_streq(tok, "erstørreenn")) || nl_streq(tok, "erstorreenn")) {
                return "storre_enn";
            }
            if ((nl_streq(tok, "større_eller_lik") || nl_streq(tok, "størreellerlik")) || nl_streq(tok, "storreellerlik")) {
                return "storre_eller_lik";
            }
            if (((((((nl_streq(tok, "større_lik") || nl_streq(tok, "storre_lik")) || nl_streq(tok, "størrelik")) || nl_streq(tok, "storrelik")) || nl_streq(tok, "er_større_lik")) || nl_streq(tok, "er_storre_lik")) || nl_streq(tok, "erstørrelik")) || nl_streq(tok, "erstorrelik")) {
                return "storre_eller_lik";
            }
            if (((nl_streq(tok, "er_større_eller_lik") || nl_streq(tok, "er_storre_eller_lik")) || nl_streq(tok, "erstørreellerlik")) || nl_streq(tok, "erstorreellerlik")) {
                return "storre_eller_lik";
            }
            if (((nl_streq(tok, "større_enn_eller_lik") || nl_streq(tok, "storre_enn_eller_lik")) || nl_streq(tok, "størreennellerlik")) || nl_streq(tok, "storreennellerlik")) {
                return "storre_eller_lik";
            }
            if (((nl_streq(tok, "er_større_enn_eller_lik") || nl_streq(tok, "er_storre_enn_eller_lik")) || nl_streq(tok, "erstørreennellerlik")) || nl_streq(tok, "erstorreennellerlik")) {
                return "storre_eller_lik";
            }
            if (nl_streq(tok, "er_mindre_enn") || nl_streq(tok, "ermindreenn")) {
                return "mindre_enn";
            }
            if (nl_streq(tok, "mindre_enn") || nl_streq(tok, "mindreenn")) {
                return "mindre_enn";
            }
            if (((nl_streq(tok, "er_mindre_eller_lik") || nl_streq(tok, "er_mindre_enn_eller_lik")) || nl_streq(tok, "ermindreellerlik")) || nl_streq(tok, "ermindreennellerlik")) {
                return "mindre_eller_lik";
            }
            if (nl_streq(tok, "mindre_enn_eller_lik") || nl_streq(tok, "mindreennellerlik")) {
                return "mindre_eller_lik";
            }
            if ((((nl_streq(tok, "mindre_lik") || nl_streq(tok, "mindrelik")) || nl_streq(tok, "mindreellerlik")) || nl_streq(tok, "er_mindre_lik")) || nl_streq(tok, "ermindrelik")) {
                return "mindre_eller_lik";
            }
            if ((((((((((((((((((((nl_streq(tok, "ikke_lik") || nl_streq(tok, "ikke_lik_med")) || nl_streq(tok, "ikkelikmed")) || nl_streq(tok, "er_ikke")) || nl_streq(tok, "erikke")) || nl_streq(tok, "er_ulik")) || nl_streq(tok, "er_ulik_med")) || nl_streq(tok, "erulik")) || nl_streq(tok, "erulikmed")) || nl_streq(tok, "erikkelik")) || nl_streq(tok, "erikkelikmed")) || nl_streq(tok, "ulik_med")) || nl_streq(tok, "ulikmed")) || nl_streq(tok, "ikkje_lik")) || nl_streq(tok, "ikkje_lik_med")) || nl_streq(tok, "ikkjelikmed")) || nl_streq(tok, "ikkje_er")) || nl_streq(tok, "er_ikkje")) || nl_streq(tok, "erikkje")) || nl_streq(tok, "erikkjelik")) || nl_streq(tok, "erikkjelikmed")) {
                return "ikke_er";
            }
            if (nl_streq(tok, "ikkje")) {
                return "ikke";
            }
            if (nl_streq(tok, "sant")) {
                return "sann";
            }
            if (nl_streq(tok, "on")) {
                return "sann";
            }
            if (nl_streq(tok, "truthy")) {
                return "sann";
            }
            if (nl_streq(tok, "true")) {
                return "sann";
            }
            if (nl_streq(tok, "ja")) {
                return "sann";
            }
            if (nl_streq(tok, "yes")) {
                return "sann";
            }
            if (nl_streq(tok, "affirmative")) {
                return "sann";
            }
            if (nl_streq(tok, "affirm")) {
                return "sann";
            }
            if (nl_streq(tok, "approved")) {
                return "sann";
            }
            if (nl_streq(tok, "accepted")) {
                return "sann";
            }
            if (nl_streq(tok, "confirmed")) {
                return "sann";
            }
            if (nl_streq(tok, "pass")) {
                return "sann";
            }
            if (nl_streq(tok, "success")) {
                return "sann";
            }
            if (nl_streq(tok, "allow")) {
                return "sann";
            }
            if (nl_streq(tok, "permit")) {
                return "sann";
            }
            if (nl_streq(tok, "valid")) {
                return "sann";
            }
            if (nl_streq(tok, "ok")) {
                return "sann";
            }
            if (nl_streq(tok, "ready")) {
                return "sann";
            }
            if (nl_streq(tok, "safe")) {
                return "sann";
            }
            if (nl_streq(tok, "secure")) {
                return "sann";
            }
            if (nl_streq(tok, "trusted")) {
                return "sann";
            }
            if (nl_streq(tok, "open")) {
                return "sann";
            }
            if (nl_streq(tok, "public")) {
                return "sann";
            }
            if (nl_streq(tok, "visible")) {
                return "sann";
            }
            if (nl_streq(tok, "present")) {
                return "sann";
            }
            if (nl_streq(tok, "online")) {
                return "sann";
            }
            if (nl_streq(tok, "connected")) {
                return "sann";
            }
            if (nl_streq(tok, "available")) {
                return "sann";
            }
            if (nl_streq(tok, "reachable")) {
                return "sann";
            }
            if (nl_streq(tok, "working")) {
                return "sann";
            }
            if (nl_streq(tok, "stable")) {
                return "sann";
            }
            if (nl_streq(tok, "correct")) {
                return "sann";
            }
            if (nl_streq(tok, "complete")) {
                return "sann";
            }
            if (nl_streq(tok, "clean")) {
                return "sann";
            }
            if (nl_streq(tok, "up")) {
                return "sann";
            }
            if (nl_streq(tok, "alive")) {
                return "sann";
            }
            if (nl_streq(tok, "awake")) {
                return "sann";
            }
            if (nl_streq(tok, "healthy")) {
                return "sann";
            }
            if (nl_streq(tok, "synced")) {
                return "sann";
            }
            if (nl_streq(tok, "enabled")) {
                return "sann";
            }
            if (nl_streq(tok, "active")) {
                return "sann";
            }
            if (nl_streq(tok, "usant")) {
                return "usann";
            }
            if (nl_streq(tok, "off")) {
                return "usann";
            }
            if (nl_streq(tok, "falsy")) {
                return "usann";
            }
            if (nl_streq(tok, "false")) {
                return "usann";
            }
            if (nl_streq(tok, "nei")) {
                return "usann";
            }
            if (nl_streq(tok, "no")) {
                return "usann";
            }
            if (nl_streq(tok, "negative")) {
                return "usann";
            }
            if (nl_streq(tok, "deny")) {
                return "usann";
            }
            if (nl_streq(tok, "rejected")) {
                return "usann";
            }
            if (nl_streq(tok, "declined")) {
                return "usann";
            }
            if (nl_streq(tok, "denied")) {
                return "usann";
            }
            if (nl_streq(tok, "fail")) {
                return "usann";
            }
            if (nl_streq(tok, "failure")) {
                return "usann";
            }
            if (nl_streq(tok, "block")) {
                return "usann";
            }
            if (nl_streq(tok, "forbid")) {
                return "usann";
            }
            if (nl_streq(tok, "invalid")) {
                return "usann";
            }
            if (nl_streq(tok, "not_ok") || nl_streq(tok, "notok")) {
                return "usann";
            }
            if (nl_streq(tok, "not_ready") || nl_streq(tok, "notready")) {
                return "usann";
            }
            if (nl_streq(tok, "unsafe")) {
                return "usann";
            }
            if (nl_streq(tok, "insecure")) {
                return "usann";
            }
            if (nl_streq(tok, "untrusted")) {
                return "usann";
            }
            if (nl_streq(tok, "closed")) {
                return "usann";
            }
            if (nl_streq(tok, "private")) {
                return "usann";
            }
            if (nl_streq(tok, "hidden")) {
                return "usann";
            }
            if (nl_streq(tok, "absent")) {
                return "usann";
            }
            if (nl_streq(tok, "offline")) {
                return "usann";
            }
            if (nl_streq(tok, "disconnected")) {
                return "usann";
            }
            if (nl_streq(tok, "unavailable")) {
                return "usann";
            }
            if (nl_streq(tok, "unreachable")) {
                return "usann";
            }
            if (nl_streq(tok, "broken")) {
                return "usann";
            }
            if (nl_streq(tok, "unstable")) {
                return "usann";
            }
            if (nl_streq(tok, "incorrect")) {
                return "usann";
            }
            if (nl_streq(tok, "incomplete")) {
                return "usann";
            }
            if (nl_streq(tok, "dirty")) {
                return "usann";
            }
            if (nl_streq(tok, "down")) {
                return "usann";
            }
            if (nl_streq(tok, "dead")) {
                return "usann";
            }
            if (nl_streq(tok, "asleep")) {
                return "usann";
            }
            if (nl_streq(tok, "unhealthy")) {
                return "usann";
            }
            if (nl_streq(tok, "unsynced")) {
                return "usann";
            }
            if (nl_streq(tok, "disabled")) {
                return "usann";
            }
            if (nl_streq(tok, "inactive")) {
                return "usann";
            }
            if (((nl_streq(tok, "er_lik") || nl_streq(tok, "er_lik_med")) || nl_streq(tok, "erlik")) || nl_streq(tok, "erlikmed")) {
                return "er";
            }
            if (nl_streq(tok, "lik_med") || nl_streq(tok, "likmed")) {
                return "er";
            }
            if (nl_streq(tok, "{")) {
                return "(";
            }
            if (nl_streq(tok, "}")) {
                return ")";
            }
            if (nl_streq(tok, "[")) {
                return "(";
            }
            if (nl_streq(tok, "]")) {
                return ")";
            }
            return tok;
            return "";
        }
        
        int selfhost__compiler__er_operator_token(char * tok) {
            if ((((nl_streq(tok, "+") || nl_streq(tok, "-")) || nl_streq(tok, "*")) || nl_streq(tok, "/")) || nl_streq(tok, "%")) {
                return 1;
            }
            if (((((nl_streq(tok, "==") || nl_streq(tok, "!=")) || nl_streq(tok, "<")) || nl_streq(tok, ">")) || nl_streq(tok, "<=")) || nl_streq(tok, ">=")) {
                return 1;
            }
            if (((((nl_streq(tok, "er") || nl_streq(tok, "ikke_er")) || nl_streq(tok, "mindre_enn")) || nl_streq(tok, "storre_enn")) || nl_streq(tok, "mindre_eller_lik")) || nl_streq(tok, "storre_eller_lik")) {
                return 1;
            }
            if ((((((((((((((nl_streq(tok, "&&") || nl_streq(tok, "||")) || nl_streq(tok, "!")) || nl_streq(tok, "og")) || nl_streq(tok, "samt")) || nl_streq(tok, "eller")) || nl_streq(tok, "enten")) || nl_streq(tok, "ikke")) || nl_streq(tok, "xor")) || nl_streq(tok, "^^")) || nl_streq(tok, "xnor")) || nl_streq(tok, "nand")) || nl_streq(tok, "nor")) || nl_streq(tok, "impliserer")) || nl_streq(tok, "impliseres_av")) {
                return 1;
            }
            return 0;
            return 0;
        }
        
        int selfhost__compiler__operator_precedens(char * tok) {
            if ((nl_streq(tok, "u!") || nl_streq(tok, "u-")) || nl_streq(tok, "uikke")) {
                return 7;
            }
            if ((nl_streq(tok, "*") || nl_streq(tok, "/")) || nl_streq(tok, "%")) {
                return 6;
            }
            if (nl_streq(tok, "+") || nl_streq(tok, "-")) {
                return 5;
            }
            if (((((((nl_streq(tok, "<") || nl_streq(tok, ">")) || nl_streq(tok, "<=")) || nl_streq(tok, ">=")) || nl_streq(tok, "mindre_enn")) || nl_streq(tok, "storre_enn")) || nl_streq(tok, "mindre_eller_lik")) || nl_streq(tok, "storre_eller_lik")) {
                return 4;
            }
            if (((nl_streq(tok, "==") || nl_streq(tok, "!=")) || nl_streq(tok, "er")) || nl_streq(tok, "ikke_er")) {
                return 3;
            }
            if (((nl_streq(tok, "&&") || nl_streq(tok, "og")) || nl_streq(tok, "samt")) || nl_streq(tok, "nand")) {
                return 2;
            }
            if ((((((((nl_streq(tok, "||") || nl_streq(tok, "eller")) || nl_streq(tok, "enten")) || nl_streq(tok, "xor")) || nl_streq(tok, "^^")) || nl_streq(tok, "xnor")) || nl_streq(tok, "nor")) || nl_streq(tok, "impliserer")) || nl_streq(tok, "impliseres_av")) {
                return 1;
            }
            return 0;
            return 0;
        }
        
        char * selfhost__compiler__operator_til_opcode(char * tok) {
            if (nl_streq(tok, "+")) {
                return "ADD";
            }
            if (nl_streq(tok, "-")) {
                return "SUB";
            }
            if (nl_streq(tok, "*")) {
                return "MUL";
            }
            if (nl_streq(tok, "/")) {
                return "DIV";
            }
            if (nl_streq(tok, "%")) {
                return "MOD";
            }
            if (nl_streq(tok, "==") || nl_streq(tok, "er")) {
                return "EQ";
            }
            if (nl_streq(tok, "<") || nl_streq(tok, "mindre_enn")) {
                return "LT";
            }
            if (nl_streq(tok, ">") || nl_streq(tok, "storre_enn")) {
                return "GT";
            }
            if ((nl_streq(tok, "&&") || nl_streq(tok, "og")) || nl_streq(tok, "samt")) {
                return "AND";
            }
            if ((nl_streq(tok, "||") || nl_streq(tok, "eller")) || nl_streq(tok, "enten")) {
                return "OR";
            }
            return "";
            return "";
        }
        
        int selfhost__compiler__emitter_operator(nl_list_text* ops, nl_list_int* verdier, char * op) {
            if (nl_streq(op, "u!") || nl_streq(op, "uikke")) {
                nl_list_text_push(ops, "NOT");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, "u-")) {
                nl_list_text_push(ops, "PUSH");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "SWAP");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "SUB");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, "!=") || nl_streq(op, "ikke_er")) {
                nl_list_text_push(ops, "EQ");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "NOT");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, "xor") || nl_streq(op, "^^")) {
                nl_list_text_push(ops, "EQ");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "NOT");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, "xnor")) {
                nl_list_text_push(ops, "EQ");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, "nand")) {
                nl_list_text_push(ops, "AND");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "NOT");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, "nor")) {
                nl_list_text_push(ops, "OR");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "NOT");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, "impliserer")) {
                nl_list_text_push(ops, "SWAP");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "NOT");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "SWAP");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "OR");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, "impliseres_av")) {
                nl_list_text_push(ops, "NOT");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "OR");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, "<=") || nl_streq(op, "mindre_eller_lik")) {
                nl_list_text_push(ops, "GT");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "NOT");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            if (nl_streq(op, ">=") || nl_streq(op, "storre_eller_lik")) {
                nl_list_text_push(ops, "LT");
                nl_list_int_push(verdier, 0);
                nl_list_text_push(ops, "NOT");
                nl_list_int_push(verdier, 0);
                return 1;
            }
            char * opcode = selfhost__compiler__operator_til_opcode(op);
            if (nl_streq(opcode, "")) {
                return 0;
            }
            nl_list_text_push(ops, opcode);
            nl_list_int_push(verdier, 0);
            return 1;
            return 0;
        }
        
        int selfhost__compiler__stack_behov(char * op) {
            if ((((nl_streq(op, "ADD") || nl_streq(op, "SUB")) || nl_streq(op, "MUL")) || nl_streq(op, "DIV")) || nl_streq(op, "MOD")) {
                return 2;
            }
            if ((((nl_streq(op, "EQ") || nl_streq(op, "GT")) || nl_streq(op, "LT")) || nl_streq(op, "AND")) || nl_streq(op, "OR")) {
                return 2;
            }
            if (((((nl_streq(op, "NOT") || nl_streq(op, "POP")) || nl_streq(op, "DUP")) || nl_streq(op, "STORE")) || nl_streq(op, "PRINT")) || nl_streq(op, "JZ")) {
                return 1;
            }
            if (nl_streq(op, "OVER") || nl_streq(op, "SWAP")) {
                return 2;
            }
            return 0;
            return 0;
        }
        
        int selfhost__compiler__stack_endring(char * op) {
            if (((nl_streq(op, "PUSH") || nl_streq(op, "LOAD")) || nl_streq(op, "DUP")) || nl_streq(op, "OVER")) {
                return 1;
            }
            if ((((nl_streq(op, "ADD") || nl_streq(op, "SUB")) || nl_streq(op, "MUL")) || nl_streq(op, "DIV")) || nl_streq(op, "MOD")) {
                return (-(1));
            }
            if ((((nl_streq(op, "EQ") || nl_streq(op, "GT")) || nl_streq(op, "LT")) || nl_streq(op, "AND")) || nl_streq(op, "OR")) {
                return (-(1));
            }
            if (nl_streq(op, "POP") || nl_streq(op, "STORE")) {
                return (-(1));
            }
            return 0;
            return 0;
        }
        
        char * selfhost__compiler__sjekk_program(nl_list_text* ops, nl_list_int* verdier) {
            nl_list_int* labels = nl_list_int_new();
            nl_list_int_push(labels, 0);
            int i = 0;
            int n = nl_list_text_len(ops);
            int dybde = 0;
            if (n == 0) {
                return "/* feil: tomt program */";
            }
            nl_list_int_remove(labels, 0);
            while (nl_list_int_len(labels) < n) {
                nl_list_int_push(labels, 0);
            }
            while (i < n) {
                if (nl_streq(ops->data[i], "LABEL")) {
                    nl_list_int_set(labels, i, 1);
                }
                i = (i + 1);
            }
            i = 0;
            while (i < n) {
                char * op = ops->data[i];
                if ((nl_streq(op, "JMP") || nl_streq(op, "JZ")) || nl_streq(op, "CALL")) {
                    int target = verdier->data[i];
                    if ((target < 0) || (target >= n)) {
                        return nl_concat(nl_concat("/* feil: ugyldig hopp-target ", nl_int_to_text(target)), " */");
                    }
                    if (labels->data[target] != 1) {
                        return nl_concat(nl_concat("/* feil: hopp-target mangler LABEL ved indeks ", nl_int_to_text(target)), " */");
                    }
                }
                if (nl_streq(op, "STORE") || nl_streq(op, "LOAD")) {
                    int idx = verdier->data[i];
                    if ((idx < 0) || (idx >= 256)) {
                        return nl_concat(nl_concat("/* feil: minneindeks utenfor range ", nl_int_to_text(idx)), " */");
                    }
                }
                i = (i + 1);
            }
            i = 0;
            while (i < n) {
                char * op2 = ops->data[i];
                int behov = selfhost__compiler__stack_behov(op2);
                if (dybde < behov) {
                    return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: stack-underflow ved indeks ", nl_int_to_text(i)), " ("), op2), ") */");
                }
                dybde = (dybde + selfhost__compiler__stack_endring(op2));
                if (dybde < 0) {
                    return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: stack-underflow ved indeks ", nl_int_to_text(i)), " ("), op2), ") */");
                }
                i = (i + 1);
            }
            return "";
            return "";
        }
        
        char * selfhost__compiler__instruksjon_til_c(char * op, int verdi) {
            if (nl_streq(op, "PUSH")) {
                return nl_concat(nl_concat("stack[sp++] = ", nl_int_to_text(verdi)), ";");
            }
            else if (nl_streq(op, "LABEL")) {
                return nl_concat(nl_concat("L", nl_int_to_text(verdi)), ":;");
            }
            else if (nl_streq(op, "JMP")) {
                return nl_concat(nl_concat("goto L", nl_int_to_text(verdi)), ";");
            }
            else if (nl_streq(op, "JZ")) {
                return nl_concat(nl_concat("if (stack[sp-1] == 0) goto L", nl_int_to_text(verdi)), ";");
            }
            else if (nl_streq(op, "CALL")) {
                return nl_concat(nl_concat("ret_stack[rsp++] = 0; goto L", nl_int_to_text(verdi)), ";");
            }
            else if (nl_streq(op, "RET")) {
                return "goto *labels[0];";
            }
            else if (nl_streq(op, "STORE")) {
                return nl_concat(nl_concat("mem[", nl_int_to_text(verdi)), "] = stack[sp-1]; sp = sp - 1;");
            }
            else if (nl_streq(op, "LOAD")) {
                return nl_concat(nl_concat("stack[sp++] = mem[", nl_int_to_text(verdi)), "];");
            }
            else if (nl_streq(op, "ADD")) {
                return "stack[sp-2] = stack[sp-2] + stack[sp-1]; sp = sp - 1;";
            }
            else if (nl_streq(op, "SUB")) {
                return "stack[sp-2] = stack[sp-2] - stack[sp-1]; sp = sp - 1;";
            }
            else if (nl_streq(op, "MUL")) {
                return "stack[sp-2] = stack[sp-2] * stack[sp-1]; sp = sp - 1;";
            }
            else if (nl_streq(op, "DIV")) {
                return "stack[sp-2] = stack[sp-2] / stack[sp-1]; sp = sp - 1;";
            }
            else if (nl_streq(op, "MOD")) {
                return "stack[sp-2] = stack[sp-2] % stack[sp-1]; sp = sp - 1;";
            }
            else if (nl_streq(op, "EQ")) {
                return "stack[sp-2] = (stack[sp-2] == stack[sp-1]); sp = sp - 1;";
            }
            else if (nl_streq(op, "GT")) {
                return "stack[sp-2] = (stack[sp-2] > stack[sp-1]); sp = sp - 1;";
            }
            else if (nl_streq(op, "LT")) {
                return "stack[sp-2] = (stack[sp-2] < stack[sp-1]); sp = sp - 1;";
            }
            else if (nl_streq(op, "AND")) {
                return "stack[sp-2] = (stack[sp-2] && stack[sp-1]); sp = sp - 1;";
            }
            else if (nl_streq(op, "OR")) {
                return "stack[sp-2] = (stack[sp-2] || stack[sp-1]); sp = sp - 1;";
            }
            else if (nl_streq(op, "NOT")) {
                return "stack[sp-1] = !stack[sp-1];";
            }
            else if (nl_streq(op, "DUP")) {
                return "stack[sp] = stack[sp-1]; sp = sp + 1;";
            }
            else if (nl_streq(op, "POP")) {
                return "sp = sp - 1;";
            }
            else if (nl_streq(op, "SWAP")) {
                return "tmp = stack[sp-1]; stack[sp-1] = stack[sp-2]; stack[sp-2] = tmp;";
            }
            else if (nl_streq(op, "OVER")) {
                return "stack[sp] = stack[sp-2]; sp = sp + 1;";
            }
            else if (nl_streq(op, "PRINT")) {
                return "printf(\"%d\\n\", stack[sp-1]);";
            }
            else if (nl_streq(op, "HALT")) {
                return "return 0;";
            }
            return nl_concat(nl_concat("/* ukjent instruksjon: ", op), " */");
            return "";
        }
        
        char * selfhost__compiler__instruksjon_til_c_med_retur(char * op, int verdi, int retur) {
            if (nl_streq(op, "CALL")) {
                return nl_concat(nl_concat(nl_concat(nl_concat("if (rsp >= 256) { printf(\"ret-stack overflow\\n\"); return 1; } ret_stack[rsp++] = ", nl_int_to_text(retur)), "; goto L"), nl_int_to_text(verdi)), ";");
            }
            if (nl_streq(op, "RET")) {
                return "if (rsp <= 0) { printf(\"ret-stack underflow\\n\"); return 1; } goto *labels[ret_stack[--rsp]];";
            }
            if (nl_streq(op, "LOAD")) {
                return nl_concat(nl_concat(nl_concat(nl_concat(nl_concat("if (sp >= 256) { printf(\"stack overflow\\n\"); return 1; } if (", nl_int_to_text(verdi)), " < 0 || "), nl_int_to_text(verdi)), " >= 256) { printf(\"mem index out of range\\n\"); return 1; } "), selfhost__compiler__instruksjon_til_c(op, verdi));
            }
            if (nl_streq(op, "STORE")) {
                return nl_concat(nl_concat(nl_concat(nl_concat(nl_concat("if (sp < 1) { printf(\"stack underflow\\n\"); return 1; } if (", nl_int_to_text(verdi)), " < 0 || "), nl_int_to_text(verdi)), " >= 256) { printf(\"mem index out of range\\n\"); return 1; } "), selfhost__compiler__instruksjon_til_c(op, verdi));
            }
            if ((nl_streq(op, "PUSH") || nl_streq(op, "DUP")) || nl_streq(op, "OVER")) {
                return nl_concat("if (sp >= 256) { printf(\"stack overflow\\n\"); return 1; } ", selfhost__compiler__instruksjon_til_c(op, verdi));
            }
            if ((((nl_streq(op, "ADD") || nl_streq(op, "SUB")) || nl_streq(op, "MUL")) || nl_streq(op, "DIV")) || nl_streq(op, "MOD")) {
                return nl_concat("if (sp < 2) { printf(\"stack underflow\\n\"); return 1; } ", selfhost__compiler__instruksjon_til_c(op, verdi));
            }
            if ((((nl_streq(op, "EQ") || nl_streq(op, "GT")) || nl_streq(op, "LT")) || nl_streq(op, "AND")) || nl_streq(op, "OR")) {
                return nl_concat("if (sp < 2) { printf(\"stack underflow\\n\"); return 1; } ", selfhost__compiler__instruksjon_til_c(op, verdi));
            }
            if (((nl_streq(op, "NOT") || nl_streq(op, "POP")) || nl_streq(op, "PRINT")) || nl_streq(op, "JZ")) {
                return nl_concat("if (sp < 1) { printf(\"stack underflow\\n\"); return 1; } ", selfhost__compiler__instruksjon_til_c(op, verdi));
            }
            if (nl_streq(op, "SWAP")) {
                return nl_concat("if (sp < 2) { printf(\"stack underflow\\n\"); return 1; } ", selfhost__compiler__instruksjon_til_c(op, verdi));
            }
            return selfhost__compiler__instruksjon_til_c(op, verdi);
            return "";
        }
        
        char * selfhost__compiler__kompiler_til_c(nl_list_text* ops, nl_list_int* verdier) {
            char * c = "";
            int i = 0;
            int n = nl_list_text_len(ops);
            char * valideringsfeil = "";
            if (nl_list_text_len(ops) != nl_list_int_len(verdier)) {
                return "/* feil: ops og verdier må ha samme lengde */";
            }
            valideringsfeil = selfhost__compiler__sjekk_program(ops, verdier);
            if (!(nl_streq(valideringsfeil, ""))) {
                return valideringsfeil;
            }
            c = selfhost__compiler__append_linje(c, "#include <stdio.h>");
            c = selfhost__compiler__append_linje(c, "");
            c = selfhost__compiler__append_linje(c, "int main(void) {");
            c = selfhost__compiler__append_linje(c, "    int stack[256];");
            c = selfhost__compiler__append_linje(c, "    int mem[256] = {0};");
            c = selfhost__compiler__append_linje(c, "    int ret_stack[256];");
            c = selfhost__compiler__append_linje(c, "    int sp = 0;");
            c = selfhost__compiler__append_linje(c, "    int rsp = 0;");
            c = selfhost__compiler__append_linje(c, "    int tmp = 0;");
            c = selfhost__compiler__append_linje(c, "    void *labels[512] = {0};");
            while (i < n) {
                c = selfhost__compiler__append_linje(c, nl_concat(nl_concat(nl_concat(nl_concat(nl_concat(nl_concat("L", nl_int_to_text(i)), ":; labels["), nl_int_to_text(i)), "] = &&L"), nl_int_to_text(i)), ";"));
                c = selfhost__compiler__append_linje(c, nl_concat("    ", selfhost__compiler__instruksjon_til_c_med_retur(ops->data[i], verdier->data[i], (i + 1))));
                i = (i + 1);
            }
            c = selfhost__compiler__append_linje(c, nl_concat(nl_concat(nl_concat(nl_concat("labels[", nl_int_to_text(n)), "] = &&L"), nl_int_to_text(n)), ";"));
            c = selfhost__compiler__append_linje(c, nl_concat(nl_concat("L", nl_int_to_text(n)), ":;"));
            c = selfhost__compiler__append_linje(c, "    return 0;");
            c = selfhost__compiler__append_linje(c, "}");
            return c;
            return "";
        }
        
        char * selfhost__compiler__demo_program() {
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "PUSH");
            nl_list_text_push(ops, "PUSH");
            nl_list_text_push(ops, "ADD");
            nl_list_text_push(ops, "PRINT");
            nl_list_text_push(ops, "HALT");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 10);
            nl_list_int_push(verdier, 32);
            nl_list_int_push(verdier, 0);
            nl_list_int_push(verdier, 0);
            nl_list_int_push(verdier, 0);
            return selfhost__compiler__kompiler_til_c(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__token_pos(int pos) {
            return nl_concat("token ", nl_int_to_text(pos));
            return "";
        }
        
        int selfhost__compiler__finn_navn_indeks(nl_list_text* navn, char * key) {
            int i = 0;
            while (i < nl_list_text_len(navn)) {
                if (nl_streq(navn->data[i], key)) {
                    return i;
                }
                i = (i + 1);
            }
            return (-(1));
            return 0;
        }
        
        int selfhost__compiler__er_navn_token(char * tok) {
            if ((((nl_streq(tok, "") || nl_streq(tok, "(")) || nl_streq(tok, ")")) || nl_streq(tok, "=")) || nl_streq(tok, ";")) {
                return 0;
            }
            if (nl_streq(tok, "sann") || nl_streq(tok, "usann")) {
                return 0;
            }
            if (selfhost__compiler__er_heltall_token(tok)) {
                return 0;
            }
            if (selfhost__compiler__er_operator_token(tok)) {
                return 0;
            }
            return 1;
            return 0;
        }
        
        int selfhost__compiler__er_assignment_op(char * tok) {
            if (((((nl_streq(tok, "=") || nl_streq(tok, "+=")) || nl_streq(tok, "-=")) || nl_streq(tok, "*=")) || nl_streq(tok, "/=")) || nl_streq(tok, "%=")) {
                return 1;
            }
            return 0;
            return 0;
        }
        
        char * selfhost__compiler__assignment_op_til_binop(char * tok) {
            if (nl_streq(tok, "+=")) {
                return "+";
            }
            if (nl_streq(tok, "-=")) {
                return "-";
            }
            if (nl_streq(tok, "*=")) {
                return "*";
            }
            if (nl_streq(tok, "/=")) {
                return "/";
            }
            if (nl_streq(tok, "%=")) {
                return "%";
            }
            return "";
            return "";
        }
        
        char * selfhost__compiler__eval_ops_til_verdi(nl_list_text* ops, nl_list_int* verdier, nl_list_int* resultat) {
            nl_list_int* stack = nl_list_int_new();
            nl_list_int_push(stack, 0);
            int i = 0;
            nl_list_int_remove(stack, 0);
            while (i < nl_list_text_len(ops)) {
                char * op = ops->data[i];
                int v = verdier->data[i];
                if (nl_streq(op, "LABEL")) {
                    i = (i + 1);
                    continue;
                }
                if (nl_streq(op, "JMP") || nl_streq(op, "JZ")) {
                    int target_label = v;
                    int j = 0;
                    int target_pc = (-(1));
                    if (nl_streq(op, "JZ")) {
                        if (nl_list_int_len(stack) < 1) {
                            return "/* feil: eval stack-underflow ved JZ */";
                        }
                        if (stack->data[(nl_list_int_len(stack) - 1)] != 0) {
                            i = (i + 1);
                            continue;
                        }
                    }
                    while (j < nl_list_text_len(ops)) {
                        if (nl_streq(ops->data[j], "LABEL") && (verdier->data[j] == target_label)) {
                            target_pc = j;
                            break;
                        }
                        j = (j + 1);
                    }
                    if (target_pc < 0) {
                        return nl_concat(nl_concat("/* feil: eval fant ikke label ", nl_int_to_text(target_label)), " */");
                    }
                    i = target_pc;
                    continue;
                }
                if (nl_streq(op, "PUSH")) {
                    nl_list_int_push(stack, v);
                    i = (i + 1);
                    continue;
                }
                if (nl_streq(op, "NOT")) {
                    if (nl_list_int_len(stack) < 1) {
                        return "/* feil: eval stack-underflow ved NOT */";
                    }
                    int x = nl_list_int_pop(stack);
                    if (x == 0) {
                        nl_list_int_push(stack, 1);
                    }
                    else {
                        nl_list_int_push(stack, 0);
                    }
                    i = (i + 1);
                    continue;
                }
                if (((((((((nl_streq(op, "ADD") || nl_streq(op, "SUB")) || nl_streq(op, "MUL")) || nl_streq(op, "DIV")) || nl_streq(op, "MOD")) || nl_streq(op, "EQ")) || nl_streq(op, "LT")) || nl_streq(op, "GT")) || nl_streq(op, "AND")) || nl_streq(op, "OR")) {
                    if (nl_list_int_len(stack) < 2) {
                        return nl_concat(nl_concat("/* feil: eval stack-underflow ved ", op), " */");
                    }
                    int b = nl_list_int_pop(stack);
                    int a = nl_list_int_pop(stack);
                    if (nl_streq(op, "ADD")) {
                        nl_list_int_push(stack, (a + b));
                    }
                    else if (nl_streq(op, "SUB")) {
                        nl_list_int_push(stack, (a - b));
                    }
                    else if (nl_streq(op, "MUL")) {
                        nl_list_int_push(stack, (a * b));
                    }
                    else if (nl_streq(op, "DIV")) {
                        nl_list_int_push(stack, (a / b));
                    }
                    else if (nl_streq(op, "MOD")) {
                        if (b == 0) {
                            return "/* feil: eval deling/modulo på 0 */";
                        }
                        int divisor = b;
                        if (divisor < 0) {
                            divisor = (0 - divisor);
                        }
                        int rest = a;
                        if (rest >= 0) {
                            while (rest >= divisor) {
                                rest = (rest - divisor);
                            }
                        }
                        else {
                            while (rest < 0) {
                                rest = (rest + divisor);
                            }
                        }
                        nl_list_int_push(stack, rest);
                    }
                    else if (nl_streq(op, "EQ")) {
                        if (a == b) {
                            nl_list_int_push(stack, 1);
                        }
                        else {
                            nl_list_int_push(stack, 0);
                        }
                    }
                    else if (nl_streq(op, "LT")) {
                        if (a < b) {
                            nl_list_int_push(stack, 1);
                        }
                        else {
                            nl_list_int_push(stack, 0);
                        }
                    }
                    else if (nl_streq(op, "GT")) {
                        if (a > b) {
                            nl_list_int_push(stack, 1);
                        }
                        else {
                            nl_list_int_push(stack, 0);
                        }
                    }
                    else if (nl_streq(op, "AND")) {
                        if ((a != 0) && (b != 0)) {
                            nl_list_int_push(stack, 1);
                        }
                        else {
                            nl_list_int_push(stack, 0);
                        }
                    }
                    else if (nl_streq(op, "OR")) {
                        if ((a != 0) || (b != 0)) {
                            nl_list_int_push(stack, 1);
                        }
                        else {
                            nl_list_int_push(stack, 0);
                        }
                    }
                    i = (i + 1);
                    continue;
                }
                if (nl_streq(op, "PRINT")) {
                    if (nl_list_int_len(stack) < 1) {
                        return "/* feil: eval stack-underflow ved PRINT */";
                    }
                    i = (i + 1);
                    continue;
                }
                if (nl_streq(op, "HALT")) {
                    break;
                }
                return nl_concat(nl_concat("/* feil: eval støtter ikke op ", op), " */");
            }
            if (nl_list_int_len(stack) < 1) {
                return "/* feil: eval forventet minst én verdi, fikk 0 */";
            }
            if (nl_list_int_len(resultat) == 0) {
                nl_list_int_push(resultat, stack->data[(nl_list_int_len(stack) - 1)]);
            }
            else {
                nl_list_int_set(resultat, 0, stack->data[(nl_list_int_len(stack) - 1)]);
            }
            return "";
            return "";
        }
        
        char * selfhost__compiler__append_ops(nl_list_text* dst_ops, nl_list_int* dst_verdier, nl_list_text* src_ops, nl_list_int* src_verdier) {
            if (nl_list_text_len(src_ops) != nl_list_int_len(src_verdier)) {
                return "/* feil: append_ops krever like lengder */";
            }
            int i = 0;
            while (i < nl_list_text_len(src_ops)) {
                nl_list_text_push(dst_ops, src_ops->data[i]);
                nl_list_int_push(dst_verdier, src_verdier->data[i]);
                i = (i + 1);
            }
            return "";
            return "";
        }
        
        char * selfhost__compiler__append_ops_med_offset(nl_list_text* dst_ops, nl_list_int* dst_verdier, nl_list_text* src_ops, nl_list_int* src_verdier, int offset) {
            if (nl_list_text_len(src_ops) != nl_list_int_len(src_verdier)) {
                return "/* feil: append_ops_med_offset krever like lengder */";
            }
            int i = 0;
            while (i < nl_list_text_len(src_ops)) {
                char * op = src_ops->data[i];
                int v = src_verdier->data[i];
                if ((nl_streq(op, "JMP") || nl_streq(op, "JZ")) || nl_streq(op, "LABEL")) {
                    v = (v + offset);
                }
                nl_list_text_push(dst_ops, op);
                nl_list_int_push(dst_verdier, v);
                i = (i + 1);
            }
            return "";
            return "";
        }
        
        char * selfhost__compiler__parse_tokens_til_ops(nl_list_text* tokens, nl_list_text* ops, nl_list_int* verdier, int strict) {
            int i = 0;
            nl_list_text_remove(ops, 0);
            nl_list_int_remove(verdier, 0);
            while (i < nl_list_text_len(tokens)) {
                char * op = tokens->data[i];
                if (strict && (selfhost__compiler__op_kjent(op) == 0)) {
                    return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: ukjent opcode ", op), " ved "), selfhost__compiler__token_pos(i)), " */");
                }
                if ((((((nl_streq(op, "PUSH") || nl_streq(op, "LABEL")) || nl_streq(op, "JMP")) || nl_streq(op, "JZ")) || nl_streq(op, "STORE")) || nl_streq(op, "LOAD")) || nl_streq(op, "CALL")) {
                    if ((i + 1) >= nl_list_text_len(tokens)) {
                        return nl_concat(nl_concat("/* feil: op mangler verdi ved ", selfhost__compiler__token_pos(i)), " */");
                    }
                    if (strict && (selfhost__compiler__er_heltall_token(tokens->data[(i + 1)]) == 0)) {
                        return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: ugyldig heltallsargument ", tokens->data[(i + 1)]), " ved "), selfhost__compiler__token_pos((i + 1))), " */");
                    }
                    nl_list_text_push(ops, op);
                    nl_list_int_push(verdier, nl_text_to_int(tokens->data[(i + 1)]));
                    i = (i + 2);
                }
                else {
                    nl_list_text_push(ops, op);
                    nl_list_int_push(verdier, 0);
                    i = (i + 1);
                }
            }
            return "";
            return "";
        }
        
        char * selfhost__compiler__kompiler_fra_tokens(nl_list_text* tokens) {
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "HALT");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = selfhost__compiler__parse_tokens_til_ops(tokens, ops, verdier, 0);
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            return selfhost__compiler__kompiler_til_c(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__disasm_fra_tokens(nl_list_text* tokens) {
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "HALT");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = selfhost__compiler__parse_tokens_til_ops(tokens, ops, verdier, 0);
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            return selfhost__compiler__disasm_program(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__disasm_fra_tokens_strict(nl_list_text* tokens) {
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "HALT");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = selfhost__compiler__parse_tokens_til_ops(tokens, ops, verdier, 1);
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            return selfhost__compiler__disasm_program(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__kompiler_fra_tokens_strict(nl_list_text* tokens) {
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "HALT");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = selfhost__compiler__parse_tokens_til_ops(tokens, ops, verdier, 1);
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            return selfhost__compiler__kompiler_til_c(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__disasm_fra_linjer(nl_list_text* linjer) {
            nl_list_text* tokens = nl_list_text_new();
            nl_list_text_push(tokens, "");
            for (int __nl_i_1 = 0; __nl_i_1 < nl_list_text_len(linjer); __nl_i_1++) {
                char * linje = linjer->data[__nl_i_1];
                if (nl_streq(linje, "")) {
                    continue;
                }
                nl_list_text_push(tokens, linje);
            }
            nl_list_text_remove(tokens, 0);
            return selfhost__compiler__disasm_fra_tokens(tokens);
            return "";
        }
        
        nl_list_text* selfhost__compiler__linjer_til_tokens(nl_list_text* linjer) {
            nl_list_text* tokens = nl_list_text_new();
            nl_list_text_push(tokens, "");
            for (int __nl_i_2 = 0; __nl_i_2 < nl_list_text_len(linjer); __nl_i_2++) {
                char * linje = linjer->data[__nl_i_2];
                if (nl_streq(linje, "")) {
                    continue;
                }
                nl_list_text_push(tokens, linje);
            }
            nl_list_text_remove(tokens, 0);
            return tokens;
            return nl_list_text_new();
        }
        
        char * selfhost__compiler__kompiler_fra_linjer(nl_list_text* linjer) {
            nl_list_text* tokens = selfhost__compiler__linjer_til_tokens(linjer);
            return selfhost__compiler__kompiler_fra_tokens(tokens);
            return "";
        }
        
        char * selfhost__compiler__kompiler_fra_kilde(char * kilde) {
            nl_list_text* tokens = nl_tokenize_simple(kilde);
            return selfhost__compiler__kompiler_fra_tokens(tokens);
            return "";
        }
        
        char * selfhost__compiler__disasm_fra_kilde(char * kilde) {
            nl_list_text* tokens = nl_tokenize_simple(kilde);
            return selfhost__compiler__disasm_fra_tokens(tokens);
            return "";
        }
        
        char * selfhost__compiler__disasm_fra_kilde_strict(char * kilde) {
            nl_list_text* tokens = nl_tokenize_simple(kilde);
            return selfhost__compiler__disasm_fra_tokens_strict(tokens);
            return "";
        }
        
        char * selfhost__compiler__kompiler_fra_kilde_strict(char * kilde) {
            nl_list_text* tokens = nl_tokenize_simple(kilde);
            return selfhost__compiler__kompiler_fra_tokens_strict(tokens);
            return "";
        }
        
        char * selfhost__compiler__uttrykk_til_ops_og_verdier_med_miljo(nl_list_text* tokens, nl_list_text* navn, nl_list_int* miljo_verdier, nl_list_text* ops, nl_list_int* verdier) {
            nl_list_text* operatorer = nl_list_text_new();
            nl_list_text_push(operatorer, "");
            nl_list_int* operator_pos = nl_list_int_new();
            nl_list_int_push(operator_pos, 0);
            int i = 0;
            int siste_token = (-(1));
            int forventer_verdi = 1;
            nl_list_text_remove(operatorer, 0);
            nl_list_int_remove(operator_pos, 0);
            if (nl_list_text_len(navn) != nl_list_int_len(miljo_verdier)) {
                return "/* feil: navn/miljø-verdier må ha samme lengde */";
            }
            while (i < nl_list_text_len(tokens)) {
                char * tok_kilde = tokens->data[i];
                char * tok_raw = tok_kilde;
                tok_raw = selfhost__compiler__normaliser_norsk_token(tok_raw);
                char * tok = tok_raw;
                int tok_step = 1;
                char * n1 = "";
                char * n2 = "";
                char * n3 = "";
                char * n4 = "";
                char * n5 = "";
                if ((i + 1) < nl_list_text_len(tokens)) {
                    n1 = selfhost__compiler__normaliser_norsk_token(tokens->data[(i + 1)]);
                }
                if ((i + 2) < nl_list_text_len(tokens)) {
                    n2 = selfhost__compiler__normaliser_norsk_token(tokens->data[(i + 2)]);
                }
                if ((i + 3) < nl_list_text_len(tokens)) {
                    n3 = selfhost__compiler__normaliser_norsk_token(tokens->data[(i + 3)]);
                }
                if ((i + 4) < nl_list_text_len(tokens)) {
                    n4 = selfhost__compiler__normaliser_norsk_token(tokens->data[(i + 4)]);
                }
                if ((i + 5) < nl_list_text_len(tokens)) {
                    n5 = selfhost__compiler__normaliser_norsk_token(tokens->data[(i + 5)]);
                }
                if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "<")) && nl_streq(n1, "-")) && nl_streq(n2, ">")) {
                    tok = "xnor";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "<")) && nl_streq(n1, "=")) && nl_streq(n2, ">")) {
                    tok = "xnor";
                    tok_step = 3;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "<=")) && nl_streq(n1, ">")) {
                    tok = "xnor";
                    tok_step = 2;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "<")) && nl_streq(n1, "-")) {
                    tok = "impliseres_av";
                    tok_step = 2;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "<")) && nl_streq(n1, ">")) {
                    tok = "ikke_er";
                    tok_step = 2;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "-")) && nl_streq(n1, ">")) {
                    tok = "impliserer";
                    tok_step = 2;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "=")) && nl_streq(n1, ">")) {
                    tok = "impliserer";
                    tok_step = 2;
                }
                else if (nl_streq(tok_raw, "->") || nl_streq(tok_raw, "=>")) {
                    tok = "impliserer";
                    tok_step = 1;
                }
                else if (nl_streq(tok_raw, "<-")) {
                    tok = "impliseres_av";
                    tok_step = 1;
                }
                else if (nl_streq(tok_raw, "<->") || nl_streq(tok_raw, "<=>")) {
                    tok = "xnor";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && ((((nl_streq(tok_raw, "divided") || (nl_streq(tok_raw, "/") && nl_streq(tok_kilde, "divide"))) || (nl_streq(tok_raw, "*") && (nl_streq(tok_kilde, "multiply") || nl_streq(tok_kilde, "multiplied")))) || nl_streq(tok_raw, "modulo")) || (nl_streq(tok_raw, "%") && nl_streq(tok_kilde, "remainder")))) && (nl_streq(n1, "by") || nl_streq(n1, "of"))) {
                    if (nl_streq(tok_raw, "divided") || (nl_streq(tok_raw, "/") && nl_streq(tok_kilde, "divide"))) {
                        tok = "/";
                    }
                    else if (nl_streq(tok_raw, "*") && (nl_streq(tok_kilde, "multiply") || nl_streq(tok_kilde, "multiplied"))) {
                        tok = "*";
                    }
                    else {
                        tok = "%";
                    }
                    tok_step = 2;
                }
                else if ((((((((((((((((((((((((((((((((((((((((((((nl_streq(tok_raw, "delt_pa") || nl_streq(tok_raw, "deltpa")) || nl_streq(tok_raw, "deltpaa")) || nl_streq(tok_raw, "deler_pa")) || nl_streq(tok_raw, "delerpa")) || nl_streq(tok_raw, "delerpaa")) || nl_streq(tok_raw, "dele_pa")) || nl_streq(tok_raw, "delepa")) || nl_streq(tok_raw, "delepaa")) || nl_streq(tok_raw, "deles_pa")) || nl_streq(tok_raw, "delespa")) || nl_streq(tok_raw, "delespaa")) || nl_streq(tok_raw, "del_pa")) || nl_streq(tok_raw, "delpa")) || nl_streq(tok_raw, "delpaa")) || nl_streq(tok_raw, "dele_seg_pa")) || nl_streq(tok_raw, "delesegpa")) || nl_streq(tok_raw, "delesegpaa")) || nl_streq(tok_raw, "deler_seg_pa")) || nl_streq(tok_raw, "delersegpa")) || nl_streq(tok_raw, "delersegpaa")) || nl_streq(tok_raw, "divider_seg_pa")) || nl_streq(tok_raw, "dividersegpa")) || nl_streq(tok_raw, "dividersegpaa")) || nl_streq(tok_raw, "dividere_seg_pa")) || nl_streq(tok_raw, "divideresegpa")) || nl_streq(tok_raw, "divideresegpaa")) || nl_streq(tok_raw, "dividerer_seg_pa")) || nl_streq(tok_raw, "dividerersegpa")) || nl_streq(tok_raw, "dividerersegpaa")) || nl_streq(tok_raw, "divider_pa")) || nl_streq(tok_raw, "dividerpa")) || nl_streq(tok_raw, "dividerpaa")) || nl_streq(tok_raw, "dividere_pa")) || nl_streq(tok_raw, "dividerepa")) || nl_streq(tok_raw, "dividerepaa")) || nl_streq(tok_raw, "dividerer_pa")) || nl_streq(tok_raw, "dividererpa")) || nl_streq(tok_raw, "dividererpaa")) || nl_streq(tok_raw, "dividert_pa")) || nl_streq(tok_raw, "dividertpa")) || nl_streq(tok_raw, "dividertpaa")) || nl_streq(tok_raw, "divideres_pa")) || nl_streq(tok_raw, "dividerespa")) || nl_streq(tok_raw, "dividerespaa")) {
                    tok = "/";
                    tok_step = 1;
                }
                else if ((((((((((((((((((((((((((((nl_streq(tok_raw, "delt_med") || nl_streq(tok_raw, "deltmed")) || nl_streq(tok_raw, "dele_med")) || nl_streq(tok_raw, "delemed")) || nl_streq(tok_raw, "deler_med")) || nl_streq(tok_raw, "deles_med")) || nl_streq(tok_raw, "delesmed")) || nl_streq(tok_raw, "dele_seg_med")) || nl_streq(tok_raw, "delesegmed")) || nl_streq(tok_raw, "deler_seg_med")) || nl_streq(tok_raw, "delersegmed")) || nl_streq(tok_raw, "divider_seg_med")) || nl_streq(tok_raw, "dividersegmed")) || nl_streq(tok_raw, "dividere_seg_med")) || nl_streq(tok_raw, "divideresegmed")) || nl_streq(tok_raw, "dividerer_seg_med")) || nl_streq(tok_raw, "dividerersegmed")) || nl_streq(tok_raw, "dividere_med")) || nl_streq(tok_raw, "divideremed")) || nl_streq(tok_raw, "divider_med")) || nl_streq(tok_raw, "dividermed")) || nl_streq(tok_raw, "dividerer_med")) || nl_streq(tok_raw, "dividerermed")) || nl_streq(tok_raw, "dividert_med")) || nl_streq(tok_raw, "dividertmed")) || nl_streq(tok_raw, "divideres_med")) || nl_streq(tok_raw, "divideresmed")) || nl_streq(tok_raw, "del_med")) || nl_streq(tok_raw, "delmed")) {
                    tok = "/";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && (((((((((nl_streq(tok_raw, "delt") || nl_streq(tok_raw, "deler")) || nl_streq(tok_raw, "dele")) || nl_streq(tok_raw, "deles")) || nl_streq(tok_raw, "del")) || nl_streq(tok_raw, "divider")) || nl_streq(tok_raw, "dividere")) || nl_streq(tok_raw, "dividerer")) || nl_streq(tok_raw, "dividert")) || nl_streq(tok_raw, "divideres"))) && (nl_streq(n1, "pa") || nl_streq(n1, "paa"))) {
                    tok = "/";
                    tok_step = 2;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && ((((nl_streq(tok_raw, "dele") || nl_streq(tok_raw, "deler")) || nl_streq(tok_raw, "divider")) || nl_streq(tok_raw, "dividere")) || nl_streq(tok_raw, "dividerer"))) && nl_streq(n1, "seg")) && ((nl_streq(n2, "med") || nl_streq(n2, "pa")) || nl_streq(n2, "paa"))) {
                    tok = "/";
                    tok_step = 3;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && (((((((((nl_streq(tok_raw, "delt") || nl_streq(tok_raw, "dele")) || nl_streq(tok_raw, "deler")) || nl_streq(tok_raw, "deles")) || nl_streq(tok_raw, "dividere")) || nl_streq(tok_raw, "divider")) || nl_streq(tok_raw, "dividerer")) || nl_streq(tok_raw, "dividert")) || nl_streq(tok_raw, "divideres")) || nl_streq(tok_raw, "del"))) && nl_streq(n1, "med")) {
                    tok = "/";
                    tok_step = 2;
                }
                else if (((((((((nl_streq(tok_raw, "delt") || nl_streq(tok_raw, "dele")) || nl_streq(tok_raw, "deler")) || nl_streq(tok_raw, "deles")) || nl_streq(tok_raw, "dividere")) || nl_streq(tok_raw, "divider")) || nl_streq(tok_raw, "dividerer")) || nl_streq(tok_raw, "dividert")) || nl_streq(tok_raw, "divideres")) || nl_streq(tok_raw, "del")) {
                    tok = "/";
                    tok_step = 1;
                }
                else if (((((((((((((((((((nl_streq(tok_raw, "gang_med") || nl_streq(tok_raw, "gangmed")) || nl_streq(tok_raw, "ganget_med")) || nl_streq(tok_raw, "gangetmed")) || nl_streq(tok_raw, "gange_med")) || nl_streq(tok_raw, "gangemed")) || nl_streq(tok_raw, "ganger_med")) || nl_streq(tok_raw, "gangermed")) || nl_streq(tok_raw, "ganges_med")) || nl_streq(tok_raw, "gangesmed")) || nl_streq(tok_raw, "multipliser_med")) || nl_streq(tok_raw, "multiplisermed")) || nl_streq(tok_raw, "multiplisere_med")) || nl_streq(tok_raw, "multipliseremed")) || nl_streq(tok_raw, "multipliserer_med")) || nl_streq(tok_raw, "multipliserermed")) || nl_streq(tok_raw, "multiplisert_med")) || nl_streq(tok_raw, "multiplisertmed")) || nl_streq(tok_raw, "multipliseres_med")) || nl_streq(tok_raw, "multipliseresmed")) {
                    tok = "*";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && (((((((((nl_streq(tok_raw, "gang") || nl_streq(tok_raw, "ganget")) || nl_streq(tok_raw, "gange")) || nl_streq(tok_raw, "ganger")) || nl_streq(tok_raw, "ganges")) || nl_streq(tok_raw, "multipliser")) || nl_streq(tok_raw, "multiplisere")) || nl_streq(tok_raw, "multipliserer")) || nl_streq(tok_raw, "multiplisert")) || nl_streq(tok_raw, "multipliseres"))) && nl_streq(n1, "med")) {
                    tok = "*";
                    tok_step = 2;
                }
                else if (((nl_streq(tok_raw, "pluss_med") || nl_streq(tok_raw, "plussmed")) || nl_streq(tok_raw, "plus_med")) || nl_streq(tok_raw, "plusmed")) {
                    tok = "+";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && (nl_streq(tok_raw, "pluss") || nl_streq(tok_raw, "plus"))) && nl_streq(n1, "med")) {
                    tok = "+";
                    tok_step = 2;
                }
                else if (nl_streq(tok_raw, "pluss") || nl_streq(tok_raw, "plus")) {
                    tok = "+";
                    tok_step = 1;
                }
                else if (((((((((((((((((((((nl_streq(tok_raw, "plusser_med") || nl_streq(tok_raw, "plussermed")) || nl_streq(tok_raw, "plusses_med")) || nl_streq(tok_raw, "plussesmed")) || nl_streq(tok_raw, "pluses_med")) || nl_streq(tok_raw, "plusesmed")) || nl_streq(tok_raw, "plusse_med")) || nl_streq(tok_raw, "plussemed")) || nl_streq(tok_raw, "adder_med")) || nl_streq(tok_raw, "addermed")) || nl_streq(tok_raw, "addere_med")) || nl_streq(tok_raw, "adderemed")) || nl_streq(tok_raw, "adderer_med")) || nl_streq(tok_raw, "adderermed")) || nl_streq(tok_raw, "adderes_med")) || nl_streq(tok_raw, "adderesmed")) || nl_streq(tok_raw, "summer_med")) || nl_streq(tok_raw, "summermed")) || nl_streq(tok_raw, "summerer_med")) || nl_streq(tok_raw, "summerermed")) || nl_streq(tok_raw, "summeres_med")) || nl_streq(tok_raw, "summeresmed")) {
                    tok = "+";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && ((((((((((nl_streq(tok_raw, "plusser") || nl_streq(tok_raw, "plusses")) || nl_streq(tok_raw, "pluses")) || nl_streq(tok_raw, "plusse")) || nl_streq(tok_raw, "adder")) || nl_streq(tok_raw, "addere")) || nl_streq(tok_raw, "adderer")) || nl_streq(tok_raw, "adderes")) || nl_streq(tok_raw, "summer")) || nl_streq(tok_raw, "summerer")) || nl_streq(tok_raw, "summeres"))) && nl_streq(n1, "med")) {
                    tok = "+";
                    tok_step = 2;
                }
                else if ((((((((((nl_streq(tok_raw, "plusser") || nl_streq(tok_raw, "plusses")) || nl_streq(tok_raw, "pluses")) || nl_streq(tok_raw, "plusse")) || nl_streq(tok_raw, "adder")) || nl_streq(tok_raw, "addere")) || nl_streq(tok_raw, "adderer")) || nl_streq(tok_raw, "adderes")) || nl_streq(tok_raw, "summer")) || nl_streq(tok_raw, "summerer")) || nl_streq(tok_raw, "summeres")) {
                    tok = "+";
                    tok_step = 1;
                }
                else if (((((nl_streq(tok_raw, "legg_til") || nl_streq(tok_raw, "legge_til")) || nl_streq(tok_raw, "legges_til")) || nl_streq(tok_raw, "leggtil")) || nl_streq(tok_raw, "leggetil")) || nl_streq(tok_raw, "leggestil")) {
                    tok = "+";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && ((nl_streq(tok_raw, "legg") || nl_streq(tok_raw, "legge")) || nl_streq(tok_raw, "legges"))) && nl_streq(n1, "til")) {
                    tok = "+";
                    tok_step = 2;
                }
                else if (((((nl_streq(tok_raw, "legg_sammen") || nl_streq(tok_raw, "legge_sammen")) || nl_streq(tok_raw, "legges_sammen")) || nl_streq(tok_raw, "leggsammen")) || nl_streq(tok_raw, "leggesammen")) || nl_streq(tok_raw, "leggessammen")) {
                    tok = "+";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && ((nl_streq(tok_raw, "legg") || nl_streq(tok_raw, "legge")) || nl_streq(tok_raw, "legges"))) && nl_streq(n1, "sammen")) {
                    tok = "+";
                    tok_step = 2;
                }
                else if ((nl_streq(tok_raw, "legg") || nl_streq(tok_raw, "legge")) || nl_streq(tok_raw, "legges")) {
                    tok = "+";
                    tok_step = 1;
                }
                else if (((((((((((((((((nl_streq(tok_raw, "trekk_fra") || nl_streq(tok_raw, "trekkfra")) || nl_streq(tok_raw, "trekke_fra")) || nl_streq(tok_raw, "trekkefra")) || nl_streq(tok_raw, "trekkes_fra")) || nl_streq(tok_raw, "trekkesfra")) || nl_streq(tok_raw, "minus_fra")) || nl_streq(tok_raw, "minusfra")) || nl_streq(tok_raw, "minuseres_fra")) || nl_streq(tok_raw, "minuseresfra")) || nl_streq(tok_raw, "subtraher_fra")) || nl_streq(tok_raw, "subtraherfra")) || nl_streq(tok_raw, "subtraherer_fra")) || nl_streq(tok_raw, "subtrahererfra")) || nl_streq(tok_raw, "subtraheres_fra")) || nl_streq(tok_raw, "subtraheresfra")) || nl_streq(tok_raw, "subtrahert_fra")) || nl_streq(tok_raw, "subtrahertfra")) {
                    tok = "-";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && ((((((((nl_streq(tok_raw, "trekk") || nl_streq(tok_raw, "trekke")) || nl_streq(tok_raw, "trekkes")) || nl_streq(tok_raw, "minus")) || nl_streq(tok_raw, "minuseres")) || nl_streq(tok_raw, "subtraher")) || nl_streq(tok_raw, "subtraherer")) || nl_streq(tok_raw, "subtraheres")) || nl_streq(tok_raw, "subtrahert"))) && nl_streq(n1, "fra")) {
                    tok = "-";
                    tok_step = 2;
                }
                else if (((((((((((((((((nl_streq(tok_raw, "minus_med") || nl_streq(tok_raw, "minusmed")) || nl_streq(tok_raw, "minuseres_med")) || nl_streq(tok_raw, "minuseresmed")) || nl_streq(tok_raw, "trekk_med")) || nl_streq(tok_raw, "trekkmed")) || nl_streq(tok_raw, "trekke_med")) || nl_streq(tok_raw, "trekkemed")) || nl_streq(tok_raw, "trekkes_med")) || nl_streq(tok_raw, "trekkesmed")) || nl_streq(tok_raw, "subtraher_med")) || nl_streq(tok_raw, "subtrahermed")) || nl_streq(tok_raw, "subtraherer_med")) || nl_streq(tok_raw, "subtraherermed")) || nl_streq(tok_raw, "subtraheres_med")) || nl_streq(tok_raw, "subtraheresmed")) || nl_streq(tok_raw, "subtrahert_med")) || nl_streq(tok_raw, "subtrahertmed")) {
                    tok = "-";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && ((((((((nl_streq(tok_raw, "minus") || nl_streq(tok_raw, "minuseres")) || nl_streq(tok_raw, "trekk")) || nl_streq(tok_raw, "trekke")) || nl_streq(tok_raw, "trekkes")) || nl_streq(tok_raw, "subtraher")) || nl_streq(tok_raw, "subtraherer")) || nl_streq(tok_raw, "subtraheres")) || nl_streq(tok_raw, "subtrahert"))) && nl_streq(n1, "med")) {
                    tok = "-";
                    tok_step = 2;
                }
                else if ((((((((nl_streq(tok_raw, "minus") || nl_streq(tok_raw, "minuseres")) || nl_streq(tok_raw, "trekk")) || nl_streq(tok_raw, "trekke")) || nl_streq(tok_raw, "trekkes")) || nl_streq(tok_raw, "subtraher")) || nl_streq(tok_raw, "subtraherer")) || nl_streq(tok_raw, "subtraheres")) || nl_streq(tok_raw, "subtrahert")) {
                    tok = "-";
                    tok_step = 1;
                }
                else if (nl_streq(tok_raw, "minus")) {
                    tok = "-";
                    tok_step = 1;
                }
                else if (((((((((nl_streq(tok_raw, "gang") || nl_streq(tok_raw, "gange")) || nl_streq(tok_raw, "ganget")) || nl_streq(tok_raw, "ganger")) || nl_streq(tok_raw, "ganges")) || nl_streq(tok_raw, "multipliser")) || nl_streq(tok_raw, "multiplisere")) || nl_streq(tok_raw, "multipliserer")) || nl_streq(tok_raw, "multiplisert")) || nl_streq(tok_raw, "multipliseres")) {
                    tok = "*";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && (((((nl_streq(tok_raw, "rest") || nl_streq(tok_raw, "resten")) || nl_streq(tok_raw, "mod")) || nl_streq(tok_raw, "modul")) || nl_streq(tok_raw, "modulus")) || nl_streq(tok_raw, "modulo"))) && nl_streq(n1, "av")) {
                    tok = "%";
                    tok_step = 2;
                }
                else if (((((((((((((((((nl_streq(tok_raw, "mod") || nl_streq(tok_raw, "mod_av")) || nl_streq(tok_raw, "modav")) || nl_streq(tok_raw, "modulo")) || nl_streq(tok_raw, "modulo_av")) || nl_streq(tok_raw, "moduloav")) || nl_streq(tok_raw, "modul")) || nl_streq(tok_raw, "modul_av")) || nl_streq(tok_raw, "modulav")) || nl_streq(tok_raw, "modulus")) || nl_streq(tok_raw, "modulus_av")) || nl_streq(tok_raw, "modulusav")) || nl_streq(tok_raw, "rest")) || nl_streq(tok_raw, "rest_av")) || nl_streq(tok_raw, "restav")) || nl_streq(tok_raw, "resten")) || nl_streq(tok_raw, "resten_av")) || nl_streq(tok_raw, "restenav")) {
                    tok = "%";
                    tok_step = 1;
                }
                else if ((((((((i + 5) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "less")) && nl_streq(n2, "than")) && (nl_streq(n3, "or") || nl_streq(n3, "eller"))) && ((nl_streq(n4, "equal") || nl_streq(n4, "equals")) || nl_streq(n4, "er"))) && nl_streq(n5, "to")) {
                    tok = "mindre_eller_lik";
                    tok_step = 6;
                }
                else if ((((((((i + 5) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "greater")) && nl_streq(n2, "than")) && (nl_streq(n3, "or") || nl_streq(n3, "eller"))) && ((nl_streq(n4, "equal") || nl_streq(n4, "equals")) || nl_streq(n4, "er"))) && nl_streq(n5, "to")) {
                    tok = "storre_eller_lik";
                    tok_step = 6;
                }
                else if (((((((i + 4) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "less")) && (nl_streq(n2, "or") || nl_streq(n2, "eller"))) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) && nl_streq(n4, "to")) {
                    tok = "mindre_eller_lik";
                    tok_step = 5;
                }
                else if (((((((i + 4) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "greater")) && (nl_streq(n2, "or") || nl_streq(n2, "eller"))) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) && nl_streq(n4, "to")) {
                    tok = "storre_eller_lik";
                    tok_step = 5;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "less")) && (nl_streq(n2, "or") || nl_streq(n2, "eller"))) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) {
                    tok = "mindre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "greater")) && (nl_streq(n2, "or") || nl_streq(n2, "eller"))) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) {
                    tok = "storre_eller_lik";
                    tok_step = 4;
                }
                else if (((((((i + 4) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "less")) && nl_streq(n2, "than")) && (nl_streq(n3, "or") || nl_streq(n3, "eller"))) && ((nl_streq(n4, "equal") || nl_streq(n4, "equals")) || nl_streq(n4, "er"))) {
                    tok = "mindre_eller_lik";
                    tok_step = 5;
                }
                else if (((((((i + 4) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "greater")) && nl_streq(n2, "than")) && (nl_streq(n3, "or") || nl_streq(n3, "eller"))) && ((nl_streq(n4, "equal") || nl_streq(n4, "equals")) || nl_streq(n4, "er"))) {
                    tok = "storre_eller_lik";
                    tok_step = 5;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "less")) && nl_streq(n2, "than")) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) {
                    tok = "mindre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "greater")) && nl_streq(n2, "than")) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) {
                    tok = "storre_eller_lik";
                    tok_step = 4;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "less")) && nl_streq(n2, "than")) {
                    tok = "mindre_enn";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "greater")) && nl_streq(n2, "than")) {
                    tok = "storre_enn";
                    tok_step = 3;
                }
                else if (((((((i + 4) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "less")) && nl_streq(n1, "than")) && (nl_streq(n2, "or") || nl_streq(n2, "eller"))) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) && nl_streq(n4, "to")) {
                    tok = "mindre_eller_lik";
                    tok_step = 5;
                }
                else if (((((((i + 4) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "greater")) && nl_streq(n1, "than")) && (nl_streq(n2, "or") || nl_streq(n2, "eller"))) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) && nl_streq(n4, "to")) {
                    tok = "storre_eller_lik";
                    tok_step = 5;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "less")) && nl_streq(n1, "than")) && (nl_streq(n2, "or") || nl_streq(n2, "eller"))) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) {
                    tok = "mindre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "greater")) && nl_streq(n1, "than")) && (nl_streq(n2, "or") || nl_streq(n2, "eller"))) && ((nl_streq(n3, "equal") || nl_streq(n3, "equals")) || nl_streq(n3, "er"))) {
                    tok = "storre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "less")) && nl_streq(n1, "than")) && ((nl_streq(n2, "equal") || nl_streq(n2, "equals")) || nl_streq(n2, "er"))) && nl_streq(n3, "to")) {
                    tok = "mindre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "greater")) && nl_streq(n1, "than")) && ((nl_streq(n2, "equal") || nl_streq(n2, "equals")) || nl_streq(n2, "er"))) && nl_streq(n3, "to")) {
                    tok = "storre_eller_lik";
                    tok_step = 4;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "less")) && nl_streq(n1, "than")) && ((nl_streq(n2, "equal") || nl_streq(n2, "equals")) || nl_streq(n2, "er"))) {
                    tok = "mindre_eller_lik";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "greater")) && nl_streq(n1, "than")) && ((nl_streq(n2, "equal") || nl_streq(n2, "equals")) || nl_streq(n2, "er"))) {
                    tok = "storre_eller_lik";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "less")) && nl_streq(n1, "or")) && ((nl_streq(n2, "equal") || nl_streq(n2, "equals")) || nl_streq(n2, "er"))) {
                    tok = "mindre_eller_lik";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "greater")) && nl_streq(n1, "or")) && ((nl_streq(n2, "equal") || nl_streq(n2, "equals")) || nl_streq(n2, "er"))) {
                    tok = "storre_eller_lik";
                    tok_step = 3;
                }
                else if (((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "less")) && nl_streq(n1, "than")) && (((i + 2) >= nl_list_text_len(tokens)) || (((!(nl_streq(n2, "or")) && !(nl_streq(n2, "equal"))) && !(nl_streq(n2, "equals"))) && !(nl_streq(n2, "er"))))) {
                    tok = "mindre_enn";
                    tok_step = 2;
                }
                else if (((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "greater")) && nl_streq(n1, "than")) && (((i + 2) >= nl_list_text_len(tokens)) || (((!(nl_streq(n2, "or")) && !(nl_streq(n2, "equal"))) && !(nl_streq(n2, "equals"))) && !(nl_streq(n2, "er"))))) {
                    tok = "storre_enn";
                    tok_step = 2;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "mindre")) && nl_streq(n1, "enn")) && nl_streq(n2, "eller")) && nl_streq(n3, "lik")) {
                    tok = "mindre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "storre")) && nl_streq(n1, "enn")) && nl_streq(n2, "eller")) && nl_streq(n3, "lik")) {
                    tok = "storre_eller_lik";
                    tok_step = 4;
                }
                else if (((((((i + 4) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "mindre")) && nl_streq(n2, "enn")) && nl_streq(n3, "eller")) && nl_streq(n4, "lik")) {
                    tok = "mindre_eller_lik";
                    tok_step = 5;
                }
                else if (((((((i + 4) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "storre")) && nl_streq(n2, "enn")) && nl_streq(n3, "eller")) && nl_streq(n4, "lik")) {
                    tok = "storre_eller_lik";
                    tok_step = 5;
                }
                else if (((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "mindre")) && nl_streq(n1, "enn")) && (((i + 2) >= nl_list_text_len(tokens)) || !(nl_streq(n2, "lik")))) {
                    tok = "mindre_enn";
                    tok_step = 2;
                }
                else if (((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "storre")) && nl_streq(n1, "enn")) && (((i + 2) >= nl_list_text_len(tokens)) || !(nl_streq(n2, "lik")))) {
                    tok = "storre_enn";
                    tok_step = 2;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "mindre")) && nl_streq(n1, "eller")) && nl_streq(n2, "lik")) {
                    tok = "mindre_eller_lik";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "storre")) && nl_streq(n1, "eller")) && nl_streq(n2, "lik")) {
                    tok = "storre_eller_lik";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "mindre")) && nl_streq(n1, "enn")) && nl_streq(n2, "lik")) {
                    tok = "mindre_eller_lik";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "storre")) && nl_streq(n1, "enn")) && nl_streq(n2, "lik")) {
                    tok = "storre_eller_lik";
                    tok_step = 3;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "mindre")) && nl_streq(n1, "lik")) {
                    tok = "mindre_eller_lik";
                    tok_step = 2;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "storre")) && nl_streq(n1, "lik")) {
                    tok = "storre_eller_lik";
                    tok_step = 2;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "mindre")) && nl_streq(n2, "lik")) {
                    tok = "mindre_eller_lik";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "storre")) && nl_streq(n2, "lik")) {
                    tok = "storre_eller_lik";
                    tok_step = 3;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "mindre")) && nl_streq(n2, "enn")) && nl_streq(n3, "lik")) {
                    tok = "mindre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "storre")) && nl_streq(n2, "enn")) && nl_streq(n3, "lik")) {
                    tok = "storre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "ikke")) && nl_streq(n2, "lik")) && (((i + 3) >= nl_list_text_len(tokens)) || !(nl_streq(n3, "med")))) {
                    tok = "ikke_er";
                    tok_step = 3;
                }
                else if ((((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "mindre")) && nl_streq(n2, "enn")) && (((i + 3) >= nl_list_text_len(tokens)) || !(nl_streq(n3, "lik")))) {
                    tok = "mindre_enn";
                    tok_step = 3;
                }
                else if ((((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "storre")) && nl_streq(n2, "enn")) && (((i + 3) >= nl_list_text_len(tokens)) || !(nl_streq(n3, "lik")))) {
                    tok = "storre_enn";
                    tok_step = 3;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "mindre")) && nl_streq(n2, "eller")) && nl_streq(n3, "lik")) {
                    tok = "mindre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "storre")) && nl_streq(n2, "eller")) && nl_streq(n3, "lik")) {
                    tok = "storre_eller_lik";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "ikke")) && nl_streq(n2, "lik")) && nl_streq(n3, "med")) {
                    tok = "ikke_er";
                    tok_step = 4;
                }
                else if ((((((i + 3) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "ikke")) && nl_streq(n2, "er")) && nl_streq(n3, "to")) {
                    tok = "ikke_er";
                    tok_step = 4;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "er")) && nl_streq(n2, "to")) {
                    tok = "er";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "ikke")) && ((nl_streq(n1, "er") || nl_streq(n1, "equal")) || nl_streq(n1, "equals"))) && nl_streq(n2, "to")) {
                    tok = "ikke_er";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "not")) && ((nl_streq(n1, "er") || nl_streq(n1, "equal")) || nl_streq(n1, "equals"))) && nl_streq(n2, "to")) {
                    tok = "ikke_er";
                    tok_step = 3;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && ((nl_streq(tok_raw, "er") || nl_streq(tok_raw, "equal")) || nl_streq(tok_raw, "equals"))) && nl_streq(n1, "to")) {
                    tok = "er";
                    tok_step = 2;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "ikke")) && nl_streq(n1, "lik")) && nl_streq(n2, "med")) {
                    tok = "ikke_er";
                    tok_step = 3;
                }
                else if (((((i + 2) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "lik")) && nl_streq(n2, "med")) {
                    tok = "er";
                    tok_step = 3;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "ulik")) {
                    tok = "ikke_er";
                    tok_step = 2;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "lik")) {
                    tok = "er";
                    tok_step = 2;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "ulik")) && nl_streq(n1, "med")) {
                    tok = "ikke_er";
                    tok_step = 2;
                }
                else if (nl_streq(tok_raw, "lik")) {
                    tok = "er";
                    tok_step = 1;
                }
                else if (nl_streq(tok_raw, "ulik")) {
                    tok = "ikke_er";
                    tok_step = 1;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "ikke")) && nl_streq(n1, "lik")) {
                    tok = "ikke_er";
                    tok_step = 2;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "er")) && nl_streq(n1, "ikke")) {
                    tok = "ikke_er";
                    tok_step = 2;
                }
                else if ((((i + 1) < nl_list_text_len(tokens)) && nl_streq(tok_raw, "ikke")) && nl_streq(n1, "er")) {
                    tok = "ikke_er";
                    tok_step = 2;
                }
                if (nl_streq(tok, "")) {
                    i = (i + 1);
                    continue;
                }
                if (selfhost__compiler__er_heltall_token(tok)) {
                    if (forventer_verdi == 0) {
                        return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: mangler operator før verdi ", tok), " ved "), selfhost__compiler__token_pos(i)), " */");
                    }
                    nl_list_text_push(ops, "PUSH");
                    nl_list_int_push(verdier, nl_text_to_int(tok));
                    forventer_verdi = 0;
                    siste_token = ((i + tok_step) - 1);
                    i = (i + tok_step);
                    continue;
                }
                if (nl_streq(tok, "sann") || nl_streq(tok, "usann")) {
                    if (forventer_verdi == 0) {
                        return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: mangler operator før verdi ", tok), " ved "), selfhost__compiler__token_pos(i)), " */");
                    }
                    nl_list_text_push(ops, "PUSH");
                    if (nl_streq(tok, "sann")) {
                        nl_list_int_push(verdier, 1);
                    }
                    else {
                        nl_list_int_push(verdier, 0);
                    }
                    forventer_verdi = 0;
                    siste_token = ((i + tok_step) - 1);
                    i = (i + tok_step);
                    continue;
                }
                if (forventer_verdi) {
                    int idx_navn = selfhost__compiler__finn_navn_indeks(navn, tok);
                    if (idx_navn >= 0) {
                        nl_list_text_push(ops, "PUSH");
                        nl_list_int_push(verdier, miljo_verdier->data[idx_navn]);
                        forventer_verdi = 0;
                        siste_token = ((i + tok_step) - 1);
                        i = (i + tok_step);
                        continue;
                    }
                }
                if (nl_streq(tok, "(")) {
                    if (forventer_verdi == 0) {
                        return nl_concat(nl_concat("/* feil: mangler operator før ( ved ", selfhost__compiler__token_pos(i)), " */");
                    }
                    nl_list_text_push(operatorer, tok);
                    nl_list_int_push(operator_pos, i);
                    siste_token = ((i + tok_step) - 1);
                    i = (i + tok_step);
                    continue;
                }
                if (nl_streq(tok, ")")) {
                    if (forventer_verdi) {
                        return nl_concat(nl_concat("/* feil: mangler verdi før ) ved ", selfhost__compiler__token_pos(i)), " */");
                    }
                    int fant_parantes = 0;
                    while (nl_list_text_len(operatorer) > 0) {
                        int top_idx = (nl_list_text_len(operatorer) - 1);
                        char * top = operatorer->data[top_idx];
                        int top_pos = operator_pos->data[top_idx];
                        nl_list_text_remove(operatorer, top_idx);
                        nl_list_int_remove(operator_pos, top_idx);
                        if (nl_streq(top, "(")) {
                            fant_parantes = 1;
                            break;
                        }
                        if (selfhost__compiler__emitter_operator(ops, verdier, top) == 0) {
                            return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: ukjent operator ", top), " ved "), selfhost__compiler__token_pos(top_pos)), " */");
                        }
                    }
                    if (fant_parantes == 0) {
                        return nl_concat(nl_concat("/* feil: ) uten matchende ( ved ", selfhost__compiler__token_pos(i)), " */");
                    }
                    forventer_verdi = 0;
                    siste_token = ((i + tok_step) - 1);
                    i = (i + tok_step);
                    continue;
                }
                if (selfhost__compiler__er_operator_token(tok)) {
                    if (forventer_verdi) {
                        if (nl_streq(tok, "!")) {
                            nl_list_text_push(operatorer, "u!");
                            nl_list_int_push(operator_pos, i);
                            siste_token = ((i + tok_step) - 1);
                            i = (i + tok_step);
                            continue;
                        }
                        if (nl_streq(tok, "ikke")) {
                            nl_list_text_push(operatorer, "uikke");
                            nl_list_int_push(operator_pos, i);
                            siste_token = ((i + tok_step) - 1);
                            i = (i + tok_step);
                            continue;
                        }
                        if (nl_streq(tok, "-")) {
                            nl_list_text_push(operatorer, "u-");
                            nl_list_int_push(operator_pos, i);
                            siste_token = ((i + tok_step) - 1);
                            i = (i + tok_step);
                            continue;
                        }
                        if (nl_streq(tok, "+")) {
                            siste_token = ((i + tok_step) - 1);
                            i = (i + tok_step);
                            continue;
                        }
                        return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: mangler verdi før operator ", tok), " ved "), selfhost__compiler__token_pos(i)), " */");
                    }
                    while (nl_list_text_len(operatorer) > 0) {
                        int top_idx2 = (nl_list_text_len(operatorer) - 1);
                        char * top2 = operatorer->data[top_idx2];
                        int top2_pos = operator_pos->data[top_idx2];
                        if (nl_streq(top2, "(")) {
                            break;
                        }
                        if (selfhost__compiler__operator_precedens(top2) < selfhost__compiler__operator_precedens(tok)) {
                            break;
                        }
                        nl_list_text_remove(operatorer, top_idx2);
                        nl_list_int_remove(operator_pos, top_idx2);
                        if (selfhost__compiler__emitter_operator(ops, verdier, top2) == 0) {
                            return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: ukjent operator ", top2), " ved "), selfhost__compiler__token_pos(top2_pos)), " */");
                        }
                    }
                    nl_list_text_push(operatorer, tok);
                    nl_list_int_push(operator_pos, i);
                    forventer_verdi = 1;
                    siste_token = ((i + tok_step) - 1);
                    i = (i + tok_step);
                    continue;
                }
                return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: ukjent token/navn i uttrykk ", tok), " ved "), selfhost__compiler__token_pos(i)), " */");
            }
            if (forventer_verdi) {
                if (siste_token >= 0) {
                    return nl_concat(nl_concat("/* feil: uttrykket avsluttes med operator ved ", selfhost__compiler__token_pos(siste_token)), " */");
                }
                return "/* feil: uttrykket avsluttes med operator */";
            }
            while (nl_list_text_len(operatorer) > 0) {
                int top_idx3 = (nl_list_text_len(operatorer) - 1);
                char * top3 = operatorer->data[top_idx3];
                int top3_pos = operator_pos->data[top_idx3];
                nl_list_text_remove(operatorer, top_idx3);
                nl_list_int_remove(operator_pos, top_idx3);
                if (nl_streq(top3, "(")) {
                    return nl_concat(nl_concat("/* feil: mangler ) i uttrykk ved ", selfhost__compiler__token_pos(top3_pos)), " */");
                }
                if (selfhost__compiler__emitter_operator(ops, verdier, top3) == 0) {
                    return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: ukjent operator ", top3), " ved "), selfhost__compiler__token_pos(top3_pos)), " */");
                }
            }
            return "";
            return "";
        }
        
        char * selfhost__compiler__uttrykk_til_ops_og_verdier(nl_list_text* tokens, nl_list_text* ops, nl_list_int* verdier) {
            nl_list_text* navn = nl_list_text_new();
            nl_list_text_push(navn, "");
            nl_list_int* miljo_verdier = nl_list_int_new();
            nl_list_int_push(miljo_verdier, 0);
            nl_list_text_remove(navn, 0);
            nl_list_int_remove(miljo_verdier, 0);
            return selfhost__compiler__uttrykk_til_ops_og_verdier_med_miljo(tokens, navn, miljo_verdier, ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__bygg_hvis_da_ellers_ops_med_miljo(nl_list_text* tokens, nl_list_text* navn, nl_list_int* miljo_verdier, nl_list_text* out_ops, nl_list_int* out_verdier) {
            if ((nl_list_text_len(tokens) == 0) || !(nl_streq(selfhost__compiler__normaliser_norsk_token(tokens->data[0]), "hvis"))) {
                return "/* feil: intern hvis-parser forventer 'hvis' ved token 0 */";
            }
            int depth = 0;
            nl_list_int* paren_pos = nl_list_int_new();
            nl_list_int_push(paren_pos, 0);
            int nested_if_etter_da = 0;
            int da_idx = (-(1));
            int ellers_idx = (-(1));
            int ellers_hvis_compact = 0;
            int i = 1;
            nl_list_int_remove(paren_pos, 0);
            while (i < nl_list_text_len(tokens)) {
                char * tok = tokens->data[i];
                char * tok_norm = selfhost__compiler__normaliser_norsk_token(tok);
                if (nl_streq(tok_norm, "(")) {
                    depth = (depth + 1);
                    nl_list_int_push(paren_pos, i);
                }
                else if (nl_streq(tok_norm, ")")) {
                    depth = (depth - 1);
                    if (depth < 0) {
                        return nl_concat(nl_concat("/* feil: ) uten matchende ( i hvis-uttrykk ved ", selfhost__compiler__token_pos(i)), " */");
                    }
                    nl_list_int_remove(paren_pos, (nl_list_int_len(paren_pos) - 1));
                }
                else if (((depth == 0) && nl_streq(tok_norm, "da")) && (da_idx < 0)) {
                    da_idx = i;
                }
                else if (((depth == 0) && (da_idx >= 0)) && nl_streq(tok_norm, "hvis")) {
                    nested_if_etter_da = (nested_if_etter_da + 1);
                }
                else if (((depth == 0) && (nl_streq(tok_norm, "ellers") || nl_streq(tok_norm, "ellers_hvis"))) && (da_idx >= 0)) {
                    if (nested_if_etter_da > 0) {
                        nested_if_etter_da = (nested_if_etter_da - 1);
                    }
                    else {
                        ellers_idx = i;
                        ellers_hvis_compact = nl_streq(tok_norm, "ellers_hvis");
                        break;
                    }
                }
                i = (i + 1);
            }
            if (depth != 0) {
                return nl_concat(nl_concat("/* feil: mangler ) i hvis-uttrykk ved ", selfhost__compiler__token_pos(paren_pos->data[(nl_list_int_len(paren_pos) - 1)])), " */");
            }
            if (da_idx < 0) {
                return nl_concat(nl_concat("/* feil: hvis-uttrykk mangler 'da' ved ", selfhost__compiler__token_pos(0)), " */");
            }
            if (ellers_idx < 0) {
                return nl_concat(nl_concat("/* feil: hvis-uttrykk mangler 'ellers' ved ", selfhost__compiler__token_pos(da_idx)), " */");
            }
            nl_list_text* cond_tokens = nl_list_text_new();
            nl_list_text_push(cond_tokens, "");
            nl_list_text* then_tokens = nl_list_text_new();
            nl_list_text_push(then_tokens, "");
            nl_list_text* else_tokens = nl_list_text_new();
            nl_list_text_push(else_tokens, "");
            nl_list_text_remove(cond_tokens, 0);
            nl_list_text_remove(then_tokens, 0);
            nl_list_text_remove(else_tokens, 0);
            i = 1;
            while (i < da_idx) {
                nl_list_text_push(cond_tokens, tokens->data[i]);
                i = (i + 1);
            }
            i = (da_idx + 1);
            while (i < ellers_idx) {
                nl_list_text_push(then_tokens, tokens->data[i]);
                i = (i + 1);
            }
            i = (ellers_idx + 1);
            if (ellers_hvis_compact) {
                nl_list_text_push(else_tokens, "hvis");
            }
            while (i < nl_list_text_len(tokens)) {
                nl_list_text_push(else_tokens, tokens->data[i]);
                i = (i + 1);
            }
            if (nl_list_text_len(cond_tokens) == 0) {
                return nl_concat(nl_concat("/* feil: hvis-uttrykk mangler betingelse ved ", selfhost__compiler__token_pos(0)), " */");
            }
            if (nl_list_text_len(then_tokens) == 0) {
                return nl_concat(nl_concat("/* feil: hvis-uttrykk mangler uttrykk etter 'da' ved ", selfhost__compiler__token_pos(da_idx)), " */");
            }
            if (nl_list_text_len(else_tokens) == 0) {
                return nl_concat(nl_concat("/* feil: hvis-uttrykk mangler uttrykk etter 'ellers' ved ", selfhost__compiler__token_pos(ellers_idx)), " */");
            }
            nl_list_text* cond_ops = nl_list_text_new();
            nl_list_text_push(cond_ops, "");
            nl_list_int* cond_verdier = nl_list_int_new();
            nl_list_int_push(cond_verdier, 0);
            nl_list_text* then_ops = nl_list_text_new();
            nl_list_text_push(then_ops, "");
            nl_list_int* then_verdier = nl_list_int_new();
            nl_list_int_push(then_verdier, 0);
            nl_list_text* else_ops = nl_list_text_new();
            nl_list_text_push(else_ops, "");
            nl_list_int* else_verdier = nl_list_int_new();
            nl_list_int_push(else_verdier, 0);
            nl_list_text_remove(cond_ops, 0);
            nl_list_int_remove(cond_verdier, 0);
            nl_list_text_remove(then_ops, 0);
            nl_list_int_remove(then_verdier, 0);
            nl_list_text_remove(else_ops, 0);
            nl_list_int_remove(else_verdier, 0);
            char * feil = selfhost__compiler__uttrykk_til_ops_og_verdier_med_miljo(cond_tokens, navn, miljo_verdier, cond_ops, cond_verdier);
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            if ((nl_list_text_len(then_tokens) > 0) && nl_streq(selfhost__compiler__normaliser_norsk_token(then_tokens->data[0]), "hvis")) {
                feil = selfhost__compiler__bygg_hvis_da_ellers_ops_med_miljo(then_tokens, navn, miljo_verdier, then_ops, then_verdier);
            }
            else {
                feil = selfhost__compiler__uttrykk_til_ops_og_verdier_med_miljo(then_tokens, navn, miljo_verdier, then_ops, then_verdier);
            }
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            if ((nl_list_text_len(else_tokens) > 0) && nl_streq(selfhost__compiler__normaliser_norsk_token(else_tokens->data[0]), "hvis")) {
                feil = selfhost__compiler__bygg_hvis_da_ellers_ops_med_miljo(else_tokens, navn, miljo_verdier, else_ops, else_verdier);
            }
            else {
                feil = selfhost__compiler__uttrykk_til_ops_og_verdier_med_miljo(else_tokens, navn, miljo_verdier, else_ops, else_verdier);
            }
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            nl_list_text_remove(out_ops, 0);
            nl_list_int_remove(out_verdier, 0);
            feil = selfhost__compiler__append_ops(out_ops, out_verdier, cond_ops, cond_verdier);
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            int ncond = nl_list_text_len(cond_ops);
            int nthen = nl_list_text_len(then_ops);
            int nelse = nl_list_text_len(else_ops);
            int l_else = ((ncond + 2) + nthen);
            int l_end = (((ncond + 3) + nthen) + nelse);
            nl_list_text_push(out_ops, "JZ");
            nl_list_int_push(out_verdier, l_else);
            feil = selfhost__compiler__append_ops_med_offset(out_ops, out_verdier, then_ops, then_verdier, (ncond + 1));
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            nl_list_text_push(out_ops, "JMP");
            nl_list_int_push(out_verdier, l_end);
            nl_list_text_push(out_ops, "LABEL");
            nl_list_int_push(out_verdier, l_else);
            feil = selfhost__compiler__append_ops_med_offset(out_ops, out_verdier, else_ops, else_verdier, ((ncond + 3) + nthen));
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            nl_list_text_push(out_ops, "LABEL");
            nl_list_int_push(out_verdier, l_end);
            return "";
            return "";
        }
        
        char * selfhost__compiler__disasm_uttrykk(char * kilde) {
            nl_list_text* tokens = nl_tokenize_expression(kilde);
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = "";
            nl_list_text_remove(ops, 0);
            nl_list_int_remove(verdier, 0);
            if ((nl_list_text_len(tokens) > 0) && nl_streq(selfhost__compiler__normaliser_norsk_token(tokens->data[0]), "hvis")) {
                nl_list_text* navn = nl_list_text_new();
                nl_list_text_push(navn, "");
                nl_list_int* miljo_verdier = nl_list_int_new();
                nl_list_int_push(miljo_verdier, 0);
                nl_list_text_remove(navn, 0);
                nl_list_int_remove(miljo_verdier, 0);
                feil = selfhost__compiler__bygg_hvis_da_ellers_ops_med_miljo(tokens, navn, miljo_verdier, ops, verdier);
            }
            else {
                feil = selfhost__compiler__uttrykk_til_ops_og_verdier(tokens, ops, verdier);
            }
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            nl_list_text_push(ops, "PRINT");
            nl_list_int_push(verdier, 0);
            nl_list_text_push(ops, "HALT");
            nl_list_int_push(verdier, 0);
            return selfhost__compiler__disasm_program(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__disasm_uttrykk_med_miljo(char * kilde, nl_list_text* navn, nl_list_int* miljo_verdier) {
            nl_list_text* tokens = nl_tokenize_expression(kilde);
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = "";
            nl_list_text_remove(ops, 0);
            nl_list_int_remove(verdier, 0);
            if ((nl_list_text_len(tokens) > 0) && nl_streq(selfhost__compiler__normaliser_norsk_token(tokens->data[0]), "hvis")) {
                feil = selfhost__compiler__bygg_hvis_da_ellers_ops_med_miljo(tokens, navn, miljo_verdier, ops, verdier);
            }
            else {
                feil = selfhost__compiler__uttrykk_til_ops_og_verdier_med_miljo(tokens, navn, miljo_verdier, ops, verdier);
            }
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            nl_list_text_push(ops, "PRINT");
            nl_list_int_push(verdier, 0);
            nl_list_text_push(ops, "HALT");
            nl_list_int_push(verdier, 0);
            return selfhost__compiler__disasm_program(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__kompiler_uttrykk_til_c(char * kilde) {
            nl_list_text* tokens = nl_tokenize_expression(kilde);
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = "";
            nl_list_text_remove(ops, 0);
            nl_list_int_remove(verdier, 0);
            if ((nl_list_text_len(tokens) > 0) && nl_streq(selfhost__compiler__normaliser_norsk_token(tokens->data[0]), "hvis")) {
                nl_list_text* navn = nl_list_text_new();
                nl_list_text_push(navn, "");
                nl_list_int* miljo_verdier = nl_list_int_new();
                nl_list_int_push(miljo_verdier, 0);
                nl_list_text_remove(navn, 0);
                nl_list_int_remove(miljo_verdier, 0);
                feil = selfhost__compiler__bygg_hvis_da_ellers_ops_med_miljo(tokens, navn, miljo_verdier, ops, verdier);
            }
            else {
                feil = selfhost__compiler__uttrykk_til_ops_og_verdier(tokens, ops, verdier);
            }
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            nl_list_text_push(ops, "PRINT");
            nl_list_int_push(verdier, 0);
            nl_list_text_push(ops, "HALT");
            nl_list_int_push(verdier, 0);
            return selfhost__compiler__kompiler_til_c(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__skript_til_ops_og_verdier(nl_list_text* tokens, nl_list_text* navn, nl_list_int* miljo_verdier, nl_list_text* out_ops, nl_list_int* out_verdier) {
            int i = 0;
            int fant_sluttuttrykk = 0;
            nl_list_text_remove(out_ops, 0);
            nl_list_int_remove(out_verdier, 0);
            while (i < nl_list_text_len(tokens)) {
                nl_list_text* stmt_tokens = nl_list_text_new();
                nl_list_text_push(stmt_tokens, "");
                int stmt_start = i;
                int stmt_has_semicolon = 0;
                nl_list_text_remove(stmt_tokens, 0);
                while ((((i < nl_list_text_len(tokens)) && !(nl_streq(tokens->data[i], ";"))) && !(nl_streq(tokens->data[i], ","))) && !(nl_streq(tokens->data[i], ":"))) {
                    if (!(nl_streq(tokens->data[i], ""))) {
                        nl_list_text_push(stmt_tokens, tokens->data[i]);
                    }
                    i = (i + 1);
                }
                if ((i < nl_list_text_len(tokens)) && ((nl_streq(tokens->data[i], ";") || nl_streq(tokens->data[i], ",")) || nl_streq(tokens->data[i], ":"))) {
                    stmt_has_semicolon = 1;
                    i = (i + 1);
                }
                if (nl_list_text_len(stmt_tokens) == 0) {
                    continue;
                }
                int ass_start = 0;
                int har_la = 0;
                int har_sett = 0;
                char * stmt_head = selfhost__compiler__normaliser_norsk_token(stmt_tokens->data[ass_start]);
                if (nl_streq(stmt_head, "la")) {
                    har_la = 1;
                    ass_start = (ass_start + 1);
                }
                else if (nl_streq(stmt_head, "sett")) {
                    har_sett = 1;
                    ass_start = (ass_start + 1);
                }
                if ((har_la || har_sett) && (ass_start >= nl_list_text_len(stmt_tokens))) {
                    if (har_la) {
                        return nl_concat(nl_concat("/* feil: 'la' må etterfølges av variabelnavn ved ", selfhost__compiler__token_pos(stmt_start)), " */");
                    }
                    return nl_concat(nl_concat("/* feil: 'sett' må etterfølges av variabelnavn ved ", selfhost__compiler__token_pos(stmt_start)), " */");
                }
                if ((((ass_start + 1) < nl_list_text_len(stmt_tokens)) && selfhost__compiler__er_navn_token(stmt_tokens->data[ass_start])) && selfhost__compiler__er_assignment_op(stmt_tokens->data[(ass_start + 1)])) {
                    char * varnavn = stmt_tokens->data[ass_start];
                    char * ass_op = stmt_tokens->data[(ass_start + 1)];
                    nl_list_text* expr_tokens = nl_list_text_new();
                    nl_list_text_push(expr_tokens, "");
                    nl_list_text* expr_ops = nl_list_text_new();
                    nl_list_text_push(expr_ops, "");
                    nl_list_int* expr_verdier = nl_list_int_new();
                    nl_list_int_push(expr_verdier, 0);
                    nl_list_int* eval_resultat = nl_list_int_new();
                    nl_list_int_push(eval_resultat, 0);
                    char * feil = "";
                    int j = (ass_start + 2);
                    nl_list_text_remove(expr_tokens, 0);
                    nl_list_text_remove(expr_ops, 0);
                    nl_list_int_remove(expr_verdier, 0);
                    nl_list_int_remove(eval_resultat, 0);
                    while (j < nl_list_text_len(stmt_tokens)) {
                        nl_list_text_push(expr_tokens, stmt_tokens->data[j]);
                        j = (j + 1);
                    }
                    if (nl_list_text_len(expr_tokens) == 0) {
                        return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: tomt uttrykk i assignment til ", varnavn), " ved "), selfhost__compiler__token_pos(stmt_start)), " */");
                    }
                    if (stmt_has_semicolon == 0) {
                        return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: mangler ';', ',' eller ':' etter assignment til ", varnavn), " ved "), selfhost__compiler__token_pos(stmt_start)), " */");
                    }
                    if (har_la && !(nl_streq(ass_op, "="))) {
                        return nl_concat(nl_concat("/* feil: 'la' støtter kun '=' ved ", selfhost__compiler__token_pos(stmt_start)), " */");
                    }
                    int idx_pre = selfhost__compiler__finn_navn_indeks(navn, varnavn);
                    if (!(nl_streq(ass_op, "=")) && (idx_pre < 0)) {
                        return nl_concat(nl_concat(nl_concat(nl_concat(nl_concat(nl_concat("/* feil: variabel '", varnavn), "' er ikke deklarert for '"), ass_op), "' ved "), selfhost__compiler__token_pos(stmt_start)), " */");
                    }
                    if (!(nl_streq(ass_op, "="))) {
                        char * binop = selfhost__compiler__assignment_op_til_binop(ass_op);
                        nl_list_text* rhs_copy = nl_list_text_new();
                        nl_list_text_push(rhs_copy, "");
                        int t = 0;
                        nl_list_text_remove(rhs_copy, 0);
                        while (t < nl_list_text_len(expr_tokens)) {
                            nl_list_text_push(rhs_copy, expr_tokens->data[t]);
                            t = (t + 1);
                        }
                        nl_list_text_remove(expr_tokens, 0);
                        nl_list_text_push(expr_tokens, varnavn);
                        nl_list_text_push(expr_tokens, binop);
                        t = 0;
                        while (t < nl_list_text_len(rhs_copy)) {
                            nl_list_text_push(expr_tokens, rhs_copy->data[t]);
                            t = (t + 1);
                        }
                    }
                    if ((nl_list_text_len(expr_tokens) > 0) && nl_streq(selfhost__compiler__normaliser_norsk_token(expr_tokens->data[0]), "hvis")) {
                        feil = selfhost__compiler__bygg_hvis_da_ellers_ops_med_miljo(expr_tokens, navn, miljo_verdier, expr_ops, expr_verdier);
                    }
                    else {
                        feil = selfhost__compiler__uttrykk_til_ops_og_verdier_med_miljo(expr_tokens, navn, miljo_verdier, expr_ops, expr_verdier);
                    }
                    if (!(nl_streq(feil, ""))) {
                        return feil;
                    }
                    feil = selfhost__compiler__eval_ops_til_verdi(expr_ops, expr_verdier, eval_resultat);
                    if (!(nl_streq(feil, ""))) {
                        return feil;
                    }
                    int idx = selfhost__compiler__finn_navn_indeks(navn, varnavn);
                    if (har_la && (idx >= 0)) {
                        return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: variabel '", varnavn), "' er allerede deklarert ved "), selfhost__compiler__token_pos(stmt_start)), " */");
                    }
                    if (har_sett && (idx < 0)) {
                        return nl_concat(nl_concat(nl_concat(nl_concat("/* feil: variabel '", varnavn), "' er ikke deklarert (bruk 'la') ved "), selfhost__compiler__token_pos(stmt_start)), " */");
                    }
                    if (idx >= 0) {
                        nl_list_int_set(miljo_verdier, idx, eval_resultat->data[0]);
                    }
                    else {
                        nl_list_text_push(navn, varnavn);
                        nl_list_int_push(miljo_verdier, eval_resultat->data[0]);
                    }
                    continue;
                }
                if (har_la) {
                    return nl_concat(nl_concat("/* feil: ugyldig deklarasjon etter 'la' ved ", selfhost__compiler__token_pos(stmt_start)), " */");
                }
                if (har_sett) {
                    return nl_concat(nl_concat("/* feil: ugyldig assignment etter 'sett' ved ", selfhost__compiler__token_pos(stmt_start)), " */");
                }
                nl_list_text* final_tokens = nl_list_text_new();
                nl_list_text_push(final_tokens, "");
                nl_list_text_remove(final_tokens, 0);
                if (nl_streq(selfhost__compiler__normaliser_norsk_token(stmt_tokens->data[0]), "returner")) {
                    int r = 1;
                    while (r < nl_list_text_len(stmt_tokens)) {
                        nl_list_text_push(final_tokens, stmt_tokens->data[r]);
                        r = (r + 1);
                    }
                    if (nl_list_text_len(final_tokens) == 0) {
                        return nl_concat(nl_concat("/* feil: 'returner' må etterfølges av uttrykk ved ", selfhost__compiler__token_pos(stmt_start)), " */");
                    }
                }
                else {
                    int r2 = 0;
                    while (r2 < nl_list_text_len(stmt_tokens)) {
                        nl_list_text_push(final_tokens, stmt_tokens->data[r2]);
                        r2 = (r2 + 1);
                    }
                }
                nl_list_text* stmt_ops = nl_list_text_new();
                nl_list_text_push(stmt_ops, "");
                nl_list_int* stmt_verdier = nl_list_int_new();
                nl_list_int_push(stmt_verdier, 0);
                char * final_feil = "";
                nl_list_text_remove(stmt_ops, 0);
                nl_list_int_remove(stmt_verdier, 0);
                if ((nl_list_text_len(final_tokens) > 0) && nl_streq(selfhost__compiler__normaliser_norsk_token(final_tokens->data[0]), "hvis")) {
                    final_feil = selfhost__compiler__bygg_hvis_da_ellers_ops_med_miljo(final_tokens, navn, miljo_verdier, stmt_ops, stmt_verdier);
                }
                else {
                    final_feil = selfhost__compiler__uttrykk_til_ops_og_verdier_med_miljo(final_tokens, navn, miljo_verdier, stmt_ops, stmt_verdier);
                }
                if (!(nl_streq(final_feil, ""))) {
                    return final_feil;
                }
                if (stmt_has_semicolon) {
                    int bare_tomme_etter = 1;
                    int sjekk_i = i;
                    while (sjekk_i < nl_list_text_len(tokens)) {
                        if (((!(nl_streq(tokens->data[sjekk_i], ";")) && !(nl_streq(tokens->data[sjekk_i], ","))) && !(nl_streq(tokens->data[sjekk_i], ":"))) && !(nl_streq(tokens->data[sjekk_i], ""))) {
                            bare_tomme_etter = 0;
                            break;
                        }
                        sjekk_i = (sjekk_i + 1);
                    }
                    if ((i >= nl_list_text_len(tokens)) || bare_tomme_etter) {
                        char * kopier_feil_slutt = selfhost__compiler__append_ops(out_ops, out_verdier, stmt_ops, stmt_verdier);
                        if (!(nl_streq(kopier_feil_slutt, ""))) {
                            return kopier_feil_slutt;
                        }
                        fant_sluttuttrykk = 1;
                        i = nl_list_text_len(tokens);
                        break;
                    }
                    if (nl_streq(selfhost__compiler__normaliser_norsk_token(stmt_tokens->data[0]), "returner")) {
                        char * kopier_feil = selfhost__compiler__append_ops(out_ops, out_verdier, stmt_ops, stmt_verdier);
                        if (!(nl_streq(kopier_feil, ""))) {
                            return kopier_feil;
                        }
                        fant_sluttuttrykk = 1;
                        while (i < nl_list_text_len(tokens)) {
                            if (((!(nl_streq(tokens->data[i], ";")) && !(nl_streq(tokens->data[i], ","))) && !(nl_streq(tokens->data[i], ":"))) && !(nl_streq(tokens->data[i], ""))) {
                                return nl_concat(nl_concat("/* feil: kun ';', ',' eller ':' er tillatt etter 'returner' (token ", nl_int_to_text(i)), ") */");
                            }
                            i = (i + 1);
                        }
                        break;
                    }
                    nl_list_int* stmt_eval = nl_list_int_new();
                    nl_list_int_push(stmt_eval, 0);
                    nl_list_int_remove(stmt_eval, 0);
                    final_feil = selfhost__compiler__eval_ops_til_verdi(stmt_ops, stmt_verdier, stmt_eval);
                    if (!(nl_streq(final_feil, ""))) {
                        return final_feil;
                    }
                    continue;
                }
                char * kopier_feil2 = selfhost__compiler__append_ops(out_ops, out_verdier, stmt_ops, stmt_verdier);
                if (!(nl_streq(kopier_feil2, ""))) {
                    return kopier_feil2;
                }
                fant_sluttuttrykk = 1;
                while (i < nl_list_text_len(tokens)) {
                    if (((!(nl_streq(tokens->data[i], ";")) && !(nl_streq(tokens->data[i], ","))) && !(nl_streq(tokens->data[i], ":"))) && !(nl_streq(tokens->data[i], ""))) {
                        return nl_concat(nl_concat("/* feil: kun siste statement kan være uttrykk (token ", nl_int_to_text(i)), ") */");
                    }
                    i = (i + 1);
                }
                break;
            }
            if (fant_sluttuttrykk == 0) {
                return "/* feil: skriptet mangler sluttuttrykk */";
            }
            return "";
            return "";
        }
        
        char * selfhost__compiler__disasm_skript(char * kilde) {
            nl_list_text* tokens = nl_tokenize_expression(kilde);
            nl_list_text* navn = nl_list_text_new();
            nl_list_text_push(navn, "");
            nl_list_int* miljo_verdier = nl_list_int_new();
            nl_list_int_push(miljo_verdier, 0);
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = "";
            nl_list_text_remove(navn, 0);
            nl_list_int_remove(miljo_verdier, 0);
            feil = selfhost__compiler__skript_til_ops_og_verdier(tokens, navn, miljo_verdier, ops, verdier);
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            nl_list_text_push(ops, "PRINT");
            nl_list_int_push(verdier, 0);
            nl_list_text_push(ops, "HALT");
            nl_list_int_push(verdier, 0);
            return selfhost__compiler__disasm_program(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__kompiler_skript_til_c(char * kilde) {
            nl_list_text* tokens = nl_tokenize_expression(kilde);
            nl_list_text* navn = nl_list_text_new();
            nl_list_text_push(navn, "");
            nl_list_int* miljo_verdier = nl_list_int_new();
            nl_list_int_push(miljo_verdier, 0);
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = "";
            nl_list_text_remove(navn, 0);
            nl_list_int_remove(miljo_verdier, 0);
            feil = selfhost__compiler__skript_til_ops_og_verdier(tokens, navn, miljo_verdier, ops, verdier);
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            nl_list_text_push(ops, "PRINT");
            nl_list_int_push(verdier, 0);
            nl_list_text_push(ops, "HALT");
            nl_list_int_push(verdier, 0);
            return selfhost__compiler__kompiler_til_c(ops, verdier);
            return "";
        }
        
        char * selfhost__compiler__kompiler_uttrykk_til_c_med_miljo(char * kilde, nl_list_text* navn, nl_list_int* miljo_verdier) {
            nl_list_text* tokens = nl_tokenize_expression(kilde);
            nl_list_text* ops = nl_list_text_new();
            nl_list_text_push(ops, "");
            nl_list_int* verdier = nl_list_int_new();
            nl_list_int_push(verdier, 0);
            char * feil = "";
            nl_list_text_remove(ops, 0);
            nl_list_int_remove(verdier, 0);
            if ((nl_list_text_len(tokens) > 0) && nl_streq(selfhost__compiler__normaliser_norsk_token(tokens->data[0]), "hvis")) {
                feil = selfhost__compiler__bygg_hvis_da_ellers_ops_med_miljo(tokens, navn, miljo_verdier, ops, verdier);
            }
            else {
                feil = selfhost__compiler__uttrykk_til_ops_og_verdier_med_miljo(tokens, navn, miljo_verdier, ops, verdier);
            }
            if (!(nl_streq(feil, ""))) {
                return feil;
            }
            nl_list_text_push(ops, "PRINT");
            nl_list_int_push(verdier, 0);
            nl_list_text_push(ops, "HALT");
            nl_list_int_push(verdier, 0);
            return selfhost__compiler__kompiler_til_c(ops, verdier);
            return "";
        }
        
        int start() {
            char * expr = selfhost__compiler__disasm_uttrykk("2 +");
            nl_print_text(expr);
            return 0;
            return 0;
        }
        
        int main(void) {
            return start();
        }