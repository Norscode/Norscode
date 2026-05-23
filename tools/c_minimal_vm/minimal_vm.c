/* Minimal VM — NCBB bytecode interpreter (26-opcode whitelist). */
#include "minimal_vm.h"
#include "mv_arena.h"
#include "mv_builtins.h"
#include "mv_internal.h"
#ifdef NORCODE_BOOTSTRAP_STUBS
#include "mv_bootstrap_stubs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if MV_USE_ARENA
#define MV_ALLOC(n) mv_arena_alloc(n)
#define MV_CALLOC(n, s) mv_arena_calloc(n, s)
#else
#define MV_ALLOC(n) malloc(n)
#define MV_CALLOC(n, s) calloc(n, s)
#endif

#define PUSH_TAG_INT 0
#define PUSH_TAG_BOOL 1
#define PUSH_TAG_STR 2

typedef struct mv_alloc_node {
    struct mv_alloc_node *next;
    void *ptr;
    mv_runtime_t *owner;
} mv_alloc_node_t;

typedef struct {
    uint32_t str_id;
    uint32_t off;
} mv_label_t;

struct mv_runtime_internal {
    mv_alloc_node_t *allocs;
};

static struct mv_runtime_internal g_rt_extra;

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

void *mv_track(mv_runtime_t *rt, void *ptr) {
#if !MV_USE_ARENA
    if (!ptr) return NULL;
    mv_alloc_node_t *node = (mv_alloc_node_t *)malloc(sizeof(mv_alloc_node_t));
    if (!node) return ptr;
    node->ptr = ptr;
    node->owner = rt;
    node->next = g_rt_extra.allocs;
    g_rt_extra.allocs = node;
#else
    (void)rt;
#endif
    return ptr;
}

int mv_opcode_from_name(const char *name) {
    if (streq(name, "LABEL")) return OP_LABEL;
    if (streq(name, "PUSH_CONST")) return OP_PUSH_CONST;
    if (streq(name, "LOAD_NAME")) return OP_LOAD_NAME;
    if (streq(name, "STORE_NAME")) return OP_STORE_NAME;
    if (streq(name, "POP")) return OP_POP;
    if (streq(name, "BUILD_LIST")) return OP_BUILD_LIST;
    if (streq(name, "BUILD_MAP")) return OP_BUILD_MAP;
    if (streq(name, "INDEX_GET")) return OP_INDEX_GET;
    if (streq(name, "INDEX_SET")) return OP_INDEX_SET;
    if (streq(name, "UNARY_NEG")) return OP_UNARY_NEG;
    if (streq(name, "BINARY_ADD")) return OP_BINARY_ADD;
    if (streq(name, "BINARY_SUB")) return OP_BINARY_SUB;
    if (streq(name, "BINARY_MUL")) return OP_BINARY_MUL;
    if (streq(name, "BINARY_DIV")) return OP_BINARY_DIV;
    if (streq(name, "BINARY_MOD")) return OP_BINARY_MOD;
    if (streq(name, "COMPARE_EQ")) return OP_COMPARE_EQ;
    if (streq(name, "COMPARE_NE")) return OP_COMPARE_NE;
    if (streq(name, "COMPARE_GT")) return OP_COMPARE_GT;
    if (streq(name, "COMPARE_LT")) return OP_COMPARE_LT;
    if (streq(name, "COMPARE_GE")) return OP_COMPARE_GE;
    if (streq(name, "COMPARE_LE")) return OP_COMPARE_LE;
    if (streq(name, "CALL")) return OP_CALL;
    if (streq(name, "JUMP")) return OP_JUMP;
    if (streq(name, "JUMP_IF_FALSE")) return OP_JUMP_IF_FALSE;
    if (streq(name, "RETURN")) return OP_RETURN;
    if (streq(name, "UNARY_NOT")) return OP_UNARY_NOT;
    return -1;
}

/* Arena-owned values: free is a no-op. Everything is reclaimed in mv_runtime_free. */
void mv_value_free(mv_runtime_t *rt, mv_value_t *v) {
    (void)rt;
    if (v) v->tag = MV_VAL_NIL;
}

/* Shallow copy. Lists/maps/strings are reference-shared (Python semantics). */
int mv_value_dup(mv_runtime_t *rt, const mv_value_t *src, mv_value_t *dst) {
    (void)rt;
    if (!src || !dst) return -1;
    *dst = *src;
    return 0;
}

