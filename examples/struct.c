#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

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

int beregn_total(nl_map_int* bruker) {
    int poeng = nl_map_int_get_required(bruker, "poeng");
    int bonus = nl_map_int_get_required(bruker, "ekstra");
    return (poeng + bonus);
    return 0;
}

int start() {
    nl_map_int* bruker = nl_map_int_new();
    nl_map_int_set(bruker, "poeng", 10);
    nl_map_int_set(bruker, "ekstra", 5);
    nl_print_text("Bruker total:");
    int total = beregn_total(bruker);
    nl_print_text(nl_int_to_text(total));
    nl_map_any* team = nl_map_any_new(NL_MAP_ANY_MAP);
    nl_map_int* __nl_map_1 = nl_map_int_new();
    nl_map_int_set(__nl_map_1, "poeng", 10);
    nl_map_int_set(__nl_map_1, "ekstra", 2);
    nl_map_any_set_map(team, "leder", __nl_map_1);
    nl_map_int* __nl_map_2 = nl_map_int_new();
    nl_map_int_set(__nl_map_2, "poeng", 7);
    nl_map_int_set(__nl_map_2, "ekstra", 1);
    nl_map_any_set_map(team, "medlem", __nl_map_2);
    nl_print_text("Lederpoeng:");
    nl_print_text(nl_int_to_text(nl_map_int_get_required(nl_map_any_get_required_map(team, "leder"), "poeng")));
    nl_print_text("Lederen har tillegg:");
    nl_print_text(nl_int_to_text(nl_map_int_get_required(nl_map_any_get_required_map(team, "leder"), "ekstra")));
    return 0;
    return 0;
}

int main(void) {
    return start();
}