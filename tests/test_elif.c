#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <setjmp.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct { int *data; int len; int cap; } nl_list_int;
typedef struct { char **data; int len; int cap; } nl_list_text;
typedef struct { char **keys; int *values; int len; int cap; } nl_map_int;
typedef struct { char **keys; char **values; int len; int cap; } nl_map_text;
typedef struct { char **keys; int *values; int len; int cap; } nl_map_bool;
typedef struct { char **keys; void **values; int len; int cap; int value_type; } nl_map_any;
static const char *nl_call_stack[256];
static int nl_call_stack_len = 0;
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

static int nl_map_int_remove(nl_map_int *m, const char *key) {
    if (!m) { return 0; }
    int idx = nl_map_int_find(m, key);
    if (idx < 0) { return m->len; }
    for (int i = idx; i < m->len - 1; i++) { m->keys[i] = m->keys[i + 1]; m->values[i] = m->values[i + 1]; }
    m->len -= 1;
    return m->len;
}

static int nl_map_text_remove(nl_map_text *m, const char *key) {
    if (!m) { return 0; }
    int idx = nl_map_text_find(m, key);
    if (idx < 0) { return m->len; }
    for (int i = idx; i < m->len - 1; i++) { m->keys[i] = m->keys[i + 1]; m->values[i] = m->values[i + 1]; }
    m->len -= 1;
    return m->len;
}

static int nl_map_bool_remove(nl_map_bool *m, const char *key) {
    if (!m) { return 0; }
    int idx = nl_map_bool_find(m, key);
    if (idx < 0) { return m->len; }
    for (int i = idx; i < m->len - 1; i++) { m->keys[i] = m->keys[i + 1]; m->values[i] = m->values[i + 1]; }
    m->len -= 1;
    return m->len;
}