int mv_value_truthy(const mv_value_t *v) {
    if (!v || v->tag == MV_VAL_NIL) return 0;
    if (v->tag == MV_VAL_BOOL) return v->u.b;
    if (v->tag == MV_VAL_INT) return v->u.i != 0;
    if (v->tag == MV_VAL_STR) {
        if (!v->u.s || !v->u.s[0]) return 0;
        if (strcmp(v->u.s, "usann") == 0 || strcmp(v->u.s, "false") == 0) return 0;
        if (strcmp(v->u.s, "sann") == 0 || strcmp(v->u.s, "true") == 0) return 1;
        return 1;
    }
    if (v->tag == MV_VAL_LIST) return v->u.list && v->u.list->len > 0;
    if (v->tag == MV_VAL_MAP) return v->u.map && v->u.map->len > 0;
    return 1;
}

mv_list_t *mv_list_new(mv_runtime_t *rt, size_t n) {
    mv_list_t *l = (mv_list_t *)mv_track(rt, MV_CALLOC(1, sizeof(mv_list_t)));
    if (!l) return NULL;
    l->cap = n ? n : 4;
    l->items = (mv_value_t *)mv_track(rt, MV_CALLOC(l->cap, sizeof(mv_value_t)));
    if (!l->items) return NULL;
    l->len = 0;
    return l;
}

int mv_list_append(mv_list_t *l, const mv_value_t *v) {
    if (!l || !v) return -1;
    if (l->len >= l->cap) {
        /* Grow without realloc: keep old buffer alive in arena (leaks until rt_free). */
        size_t nc = l->cap ? l->cap * 2 : 4;
        mv_value_t *ni = (mv_value_t *)MV_ALLOC(nc * sizeof(mv_value_t));
        if (!ni) return -1;
        if (l->items && l->len) memcpy(ni, l->items, l->len * sizeof(mv_value_t));
        l->items = ni;
        l->cap = nc;
        /* TODO: track ni in arena. mv_track requires rt; lists are owned via mv_list_new
         * which already tracked the initial items array. We leak old buffer + new buffer
         * until call frame ends. Acceptable for hello-compile sized workloads. */
    }
    l->items[l->len++] = *v;
    return 0;
}

