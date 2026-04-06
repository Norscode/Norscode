#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { int *data; int len; int cap; } nl_list_int;
typedef struct { char **data; int len; int cap; } nl_list_text;

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
    char * push_linje = selfhost__compiler__instruksjon_til_c("PUSH", 42);
    nl_assert_eq_text(push_linje, "stack[sp++] = 42;");
    char * add_linje = selfhost__compiler__instruksjon_til_c("ADD", 0);
    nl_assert_eq_text(add_linje, "stack[sp-2] = stack[sp-2] + stack[sp-1]; sp = sp - 1;");
    char * halt_linje = selfhost__compiler__instruksjon_til_c("HALT", 0);
    nl_assert_eq_text(halt_linje, "return 0;");
    char * label_linje = selfhost__compiler__instruksjon_til_c("LABEL", 7);
    nl_assert_eq_text(label_linje, "L7:;");
    char * jmp_linje = selfhost__compiler__instruksjon_til_c("JMP", 7);
    nl_assert_eq_text(jmp_linje, "goto L7;");
    char * jz_linje = selfhost__compiler__instruksjon_til_c("JZ", 7);
    nl_assert_eq_text(jz_linje, "if (stack[sp-1] == 0) goto L7;");
    char * call_linje = selfhost__compiler__instruksjon_til_c("CALL", 7);
    nl_assert_eq_text(call_linje, "ret_stack[rsp++] = 0; goto L7;");
    char * ret_linje = selfhost__compiler__instruksjon_til_c("RET", 0);
    nl_assert_eq_text(ret_linje, "goto *labels[0];");
    char * store_linje = selfhost__compiler__instruksjon_til_c("STORE", 3);
    nl_assert_eq_text(store_linje, "mem[3] = stack[sp-1]; sp = sp - 1;");
    char * load_linje = selfhost__compiler__instruksjon_til_c("LOAD", 3);
    nl_assert_eq_text(load_linje, "stack[sp++] = mem[3];");
    char * eq_linje = selfhost__compiler__instruksjon_til_c("EQ", 0);
    nl_assert_eq_text(eq_linje, "stack[sp-2] = (stack[sp-2] == stack[sp-1]); sp = sp - 1;");
    char * gt_linje = selfhost__compiler__instruksjon_til_c("GT", 0);
    nl_assert_eq_text(gt_linje, "stack[sp-2] = (stack[sp-2] > stack[sp-1]); sp = sp - 1;");
    char * lt_linje = selfhost__compiler__instruksjon_til_c("LT", 0);
    nl_assert_eq_text(lt_linje, "stack[sp-2] = (stack[sp-2] < stack[sp-1]); sp = sp - 1;");
    char * mod_linje = selfhost__compiler__instruksjon_til_c("MOD", 0);
    nl_assert_eq_text(mod_linje, "stack[sp-2] = stack[sp-2] % stack[sp-1]; sp = sp - 1;");
    char * and_linje = selfhost__compiler__instruksjon_til_c("AND", 0);
    nl_assert_eq_text(and_linje, "stack[sp-2] = (stack[sp-2] && stack[sp-1]); sp = sp - 1;");
    char * or_linje = selfhost__compiler__instruksjon_til_c("OR", 0);
    nl_assert_eq_text(or_linje, "stack[sp-2] = (stack[sp-2] || stack[sp-1]); sp = sp - 1;");
    char * not_linje = selfhost__compiler__instruksjon_til_c("NOT", 0);
    nl_assert_eq_text(not_linje, "stack[sp-1] = !stack[sp-1];");
    char * dup_linje = selfhost__compiler__instruksjon_til_c("DUP", 0);
    nl_assert_eq_text(dup_linje, "stack[sp] = stack[sp-1]; sp = sp + 1;");
    char * pop_linje = selfhost__compiler__instruksjon_til_c("POP", 0);
    nl_assert_eq_text(pop_linje, "sp = sp - 1;");
    char * swap_linje = selfhost__compiler__instruksjon_til_c("SWAP", 0);
    nl_assert_eq_text(swap_linje, "tmp = stack[sp-1]; stack[sp-1] = stack[sp-2]; stack[sp-2] = tmp;");
    char * over_linje = selfhost__compiler__instruksjon_til_c("OVER", 0);
    nl_assert_eq_text(over_linje, "stack[sp] = stack[sp-2]; sp = sp + 1;");
    char * demo = selfhost__compiler__demo_program();
    nl_assert_ne_text(demo, "");
    nl_list_text* ops = nl_list_text_new();
    nl_list_text_push(ops, "PUSH");
    nl_list_text_push(ops, "PUSH");
    nl_list_text_push(ops, "MUL");
    nl_list_text_push(ops, "PRINT");
    nl_list_text_push(ops, "HALT");
    nl_list_int* verdier = nl_list_int_new();
    nl_list_int_push(verdier, 6);
    nl_list_int_push(verdier, 7);
    nl_list_int_push(verdier, 0);
    nl_list_int_push(verdier, 0);
    nl_list_int_push(verdier, 0);
    char * c = selfhost__compiler__kompiler_til_c(ops, verdier);
    nl_assert_ne_text(c, "");
    nl_list_text* tokens = nl_list_text_new();
    nl_list_text_push(tokens, "PUSH");
    nl_list_text_push(tokens, "8");
    nl_list_text_push(tokens, "PUSH");
    nl_list_text_push(tokens, "9");
    nl_list_text_push(tokens, "ADD");
    nl_list_text_push(tokens, "PRINT");
    nl_list_text_push(tokens, "HALT");
    char * c2 = selfhost__compiler__kompiler_fra_tokens(tokens);
    nl_assert_ne_text(c2, "");
    nl_list_text* tokens_jmp = nl_list_text_new();
    nl_list_text_push(tokens_jmp, "PUSH");
    nl_list_text_push(tokens_jmp, "0");
    nl_list_text_push(tokens_jmp, "JZ");
    nl_list_text_push(tokens_jmp, "1");
    nl_list_text_push(tokens_jmp, "PUSH");
    nl_list_text_push(tokens_jmp, "99");
    nl_list_text_push(tokens_jmp, "PRINT");
    nl_list_text_push(tokens_jmp, "LABEL");
    nl_list_text_push(tokens_jmp, "1");
    nl_list_text_push(tokens_jmp, "HALT");
    char * c2b = selfhost__compiler__kompiler_fra_tokens(tokens_jmp);
    nl_assert_ne_text(c2b, "");
    nl_list_text* tokens_cmp_jz = nl_list_text_new();
    nl_list_text_push(tokens_cmp_jz, "PUSH");
    nl_list_text_push(tokens_cmp_jz, "4");
    nl_list_text_push(tokens_cmp_jz, "PUSH");
    nl_list_text_push(tokens_cmp_jz, "4");
    nl_list_text_push(tokens_cmp_jz, "EQ");
    nl_list_text_push(tokens_cmp_jz, "JZ");
    nl_list_text_push(tokens_cmp_jz, "2");
    nl_list_text_push(tokens_cmp_jz, "PUSH");
    nl_list_text_push(tokens_cmp_jz, "1");
    nl_list_text_push(tokens_cmp_jz, "PRINT");
    nl_list_text_push(tokens_cmp_jz, "LABEL");
    nl_list_text_push(tokens_cmp_jz, "2");
    nl_list_text_push(tokens_cmp_jz, "HALT");
    char * c2c = selfhost__compiler__kompiler_fra_tokens(tokens_cmp_jz);
    nl_assert_ne_text(c2c, "");
    nl_list_text* tokens_stack = nl_list_text_new();
    nl_list_text_push(tokens_stack, "PUSH");
    nl_list_text_push(tokens_stack, "5");
    nl_list_text_push(tokens_stack, "DUP");
    nl_list_text_push(tokens_stack, "ADD");
    nl_list_text_push(tokens_stack, "POP");
    nl_list_text_push(tokens_stack, "HALT");
    char * c2d = selfhost__compiler__kompiler_fra_tokens(tokens_stack);
    nl_assert_ne_text(c2d, "");
    nl_list_text* tokens_stack2 = nl_list_text_new();
    nl_list_text_push(tokens_stack2, "PUSH");
    nl_list_text_push(tokens_stack2, "2");
    nl_list_text_push(tokens_stack2, "PUSH");
    nl_list_text_push(tokens_stack2, "9");
    nl_list_text_push(tokens_stack2, "SWAP");
    nl_list_text_push(tokens_stack2, "OVER");
    nl_list_text_push(tokens_stack2, "ADD");
    nl_list_text_push(tokens_stack2, "HALT");
    char * c2e = selfhost__compiler__kompiler_fra_tokens(tokens_stack2);
    nl_assert_ne_text(c2e, "");
    nl_list_text* tokens_mod = nl_list_text_new();
    nl_list_text_push(tokens_mod, "PUSH");
    nl_list_text_push(tokens_mod, "17");
    nl_list_text_push(tokens_mod, "PUSH");
    nl_list_text_push(tokens_mod, "5");
    nl_list_text_push(tokens_mod, "MOD");
    nl_list_text_push(tokens_mod, "PRINT");
    nl_list_text_push(tokens_mod, "HALT");
    char * c2f = selfhost__compiler__kompiler_fra_tokens(tokens_mod);
    nl_assert_ne_text(c2f, "");
    nl_list_text* tokens_bool = nl_list_text_new();
    nl_list_text_push(tokens_bool, "PUSH");
    nl_list_text_push(tokens_bool, "1");
    nl_list_text_push(tokens_bool, "PUSH");
    nl_list_text_push(tokens_bool, "0");
    nl_list_text_push(tokens_bool, "AND");
    nl_list_text_push(tokens_bool, "NOT");
    nl_list_text_push(tokens_bool, "PRINT");
    nl_list_text_push(tokens_bool, "HALT");
    char * c2g = selfhost__compiler__kompiler_fra_tokens(tokens_bool);
    nl_assert_ne_text(c2g, "");
    nl_list_text* tokens_mem = nl_list_text_new();
    nl_list_text_push(tokens_mem, "PUSH");
    nl_list_text_push(tokens_mem, "42");
    nl_list_text_push(tokens_mem, "STORE");
    nl_list_text_push(tokens_mem, "0");
    nl_list_text_push(tokens_mem, "LOAD");
    nl_list_text_push(tokens_mem, "0");
    nl_list_text_push(tokens_mem, "PUSH");
    nl_list_text_push(tokens_mem, "1");
    nl_list_text_push(tokens_mem, "ADD");
    nl_list_text_push(tokens_mem, "PRINT");
    nl_list_text_push(tokens_mem, "HALT");
    char * c2h = selfhost__compiler__kompiler_fra_tokens(tokens_mem);
    nl_assert_ne_text(c2h, "");
    nl_list_text* tokens_call = nl_list_text_new();
    nl_list_text_push(tokens_call, "CALL");
    nl_list_text_push(tokens_call, "3");
    nl_list_text_push(tokens_call, "HALT");
    nl_list_text_push(tokens_call, "LABEL");
    nl_list_text_push(tokens_call, "3");
    nl_list_text_push(tokens_call, "PUSH");
    nl_list_text_push(tokens_call, "99");
    nl_list_text_push(tokens_call, "RET");
    char * c2i = selfhost__compiler__kompiler_fra_tokens(tokens_call);
    nl_assert_ne_text(c2i, "");
    nl_list_text* tokens_bad_jump = nl_list_text_new();
    nl_list_text_push(tokens_bad_jump, "JMP");
    nl_list_text_push(tokens_bad_jump, "9");
    nl_list_text_push(tokens_bad_jump, "HALT");
    char * c_bad = selfhost__compiler__kompiler_fra_tokens(tokens_bad_jump);
    nl_assert_eq_text(c_bad, "/* feil: ugyldig hopp-target 9 */");
    nl_list_text* tokens_underflow = nl_list_text_new();
    nl_list_text_push(tokens_underflow, "ADD");
    nl_list_text_push(tokens_underflow, "HALT");
    char * c_underflow = selfhost__compiler__kompiler_fra_tokens(tokens_underflow);
    nl_assert_eq_text(c_underflow, "/* feil: stack-underflow ved indeks 0 (ADD) */");
    nl_list_text* tokens_bad_mem = nl_list_text_new();
    nl_list_text_push(tokens_bad_mem, "PUSH");
    nl_list_text_push(tokens_bad_mem, "1");
    nl_list_text_push(tokens_bad_mem, "STORE");
    nl_list_text_push(tokens_bad_mem, "999");
    nl_list_text_push(tokens_bad_mem, "HALT");
    char * c_bad_mem = selfhost__compiler__kompiler_fra_tokens(tokens_bad_mem);
    nl_assert_eq_text(c_bad_mem, "/* feil: minneindeks utenfor range 999 */");
    nl_list_text* tokens_dis = nl_list_text_new();
    nl_list_text_push(tokens_dis, "PUSH");
    nl_list_text_push(tokens_dis, "3");
    nl_list_text_push(tokens_dis, "PUSH");
    nl_list_text_push(tokens_dis, "4");
    nl_list_text_push(tokens_dis, "ADD");
    nl_list_text_push(tokens_dis, "HALT");
    char * dis = selfhost__compiler__disasm_fra_tokens(tokens_dis);
    nl_assert_eq_text(dis, "0: PUSH 3\n1: PUSH 4\n2: ADD\n3: HALT\n");
    char * dis_src = selfhost__compiler__disasm_fra_kilde("# c\nPUSH 3;\nPUSH 4\nADD\nHALT");
    nl_assert_eq_text(dis_src, "0: PUSH 3\n1: PUSH 4\n2: ADD\n3: HALT\n");
    char * dis_src_strict = selfhost__compiler__disasm_fra_kilde_strict("PUSH 3\nPUSH 4\nADD\nHALT");
    nl_assert_eq_text(dis_src_strict, "0: PUSH 3\n1: PUSH 4\n2: ADD\n3: HALT\n");
    char * dis_src_bad = selfhost__compiler__disasm_fra_kilde_strict("PUSH x\nHALT");
    nl_assert_eq_text(dis_src_bad, "/* feil: ugyldig heltallsargument x ved token 1 */");
    char * dis_src_bad_opcode = selfhost__compiler__disasm_fra_kilde_strict("PUHS 1\nHALT");
    nl_assert_eq_text(dis_src_bad_opcode, "/* feil: ukjent opcode PUHS ved token 0 */");
    char * c_src_bad = selfhost__compiler__kompiler_fra_kilde_strict("PUSH 2\nPUSH q\nADD\nHALT");
    nl_assert_eq_text(c_src_bad, "/* feil: ugyldig heltallsargument q ved token 3 */");
    char * expr_dis = selfhost__compiler__disasm_uttrykk("2+3*4");
    nl_assert_eq_text(expr_dis, "0: PUSH 2\n1: PUSH 3\n2: PUSH 4\n3: MUL\n4: ADD\n5: PRINT\n6: HALT\n");
    char * expr_dis_paren = selfhost__compiler__disasm_uttrykk("(2+3)*4");
    nl_assert_eq_text(expr_dis_paren, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PUSH 4\n4: MUL\n5: PRINT\n6: HALT\n");
    char * expr_dis_brace_group = selfhost__compiler__disasm_uttrykk("{2+3}*4");
    nl_assert_eq_text(expr_dis_brace_group, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PUSH 4\n4: MUL\n5: PRINT\n6: HALT\n");
    char * expr_dis_bracket_group = selfhost__compiler__disasm_uttrykk("[2+3]*4");
    nl_assert_eq_text(expr_dis_bracket_group, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PUSH 4\n4: MUL\n5: PRINT\n6: HALT\n");
    char * expr_c = selfhost__compiler__kompiler_uttrykk_til_c("10 + 32");
    nl_assert_ne_text(expr_c, "");
    char * expr_cmp = selfhost__compiler__disasm_uttrykk("5>3&&2<4");
    nl_assert_eq_text(expr_cmp, "0: PUSH 5\n1: PUSH 3\n2: GT\n3: PUSH 2\n4: PUSH 4\n5: LT\n6: AND\n7: PRINT\n8: HALT\n");
    char * expr_eq = selfhost__compiler__disasm_uttrykk("7!=7||2==2");
    nl_assert_eq_text(expr_eq, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: NOT\n4: PUSH 2\n5: PUSH 2\n6: EQ\n7: OR\n8: PRINT\n9: HALT\n");
    char * expr_not_equal_angle_alias = selfhost__compiler__disasm_uttrykk("7<>8");
    nl_assert_eq_text(expr_not_equal_angle_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_xor_alias = selfhost__compiler__disasm_uttrykk("1 xor 0");
    nl_assert_eq_text(expr_xor_alias, "0: PUSH 1\n1: PUSH 0\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_xnor_alias = selfhost__compiler__disasm_uttrykk("1 xnor 1");
    nl_assert_eq_text(expr_xnor_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_norsk_xeller_ikke_alias = selfhost__compiler__disasm_uttrykk("1 xeller_ikke 1");
    nl_assert_eq_text(expr_norsk_xeller_ikke_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_nand_operator = selfhost__compiler__disasm_uttrykk("1 nand 1");
    nl_assert_eq_text(expr_nand_operator, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_nand_alias_and_not = selfhost__compiler__disasm_uttrykk("1 and_not 1");
    nl_assert_eq_text(expr_nand_alias_and_not, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_nand_alias_og_ikke = selfhost__compiler__disasm_uttrykk("1 og_ikke 1");
    nl_assert_eq_text(expr_nand_alias_og_ikke, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_nor_operator = selfhost__compiler__disasm_uttrykk("0 nor 0");
    nl_assert_eq_text(expr_nor_operator, "0: PUSH 0\n1: PUSH 0\n2: OR\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_nor_alias_or_not = selfhost__compiler__disasm_uttrykk("0 or_not 0");
    nl_assert_eq_text(expr_nor_alias_or_not, "0: PUSH 0\n1: PUSH 0\n2: OR\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_nor_alias_eller_ikke = selfhost__compiler__disasm_uttrykk("0 eller_ikke 0");
    nl_assert_eq_text(expr_nor_alias_eller_ikke, "0: PUSH 0\n1: PUSH 0\n2: OR\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_implies_operator = selfhost__compiler__disasm_uttrykk("1 implies 0");
    nl_assert_eq_text(expr_implies_operator, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_implies_that_alias = selfhost__compiler__disasm_uttrykk("1 implies_that 0");
    nl_assert_eq_text(expr_implies_that_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_this_implies_alias = selfhost__compiler__disasm_uttrykk("1 this_implies 0");
    nl_assert_eq_text(expr_this_implies_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_dette_impliserer_alias = selfhost__compiler__disasm_uttrykk("1 dette_impliserer 0");
    nl_assert_eq_text(expr_dette_impliserer_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_impliserer_at_alias = selfhost__compiler__disasm_uttrykk("1 impliserer_at 0");
    nl_assert_eq_text(expr_impliserer_at_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_norsk_impliserer_operator = selfhost__compiler__disasm_uttrykk("1 impliserer 0");
    nl_assert_eq_text(expr_norsk_impliserer_operator, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_norsk_medforer_alias = selfhost__compiler__disasm_uttrykk("1 medforer 0");
    nl_assert_eq_text(expr_norsk_medforer_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_impl_alias = selfhost__compiler__disasm_uttrykk("1 impl 0");
    nl_assert_eq_text(expr_impl_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_impliser_alias = selfhost__compiler__disasm_uttrykk("1 impliser 0");
    nl_assert_eq_text(expr_impliser_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_follows_alias = selfhost__compiler__disasm_uttrykk("1 follows 0");
    nl_assert_eq_text(expr_follows_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_folger_av_alias = selfhost__compiler__disasm_uttrykk("1 folger_av 0");
    nl_assert_eq_text(expr_folger_av_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_it_follows_that_alias = selfhost__compiler__disasm_uttrykk("1 it_follows_that 0");
    nl_assert_eq_text(expr_it_follows_that_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_det_folger_at_alias = selfhost__compiler__disasm_uttrykk("1 det_folger_at 0");
    nl_assert_eq_text(expr_det_folger_at_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_therefore_alias = selfhost__compiler__disasm_uttrykk("1 therefore 0");
    nl_assert_eq_text(expr_therefore_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_derfor_alias = selfhost__compiler__disasm_uttrykk("1 derfor 0");
    nl_assert_eq_text(expr_derfor_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_hence_alias = selfhost__compiler__disasm_uttrykk("1 hence 0");
    nl_assert_eq_text(expr_hence_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_altsa_alias = selfhost__compiler__disasm_uttrykk("1 altsa 0");
    nl_assert_eq_text(expr_altsa_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_thus_alias = selfhost__compiler__disasm_uttrykk("1 thus 0");
    nl_assert_eq_text(expr_thus_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_dermed_alias = selfhost__compiler__disasm_uttrykk("1 dermed 0");
    nl_assert_eq_text(expr_dermed_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_consequently_alias = selfhost__compiler__disasm_uttrykk("1 consequently 0");
    nl_assert_eq_text(expr_consequently_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_folgelig_alias = selfhost__compiler__disasm_uttrykk("1 folgelig 0");
    nl_assert_eq_text(expr_folgelig_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_thereby_alias = selfhost__compiler__disasm_uttrykk("1 thereby 0");
    nl_assert_eq_text(expr_thereby_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_derved_alias = selfhost__compiler__disasm_uttrykk("1 derved 0");
    nl_assert_eq_text(expr_derved_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_thereupon_alias = selfhost__compiler__disasm_uttrykk("1 thereupon 0");
    nl_assert_eq_text(expr_thereupon_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_derpa_alias = selfhost__compiler__disasm_uttrykk("1 derpa 0");
    nl_assert_eq_text(expr_derpa_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_ergo_alias = selfhost__compiler__disasm_uttrykk("1 ergo 0");
    nl_assert_eq_text(expr_ergo_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_saledes_alias = selfhost__compiler__disasm_uttrykk("1 saledes 0");
    nl_assert_eq_text(expr_saledes_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_infer_alias = selfhost__compiler__disasm_uttrykk("1 infer 0");
    nl_assert_eq_text(expr_infer_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_derav_alias = selfhost__compiler__disasm_uttrykk("1 derav 0");
    nl_assert_eq_text(expr_derav_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_as_consequence_alias = selfhost__compiler__disasm_uttrykk("1 as_consequence 0");
    nl_assert_eq_text(expr_as_consequence_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_som_konsekvens_alias = selfhost__compiler__disasm_uttrykk("1 som_konsekvens 0");
    nl_assert_eq_text(expr_som_konsekvens_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_as_a_result_alias = selfhost__compiler__disasm_uttrykk("1 as_a_result 0");
    nl_assert_eq_text(expr_as_a_result_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_som_resultat_alias = selfhost__compiler__disasm_uttrykk("1 som_resultat 0");
    nl_assert_eq_text(expr_som_resultat_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_only_if_alias = selfhost__compiler__disasm_uttrykk("1 only_if 0");
    nl_assert_eq_text(expr_only_if_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_kun_hvis_alias = selfhost__compiler__disasm_uttrykk("1 kun_hvis 0");
    nl_assert_eq_text(expr_kun_hvis_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_requires_alias = selfhost__compiler__disasm_uttrykk("1 requires 0");
    nl_assert_eq_text(expr_requires_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_krever_alias = selfhost__compiler__disasm_uttrykk("1 krever 0");
    nl_assert_eq_text(expr_krever_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_arrow_implies_operator = selfhost__compiler__disasm_uttrykk("1 -> 0");
    nl_assert_eq_text(expr_arrow_implies_operator, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_fat_arrow_implies_operator = selfhost__compiler__disasm_uttrykk("1 => 0");
    nl_assert_eq_text(expr_fat_arrow_implies_operator, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_left_arrow_implied_by_operator = selfhost__compiler__disasm_uttrykk("1 <- 0");
    nl_assert_eq_text(expr_left_arrow_implied_by_operator, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_implied_by_alias = selfhost__compiler__disasm_uttrykk("1 implied_by 0");
    nl_assert_eq_text(expr_implied_by_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_folger_fra_alias = selfhost__compiler__disasm_uttrykk("1 folger_fra 0");
    nl_assert_eq_text(expr_folger_fra_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_given_alias = selfhost__compiler__disasm_uttrykk("1 given 0");
    nl_assert_eq_text(expr_given_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_gitt_alias = selfhost__compiler__disasm_uttrykk("1 gitt 0");
    nl_assert_eq_text(expr_gitt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_provided_alias = selfhost__compiler__disasm_uttrykk("1 provided 0");
    nl_assert_eq_text(expr_provided_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_forutsatt_alias = selfhost__compiler__disasm_uttrykk("1 forutsatt 0");
    nl_assert_eq_text(expr_forutsatt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_assuming_alias = selfhost__compiler__disasm_uttrykk("1 assuming 0");
    nl_assert_eq_text(expr_assuming_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_antar_alias = selfhost__compiler__disasm_uttrykk("1 antar 0");
    nl_assert_eq_text(expr_antar_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_assuming_that_alias = selfhost__compiler__disasm_uttrykk("1 assuming_that 0");
    nl_assert_eq_text(expr_assuming_that_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_antar_at_alias = selfhost__compiler__disasm_uttrykk("1 antar_at 0");
    nl_assert_eq_text(expr_antar_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_when_assuming_alias = selfhost__compiler__disasm_uttrykk("1 when_assuming 0");
    nl_assert_eq_text(expr_when_assuming_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_nar_antatt_alias = selfhost__compiler__disasm_uttrykk("1 nar_antatt 0");
    nl_assert_eq_text(expr_nar_antatt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_presuming_alias = selfhost__compiler__disasm_uttrykk("1 presuming 0");
    nl_assert_eq_text(expr_presuming_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_forutsattvis_alias = selfhost__compiler__disasm_uttrykk("1 forutsattvis 0");
    nl_assert_eq_text(expr_forutsattvis_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_since_alias = selfhost__compiler__disasm_uttrykk("1 since 0");
    nl_assert_eq_text(expr_since_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_siden_alias = selfhost__compiler__disasm_uttrykk("1 siden 0");
    nl_assert_eq_text(expr_siden_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_because_alias = selfhost__compiler__disasm_uttrykk("1 because 0");
    nl_assert_eq_text(expr_because_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_fordi_alias = selfhost__compiler__disasm_uttrykk("1 fordi 0");
    nl_assert_eq_text(expr_fordi_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_given_that_alias = selfhost__compiler__disasm_uttrykk("1 given_that 0");
    nl_assert_eq_text(expr_given_that_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_gitt_at_alias = selfhost__compiler__disasm_uttrykk("1 gitt_at 0");
    nl_assert_eq_text(expr_gitt_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_provided_that_alias = selfhost__compiler__disasm_uttrykk("1 provided_that 0");
    nl_assert_eq_text(expr_provided_that_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_forutsatt_at_alias = selfhost__compiler__disasm_uttrykk("1 forutsatt_at 0");
    nl_assert_eq_text(expr_forutsatt_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_follows_if_alias = selfhost__compiler__disasm_uttrykk("1 follows_if 0");
    nl_assert_eq_text(expr_follows_if_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_folger_hvis_alias = selfhost__compiler__disasm_uttrykk("1 folger_hvis 0");
    nl_assert_eq_text(expr_folger_hvis_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_if_given_alias = selfhost__compiler__disasm_uttrykk("1 if_given 0");
    nl_assert_eq_text(expr_if_given_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_hvis_gitt_alias = selfhost__compiler__disasm_uttrykk("1 hvis_gitt 0");
    nl_assert_eq_text(expr_hvis_gitt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_as_alias = selfhost__compiler__disasm_uttrykk("1 as 0");
    nl_assert_eq_text(expr_as_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_tersom_alias = selfhost__compiler__disasm_uttrykk("1 ettersom 0");
    nl_assert_eq_text(expr_tersom_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_inasmuch_as_alias = selfhost__compiler__disasm_uttrykk("1 inasmuch_as 0");
    nl_assert_eq_text(expr_inasmuch_as_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_i_og_med_at_alias = selfhost__compiler__disasm_uttrykk("1 i_og_med_at 0");
    nl_assert_eq_text(expr_i_og_med_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_on_condition_that_alias = selfhost__compiler__disasm_uttrykk("1 on_condition_that 0");
    nl_assert_eq_text(expr_on_condition_that_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_pa_vilkar_av_at_alias = selfhost__compiler__disasm_uttrykk("1 pa_vilkar_av_at 0");
    nl_assert_eq_text(expr_pa_vilkar_av_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_granted_alias = selfhost__compiler__disasm_uttrykk("1 granted 0");
    nl_assert_eq_text(expr_granted_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_gitt_dette_alias = selfhost__compiler__disasm_uttrykk("1 gitt_dette 0");
    nl_assert_eq_text(expr_gitt_dette_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_with_premise_alias = selfhost__compiler__disasm_uttrykk("1 with_premise 0");
    nl_assert_eq_text(expr_with_premise_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_med_premiss_alias = selfhost__compiler__disasm_uttrykk("1 med_premiss 0");
    nl_assert_eq_text(expr_med_premiss_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_given_premise_alias = selfhost__compiler__disasm_uttrykk("1 given_premise 0");
    nl_assert_eq_text(expr_given_premise_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_gitt_premiss_alias = selfhost__compiler__disasm_uttrykk("1 gitt_premiss 0");
    nl_assert_eq_text(expr_gitt_premiss_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_premise_given_alias = selfhost__compiler__disasm_uttrykk("1 premise_given 0");
    nl_assert_eq_text(expr_premise_given_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_premiss_gitt_alias = selfhost__compiler__disasm_uttrykk("1 premiss_gitt 0");
    nl_assert_eq_text(expr_premiss_gitt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_premise_assumed_alias = selfhost__compiler__disasm_uttrykk("1 premise_assumed 0");
    nl_assert_eq_text(expr_premise_assumed_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_premiss_antatt_alias = selfhost__compiler__disasm_uttrykk("1 premiss_antatt 0");
    nl_assert_eq_text(expr_premiss_antatt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_premise_condition_alias = selfhost__compiler__disasm_uttrykk("1 premise_condition 0");
    nl_assert_eq_text(expr_premise_condition_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_premiss_vilkar_alias = selfhost__compiler__disasm_uttrykk("1 premiss_vilkar 0");
    nl_assert_eq_text(expr_premiss_vilkar_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_iff_alias = selfhost__compiler__disasm_uttrykk("1 iff 1");
    nl_assert_eq_text(expr_iff_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_equiv_alias = selfhost__compiler__disasm_uttrykk("1 equiv 1");
    nl_assert_eq_text(expr_equiv_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_ekvivalent_alias = selfhost__compiler__disasm_uttrykk("1 ekvivalent 1");
    nl_assert_eq_text(expr_ekvivalent_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_if_and_only_if_alias = selfhost__compiler__disasm_uttrykk("1 if_and_only_if 1");
    nl_assert_eq_text(expr_if_and_only_if_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_hvis_og_bare_hvis_alias = selfhost__compiler__disasm_uttrykk("1 hvis_og_bare_hvis 1");
    nl_assert_eq_text(expr_hvis_og_bare_hvis_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_equiv_arrow_operator = selfhost__compiler__disasm_uttrykk("1 <-> 1");
    nl_assert_eq_text(expr_equiv_arrow_operator, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_equiv_fat_arrow_operator = selfhost__compiler__disasm_uttrykk("1 <=> 1");
    nl_assert_eq_text(expr_equiv_fat_arrow_operator, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_le = selfhost__compiler__disasm_uttrykk("3 <= 4");
    nl_assert_eq_text(expr_le, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_ge = selfhost__compiler__disasm_uttrykk("4 >= 3");
    nl_assert_eq_text(expr_ge, "0: PUSH 4\n1: PUSH 3\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_unary_not = selfhost__compiler__disasm_uttrykk("!0||0");
    nl_assert_eq_text(expr_unary_not, "0: PUSH 0\n1: NOT\n2: PUSH 0\n3: OR\n4: PRINT\n5: HALT\n");
    char * expr_unary_minus = selfhost__compiler__disasm_uttrykk("-3+5");
    nl_assert_eq_text(expr_unary_minus, "0: PUSH 3\n1: PUSH 0\n2: SWAP\n3: SUB\n4: PUSH 5\n5: ADD\n6: PRINT\n7: HALT\n");
    char * expr_norsk_minus_med = selfhost__compiler__disasm_uttrykk("10 minus med 3");
    nl_assert_eq_text(expr_norsk_minus_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minus_med_underscore = selfhost__compiler__disasm_uttrykk("10 minus_med 3");
    nl_assert_eq_text(expr_norsk_minus_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minusmed_kompakt = selfhost__compiler__disasm_uttrykk("10 minusmed 3");
    nl_assert_eq_text(expr_norsk_minusmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minuseres_med = selfhost__compiler__disasm_uttrykk("10 minuseres med 3");
    nl_assert_eq_text(expr_norsk_minuseres_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minuseres_med_underscore = selfhost__compiler__disasm_uttrykk("10 minuseres_med 3");
    nl_assert_eq_text(expr_norsk_minuseres_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minuseresmed_kompakt = selfhost__compiler__disasm_uttrykk("10 minuseresmed 3");
    nl_assert_eq_text(expr_norsk_minuseresmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekk_med = selfhost__compiler__disasm_uttrykk("10 trekk med 3");
    nl_assert_eq_text(expr_norsk_trekk_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekk_med_underscore = selfhost__compiler__disasm_uttrykk("10 trekk_med 3");
    nl_assert_eq_text(expr_norsk_trekk_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkmed_kompakt = selfhost__compiler__disasm_uttrykk("10 trekkmed 3");
    nl_assert_eq_text(expr_norsk_trekkmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekke_med = selfhost__compiler__disasm_uttrykk("10 trekke med 3");
    nl_assert_eq_text(expr_norsk_trekke_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekke_med_underscore = selfhost__compiler__disasm_uttrykk("10 trekke_med 3");
    nl_assert_eq_text(expr_norsk_trekke_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkemed_kompakt = selfhost__compiler__disasm_uttrykk("10 trekkemed 3");
    nl_assert_eq_text(expr_norsk_trekkemed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkes_med = selfhost__compiler__disasm_uttrykk("10 trekkes med 3");
    nl_assert_eq_text(expr_norsk_trekkes_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkes_med_underscore = selfhost__compiler__disasm_uttrykk("10 trekkes_med 3");
    nl_assert_eq_text(expr_norsk_trekkes_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkesmed_kompakt = selfhost__compiler__disasm_uttrykk("10 trekkesmed 3");
    nl_assert_eq_text(expr_norsk_trekkesmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraher_med = selfhost__compiler__disasm_uttrykk("10 subtraher med 3");
    nl_assert_eq_text(expr_norsk_subtraher_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraher_med_underscore = selfhost__compiler__disasm_uttrykk("10 subtraher_med 3");
    nl_assert_eq_text(expr_norsk_subtraher_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtrahermed_kompakt = selfhost__compiler__disasm_uttrykk("10 subtrahermed 3");
    nl_assert_eq_text(expr_norsk_subtrahermed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraherer_med = selfhost__compiler__disasm_uttrykk("10 subtraherer med 3");
    nl_assert_eq_text(expr_norsk_subtraherer_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraherer_med_underscore = selfhost__compiler__disasm_uttrykk("10 subtraherer_med 3");
    nl_assert_eq_text(expr_norsk_subtraherer_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraherermed_kompakt = selfhost__compiler__disasm_uttrykk("10 subtraherermed 3");
    nl_assert_eq_text(expr_norsk_subtraherermed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraheres_med = selfhost__compiler__disasm_uttrykk("10 subtraheres med 3");
    nl_assert_eq_text(expr_norsk_subtraheres_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraheres_med_underscore = selfhost__compiler__disasm_uttrykk("10 subtraheres_med 3");
    nl_assert_eq_text(expr_norsk_subtraheres_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraheresmed_kompakt = selfhost__compiler__disasm_uttrykk("10 subtraheresmed 3");
    nl_assert_eq_text(expr_norsk_subtraheresmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtrahert_med = selfhost__compiler__disasm_uttrykk("10 subtrahert med 3");
    nl_assert_eq_text(expr_norsk_subtrahert_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtrahert_med_underscore = selfhost__compiler__disasm_uttrykk("10 subtrahert_med 3");
    nl_assert_eq_text(expr_norsk_subtrahert_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtrahertmed_kompakt = selfhost__compiler__disasm_uttrykk("10 subtrahertmed 3");
    nl_assert_eq_text(expr_norsk_subtrahertmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekk = selfhost__compiler__disasm_uttrykk("10 trekk 3");
    nl_assert_eq_text(expr_norsk_trekk, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekke = selfhost__compiler__disasm_uttrykk("10 trekke 3");
    nl_assert_eq_text(expr_norsk_trekke, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraher = selfhost__compiler__disasm_uttrykk("10 subtraher 3");
    nl_assert_eq_text(expr_norsk_subtraher, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtrahert = selfhost__compiler__disasm_uttrykk("10 subtrahert 3");
    nl_assert_eq_text(expr_norsk_subtrahert, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_unary_plus = selfhost__compiler__disasm_uttrykk("+3+5");
    nl_assert_eq_text(expr_unary_plus, "0: PUSH 3\n1: PUSH 5\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_arith = selfhost__compiler__disasm_uttrykk("2 pluss 3 ganger 4");
    nl_assert_eq_text(expr_norsk_arith, "0: PUSH 2\n1: PUSH 3\n2: PUSH 4\n3: MUL\n4: ADD\n5: PRINT\n6: HALT\n");
    char * expr_norsk_legg_til = selfhost__compiler__disasm_uttrykk("2 legg til 3");
    nl_assert_eq_text(expr_norsk_legg_til, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legg_til_underscore = selfhost__compiler__disasm_uttrykk("2 legg_til 3");
    nl_assert_eq_text(expr_norsk_legg_til_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legge_til = selfhost__compiler__disasm_uttrykk("2 legge til 3");
    nl_assert_eq_text(expr_norsk_legge_til, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legge_til_underscore = selfhost__compiler__disasm_uttrykk("2 legge_til 3");
    nl_assert_eq_text(expr_norsk_legge_til_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legges_til = selfhost__compiler__disasm_uttrykk("2 legges til 3");
    nl_assert_eq_text(expr_norsk_legges_til, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legges_til_underscore = selfhost__compiler__disasm_uttrykk("2 legges_til 3");
    nl_assert_eq_text(expr_norsk_legges_til_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_leggtil_kompakt = selfhost__compiler__disasm_uttrykk("2 leggtil 3");
    nl_assert_eq_text(expr_norsk_leggtil_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_leggetil_kompakt = selfhost__compiler__disasm_uttrykk("2 leggetil 3");
    nl_assert_eq_text(expr_norsk_leggetil_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_leggestil_kompakt = selfhost__compiler__disasm_uttrykk("2 leggestil 3");
    nl_assert_eq_text(expr_norsk_leggestil_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legg_sammen = selfhost__compiler__disasm_uttrykk("2 legg sammen 3");
    nl_assert_eq_text(expr_norsk_legg_sammen, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legg_sammen_underscore = selfhost__compiler__disasm_uttrykk("2 legg_sammen 3");
    nl_assert_eq_text(expr_norsk_legg_sammen_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legge_sammen = selfhost__compiler__disasm_uttrykk("2 legge sammen 3");
    nl_assert_eq_text(expr_norsk_legge_sammen, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legge_sammen_underscore = selfhost__compiler__disasm_uttrykk("2 legge_sammen 3");
    nl_assert_eq_text(expr_norsk_legge_sammen_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legges_sammen = selfhost__compiler__disasm_uttrykk("2 legges sammen 3");
    nl_assert_eq_text(expr_norsk_legges_sammen, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legges_sammen_underscore = selfhost__compiler__disasm_uttrykk("2 legges_sammen 3");
    nl_assert_eq_text(expr_norsk_legges_sammen_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_leggsammen_kompakt = selfhost__compiler__disasm_uttrykk("2 leggsammen 3");
    nl_assert_eq_text(expr_norsk_leggsammen_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_leggesammen_kompakt = selfhost__compiler__disasm_uttrykk("2 leggesammen 3");
    nl_assert_eq_text(expr_norsk_leggesammen_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_leggessammen_kompakt = selfhost__compiler__disasm_uttrykk("2 leggessammen 3");
    nl_assert_eq_text(expr_norsk_leggessammen_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legg_kort = selfhost__compiler__disasm_uttrykk("2 legg 3");
    nl_assert_eq_text(expr_norsk_legg_kort, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legge_kort = selfhost__compiler__disasm_uttrykk("2 legge 3");
    nl_assert_eq_text(expr_norsk_legge_kort, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_legges_kort = selfhost__compiler__disasm_uttrykk("2 legges 3");
    nl_assert_eq_text(expr_norsk_legges_kort, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_pluss_med = selfhost__compiler__disasm_uttrykk("2 pluss med 3");
    nl_assert_eq_text(expr_norsk_pluss_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_pluss_med_underscore = selfhost__compiler__disasm_uttrykk("2 pluss_med 3");
    nl_assert_eq_text(expr_norsk_pluss_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plussmed_kompakt = selfhost__compiler__disasm_uttrykk("2 plussmed 3");
    nl_assert_eq_text(expr_norsk_plussmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plus = selfhost__compiler__disasm_uttrykk("2 plus 3");
    nl_assert_eq_text(expr_norsk_plus, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plus_med = selfhost__compiler__disasm_uttrykk("2 plus med 3");
    nl_assert_eq_text(expr_norsk_plus_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plus_med_underscore = selfhost__compiler__disasm_uttrykk("2 plus_med 3");
    nl_assert_eq_text(expr_norsk_plus_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plusmed_kompakt = selfhost__compiler__disasm_uttrykk("2 plusmed 3");
    nl_assert_eq_text(expr_norsk_plusmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plusser_med = selfhost__compiler__disasm_uttrykk("2 plusser med 3");
    nl_assert_eq_text(expr_norsk_plusser_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plusser_med_underscore = selfhost__compiler__disasm_uttrykk("2 plusser_med 3");
    nl_assert_eq_text(expr_norsk_plusser_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plussermed_kompakt = selfhost__compiler__disasm_uttrykk("2 plussermed 3");
    nl_assert_eq_text(expr_norsk_plussermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plusses_med = selfhost__compiler__disasm_uttrykk("2 plusses med 3");
    nl_assert_eq_text(expr_norsk_plusses_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plusses_med_underscore = selfhost__compiler__disasm_uttrykk("2 plusses_med 3");
    nl_assert_eq_text(expr_norsk_plusses_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plussesmed_kompakt = selfhost__compiler__disasm_uttrykk("2 plussesmed 3");
    nl_assert_eq_text(expr_norsk_plussesmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_pluses_med = selfhost__compiler__disasm_uttrykk("2 pluses med 3");
    nl_assert_eq_text(expr_norsk_pluses_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_pluses_med_underscore = selfhost__compiler__disasm_uttrykk("2 pluses_med 3");
    nl_assert_eq_text(expr_norsk_pluses_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plusesmed_kompakt = selfhost__compiler__disasm_uttrykk("2 plusesmed 3");
    nl_assert_eq_text(expr_norsk_plusesmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plusse_med = selfhost__compiler__disasm_uttrykk("2 plusse med 3");
    nl_assert_eq_text(expr_norsk_plusse_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plusse_med_underscore = selfhost__compiler__disasm_uttrykk("2 plusse_med 3");
    nl_assert_eq_text(expr_norsk_plusse_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plussemed_kompakt = selfhost__compiler__disasm_uttrykk("2 plussemed 3");
    nl_assert_eq_text(expr_norsk_plussemed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_plusser = selfhost__compiler__disasm_uttrykk("2 plusser 3");
    nl_assert_eq_text(expr_norsk_plusser, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adder_med = selfhost__compiler__disasm_uttrykk("2 adder med 3");
    nl_assert_eq_text(expr_norsk_adder_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adder_med_underscore = selfhost__compiler__disasm_uttrykk("2 adder_med 3");
    nl_assert_eq_text(expr_norsk_adder_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_addermed_kompakt = selfhost__compiler__disasm_uttrykk("2 addermed 3");
    nl_assert_eq_text(expr_norsk_addermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_addere_med = selfhost__compiler__disasm_uttrykk("2 addere med 3");
    nl_assert_eq_text(expr_norsk_addere_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_addere_med_underscore = selfhost__compiler__disasm_uttrykk("2 addere_med 3");
    nl_assert_eq_text(expr_norsk_addere_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adderemed_kompakt = selfhost__compiler__disasm_uttrykk("2 adderemed 3");
    nl_assert_eq_text(expr_norsk_adderemed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adderer_med = selfhost__compiler__disasm_uttrykk("2 adderer med 3");
    nl_assert_eq_text(expr_norsk_adderer_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adderer_med_underscore = selfhost__compiler__disasm_uttrykk("2 adderer_med 3");
    nl_assert_eq_text(expr_norsk_adderer_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adderermed_kompakt = selfhost__compiler__disasm_uttrykk("2 adderermed 3");
    nl_assert_eq_text(expr_norsk_adderermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adderes_med = selfhost__compiler__disasm_uttrykk("2 adderes med 3");
    nl_assert_eq_text(expr_norsk_adderes_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adderes_med_underscore = selfhost__compiler__disasm_uttrykk("2 adderes_med 3");
    nl_assert_eq_text(expr_norsk_adderes_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adderesmed_kompakt = selfhost__compiler__disasm_uttrykk("2 adderesmed 3");
    nl_assert_eq_text(expr_norsk_adderesmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_adder = selfhost__compiler__disasm_uttrykk("2 adder 3");
    nl_assert_eq_text(expr_norsk_adder, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summer_med = selfhost__compiler__disasm_uttrykk("2 summer med 3");
    nl_assert_eq_text(expr_norsk_summer_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summer_med_underscore = selfhost__compiler__disasm_uttrykk("2 summer_med 3");
    nl_assert_eq_text(expr_norsk_summer_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summermed_kompakt = selfhost__compiler__disasm_uttrykk("2 summermed 3");
    nl_assert_eq_text(expr_norsk_summermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summerer_med = selfhost__compiler__disasm_uttrykk("2 summerer med 3");
    nl_assert_eq_text(expr_norsk_summerer_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summerer_med_underscore = selfhost__compiler__disasm_uttrykk("2 summerer_med 3");
    nl_assert_eq_text(expr_norsk_summerer_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summerermed_kompakt = selfhost__compiler__disasm_uttrykk("2 summerermed 3");
    nl_assert_eq_text(expr_norsk_summerermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summeres_med = selfhost__compiler__disasm_uttrykk("2 summeres med 3");
    nl_assert_eq_text(expr_norsk_summeres_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summeres_med_underscore = selfhost__compiler__disasm_uttrykk("2 summeres_med 3");
    nl_assert_eq_text(expr_norsk_summeres_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summeresmed_kompakt = selfhost__compiler__disasm_uttrykk("2 summeresmed 3");
    nl_assert_eq_text(expr_norsk_summeresmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_summer = selfhost__compiler__disasm_uttrykk("2 summer 3");
    nl_assert_eq_text(expr_norsk_summer, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekk_fra = selfhost__compiler__disasm_uttrykk("10 trekk fra 3");
    nl_assert_eq_text(expr_norsk_trekk_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekk_fra_underscore = selfhost__compiler__disasm_uttrykk("10 trekk_fra 3");
    nl_assert_eq_text(expr_norsk_trekk_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkfra_kompakt = selfhost__compiler__disasm_uttrykk("10 trekkfra 3");
    nl_assert_eq_text(expr_norsk_trekkfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekke_fra_underscore = selfhost__compiler__disasm_uttrykk("10 trekke_fra 3");
    nl_assert_eq_text(expr_norsk_trekke_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkefra_kompakt = selfhost__compiler__disasm_uttrykk("10 trekkefra 3");
    nl_assert_eq_text(expr_norsk_trekkefra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkes_fra = selfhost__compiler__disasm_uttrykk("10 trekkes fra 3");
    nl_assert_eq_text(expr_norsk_trekkes_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkes_fra_underscore = selfhost__compiler__disasm_uttrykk("10 trekkes_fra 3");
    nl_assert_eq_text(expr_norsk_trekkes_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_trekkesfra_kompakt = selfhost__compiler__disasm_uttrykk("10 trekkesfra 3");
    nl_assert_eq_text(expr_norsk_trekkesfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minus_fra = selfhost__compiler__disasm_uttrykk("10 minus fra 3");
    nl_assert_eq_text(expr_norsk_minus_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minus_fra_underscore = selfhost__compiler__disasm_uttrykk("10 minus_fra 3");
    nl_assert_eq_text(expr_norsk_minus_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minusfra_kompakt = selfhost__compiler__disasm_uttrykk("10 minusfra 3");
    nl_assert_eq_text(expr_norsk_minusfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minuseres_fra = selfhost__compiler__disasm_uttrykk("10 minuseres fra 3");
    nl_assert_eq_text(expr_norsk_minuseres_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minuseres_fra_underscore = selfhost__compiler__disasm_uttrykk("10 minuseres_fra 3");
    nl_assert_eq_text(expr_norsk_minuseres_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_minuseresfra_kompakt = selfhost__compiler__disasm_uttrykk("10 minuseresfra 3");
    nl_assert_eq_text(expr_norsk_minuseresfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraher_fra = selfhost__compiler__disasm_uttrykk("10 subtraher fra 3");
    nl_assert_eq_text(expr_norsk_subtraher_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraher_fra_underscore = selfhost__compiler__disasm_uttrykk("10 subtraher_fra 3");
    nl_assert_eq_text(expr_norsk_subtraher_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraherfra_kompakt = selfhost__compiler__disasm_uttrykk("10 subtraherfra 3");
    nl_assert_eq_text(expr_norsk_subtraherfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraherer_fra = selfhost__compiler__disasm_uttrykk("10 subtraherer fra 3");
    nl_assert_eq_text(expr_norsk_subtraherer_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraherer_fra_underscore = selfhost__compiler__disasm_uttrykk("10 subtraherer_fra 3");
    nl_assert_eq_text(expr_norsk_subtraherer_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtrahererfra_kompakt = selfhost__compiler__disasm_uttrykk("10 subtrahererfra 3");
    nl_assert_eq_text(expr_norsk_subtrahererfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraheres_fra = selfhost__compiler__disasm_uttrykk("10 subtraheres fra 3");
    nl_assert_eq_text(expr_norsk_subtraheres_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraheres_fra_underscore = selfhost__compiler__disasm_uttrykk("10 subtraheres_fra 3");
    nl_assert_eq_text(expr_norsk_subtraheres_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtraheresfra_kompakt = selfhost__compiler__disasm_uttrykk("10 subtraheresfra 3");
    nl_assert_eq_text(expr_norsk_subtraheresfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtrahert_fra = selfhost__compiler__disasm_uttrykk("10 subtrahert fra 3");
    nl_assert_eq_text(expr_norsk_subtrahert_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtrahert_fra_underscore = selfhost__compiler__disasm_uttrykk("10 subtrahert_fra 3");
    nl_assert_eq_text(expr_norsk_subtrahert_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_subtrahertfra_kompakt = selfhost__compiler__disasm_uttrykk("10 subtrahertfra 3");
    nl_assert_eq_text(expr_norsk_subtrahertfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_norsk_div = selfhost__compiler__disasm_uttrykk("8 delt pa 2");
    nl_assert_eq_text(expr_norsk_div, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_mod = selfhost__compiler__disasm_uttrykk("17 mod 5");
    nl_assert_eq_text(expr_norsk_mod, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_mod_av = selfhost__compiler__disasm_uttrykk("17 mod_av 5");
    nl_assert_eq_text(expr_norsk_mod_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modav = selfhost__compiler__disasm_uttrykk("17 modav 5");
    nl_assert_eq_text(expr_norsk_modav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_mod_av_phrase = selfhost__compiler__disasm_uttrykk("17 mod av 5");
    nl_assert_eq_text(expr_norsk_mod_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modulo = selfhost__compiler__disasm_uttrykk("17 modulo 5");
    nl_assert_eq_text(expr_norsk_modulo, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modulo_av = selfhost__compiler__disasm_uttrykk("17 modulo_av 5");
    nl_assert_eq_text(expr_norsk_modulo_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_moduloav = selfhost__compiler__disasm_uttrykk("17 moduloav 5");
    nl_assert_eq_text(expr_norsk_moduloav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modulo_av_phrase = selfhost__compiler__disasm_uttrykk("17 modulo av 5");
    nl_assert_eq_text(expr_norsk_modulo_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modul = selfhost__compiler__disasm_uttrykk("17 modul 5");
    nl_assert_eq_text(expr_norsk_modul, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modul_av = selfhost__compiler__disasm_uttrykk("17 modul_av 5");
    nl_assert_eq_text(expr_norsk_modul_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modulav = selfhost__compiler__disasm_uttrykk("17 modulav 5");
    nl_assert_eq_text(expr_norsk_modulav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modul_av_phrase = selfhost__compiler__disasm_uttrykk("17 modul av 5");
    nl_assert_eq_text(expr_norsk_modul_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modulus = selfhost__compiler__disasm_uttrykk("17 modulus 5");
    nl_assert_eq_text(expr_norsk_modulus, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modulus_av = selfhost__compiler__disasm_uttrykk("17 modulus_av 5");
    nl_assert_eq_text(expr_norsk_modulus_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modulusav = selfhost__compiler__disasm_uttrykk("17 modulusav 5");
    nl_assert_eq_text(expr_norsk_modulusav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_modulus_av_phrase = selfhost__compiler__disasm_uttrykk("17 modulus av 5");
    nl_assert_eq_text(expr_norsk_modulus_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_rest = selfhost__compiler__disasm_uttrykk("17 rest 5");
    nl_assert_eq_text(expr_norsk_rest, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_resten = selfhost__compiler__disasm_uttrykk("17 resten 5");
    nl_assert_eq_text(expr_norsk_resten, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_rest_av = selfhost__compiler__disasm_uttrykk("17 rest_av 5");
    nl_assert_eq_text(expr_norsk_rest_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_restav = selfhost__compiler__disasm_uttrykk("17 restav 5");
    nl_assert_eq_text(expr_norsk_restav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_rest_av_phrase = selfhost__compiler__disasm_uttrykk("17 rest av 5");
    nl_assert_eq_text(expr_norsk_rest_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_resten_av = selfhost__compiler__disasm_uttrykk("17 resten_av 5");
    nl_assert_eq_text(expr_norsk_resten_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_restenav = selfhost__compiler__disasm_uttrykk("17 restenav 5");
    nl_assert_eq_text(expr_norsk_restenav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_resten_av_phrase = selfhost__compiler__disasm_uttrykk("17 resten av 5");
    nl_assert_eq_text(expr_norsk_resten_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ganget_med = selfhost__compiler__disasm_uttrykk("3 ganget med 4");
    nl_assert_eq_text(expr_norsk_ganget_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ganget_med_underscore = selfhost__compiler__disasm_uttrykk("3 ganget_med 4");
    nl_assert_eq_text(expr_norsk_ganget_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gange_med = selfhost__compiler__disasm_uttrykk("3 gange med 4");
    nl_assert_eq_text(expr_norsk_gange_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gange_med_underscore = selfhost__compiler__disasm_uttrykk("3 gange_med 4");
    nl_assert_eq_text(expr_norsk_gange_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gangemed_kompakt = selfhost__compiler__disasm_uttrykk("3 gangemed 4");
    nl_assert_eq_text(expr_norsk_gangemed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ganger_med = selfhost__compiler__disasm_uttrykk("3 ganger med 4");
    nl_assert_eq_text(expr_norsk_ganger_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ganger_med_underscore = selfhost__compiler__disasm_uttrykk("3 ganger_med 4");
    nl_assert_eq_text(expr_norsk_ganger_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gangermed_kompakt = selfhost__compiler__disasm_uttrykk("3 gangermed 4");
    nl_assert_eq_text(expr_norsk_gangermed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ganges_med = selfhost__compiler__disasm_uttrykk("3 ganges med 4");
    nl_assert_eq_text(expr_norsk_ganges_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ganges_med_underscore = selfhost__compiler__disasm_uttrykk("3 ganges_med 4");
    nl_assert_eq_text(expr_norsk_ganges_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gangesmed_kompakt = selfhost__compiler__disasm_uttrykk("3 gangesmed 4");
    nl_assert_eq_text(expr_norsk_gangesmed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gang_med = selfhost__compiler__disasm_uttrykk("3 gang med 4");
    nl_assert_eq_text(expr_norsk_gang_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gang_med_underscore = selfhost__compiler__disasm_uttrykk("3 gang_med 4");
    nl_assert_eq_text(expr_norsk_gang_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gangmed_kompakt = selfhost__compiler__disasm_uttrykk("3 gangmed 4");
    nl_assert_eq_text(expr_norsk_gangmed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gang = selfhost__compiler__disasm_uttrykk("3 gang 4");
    nl_assert_eq_text(expr_norsk_gang, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_gange = selfhost__compiler__disasm_uttrykk("3 gange 4");
    nl_assert_eq_text(expr_norsk_gange, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ganget = selfhost__compiler__disasm_uttrykk("3 ganget 4");
    nl_assert_eq_text(expr_norsk_ganget, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ganger = selfhost__compiler__disasm_uttrykk("3 ganger 4");
    nl_assert_eq_text(expr_norsk_ganger, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ganges = selfhost__compiler__disasm_uttrykk("3 ganges 4");
    nl_assert_eq_text(expr_norsk_ganges, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliser_med = selfhost__compiler__disasm_uttrykk("3 multipliser med 4");
    nl_assert_eq_text(expr_norsk_multipliser_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliser_med_underscore = selfhost__compiler__disasm_uttrykk("3 multipliser_med 4");
    nl_assert_eq_text(expr_norsk_multipliser_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multiplisermed_kompakt = selfhost__compiler__disasm_uttrykk("3 multiplisermed 4");
    nl_assert_eq_text(expr_norsk_multiplisermed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multiplisere_med = selfhost__compiler__disasm_uttrykk("3 multiplisere med 4");
    nl_assert_eq_text(expr_norsk_multiplisere_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multiplisere_med_underscore = selfhost__compiler__disasm_uttrykk("3 multiplisere_med 4");
    nl_assert_eq_text(expr_norsk_multiplisere_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliseremed_kompakt = selfhost__compiler__disasm_uttrykk("3 multipliseremed 4");
    nl_assert_eq_text(expr_norsk_multipliseremed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliserer_med = selfhost__compiler__disasm_uttrykk("3 multipliserer med 4");
    nl_assert_eq_text(expr_norsk_multipliserer_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliserer_med_underscore = selfhost__compiler__disasm_uttrykk("3 multipliserer_med 4");
    nl_assert_eq_text(expr_norsk_multipliserer_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliserermed_kompakt = selfhost__compiler__disasm_uttrykk("3 multipliserermed 4");
    nl_assert_eq_text(expr_norsk_multipliserermed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multiplisert_med = selfhost__compiler__disasm_uttrykk("3 multiplisert med 4");
    nl_assert_eq_text(expr_norsk_multiplisert_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multiplisert_med_underscore = selfhost__compiler__disasm_uttrykk("3 multiplisert_med 4");
    nl_assert_eq_text(expr_norsk_multiplisert_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multiplisertmed_kompakt = selfhost__compiler__disasm_uttrykk("3 multiplisertmed 4");
    nl_assert_eq_text(expr_norsk_multiplisertmed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliseres_med = selfhost__compiler__disasm_uttrykk("3 multipliseres med 4");
    nl_assert_eq_text(expr_norsk_multipliseres_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliseres_med_underscore = selfhost__compiler__disasm_uttrykk("3 multipliseres_med 4");
    nl_assert_eq_text(expr_norsk_multipliseres_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliseresmed_kompakt = selfhost__compiler__disasm_uttrykk("3 multipliseresmed 4");
    nl_assert_eq_text(expr_norsk_multipliseresmed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliser_kort = selfhost__compiler__disasm_uttrykk("3 multipliser 4");
    nl_assert_eq_text(expr_norsk_multipliser_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multiplisere_kort = selfhost__compiler__disasm_uttrykk("3 multiplisere 4");
    nl_assert_eq_text(expr_norsk_multiplisere_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliserer_kort = selfhost__compiler__disasm_uttrykk("3 multipliserer 4");
    nl_assert_eq_text(expr_norsk_multipliserer_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multiplisert_kort = selfhost__compiler__disasm_uttrykk("3 multiplisert 4");
    nl_assert_eq_text(expr_norsk_multiplisert_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_multipliseres_kort = selfhost__compiler__disasm_uttrykk("3 multipliseres 4");
    nl_assert_eq_text(expr_norsk_multipliseres_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delt_med = selfhost__compiler__disasm_uttrykk("8 delt med 2");
    nl_assert_eq_text(expr_norsk_delt_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delt_med_underscore = selfhost__compiler__disasm_uttrykk("8 delt_med 2");
    nl_assert_eq_text(expr_norsk_delt_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deltmed_kompakt = selfhost__compiler__disasm_uttrykk("8 deltmed 2");
    nl_assert_eq_text(expr_norsk_deltmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_med = selfhost__compiler__disasm_uttrykk("8 divider med 2");
    nl_assert_eq_text(expr_norsk_divider_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_med_underscore = selfhost__compiler__disasm_uttrykk("8 divider_med 2");
    nl_assert_eq_text(expr_norsk_divider_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividermed_kompakt = selfhost__compiler__disasm_uttrykk("8 dividermed 2");
    nl_assert_eq_text(expr_norsk_dividermed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_med = selfhost__compiler__disasm_uttrykk("8 dividere med 2");
    nl_assert_eq_text(expr_norsk_dividere_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_med_underscore = selfhost__compiler__disasm_uttrykk("8 dividere_med 2");
    nl_assert_eq_text(expr_norsk_dividere_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideremed_kompakt = selfhost__compiler__disasm_uttrykk("8 divideremed 2");
    nl_assert_eq_text(expr_norsk_divideremed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_seg_med = selfhost__compiler__disasm_uttrykk("8 dele seg med 2");
    nl_assert_eq_text(expr_norsk_dele_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_seg_med_underscore = selfhost__compiler__disasm_uttrykk("8 dele_seg_med 2");
    nl_assert_eq_text(expr_norsk_dele_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delesegmed_kompakt = selfhost__compiler__disasm_uttrykk("8 delesegmed 2");
    nl_assert_eq_text(expr_norsk_delesegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_seg_med = selfhost__compiler__disasm_uttrykk("8 deler seg med 2");
    nl_assert_eq_text(expr_norsk_deler_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_seg_med_underscore = selfhost__compiler__disasm_uttrykk("8 deler_seg_med 2");
    nl_assert_eq_text(expr_norsk_deler_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delersegmed_kompakt = selfhost__compiler__disasm_uttrykk("8 delersegmed 2");
    nl_assert_eq_text(expr_norsk_delersegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_seg_pa = selfhost__compiler__disasm_uttrykk("8 dele seg pa 2");
    nl_assert_eq_text(expr_norsk_dele_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_seg_paa_phrase_utf8 = selfhost__compiler__disasm_uttrykk("8 dele seg på 2");
    nl_assert_eq_text(expr_norsk_dele_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_seg_paa_phrase_ascii = selfhost__compiler__disasm_uttrykk("8 dele seg paa 2");
    nl_assert_eq_text(expr_norsk_dele_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_seg_pa_underscore = selfhost__compiler__disasm_uttrykk("8 dele_seg_pa 2");
    nl_assert_eq_text(expr_norsk_dele_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delesegpa_kompakt = selfhost__compiler__disasm_uttrykk("8 delesegpa 2");
    nl_assert_eq_text(expr_norsk_delesegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delesegpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 delesegpaa 2");
    nl_assert_eq_text(expr_norsk_delesegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_seg_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 dele_seg_på 2");
    nl_assert_eq_text(expr_norsk_dele_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_seg_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 dele_seg_paa 2");
    nl_assert_eq_text(expr_norsk_dele_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_seg_pa = selfhost__compiler__disasm_uttrykk("8 deler seg pa 2");
    nl_assert_eq_text(expr_norsk_deler_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_seg_paa_phrase_utf8 = selfhost__compiler__disasm_uttrykk("8 deler seg på 2");
    nl_assert_eq_text(expr_norsk_deler_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_seg_paa_phrase_ascii = selfhost__compiler__disasm_uttrykk("8 deler seg paa 2");
    nl_assert_eq_text(expr_norsk_deler_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_seg_pa_underscore = selfhost__compiler__disasm_uttrykk("8 deler_seg_pa 2");
    nl_assert_eq_text(expr_norsk_deler_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delersegpa_kompakt = selfhost__compiler__disasm_uttrykk("8 delersegpa 2");
    nl_assert_eq_text(expr_norsk_delersegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delersegpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 delersegpaa 2");
    nl_assert_eq_text(expr_norsk_delersegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_seg_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 deler_seg_på 2");
    nl_assert_eq_text(expr_norsk_deler_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_seg_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 deler_seg_paa 2");
    nl_assert_eq_text(expr_norsk_deler_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_seg_med = selfhost__compiler__disasm_uttrykk("8 divider seg med 2");
    nl_assert_eq_text(expr_norsk_divider_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_seg_med_underscore = selfhost__compiler__disasm_uttrykk("8 divider_seg_med 2");
    nl_assert_eq_text(expr_norsk_divider_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividersegmed_kompakt = selfhost__compiler__disasm_uttrykk("8 dividersegmed 2");
    nl_assert_eq_text(expr_norsk_dividersegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_seg_pa = selfhost__compiler__disasm_uttrykk("8 divider seg pa 2");
    nl_assert_eq_text(expr_norsk_divider_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_seg_paa_phrase_utf8 = selfhost__compiler__disasm_uttrykk("8 divider seg på 2");
    nl_assert_eq_text(expr_norsk_divider_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_seg_paa_phrase_ascii = selfhost__compiler__disasm_uttrykk("8 divider seg paa 2");
    nl_assert_eq_text(expr_norsk_divider_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_seg_pa_underscore = selfhost__compiler__disasm_uttrykk("8 divider_seg_pa 2");
    nl_assert_eq_text(expr_norsk_divider_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividersegpa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividersegpa 2");
    nl_assert_eq_text(expr_norsk_dividersegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividersegpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividersegpaa 2");
    nl_assert_eq_text(expr_norsk_dividersegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_seg_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 divider_seg_på 2");
    nl_assert_eq_text(expr_norsk_divider_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_seg_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 divider_seg_paa 2");
    nl_assert_eq_text(expr_norsk_divider_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_seg_med = selfhost__compiler__disasm_uttrykk("8 dividere seg med 2");
    nl_assert_eq_text(expr_norsk_dividere_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_seg_med_underscore = selfhost__compiler__disasm_uttrykk("8 dividere_seg_med 2");
    nl_assert_eq_text(expr_norsk_dividere_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideresegmed_kompakt = selfhost__compiler__disasm_uttrykk("8 divideresegmed 2");
    nl_assert_eq_text(expr_norsk_divideresegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_seg_pa = selfhost__compiler__disasm_uttrykk("8 dividere seg pa 2");
    nl_assert_eq_text(expr_norsk_dividere_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_seg_paa_phrase_utf8 = selfhost__compiler__disasm_uttrykk("8 dividere seg på 2");
    nl_assert_eq_text(expr_norsk_dividere_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_seg_paa_phrase_ascii = selfhost__compiler__disasm_uttrykk("8 dividere seg paa 2");
    nl_assert_eq_text(expr_norsk_dividere_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_seg_pa_underscore = selfhost__compiler__disasm_uttrykk("8 dividere_seg_pa 2");
    nl_assert_eq_text(expr_norsk_dividere_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideresegpa_kompakt = selfhost__compiler__disasm_uttrykk("8 divideresegpa 2");
    nl_assert_eq_text(expr_norsk_divideresegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideresegpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 divideresegpaa 2");
    nl_assert_eq_text(expr_norsk_divideresegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_seg_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 dividere_seg_på 2");
    nl_assert_eq_text(expr_norsk_dividere_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_seg_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 dividere_seg_paa 2");
    nl_assert_eq_text(expr_norsk_dividere_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_seg_med = selfhost__compiler__disasm_uttrykk("8 dividerer seg med 2");
    nl_assert_eq_text(expr_norsk_dividerer_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_seg_med_underscore = selfhost__compiler__disasm_uttrykk("8 dividerer_seg_med 2");
    nl_assert_eq_text(expr_norsk_dividerer_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerersegmed_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerersegmed 2");
    nl_assert_eq_text(expr_norsk_dividerersegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_seg_pa = selfhost__compiler__disasm_uttrykk("8 dividerer seg pa 2");
    nl_assert_eq_text(expr_norsk_dividerer_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_seg_paa_phrase_utf8 = selfhost__compiler__disasm_uttrykk("8 dividerer seg på 2");
    nl_assert_eq_text(expr_norsk_dividerer_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_seg_paa_phrase_ascii = selfhost__compiler__disasm_uttrykk("8 dividerer seg paa 2");
    nl_assert_eq_text(expr_norsk_dividerer_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_seg_pa_underscore = selfhost__compiler__disasm_uttrykk("8 dividerer_seg_pa 2");
    nl_assert_eq_text(expr_norsk_dividerer_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerersegpa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerersegpa 2");
    nl_assert_eq_text(expr_norsk_dividerersegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerersegpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerersegpaa 2");
    nl_assert_eq_text(expr_norsk_dividerersegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_seg_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 dividerer_seg_på 2");
    nl_assert_eq_text(expr_norsk_dividerer_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_seg_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 dividerer_seg_paa 2");
    nl_assert_eq_text(expr_norsk_dividerer_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_med = selfhost__compiler__disasm_uttrykk("8 dividerer med 2");
    nl_assert_eq_text(expr_norsk_dividerer_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_med_underscore = selfhost__compiler__disasm_uttrykk("8 dividerer_med 2");
    nl_assert_eq_text(expr_norsk_dividerer_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerermed_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerermed 2");
    nl_assert_eq_text(expr_norsk_dividerermed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividert_med = selfhost__compiler__disasm_uttrykk("8 dividert med 2");
    nl_assert_eq_text(expr_norsk_dividert_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividert_med_underscore = selfhost__compiler__disasm_uttrykk("8 dividert_med 2");
    nl_assert_eq_text(expr_norsk_dividert_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividertmed_kompakt = selfhost__compiler__disasm_uttrykk("8 dividertmed 2");
    nl_assert_eq_text(expr_norsk_dividertmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideres_med = selfhost__compiler__disasm_uttrykk("8 divideres med 2");
    nl_assert_eq_text(expr_norsk_divideres_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideres_med_underscore = selfhost__compiler__disasm_uttrykk("8 divideres_med 2");
    nl_assert_eq_text(expr_norsk_divideres_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideresmed_kompakt = selfhost__compiler__disasm_uttrykk("8 divideresmed 2");
    nl_assert_eq_text(expr_norsk_divideresmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deles_med = selfhost__compiler__disasm_uttrykk("8 deles med 2");
    nl_assert_eq_text(expr_norsk_deles_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deles_med_underscore = selfhost__compiler__disasm_uttrykk("8 deles_med 2");
    nl_assert_eq_text(expr_norsk_deles_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delesmed_kompakt = selfhost__compiler__disasm_uttrykk("8 delesmed 2");
    nl_assert_eq_text(expr_norsk_delesmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_del_med = selfhost__compiler__disasm_uttrykk("8 del med 2");
    nl_assert_eq_text(expr_norsk_del_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_del_med_underscore = selfhost__compiler__disasm_uttrykk("8 del_med 2");
    nl_assert_eq_text(expr_norsk_del_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delmed_kompakt = selfhost__compiler__disasm_uttrykk("8 delmed 2");
    nl_assert_eq_text(expr_norsk_delmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_med = selfhost__compiler__disasm_uttrykk("8 dele med 2");
    nl_assert_eq_text(expr_norsk_dele_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_med_underscore = selfhost__compiler__disasm_uttrykk("8 dele_med 2");
    nl_assert_eq_text(expr_norsk_dele_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delemed_kompakt = selfhost__compiler__disasm_uttrykk("8 delemed 2");
    nl_assert_eq_text(expr_norsk_delemed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_paa_utf8 = selfhost__compiler__disasm_uttrykk("8 dele på 2");
    nl_assert_eq_text(expr_norsk_dele_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_paa_ascii = selfhost__compiler__disasm_uttrykk("8 dele pa 2");
    nl_assert_eq_text(expr_norsk_dele_paa_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_paa_underscore = selfhost__compiler__disasm_uttrykk("8 dele_pa 2");
    nl_assert_eq_text(expr_norsk_dele_paa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delepa_kompakt = selfhost__compiler__disasm_uttrykk("8 delepa 2");
    nl_assert_eq_text(expr_norsk_delepa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delepaa_kompakt = selfhost__compiler__disasm_uttrykk("8 delepaa 2");
    nl_assert_eq_text(expr_norsk_delepaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_del_paa_utf8 = selfhost__compiler__disasm_uttrykk("8 del på 2");
    nl_assert_eq_text(expr_norsk_del_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_del_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 del_på 2");
    nl_assert_eq_text(expr_norsk_del_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_del_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 del_pa 2");
    nl_assert_eq_text(expr_norsk_del_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delpa_kompakt = selfhost__compiler__disasm_uttrykk("8 delpa 2");
    nl_assert_eq_text(expr_norsk_delpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 delpaa 2");
    nl_assert_eq_text(expr_norsk_delpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_del_paa_double_a_phrase = selfhost__compiler__disasm_uttrykk("8 del paa 2");
    nl_assert_eq_text(expr_norsk_del_paa_double_a_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_del_paa_double_a_underscore = selfhost__compiler__disasm_uttrykk("8 del_paa 2");
    nl_assert_eq_text(expr_norsk_del_paa_double_a_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deles_paa_utf8 = selfhost__compiler__disasm_uttrykk("8 deles på 2");
    nl_assert_eq_text(expr_norsk_deles_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deles_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 deles_på 2");
    nl_assert_eq_text(expr_norsk_deles_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deles_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 deles_pa 2");
    nl_assert_eq_text(expr_norsk_deles_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delespa_kompakt = selfhost__compiler__disasm_uttrykk("8 delespa 2");
    nl_assert_eq_text(expr_norsk_delespa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delespaa_kompakt = selfhost__compiler__disasm_uttrykk("8 delespaa 2");
    nl_assert_eq_text(expr_norsk_delespaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deles_paa_double_a_phrase = selfhost__compiler__disasm_uttrykk("8 deles paa 2");
    nl_assert_eq_text(expr_norsk_deles_paa_double_a_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deles_paa_double_a_underscore = selfhost__compiler__disasm_uttrykk("8 deles_paa 2");
    nl_assert_eq_text(expr_norsk_deles_paa_double_a_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_paa_utf8 = selfhost__compiler__disasm_uttrykk("8 divider på 2");
    nl_assert_eq_text(expr_norsk_divider_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 divider_pa 2");
    nl_assert_eq_text(expr_norsk_divider_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerpa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerpa 2");
    nl_assert_eq_text(expr_norsk_dividerpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerpaa 2");
    nl_assert_eq_text(expr_norsk_dividerpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_paa_utf8 = selfhost__compiler__disasm_uttrykk("8 dividere på 2");
    nl_assert_eq_text(expr_norsk_dividere_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 dividere_pa 2");
    nl_assert_eq_text(expr_norsk_dividere_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerepa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerepa 2");
    nl_assert_eq_text(expr_norsk_dividerepa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerepaa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerepaa 2");
    nl_assert_eq_text(expr_norsk_dividerepaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_paa_utf8 = selfhost__compiler__disasm_uttrykk("8 dividerer på 2");
    nl_assert_eq_text(expr_norsk_dividerer_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerer_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 dividerer_pa 2");
    nl_assert_eq_text(expr_norsk_dividerer_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividererpa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividererpa 2");
    nl_assert_eq_text(expr_norsk_dividererpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividererpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividererpaa 2");
    nl_assert_eq_text(expr_norsk_dividererpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividert_paa_utf8 = selfhost__compiler__disasm_uttrykk("8 dividert på 2");
    nl_assert_eq_text(expr_norsk_dividert_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividert_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 dividert_pa 2");
    nl_assert_eq_text(expr_norsk_dividert_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividertpa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividertpa 2");
    nl_assert_eq_text(expr_norsk_dividertpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividertpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividertpaa 2");
    nl_assert_eq_text(expr_norsk_dividertpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideres_paa_utf8 = selfhost__compiler__disasm_uttrykk("8 divideres på 2");
    nl_assert_eq_text(expr_norsk_divideres_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideres_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 divideres_på 2");
    nl_assert_eq_text(expr_norsk_divideres_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideres_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 divideres_pa 2");
    nl_assert_eq_text(expr_norsk_divideres_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerespa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerespa 2");
    nl_assert_eq_text(expr_norsk_dividerespa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividerespaa_kompakt = selfhost__compiler__disasm_uttrykk("8 dividerespaa 2");
    nl_assert_eq_text(expr_norsk_dividerespaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideres_paa_double_a_phrase = selfhost__compiler__disasm_uttrykk("8 divideres paa 2");
    nl_assert_eq_text(expr_norsk_divideres_paa_double_a_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divideres_paa_double_a_underscore = selfhost__compiler__disasm_uttrykk("8 divideres_paa 2");
    nl_assert_eq_text(expr_norsk_divideres_paa_double_a_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delt_paa_utf8 = selfhost__compiler__disasm_uttrykk("8 delt på 2");
    nl_assert_eq_text(expr_norsk_delt_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delt_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 delt_på 2");
    nl_assert_eq_text(expr_norsk_delt_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delt_paa_underscore_ascii = selfhost__compiler__disasm_uttrykk("8 delt_pa 2");
    nl_assert_eq_text(expr_norsk_delt_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deltpa_kompakt = selfhost__compiler__disasm_uttrykk("8 deltpa 2");
    nl_assert_eq_text(expr_norsk_deltpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deltpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 deltpaa 2");
    nl_assert_eq_text(expr_norsk_deltpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delt_paa_double_a_phrase = selfhost__compiler__disasm_uttrykk("8 delt paa 2");
    nl_assert_eq_text(expr_norsk_delt_paa_double_a_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delt_paa_double_a_underscore = selfhost__compiler__disasm_uttrykk("8 delt_paa 2");
    nl_assert_eq_text(expr_norsk_delt_paa_double_a_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_pa = selfhost__compiler__disasm_uttrykk("8 deler pa 2");
    nl_assert_eq_text(expr_norsk_deler_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_paa_underscore_utf8 = selfhost__compiler__disasm_uttrykk("8 deler_på 2");
    nl_assert_eq_text(expr_norsk_deler_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_med = selfhost__compiler__disasm_uttrykk("8 deler med 2");
    nl_assert_eq_text(expr_norsk_deler_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_pa_underscore = selfhost__compiler__disasm_uttrykk("8 deler_pa 2");
    nl_assert_eq_text(expr_norsk_deler_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delerpa_kompakt = selfhost__compiler__disasm_uttrykk("8 delerpa 2");
    nl_assert_eq_text(expr_norsk_delerpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_delerpaa_kompakt = selfhost__compiler__disasm_uttrykk("8 delerpaa 2");
    nl_assert_eq_text(expr_norsk_delerpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_med_underscore = selfhost__compiler__disasm_uttrykk("8 deler_med 2");
    nl_assert_eq_text(expr_norsk_deler_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dele_kort = selfhost__compiler__disasm_uttrykk("8 dele 2");
    nl_assert_eq_text(expr_norsk_dele_kort, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_deler_kort = selfhost__compiler__disasm_uttrykk("8 deler 2");
    nl_assert_eq_text(expr_norsk_deler_kort, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_divider_kort = selfhost__compiler__disasm_uttrykk("8 divider 2");
    nl_assert_eq_text(expr_norsk_divider_kort, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_norsk_dividere_kort = selfhost__compiler__disasm_uttrykk("8 dividere 2");
    nl_assert_eq_text(expr_norsk_dividere_kort, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_bool_literals = selfhost__compiler__disasm_uttrykk("sann&&usann||!usann");
    nl_assert_eq_text(expr_bool_literals, "0: PUSH 1\n1: PUSH 0\n2: AND\n3: PUSH 0\n4: NOT\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_bool_literals_sant = selfhost__compiler__disasm_uttrykk("sant&&usann");
    nl_assert_eq_text(expr_bool_literals_sant, "0: PUSH 1\n1: PUSH 0\n2: AND\n3: PRINT\n4: HALT\n");
    char * expr_bool_literals_usant = selfhost__compiler__disasm_uttrykk("sant&&usant");
    nl_assert_eq_text(expr_bool_literals_usant, "0: PUSH 1\n1: PUSH 0\n2: AND\n3: PRINT\n4: HALT\n");
    char * expr_bool_literals_ja_nei = selfhost__compiler__disasm_uttrykk("ja&&nei");
    nl_assert_eq_text(expr_bool_literals_ja_nei, "0: PUSH 1\n1: PUSH 0\n2: AND\n3: PRINT\n4: HALT\n");
    char * expr_bool_literals_true_false = selfhost__compiler__disasm_uttrykk("true&&false");
    nl_assert_eq_text(expr_bool_literals_true_false, "0: PUSH 1\n1: PUSH 0\n2: AND\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ops = selfhost__compiler__disasm_uttrykk("sann og ikke usann eller usann");
    nl_assert_eq_text(expr_norsk_ops, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PUSH 0\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_norsk_ops_enten = selfhost__compiler__disasm_uttrykk("usann enten sann");
    nl_assert_eq_text(expr_norsk_ops_enten, "0: PUSH 0\n1: PUSH 1\n2: OR\n3: PRINT\n4: HALT\n");
    char * expr_norsk_ops_ikkje = selfhost__compiler__disasm_uttrykk("sann og ikkje usann");
    nl_assert_eq_text(expr_norsk_ops_ikkje, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_norsk_ops_samt = selfhost__compiler__disasm_uttrykk("sann samt ikke usann");
    nl_assert_eq_text(expr_norsk_ops_samt, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp = selfhost__compiler__disasm_uttrykk("7 er 7 og 3 mindre_enn 4");
    nl_assert_eq_text(expr_norsk_cmp, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PUSH 3\n4: PUSH 4\n5: LT\n6: AND\n7: PRINT\n8: HALT\n");
    char * expr_norsk_cmp2 = selfhost__compiler__disasm_uttrykk("7 ikke_er 8 og 4 storre_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp2, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PUSH 4\n5: PUSH 4\n6: LT\n7: NOT\n8: AND\n9: PRINT\n10: HALT\n");
    char * expr_norsk_cmp_phrase = selfhost__compiler__disasm_uttrykk("7 er 7 og 3 mindre enn 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PUSH 3\n4: PUSH 4\n5: LT\n6: AND\n7: PRINT\n8: HALT\n");
    char * expr_norsk_cmp_phrase2 = selfhost__compiler__disasm_uttrykk("7 ikke er 8 og 4 storre eller lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase2, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PUSH 4\n5: PUSH 4\n6: LT\n7: NOT\n8: AND\n9: PRINT\n10: HALT\n");
    char * expr_norsk_cmp_phrase3 = selfhost__compiler__disasm_uttrykk("7 er ikke 8 og 4 storre eller lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase3, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PUSH 4\n5: PUSH 4\n6: LT\n7: NOT\n8: AND\n9: PRINT\n10: HALT\n");
    char * expr_norsk_cmp_phrase4 = selfhost__compiler__disasm_uttrykk("7 er lik 7 og 3 mindre enn 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase4, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PUSH 3\n4: PUSH 4\n5: LT\n6: AND\n7: PRINT\n8: HALT\n");
    char * expr_norsk_cmp_phrase5 = selfhost__compiler__disasm_uttrykk("7 er ikke lik 8 og 4 storre eller lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase5, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PUSH 4\n5: PUSH 4\n6: LT\n7: NOT\n8: AND\n9: PRINT\n10: HALT\n");
    char * expr_norsk_cmp_phrase6 = selfhost__compiler__disasm_uttrykk("3 er mindre enn 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase6, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase6_underscore = selfhost__compiler__disasm_uttrykk("3 er_mindre_enn 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase6_underscore, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase6_underscore_lte = selfhost__compiler__disasm_uttrykk("3 er_mindre_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase6_underscore_lte, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase6_kompakt_lte = selfhost__compiler__disasm_uttrykk("3 ermindreellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase6_kompakt_lte, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase6_underscore_lte_enn = selfhost__compiler__disasm_uttrykk("3 er_mindre_enn_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase6_underscore_lte_enn, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7 = selfhost__compiler__disasm_uttrykk("4 er storre eller lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_underscore = selfhost__compiler__disasm_uttrykk("4 er_storre_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_underscore, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_underscore_utf8 = selfhost__compiler__disasm_uttrykk("4 er_større_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_underscore_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_erstorrelik_kompakt = selfhost__compiler__disasm_uttrykk("4 erstorrelik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_erstorrelik_kompakt, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_erstorrelik_kompakt_utf8 = selfhost__compiler__disasm_uttrykk("4 erstørrelik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_erstorrelik_kompakt_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_kompakt_gte = selfhost__compiler__disasm_uttrykk("4 erstorreellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_kompakt_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_kompakt_ellerlik_gte_utf8 = selfhost__compiler__disasm_uttrykk("4 erstørreellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_kompakt_ellerlik_gte_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_underscore_gt = selfhost__compiler__disasm_uttrykk("4 er_storre_enn 3");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_underscore_gt, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase7_underscore_gt_utf8 = selfhost__compiler__disasm_uttrykk("4 er_større_enn 3");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_underscore_gt_utf8, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase7_kompakt_gt_ascii = selfhost__compiler__disasm_uttrykk("4 erstorreenn 3");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_kompakt_gt_ascii, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase7_kompakt_gt_utf8 = selfhost__compiler__disasm_uttrykk("4 erstørreenn 3");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_kompakt_gt_utf8, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase7_underscore_gte = selfhost__compiler__disasm_uttrykk("4 er_storre_enn_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_underscore_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_underscore_gte_utf8 = selfhost__compiler__disasm_uttrykk("4 er_større_enn_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_underscore_gte_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_kompakt_gte_ascii = selfhost__compiler__disasm_uttrykk("4 erstorreennellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_kompakt_gte_ascii, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase7_kompakt_gte_utf8 = selfhost__compiler__disasm_uttrykk("4 erstørreennellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase7_kompakt_gte_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase8 = selfhost__compiler__disasm_uttrykk("7 ikke lik 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase8, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase9 = selfhost__compiler__disasm_uttrykk("7 ulik 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase9, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase10 = selfhost__compiler__disasm_uttrykk("7 er ulik 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase10, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase10_underscore = selfhost__compiler__disasm_uttrykk("7 er_ulik 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase10_underscore, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase11 = selfhost__compiler__disasm_uttrykk("4 storre enn eller lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase11, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase12 = selfhost__compiler__disasm_uttrykk("4 er storre enn eller lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase12, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase13 = selfhost__compiler__disasm_uttrykk("7 er lik med 7");
    nl_assert_eq_text(expr_norsk_cmp_phrase13, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase13_underscore = selfhost__compiler__disasm_uttrykk("7 er_lik_med 7");
    nl_assert_eq_text(expr_norsk_cmp_phrase13_underscore, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase13_alias = selfhost__compiler__disasm_uttrykk("7 er_lik 7");
    nl_assert_eq_text(expr_norsk_cmp_phrase13_alias, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase13_lik_med_alias = selfhost__compiler__disasm_uttrykk("7 lik_med 7");
    nl_assert_eq_text(expr_norsk_cmp_phrase13_lik_med_alias, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase13_likmed_kompakt = selfhost__compiler__disasm_uttrykk("7 likmed 7");
    nl_assert_eq_text(expr_norsk_cmp_phrase13_likmed_kompakt, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase14 = selfhost__compiler__disasm_uttrykk("7 ikke lik med 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase14, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase14_underscore = selfhost__compiler__disasm_uttrykk("7 ikke_lik_med 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase14_underscore, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase14_ikkelikmed_kompakt = selfhost__compiler__disasm_uttrykk("7 ikkelikmed 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase14_ikkelikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase14_alias = selfhost__compiler__disasm_uttrykk("7 ikke_lik 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase14_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase15 = selfhost__compiler__disasm_uttrykk("7 er ikke lik med 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase15, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase16 = selfhost__compiler__disasm_uttrykk("7 lik 7");
    nl_assert_eq_text(expr_norsk_cmp_phrase16, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_phrase17 = selfhost__compiler__disasm_uttrykk("7 ulik med 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase17, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase17_underscore = selfhost__compiler__disasm_uttrykk("7 ulik_med 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase17_underscore, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase17_ulikmed_kompakt = selfhost__compiler__disasm_uttrykk("7 ulikmed 8");
    nl_assert_eq_text(expr_norsk_cmp_phrase17_ulikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase18 = selfhost__compiler__disasm_uttrykk("3 mindre lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase18, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase19 = selfhost__compiler__disasm_uttrykk("4 storre lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase19, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase19_underscore = selfhost__compiler__disasm_uttrykk("4 storre_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase19_underscore, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase19_storrelik_kompakt = selfhost__compiler__disasm_uttrykk("4 storrelik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase19_storrelik_kompakt, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase19_underscore_utf8 = selfhost__compiler__disasm_uttrykk("4 større_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase19_underscore_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase19_storrelik_kompakt_utf8 = selfhost__compiler__disasm_uttrykk("4 størrelik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase19_storrelik_kompakt_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase20 = selfhost__compiler__disasm_uttrykk("4 er storre lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase20, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase_utf8 = selfhost__compiler__disasm_uttrykk("4 er større enn eller lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_underscore_utf8 = selfhost__compiler__disasm_uttrykk("4 større_enn_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_underscore_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_underscore_utf8_gt = selfhost__compiler__disasm_uttrykk("4 større_enn 3");
    nl_assert_eq_text(expr_norsk_cmp_underscore_utf8_gt, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_storreen_kompakt_ascii = selfhost__compiler__disasm_uttrykk("4 storreenn 3");
    nl_assert_eq_text(expr_norsk_cmp_storreen_kompakt_ascii, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_storreen_kompakt_utf8 = selfhost__compiler__disasm_uttrykk("4 størreenn 3");
    nl_assert_eq_text(expr_norsk_cmp_storreen_kompakt_utf8, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_underscore_utf8_gte = selfhost__compiler__disasm_uttrykk("4 større_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_underscore_utf8_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_kompakt_utf8_gte = selfhost__compiler__disasm_uttrykk("4 størreellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_kompakt_utf8_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_kompakt_ascii_gte = selfhost__compiler__disasm_uttrykk("4 storreellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_kompakt_ascii_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_underscore_ascii_storre = selfhost__compiler__disasm_uttrykk("4 storre_enn_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_underscore_ascii_storre, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_underscore_ascii = selfhost__compiler__disasm_uttrykk("3 mindre_enn_eller_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_underscore_ascii, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_kompakt_lte_ascii = selfhost__compiler__disasm_uttrykk("3 mindreennellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_kompakt_lte_ascii, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_kompakt_lte_er_ascii = selfhost__compiler__disasm_uttrykk("3 ermindreennellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_kompakt_lte_er_ascii, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_kompakt_lt_er_ascii = selfhost__compiler__disasm_uttrykk("3 ermindreenn 4");
    nl_assert_eq_text(expr_norsk_cmp_kompakt_lt_er_ascii, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_mindreenn_kompakt = selfhost__compiler__disasm_uttrykk("3 mindreenn 4");
    nl_assert_eq_text(expr_norsk_cmp_mindreenn_kompakt, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_underscore_ascii_mindre_lik = selfhost__compiler__disasm_uttrykk("3 mindre_lik 4");
    nl_assert_eq_text(expr_norsk_cmp_underscore_ascii_mindre_lik, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_ermindrelik_kompakt = selfhost__compiler__disasm_uttrykk("3 ermindrelik 4");
    nl_assert_eq_text(expr_norsk_cmp_ermindrelik_kompakt, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_kompakt_ascii_lte = selfhost__compiler__disasm_uttrykk("3 mindreellerlik 4");
    nl_assert_eq_text(expr_norsk_cmp_kompakt_ascii_lte, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_underscore_ascii_mindrelik_kompakt = selfhost__compiler__disasm_uttrykk("3 mindrelik 4");
    nl_assert_eq_text(expr_norsk_cmp_underscore_ascii_mindrelik_kompakt, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase21 = selfhost__compiler__disasm_uttrykk("3 mindre enn lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase21, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_er_ikke_alias = selfhost__compiler__disasm_uttrykk("7 er_ikke 8");
    nl_assert_eq_text(expr_norsk_cmp_er_ikke_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_erikke_kompakt = selfhost__compiler__disasm_uttrykk("7 erikke 8");
    nl_assert_eq_text(expr_norsk_cmp_erikke_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_ikkje_er_alias = selfhost__compiler__disasm_uttrykk("7 ikkje_er 8");
    nl_assert_eq_text(expr_norsk_cmp_ikkje_er_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_ikkje_lik_alias = selfhost__compiler__disasm_uttrykk("7 ikkje_lik 8");
    nl_assert_eq_text(expr_norsk_cmp_ikkje_lik_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_ikkje_lik_med_alias = selfhost__compiler__disasm_uttrykk("7 ikkje_lik_med 8");
    nl_assert_eq_text(expr_norsk_cmp_ikkje_lik_med_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_ikkjelikmed_kompakt = selfhost__compiler__disasm_uttrykk("7 ikkjelikmed 8");
    nl_assert_eq_text(expr_norsk_cmp_ikkjelikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_er_ulik_med_alias = selfhost__compiler__disasm_uttrykk("7 er_ulik_med 8");
    nl_assert_eq_text(expr_norsk_cmp_er_ulik_med_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_erlikmed_kompakt = selfhost__compiler__disasm_uttrykk("7 erlikmed 7");
    nl_assert_eq_text(expr_norsk_cmp_erlikmed_kompakt, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_erulikmed_kompakt = selfhost__compiler__disasm_uttrykk("7 erulikmed 8");
    nl_assert_eq_text(expr_norsk_cmp_erulikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_erikkelikmed_kompakt = selfhost__compiler__disasm_uttrykk("7 erikkelikmed 8");
    nl_assert_eq_text(expr_norsk_cmp_erikkelikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_er_ikkje_alias = selfhost__compiler__disasm_uttrykk("7 er_ikkje 8");
    nl_assert_eq_text(expr_norsk_cmp_er_ikkje_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_erlik_kompakt = selfhost__compiler__disasm_uttrykk("7 erlik 7");
    nl_assert_eq_text(expr_norsk_cmp_erlik_kompakt, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_norsk_cmp_erulik_kompakt = selfhost__compiler__disasm_uttrykk("7 erulik 8");
    nl_assert_eq_text(expr_norsk_cmp_erulik_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_erikkje_kompakt = selfhost__compiler__disasm_uttrykk("7 erikkje 8");
    nl_assert_eq_text(expr_norsk_cmp_erikkje_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_erikkelik_kompakt = selfhost__compiler__disasm_uttrykk("7 erikkelik 8");
    nl_assert_eq_text(expr_norsk_cmp_erikkelik_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_erikkjelik_kompakt = selfhost__compiler__disasm_uttrykk("7 erikkjelik 8");
    nl_assert_eq_text(expr_norsk_cmp_erikkjelik_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_erikkjelikmed_kompakt = selfhost__compiler__disasm_uttrykk("7 erikkjelikmed 8");
    nl_assert_eq_text(expr_norsk_cmp_erikkjelikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_ikkje_phrase = selfhost__compiler__disasm_uttrykk("7 er ikkje 8");
    nl_assert_eq_text(expr_norsk_cmp_ikkje_phrase, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_ikkje_lik_phrase = selfhost__compiler__disasm_uttrykk("7 ikkje lik 8");
    nl_assert_eq_text(expr_norsk_cmp_ikkje_lik_phrase, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_norsk_cmp_phrase22 = selfhost__compiler__disasm_uttrykk("4 er storre enn lik 4");
    nl_assert_eq_text(expr_norsk_cmp_phrase22, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    nl_list_text* env_navn = nl_list_text_new();
    nl_list_text_push(env_navn, "x");
    nl_list_text_push(env_navn, "y");
    nl_list_int* env_verdier = nl_list_int_new();
    nl_list_int_push(env_verdier, 7);
    nl_list_int_push(env_verdier, 3);
    char * expr_env = selfhost__compiler__disasm_uttrykk_med_miljo("x*2+y", env_navn, env_verdier);
    nl_assert_eq_text(expr_env, "0: PUSH 7\n1: PUSH 2\n2: MUL\n3: PUSH 3\n4: ADD\n5: PRINT\n6: HALT\n");
    char * expr_env_c = selfhost__compiler__kompiler_uttrykk_til_c_med_miljo("x+y", env_navn, env_verdier);
    nl_assert_ne_text(expr_env_c, "");
    char * expr_hvis = selfhost__compiler__disasm_uttrykk("hvis 1==1 da 7 ellers 9");
    nl_assert_eq_text(expr_hvis, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: JZ 6\n4: PUSH 7\n5: JMP 8\n6: LABEL 6\n7: PUSH 9\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * expr_hvis_ellers_hvis = selfhost__compiler__disasm_uttrykk("hvis 0==1 da 10 ellers_hvis 1==1 da 20 ellers 30");
    nl_assert_eq_text(expr_hvis_ellers_hvis, "0: PUSH 0\n1: PUSH 1\n2: EQ\n3: JZ 6\n4: PUSH 10\n5: JMP 16\n6: LABEL 6\n7: PUSH 1\n8: PUSH 1\n9: EQ\n10: JZ 13\n11: PUSH 20\n12: JMP 15\n13: LABEL 13\n14: PUSH 30\n15: LABEL 15\n16: LABEL 16\n17: PRINT\n18: HALT\n");
    char * expr_hvis_elif_alias = selfhost__compiler__disasm_uttrykk("hvis 0==1 da 10 elif 1==1 da 20 ellers 30");
    nl_assert_eq_text(expr_hvis_elif_alias, "0: PUSH 0\n1: PUSH 1\n2: EQ\n3: JZ 6\n4: PUSH 10\n5: JMP 16\n6: LABEL 6\n7: PUSH 1\n8: PUSH 1\n9: EQ\n10: JZ 13\n11: PUSH 20\n12: JMP 15\n13: LABEL 13\n14: PUSH 30\n15: LABEL 15\n16: LABEL 16\n17: PRINT\n18: HALT\n");
    char * expr_if_then_else_alias = selfhost__compiler__disasm_uttrykk("if 1==1 then 7 else 9");
    nl_assert_eq_text(expr_if_then_else_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: JZ 6\n4: PUSH 7\n5: JMP 8\n6: LABEL 6\n7: PUSH 9\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * expr_if_elseif_alias = selfhost__compiler__disasm_uttrykk("if 0==1 then 10 elseif 1==1 then 20 else 30");
    nl_assert_eq_text(expr_if_elseif_alias, "0: PUSH 0\n1: PUSH 1\n2: EQ\n3: JZ 6\n4: PUSH 10\n5: JMP 16\n6: LABEL 6\n7: PUSH 1\n8: PUSH 1\n9: EQ\n10: JZ 13\n11: PUSH 20\n12: JMP 15\n13: LABEL 13\n14: PUSH 30\n15: LABEL 15\n16: LABEL 16\n17: PRINT\n18: HALT\n");
    char * expr_if_elsif_alias = selfhost__compiler__disasm_uttrykk("if 0==1 then 10 elsif 1==1 then 20 else 30");
    nl_assert_eq_text(expr_if_elsif_alias, "0: PUSH 0\n1: PUSH 1\n2: EQ\n3: JZ 6\n4: PUSH 10\n5: JMP 16\n6: LABEL 6\n7: PUSH 1\n8: PUSH 1\n9: EQ\n10: JZ 13\n11: PUSH 20\n12: JMP 15\n13: LABEL 13\n14: PUSH 30\n15: LABEL 15\n16: LABEL 16\n17: PRINT\n18: HALT\n");
    char * expr_and_or_not_alias = selfhost__compiler__disasm_uttrykk("not 0 and 1 or 0");
    nl_assert_eq_text(expr_and_or_not_alias, "0: PUSH 0\n1: NOT\n2: PUSH 1\n3: AND\n4: PUSH 0\n5: OR\n6: PRINT\n7: HALT\n");
    char * expr_english_cmp_alias = selfhost__compiler__disasm_uttrykk("4 greater_or_equal 4 and 3 less_than 4");
    nl_assert_eq_text(expr_english_cmp_alias, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PUSH 3\n5: PUSH 4\n6: LT\n7: AND\n8: PRINT\n9: HALT\n");
    char * expr_english_is_not_alias = selfhost__compiler__disasm_uttrykk("7 is_not 8");
    nl_assert_eq_text(expr_english_is_not_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_eq_alias = selfhost__compiler__disasm_uttrykk("7 eq 7");
    nl_assert_eq_text(expr_english_eq_alias, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_english_neq_alias = selfhost__compiler__disasm_uttrykk("7 neq 8");
    nl_assert_eq_text(expr_english_neq_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_on_off_alias = selfhost__compiler__disasm_uttrykk("on and not off");
    nl_assert_eq_text(expr_english_on_off_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_yes_no_alias = selfhost__compiler__disasm_uttrykk("yes and not no");
    nl_assert_eq_text(expr_english_yes_no_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_enabled_disabled_alias = selfhost__compiler__disasm_uttrykk("enabled and not disabled");
    nl_assert_eq_text(expr_english_enabled_disabled_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_active_inactive_alias = selfhost__compiler__disasm_uttrykk("active and not inactive");
    nl_assert_eq_text(expr_english_active_inactive_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_truthy_falsy_alias = selfhost__compiler__disasm_uttrykk("truthy and not falsy");
    nl_assert_eq_text(expr_english_truthy_falsy_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_affirmative_negative_alias = selfhost__compiler__disasm_uttrykk("affirmative and not negative");
    nl_assert_eq_text(expr_english_affirmative_negative_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_affirm_deny_alias = selfhost__compiler__disasm_uttrykk("affirm and not deny");
    nl_assert_eq_text(expr_english_affirm_deny_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_approved_rejected_alias = selfhost__compiler__disasm_uttrykk("approved and not rejected");
    nl_assert_eq_text(expr_english_approved_rejected_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_accepted_declined_alias = selfhost__compiler__disasm_uttrykk("accepted and not declined");
    nl_assert_eq_text(expr_english_accepted_declined_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_confirmed_denied_alias = selfhost__compiler__disasm_uttrykk("confirmed and not denied");
    nl_assert_eq_text(expr_english_confirmed_denied_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_pass_fail_alias = selfhost__compiler__disasm_uttrykk("pass and not fail");
    nl_assert_eq_text(expr_english_pass_fail_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_success_failure_alias = selfhost__compiler__disasm_uttrykk("success and not failure");
    nl_assert_eq_text(expr_english_success_failure_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_allow_block_alias = selfhost__compiler__disasm_uttrykk("allow and not block");
    nl_assert_eq_text(expr_english_allow_block_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_permit_forbid_alias = selfhost__compiler__disasm_uttrykk("permit and not forbid");
    nl_assert_eq_text(expr_english_permit_forbid_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_valid_invalid_alias = selfhost__compiler__disasm_uttrykk("valid and not invalid");
    nl_assert_eq_text(expr_english_valid_invalid_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_ok_not_ok_alias = selfhost__compiler__disasm_uttrykk("ok and not not_ok");
    nl_assert_eq_text(expr_english_ok_not_ok_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_ready_not_ready_alias = selfhost__compiler__disasm_uttrykk("ready and not not_ready");
    nl_assert_eq_text(expr_english_ready_not_ready_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_safe_unsafe_alias = selfhost__compiler__disasm_uttrykk("safe and not unsafe");
    nl_assert_eq_text(expr_english_safe_unsafe_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_secure_insecure_alias = selfhost__compiler__disasm_uttrykk("secure and not insecure");
    nl_assert_eq_text(expr_english_secure_insecure_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_trusted_untrusted_alias = selfhost__compiler__disasm_uttrykk("trusted and not untrusted");
    nl_assert_eq_text(expr_english_trusted_untrusted_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_open_closed_alias = selfhost__compiler__disasm_uttrykk("open and not closed");
    nl_assert_eq_text(expr_english_open_closed_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_public_private_alias = selfhost__compiler__disasm_uttrykk("public and not private");
    nl_assert_eq_text(expr_english_public_private_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_visible_hidden_alias = selfhost__compiler__disasm_uttrykk("visible and not hidden");
    nl_assert_eq_text(expr_english_visible_hidden_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_present_absent_alias = selfhost__compiler__disasm_uttrykk("present and not absent");
    nl_assert_eq_text(expr_english_present_absent_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_online_offline_alias = selfhost__compiler__disasm_uttrykk("online and not offline");
    nl_assert_eq_text(expr_english_online_offline_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_connected_disconnected_alias = selfhost__compiler__disasm_uttrykk("connected and not disconnected");
    nl_assert_eq_text(expr_english_connected_disconnected_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_available_unavailable_alias = selfhost__compiler__disasm_uttrykk("available and not unavailable");
    nl_assert_eq_text(expr_english_available_unavailable_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_reachable_unreachable_alias = selfhost__compiler__disasm_uttrykk("reachable and not unreachable");
    nl_assert_eq_text(expr_english_reachable_unreachable_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_working_broken_alias = selfhost__compiler__disasm_uttrykk("working and not broken");
    nl_assert_eq_text(expr_english_working_broken_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_stable_unstable_alias = selfhost__compiler__disasm_uttrykk("stable and not unstable");
    nl_assert_eq_text(expr_english_stable_unstable_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_correct_incorrect_alias = selfhost__compiler__disasm_uttrykk("correct and not incorrect");
    nl_assert_eq_text(expr_english_correct_incorrect_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_complete_incomplete_alias = selfhost__compiler__disasm_uttrykk("complete and not incomplete");
    nl_assert_eq_text(expr_english_complete_incomplete_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_clean_dirty_alias = selfhost__compiler__disasm_uttrykk("clean and not dirty");
    nl_assert_eq_text(expr_english_clean_dirty_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_up_down_alias = selfhost__compiler__disasm_uttrykk("up and not down");
    nl_assert_eq_text(expr_english_up_down_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_alive_dead_alias = selfhost__compiler__disasm_uttrykk("alive and not dead");
    nl_assert_eq_text(expr_english_alive_dead_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_awake_asleep_alias = selfhost__compiler__disasm_uttrykk("awake and not asleep");
    nl_assert_eq_text(expr_english_awake_asleep_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_healthy_unhealthy_alias = selfhost__compiler__disasm_uttrykk("healthy and not unhealthy");
    nl_assert_eq_text(expr_english_healthy_unhealthy_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_synced_unsynced_alias = selfhost__compiler__disasm_uttrykk("synced and not unsynced");
    nl_assert_eq_text(expr_english_synced_unsynced_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * expr_english_equal_to_alias = selfhost__compiler__disasm_uttrykk("7 equal_to 7");
    nl_assert_eq_text(expr_english_equal_to_alias, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_english_equal_to_phrase = selfhost__compiler__disasm_uttrykk("7 equal to 7");
    nl_assert_eq_text(expr_english_equal_to_phrase, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_english_is_equal_to_phrase = selfhost__compiler__disasm_uttrykk("7 is equal to 7");
    nl_assert_eq_text(expr_english_is_equal_to_phrase, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_english_is_equal_to_compact_alias = selfhost__compiler__disasm_uttrykk("7 is_equal_to 7");
    nl_assert_eq_text(expr_english_is_equal_to_compact_alias, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: PRINT\n4: HALT\n");
    char * expr_english_not_equal_to_alias = selfhost__compiler__disasm_uttrykk("7 not_equal_to 8");
    nl_assert_eq_text(expr_english_not_equal_to_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_not_equal_to_phrase = selfhost__compiler__disasm_uttrykk("7 not equal to 8");
    nl_assert_eq_text(expr_english_not_equal_to_phrase, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_not_equal_to_phrase = selfhost__compiler__disasm_uttrykk("7 is not equal to 8");
    nl_assert_eq_text(expr_english_is_not_equal_to_phrase, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_not_equal_to_compact_alias = selfhost__compiler__disasm_uttrykk("7 is_not_equal_to 8");
    nl_assert_eq_text(expr_english_is_not_equal_to_compact_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_isnt_alias = selfhost__compiler__disasm_uttrykk("7 isnt 8");
    nl_assert_eq_text(expr_english_isnt_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_less_equal_alias = selfhost__compiler__disasm_uttrykk("3 less_equal 4");
    nl_assert_eq_text(expr_english_less_equal_alias, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_greater_equal_alias = selfhost__compiler__disasm_uttrykk("4 greater_equal 4");
    nl_assert_eq_text(expr_english_greater_equal_alias, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_less_than_phrase = selfhost__compiler__disasm_uttrykk("3 less than 4");
    nl_assert_eq_text(expr_english_less_than_phrase, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * expr_english_greater_than_phrase = selfhost__compiler__disasm_uttrykk("4 greater than 3");
    nl_assert_eq_text(expr_english_greater_than_phrase, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * expr_english_is_less_than_phrase = selfhost__compiler__disasm_uttrykk("3 is less than 4");
    nl_assert_eq_text(expr_english_is_less_than_phrase, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * expr_english_is_greater_than_phrase = selfhost__compiler__disasm_uttrykk("4 is greater than 3");
    nl_assert_eq_text(expr_english_is_greater_than_phrase, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * expr_english_less_than_or_equal_phrase = selfhost__compiler__disasm_uttrykk("3 less than or equal 3");
    nl_assert_eq_text(expr_english_less_than_or_equal_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_less_than_or_equal_to_phrase = selfhost__compiler__disasm_uttrykk("3 less than or equal to 3");
    nl_assert_eq_text(expr_english_less_than_or_equal_to_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_greater_than_or_equal_phrase = selfhost__compiler__disasm_uttrykk("4 greater than or equal 4");
    nl_assert_eq_text(expr_english_greater_than_or_equal_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_greater_than_or_equal_to_phrase = selfhost__compiler__disasm_uttrykk("4 greater than or equal to 4");
    nl_assert_eq_text(expr_english_greater_than_or_equal_to_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_less_than_or_equal_to_phrase = selfhost__compiler__disasm_uttrykk("3 is less than or equal to 3");
    nl_assert_eq_text(expr_english_is_less_than_or_equal_to_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_greater_than_or_equal_to_phrase = selfhost__compiler__disasm_uttrykk("4 is greater than or equal to 4");
    nl_assert_eq_text(expr_english_is_greater_than_or_equal_to_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_less_or_equal_phrase = selfhost__compiler__disasm_uttrykk("3 is less or equal 3");
    nl_assert_eq_text(expr_english_is_less_or_equal_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_less_or_equal_to_phrase = selfhost__compiler__disasm_uttrykk("3 is less or equal to 3");
    nl_assert_eq_text(expr_english_is_less_or_equal_to_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_greater_or_equal_phrase = selfhost__compiler__disasm_uttrykk("4 is greater or equal 4");
    nl_assert_eq_text(expr_english_is_greater_or_equal_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_greater_or_equal_to_phrase = selfhost__compiler__disasm_uttrykk("4 is greater or equal to 4");
    nl_assert_eq_text(expr_english_is_greater_or_equal_to_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_less_than_compact_alias = selfhost__compiler__disasm_uttrykk("3 is_less_than 4");
    nl_assert_eq_text(expr_english_is_less_than_compact_alias, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * expr_english_is_greater_or_equal_compact_alias = selfhost__compiler__disasm_uttrykk("4 is_greater_or_equal 4");
    nl_assert_eq_text(expr_english_is_greater_or_equal_compact_alias, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_less_than_or_equal_to_compact_alias = selfhost__compiler__disasm_uttrykk("3 is_less_than_or_equal_to 3");
    nl_assert_eq_text(expr_english_is_less_than_or_equal_to_compact_alias, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_greater_than_or_equal_to_compact_alias = selfhost__compiler__disasm_uttrykk("4 is_greater_than_or_equal_to 4");
    nl_assert_eq_text(expr_english_is_greater_than_or_equal_to_compact_alias, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_less_or_equal_to_compact_alias = selfhost__compiler__disasm_uttrykk("3 is_less_or_equal_to 3");
    nl_assert_eq_text(expr_english_is_less_or_equal_to_compact_alias, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_is_greater_or_equal_to_compact_alias = selfhost__compiler__disasm_uttrykk("4 is_greater_or_equal_to 4");
    nl_assert_eq_text(expr_english_is_greater_or_equal_to_compact_alias, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * expr_english_times_alias = selfhost__compiler__disasm_uttrykk("3 times 4");
    nl_assert_eq_text(expr_english_times_alias, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_english_multiplied_by_alias = selfhost__compiler__disasm_uttrykk("3 multiplied_by 4");
    nl_assert_eq_text(expr_english_multiplied_by_alias, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_english_multiply_by_phrase = selfhost__compiler__disasm_uttrykk("3 multiply by 4");
    nl_assert_eq_text(expr_english_multiply_by_phrase, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_english_multiplied_by_phrase = selfhost__compiler__disasm_uttrykk("3 multiplied by 4");
    nl_assert_eq_text(expr_english_multiplied_by_phrase, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * expr_english_divided_by_alias = selfhost__compiler__disasm_uttrykk("8 divided_by 2");
    nl_assert_eq_text(expr_english_divided_by_alias, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_english_divided_by_phrase = selfhost__compiler__disasm_uttrykk("8 divided by 2");
    nl_assert_eq_text(expr_english_divided_by_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_english_divide_by_phrase = selfhost__compiler__disasm_uttrykk("8 divide by 2");
    nl_assert_eq_text(expr_english_divide_by_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_english_add_alias = selfhost__compiler__disasm_uttrykk("3 add 4");
    nl_assert_eq_text(expr_english_add_alias, "0: PUSH 3\n1: PUSH 4\n2: ADD\n3: PRINT\n4: HALT\n");
    char * expr_english_subtract_alias = selfhost__compiler__disasm_uttrykk("10 subtract 3");
    nl_assert_eq_text(expr_english_subtract_alias, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * expr_english_divide_alias = selfhost__compiler__disasm_uttrykk("8 divide 2");
    nl_assert_eq_text(expr_english_divide_alias, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * expr_english_mod_of_alias = selfhost__compiler__disasm_uttrykk("17 mod_of 5");
    nl_assert_eq_text(expr_english_mod_of_alias, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_english_modulo_of_phrase = selfhost__compiler__disasm_uttrykk("17 modulo of 5");
    nl_assert_eq_text(expr_english_modulo_of_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_english_remainder_alias = selfhost__compiler__disasm_uttrykk("17 remainder 5");
    nl_assert_eq_text(expr_english_remainder_alias, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_english_remainder_of_phrase = selfhost__compiler__disasm_uttrykk("17 remainder of 5");
    nl_assert_eq_text(expr_english_remainder_of_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * expr_hvis_env = selfhost__compiler__disasm_uttrykk_med_miljo("hvis x>y da x ellers y", env_navn, env_verdier);
    nl_assert_eq_text(expr_hvis_env, "0: PUSH 7\n1: PUSH 3\n2: GT\n3: JZ 6\n4: PUSH 7\n5: JMP 8\n6: LABEL 6\n7: PUSH 3\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * expr_hvis_c = selfhost__compiler__kompiler_uttrykk_til_c("hvis 1==1 da 7 ellers 9");
    nl_assert_ne_text(expr_hvis_c, "");
    char * expr_err = selfhost__compiler__disasm_uttrykk("2 +");
    nl_assert_eq_text(expr_err, "/* feil: uttrykket avsluttes med operator ved token 1 */");
    char * expr_err2 = selfhost__compiler__disasm_uttrykk("2 + )");
    nl_assert_eq_text(expr_err2, "/* feil: mangler verdi før ) ved token 2 */");
    char * expr_err3 = selfhost__compiler__disasm_uttrykk("2+$");
    nl_assert_eq_text(expr_err3, "/* feil: ukjent token/navn i uttrykk $ ved token 2 */");
    char * expr_err4 = selfhost__compiler__disasm_uttrykk("foo+1");
    nl_assert_eq_text(expr_err4, "/* feil: ukjent token/navn i uttrykk foo ved token 0 */");
    char * expr_env_err = selfhost__compiler__disasm_uttrykk_med_miljo("z+1", env_navn, env_verdier);
    nl_assert_eq_text(expr_env_err, "/* feil: ukjent token/navn i uttrykk z ved token 0 */");
    nl_list_text* env_navn_kort = nl_list_text_new();
    nl_list_text_push(env_navn_kort, "x");
    nl_list_int* env_verdier_lang = nl_list_int_new();
    nl_list_int_push(env_verdier_lang, 1);
    nl_list_int_push(env_verdier_lang, 2);
    char * expr_env_err2 = selfhost__compiler__disasm_uttrykk_med_miljo("x+1", env_navn_kort, env_verdier_lang);
    nl_assert_eq_text(expr_env_err2, "/* feil: navn/miljø-verdier må ha samme lengde */");
    char * expr_err5 = selfhost__compiler__disasm_uttrykk("(2+3");
    nl_assert_eq_text(expr_err5, "/* feil: mangler ) i uttrykk ved token 0 */");
    char * script_dis = selfhost__compiler__disasm_skript("x=2+3;y=x*4;y+1");
    nl_assert_eq_text(script_dis, "0: PUSH 20\n1: PUSH 1\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_la_dis = selfhost__compiler__disasm_skript("la x=2+3;la y=x*4;y+1");
    nl_assert_eq_text(script_la_dis, "0: PUSH 20\n1: PUSH 1\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_trailing_semicolon = selfhost__compiler__disasm_skript("x=2;y=3;x+y;");
    nl_assert_eq_text(script_trailing_semicolon, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_empty_statements = selfhost__compiler__disasm_skript(";;la x=2;;x+1;;");
    nl_assert_eq_text(script_empty_statements, "0: PUSH 2\n1: PUSH 1\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_if_statement_before_final = selfhost__compiler__disasm_skript("la x=1;hvis x==1 da 10 ellers 20;returner x+2");
    nl_assert_eq_text(script_if_statement_before_final, "0: PUSH 1\n1: PUSH 2\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_expr_statement_before_final = selfhost__compiler__disasm_skript("1+1;la y=2;y");
    nl_assert_eq_text(script_expr_statement_before_final, "0: PUSH 2\n1: PRINT\n2: HALT\n");
    char * script_returner = selfhost__compiler__disasm_skript("la x=2;returner x+3");
    nl_assert_eq_text(script_returner, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_returner_semicolon = selfhost__compiler__disasm_skript("la x=2;returner x+3;");
    nl_assert_eq_text(script_returner_semicolon, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_sett_ok = selfhost__compiler__disasm_skript("la x=2;sett x=x+3;returner x");
    nl_assert_eq_text(script_sett_ok, "0: PUSH 5\n1: PRINT\n2: HALT\n");
    char * script_assign_alias = selfhost__compiler__disasm_skript("let x=2;assign x=x+3;return x");
    nl_assert_eq_text(script_assign_alias, "0: PUSH 5\n1: PRINT\n2: HALT\n");
    char * script_compound_ok = selfhost__compiler__disasm_skript("la x=2;x+=3;x*=2;returner x");
    nl_assert_eq_text(script_compound_ok, "0: PUSH 10\n1: PRINT\n2: HALT\n");
    char * script_hvis_assignment = selfhost__compiler__disasm_skript("la x=hvis 1==1 da 7 ellers 9;returner x+1");
    nl_assert_eq_text(script_hvis_assignment, "0: PUSH 7\n1: PUSH 1\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_hvis = selfhost__compiler__disasm_skript("la x=1;hvis x==1 da 10 ellers 20");
    nl_assert_eq_text(script_hvis, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: JZ 6\n4: PUSH 10\n5: JMP 8\n6: LABEL 6\n7: PUSH 20\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_returner_hvis = selfhost__compiler__disasm_skript("la x=0;returner hvis x==1 da 10 ellers 20");
    nl_assert_eq_text(script_returner_hvis, "0: PUSH 0\n1: PUSH 1\n2: EQ\n3: JZ 6\n4: PUSH 10\n5: JMP 8\n6: LABEL 6\n7: PUSH 20\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_nested_hvis = selfhost__compiler__disasm_skript("la x=1;hvis x==1 da hvis x==1 da 10 ellers 11 ellers 20");
    nl_assert_eq_text(script_nested_hvis, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: JZ 14\n4: PUSH 1\n5: PUSH 1\n6: EQ\n7: JZ 10\n8: PUSH 10\n9: JMP 12\n10: LABEL 10\n11: PUSH 11\n12: LABEL 12\n13: JMP 16\n14: LABEL 14\n15: PUSH 20\n16: LABEL 16\n17: PRINT\n18: HALT\n");
    char * script_if_then_else_alias = selfhost__compiler__disasm_skript("la x=1;if x==1 then 10 else 20");
    nl_assert_eq_text(script_if_then_else_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: JZ 6\n4: PUSH 10\n5: JMP 8\n6: LABEL 6\n7: PUSH 20\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_and_or_not_alias = selfhost__compiler__disasm_skript("la x=sann;la y=usann;returner x and not y or usann");
    nl_assert_eq_text(script_and_or_not_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PUSH 0\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_let_set_return_alias = selfhost__compiler__disasm_skript("let x=2;set x=x+3;return x");
    nl_assert_eq_text(script_let_set_return_alias, "0: PUSH 5\n1: PRINT\n2: HALT\n");
    char * script_const_alias = selfhost__compiler__disasm_skript("const x=2;return x+3");
    nl_assert_eq_text(script_const_alias, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_var_alias = selfhost__compiler__disasm_skript("var x=2;return x+3");
    nl_assert_eq_text(script_var_alias, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_declare_alias = selfhost__compiler__disasm_skript("declare x=2;return x+3");
    nl_assert_eq_text(script_declare_alias, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_return_alias_only = selfhost__compiler__disasm_skript("la x=2;return x+3");
    nl_assert_eq_text(script_return_alias_only, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_english_cmp_alias = selfhost__compiler__disasm_skript("let x=4;let y=4;return x greater_or_equal y");
    nl_assert_eq_text(script_english_cmp_alias, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_less_than_phrase = selfhost__compiler__disasm_skript("let x=3;let y=4;return x less than y");
    nl_assert_eq_text(script_english_less_than_phrase, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * script_english_greater_than_phrase = selfhost__compiler__disasm_skript("let x=4;let y=3;return x greater than y");
    nl_assert_eq_text(script_english_greater_than_phrase, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * script_english_is_less_than_phrase = selfhost__compiler__disasm_skript("let x=3;let y=4;return x is less than y");
    nl_assert_eq_text(script_english_is_less_than_phrase, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * script_english_is_greater_than_phrase = selfhost__compiler__disasm_skript("let x=4;let y=3;return x is greater than y");
    nl_assert_eq_text(script_english_is_greater_than_phrase, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: PRINT\n4: HALT\n");
    char * script_english_less_than_or_equal_phrase = selfhost__compiler__disasm_skript("let x=3;let y=3;return x less than or equal y");
    nl_assert_eq_text(script_english_less_than_or_equal_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_less_than_or_equal_to_phrase = selfhost__compiler__disasm_skript("let x=3;let y=3;return x less than or equal to y");
    nl_assert_eq_text(script_english_less_than_or_equal_to_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_greater_than_or_equal_phrase = selfhost__compiler__disasm_skript("let x=4;let y=4;return x greater than or equal y");
    nl_assert_eq_text(script_english_greater_than_or_equal_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_greater_than_or_equal_to_phrase = selfhost__compiler__disasm_skript("let x=4;let y=4;return x greater than or equal to y");
    nl_assert_eq_text(script_english_greater_than_or_equal_to_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_less_than_or_equal_to_phrase = selfhost__compiler__disasm_skript("let x=3;let y=3;return x is less than or equal to y");
    nl_assert_eq_text(script_english_is_less_than_or_equal_to_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_greater_than_or_equal_to_phrase = selfhost__compiler__disasm_skript("let x=4;let y=4;return x is greater than or equal to y");
    nl_assert_eq_text(script_english_is_greater_than_or_equal_to_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_less_or_equal_phrase = selfhost__compiler__disasm_skript("let x=3;let y=3;return x is less or equal y");
    nl_assert_eq_text(script_english_is_less_or_equal_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_less_or_equal_to_phrase = selfhost__compiler__disasm_skript("let x=3;let y=3;return x is less or equal to y");
    nl_assert_eq_text(script_english_is_less_or_equal_to_phrase, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_greater_or_equal_phrase = selfhost__compiler__disasm_skript("let x=4;let y=4;return x is greater or equal y");
    nl_assert_eq_text(script_english_is_greater_or_equal_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_greater_or_equal_to_phrase = selfhost__compiler__disasm_skript("let x=4;let y=4;return x is greater or equal to y");
    nl_assert_eq_text(script_english_is_greater_or_equal_to_phrase, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_less_than_compact_alias = selfhost__compiler__disasm_skript("let x=3;let y=4;return x is_less_than y");
    nl_assert_eq_text(script_english_is_less_than_compact_alias, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: PRINT\n4: HALT\n");
    char * script_english_is_greater_or_equal_compact_alias = selfhost__compiler__disasm_skript("let x=4;let y=4;return x is_greater_or_equal y");
    nl_assert_eq_text(script_english_is_greater_or_equal_compact_alias, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_less_than_or_equal_to_compact_alias = selfhost__compiler__disasm_skript("let x=3;let y=3;return x is_less_than_or_equal_to y");
    nl_assert_eq_text(script_english_is_less_than_or_equal_to_compact_alias, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_greater_than_or_equal_to_compact_alias = selfhost__compiler__disasm_skript("let x=4;let y=4;return x is_greater_than_or_equal_to y");
    nl_assert_eq_text(script_english_is_greater_than_or_equal_to_compact_alias, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_less_or_equal_to_compact_alias = selfhost__compiler__disasm_skript("let x=3;let y=3;return x is_less_or_equal_to y");
    nl_assert_eq_text(script_english_is_less_or_equal_to_compact_alias, "0: PUSH 3\n1: PUSH 3\n2: GT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_is_greater_or_equal_to_compact_alias = selfhost__compiler__disasm_skript("let x=4;let y=4;return x is_greater_or_equal_to y");
    nl_assert_eq_text(script_english_is_greater_or_equal_to_compact_alias, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_cmp_equal_to_alias = selfhost__compiler__disasm_skript("let x=4;let y=4;return x equal_to y");
    nl_assert_eq_text(script_english_cmp_equal_to_alias, "0: PUSH 4\n1: PUSH 4\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_english_cmp_equal_to_phrase = selfhost__compiler__disasm_skript("let x=4;let y=4;return x equal to y");
    nl_assert_eq_text(script_english_cmp_equal_to_phrase, "0: PUSH 4\n1: PUSH 4\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_english_cmp_is_equal_to_phrase = selfhost__compiler__disasm_skript("let x=4;let y=4;return x is equal to y");
    nl_assert_eq_text(script_english_cmp_is_equal_to_phrase, "0: PUSH 4\n1: PUSH 4\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_english_cmp_is_equal_to_compact_alias = selfhost__compiler__disasm_skript("let x=4;let y=4;return x is_equal_to y");
    nl_assert_eq_text(script_english_cmp_is_equal_to_compact_alias, "0: PUSH 4\n1: PUSH 4\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_english_cmp_is_not_equal_to_phrase = selfhost__compiler__disasm_skript("let x=4;let y=5;return x is not equal to y");
    nl_assert_eq_text(script_english_cmp_is_not_equal_to_phrase, "0: PUSH 4\n1: PUSH 5\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_not_equal_angle_alias = selfhost__compiler__disasm_skript("returner 7<>8");
    nl_assert_eq_text(script_not_equal_angle_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_xor_alias = selfhost__compiler__disasm_skript("returner 1 xor 0");
    nl_assert_eq_text(script_xor_alias, "0: PUSH 1\n1: PUSH 0\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_xnor_alias = selfhost__compiler__disasm_skript("returner 1 xnor 1");
    nl_assert_eq_text(script_xnor_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_nand_operator = selfhost__compiler__disasm_skript("returner 1 nand 1");
    nl_assert_eq_text(script_nand_operator, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_nand_alias_and_not = selfhost__compiler__disasm_skript("returner 1 and_not 1");
    nl_assert_eq_text(script_nand_alias_and_not, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_nor_operator = selfhost__compiler__disasm_skript("returner 0 nor 0");
    nl_assert_eq_text(script_nor_operator, "0: PUSH 0\n1: PUSH 0\n2: OR\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_nor_alias_or_not = selfhost__compiler__disasm_skript("returner 0 or_not 0");
    nl_assert_eq_text(script_nor_alias_or_not, "0: PUSH 0\n1: PUSH 0\n2: OR\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_implies_operator = selfhost__compiler__disasm_skript("returner 1 implies 0");
    nl_assert_eq_text(script_implies_operator, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_implies_that_alias = selfhost__compiler__disasm_skript("returner 1 implies_that 0");
    nl_assert_eq_text(script_implies_that_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_this_implies_alias = selfhost__compiler__disasm_skript("returner 1 this_implies 0");
    nl_assert_eq_text(script_this_implies_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_dette_impliserer_alias = selfhost__compiler__disasm_skript("returner 1 dette_impliserer 0");
    nl_assert_eq_text(script_dette_impliserer_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_impliserer_at_alias = selfhost__compiler__disasm_skript("returner 1 impliserer_at 0");
    nl_assert_eq_text(script_impliserer_at_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_norsk_medforer_alias = selfhost__compiler__disasm_skript("returner 1 medforer 0");
    nl_assert_eq_text(script_norsk_medforer_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_impl_alias = selfhost__compiler__disasm_skript("returner 1 impl 0");
    nl_assert_eq_text(script_impl_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_follows_alias = selfhost__compiler__disasm_skript("returner 1 follows 0");
    nl_assert_eq_text(script_follows_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_it_follows_that_alias = selfhost__compiler__disasm_skript("returner 1 it_follows_that 0");
    nl_assert_eq_text(script_it_follows_that_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_det_folger_at_alias = selfhost__compiler__disasm_skript("returner 1 det_folger_at 0");
    nl_assert_eq_text(script_det_folger_at_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_therefore_alias = selfhost__compiler__disasm_skript("returner 1 therefore 0");
    nl_assert_eq_text(script_therefore_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_hence_alias = selfhost__compiler__disasm_skript("returner 1 hence 0");
    nl_assert_eq_text(script_hence_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_altsa_alias = selfhost__compiler__disasm_skript("returner 1 altsa 0");
    nl_assert_eq_text(script_altsa_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_thus_alias = selfhost__compiler__disasm_skript("returner 1 thus 0");
    nl_assert_eq_text(script_thus_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_dermed_alias = selfhost__compiler__disasm_skript("returner 1 dermed 0");
    nl_assert_eq_text(script_dermed_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_consequently_alias = selfhost__compiler__disasm_skript("returner 1 consequently 0");
    nl_assert_eq_text(script_consequently_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_folgelig_alias = selfhost__compiler__disasm_skript("returner 1 folgelig 0");
    nl_assert_eq_text(script_folgelig_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_thereby_alias = selfhost__compiler__disasm_skript("returner 1 thereby 0");
    nl_assert_eq_text(script_thereby_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_derved_alias = selfhost__compiler__disasm_skript("returner 1 derved 0");
    nl_assert_eq_text(script_derved_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_thereupon_alias = selfhost__compiler__disasm_skript("returner 1 thereupon 0");
    nl_assert_eq_text(script_thereupon_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_derpa_alias = selfhost__compiler__disasm_skript("returner 1 derpa 0");
    nl_assert_eq_text(script_derpa_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_ergo_alias = selfhost__compiler__disasm_skript("returner 1 ergo 0");
    nl_assert_eq_text(script_ergo_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_saledes_alias = selfhost__compiler__disasm_skript("returner 1 saledes 0");
    nl_assert_eq_text(script_saledes_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_infer_alias = selfhost__compiler__disasm_skript("returner 1 infer 0");
    nl_assert_eq_text(script_infer_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_derav_alias = selfhost__compiler__disasm_skript("returner 1 derav 0");
    nl_assert_eq_text(script_derav_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_as_consequence_alias = selfhost__compiler__disasm_skript("returner 1 as_consequence 0");
    nl_assert_eq_text(script_as_consequence_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_som_konsekvens_alias = selfhost__compiler__disasm_skript("returner 1 som_konsekvens 0");
    nl_assert_eq_text(script_som_konsekvens_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_as_a_result_alias = selfhost__compiler__disasm_skript("returner 1 as_a_result 0");
    nl_assert_eq_text(script_as_a_result_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_som_resultat_alias = selfhost__compiler__disasm_skript("returner 1 som_resultat 0");
    nl_assert_eq_text(script_som_resultat_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_only_if_alias = selfhost__compiler__disasm_skript("returner 1 only_if 0");
    nl_assert_eq_text(script_only_if_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_kun_hvis_alias = selfhost__compiler__disasm_skript("returner 1 kun_hvis 0");
    nl_assert_eq_text(script_kun_hvis_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_requires_alias = selfhost__compiler__disasm_skript("returner 1 requires 0");
    nl_assert_eq_text(script_requires_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_krever_alias = selfhost__compiler__disasm_skript("returner 1 krever 0");
    nl_assert_eq_text(script_krever_alias, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_arrow_implies_operator = selfhost__compiler__disasm_skript("returner 1 -> 0");
    nl_assert_eq_text(script_arrow_implies_operator, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_fat_arrow_implies_operator = selfhost__compiler__disasm_skript("returner 1 => 0");
    nl_assert_eq_text(script_fat_arrow_implies_operator, "0: PUSH 1\n1: PUSH 0\n2: SWAP\n3: NOT\n4: SWAP\n5: OR\n6: PRINT\n7: HALT\n");
    char * script_left_arrow_implied_by_operator = selfhost__compiler__disasm_skript("returner 1 <- 0");
    nl_assert_eq_text(script_left_arrow_implied_by_operator, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_implied_by_alias = selfhost__compiler__disasm_skript("returner 1 implied_by 0");
    nl_assert_eq_text(script_implied_by_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_folger_fra_alias = selfhost__compiler__disasm_skript("returner 1 folger_fra 0");
    nl_assert_eq_text(script_folger_fra_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_given_alias = selfhost__compiler__disasm_skript("returner 1 given 0");
    nl_assert_eq_text(script_given_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_gitt_alias = selfhost__compiler__disasm_skript("returner 1 gitt 0");
    nl_assert_eq_text(script_gitt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_provided_alias = selfhost__compiler__disasm_skript("returner 1 provided 0");
    nl_assert_eq_text(script_provided_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_forutsatt_alias = selfhost__compiler__disasm_skript("returner 1 forutsatt 0");
    nl_assert_eq_text(script_forutsatt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_assuming_alias = selfhost__compiler__disasm_skript("returner 1 assuming 0");
    nl_assert_eq_text(script_assuming_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_antar_alias = selfhost__compiler__disasm_skript("returner 1 antar 0");
    nl_assert_eq_text(script_antar_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_assuming_that_alias = selfhost__compiler__disasm_skript("returner 1 assuming_that 0");
    nl_assert_eq_text(script_assuming_that_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_antar_at_alias = selfhost__compiler__disasm_skript("returner 1 antar_at 0");
    nl_assert_eq_text(script_antar_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_when_assuming_alias = selfhost__compiler__disasm_skript("returner 1 when_assuming 0");
    nl_assert_eq_text(script_when_assuming_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_nar_antatt_alias = selfhost__compiler__disasm_skript("returner 1 nar_antatt 0");
    nl_assert_eq_text(script_nar_antatt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_presuming_alias = selfhost__compiler__disasm_skript("returner 1 presuming 0");
    nl_assert_eq_text(script_presuming_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_forutsattvis_alias = selfhost__compiler__disasm_skript("returner 1 forutsattvis 0");
    nl_assert_eq_text(script_forutsattvis_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_since_alias = selfhost__compiler__disasm_skript("returner 1 since 0");
    nl_assert_eq_text(script_since_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_siden_alias = selfhost__compiler__disasm_skript("returner 1 siden 0");
    nl_assert_eq_text(script_siden_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_because_alias = selfhost__compiler__disasm_skript("returner 1 because 0");
    nl_assert_eq_text(script_because_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_fordi_alias = selfhost__compiler__disasm_skript("returner 1 fordi 0");
    nl_assert_eq_text(script_fordi_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_given_that_alias = selfhost__compiler__disasm_skript("returner 1 given_that 0");
    nl_assert_eq_text(script_given_that_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_gitt_at_alias = selfhost__compiler__disasm_skript("returner 1 gitt_at 0");
    nl_assert_eq_text(script_gitt_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_provided_that_alias = selfhost__compiler__disasm_skript("returner 1 provided_that 0");
    nl_assert_eq_text(script_provided_that_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_forutsatt_at_alias = selfhost__compiler__disasm_skript("returner 1 forutsatt_at 0");
    nl_assert_eq_text(script_forutsatt_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_follows_if_alias = selfhost__compiler__disasm_skript("returner 1 follows_if 0");
    nl_assert_eq_text(script_follows_if_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_folger_hvis_alias = selfhost__compiler__disasm_skript("returner 1 folger_hvis 0");
    nl_assert_eq_text(script_folger_hvis_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_if_given_alias = selfhost__compiler__disasm_skript("returner 1 if_given 0");
    nl_assert_eq_text(script_if_given_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_hvis_gitt_alias = selfhost__compiler__disasm_skript("returner 1 hvis_gitt 0");
    nl_assert_eq_text(script_hvis_gitt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_as_alias = selfhost__compiler__disasm_skript("returner 1 as 0");
    nl_assert_eq_text(script_as_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_tersom_alias = selfhost__compiler__disasm_skript("returner 1 ettersom 0");
    nl_assert_eq_text(script_tersom_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_inasmuch_as_alias = selfhost__compiler__disasm_skript("returner 1 inasmuch_as 0");
    nl_assert_eq_text(script_inasmuch_as_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_i_og_med_at_alias = selfhost__compiler__disasm_skript("returner 1 i_og_med_at 0");
    nl_assert_eq_text(script_i_og_med_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_on_condition_that_alias = selfhost__compiler__disasm_skript("returner 1 on_condition_that 0");
    nl_assert_eq_text(script_on_condition_that_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_pa_vilkar_av_at_alias = selfhost__compiler__disasm_skript("returner 1 pa_vilkar_av_at 0");
    nl_assert_eq_text(script_pa_vilkar_av_at_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_granted_alias = selfhost__compiler__disasm_skript("returner 1 granted 0");
    nl_assert_eq_text(script_granted_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_gitt_dette_alias = selfhost__compiler__disasm_skript("returner 1 gitt_dette 0");
    nl_assert_eq_text(script_gitt_dette_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_with_premise_alias = selfhost__compiler__disasm_skript("returner 1 with_premise 0");
    nl_assert_eq_text(script_with_premise_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_med_premiss_alias = selfhost__compiler__disasm_skript("returner 1 med_premiss 0");
    nl_assert_eq_text(script_med_premiss_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_given_premise_alias = selfhost__compiler__disasm_skript("returner 1 given_premise 0");
    nl_assert_eq_text(script_given_premise_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_gitt_premiss_alias = selfhost__compiler__disasm_skript("returner 1 gitt_premiss 0");
    nl_assert_eq_text(script_gitt_premiss_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_premise_given_alias = selfhost__compiler__disasm_skript("returner 1 premise_given 0");
    nl_assert_eq_text(script_premise_given_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_premiss_gitt_alias = selfhost__compiler__disasm_skript("returner 1 premiss_gitt 0");
    nl_assert_eq_text(script_premiss_gitt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_premise_assumed_alias = selfhost__compiler__disasm_skript("returner 1 premise_assumed 0");
    nl_assert_eq_text(script_premise_assumed_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_premiss_antatt_alias = selfhost__compiler__disasm_skript("returner 1 premiss_antatt 0");
    nl_assert_eq_text(script_premiss_antatt_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_premise_condition_alias = selfhost__compiler__disasm_skript("returner 1 premise_condition 0");
    nl_assert_eq_text(script_premise_condition_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_premiss_vilkar_alias = selfhost__compiler__disasm_skript("returner 1 premiss_vilkar 0");
    nl_assert_eq_text(script_premiss_vilkar_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: OR\n4: PRINT\n5: HALT\n");
    char * script_iff_alias = selfhost__compiler__disasm_skript("returner 1 iff 1");
    nl_assert_eq_text(script_iff_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_equiv_alias = selfhost__compiler__disasm_skript("returner 1 equiv 1");
    nl_assert_eq_text(script_equiv_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_if_and_only_if_alias = selfhost__compiler__disasm_skript("returner 1 if_and_only_if 1");
    nl_assert_eq_text(script_if_and_only_if_alias, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_equiv_arrow_operator = selfhost__compiler__disasm_skript("returner 1 <-> 1");
    nl_assert_eq_text(script_equiv_arrow_operator, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_equiv_fat_arrow_operator = selfhost__compiler__disasm_skript("returner 1 <=> 1");
    nl_assert_eq_text(script_equiv_fat_arrow_operator, "0: PUSH 1\n1: PUSH 1\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_english_cmp_is_not_equal_to_compact_alias = selfhost__compiler__disasm_skript("let x=4;let y=5;return x is_not_equal_to y");
    nl_assert_eq_text(script_english_cmp_is_not_equal_to_compact_alias, "0: PUSH 4\n1: PUSH 5\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_cmp_isnt_alias = selfhost__compiler__disasm_skript("let x=4;let y=5;return x isnt y");
    nl_assert_eq_text(script_english_cmp_isnt_alias, "0: PUSH 4\n1: PUSH 5\n2: EQ\n3: NOT\n4: PRINT\n5: HALT\n");
    char * script_english_cmp_eq_alias = selfhost__compiler__disasm_skript("let x=4;let y=4;return x eq y");
    nl_assert_eq_text(script_english_cmp_eq_alias, "0: PUSH 4\n1: PUSH 4\n2: EQ\n3: PRINT\n4: HALT\n");
    char * script_english_on_off_alias = selfhost__compiler__disasm_skript("let x=on;return x and not off");
    nl_assert_eq_text(script_english_on_off_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_yes_no_alias = selfhost__compiler__disasm_skript("let x=yes;return x and not no");
    nl_assert_eq_text(script_english_yes_no_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_enabled_disabled_alias = selfhost__compiler__disasm_skript("let x=enabled;return x and not disabled");
    nl_assert_eq_text(script_english_enabled_disabled_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_active_inactive_alias = selfhost__compiler__disasm_skript("let x=active;return x and not inactive");
    nl_assert_eq_text(script_english_active_inactive_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_truthy_falsy_alias = selfhost__compiler__disasm_skript("let x=truthy;return x and not falsy");
    nl_assert_eq_text(script_english_truthy_falsy_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_affirmative_negative_alias = selfhost__compiler__disasm_skript("let x=affirmative;return x and not negative");
    nl_assert_eq_text(script_english_affirmative_negative_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_affirm_deny_alias = selfhost__compiler__disasm_skript("let x=affirm;return x and not deny");
    nl_assert_eq_text(script_english_affirm_deny_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_approved_rejected_alias = selfhost__compiler__disasm_skript("let x=approved;return x and not rejected");
    nl_assert_eq_text(script_english_approved_rejected_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_accepted_declined_alias = selfhost__compiler__disasm_skript("let x=accepted;return x and not declined");
    nl_assert_eq_text(script_english_accepted_declined_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_confirmed_denied_alias = selfhost__compiler__disasm_skript("let x=confirmed;return x and not denied");
    nl_assert_eq_text(script_english_confirmed_denied_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_pass_fail_alias = selfhost__compiler__disasm_skript("let x=pass;return x and not fail");
    nl_assert_eq_text(script_english_pass_fail_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_success_failure_alias = selfhost__compiler__disasm_skript("let x=success;return x and not failure");
    nl_assert_eq_text(script_english_success_failure_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_allow_block_alias = selfhost__compiler__disasm_skript("let x=allow;return x and not block");
    nl_assert_eq_text(script_english_allow_block_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_permit_forbid_alias = selfhost__compiler__disasm_skript("let x=permit;return x and not forbid");
    nl_assert_eq_text(script_english_permit_forbid_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_valid_invalid_alias = selfhost__compiler__disasm_skript("let x=valid;return x and not invalid");
    nl_assert_eq_text(script_english_valid_invalid_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_ok_not_ok_alias = selfhost__compiler__disasm_skript("let x=ok;return x and not not_ok");
    nl_assert_eq_text(script_english_ok_not_ok_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_ready_not_ready_alias = selfhost__compiler__disasm_skript("let x=ready;return x and not not_ready");
    nl_assert_eq_text(script_english_ready_not_ready_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_safe_unsafe_alias = selfhost__compiler__disasm_skript("let x=safe;return x and not unsafe");
    nl_assert_eq_text(script_english_safe_unsafe_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_secure_insecure_alias = selfhost__compiler__disasm_skript("let x=secure;return x and not insecure");
    nl_assert_eq_text(script_english_secure_insecure_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_trusted_untrusted_alias = selfhost__compiler__disasm_skript("let x=trusted;return x and not untrusted");
    nl_assert_eq_text(script_english_trusted_untrusted_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_open_closed_alias = selfhost__compiler__disasm_skript("let x=open;return x and not closed");
    nl_assert_eq_text(script_english_open_closed_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_public_private_alias = selfhost__compiler__disasm_skript("let x=public;return x and not private");
    nl_assert_eq_text(script_english_public_private_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_visible_hidden_alias = selfhost__compiler__disasm_skript("let x=visible;return x and not hidden");
    nl_assert_eq_text(script_english_visible_hidden_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_present_absent_alias = selfhost__compiler__disasm_skript("let x=present;return x and not absent");
    nl_assert_eq_text(script_english_present_absent_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_online_offline_alias = selfhost__compiler__disasm_skript("let x=online;return x and not offline");
    nl_assert_eq_text(script_english_online_offline_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_connected_disconnected_alias = selfhost__compiler__disasm_skript("let x=connected;return x and not disconnected");
    nl_assert_eq_text(script_english_connected_disconnected_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_available_unavailable_alias = selfhost__compiler__disasm_skript("let x=available;return x and not unavailable");
    nl_assert_eq_text(script_english_available_unavailable_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_reachable_unreachable_alias = selfhost__compiler__disasm_skript("let x=reachable;return x and not unreachable");
    nl_assert_eq_text(script_english_reachable_unreachable_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_working_broken_alias = selfhost__compiler__disasm_skript("let x=working;return x and not broken");
    nl_assert_eq_text(script_english_working_broken_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_stable_unstable_alias = selfhost__compiler__disasm_skript("let x=stable;return x and not unstable");
    nl_assert_eq_text(script_english_stable_unstable_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_correct_incorrect_alias = selfhost__compiler__disasm_skript("let x=correct;return x and not incorrect");
    nl_assert_eq_text(script_english_correct_incorrect_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_complete_incomplete_alias = selfhost__compiler__disasm_skript("let x=complete;return x and not incomplete");
    nl_assert_eq_text(script_english_complete_incomplete_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_clean_dirty_alias = selfhost__compiler__disasm_skript("let x=clean;return x and not dirty");
    nl_assert_eq_text(script_english_clean_dirty_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_up_down_alias = selfhost__compiler__disasm_skript("let x=up;return x and not down");
    nl_assert_eq_text(script_english_up_down_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_alive_dead_alias = selfhost__compiler__disasm_skript("let x=alive;return x and not dead");
    nl_assert_eq_text(script_english_alive_dead_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_awake_asleep_alias = selfhost__compiler__disasm_skript("let x=awake;return x and not asleep");
    nl_assert_eq_text(script_english_awake_asleep_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_healthy_unhealthy_alias = selfhost__compiler__disasm_skript("let x=healthy;return x and not unhealthy");
    nl_assert_eq_text(script_english_healthy_unhealthy_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_synced_unsynced_alias = selfhost__compiler__disasm_skript("let x=synced;return x and not unsynced");
    nl_assert_eq_text(script_english_synced_unsynced_alias, "0: PUSH 1\n1: PUSH 0\n2: NOT\n3: AND\n4: PRINT\n5: HALT\n");
    char * script_english_math_alias = selfhost__compiler__disasm_skript("let x=8;let y=2;return x divided_by y");
    nl_assert_eq_text(script_english_math_alias, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_english_divided_by_phrase = selfhost__compiler__disasm_skript("let x=8;let y=2;return x divided by y");
    nl_assert_eq_text(script_english_divided_by_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_english_divide_by_phrase = selfhost__compiler__disasm_skript("let x=8;let y=2;return x divide by y");
    nl_assert_eq_text(script_english_divide_by_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_english_multiply_by_phrase = selfhost__compiler__disasm_skript("let x=3;let y=4;return x multiply by y");
    nl_assert_eq_text(script_english_multiply_by_phrase, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_english_multiplied_by_phrase = selfhost__compiler__disasm_skript("let x=3;let y=4;return x multiplied by y");
    nl_assert_eq_text(script_english_multiplied_by_phrase, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_english_math_short_alias = selfhost__compiler__disasm_skript("let x=8;let y=2;return x divide y");
    nl_assert_eq_text(script_english_math_short_alias, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_english_modulo_alias = selfhost__compiler__disasm_skript("let x=17;let y=5;return x modulo_of y");
    nl_assert_eq_text(script_english_modulo_alias, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_english_modulo_of_phrase = selfhost__compiler__disasm_skript("let x=17;let y=5;return x modulo of y");
    nl_assert_eq_text(script_english_modulo_of_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_english_remainder_of_phrase = selfhost__compiler__disasm_skript("let x=17;let y=5;return x remainder of y");
    nl_assert_eq_text(script_english_remainder_of_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_ops = selfhost__compiler__disasm_skript("x=sann;y=ikke usann;x og y");
    nl_assert_eq_text(script_norsk_ops, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: PRINT\n4: HALT\n");
    char * script_norsk_ops_enten = selfhost__compiler__disasm_skript("x=usann;y=sann;x enten y");
    nl_assert_eq_text(script_norsk_ops_enten, "0: PUSH 0\n1: PUSH 1\n2: OR\n3: PRINT\n4: HALT\n");
    char * script_norsk_ops_sant = selfhost__compiler__disasm_skript("x=sant;y=ikke usann;x og y");
    nl_assert_eq_text(script_norsk_ops_sant, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: PRINT\n4: HALT\n");
    char * script_norsk_ops_usant = selfhost__compiler__disasm_skript("x=sant;y=ikke usant;x og y");
    nl_assert_eq_text(script_norsk_ops_usant, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: PRINT\n4: HALT\n");
    char * script_norsk_ops_ja_nei = selfhost__compiler__disasm_skript("x=ja;y=ikke nei;x og y");
    nl_assert_eq_text(script_norsk_ops_ja_nei, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: PRINT\n4: HALT\n");
    char * script_norsk_ops_true_false = selfhost__compiler__disasm_skript("x=true;y=ikke false;x og y");
    nl_assert_eq_text(script_norsk_ops_true_false, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: PRINT\n4: HALT\n");
    char * script_norsk_ops_ikkje = selfhost__compiler__disasm_skript("x=sann;y=ikkje usann;x og y");
    nl_assert_eq_text(script_norsk_ops_ikkje, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: PRINT\n4: HALT\n");
    char * script_norsk_ops_samt = selfhost__compiler__disasm_skript("x=sann;y=ikke usann;x samt y");
    nl_assert_eq_text(script_norsk_ops_samt, "0: PUSH 1\n1: PUSH 1\n2: AND\n3: PRINT\n4: HALT\n");
    char * script_norsk_cmp = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x mindre_enn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x mindre enn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase2 = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x er ikke y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase2, "0: PUSH 3\n1: PUSH 4\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase3 = selfhost__compiler__disasm_skript("la x=3;la y=3;hvis x er lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase3, "0: PUSH 3\n1: PUSH 3\n2: EQ\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase4 = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x er mindre enn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase4, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase4_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x er_mindre_enn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase4_underscore, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase4_underscore_lte = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x er_mindre_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase4_underscore_lte, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase4_kompakt_lte = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x ermindreellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase4_kompakt_lte, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase4_underscore_lte_enn = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x er_mindre_enn_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase4_underscore_lte_enn, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase5 = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x ikke lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase5, "0: PUSH 3\n1: PUSH 4\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase6 = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x er ulik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase6, "0: PUSH 3\n1: PUSH 4\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase6_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x er_ulik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase6_underscore, "0: PUSH 3\n1: PUSH 4\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase7 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x storre enn eller lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase7, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase8 = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x er ikke lik med y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase8, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase8_underscore = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ikke_lik_med y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase8_underscore, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase8_ikkelikmed_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ikkelikmed y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase8_ikkelikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase8_alias = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ikke_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase8_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase9 = selfhost__compiler__disasm_skript("la x=7;la y=7;hvis x lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase9, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase9_underscore = selfhost__compiler__disasm_skript("la x=7;la y=7;hvis x er_lik_med y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase9_underscore, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase9_alias = selfhost__compiler__disasm_skript("la x=7;la y=7;hvis x er_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase9_alias, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase9_lik_med_alias = selfhost__compiler__disasm_skript("la x=7;la y=7;hvis x lik_med y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase9_lik_med_alias, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase9_likmed_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=7;hvis x likmed y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase9_likmed_kompakt, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase10 = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ulik med y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase10, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase10_underscore = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ulik_med y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase10_underscore, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase10_ulikmed_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ulikmed y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase10_ulikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase11 = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x mindre lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase11, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x er storre lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_underscore = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x er_storre_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_underscore, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_underscore_storre_lik = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x storre_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_underscore_storre_lik, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_kompakt_gte = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x erstorreellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_kompakt_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_storrelik_kompakt = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x storrelik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_storrelik_kompakt, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_underscore_storre_lik_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x større_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_underscore_storre_lik_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_kompakt_ellerlik_gte_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x erstørreellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_kompakt_ellerlik_gte_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_storrelik_kompakt_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x størrelik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_storrelik_kompakt_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_underscore_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x er_større_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_underscore_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_erstorrelik_kompakt = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x erstorrelik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_erstorrelik_kompakt, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_erstorrelik_kompakt_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x erstørrelik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_erstorrelik_kompakt_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_underscore_gt = selfhost__compiler__disasm_skript("la x=4;la y=3;hvis x er_storre_enn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_underscore_gt, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase12_underscore_gt_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=3;hvis x er_større_enn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_underscore_gt_utf8, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase12_kompakt_gt_ascii = selfhost__compiler__disasm_skript("la x=4;la y=3;hvis x erstorreenn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_kompakt_gt_ascii, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase12_kompakt_gt_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=3;hvis x erstørreenn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_kompakt_gt_utf8, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_phrase12_underscore_gte = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x er_storre_enn_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_underscore_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_underscore_gte_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x er_større_enn_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_underscore_gte_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_kompakt_gte_ascii = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x erstorreennellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_kompakt_gte_ascii, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase12_kompakt_gte_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x erstørreennellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase12_kompakt_gte_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase13 = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x storre enn lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase13, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase14 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x er storre enn lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase14, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_phrase_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x er større enn eller lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_phrase_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_underscore_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x større_enn_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_underscore_utf8, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_underscore_utf8_gt = selfhost__compiler__disasm_skript("la x=4;la y=3;hvis x større_enn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_underscore_utf8_gt, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_storreen_kompakt_ascii = selfhost__compiler__disasm_skript("la x=4;la y=3;hvis x storreenn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_storreen_kompakt_ascii, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_storreen_kompakt_utf8 = selfhost__compiler__disasm_skript("la x=4;la y=3;hvis x størreenn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_storreen_kompakt_utf8, "0: PUSH 4\n1: PUSH 3\n2: GT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_underscore_utf8_gte = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x større_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_underscore_utf8_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_kompakt_utf8_gte = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x størreellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_kompakt_utf8_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_kompakt_ascii_gte = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x storreellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_kompakt_ascii_gte, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_underscore_ascii_storre = selfhost__compiler__disasm_skript("la x=4;la y=4;hvis x storre_enn_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_underscore_ascii_storre, "0: PUSH 4\n1: PUSH 4\n2: LT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_underscore_ascii = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x mindre_enn_eller_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_underscore_ascii, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_kompakt_lte_ascii = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x mindreennellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_kompakt_lte_ascii, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_kompakt_lte_er_ascii = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x ermindreennellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_kompakt_lte_er_ascii, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_kompakt_lt_er_ascii = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x ermindreenn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_kompakt_lt_er_ascii, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_mindreenn_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x mindreenn y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_mindreenn_kompakt, "0: PUSH 3\n1: PUSH 4\n2: LT\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_underscore_ascii_mindre_lik = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x mindre_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_underscore_ascii_mindre_lik, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_ermindrelik_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x ermindrelik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_ermindrelik_kompakt, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_kompakt_ascii_lte = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x mindreellerlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_kompakt_ascii_lte, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_underscore_ascii_mindrelik_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;hvis x mindrelik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_underscore_ascii_mindrelik_kompakt, "0: PUSH 3\n1: PUSH 4\n2: GT\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_er_ikke_alias = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x er_ikke y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_er_ikke_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_erikke_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x erikke y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erikke_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_ikkje_er_alias = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ikkje_er y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_ikkje_er_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_ikkje_lik_alias = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ikkje_lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_ikkje_lik_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_ikkje_lik_med_alias = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ikkje_lik_med y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_ikkje_lik_med_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_ikkjelikmed_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ikkjelikmed y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_ikkjelikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_er_ulik_med_alias = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x er_ulik_med y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_er_ulik_med_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_erlikmed_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=7;hvis x erlikmed y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erlikmed_kompakt, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_erulikmed_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x erulikmed y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erulikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_erikkelikmed_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x erikkelikmed y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erikkelikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_er_ikkje_alias = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x er_ikkje y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_er_ikkje_alias, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_erlik_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=7;hvis x erlik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erlik_kompakt, "0: PUSH 7\n1: PUSH 7\n2: EQ\n3: JZ 6\n4: PUSH 1\n5: JMP 8\n6: LABEL 6\n7: PUSH 0\n8: LABEL 8\n9: PRINT\n10: HALT\n");
    char * script_norsk_cmp_erulik_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x erulik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erulik_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_erikkje_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x erikkje y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erikkje_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_erikkelik_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x erikkelik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erikkelik_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_erikkjelik_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x erikkjelik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erikkjelik_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_erikkjelikmed_kompakt = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x erikkjelikmed y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_erikkjelikmed_kompakt, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_ikkje_phrase = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x er ikkje y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_ikkje_phrase, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_norsk_cmp_ikkje_lik_phrase = selfhost__compiler__disasm_skript("la x=7;la y=8;hvis x ikkje lik y da 1 ellers 0");
    nl_assert_eq_text(script_norsk_cmp_ikkje_lik_phrase, "0: PUSH 7\n1: PUSH 8\n2: EQ\n3: NOT\n4: JZ 7\n5: PUSH 1\n6: JMP 9\n7: LABEL 7\n8: PUSH 0\n9: LABEL 9\n10: PRINT\n11: HALT\n");
    char * script_unary_plus = selfhost__compiler__disasm_skript("la x=2;returner +x");
    nl_assert_eq_text(script_unary_plus, "0: PUSH 2\n1: PRINT\n2: HALT\n");
    char * script_norsk_arith = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x pluss y ganger 4");
    nl_assert_eq_text(script_norsk_arith, "0: PUSH 2\n1: PUSH 3\n2: PUSH 4\n3: MUL\n4: ADD\n5: PRINT\n6: HALT\n");
    char * script_norsk_legg_til = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legg til y");
    nl_assert_eq_text(script_norsk_legg_til, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legg_til_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legg_til y");
    nl_assert_eq_text(script_norsk_legg_til_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legge_til = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legge til y");
    nl_assert_eq_text(script_norsk_legge_til, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legge_til_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legge_til y");
    nl_assert_eq_text(script_norsk_legge_til_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legges_til = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legges til y");
    nl_assert_eq_text(script_norsk_legges_til, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legges_til_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legges_til y");
    nl_assert_eq_text(script_norsk_legges_til_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_leggtil_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x leggtil y");
    nl_assert_eq_text(script_norsk_leggtil_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_leggetil_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x leggetil y");
    nl_assert_eq_text(script_norsk_leggetil_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_leggestil_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x leggestil y");
    nl_assert_eq_text(script_norsk_leggestil_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legg_sammen = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legg sammen y");
    nl_assert_eq_text(script_norsk_legg_sammen, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legg_sammen_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legg_sammen y");
    nl_assert_eq_text(script_norsk_legg_sammen_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legge_sammen = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legge sammen y");
    nl_assert_eq_text(script_norsk_legge_sammen, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legge_sammen_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legge_sammen y");
    nl_assert_eq_text(script_norsk_legge_sammen_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legges_sammen = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legges sammen y");
    nl_assert_eq_text(script_norsk_legges_sammen, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legges_sammen_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legges_sammen y");
    nl_assert_eq_text(script_norsk_legges_sammen_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_leggsammen_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x leggsammen y");
    nl_assert_eq_text(script_norsk_leggsammen_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_leggesammen_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x leggesammen y");
    nl_assert_eq_text(script_norsk_leggesammen_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_leggessammen_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x leggessammen y");
    nl_assert_eq_text(script_norsk_leggessammen_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legg_kort = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legg y");
    nl_assert_eq_text(script_norsk_legg_kort, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legge_kort = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legge y");
    nl_assert_eq_text(script_norsk_legge_kort, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_legges_kort = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x legges y");
    nl_assert_eq_text(script_norsk_legges_kort, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_pluss_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x pluss med y");
    nl_assert_eq_text(script_norsk_pluss_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_pluss_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x pluss_med y");
    nl_assert_eq_text(script_norsk_pluss_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plussmed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plussmed y");
    nl_assert_eq_text(script_norsk_plussmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plus = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plus y");
    nl_assert_eq_text(script_norsk_plus, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plus_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plus med y");
    nl_assert_eq_text(script_norsk_plus_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plus_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plus_med y");
    nl_assert_eq_text(script_norsk_plus_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plusmed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plusmed y");
    nl_assert_eq_text(script_norsk_plusmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plusser_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plusser med y");
    nl_assert_eq_text(script_norsk_plusser_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plusser_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plusser_med y");
    nl_assert_eq_text(script_norsk_plusser_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plussermed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plussermed y");
    nl_assert_eq_text(script_norsk_plussermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plusses_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plusses med y");
    nl_assert_eq_text(script_norsk_plusses_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plusses_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plusses_med y");
    nl_assert_eq_text(script_norsk_plusses_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plussesmed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plussesmed y");
    nl_assert_eq_text(script_norsk_plussesmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_pluses_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x pluses med y");
    nl_assert_eq_text(script_norsk_pluses_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_pluses_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x pluses_med y");
    nl_assert_eq_text(script_norsk_pluses_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plusesmed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plusesmed y");
    nl_assert_eq_text(script_norsk_plusesmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plusse_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plusse med y");
    nl_assert_eq_text(script_norsk_plusse_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plusse_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plusse_med y");
    nl_assert_eq_text(script_norsk_plusse_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plussemed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plussemed y");
    nl_assert_eq_text(script_norsk_plussemed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_plusser = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x plusser y");
    nl_assert_eq_text(script_norsk_plusser, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adder_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adder med y");
    nl_assert_eq_text(script_norsk_adder_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adder_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adder_med y");
    nl_assert_eq_text(script_norsk_adder_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_addermed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x addermed y");
    nl_assert_eq_text(script_norsk_addermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_addere_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x addere med y");
    nl_assert_eq_text(script_norsk_addere_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_addere_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x addere_med y");
    nl_assert_eq_text(script_norsk_addere_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adderemed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adderemed y");
    nl_assert_eq_text(script_norsk_adderemed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adderer_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adderer med y");
    nl_assert_eq_text(script_norsk_adderer_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adderer_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adderer_med y");
    nl_assert_eq_text(script_norsk_adderer_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adderermed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adderermed y");
    nl_assert_eq_text(script_norsk_adderermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adderes_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adderes med y");
    nl_assert_eq_text(script_norsk_adderes_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adderes_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adderes_med y");
    nl_assert_eq_text(script_norsk_adderes_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adderesmed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adderesmed y");
    nl_assert_eq_text(script_norsk_adderesmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_adder = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x adder y");
    nl_assert_eq_text(script_norsk_adder, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summer_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summer med y");
    nl_assert_eq_text(script_norsk_summer_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summer_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summer_med y");
    nl_assert_eq_text(script_norsk_summer_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summermed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summermed y");
    nl_assert_eq_text(script_norsk_summermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summerer_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summerer med y");
    nl_assert_eq_text(script_norsk_summerer_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summerer_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summerer_med y");
    nl_assert_eq_text(script_norsk_summerer_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summerermed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summerermed y");
    nl_assert_eq_text(script_norsk_summerermed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summeres_med = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summeres med y");
    nl_assert_eq_text(script_norsk_summeres_med, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summeres_med_underscore = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summeres_med y");
    nl_assert_eq_text(script_norsk_summeres_med_underscore, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summeresmed_kompakt = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summeresmed y");
    nl_assert_eq_text(script_norsk_summeresmed_kompakt, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_summer = selfhost__compiler__disasm_skript("la x=2;la y=3;returner x summer y");
    nl_assert_eq_text(script_norsk_summer, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekk_fra = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekk fra y");
    nl_assert_eq_text(script_norsk_trekk_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekk_fra_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekk_fra y");
    nl_assert_eq_text(script_norsk_trekk_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkfra_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkfra y");
    nl_assert_eq_text(script_norsk_trekkfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekke_fra_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekke_fra y");
    nl_assert_eq_text(script_norsk_trekke_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkefra_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkefra y");
    nl_assert_eq_text(script_norsk_trekkefra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkes_fra = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkes fra y");
    nl_assert_eq_text(script_norsk_trekkes_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkes_fra_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkes_fra y");
    nl_assert_eq_text(script_norsk_trekkes_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkesfra_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkesfra y");
    nl_assert_eq_text(script_norsk_trekkesfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minus_fra = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minus fra y");
    nl_assert_eq_text(script_norsk_minus_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minus_fra_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minus_fra y");
    nl_assert_eq_text(script_norsk_minus_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minusfra_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minusfra y");
    nl_assert_eq_text(script_norsk_minusfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minuseres_fra = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minuseres fra y");
    nl_assert_eq_text(script_norsk_minuseres_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minuseres_fra_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minuseres_fra y");
    nl_assert_eq_text(script_norsk_minuseres_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minuseresfra_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minuseresfra y");
    nl_assert_eq_text(script_norsk_minuseresfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraher_fra = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraher fra y");
    nl_assert_eq_text(script_norsk_subtraher_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraher_fra_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraher_fra y");
    nl_assert_eq_text(script_norsk_subtraher_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraherfra_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraherfra y");
    nl_assert_eq_text(script_norsk_subtraherfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraherer_fra = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraherer fra y");
    nl_assert_eq_text(script_norsk_subtraherer_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraherer_fra_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraherer_fra y");
    nl_assert_eq_text(script_norsk_subtraherer_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtrahererfra_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtrahererfra y");
    nl_assert_eq_text(script_norsk_subtrahererfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraheres_fra = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraheres fra y");
    nl_assert_eq_text(script_norsk_subtraheres_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraheres_fra_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraheres_fra y");
    nl_assert_eq_text(script_norsk_subtraheres_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraheresfra_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraheresfra y");
    nl_assert_eq_text(script_norsk_subtraheresfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtrahert_fra = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtrahert fra y");
    nl_assert_eq_text(script_norsk_subtrahert_fra, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtrahert_fra_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtrahert_fra y");
    nl_assert_eq_text(script_norsk_subtrahert_fra_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtrahertfra_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtrahertfra y");
    nl_assert_eq_text(script_norsk_subtrahertfra_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minus_med = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minus med y");
    nl_assert_eq_text(script_norsk_minus_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minus_med_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minus_med y");
    nl_assert_eq_text(script_norsk_minus_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minusmed_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minusmed y");
    nl_assert_eq_text(script_norsk_minusmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minuseres_med = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minuseres med y");
    nl_assert_eq_text(script_norsk_minuseres_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minuseres_med_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minuseres_med y");
    nl_assert_eq_text(script_norsk_minuseres_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_minuseresmed_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x minuseresmed y");
    nl_assert_eq_text(script_norsk_minuseresmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekk_med = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekk med y");
    nl_assert_eq_text(script_norsk_trekk_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekk_med_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekk_med y");
    nl_assert_eq_text(script_norsk_trekk_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkmed_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkmed y");
    nl_assert_eq_text(script_norsk_trekkmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekke_med = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekke med y");
    nl_assert_eq_text(script_norsk_trekke_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekke_med_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekke_med y");
    nl_assert_eq_text(script_norsk_trekke_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkemed_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkemed y");
    nl_assert_eq_text(script_norsk_trekkemed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekk = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekk y");
    nl_assert_eq_text(script_norsk_trekk, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekke = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekke y");
    nl_assert_eq_text(script_norsk_trekke, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraher = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraher y");
    nl_assert_eq_text(script_norsk_subtraher, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtrahert = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtrahert y");
    nl_assert_eq_text(script_norsk_subtrahert, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkes_med = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkes med y");
    nl_assert_eq_text(script_norsk_trekkes_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkes_med_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkes_med y");
    nl_assert_eq_text(script_norsk_trekkes_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_trekkesmed_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x trekkesmed y");
    nl_assert_eq_text(script_norsk_trekkesmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraher_med = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraher med y");
    nl_assert_eq_text(script_norsk_subtraher_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraher_med_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraher_med y");
    nl_assert_eq_text(script_norsk_subtraher_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtrahermed_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtrahermed y");
    nl_assert_eq_text(script_norsk_subtrahermed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraherer_med = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraherer med y");
    nl_assert_eq_text(script_norsk_subtraherer_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraherer_med_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraherer_med y");
    nl_assert_eq_text(script_norsk_subtraherer_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraherermed_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraherermed y");
    nl_assert_eq_text(script_norsk_subtraherermed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraheres_med = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraheres med y");
    nl_assert_eq_text(script_norsk_subtraheres_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraheres_med_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraheres_med y");
    nl_assert_eq_text(script_norsk_subtraheres_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtraheresmed_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtraheresmed y");
    nl_assert_eq_text(script_norsk_subtraheresmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtrahert_med = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtrahert med y");
    nl_assert_eq_text(script_norsk_subtrahert_med, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtrahert_med_underscore = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtrahert_med y");
    nl_assert_eq_text(script_norsk_subtrahert_med_underscore, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_subtrahertmed_kompakt = selfhost__compiler__disasm_skript("la x=10;la y=3;returner x subtrahertmed y");
    nl_assert_eq_text(script_norsk_subtrahertmed_kompakt, "0: PUSH 10\n1: PUSH 3\n2: SUB\n3: PRINT\n4: HALT\n");
    char * script_norsk_mod = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x mod y");
    nl_assert_eq_text(script_norsk_mod, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_mod_av = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x mod_av y");
    nl_assert_eq_text(script_norsk_mod_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modav = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modav y");
    nl_assert_eq_text(script_norsk_modav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_mod_av_phrase = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x mod av y");
    nl_assert_eq_text(script_norsk_mod_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modulo_av = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modulo_av y");
    nl_assert_eq_text(script_norsk_modulo_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_moduloav = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x moduloav y");
    nl_assert_eq_text(script_norsk_moduloav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modulo_av_phrase = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modulo av y");
    nl_assert_eq_text(script_norsk_modulo_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modul = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modul y");
    nl_assert_eq_text(script_norsk_modul, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modul_av = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modul_av y");
    nl_assert_eq_text(script_norsk_modul_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modulav = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modulav y");
    nl_assert_eq_text(script_norsk_modulav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modul_av_phrase = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modul av y");
    nl_assert_eq_text(script_norsk_modul_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modulus = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modulus y");
    nl_assert_eq_text(script_norsk_modulus, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modulus_av = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modulus_av y");
    nl_assert_eq_text(script_norsk_modulus_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modulusav = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modulusav y");
    nl_assert_eq_text(script_norsk_modulusav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_modulus_av_phrase = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x modulus av y");
    nl_assert_eq_text(script_norsk_modulus_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_rest = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x rest y");
    nl_assert_eq_text(script_norsk_rest, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_resten = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x resten y");
    nl_assert_eq_text(script_norsk_resten, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_rest_av = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x rest_av y");
    nl_assert_eq_text(script_norsk_rest_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_restav = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x restav y");
    nl_assert_eq_text(script_norsk_restav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_rest_av_phrase = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x rest av y");
    nl_assert_eq_text(script_norsk_rest_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_resten_av = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x resten_av y");
    nl_assert_eq_text(script_norsk_resten_av, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_restenav = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x restenav y");
    nl_assert_eq_text(script_norsk_restenav, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_resten_av_phrase = selfhost__compiler__disasm_skript("la x=17;la y=5;returner x resten av y");
    nl_assert_eq_text(script_norsk_resten_av_phrase, "0: PUSH 17\n1: PUSH 5\n2: MOD\n3: PRINT\n4: HALT\n");
    char * script_norsk_ganget_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x ganget med y");
    nl_assert_eq_text(script_norsk_ganget_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_ganget_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x ganget_med y");
    nl_assert_eq_text(script_norsk_ganget_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gange_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gange med y");
    nl_assert_eq_text(script_norsk_gange_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gange_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gange_med y");
    nl_assert_eq_text(script_norsk_gange_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gangemed_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gangemed y");
    nl_assert_eq_text(script_norsk_gangemed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_ganger_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x ganger med y");
    nl_assert_eq_text(script_norsk_ganger_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_ganger_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x ganger_med y");
    nl_assert_eq_text(script_norsk_ganger_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gangermed_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gangermed y");
    nl_assert_eq_text(script_norsk_gangermed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_ganges_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x ganges med y");
    nl_assert_eq_text(script_norsk_ganges_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_ganges_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x ganges_med y");
    nl_assert_eq_text(script_norsk_ganges_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gangesmed_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gangesmed y");
    nl_assert_eq_text(script_norsk_gangesmed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gang_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gang med y");
    nl_assert_eq_text(script_norsk_gang_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gang_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gang_med y");
    nl_assert_eq_text(script_norsk_gang_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gangmed_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gangmed y");
    nl_assert_eq_text(script_norsk_gangmed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gang = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gang y");
    nl_assert_eq_text(script_norsk_gang, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_gange = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x gange y");
    nl_assert_eq_text(script_norsk_gange, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_ganget = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x ganget y");
    nl_assert_eq_text(script_norsk_ganget, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_ganger = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x ganger y");
    nl_assert_eq_text(script_norsk_ganger, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_ganges = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x ganges y");
    nl_assert_eq_text(script_norsk_ganges, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliser_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliser med y");
    nl_assert_eq_text(script_norsk_multipliser_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliser_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliser_med y");
    nl_assert_eq_text(script_norsk_multipliser_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multiplisermed_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multiplisermed y");
    nl_assert_eq_text(script_norsk_multiplisermed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multiplisere_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multiplisere med y");
    nl_assert_eq_text(script_norsk_multiplisere_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multiplisere_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multiplisere_med y");
    nl_assert_eq_text(script_norsk_multiplisere_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliseremed_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliseremed y");
    nl_assert_eq_text(script_norsk_multipliseremed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliserer_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliserer med y");
    nl_assert_eq_text(script_norsk_multipliserer_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliserer_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliserer_med y");
    nl_assert_eq_text(script_norsk_multipliserer_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliserermed_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliserermed y");
    nl_assert_eq_text(script_norsk_multipliserermed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multiplisert_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multiplisert med y");
    nl_assert_eq_text(script_norsk_multiplisert_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multiplisert_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multiplisert_med y");
    nl_assert_eq_text(script_norsk_multiplisert_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multiplisertmed_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multiplisertmed y");
    nl_assert_eq_text(script_norsk_multiplisertmed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliseres_med = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliseres med y");
    nl_assert_eq_text(script_norsk_multipliseres_med, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliseres_med_underscore = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliseres_med y");
    nl_assert_eq_text(script_norsk_multipliseres_med_underscore, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliseresmed_kompakt = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliseresmed y");
    nl_assert_eq_text(script_norsk_multipliseresmed_kompakt, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliser_kort = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliser y");
    nl_assert_eq_text(script_norsk_multipliser_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multiplisere_kort = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multiplisere y");
    nl_assert_eq_text(script_norsk_multiplisere_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliserer_kort = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliserer y");
    nl_assert_eq_text(script_norsk_multipliserer_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multiplisert_kort = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multiplisert y");
    nl_assert_eq_text(script_norsk_multiplisert_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_multipliseres_kort = selfhost__compiler__disasm_skript("la x=3;la y=4;returner x multipliseres y");
    nl_assert_eq_text(script_norsk_multipliseres_kort, "0: PUSH 3\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_delt_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delt med y");
    nl_assert_eq_text(script_norsk_delt_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delt_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delt_med y");
    nl_assert_eq_text(script_norsk_delt_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deltmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deltmed y");
    nl_assert_eq_text(script_norsk_deltmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider med y");
    nl_assert_eq_text(script_norsk_divider_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider_med y");
    nl_assert_eq_text(script_norsk_divider_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividermed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividermed y");
    nl_assert_eq_text(script_norsk_dividermed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere med y");
    nl_assert_eq_text(script_norsk_dividere_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere_med y");
    nl_assert_eq_text(script_norsk_dividere_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideremed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideremed y");
    nl_assert_eq_text(script_norsk_divideremed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_seg_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele seg med y");
    nl_assert_eq_text(script_norsk_dele_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_seg_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele_seg_med y");
    nl_assert_eq_text(script_norsk_dele_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delesegmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delesegmed y");
    nl_assert_eq_text(script_norsk_delesegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_seg_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler seg med y");
    nl_assert_eq_text(script_norsk_deler_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_seg_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler_seg_med y");
    nl_assert_eq_text(script_norsk_deler_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delersegmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delersegmed y");
    nl_assert_eq_text(script_norsk_delersegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_seg_pa = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele seg pa y");
    nl_assert_eq_text(script_norsk_dele_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_seg_paa_phrase_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele seg på y");
    nl_assert_eq_text(script_norsk_dele_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_seg_paa_phrase_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele seg paa y");
    nl_assert_eq_text(script_norsk_dele_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_seg_pa_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele_seg_pa y");
    nl_assert_eq_text(script_norsk_dele_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delesegpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delesegpa y");
    nl_assert_eq_text(script_norsk_delesegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delesegpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delesegpaa y");
    nl_assert_eq_text(script_norsk_delesegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_seg_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele_seg_på y");
    nl_assert_eq_text(script_norsk_dele_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_seg_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele_seg_paa y");
    nl_assert_eq_text(script_norsk_dele_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_seg_pa = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler seg pa y");
    nl_assert_eq_text(script_norsk_deler_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_seg_paa_phrase_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler seg på y");
    nl_assert_eq_text(script_norsk_deler_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_seg_paa_phrase_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler seg paa y");
    nl_assert_eq_text(script_norsk_deler_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_seg_pa_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler_seg_pa y");
    nl_assert_eq_text(script_norsk_deler_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delersegpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delersegpa y");
    nl_assert_eq_text(script_norsk_delersegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delersegpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delersegpaa y");
    nl_assert_eq_text(script_norsk_delersegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_seg_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler_seg_på y");
    nl_assert_eq_text(script_norsk_deler_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_seg_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler_seg_paa y");
    nl_assert_eq_text(script_norsk_deler_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_seg_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider seg med y");
    nl_assert_eq_text(script_norsk_divider_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_seg_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider_seg_med y");
    nl_assert_eq_text(script_norsk_divider_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividersegmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividersegmed y");
    nl_assert_eq_text(script_norsk_dividersegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_seg_pa = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider seg pa y");
    nl_assert_eq_text(script_norsk_divider_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_seg_paa_phrase_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider seg på y");
    nl_assert_eq_text(script_norsk_divider_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_seg_paa_phrase_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider seg paa y");
    nl_assert_eq_text(script_norsk_divider_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_seg_pa_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider_seg_pa y");
    nl_assert_eq_text(script_norsk_divider_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividersegpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividersegpa y");
    nl_assert_eq_text(script_norsk_dividersegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividersegpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividersegpaa y");
    nl_assert_eq_text(script_norsk_dividersegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_seg_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider_seg_på y");
    nl_assert_eq_text(script_norsk_divider_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_seg_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider_seg_paa y");
    nl_assert_eq_text(script_norsk_divider_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_seg_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere seg med y");
    nl_assert_eq_text(script_norsk_dividere_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_seg_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere_seg_med y");
    nl_assert_eq_text(script_norsk_dividere_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideresegmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideresegmed y");
    nl_assert_eq_text(script_norsk_divideresegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_seg_pa = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere seg pa y");
    nl_assert_eq_text(script_norsk_dividere_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_seg_paa_phrase_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere seg på y");
    nl_assert_eq_text(script_norsk_dividere_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_seg_paa_phrase_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere seg paa y");
    nl_assert_eq_text(script_norsk_dividere_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_seg_pa_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere_seg_pa y");
    nl_assert_eq_text(script_norsk_dividere_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideresegpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideresegpa y");
    nl_assert_eq_text(script_norsk_divideresegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideresegpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideresegpaa y");
    nl_assert_eq_text(script_norsk_divideresegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_seg_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere_seg_på y");
    nl_assert_eq_text(script_norsk_dividere_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_seg_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere_seg_paa y");
    nl_assert_eq_text(script_norsk_dividere_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_seg_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer seg med y");
    nl_assert_eq_text(script_norsk_dividerer_seg_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_seg_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer_seg_med y");
    nl_assert_eq_text(script_norsk_dividerer_seg_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerersegmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerersegmed y");
    nl_assert_eq_text(script_norsk_dividerersegmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_seg_pa = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer seg pa y");
    nl_assert_eq_text(script_norsk_dividerer_seg_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_seg_paa_phrase_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer seg på y");
    nl_assert_eq_text(script_norsk_dividerer_seg_paa_phrase_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_seg_paa_phrase_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer seg paa y");
    nl_assert_eq_text(script_norsk_dividerer_seg_paa_phrase_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_seg_pa_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer_seg_pa y");
    nl_assert_eq_text(script_norsk_dividerer_seg_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerersegpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerersegpa y");
    nl_assert_eq_text(script_norsk_dividerersegpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerersegpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerersegpaa y");
    nl_assert_eq_text(script_norsk_dividerersegpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_seg_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer_seg_på y");
    nl_assert_eq_text(script_norsk_dividerer_seg_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_seg_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer_seg_paa y");
    nl_assert_eq_text(script_norsk_dividerer_seg_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer med y");
    nl_assert_eq_text(script_norsk_dividerer_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer_med y");
    nl_assert_eq_text(script_norsk_dividerer_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerermed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerermed y");
    nl_assert_eq_text(script_norsk_dividerermed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividert_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividert med y");
    nl_assert_eq_text(script_norsk_dividert_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividert_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividert_med y");
    nl_assert_eq_text(script_norsk_dividert_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividertmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividertmed y");
    nl_assert_eq_text(script_norsk_dividertmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideres_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideres med y");
    nl_assert_eq_text(script_norsk_divideres_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideres_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideres_med y");
    nl_assert_eq_text(script_norsk_divideres_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideresmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideresmed y");
    nl_assert_eq_text(script_norsk_divideresmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deles_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deles med y");
    nl_assert_eq_text(script_norsk_deles_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deles_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deles_med y");
    nl_assert_eq_text(script_norsk_deles_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delesmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delesmed y");
    nl_assert_eq_text(script_norsk_delesmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_del_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x del med y");
    nl_assert_eq_text(script_norsk_del_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_del_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x del_med y");
    nl_assert_eq_text(script_norsk_del_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delmed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delmed y");
    nl_assert_eq_text(script_norsk_delmed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_brace_group_assignment = selfhost__compiler__disasm_skript("la x={2+3};returner x*4");
    nl_assert_eq_text(script_brace_group_assignment, "0: PUSH 5\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_bracket_group_assignment = selfhost__compiler__disasm_skript("la x=[2+3];returner x*4");
    nl_assert_eq_text(script_bracket_group_assignment, "0: PUSH 5\n1: PUSH 4\n2: MUL\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele med y");
    nl_assert_eq_text(script_norsk_dele_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele_med y");
    nl_assert_eq_text(script_norsk_dele_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delemed_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delemed y");
    nl_assert_eq_text(script_norsk_delemed_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_paa_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele på y");
    nl_assert_eq_text(script_norsk_dele_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_paa_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele pa y");
    nl_assert_eq_text(script_norsk_dele_paa_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_paa_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele_pa y");
    nl_assert_eq_text(script_norsk_dele_paa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delepa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delepa y");
    nl_assert_eq_text(script_norsk_delepa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delepaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delepaa y");
    nl_assert_eq_text(script_norsk_delepaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_del_paa_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x del på y");
    nl_assert_eq_text(script_norsk_del_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_del_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x del_på y");
    nl_assert_eq_text(script_norsk_del_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_del_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x del_pa y");
    nl_assert_eq_text(script_norsk_del_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delpa y");
    nl_assert_eq_text(script_norsk_delpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delpaa y");
    nl_assert_eq_text(script_norsk_delpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_del_paa_double_a_phrase = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x del paa y");
    nl_assert_eq_text(script_norsk_del_paa_double_a_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_del_paa_double_a_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x del_paa y");
    nl_assert_eq_text(script_norsk_del_paa_double_a_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deles_paa_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deles på y");
    nl_assert_eq_text(script_norsk_deles_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deles_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deles_på y");
    nl_assert_eq_text(script_norsk_deles_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deles_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deles_pa y");
    nl_assert_eq_text(script_norsk_deles_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delespa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delespa y");
    nl_assert_eq_text(script_norsk_delespa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delespaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delespaa y");
    nl_assert_eq_text(script_norsk_delespaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deles_paa_double_a_phrase = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deles paa y");
    nl_assert_eq_text(script_norsk_deles_paa_double_a_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deles_paa_double_a_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deles_paa y");
    nl_assert_eq_text(script_norsk_deles_paa_double_a_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_paa_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider på y");
    nl_assert_eq_text(script_norsk_divider_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider_pa y");
    nl_assert_eq_text(script_norsk_divider_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerpa y");
    nl_assert_eq_text(script_norsk_dividerpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerpaa y");
    nl_assert_eq_text(script_norsk_dividerpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_paa_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere på y");
    nl_assert_eq_text(script_norsk_dividere_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere_pa y");
    nl_assert_eq_text(script_norsk_dividere_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerepa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerepa y");
    nl_assert_eq_text(script_norsk_dividerepa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerepaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerepaa y");
    nl_assert_eq_text(script_norsk_dividerepaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_paa_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer på y");
    nl_assert_eq_text(script_norsk_dividerer_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerer_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerer_pa y");
    nl_assert_eq_text(script_norsk_dividerer_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividererpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividererpa y");
    nl_assert_eq_text(script_norsk_dividererpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividererpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividererpaa y");
    nl_assert_eq_text(script_norsk_dividererpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividert_paa_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividert på y");
    nl_assert_eq_text(script_norsk_dividert_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividert_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividert_pa y");
    nl_assert_eq_text(script_norsk_dividert_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividertpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividertpa y");
    nl_assert_eq_text(script_norsk_dividertpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividertpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividertpaa y");
    nl_assert_eq_text(script_norsk_dividertpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideres_paa_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideres på y");
    nl_assert_eq_text(script_norsk_divideres_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideres_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideres_på y");
    nl_assert_eq_text(script_norsk_divideres_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideres_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideres_pa y");
    nl_assert_eq_text(script_norsk_divideres_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerespa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerespa y");
    nl_assert_eq_text(script_norsk_dividerespa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividerespaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividerespaa y");
    nl_assert_eq_text(script_norsk_dividerespaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideres_paa_double_a_phrase = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideres paa y");
    nl_assert_eq_text(script_norsk_divideres_paa_double_a_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divideres_paa_double_a_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divideres_paa y");
    nl_assert_eq_text(script_norsk_divideres_paa_double_a_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delt_paa_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delt på y");
    nl_assert_eq_text(script_norsk_delt_paa_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delt_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delt_på y");
    nl_assert_eq_text(script_norsk_delt_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delt_paa_underscore_ascii = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delt_pa y");
    nl_assert_eq_text(script_norsk_delt_paa_underscore_ascii, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deltpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deltpa y");
    nl_assert_eq_text(script_norsk_deltpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deltpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deltpaa y");
    nl_assert_eq_text(script_norsk_deltpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delt_paa_double_a_phrase = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delt paa y");
    nl_assert_eq_text(script_norsk_delt_paa_double_a_phrase, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delt_paa_double_a_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delt_paa y");
    nl_assert_eq_text(script_norsk_delt_paa_double_a_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_pa = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler pa y");
    nl_assert_eq_text(script_norsk_deler_pa, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_paa_underscore_utf8 = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler_på y");
    nl_assert_eq_text(script_norsk_deler_paa_underscore_utf8, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_med = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler med y");
    nl_assert_eq_text(script_norsk_deler_med, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_pa_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler_pa y");
    nl_assert_eq_text(script_norsk_deler_pa_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delerpa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delerpa y");
    nl_assert_eq_text(script_norsk_delerpa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_delerpaa_kompakt = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x delerpaa y");
    nl_assert_eq_text(script_norsk_delerpaa_kompakt, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_med_underscore = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler_med y");
    nl_assert_eq_text(script_norsk_deler_med_underscore, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dele_kort = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dele y");
    nl_assert_eq_text(script_norsk_dele_kort, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_deler_kort = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x deler y");
    nl_assert_eq_text(script_norsk_deler_kort, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_divider_kort = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x divider y");
    nl_assert_eq_text(script_norsk_divider_kort, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_norsk_dividere_kort = selfhost__compiler__disasm_skript("la x=8;la y=2;returner x dividere y");
    nl_assert_eq_text(script_norsk_dividere_kort, "0: PUSH 8\n1: PUSH 2\n2: DIV\n3: PRINT\n4: HALT\n");
    char * script_c = selfhost__compiler__kompiler_skript_til_c("x=2;y=x+5;y*2");
    nl_assert_ne_text(script_c, "");
    char * script_err1 = selfhost__compiler__disasm_skript("x=2+3");
    nl_assert_eq_text(script_err1, "/* feil: mangler ';', ',' eller ':' etter assignment til x ved token 0 */");
    char * script_skip_empty = selfhost__compiler__disasm_skript("x=2;;x+1");
    nl_assert_eq_text(script_skip_empty, "0: PUSH 2\n1: PUSH 1\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_comma_separator = selfhost__compiler__disasm_skript("la x=2,la y=3,returner x+y");
    nl_assert_eq_text(script_comma_separator, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_colon_separator = selfhost__compiler__disasm_skript("la x=2:la y=3:returner x+y");
    nl_assert_eq_text(script_colon_separator, "0: PUSH 2\n1: PUSH 3\n2: ADD\n3: PRINT\n4: HALT\n");
    char * script_comma_trailing = selfhost__compiler__disasm_skript("la x=2,returner x,");
    nl_assert_eq_text(script_comma_trailing, "0: PUSH 2\n1: PRINT\n2: HALT\n");
    char * script_colon_trailing = selfhost__compiler__disasm_skript("la x=2:returner x:");
    nl_assert_eq_text(script_colon_trailing, "0: PUSH 2\n1: PRINT\n2: HALT\n");
    char * script_err3 = selfhost__compiler__disasm_skript("x=2+3;");
    nl_assert_eq_text(script_err3, "/* feil: skriptet mangler sluttuttrykk */");
    char * script_err4 = selfhost__compiler__disasm_skript("la ;x+1");
    nl_assert_eq_text(script_err4, "/* feil: 'la' må etterfølges av variabelnavn ved token 0 */");
    char * script_err5 = selfhost__compiler__disasm_skript("la x;x+1");
    nl_assert_eq_text(script_err5, "/* feil: ugyldig deklarasjon etter 'la' ved token 0 */");
    char * script_err6 = selfhost__compiler__disasm_skript("1+1; y=2; y");
    nl_assert_eq_text(script_err6, "0: PUSH 2\n1: PRINT\n2: HALT\n");
    char * script_err7 = selfhost__compiler__disasm_skript("la x=1;returner");
    nl_assert_eq_text(script_err7, "/* feil: 'returner' må etterfølges av uttrykk ved token 5 */");
    char * script_err8 = selfhost__compiler__disasm_skript("sett x=1;returner 0");
    nl_assert_eq_text(script_err8, "/* feil: variabel 'x' er ikke deklarert (bruk 'la') ved token 0 */");
    char * script_err9 = selfhost__compiler__disasm_skript("la x=1;la x=2;returner x");
    nl_assert_eq_text(script_err9, "/* feil: variabel 'x' er allerede deklarert ved token 5 */");
    char * script_err12 = selfhost__compiler__disasm_skript("la x+=1;returner x");
    nl_assert_eq_text(script_err12, "/* feil: 'la' støtter kun '=' ved token 0 */");
    char * script_err13 = selfhost__compiler__disasm_skript("x+=1;returner x");
    nl_assert_eq_text(script_err13, "/* feil: variabel 'x' er ikke deklarert for '+=' ved token 0 */");
    char * script_err10 = selfhost__compiler__disasm_skript("hvis 1==1 10 ellers 20");
    nl_assert_eq_text(script_err10, "/* feil: hvis-uttrykk mangler 'da' ved token 0 */");
    char * script_err11 = selfhost__compiler__disasm_skript("hvis 1==1 da 10");
    nl_assert_eq_text(script_err11, "/* feil: hvis-uttrykk mangler 'ellers' ved token 4 */");
    char * script_err14 = selfhost__compiler__disasm_skript("hvis (1==1 da 10 ellers 20");
    nl_assert_eq_text(script_err14, "/* feil: mangler ) i hvis-uttrykk ved token 1 */");
    nl_list_text* linjer = nl_list_text_new();
    nl_list_text_push(linjer, "");
    nl_list_text_push(linjer, "PUSH");
    nl_list_text_push(linjer, "11");
    nl_list_text_push(linjer, "PUSH");
    nl_list_text_push(linjer, "4");
    nl_list_text_push(linjer, "SUB");
    nl_list_text_push(linjer, "PRINT");
    nl_list_text_push(linjer, "HALT");
    char * c3 = selfhost__compiler__kompiler_fra_linjer(linjer);
    nl_assert_ne_text(c3, "");
    char * kilde = "# kommentar\nPUSH 10;\nPUSH 32\nADD\nPRINT\nHALT";
    char * c4 = selfhost__compiler__kompiler_fra_kilde(kilde);
    nl_assert_eq_text(c4, demo);
    return 0;
    return 0;
}

int main(void) {
    return start();
}