static int nl_map_any_remove(nl_map_any *m, const char *key) {
    if (!m) { return 0; }
    int idx = nl_map_any_find(m, key);
    if (idx < 0) { return m->len; }
    for (int i = idx; i < m->len - 1; i++) { m->keys[i] = m->keys[i + 1]; m->values[i] = m->values[i + 1]; }
    m->len -= 1;
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
        
        static int nl_assert_starter_med(const char *text, const char *prefix) {
            if (!text) { text = ""; }
            if (!prefix) { prefix = ""; }
            if (strncmp(text, prefix, strlen(prefix)) != 0) {
                printf("assert_starter_med failed: \"%s\" does not start with \"%s\"\n", text, prefix);
                exit(1);
            }
            return 0;
        }
        
        static int nl_assert_slutter_med(const char *text, const char *suffix) {
            if (!text) { text = ""; }
            if (!suffix) { suffix = ""; }
            size_t text_len = strlen(text);
            size_t suffix_len = strlen(suffix);
            if (text_len < suffix_len || strcmp(text + (text_len - suffix_len), suffix) != 0) {
                printf("assert_slutter_med failed: \"%s\" does not end with \"%s\"\n", text, suffix);
                exit(1);
            }
            return 0;
        }
        
        static int nl_assert_inneholder(const char *text, const char *needle) {
            if (!text) { text = ""; }
            if (!needle) { needle = ""; }
            if (!strstr(text, needle)) {
                printf("assert_inneholder failed: \"%s\" does not contain \"%s\"\n", text, needle);
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
            int call_depth;
        };
        
        static nl_try_frame *nl_try_stack = NULL;
        
        static void nl_push_try_frame(nl_try_frame *frame) {
            frame->previous = nl_try_stack;
            frame->error_message = NULL;
            frame->call_depth = nl_call_stack_len;
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
                if (nl_call_stack_len > 0) {
                    fprintf(stderr, "Kallstakk: ");
                    for (int i = nl_call_stack_len - 1; i >= 0; --i) {
                        fprintf(stderr, "%s%s", nl_call_stack[i], i > 0 ? " -> " : "");
                    }
                    fprintf(stderr, "\n");
                }
                exit(1);
            }
            nl_try_stack->error_message = nl_strdup(message ? message : "");
            longjmp(nl_try_stack->jump_point, 1);
        }
        
        typedef struct {
            int active;
            int value;
            int cancelled;
            int has_deadline;
            long long deadline_ms;
        } nl_async_handle;
        static nl_async_handle nl_async_handles[256];
        static int nl_async_next_handle = 1;
        
        static long long nl_now_ms(void) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            return (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;
        }
        
        static int nl_async_make_handle(int value, int cancelled, int has_deadline, long long deadline_ms) {
            int id = nl_async_next_handle++;
            if (id >= 256) {
                fprintf(stderr, "for mange async-handles\n");
                exit(1);
            }
            nl_async_handles[id].active = 1;
            nl_async_handles[id].value = value;
            nl_async_handles[id].cancelled = cancelled;
            nl_async_handles[id].has_deadline = has_deadline;
            nl_async_handles[id].deadline_ms = deadline_ms;
            return -id;
        }
        
        static int nl_async_is_handle(int value) {
            int id = -value;
            return value < 0 && id > 0 && id < 256 && nl_async_handles[id].active;
        }
        
        static int nl_async_timeout(int value, int timeout_ms) {
            long long deadline = nl_now_ms() + (timeout_ms < 0 ? 0 : timeout_ms);
            return nl_async_make_handle(value, 0, 1, deadline);
        }
        
        static int nl_async_cancel(int value) {
            if (nl_async_is_handle(value)) {
                int id = -value;
                nl_async_handles[id].cancelled = 1;
                return value;
            }
            return nl_async_make_handle(value, 1, 0, 0);
        }
        
        static int nl_async_is_cancelled(int value) {
            if (!nl_async_is_handle(value)) { return 0; }
            return nl_async_handles[-value].cancelled;
        }
        
        static int nl_async_is_timed_out(int value) {
            if (!nl_async_is_handle(value)) { return 0; }
            nl_async_handle *h = &nl_async_handles[-value];
            return h->has_deadline && nl_now_ms() > h->deadline_ms;
        }
        
        static void nl_sleep_ms(int ms) {
            if (ms < 0) { ms = 0; }
            usleep((useconds_t)ms * 1000);
        }
        
        static int nl_async_await(int value) {
            if (!nl_async_is_handle(value)) { return value; }
            nl_async_handle *h = &nl_async_handles[-value];
            if (h->cancelled) { nl_throw("AvbruttFeil: future ble kansellert"); return h->value; }
            if (h->has_deadline && nl_now_ms() > h->deadline_ms) { nl_throw("TimeoutFeil: future gikk ut på tid"); return h->value; }
            return h->value;
        }
        
        static char *nl_io_error(const char *handling, const char *path) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "IOFeil: kunne ikke %s fil %s", handling ? handling : "", path ? path : "");
            return nl_strdup(buffer);
        }
        
        static int nl_slice_start(int len, int value, int has_value) {
            if (!has_value) { return 0; }
            if (value < 0) { value += len; }
            if (value < 0) { value = 0; }
            if (value > len) { value = len; }
            return value;
        }
        
        static int nl_slice_end(int len, int value, int has_value) {
            if (!has_value) { return len; }
            if (value < 0) { value += len; }
            if (value < 0) { value = 0; }
            if (value > len) { value = len; }
            return value;
        }
        
        static char *nl_path_join(const char *left, const char *right) {
            if (!left || !*left) { return nl_strdup(right ? right : ""); }
            if (!right || !*right) { return nl_strdup(left); }
            size_t left_len = strlen(left);
            while (left_len > 0 && (left[left_len - 1] == '/' || left[left_len - 1] == '\\')) { left_len--; }
            const char *right_part = right;
            while (*right_part == '/' || *right_part == '\\') { right_part++; }
            size_t right_len = strlen(right_part);
            char *out = malloc(left_len + 1 + right_len + 1);
            if (!out) { perror("malloc"); exit(1); }
            memcpy(out, left, left_len);
            out[left_len] = '/';
            memcpy(out + left_len + 1, right_part, right_len);
            out[left_len + 1 + right_len] = '\0';
            return out;
        }
        
        static char *nl_path_basename(const char *path) {
            if (!path || !*path) { return nl_strdup(""); }
            const char *last = path;
            for (const char *p = path; *p; ++p) { if (*p == '/' || *p == '\\') { last = p + 1; } }
            return nl_strdup(last);
        }
        
        static char *nl_path_dirname(const char *path) {
            if (!path || !*path) { return nl_strdup(""); }
            const char *last = NULL;
            for (const char *p = path; *p; ++p) { if (*p == '/' || *p == '\\') { last = p; } }
            if (!last) { return nl_strdup(""); }
            size_t len = (size_t)(last - path);
            char *out = malloc(len + 1);
            if (!out) { perror("malloc"); exit(1); }
            memcpy(out, path, len);
            out[len] = '\0';
            return out;
        }
        
        static char *nl_path_stem(const char *path) {
            char *base = nl_path_basename(path);
            char *dot = strrchr(base, '.');
            if (dot) { *dot = '\0'; }
            return base;
        }
        
        static int nl_path_exists(const char *path) {
            if (!path || !*path) { return 0; }
            FILE *f = fopen(path, "rb");
            if (!f) { return 0; }
            fclose(f);
            return 1;
        }
        
        static char *nl_env_get(const char *key) {
            const char *value = key ? getenv(key) : NULL;
            return nl_strdup(value ? value : "");
        }
        
        static int nl_env_exists(const char *key) {
            return key ? getenv(key) != NULL : 0;
        }
        
        static char *nl_env_set(const char *key, const char *value) {
            if (!key) { return nl_strdup(value ? value : ""); }
            setenv(key, value ? value : "", 1);
            return nl_strdup(value ? value : "");
        }
        
        static nl_map_text *nl_http_response_parse(const char *response) {
            if (!response) { return nl_map_text_new(); }
            return json_parse(response);
        }
        
        static int nl_http_response_status(const char *response) {
            nl_map_text *parsed = nl_http_response_parse(response);
            return atoi(nl_map_text_get(parsed, "status"));
        }
        
        static char *nl_http_response_text(const char *response) {
            nl_map_text *parsed = nl_http_response_parse(response);
            return nl_strdup(nl_map_text_get(parsed, "body"));
        }
        
        static nl_map_text *nl_http_response_json(const char *response) {
            nl_map_text *parsed = nl_http_response_parse(response);
            char *body = nl_map_text_get(parsed, "body");
            return json_parse(body);
        }
        
        static char *nl_http_response_header(const char *response, const char *key) {
            nl_map_text *parsed = nl_http_response_parse(response);
            char *headers_raw = nl_map_text_get(parsed, "headers");
            nl_map_text *headers = json_parse(headers_raw);
            return nl_strdup(nl_map_text_get(headers, key));
        }
        
        static nl_map_text *nl_web_request_context(const char *method, const char *path, nl_map_text *query, nl_map_text *headers, const char *body) {
            nl_map_text *result = nl_map_text_new();
            char *method_text = nl_strdup(method ? method : "");
            for (char *p = method_text; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }
            nl_map_text_set(result, "metode", method_text);
            nl_map_text_set(result, "sti", nl_strdup(path ? path : ""));
            nl_map_text_set(result, "query", json_stringify(query ? query : nl_map_text_new()));
            nl_map_text_set(result, "headers", json_stringify(headers ? headers : nl_map_text_new()));
            nl_map_text_set(result, "body", nl_strdup(body ? body : ""));
            nl_map_text_set(result, "params", json_stringify(nl_map_text_new()));
            return result;
        }
        
        static char *nl_web_request_method(nl_map_text *ctx) {
            return nl_strdup(nl_map_text_get(ctx, "metode"));
        }
        
        static char *nl_web_request_path(nl_map_text *ctx) {
            return nl_strdup(nl_map_text_get(ctx, "sti"));
        }
        
        static nl_map_text *nl_web_request_query(nl_map_text *ctx) {
            return json_parse(nl_map_text_get(ctx, "query"));
        }
        
        static nl_map_text *nl_web_request_headers(nl_map_text *ctx) {
            return json_parse(nl_map_text_get(ctx, "headers"));
        }
        
        static char *nl_web_request_body(nl_map_text *ctx) {
            return nl_strdup(nl_map_text_get(ctx, "body"));
        }
        
        static nl_map_text *nl_web_request_params(nl_map_text *ctx) {
            return json_parse(nl_map_text_get(ctx, "params"));
        }
        
        static char *nl_web_request_param(nl_map_text *ctx, const char *key) {
            nl_map_text *params = nl_web_request_params(ctx);
            return nl_strdup(nl_map_text_get(params, key));
        }
        
        static int nl_web_validate_int_text(const char *value, const char *field_name, const char *source_name);
        static int nl_web_validate_bool_text(const char *value, const char *field_name, const char *source_name);
        
        static int nl_web_request_param_int(nl_map_text *ctx, const char *key) {
            nl_map_text *params = nl_web_request_params(ctx);
            return nl_web_validate_int_text(nl_map_text_get(params, key), key, "path");
        }
        
        static int nl_web_validate_int_text(const char *value, const char *field_name, const char *source_name) {
            if (!value || !*value) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "ValideringsFeil: mangler %s-felt '%s'", source_name ? source_name : "", field_name ? field_name : "");
                nl_throw(buffer);
                return 0;
            }
            char *end = NULL;
            long parsed = strtol(value, &end, 10);
            if (!end || *end != '\0') {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "ValideringsFeil: felt '%s' forventet heltall, fikk '%s'", field_name ? field_name : "", value ? value : "");
                nl_throw(buffer);
                return 0;
            }
            return (int)parsed;
        }
        
        static int nl_web_validate_bool_text(const char *value, const char *field_name, const char *source_name) {
            if (!value || !*value) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "ValideringsFeil: mangler %s-felt '%s'", source_name ? source_name : "", field_name ? field_name : "");
                nl_throw(buffer);
                return 0;
            }
            char lower[32];
            size_t n = strlen(value);
            if (n >= sizeof(lower)) { n = sizeof(lower) - 1; }
            for (size_t i = 0; i < n; ++i) { lower[i] = (char)tolower((unsigned char)value[i]); }
            lower[n] = '\0';
            if (strcmp(lower, "true") == 0 || strcmp(lower, "1") == 0 || strcmp(lower, "ja") == 0) { return 1; }
            if (strcmp(lower, "false") == 0 || strcmp(lower, "0") == 0 || strcmp(lower, "nei") == 0) { return 0; }
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "ValideringsFeil: felt '%s' forventet bool, fikk '%s'", field_name ? field_name : "", value ? value : "");
            nl_throw(buffer);
            return 0;
        }
        
        static char *nl_web_request_query_param(nl_map_text *ctx, const char *key) {
            nl_map_text *query = nl_web_request_query(ctx);
            return nl_strdup(nl_map_text_get(query, key));
        }
        
        static char *nl_web_request_query_required(nl_map_text *ctx, const char *key) {
            nl_map_text *query = nl_web_request_query(ctx);
            char *value = nl_map_text_get(query, key);
            if (!value || !*value) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "ValideringsFeil: mangler query-felt '%s'", key ? key : "");
                nl_throw(buffer);
                return nl_strdup("");
            }
            return nl_strdup(value);
        }
        
        static int nl_web_request_query_int(nl_map_text *ctx, const char *key) {
            nl_map_text *query = nl_web_request_query(ctx);
            return nl_web_validate_int_text(nl_map_text_get(query, key), key, "query");
        }
        
        static nl_map_text *nl_web_request_json(nl_map_text *ctx) {
            char *body = nl_web_request_body(ctx);
            const char *p = body;
            while (p && *p && isspace((unsigned char)*p)) { p++; }
            if (!p || !*p) {
                return nl_map_text_new();
            }
            if (*p != '{') { nl_throw("ValideringsFeil: body forventet JSON-objekt"); }
            return json_parse(body);
        }
        
        static char *nl_web_request_json_field(nl_map_text *ctx, const char *key) {
            nl_map_text *data = nl_web_request_json(ctx);
            char *value = nl_map_text_get(data, key);
            if (!value || !*value) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "ValideringsFeil: mangler felt '%s'", key ? key : "");
                nl_throw(buffer);
                return nl_strdup("");
            }
            return nl_strdup(value);
        }
        
        static char *nl_web_request_json_field_or(nl_map_text *ctx, const char *key, const char *fallback) {
            nl_map_text *data = nl_web_request_json(ctx);
            char *value = nl_map_text_get(data, key);
            if (!value || !*value) { return nl_strdup(fallback ? fallback : ""); }
            return nl_strdup(value);
        }
        
        static int nl_web_request_json_field_int(nl_map_text *ctx, const char *key) {
            nl_map_text *data = nl_web_request_json(ctx);
            return nl_web_validate_int_text(nl_map_text_get(data, key), key, "body");
        }
        
        static int nl_web_request_json_field_bool(nl_map_text *ctx, const char *key) {
            nl_map_text *data = nl_web_request_json(ctx);
            return nl_web_validate_bool_text(nl_map_text_get(data, key), key, "body");
        }
        
        static char *nl_web_request_header(nl_map_text *ctx, const char *key) {
            nl_map_text *headers = nl_web_request_headers(ctx);
            return nl_strdup(nl_map_text_get(headers, key));
        }
        
        static nl_map_text *nl_web_response_builder(int status, nl_map_text *headers, const char *body) {
            nl_map_text *result = nl_map_text_new();
            char status_buf[32];
            snprintf(status_buf, sizeof(status_buf), "%d", status);
            nl_map_text_set(result, "status", nl_strdup(status_buf));
            nl_map_text_set(result, "headers", json_stringify(headers ? headers : nl_map_text_new()));
            nl_map_text_set(result, "body", nl_strdup(body ? body : ""));
            return result;
        }
        
        static int nl_web_response_status(nl_map_text *response) {
            return atoi(nl_map_text_get(response, "status"));
        }
        
        static nl_map_text *nl_web_response_headers(nl_map_text *response) {
            return json_parse(nl_map_text_get(response, "headers"));
        }
        
        static char *nl_web_response_body(nl_map_text *response) {
            return nl_strdup(nl_map_text_get(response, "body"));
        }
        
        static char *nl_web_response_header(nl_map_text *response, const char *key) {
            nl_map_text *headers = nl_web_response_headers(response);
            return nl_strdup(nl_map_text_get(headers, key));
        }
        
        static char *nl_web_response_text(nl_map_text *response) {
            return nl_web_response_body(response);
        }
        
        static nl_map_text *nl_web_response_json(nl_map_text *response) {
            return json_parse(nl_map_text_get(response, "body"));
        }
        
        static nl_map_text *nl_web_response_error(int status, const char *message) {
            nl_map_text *headers = nl_map_text_new();
            nl_map_text_set(headers, "content-type", nl_strdup("application/json"));
            nl_map_text *body = nl_map_text_new();
            nl_map_text_set(body, "error", nl_strdup(message ? message : ""));
            char *body_json = json_stringify(body);
            return nl_web_response_builder(status, headers, body_json);
        }
        
        static char *nl_web_route(const char *spec) {
            return nl_strdup(spec ? spec : "");
        }
        
        static nl_map_text *nl_web_handle_request(nl_map_text *ctx);
        
        static nl_map_text *nl_web_response_file(const char *path, const char *content_type) {
            char *body = nl_strdup("");
            if (path) {
                FILE *f = fopen(path, "rb");
                if (!f) { nl_throw(nl_io_error("lese", path)); return nl_web_response_builder(200, nl_map_text_new(), body); }
                fseek(f, 0, SEEK_END);
                long len = ftell(f);
                fseek(f, 0, SEEK_SET);
                if (len < 0) { fclose(f); nl_throw(nl_io_error("lese", path)); return nl_web_response_builder(200, nl_map_text_new(), body); }
                char *buffer = malloc((size_t)len + 1);
                if (!buffer) { fclose(f); perror("malloc"); exit(1); }
                size_t read_len = fread(buffer, 1, (size_t)len, f);
                buffer[read_len] = '\0';
                fclose(f);
                body = buffer;
            }
            nl_map_text *headers = nl_map_text_new();
            nl_map_text_set(headers, "content-type", nl_strdup(content_type ? content_type : "application/octet-stream"));
            return nl_web_response_builder(200, headers, body);
        }
        
        static int nl_web_is_int_segment(const char *text) {
            if (!text || !*text) { return 0; }
            const char *p = text;
            if (*p == '-') { p++; }
            if (!*p) { return 0; }
            while (*p) {
                if (*p < '0' || *p > '9') { return 0; }
                p++;
            }
            return 1;
        }
        
        static int nl_web_match_pattern(const char *pattern, const char *path, nl_map_text *params) {
            char *pattern_copy = nl_strdup(pattern ? pattern : "");
            char *path_copy = nl_strdup(path ? path : "");
            char *pattern_state = NULL;
            char *path_state = NULL;
            char *pattern_part = strtok_r(pattern_copy, "/", &pattern_state);
            char *path_part = strtok_r(path_copy, "/", &path_state);
            while (pattern_part || path_part) {
                if (!pattern_part || !path_part) { free(pattern_copy); free(path_copy); return 0; }
                size_t pattern_len = strlen(pattern_part);
                if (pattern_len >= 3 && pattern_part[0] == '{' && pattern_part[pattern_len - 1] == '}') {
                    char *inner = nl_strdup(pattern_part + 1);
                    inner[pattern_len - 2] = '\0';
                    char *colon = strchr(inner, ':');
                    char *name = inner;
                    char *type = NULL;
                    if (colon) {
                        *colon = '\0';
                        type = colon + 1;
                    }
                    if (!name || !*name) { free(inner); free(pattern_copy); free(path_copy); return 0; }
                    if (type && strcmp(type, "int") == 0 && !nl_web_is_int_segment(path_part)) { free(inner); free(pattern_copy); free(path_copy); return 0; }
                    if (params) { nl_map_text_set(params, nl_strdup(name), nl_strdup(path_part)); }
                    free(inner);
                } else if (strcmp(pattern_part, path_part) != 0) {
                    free(pattern_copy);
                    free(path_copy);
                    return 0;
                }
                pattern_part = strtok_r(NULL, "/", &pattern_state);
                path_part = strtok_r(NULL, "/", &path_state);
            }
            free(pattern_copy);
            free(path_copy);
            return 1;
        }
        
        static void nl_web_split_route_spec(const char *spec, char **method_out, char **path_out) {
            char *copy = nl_strdup(spec ? spec : "");
            char *space = strchr(copy, ' ');
            if (!space) {
                *method_out = nl_strdup("");
                *path_out = copy;
                return;
            }
            *space = '\0';
            *method_out = nl_strdup(copy);
            for (char *p = *method_out; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }
            *path_out = nl_strdup(space + 1);
            free(copy);
        }
        
        static int nl_web_route_match(const char *spec, const char *method, const char *path) {
            char *route_method = NULL;
            char *route_path = NULL;
            nl_web_split_route_spec(spec, &route_method, &route_path);
            int ok = 1;
            if (route_method && *route_method) {
                char *method_copy = nl_strdup(method ? method : "");
                for (char *p = method_copy; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }
                ok = strcmp(route_method, method_copy) == 0;
                free(method_copy);
            }
            if (ok) { ok = nl_web_match_pattern(route_path, path, NULL); }
            free(route_method);
            free(route_path);
            return ok;
        }
        
        static char *nl_web_dispatch(nl_map_text *routes, const char *method, const char *path) {
            if (!routes) { return nl_strdup(""); }
            for (int i = 0; i < routes->len; ++i) {
                if (nl_web_route_match(routes->values[i], method, path)) {
                    return nl_strdup(routes->keys[i]);
                }
            }
            return nl_strdup("");
        }
        
        static nl_map_text *nl_web_dispatch_params(nl_map_text *routes, const char *method, const char *path) {
            nl_map_text *result = nl_map_text_new();
            if (!routes) { return result; }
            for (int i = 0; i < routes->len; ++i) {
                char *route_method = NULL;
                char *route_path = NULL;
                nl_web_split_route_spec(routes->values[i], &route_method, &route_path);
                int ok = 1;
                if (route_method && *route_method) {
                    char *method_copy = nl_strdup(method ? method : "");
                    for (char *p = method_copy; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }
                    ok = strcmp(route_method, method_copy) == 0;
                    free(method_copy);
                }
                if (ok) {
                    nl_map_text *params = nl_map_text_new();
                    if (nl_web_match_pattern(route_path, path, params)) {
                        free(route_method);
                        free(route_path);
                        return params;
                    }
                    }
                    free(route_method);
                    free(route_path);
                }
                return result;
            }
            
            static int nl_web_path_match(const char *pattern, const char *path) {
                return nl_web_match_pattern(pattern, path, NULL);
            }
            
            static nl_map_text *nl_web_path_params(const char *pattern, const char *path) {
                nl_map_text *result = nl_map_text_new();
                if (!nl_web_match_pattern(pattern, path, result)) {
                    return nl_map_text_new();
                }
                return result;
            }
            
            static int nl_text_starter_med(const char *text, const char *prefix) {
                return text && prefix ? strncmp(text, prefix, strlen(prefix)) == 0 : 0;
            }
            
            static int nl_text_slutter_med(const char *text, const char *suffix) {
                if (!text || !suffix) { return 0; }
                size_t text_len = strlen(text);
                size_t suffix_len = strlen(suffix);
                if (suffix_len > text_len) { return 0; }
                return strcmp(text + (text_len - suffix_len), suffix) == 0;
            }
            
            static int nl_text_inneholder(const char *text, const char *needle) {
                if (!text || !needle) { return 0; }
                return strstr(text, needle) != NULL;
            }
            
            static char *nl_text_trim(const char *text) {
                if (!text) { return nl_strdup(""); }
                const char *start = text;
                while (*start && isspace((unsigned char)*start)) { start++; }
                const char *end = text + strlen(text);
                while (end > start && isspace((unsigned char)*(end - 1))) { end--; }
                size_t len = (size_t)(end - start);
                char *out = malloc(len + 1);
                if (!out) { perror("malloc"); exit(1); }
                memcpy(out, start, len);
                out[len] = '\0';
                return out;
            }
            
            static char *nl_text_slice(const char *text, int start, int has_start, int end, int has_end) {
                if (!text) { return nl_strdup(""); }
                int len = (int)strlen(text);
                int s = nl_slice_start(len, start, has_start);
                int e = nl_slice_end(len, end, has_end);
                if (e < s) { e = s; }
                int out_len = e - s;
                char *out = malloc((size_t)out_len + 1);
                if (!out) { perror("malloc"); exit(1); }
                memcpy(out, text + s, (size_t)out_len);
                out[out_len] = '\0';
                return out;
            }
            
            static nl_list_int *nl_list_int_slice(nl_list_int *list, int start, int has_start, int end, int has_end) {
                nl_list_int *out = nl_list_int_new();
                if (!list) { return out; }
                int s = nl_slice_start(list->len, start, has_start);
                int e = nl_slice_end(list->len, end, has_end);
                if (e < s) { e = s; }
                for (int i = s; i < e; i++) { nl_list_int_push(out, list->data[i]); }
                return out;
            }
            
            static nl_list_text *nl_list_text_slice(nl_list_text *list, int start, int has_start, int end, int has_end) {
                nl_list_text *out = nl_list_text_new();
                if (!list) { return out; }
                int s = nl_slice_start(list->len, start, has_start);
                int e = nl_slice_end(list->len, end, has_end);
                if (e < s) { e = s; }
                for (int i = s; i < e; i++) { nl_list_text_push(out, nl_strdup(list->data[i])); }
                return out;
            }
            
            static char *nl_file_read_text(const char *path) {
                if (!path) { return nl_strdup(""); }
                FILE *f = fopen(path, "rb");
                if (!f) { nl_throw(nl_io_error("lese", path)); return nl_strdup(""); }
                fseek(f, 0, SEEK_END);
                long len = ftell(f);
                fseek(f, 0, SEEK_SET);
                if (len < 0) { fclose(f); nl_throw(nl_io_error("lese", path)); return nl_strdup(""); }
                char *buffer = malloc((size_t)len + 1);
                if (!buffer) { fclose(f); perror("malloc"); exit(1); }
                size_t read_len = fread(buffer, 1, (size_t)len, f);
                buffer[read_len] = '\0';
                fclose(f);
                return buffer;
            }
            
            static char *nl_file_write_text(const char *path, const char *text, const char *mode) {
                if (!path) { return nl_strdup(text ? text : ""); }
                FILE *f = fopen(path, mode);
                if (!f) { nl_throw(nl_io_error(strcmp(mode, "a") == 0 ? "legge til i" : "skrive", path)); return nl_strdup(text ? text : ""); }
                if (text) { fputs(text, f); }
                fclose(f);
                return nl_strdup(text ? text : "");
            }
            
            static int nl_file_exists(const char *path) {
                if (!path) { return 0; }
                FILE *f = fopen(path, "rb");
                if (!f) { return 0; }
                fclose(f);
                return 1;
            }
            
            int start() {
                const int nl_call_stack_mark = nl_call_stack_len;
                nl_call_stack[nl_call_stack_len++] = "start";
                int x = 5;
                if (x > 5) {
                    nl_assert_eq_int(1, 0);
                }
                else if (x == 5) {
                    nl_assert_eq_int(1, 1);
                }
                else {
                    nl_assert_eq_int(1, 0);
                }
                nl_call_stack_len = nl_call_stack_mark;
                return 0;
                nl_call_stack_len = nl_call_stack_mark;
                return 0;
            }
            
            static int nl_web_route_match_params(const char *spec, const char *method, const char *path, nl_map_text *params) {
                char *route_method = NULL;
                char *route_path = NULL;
                nl_web_split_route_spec(spec, &route_method, &route_path);
                int ok = 1;
                if (route_method && *route_method) {
                    char *method_copy = nl_strdup(method ? method : "");
                    for (char *p = method_copy; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }
                    ok = strcmp(route_method, method_copy) == 0;
                    free(method_copy);
                }
                if (ok) { ok = nl_web_match_pattern(route_path, path, params); }
                free(route_method);
                free(route_path);
                return ok;
            }
            
            static int nl_web_route_path_match(const char *spec, const char *path) {
                char *route_method = NULL;
                char *route_path = NULL;
                nl_web_split_route_spec(spec, &route_method, &route_path);
                int ok = nl_web_match_pattern(route_path, path, NULL);
                free(route_method);
                free(route_path);
                return ok;
            }
            
            static nl_map_text *nl_web_handle_request(nl_map_text *ctx) {
                char *method = nl_web_request_method(ctx);
                char *path = nl_web_request_path(ctx);
                int path_mismatch = 0;
                if (path_mismatch) {
                    return nl_web_response_error(405, "Metode ikke tillatt");
                }
                return nl_web_response_error(404, "Ikke funnet");
            }
            
            
            int main(void) {
                return start();
            }