char *mv_strdup(mv_runtime_t *rt, const char *s) {
    size_t n = strlen(s);
    char *p = (char *)mv_track(rt, MV_ALLOC(n + 1));
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static uint32_t mv_instr_size(const uint8_t *code, uint32_t len, uint32_t ip) {
    if (ip >= len) return 0;
    uint8_t op = code[ip];
    switch (op) {
    case OP_LABEL:
    case OP_JUMP:
    case OP_JUMP_IF_FALSE:
        return (ip + 5 <= len) ? 5 : 0;
    case OP_PUSH_CONST:
        if (ip + 1 >= len) return 0;
        if (code[ip + 1] == PUSH_TAG_INT) return (ip + 10 <= len) ? 10 : 0;
        if (code[ip + 1] == PUSH_TAG_BOOL) return (ip + 3 <= len) ? 3 : 0;
        if (code[ip + 1] == PUSH_TAG_STR) return (ip + 6 <= len) ? 6 : 0;
        return 0;
    case OP_LOAD_NAME:
    case OP_STORE_NAME:
        return (ip + 3 <= len) ? 3 : 0;
    case OP_BUILD_LIST:
    case OP_BUILD_MAP:
        return (ip + 2 <= len) ? 2 : 0;
    case OP_CALL:
        return (ip + 6 <= len) ? 6 : 0;
    default:
        return 1;
    }
}

static int mv_build_labels(
    const uint8_t *code,
    uint32_t len,
    mv_label_t **labels,
    uint32_t *n_labels) {
    uint32_t cap = 32;
    uint32_t n = 0;
    mv_label_t *lbl = (mv_label_t *)MV_ALLOC(cap * sizeof(mv_label_t));
    if (!lbl) return -1;
    uint32_t ip = 0;
    while (ip < len) {
        uint32_t sz = mv_instr_size(code, len, ip);
        if (sz == 0) {
#if !MV_USE_ARENA
            free(lbl);
#endif
            return -1;
        }
        if (code[ip] == OP_LABEL) {
            if (n >= cap) {
                cap *= 2;
                mv_label_t *nl = (mv_label_t *)MV_ALLOC(cap * sizeof(mv_label_t));
                if (!nl) return -1;
                memcpy(nl, lbl, n * sizeof(mv_label_t));
                lbl = nl;
            }
            lbl[n].str_id = rd32(code + ip + 1);
            lbl[n].off = ip;
            n++;
        }
        ip += sz;
    }
    *labels = lbl;
    *n_labels = n;
    return 0;
}

static uint32_t mv_find_label(const mv_label_t *labels, uint32_t n, uint32_t str_id) {
    for (uint32_t i = 0; i < n; i++) {
        if (labels[i].str_id == str_id) return labels[i].off;
    }
    return UINT32_MAX;
}

static int mv_compare_values(const mv_value_t *a, const mv_value_t *b, int *eq, int *ord) {
    if (a->tag != b->tag) return MV_ERR_TYPE;
    if (a->tag == MV_VAL_INT) {
        *eq = (a->u.i == b->u.i);
        *ord = (a->u.i < b->u.i) ? -1 : (a->u.i > b->u.i) ? 1 : 0;
        return 0;
    }
    if (a->tag == MV_VAL_BOOL) {
        *eq = (a->u.b == b->u.b);
        *ord = (a->u.b < b->u.b) ? -1 : (a->u.b > b->u.b) ? 1 : 0;
        return 0;
    }
    if (a->tag == MV_VAL_STR) {
        int c = strcmp(a->u.s ? a->u.s : "", b->u.s ? b->u.s : "");
        *eq = (c == 0);
        *ord = (c < 0) ? -1 : (c > 0) ? 1 : 0;
        return 0;
    }
    return MV_ERR_TYPE;
}

static int mv_push(mv_value_t *stack, size_t *sp, const mv_value_t *v) {
    if (*sp >= MV_STACK_MAX) return MV_ERR_STACK;
    stack[(*sp)++] = *v;
    return 0;
}

static int mv_pop(mv_value_t *stack, size_t *sp, mv_value_t *out) {
    if (*sp == 0) return MV_ERR_STACK;
    *out = stack[--(*sp)];
    return 0;
}

static int mv_call_target(
    mv_runtime_t *rt,
    uint32_t target_id,
    mv_value_t *args,
    uint8_t argc,
    mv_value_t *out) {
    if (target_id & 0x80000000) {
        uint32_t bid = target_id & 0x7FFFFFFF;
        if (!rt->bundle || bid >= rt->bundle->n_builtins) return MV_ERR_BUILTIN;
        const char *bname = rt->bundle->builtins[bid];
        int brc = mv_call_builtin(rt, bname, args, argc, out);
        if (brc == 0) return 0;
#ifdef NORCODE_BOOTSTRAP_STUBS
        {
            const char *shortn = strrchr(bname, '.');
            shortn = shortn ? shortn + 1 : bname;
            char full[280];
            snprintf(full, sizeof(full), "__main__.%s", shortn);
            int sr = mv_bootstrap_stub_call(rt, full, args, argc, out);
            if (sr == 0) return 0;
        }
#endif
        return brc;
    }
    if (!rt->bundle || target_id >= rt->bundle->n_functions) return MV_ERR_FN;
    return mv_call(rt, rt->bundle->functions[target_id].name, args, argc, out);
}

static int mv_run_function(
    mv_runtime_t *rt,
    const ncbb_function_t *fn,
    const mv_value_t *args,
    size_t argc,
    mv_value_t *out) {
    if (!fn || !fn->code) return MV_ERR_FN;
    if (rt->depth >= MV_CALL_DEPTH_MAX) return MV_ERR_DEPTH;

  rt->depth++;
    const uint8_t *code = fn->code;
    uint32_t len = fn->code_len;

    mv_label_t *labels = NULL;
    uint32_t n_labels = 0;
    if (mv_build_labels(code, len, &labels, &n_labels) != 0) {
        rt->depth--;
        return MV_ERR_DECODE;
    }
    /* labels used below; locals/stack allocated after n_slots check */

    if (fn->n_slots > MV_LOCALS_MAX) {
        rt->depth--;
        return MV_ERR_LOCALS;
    }
    mv_value_t *locals = (mv_value_t *)MV_CALLOC(fn->n_slots ? fn->n_slots : 1, sizeof(mv_value_t));
    if (!locals) {
        rt->depth--;
        return MV_ERR_LOCALS;
    }
    for (uint16_t i = 0; i < fn->n_params && (size_t)i < argc; i++) {
        if (mv_value_dup(rt, &args[i], &locals[i]) != 0) {
            rt->depth--;
            return MV_ERR_LOCALS;
        }
    }

    mv_value_t *stack = (mv_value_t *)MV_CALLOC(MV_STACK_MAX, sizeof(mv_value_t));
    if (!stack) {
        rt->depth--;
        return MV_ERR_LOCALS;
    }
    size_t sp = 0;
    uint32_t ip = 0;
    int rc = 0;

    while (ip < len) {
        if (++rt->steps > rt->max_steps) {
            rc = MV_ERR_STEPS;
            break;
        }
        uint32_t sz = mv_instr_size(code, len, ip);
        if (sz == 0) {
            rc = MV_ERR_DECODE;
            break;
        }
        uint8_t op = code[ip];

        if (op == OP_LABEL) {
            ip += sz;
            continue;
        }
        if (op == OP_PUSH_CONST) {
            uint8_t tag = code[ip + 1];
            mv_value_t v = MV_NIL();
            if (tag == PUSH_TAG_INT) {
                int64_t val;
                memcpy(&val, code + ip + 2, 8);
                v = MV_INT(val);
            } else if (tag == PUSH_TAG_BOOL) {
                v = MV_BOOL(code[ip + 2]);
            } else if (tag == PUSH_TAG_STR) {
                uint32_t sid = rd32(code + ip + 2);
                if (!rt->bundle || sid >= rt->bundle->n_strings) {
                    rc = MV_ERR_DECODE;
                    break;
                }
                v.tag = MV_VAL_STR;
                v.u.s = rt->bundle->strings[sid];
            } else {
                rc = MV_ERR_DECODE;
                break;
            }
            rc = mv_push(stack, &sp, &v);
        } else if (op == OP_LOAD_NAME) {
            uint16_t slot = rd16(code + ip + 1);
            if (slot >= fn->n_slots) {
                rc = MV_ERR_LOCALS;
                break;
            }
            rc = mv_push(stack, &sp, &locals[slot]);
        } else if (op == OP_STORE_NAME) {
            uint16_t slot = rd16(code + ip + 1);
            mv_value_t v;
            if (slot >= fn->n_slots || mv_pop(stack, &sp, &v) != 0) {
                rc = MV_ERR_LOCALS;
                break;
            }
            /* Overwrite slot; deep free deferred to frame exit (avoid shared-ref UAF). */
            locals[slot] = v;
        } else if (op == OP_POP) {
            mv_value_t tmp;
            if (sp > 0) mv_pop(stack, &sp, &tmp);
        } else if (op == OP_BUILD_LIST) {
            uint8_t n = code[ip + 1];
            mv_list_t *l = mv_list_new(rt, n);
            if (!l) {
                rc = MV_ERR_BUILTIN;
                break;
            }
            for (int i = (int)n - 1; i >= 0; i--) {
                (void)i;
                mv_value_t v;
                if (mv_pop(stack, &sp, &v) != 0) {
                    rc = MV_ERR_STACK;
                    break;
                }
                if (mv_list_append(l, &v) != 0) {
                    rc = MV_ERR_STACK;
                    break;
                }
            }
            if (rc != 0) break;
            /* reverse to preserve order */
            for (size_t a = 0, b = l->len; a < b / 2; a++) {
                mv_value_t t = l->items[a];
                l->items[a] = l->items[b - 1 - a];
                l->items[b - 1 - a] = t;
            }
            mv_value_t lv;
            lv.tag = MV_VAL_LIST;
            lv.u.list = l;
            rc = mv_push(stack, &sp, &lv);
        } else if (op == OP_BUILD_MAP) {
            uint8_t n = code[ip + 1];
            mv_map_t *m = (mv_map_t *)mv_track(rt, MV_CALLOC(1, sizeof(mv_map_t)));
            if (!m) {
                rc = MV_ERR_BUILTIN;
                break;
            }
            m->cap = n ? n : 4;
            m->entries = (mv_map_entry_t *)mv_track(rt, MV_CALLOC(m->cap, sizeof(mv_map_entry_t)));
            if (!m->entries) {
                rc = MV_ERR_BUILTIN;
                break;
            }
            for (int i = (int)n - 1; i >= 0; i--) {
                (void)i;
                mv_value_t val, key;
                if (mv_pop(stack, &sp, &val) != 0 || mv_pop(stack, &sp, &key) != 0) {
                    rc = MV_ERR_STACK;
                    break;
                }
                m->entries[m->len].key = key;
                m->entries[m->len].val = val;
                m->len++;
            }
            if (rc != 0) break;
            mv_value_t mv;
            mv.tag = MV_VAL_MAP;
            mv.u.map = m;
            rc = mv_push(stack, &sp, &mv);
        } else if (op == OP_INDEX_GET) {
            mv_value_t idx, obj;
            if (mv_pop(stack, &sp, &idx) != 0 || mv_pop(stack, &sp, &obj) != 0) {
                rc = MV_ERR_STACK;
                break;
            }
            mv_value_t res = MV_NIL();
            if (obj.tag == MV_VAL_MAP && obj.u.map) {
                int found = 0;
                for (size_t k = 0; k < obj.u.map->len; k++) {
                    int eq = 0, ord = 0;
                    if (mv_compare_values(&obj.u.map->entries[k].key, &idx, &eq, &ord) == 0 && eq) {
                        res = obj.u.map->entries[k].val;
                        found = 1;
                        break;
                    }
                }
                if (!found) res = MV_NIL();
            } else if (idx.tag == MV_VAL_INT && obj.tag == MV_VAL_LIST && obj.u.list) {
                int64_t i = idx.u.i;
                if (i < 0 || (size_t)i >= obj.u.list->len) {
                    rc = MV_ERR_TYPE;
                    break;
                }
                res = obj.u.list->items[i];
            } else if (idx.tag == MV_VAL_INT && obj.tag == MV_VAL_STR) {
                const char *s = obj.u.s ? obj.u.s : "";
                size_t sl = strlen(s);
                if (idx.u.i < 0 || (size_t)idx.u.i >= sl) {
                    rc = MV_ERR_TYPE;
                    break;
                }
                res = MV_INT((unsigned char)s[idx.u.i]);
            } else {
                rc = MV_ERR_TYPE;
                break;
            }
            rc = mv_push(stack, &sp, &res);
        } else if (op == OP_INDEX_SET) {
            mv_value_t val, idx, obj;
            if (mv_pop(stack, &sp, &val) != 0 || mv_pop(stack, &sp, &idx) != 0 ||
                mv_pop(stack, &sp, &obj) != 0) {
                rc = MV_ERR_STACK;
                break;
            }
            if (obj.tag == MV_VAL_MAP && obj.u.map) {
                mv_map_t *m = obj.u.map;
                int updated = 0;
                for (size_t k = 0; k < m->len; k++) {
                    int eq = 0, ord = 0;
                    if (mv_compare_values(&m->entries[k].key, &idx, &eq, &ord) == 0 && eq) {
                        m->entries[k].val = val;
                        updated = 1;
                        break;
                    }
                }
                if (!updated) {
                    if (m->len >= m->cap) {
                        size_t nc = m->cap ? m->cap * 2 : 4;
                        mv_map_entry_t *ne = (mv_map_entry_t *)MV_ALLOC(nc * sizeof(mv_map_entry_t));
                        if (!ne) {
                            rc = MV_ERR_BUILTIN;
                            break;
                        }
                        if (m->entries && m->len) {
                            memcpy(ne, m->entries, m->len * sizeof(mv_map_entry_t));
                        }
                        m->entries = ne;
                        m->cap = nc;
                    }
                    m->entries[m->len].key = idx;
                    m->entries[m->len].val = val;
                    m->len++;
                }
            } else if (idx.tag == MV_VAL_INT && obj.tag == MV_VAL_LIST && obj.u.list) {
                int64_t i = idx.u.i;
                if (i < 0 || (size_t)i >= obj.u.list->len) {
                    rc = MV_ERR_TYPE;
                    break;
                }
                obj.u.list->items[i] = val;
            } else {
                rc = MV_ERR_TYPE;
                break;
            }
        } else if (op == OP_UNARY_NEG) {
            mv_value_t a;
            if (mv_pop(stack, &sp, &a) != 0 || a.tag != MV_VAL_INT) {
                rc = MV_ERR_TYPE;
                break;
            }
            a.u.i = -a.u.i;
            rc = mv_push(stack, &sp, &a);
        } else if (op == OP_UNARY_NOT) {
            mv_value_t a;
            if (mv_pop(stack, &sp, &a) != 0) {
                rc = MV_ERR_STACK;
                break;
            }
            int truth = mv_value_truthy(&a);
            mv_value_t notv;
            notv.tag = MV_VAL_BOOL;
            notv.u.b = truth ? 0 : 1;
            rc = mv_push(stack, &sp, &notv);
        } else if (op >= OP_BINARY_ADD && op <= OP_BINARY_MOD) {
            mv_value_t b, a;
            if (mv_pop(stack, &sp, &b) != 0 || mv_pop(stack, &sp, &a) != 0) {
                rc = MV_ERR_STACK;
                break;
            }
            mv_value_t r = MV_NIL();
            if (op == OP_BINARY_ADD && a.tag == MV_VAL_STR && b.tag == MV_VAL_STR) {
                size_t la = strlen(a.u.s ? a.u.s : "");
                size_t lb = strlen(b.u.s ? b.u.s : "");
                char *buf = (char *)mv_track(rt, MV_ALLOC(la + lb + 1));
                if (!buf) {
                    rc = MV_ERR_BUILTIN;
                    break;
                }
                memcpy(buf, a.u.s ? a.u.s : "", la);
                memcpy(buf + la, b.u.s ? b.u.s : "", lb + 1);
                r.tag = MV_VAL_STR;
                r.u.s = buf;
            } else if (a.tag == MV_VAL_INT && b.tag == MV_VAL_INT) {
                switch (op) {
                case OP_BINARY_ADD:
                    r = MV_INT(a.u.i + b.u.i);
                    break;
                case OP_BINARY_SUB:
                    r = MV_INT(a.u.i - b.u.i);
                    break;
                case OP_BINARY_MUL:
                    r = MV_INT(a.u.i * b.u.i);
                    break;
                case OP_BINARY_DIV:
                    r = MV_INT(b.u.i != 0 ? a.u.i / b.u.i : 0);
                    break;
                case OP_BINARY_MOD:
                    r = MV_INT(b.u.i != 0 ? a.u.i % b.u.i : 0);
                    break;
                default:
                    break;
                }
            } else {
                rc = MV_ERR_TYPE;
                break;
            }
            rc = mv_push(stack, &sp, &r);
        } else if (op >= OP_COMPARE_EQ && op <= OP_COMPARE_LE) {
            mv_value_t b, a;
            if (mv_pop(stack, &sp, &b) != 0 || mv_pop(stack, &sp, &a) != 0) {
                rc = MV_ERR_STACK;
                break;
            }
            int eq = 0, ord = 0;
            if (mv_compare_values(&a, &b, &eq, &ord) != 0) {
                rc = MV_ERR_TYPE;
                break;
            }
            int truth = 0;
            switch (op) {
            case OP_COMPARE_EQ:
                truth = eq;
                break;
            case OP_COMPARE_NE:
                truth = !eq;
                break;
            case OP_COMPARE_GT:
                truth = ord > 0;
                break;
            case OP_COMPARE_LT:
                truth = ord < 0;
                break;
            case OP_COMPARE_GE:
                truth = ord >= 0;
                break;
            case OP_COMPARE_LE:
                truth = ord <= 0;
                break;
            default:
                break;
            }
            {
                mv_value_t tv = MV_BOOL(truth);
                rc = mv_push(stack, &sp, &tv);
            }
        } else if (op == OP_CALL) {
            uint32_t target_id = rd32(code + ip + 1);
            uint8_t argc = code[ip + 5];
            mv_value_t call_args[32];
            if (argc > 32) {
                rc = MV_ERR_STACK;
                break;
            }
            for (int i = (int)argc - 1; i >= 0; i--) {
                if (mv_pop(stack, &sp, &call_args[i]) != 0) {
                    rc = MV_ERR_STACK;
                    break;
                }
            }
            if (rc != 0) break;
            mv_value_t res;
            int cr = mv_call_target(rt, target_id, call_args, argc, &res);
            if (cr != 0) {
                rc = cr;
                break;
            }
            rc = mv_push(stack, &sp, &res);
        } else if (op == OP_JUMP) {
            uint32_t sid = rd32(code + ip + 1);
            uint32_t dest = mv_find_label(labels, n_labels, sid);
            if (dest == UINT32_MAX) {
                rc = MV_ERR_LABEL;
                break;
            }
            ip = dest;
            continue;
        } else if (op == OP_JUMP_IF_FALSE) {
            mv_value_t cond;
            if (mv_pop(stack, &sp, &cond) != 0) {
                rc = MV_ERR_STACK;
                break;
            }
            if (!mv_value_truthy(&cond)) {
                uint32_t sid = rd32(code + ip + 1);
                uint32_t dest = mv_find_label(labels, n_labels, sid);
                if (dest == UINT32_MAX) {
                    rc = MV_ERR_LABEL;
                    break;
                }
                ip = dest;
                continue;
            }
        } else if (op == OP_RETURN) {
            if (sp == 0) {
                *out = MV_NIL();
            } else {
                mv_pop(stack, &sp, out);
            }
            rc = 0;
            goto done;
        } else {
            rc = MV_ERR_DECODE;
            break;
        }
        if (rc != 0) break;
        ip += sz;
    }

done:
#if !MV_USE_ARENA
    for (uint16_t i = 0; i < fn->n_slots; i++) {
        mv_value_free(rt, &locals[i]);
    }
    free(locals);
    free(stack);
    free(labels);
#endif
    rt->depth--;
    if (rc != 0) {
        rt->last_error = rc;
        if (getenv("NORCODE_VM_TRACE")) {
            fprintf(stderr, "  in %s at ip=%u op=%u rc=%d\n",
                    fn->name ? fn->name : "?", ip, ip < len ? code[ip] : 255, rc);
        }
    }
    return rc;
}

int mv_runtime_init(mv_runtime_t *rt, const ncbb_bundle_t *bundle) {
    if (!rt) return -1;
    memset(rt, 0, sizeof(*rt));
    rt->bundle = bundle;
    rt->max_steps = MV_MAX_STEPS_DEFAULT;
    return 0;
}

void mv_runtime_free(mv_runtime_t *rt) {
#if MV_USE_ARENA
    (void)rt;
    mv_arena_reset();
#else
    mv_alloc_node_t **cur = &g_rt_extra.allocs;
    while (*cur) {
        mv_alloc_node_t *n = *cur;
        if (n->owner == rt) {
            *cur = n->next;
            free(n->ptr);
            free(n);
        } else {
            cur = &n->next;
        }
    }
#endif
}

int mv_call(mv_runtime_t *rt, const char *name, const mv_value_t *args, size_t argc, mv_value_t *out) {
    if (!rt || !name || !out) return -1;
    const ncbb_function_t *fn = ncbb_find_function(rt->bundle, name);
    if (!fn) {
#ifdef NORCODE_BOOTSTRAP_STUBS
        int sr = mv_bootstrap_stub_call(rt, name, args, (uint8_t)argc, out);
        if (sr == 0) return 0;
#endif
        return MV_ERR_FN;
    }
    return mv_run_function(rt, fn, args, argc, out);
}

int mv_run_int_program(const uint8_t *program, size_t len, int64_t *out_result) {
    /* Legacy probe format: op, int8_arg byte pairs (not NCBB). */
    int64_t stack[64];
    size_t sp = 0;
    size_t ip = 0;
    while (ip + 1 < len) {
        uint8_t op = program[ip++];
        int64_t arg = (int64_t)(int8_t)program[ip++];
        switch (op) {
        case OP_PUSH_CONST:
            if (sp >= 64) return -1;
            stack[sp++] = arg;
            break;
        case OP_BINARY_ADD:
            if (sp < 2) return -2;
            stack[sp - 2] += stack[sp - 1];
            sp--;
            break;
        case OP_RETURN:
            if (sp < 1) return -3;
            if (out_result) *out_result = stack[--sp];
            return 0;
        default:
            return -100 - (int)op;
        }
    }
    return -4;
}
