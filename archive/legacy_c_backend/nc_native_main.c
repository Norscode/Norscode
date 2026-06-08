/*
 * nc_native_main.c — native main for norscode_native
 *
 * Overstyr main() frå generert kode for å:
 * 1. Kompilere .no-filer direkte via kompiler_fil (C-funksjon)
 * 2. Køyre NCB via nc_vm.c sin executor (ikkje selfhost/vm.no)
 *
 * Build:
 *   clang -O2 -Wno-everything -DNORSCODE_NATIVE_MAIN \
 *         norscode_generated.c tools/nc_native_main.c -o dist/norscode_native
 */

/* Forhandsdeklarasjonar — brukar nc_dispatch_call for dynamisk oppslag */
struct NcVal; typedef struct NcVal NcVal;
NcVal *nc_builtin_ncb_route_handlers(NcVal **args, int na);
NcVal *nc_builtin_ncb_metadata(NcVal **args, int na);
NcVal *nc_builtin_ncb_next_request_id(NcVal **args, int na);
NcVal *nc_builtin_ncb_call_fn(NcVal **args, int na);

/* Stdlib dispatch handlers */
static NcVal *nc_std_path_basename(NcVal **args, int na);
static NcVal *nc_std_path_dirname(NcVal **args, int na);
static NcVal *nc_std_path_stem(NcVal **args, int na);
static NcVal *nc_std_path_join(NcVal **args, int na);
static NcVal *nc_std_path_exists(NcVal **args, int na);
static NcVal *nc_std_env_sett(NcVal **args, int na);
static NcVal *nc_std_env_hent(NcVal **args, int na);
static NcVal *nc_std_env_finnes(NcVal **args, int na);
static NcVal *g_std_env;

/* Kompilator-pipeline i bundle skal bruke bootstrap-host-dispatch, ikkje bytecode frå Gen1-NCB */
static int nc_exec_prefer_dispatch(const char *name) {
    if (!strncmp(name, "selfhost.kompiler.", 18)) return 1;
    if (!strncmp(name, "selfhost.lexer.", 15)) return 1;
    if (!strncmp(name, "selfhost.parser.", 16)) return 1;
    if (!strncmp(name, "selfhost.compiler.", 18)) return 1;
    if (!strncmp(name, "selfhost.json.", 14)) return 1;
    if (!strncmp(name, "std.path.", 9)) return 1;
    if (!strncmp(name, "std.env.", 8)) return 1;
    if (!strncmp(name, "std.web.", 8)) return 1;
    if (!strncmp(name, "std.csrf.", 9)) return 1;
    if (!strncmp(name, "std.security.", 13)) return 1;
    if (!strcmp(name, "kompiler_fil")) return 1;
    if (!strcmp(name, "json_skriv")) return 1;
    if (!strcmp(name, "json_parse_raw")) return 1;
    if (!strcmp(name, "json_parse")) return 1;
    return 0;
}

/* Bundler frå fersk .no skal bruke lokal bytecode, ikkje innebygd C __main__.bygg_bundle */
static int nc_exec_prefer_local(const char *name) {
    if (!strncmp(name, "selfhost.bundler.", 17)) return 1;
    if (!strncmp(name, "__main__.", 9)) return 1;
    const char *last = strrchr(name, '.');
    last = last ? last + 1 : name;
    if (!strcmp(last, "bygg_bundle")) return 1;
    return 0;
}

/* Finn funksjon med fuzzy matching */
static NcVal *nc_exec_find_fn(NcVal *functions, const char *name) {
    /* Direkte treff */
    NcVal *k = nc_str(name); NcVal *r = nc_index_get(functions, k);
    if (r && r->type != NC_NIL) return r;
    /* Strip "builtin." og prøv direkte */
    const char *qname = name;
    if (!strncmp(name, "builtin.", 8)) qname = name + 8;
    if (qname != name) {
        NcVal *k2 = nc_str(qname); NcVal *r2 = nc_index_get(functions, k2);
        if (r2 && r2->type != NC_NIL) return r2;
    }
    /* Fuzzy: søk etter siste segment, med prefix-prioritering */
    const char *last = strrchr(qname, '.'); if (last) last++; else last = qname;
    /* Berekn alias-prefix frå søkenamnet (det som er foran siste .):
     * "builtin.trace.start" → qname="trace.start" → prefix="trace"
     * "__main__.request_header" → qname=same → prefix="__main__"      */
    size_t pfx_len = last > qname ? (size_t)(last - qname - 1) : 0;
    NcVal *fuzzy_match = NULL;
    if (functions->type == NC_MAP) {
        for (int i = 0; i < functions->map->len; i++) {
            const char *fn = functions->map->keys[i];
            const char *fn_last = strrchr(fn, '.'); fn_last = fn_last ? fn_last+1 : fn;
            if (!strcmp(fn_last, last) || !strcmp(fn, last)) {
                /* Prefix-prioritering: føretrekk om prefikset matchar */
                if (pfx_len > 0) {
                    size_t fn_pfx = fn_last > fn ? (size_t)(fn_last - fn - 1) : 0;
                    if (fn_pfx == pfx_len && !strncmp(fn, qname, pfx_len)) return functions->map->vals[i];
                }
                if (!fuzzy_match) fuzzy_match = functions->map->vals[i];
            }
        }
    }
    return fuzzy_match;
}

/* Forward decl */
static NcVal *g_nc_closure_fwd;
#define g_nc_closure g_nc_closure_fwd
static NcVal *nc_exec_call(NcVal *functions, const char *fn_name, NcVal **args, int nargs, int depth);
static NcVal *nc_exec_call_closure(NcVal *functions, const char *fn_name, NcVal **args, int nargs, NcVal *closure, int depth);
static int nc_val_til_exit(NcVal *v);
static NcVal *nc_native_kompiler(const char *src_path, const char *modul);

/* ── SQLite3 forward declarations (no header required — links against -lsqlite3) ── */
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef long long sqlite3_int64;
#define SQLITE_OK   0
#define SQLITE_ROW  100
#define SQLITE_DONE 101
extern int sqlite3_open(const char *filename, sqlite3 **ppDb);
extern int sqlite3_close(sqlite3 *db);
extern int sqlite3_exec(sqlite3 *db, const char *sql, int (*cb)(void*,int,char**,char**), void *arg, char **errmsg);
extern void sqlite3_free(void *p);
extern const char *sqlite3_errmsg(sqlite3 *db);
extern int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
extern int sqlite3_bind_text(sqlite3_stmt *pStmt, int n, const char *zData, int nData, void (*xDel)(void*));
extern int sqlite3_step(sqlite3_stmt *pStmt);
extern int sqlite3_finalize(sqlite3_stmt *pStmt);
extern const unsigned char *sqlite3_column_text(sqlite3_stmt *pStmt, int iCol);
extern sqlite3_int64 sqlite3_column_int64(sqlite3_stmt *pStmt, int iCol);
extern int sqlite3_changes(sqlite3 *db);
#define SQLITE_TRANSIENT ((void*)(-1))

/* ── DB pool state ── */
#define NC_DB_MAX_POOLS 8
#define NC_DB_MAX_POOL_CONNS 16
typedef struct { sqlite3 *conns[NC_DB_MAX_POOL_CONNS]; int avail; char path[512]; } NcDbPool;
static NcDbPool g_nc_db_pools[NC_DB_MAX_POOLS];
static int g_nc_db_pool_count = 0;

static sqlite3 *nc_db_handle_ptr(NcVal *h) {
    if (!h || h->type != NC_MAP) return NULL;
    NcVal *p = nc_index_get(h, nc_str("__db"));
    return (p && p->type == NC_INT) ? (sqlite3*)(intptr_t)(unsigned long long)p->i : NULL;
}
static NcVal *nc_db_make_handle(sqlite3 *db, int pool_idx) {
    NcVal *m = nc_map_new();
    nc_index_set(m, nc_str("__db"), nc_int((long long)(intptr_t)(unsigned long long)db));
    if (pool_idx >= 0) nc_index_set(m, nc_str("__pool"), nc_int(pool_idx));
    return m;
}
static unsigned long long nc_db_sql_hash(const char *s) {
    unsigned long long h = 5381;
    for (int c; (c = (unsigned char)*s++);) h = ((h << 5) + h) ^ c;
    return h;
}

/* Lazy-load selfhost/common.no for sh.* / selfhost.common.* / selfhost.compiler.* */
static NcVal *g_sh_common_fns = NULL;

static void nc_ensure_sh_common(void) {
    if (g_sh_common_fns) return;
    /* Try precompiled NCB first (fast, no compilation needed) */
    NcVal *ncb = NULL;
    NcVal *ncb_file = nc_builtin_fil_les(nc_str("bootstrap/precompiled/common.ncb.json"));
    if (ncb_file && ncb_file->type == NC_STR) {
        ncb = nc_builtin_json_parse_str(ncb_file);
    }
    /* Fall back to compiling from source */
    if (!ncb || ncb->type != NC_MAP) {
        NcVal *ncb_json = nc_native_kompiler("selfhost/common.no", "selfhost.common");
        if (!ncb_json || ncb_json->type != NC_STR) {
            nc_throw("Kunne ikkje laste selfhost/common.no");
            return;
        }
        ncb = nc_builtin_json_parse_str(ncb_json);
    }
    NcVal *fns = nc_index_get(ncb, nc_str("functions"));
    if (!fns || fns->type != NC_MAP) {
        nc_throw("selfhost/common.no manglar functions");
        return;
    }
    g_sh_common_fns = fns;
}

static int nc_is_sh_api(const char *cn) {
    if (!strncmp(cn, "sh.", 3)) return 1;
    if (!strncmp(cn, "selfhost.common.", 16)) return 1;
    if (!strncmp(cn, "selfhost.compiler.", 18)) return 1;
    return 0;
}

static NcVal *nc_call_sh_api(const char *cn, NcVal **args, int nargs) {
    nc_ensure_sh_common();
    const char *short_fn = strrchr(cn, '.');
    short_fn = short_fn ? short_fn + 1 : cn;
    char full[160];
    snprintf(full, sizeof(full), "selfhost.common.%s", short_fn);
    if (!g_sh_common_fns ||
        (!nc_exec_find_fn(g_sh_common_fns, full) && !nc_exec_find_fn(g_sh_common_fns, short_fn))) {
        nc_panic("Ukjent selfhost API: %s", cn);
        return nc_nil();
    }
    return nc_exec_call(g_sh_common_fns, full, args, nargs, 0);
}

/* Lazy-load selfhost/ir_contract.no for ir.* / selfhost.ir_contract.* */
static NcVal *g_sh_ir_contract_fns = NULL;

static void nc_ensure_sh_ir_contract(void) {
    if (g_sh_ir_contract_fns) return;
    NcVal *ncb = NULL;
    NcVal *ncb_file = nc_builtin_fil_les(nc_str("bootstrap/precompiled/ir_contract.ncb.json"));
    if (ncb_file && ncb_file->type == NC_STR) {
        ncb = nc_builtin_json_parse_str(ncb_file);
    }
    if (!ncb || ncb->type != NC_MAP) {
        NcVal *ncb_json = nc_native_kompiler("selfhost/ir_contract.no", "selfhost.ir_contract");
        if (!ncb_json || ncb_json->type != NC_STR) {
            nc_throw("Kunne ikkje laste selfhost/ir_contract.no");
            return;
        }
        ncb = nc_builtin_json_parse_str(ncb_json);
    }
    NcVal *fns = nc_index_get(ncb, nc_str("functions"));
    if (!fns || fns->type != NC_MAP) {
        nc_throw("selfhost/ir_contract.no manglar functions");
        return;
    }
    g_sh_ir_contract_fns = fns;
}

static int nc_is_ir_contract_api(const char *cn) {
    if (!strncmp(cn, "ir.", 3)) return 1;
    if (!strncmp(cn, "selfhost.ir_contract.", 21)) return 1;
    return 0;
}

static NcVal *nc_call_ir_contract_api(const char *cn, NcVal **args, int nargs) {
    nc_ensure_sh_ir_contract();
    const char *short_fn = strrchr(cn, '.');
    short_fn = short_fn ? short_fn + 1 : cn;
    char full[160];
    snprintf(full, sizeof(full), "selfhost.ir_contract.%s", short_fn);
    if (!g_sh_ir_contract_fns ||
        (!nc_exec_find_fn(g_sh_ir_contract_fns, full) && !nc_exec_find_fn(g_sh_ir_contract_fns, short_fn))) {
        nc_panic("Ukjent ir_contract API: %s", cn);
        return nc_nil();
    }
    return nc_exec_call(g_sh_ir_contract_fns, full, args, nargs, 0);
}
static NcVal *nc_stub_t_hilsen(NcVal *navn) { char *n=nc_to_str_raw(navn); char r[256]; snprintf(r,sizeof(r),"Hei %s",n); free(n); return nc_str(r); }
static NcVal *nc_stub_t_starter_med(NcVal *s, NcVal *p) { return nc_builtin_starts_with(s, p); }
static NcVal *nc_stub_assert_slutter_med(NcVal *s, NcVal *p) {
    if (!nc_truthy(nc_builtin_ends_with(s, p))) {
        char *sv=nc_to_str_raw(s), *pv=nc_to_str_raw(p);
        char msg[512]; snprintf(msg, sizeof(msg), "assert_slutter_med: '%s' sluttar ikkje med '%s'", sv, pv);
        free(sv); free(pv); nc_throw(msg);
    }
    return nc_nil();
}
static NcVal *nc_stub_assert_starter_med(NcVal *s, NcVal *p) {
    if (!nc_truthy(nc_builtin_starts_with(s, p))) {
        char *sv=nc_to_str_raw(s), *pv=nc_to_str_raw(p);
        char msg[512]; snprintf(msg, sizeof(msg), "assert_starter_med feilet: '%s' startar ikkje med '%s'", sv, pv);
        free(sv); free(pv); nc_throw(msg);
    }
    return nc_nil();
}
static NcVal *nc_stub_path_join(NcVal *a, NcVal *b) {
    char *sa=nc_to_str_raw(a), *sb=nc_to_str_raw(b);
    size_t la=strlen(sa), lb=strlen(sb);
    char *r = malloc(la+lb+2);
    strcpy(r, sa);
    if (la>0 && sa[la-1]!='/' && lb>0 && sb[0]!='/') strcat(r, "/");
    strcat(r, sb);
    free(sa); free(sb);
    return nc_str_own(r);
}
static NcVal *nc_stub_web_escape_html(NcVal *v) {
    char *s = nc_to_str_raw(v);
    /* Minimal HTML escape */
    char *r = malloc(strlen(s)*6+1); char *wp = r;
    for (char *p=s; *p; p++) {
        if (*p=='<') { strcpy(wp,"&lt;"); wp+=4; }
        else if (*p=='>') { strcpy(wp,"&gt;"); wp+=4; }
        else if (*p=='&') { strcpy(wp,"&amp;"); wp+=5; }
        else if (*p=='"') { strcpy(wp,"&quot;"); wp+=6; }
        else if (*p=='\'') { strcpy(wp,"&#x27;"); wp+=6; }
        else *wp++ = *p;
    }
    *wp=0; free(s);
    return nc_str_own(r);
}


/* Kjøyr ein funksjon */
static NcVal *nc_exec_call(NcVal *functions, const char *fn_name, NcVal **args, int nargs, int depth) {
    if (depth > 800) { nc_throw("For djup rekursjon"); return nc_nil(); }

    NcVal *fn_def = nc_exec_find_fn(functions, fn_name);
    if (!fn_def) {
        /* Prøv innebygd */
        nc_panic("Ukjent funksjon: %s", fn_name);
        return nc_nil();
    }

    NcVal *params_v = nc_index_get(fn_def, nc_str("params"));
    NcVal *code_v   = nc_index_get(fn_def, nc_str("code"));

    NcVal **stack_arr = calloc(512, sizeof(NcVal*)); int sp = 0;
    NcVal **vars_arr  = calloc(2048, sizeof(NcVal*));
    char **varnames   = calloc(2048, sizeof(char*)); int nvars = 0;
    /* TRY/CATCH stack */
    struct { const char *catch_lbl; int sp_depth; jmp_buf jmp; } try_stack[32];
    int try_depth = 0;
    static char last_exception[4096];

    /* Last inn parametrar */
    if (params_v && params_v->type == NC_LIST) {
        for (int i = 0; i < params_v->list->len && i < nargs; i++) {
            NcVal *pname = params_v->list->items[i];
            char *n = nc_to_str_raw(pname);
            nc_store(vars_arr, varnames, &nvars, n, args[i]); free(n);
        }
    }

    /* Bygg label-kart */
    NcVal *label_map = nc_map_new();
    if (code_v && code_v->type == NC_LIST) {
        for (int ip = 0; ip < code_v->list->len; ip++) {
            NcVal *instr = code_v->list->items[ip];
            if (!instr || instr->type != NC_LIST || instr->list->len < 1) continue;
            NcVal *op_v = instr->list->items[0];
            if (op_v && op_v->type == NC_STR && !strcmp(op_v->s, "LABEL")) {
                if (instr->list->len >= 2) {
                    char ipbuf[32]; snprintf(ipbuf, sizeof(ipbuf), "%d", ip);
                    nc_index_set(label_map, instr->list->items[1], nc_int(ip));
                }
            }
        }
    }

    NcVal *retval = nc_nil();
    if (!code_v || code_v->type != NC_LIST) { free(stack_arr); free(vars_arr); free(varnames); return retval; }

    int ip = 0;
    while (ip < code_v->list->len) {
        NcVal *instr = code_v->list->items[ip];
        if (!instr || instr->type != NC_LIST || instr->list->len < 1) { ip++; continue; }
        NcVal *op_v = instr->list->items[0];
        if (!op_v || op_v->type != NC_STR) { ip++; continue; }
        const char *op = op_v->s;

        if (!strcmp(op, "PUSH_CONST")) {
            NcVal *v = instr->list->len >= 2 ? instr->list->items[1] : nc_nil();
            nc_push(&sp, stack_arr, v); ip++;
        } else if (!strcmp(op, "STORE_NAME")) {
            if (instr->list->len >= 2) {
                char *n = nc_to_str_raw(instr->list->items[1]);
                nc_store(vars_arr, varnames, &nvars, n, nc_pop(&sp, stack_arr)); free(n);
            }
            ip++;
        } else if (!strcmp(op, "LOAD_NAME")) {
            if (instr->list->len >= 2) {
                char *n = nc_to_str_raw(instr->list->items[1]);
                /* Sjekk lokale vars fyrst */
                NcVal *lv = nc_nil();
                for (int _li=0; _li<nvars; _li++) {
                    if (!strcmp(varnames[_li], n)) { lv = vars_arr[_li]; goto _load_done; }
                }
                /* Sjekk global closure-captures */
                if (g_nc_closure && g_nc_closure->type == NC_MAP) {
                    NcVal *cv = nc_index_get(g_nc_closure, nc_str(n));
                    if (cv && cv->type != NC_NIL) { lv = cv; goto _load_done; }
                }
                lv = nc_load(vars_arr, varnames, nvars, n); /* kastar for ukjent */
                _load_done: free(n);
                nc_push(&sp, stack_arr, lv);
            }
            ip++;
        } else if (!strcmp(op, "POP")) {
            nc_pop(&sp, stack_arr); ip++;
        } else if (!strcmp(op, "RETURN")) {
            retval = nc_pop(&sp, stack_arr); goto done;
        } else if (!strcmp(op, "LABEL")) {
            ip++;
        } else if (!strcmp(op, "JUMP")) {
            if (instr->list->len >= 2) {
                NcVal *tgt = nc_index_get(label_map, instr->list->items[1]);
                if (tgt && tgt->type == NC_INT) { ip = (int)tgt->i; continue; }
            }
            ip++;
        } else if (!strcmp(op, "JUMP_IF_FALSE")) {
            NcVal *cond = nc_pop(&sp, stack_arr);
            if (!nc_truthy(cond)) {
                if (instr->list->len >= 2) {
                    NcVal *tgt = nc_index_get(label_map, instr->list->items[1]);
                    if (tgt && tgt->type == NC_INT) { ip = (int)tgt->i; continue; }
                }
            }
            ip++;
        } else if (!strcmp(op, "BINARY_ADD")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_add(a,b)); ip++;
        } else if (!strcmp(op, "BINARY_SUB")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_sub(a,b)); ip++;
        } else if (!strcmp(op, "BINARY_MUL")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_mul(a,b)); ip++;
        } else if (!strcmp(op, "BINARY_DIV")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_div(a,b)); ip++;
        } else if (!strcmp(op, "BINARY_MOD")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_mod(a,b)); ip++;
        } else if (!strcmp(op, "BINARY_LSHIFT")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
            nc_push(&sp,stack_arr,nc_int(av<<bv)); ip++;
        } else if (!strcmp(op, "BINARY_RSHIFT")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
            nc_push(&sp,stack_arr,nc_int(av>>bv)); ip++;
        } else if (!strcmp(op, "BINARY_AND")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
            nc_push(&sp,stack_arr,nc_int(av&bv)); ip++;
        } else if (!strcmp(op, "BINARY_OR")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
            nc_push(&sp,stack_arr,nc_int(av|bv)); ip++;
        } else if (!strcmp(op, "BINARY_XOR")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
            nc_push(&sp,stack_arr,nc_int(av^bv)); ip++;
        } else if (!strcmp(op, "UNARY_NEG")) {
            nc_push(&sp,stack_arr,nc_neg(nc_pop(&sp,stack_arr))); ip++;
        } else if (!strcmp(op, "UNARY_NOT")) {
            nc_push(&sp,stack_arr,nc_bool(!nc_truthy(nc_pop(&sp,stack_arr)))); ip++;
        } else if (!strcmp(op, "COMPARE_EQ")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_bool(nc_eq(a,b))); ip++;
        } else if (!strcmp(op, "COMPARE_NE")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_bool(!nc_eq(a,b))); ip++;
        } else if (!strcmp(op, "COMPARE_LT")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_cmp(a,b,-1)); ip++;
        } else if (!strcmp(op, "COMPARE_GT")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_cmp(a,b,1)); ip++;
        } else if (!strcmp(op, "COMPARE_LE")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_cmp(a,b,-2)); ip++;
        } else if (!strcmp(op, "COMPARE_GE")) {
            NcVal *b=nc_pop(&sp,stack_arr),*a=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_cmp(a,b,2)); ip++;
        } else if (!strcmp(op, "BUILD_LAMBDA")) {
            /* Lag closure-map med fn-namn + alle noverande variablar */
            NcVal *closure = nc_map_new();
            if (instr->list->len >= 2) {
                nc_index_set(closure, nc_str("__closure__"), instr->list->items[1]);
            }
            /* Fang alle noverande variablar */
            for (int _ci=0; _ci<nvars; _ci++) {
                nc_index_set(closure, nc_str(varnames[_ci]), vars_arr[_ci]);
            }
            nc_push(&sp, stack_arr, closure);
            ip++;
        } else if (!strcmp(op, "BUILD_LIST")) {
            int n = instr->list->len>=2 && instr->list->items[1]->type==NC_INT ? (int)instr->list->items[1]->i : 0;
            nc_push(&sp,stack_arr,nc_build_list(&sp,stack_arr,n)); ip++;
        } else if (!strcmp(op, "BUILD_MAP")) {
            int n = instr->list->len>=2 && instr->list->items[1]->type==NC_INT ? (int)instr->list->items[1]->i : 0;
            nc_push(&sp,stack_arr,nc_build_map(&sp,stack_arr,n)); ip++;
        } else if (!strcmp(op, "INDEX_GET")) {
            NcVal *k=nc_pop(&sp,stack_arr),*o=nc_pop(&sp,stack_arr);
            nc_push(&sp,stack_arr,nc_index_get(o,k)); ip++;
        } else if (!strcmp(op, "INDEX_SET")) {
            NcVal *v=nc_pop(&sp,stack_arr),*k=nc_pop(&sp,stack_arr),*o=nc_pop(&sp,stack_arr);
            nc_index_set(o,k,v); nc_push(&sp,stack_arr,o); ip++;
        } else if (!strcmp(op, "CALL")) {
            if (instr->list->len < 2) { ip++; continue; }
            char *callee = nc_to_str_raw(instr->list->items[1]);
            int narg = instr->list->len>=3 && instr->list->items[2]->type==NC_INT ? (int)instr->list->items[2]->i : 0;
            NcVal **cargs = calloc(narg, sizeof(NcVal*));
            for (int ci=narg-1;ci>=0;ci--) cargs[ci]=nc_pop(&sp,stack_arr);

            /* Handter builtins direkte */
            const char *cn = callee;
            if (strncmp(cn,"builtin.",8)==0) cn+=8;
            if (strncmp(cn,"builtin.",8)==0) cn+=8; /* strip dobbelt prefiks */
            /* Variabel-kall: sjekk om cn er ein variabel som held eit fn-namn */
            NcVal *var_fn = nc_nil();
            for (int _vi=0; _vi<nvars; _vi++) {
                if (!strcmp(varnames[_vi], cn)) { var_fn = vars_arr[_vi]; break; }
            }
            /* Closure-kall: var er ein MAP med __closure__ */
            if (var_fn && var_fn->type == NC_MAP) {
                NcVal *cl_fn = nc_index_get(var_fn, nc_str("__closure__"));
                if (cl_fn && cl_fn->type == NC_STR && nc_exec_find_fn(functions, cl_fn->s)) {
                    /* Legg closure-vars til args-kontekst via ein wrapper */
                    NcVal **cl_cargs = calloc(narg + var_fn->map->len, sizeof(NcVal*));
                    memcpy(cl_cargs, cargs, narg * sizeof(NcVal*));
                    /* Kjøyr med closure som ekstra kontekst */
                    NcVal *lambda_r = nc_exec_call_closure(functions, cl_fn->s, cargs, narg, var_fn, depth+1);
                    free(cl_cargs); free(cargs); free(callee);
                    nc_push(&sp,stack_arr,lambda_r); ip++;
                    continue;
                }
            }
            /* Direkte funksjonsnamnkall */
            if (var_fn && var_fn->type == NC_STR && nc_exec_find_fn(functions, var_fn->s)) {
                NcVal *lambda_r = nc_exec_call(functions, var_fn->s, cargs, narg, depth+1);
                free(cargs); free(callee);
                nc_push(&sp,stack_arr,lambda_r); ip++;
                continue;
            }
            NcVal *fn_r = nc_nil();
            if (!strcmp(cn,"skriv"))                fn_r = nc_builtin_skriv(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"lengde"))           fn_r = nc_builtin_lengde(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"legg_til"))         { if(narg>=2) nc_builtin_legg_til(cargs[0],cargs[1]); }
            else if (!strcmp(cn,"fjern_siste")||!strcmp(cn,"pop_siste")||!strcmp(cn,"builtin.pop_siste")||!strcmp(cn,"builtin.fjern_siste"))
                fn_r = nc_builtin_fjern_siste(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"fjern")||!strcmp(cn,"fjern_indeks")) fn_r = nc_builtin_fjern(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"slice"))            fn_r = nc_builtin_slice(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil(),narg>2?cargs[2]:nc_nil());
            else if (!strcmp(cn,"starts_with")||!strcmp(cn,"tekst_starter_med")) fn_r=nc_builtin_starts_with(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"ends_with")||!strcmp(cn,"tekst_slutter_med"))   fn_r=nc_builtin_ends_with(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"contains")||!strcmp(cn,"tekst_inneholder"))     fn_r=nc_builtin_contains(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"split")||!strcmp(cn,"tekst_splitt"))  fn_r=nc_builtin_split(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"join")||!strcmp(cn,"tekst_join"))     fn_r=nc_builtin_join(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"replace")||!strcmp(cn,"tekst_erstatt")) fn_r=nc_builtin_replace(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil(),narg>2?cargs[2]:nc_nil());
            else if (!strcmp(cn,"trim")||!strcmp(cn,"tekst_trim"))     fn_r=nc_builtin_trim(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"heltall")||!strcmp(cn,"heltall_fra_tekst")) fn_r=nc_builtin_heltall(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"tekst_fra_heltall"))fn_r=nc_builtin_tekst_fra_heltall(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"tekst")||!strcmp(cn,"til_tekst"))     fn_r=nc_to_str(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"t.hilsen")) {
                char *s = nc_to_str_raw(narg>0 ? cargs[0] : nc_nil());
                size_t need = strlen("Hei ") + strlen(s) + 1;
                char *msg = malloc(need);
                snprintf(msg, need, "Hei %s", s);
                fn_r = nc_str(msg);
                free(msg);
                free(s);
            }
            else if (!strcmp(cn,"t.rop")) {
                char *s = nc_to_str_raw(narg>0 ? cargs[0] : nc_nil());
                size_t need = strlen(s) + 2;
                char *msg = malloc(need);
                snprintf(msg, need, "%s!", s);
                fn_r = nc_str(msg);
                free(msg);
                free(s);
            }
            else if (!strcmp(cn,"finnes_nøkkel")||!strcmp(cn,"har_nokkel")) fn_r=nc_builtin_finnes_nokkel(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"nøkler"))           fn_r=nc_builtin_nokler(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"verdier"))          fn_r=nc_builtin_verdier(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"fjern_nokkel"))     { if(narg>=2) nc_builtin_fjern_nokkel(cargs[0],cargs[1]); }
            else if (!strcmp(cn,"json_stringify"))   fn_r=nc_builtin_json_stringify_smart(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json_parse"))       fn_r=nc_builtin_json_parse_norscode(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"kompiler_fil"))     fn_r=nc_dispatch_call("selfhost.kompiler.kompiler_fil", cargs, narg);
            else if (!strcmp(cn,"fil_les")||!strcmp(cn,"fil_skriv")) {
                /* fil_les og fil_skriv kan kaste IOFeil — bruk setjmp */
                int _fil_caught = 0;
                if (try_depth > 0) {
                    jmp_buf _fil_saved; memcpy(&_fil_saved,&g_err_jmp,sizeof(jmp_buf));
                    if (setjmp(g_err_jmp)) {
                        memcpy(&g_err_jmp,&_fil_saved,sizeof(jmp_buf));
                        if (try_depth > 0) {
                            { const char *_em=g_err_msg;
                              if (strncmp(_em,"Norscode unntak: ",17)==0) _em+=17;
                              else if (strncmp(_em,"nc-vm feil: Norscode unntak: ",28)==0) _em+=28;
                              strncpy(last_exception,_em,sizeof(last_exception)-1); }
                            g_err_msg[0]=0;
                            const char *_cl=try_stack[try_depth-1].catch_lbl;
                            sp=try_stack[try_depth-1].sp_depth;
                            NcVal *_tgt=nc_index_get(label_map,nc_str(_cl));
                            if (_tgt&&_tgt->type==NC_INT){ip=(int)_tgt->i+1;_fil_caught=1;}
                        }
                        fn_r=nc_nil();
                    } else {
                        if (!strcmp(cn,"fil_les")) fn_r=nc_builtin_fil_les(narg>0?cargs[0]:nc_nil());
                        else if (narg>=2) nc_builtin_fil_skriv(cargs[0],cargs[1]);
                        memcpy(&g_err_jmp,&_fil_saved,sizeof(jmp_buf));
                    }
                } else {
                    if (!strcmp(cn,"fil_les")) fn_r=nc_builtin_fil_les(narg>0?cargs[0]:nc_nil());
                    else if (narg>=2) nc_builtin_fil_skriv(cargs[0],cargs[1]);
                }
                if (_fil_caught) { free(cargs); free(callee); continue; }
            }
            else if (!strcmp(cn,"fil_skriv_bin\xc3\xa6r")||!strcmp(cn,"fil_skriv_binary")) { if(narg>=2) nc_builtin_fil_skriv_binary(cargs[0],cargs[1]); }
            else if (!strcmp(cn,"char_code")) fn_r=nc_builtin_char_code(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"chr")) fn_r=nc_builtin_chr(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json_parse_raw")) fn_r=nc_builtin_json_parse_raw(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"fil_finnes"))       fn_r=nc_builtin_fil_finnes(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"miljo_hent"))       fn_r=nc_builtin_miljo_hent(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"miljo_finnes"))     fn_r=nc_builtin_miljo_finnes(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"bool"))             fn_r=nc_builtin_bool(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"feil"))             fn_r=nc_builtin_feil(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"type")||!strcmp(cn,"type_av")) fn_r=nc_builtin_type(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"index_of"))         fn_r=nc_builtin_index_of(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"upper")||!strcmp(cn,"tekst_til_store")||!strcmp(cn,"builtin.upper")||!strcmp(cn,"builtin.tekst_til_store")||!strcmp(cn,"builtin.tekst_store"))
                fn_r=nc_builtin_upper(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"lower")||!strcmp(cn,"tekst_til_liten")) fn_r=nc_builtin_lower(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"exit")||!strcmp(cn,"stopp")) nc_builtin_exit(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"math.pluss")||!strcmp(cn,"std.math.pluss")||!strcmp(cn,"builtin.math.pluss"))   fn_r=nc_add(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.minus")||!strcmp(cn,"std.math.minus")||!strcmp(cn,"builtin.math.minus"))   fn_r=nc_sub(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.gange")||!strcmp(cn,"std.math.gange")||!strcmp(cn,"builtin.math.gange"))   fn_r=nc_mul(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.dele")||!strcmp(cn,"std.math.dele")||!strcmp(cn,"builtin.math.dele"))     fn_r=nc_div(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.rest")||!strcmp(cn,"std.math.rest")||!strcmp(cn,"builtin.math.rest"))     fn_r=nc_mod(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.abs")||!strcmp(cn,"std.math.abs")||!strcmp(cn,"builtin.math.abs")) {
                NcVal *v=narg>0?cargs[0]:nc_nil();
                fn_r = (v->type==NC_INT && v->i<0) ? nc_int(-v->i) : v;
            }
            else if (!strcmp(cn,"math.min")||!strcmp(cn,"std.math.min")||!strcmp(cn,"builtin.math.min"))   fn_r=nc_truthy(nc_cmp(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil(),-1))?cargs[0]:cargs[1];
            else if (!strcmp(cn,"math.maks")||!strcmp(cn,"std.math.maks")||!strcmp(cn,"builtin.math.maks")) fn_r=nc_truthy(nc_cmp(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil(),1))?cargs[0]:cargs[1];
            else if (!strcmp(cn,"assert") || !strcmp(cn,"builtin.assert")) {
                if (narg>0 && !nc_truthy(cargs[0])) {
                    char *msg = narg>1 ? nc_to_str_raw(cargs[1]) : strdup("assert feilet");
                    nc_throw(msg); free(msg);
                }
            }
            else if (!strcmp(cn,"assert_eq") || !strcmp(cn,"builtin.assert_eq")) {
                if (narg>=2 && !nc_eq(cargs[0],cargs[1])) {
                    char *a=nc_to_str_raw(cargs[0]),*b=nc_to_str_raw(cargs[1]);
                    char msg[512]; snprintf(msg,sizeof(msg),"assert_eq feilet: %s != %s",a,b);
                    free(a); free(b); nc_throw(msg);
                }
            }
            else if (!strcmp(cn,"assert_ne") || !strcmp(cn,"builtin.assert_ne")) {
                if (narg>=2 && nc_eq(cargs[0],cargs[1])) {
                    char *a=nc_to_str_raw(cargs[0]);
                    char msg[512]; snprintf(msg,sizeof(msg),"assert_ne feilet: %s == %s",a,a);
                    free(a); nc_throw(msg);
                }
            }
            else if (!strcmp(cn,"tekst_fra_bool")||!strcmp(cn,"tekst_fra_boolsk")) {
                fn_r = nc_str(narg>0 && nc_truthy(cargs[0]) ? "sann" : "usann");
            }
            else if (!strcmp(cn,"desimaltall"))     fn_r=nc_builtin_desimaltall(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"n"))                fn_r=nc_builtin_n(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"fil_append"))       { if(narg>=2) nc_builtin_fil_append(cargs[0],cargs[1]); }
            /* ── async/await — synchronous implementation ── */
            else if (!strcmp(cn,"await_value") || !strcmp(cn,"builtin.await_value")) {
                NcVal *v = narg>0 ? cargs[0] : nc_nil();
                const char *_async_exc = NULL;
                if (v && v->type == NC_MAP) {
                    NcVal *tm = nc_index_get(v, nc_str("__timeout"));
                    if (tm && nc_truthy(tm)) _async_exc = "TimeoutFeil: vent.timeout utlaupt";
                    if (!_async_exc) {
                        NcVal *cm = nc_index_get(v, nc_str("__kansellert"));
                        if (cm && nc_truthy(cm)) _async_exc = "AvbruttFeil: vent.kanseller avbrutt";
                    }
                }
                if (_async_exc) {
                    free(cargs); free(callee);
                    if (try_depth > 0) {
                        strncpy(last_exception, _async_exc, sizeof(last_exception)-1);
                        const char *_cl = try_stack[try_depth-1].catch_lbl;
                        sp = try_stack[try_depth-1].sp_depth;
                        try_depth--;
                        NcVal *_tgt = nc_index_get(label_map, nc_str(_cl));
                        if (_tgt && _tgt->type == NC_INT) { ip = (int)_tgt->i; continue; }
                    }
                    nc_throw(_async_exc);
                }
                fn_r = v;
            }
            else if (!strcmp(cn,"vent.sov") || !strcmp(cn,"builtin.vent.sov")) { /* noop — sync runtime */ }
            else if (!strcmp(cn,"vent.timeout") || !strcmp(cn,"builtin.vent.timeout")) {
                /* vent.timeout(value, ms) — ms=0 → always timed out in sync mode */
                NcVal *m = nc_map_new();
                nc_index_set(m, nc_str("__timeout"), nc_bool(1));
                nc_index_set(m, nc_str("value"), narg>0 ? cargs[0] : nc_nil());
                fn_r = m;
            }
            else if (!strcmp(cn,"vent.er_timeoutet") || !strcmp(cn,"builtin.vent.er_timeoutet")) {
                NcVal *v = narg>0 ? cargs[0] : nc_nil();
                NcVal *tm = (v && v->type==NC_MAP) ? nc_index_get(v, nc_str("__timeout")) : NULL;
                fn_r = nc_bool(tm && nc_truthy(tm));
            }
            else if (!strcmp(cn,"vent.kanseller") || !strcmp(cn,"builtin.vent.kanseller")) {
                NcVal *m = nc_map_new();
                nc_index_set(m, nc_str("__kansellert"), nc_bool(1));
                fn_r = m;
            }
            else if (!strcmp(cn,"vent.er_kansellert") || !strcmp(cn,"builtin.vent.er_kansellert")) {
                NcVal *v = narg>0 ? cargs[0] : nc_nil();
                NcVal *cm = (v && v->type==NC_MAP) ? nc_index_get(v, nc_str("__kansellert")) : NULL;
                fn_r = nc_bool(cm && nc_truthy(cm));
            }
            /* ── std.db — SQLite backend ── */
            else if (!strcmp(cn,"db.open") || !strcmp(cn,"builtin.db.open")) {
                char *p = nc_to_str_raw(narg>0?cargs[0]:nc_nil()); sqlite3 *db=NULL;
                fn_r = (sqlite3_open(p,&db)==SQLITE_OK) ? nc_db_make_handle(db,-1) : nc_nil(); free(p);
            }
            else if (!strcmp(cn,"db.close") || !strcmp(cn,"builtin.db.close")) {
                NcVal *h=narg>0?cargs[0]:nc_nil();
                sqlite3 *db=nc_db_handle_ptr(h);
                if (!db) { fn_r=nc_bool(0); } else {
                    NcVal *pv=nc_index_get(h,nc_str("__pool"));
                    if (pv&&pv->type==NC_INT&&pv->i>=0&&(int)pv->i<g_nc_db_pool_count) {
                        int idx=(int)pv->i;
                        if (g_nc_db_pools[idx].avail<NC_DB_MAX_POOL_CONNS)
                            g_nc_db_pools[idx].conns[g_nc_db_pools[idx].avail++]=db;
                        else sqlite3_close(db);
                    } else sqlite3_close(db);
                    fn_r=nc_bool(1);
                }
            }
            else if (!strcmp(cn,"db.ping") || !strcmp(cn,"builtin.db.ping")) { fn_r=nc_bool(nc_db_handle_ptr(narg>0?cargs[0]:nc_nil())!=NULL); }
            else if (!strcmp(cn,"db.execute") || !strcmp(cn,"builtin.db.execute")) {
                sqlite3 *db=nc_db_handle_ptr(narg>0?cargs[0]:nc_nil());
                if (!db) { fn_r=nc_int(-1); } else {
                    char *s=nc_to_str_raw(narg>1?cargs[1]:nc_nil()); char *e=NULL;
                    sqlite3_exec(db,s,NULL,NULL,&e); free(s);
                    if (e) { sqlite3_free(e); fn_r=nc_int(-1); } else fn_r=nc_int(sqlite3_changes(db));
                }
            }
            else if (!strcmp(cn,"db.query_text") || !strcmp(cn,"builtin.db.query_text")) {
                sqlite3 *db=nc_db_handle_ptr(narg>0?cargs[0]:nc_nil());
                fn_r=nc_str(""); if (db) {
                    char *s=nc_to_str_raw(narg>1?cargs[1]:nc_nil()); sqlite3_stmt *st=NULL;
                    if (sqlite3_prepare_v2(db,s,-1,&st,NULL)==SQLITE_OK) {
                        if (sqlite3_step(st)==SQLITE_ROW) {
                            const unsigned char *t=sqlite3_column_text(st,0);
                            if (t) fn_r=nc_str((const char*)t);
                        }
                        sqlite3_finalize(st);
                    }
                    free(s);
                }
            }
            else if (!strcmp(cn,"db.query_int") || !strcmp(cn,"builtin.db.query_int")) {
                sqlite3 *db=nc_db_handle_ptr(narg>0?cargs[0]:nc_nil());
                fn_r=nc_int(0); if (db) {
                    char *s=nc_to_str_raw(narg>1?cargs[1]:nc_nil()); sqlite3_stmt *st=NULL;
                    if (sqlite3_prepare_v2(db,s,-1,&st,NULL)==SQLITE_OK) {
                        if (sqlite3_step(st)==SQLITE_ROW) fn_r=nc_int((long long)sqlite3_column_int64(st,0));
                        sqlite3_finalize(st);
                    }
                    free(s);
                }
            }
            else if (!strcmp(cn,"db.begin") || !strcmp(cn,"builtin.db.begin")) {
                sqlite3 *db=nc_db_handle_ptr(narg>0?cargs[0]:nc_nil()); char *e=NULL;
                fn_r = (db && sqlite3_exec(db,"BEGIN",NULL,NULL,&e)==SQLITE_OK) ? nc_bool(1) : nc_bool(0);
                if (e) sqlite3_free(e);
            }
            else if (!strcmp(cn,"db.commit") || !strcmp(cn,"builtin.db.commit")) {
                sqlite3 *db=nc_db_handle_ptr(narg>0?cargs[0]:nc_nil()); char *e=NULL;
                fn_r = (db && sqlite3_exec(db,"COMMIT",NULL,NULL,&e)==SQLITE_OK) ? nc_bool(1) : nc_bool(0);
                if (e) sqlite3_free(e);
            }
            else if (!strcmp(cn,"db.rollback") || !strcmp(cn,"builtin.db.rollback")) {
                sqlite3 *db=nc_db_handle_ptr(narg>0?cargs[0]:nc_nil()); char *e=NULL;
                fn_r = (db && sqlite3_exec(db,"ROLLBACK",NULL,NULL,&e)==SQLITE_OK) ? nc_bool(1) : nc_bool(0);
                if (e) sqlite3_free(e);
            }
            else if (!strcmp(cn,"db.migrate") || !strcmp(cn,"builtin.db.migrate")) {
                sqlite3 *db=nc_db_handle_ptr(narg>0?cargs[0]:nc_nil());
                fn_r=nc_int(0); if (db) {
                    char *s=nc_to_str_raw(narg>1?cargs[1]:nc_nil());
                    sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS __nc_migrations(hash TEXT PRIMARY KEY)",NULL,NULL,NULL);
                    char hstr[24]; snprintf(hstr,sizeof(hstr),"%llu",nc_db_sql_hash(s));
                    char qsql[80]; snprintf(qsql,sizeof(qsql),"SELECT 1 FROM __nc_migrations WHERE hash='%s'",hstr);
                    sqlite3_stmt *chk=NULL; int already=0;
                    if (sqlite3_prepare_v2(db,qsql,-1,&chk,NULL)==SQLITE_OK) { if(sqlite3_step(chk)==SQLITE_ROW) already=1; sqlite3_finalize(chk); }
                    if (!already) {
                        char *e=NULL; int rc=sqlite3_exec(db,s,NULL,NULL,&e);
                        if (!e && rc==SQLITE_OK) {
                            char isql[80]; snprintf(isql,sizeof(isql),"INSERT INTO __nc_migrations VALUES('%s')",hstr);
                            sqlite3_exec(db,isql,NULL,NULL,NULL); fn_r=nc_int(1);
                        } else if (e) sqlite3_free(e);
                    }
                    free(s);
                }
            }
            else if (!strcmp(cn,"db.transaction") || !strcmp(cn,"builtin.db.transaction")) {
                sqlite3 *db=nc_db_handle_ptr(narg>0?cargs[0]:nc_nil());
                fn_r=nc_int(0); if (db) {
                    char *s=nc_to_str_raw(narg>1?cargs[1]:nc_nil()); char *e=NULL;
                    sqlite3_exec(db,"BEGIN",NULL,NULL,NULL);
                    int rc=sqlite3_exec(db,s,NULL,NULL,&e); free(s);
                    if (e||rc!=SQLITE_OK) { if(e) sqlite3_free(e); sqlite3_exec(db,"ROLLBACK",NULL,NULL,NULL); }
                    else { fn_r=nc_int(sqlite3_changes(db)); sqlite3_exec(db,"COMMIT",NULL,NULL,NULL); }
                }
            }
            else if (!strcmp(cn,"db.pool") || !strcmp(cn,"builtin.db.pool")) {
                char *p=nc_to_str_raw(narg>0?cargs[0]:nc_nil());
                int sz=narg>1&&cargs[1]&&cargs[1]->type==NC_INT?(int)cargs[1]->i:1;
                int idx=g_nc_db_pool_count++;
                if (idx<NC_DB_MAX_POOLS) {
                    strncpy(g_nc_db_pools[idx].path,p,511); g_nc_db_pools[idx].avail=0;
                    sqlite3 *db=NULL;
                    if (sqlite3_open(p,&db)==SQLITE_OK) g_nc_db_pools[idx].conns[g_nc_db_pools[idx].avail++]=db;
                    NcVal *m=nc_map_new(); nc_index_set(m,nc_str("__pool"),nc_int(idx)); (void)sz;
                    fn_r=m;
                } else fn_r=nc_nil();
                free(p);
            }
            else if (!strcmp(cn,"db.pool_size") || !strcmp(cn,"builtin.db.pool_size")) {
                NcVal *p=narg>0?cargs[0]:nc_nil(); fn_r=nc_int(0);
                if (p&&p->type==NC_MAP) { NcVal *iv=nc_index_get(p,nc_str("__pool"));
                    if (iv&&iv->type==NC_INT&&iv->i<g_nc_db_pool_count) fn_r=nc_int(g_nc_db_pools[(int)iv->i].avail); }
            }
            else if (!strcmp(cn,"db.pool_acquire") || !strcmp(cn,"builtin.db.pool_acquire")) {
                NcVal *pool=narg>0?cargs[0]:nc_nil(); fn_r=nc_nil();
                if (pool&&pool->type==NC_MAP) { NcVal *iv=nc_index_get(pool,nc_str("__pool"));
                    if (iv&&iv->type==NC_INT) { int idx=(int)iv->i;
                        if (idx<g_nc_db_pool_count) {
                            sqlite3 *db=NULL;
                            if (g_nc_db_pools[idx].avail>0) db=g_nc_db_pools[idx].conns[--g_nc_db_pools[idx].avail];
                            else sqlite3_open(g_nc_db_pools[idx].path,&db);
                            if (db) fn_r=nc_db_make_handle(db,idx);
                        }
                    }
                }
            }
            else if (!strcmp(cn,"db.pool_close") || !strcmp(cn,"builtin.db.pool_close")) {
                NcVal *pool=narg>0?cargs[0]:nc_nil(); fn_r=nc_bool(0);
                if (pool&&pool->type==NC_MAP) { NcVal *iv=nc_index_get(pool,nc_str("__pool"));
                    if (iv&&iv->type==NC_INT&&iv->i<g_nc_db_pool_count) {
                        int idx=(int)iv->i;
                        for (int _i=0;_i<g_nc_db_pools[idx].avail;_i++) sqlite3_close(g_nc_db_pools[idx].conns[_i]);
                        g_nc_db_pools[idx].avail=0; fn_r=nc_bool(1);
                    }
                }
            }
            /* ir.* / selfhost.ir_contract.* (lazy ir_contract.no) */
            else if (nc_is_ir_contract_api(cn) || !strncmp(cn, "builtin.ir.", 11)) {
                const char *ir_cn = !strncmp(cn, "builtin.ir.", 11) ? cn + 8 : cn;
                fn_r = nc_call_ir_contract_api(ir_cn, cargs, narg);
            }
            /* sh.* / selfhost.common.* / selfhost.compiler.* (lazy common.no) */
            else if (nc_is_sh_api(cn) || !strncmp(cn, "builtin.sh.", 11)) {
                const char *sh_cn = !strncmp(cn, "builtin.sh.", 11) ? cn + 8 : cn;
                if (try_depth > 0) {
                    /* Bruk setjmp-veg så prøv/fang i kallaren kan fange unntak */
                    jmp_buf _sh_saved_jmp;
                    memcpy(&_sh_saved_jmp, &g_err_jmp, sizeof(jmp_buf));
                    int _sh_caught = 0;
                    if (setjmp(g_err_jmp)) {
                        memcpy(&g_err_jmp, &_sh_saved_jmp, sizeof(jmp_buf));
                        if (try_depth > 0) {
                            const char *_em = g_err_msg;
                            if (strncmp(_em,"Norscode unntak: ",17)==0) _em+=17;
                            else if (strncmp(_em,"nc-vm feil: Norscode unntak: ",28)==0) _em+=28;
                            strncpy(last_exception, _em, sizeof(last_exception)-1);
                            g_err_msg[0] = 0;
                            const char *_cl = try_stack[try_depth - 1].catch_lbl;
                            sp = try_stack[try_depth - 1].sp_depth;
                            NcVal *_tgt = nc_index_get(label_map, nc_str(_cl));
                            if (_tgt && _tgt->type == NC_INT) { ip = (int)_tgt->i + 1; _sh_caught = 1; }
                        }
                        fn_r = nc_nil();
                    } else {
                        /* Prøv bundle-funksjonen først, deretter sh-api */
                        NcVal *_local = nc_exec_find_fn(functions, callee);
                        if (_local) fn_r = nc_exec_call(functions, callee, cargs, narg, depth+1);
                        else fn_r = nc_call_sh_api(sh_cn, cargs, narg);
                        memcpy(&g_err_jmp, &_sh_saved_jmp, sizeof(jmp_buf));
                    }
                    if (_sh_caught) { free(cargs); free(callee); continue; }
                } else {
                    fn_r = nc_call_sh_api(sh_cn, cargs, narg);
                }
            }
            /* Globale assert-helpers for testar */
            else if (!strcmp(cn,"assert_starter_med")) fn_r=nc_stub_assert_starter_med(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"assert_slutter_med")) fn_r=nc_stub_assert_slutter_med(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"assert_inneholder") || !strcmp(cn,"builtin.assert_inneholder")) {
                NcVal *s2=narg>0?cargs[0]:nc_nil(), *sub=narg>1?cargs[1]:nc_nil();
                if (!nc_truthy(nc_builtin_contains(s2,sub))) {
                    char *sv=nc_to_str_raw(s2), *pv=nc_to_str_raw(sub);
                    char msg[512]; snprintf(msg,sizeof(msg),"assert_inneholder: '%s' inneheld ikkje '%s'",sv,pv);
                    free(sv); free(pv); nc_throw(msg);
                }
                fn_r = nc_nil();
            }
            else if (!strcmp(cn,"t.inneholder") || !strcmp(cn,"builtin.t.inneholder")) fn_r=nc_builtin_contains(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"path.join"))          fn_r=nc_stub_path_join(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"web_escape_html")||!strcmp(cn,"html.escape")) fn_r=nc_stub_web_escape_html(narg>0?cargs[0]:nc_nil());
            /* env.*, json.*, t.* aliases */
            else if (!strcmp(cn,"env.finnes")||!strcmp(cn,"env_finnes")) fn_r=nc_builtin_miljo_finnes(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"env.hent")||!strcmp(cn,"env_hent"))     fn_r=nc_builtin_miljo_hent(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json.parse")||!strcmp(cn,"json_parse")) fn_r=nc_builtin_json_parse_norscode(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json.parse_raw")||!strcmp(cn,"json_parse_raw")) fn_r=nc_builtin_json_parse_raw(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json.stringify")||!strcmp(cn,"json_stringify")) fn_r=nc_builtin_json_stringify_smart(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json_skriv")) fn_r=nc_builtin_json_stringify(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json.hent_tekst")||!strcmp(cn,"builtin.json.hent_tekst")) {
                NcVal *v = nc_index_get(narg>0?cargs[0]:nc_nil(), narg>1?cargs[1]:nc_nil());
                fn_r = v ? nc_str(nc_to_str_raw(v)) : nc_str("");
            }
            else if (!strcmp(cn,"json.hent_tall")||!strcmp(cn,"builtin.json.hent_tall")) {
                NcVal *v = nc_index_get(narg>0?cargs[0]:nc_nil(), narg>1?cargs[1]:nc_nil());
                if (v && v->type==NC_INT) fn_r = v;
                else {
                    char *sv = v ? nc_to_str_raw(v) : NULL;
                    long long iv = sv ? atoll(sv) : 0;
                    if (sv) free(sv);
                    fn_r = nc_int(iv);
                }
            }
            else if (!strcmp(cn,"json.hent_bool")||!strcmp(cn,"builtin.json.hent_bool")) {
                NcVal *v = nc_index_get(narg>0?cargs[0]:nc_nil(), narg>1?cargs[1]:nc_nil());
                if (!v || v->type == NC_NIL) fn_r = nc_bool(0);
                else if (v->type == NC_BOOL) fn_r = v;
                else if (v->type == NC_INT) fn_r = nc_bool(v->i != 0);
                else {
                    char *sv = nc_to_str_raw(v);
                    int truthy = (strcmp(sv, "false") != 0 && strcmp(sv, "usann") != 0 && strcmp(sv, "0") != 0 && sv[0] != '\0');
                    free(sv);
                    fn_r = nc_bool(truthy);
                }
            }
            else if (!strcmp(cn,"liste.første_tall")||!strcmp(cn,"builtin.liste.første_tall")) {
                NcVal *lst = narg>0 ? cargs[0] : nc_nil();
                fn_r = (lst && lst->type==NC_LIST && lst->list->len>0) ? lst->list->items[0] : nc_int(0);
            }
            else if (!strcmp(cn,"liste.første_tekst")||!strcmp(cn,"builtin.liste.første_tekst")) {
                NcVal *lst = narg>0 ? cargs[0] : nc_nil();
                fn_r = (lst && lst->type==NC_LIST && lst->list->len>0) ? lst->list->items[0] : nc_str("");
            }
            else if (!strcmp(cn,"liste.antall_tall")||!strcmp(cn,"builtin.liste.antall_tall")) fn_r = nc_int(narg>0 && cargs[0] && cargs[0]->type==NC_LIST ? cargs[0]->list->len : 0);
            else if (!strcmp(cn,"liste.antall_tekst")||!strcmp(cn,"builtin.liste.antall_tekst")) fn_r = nc_int(narg>0 && cargs[0] && cargs[0]->type==NC_LIST ? cargs[0]->list->len : 0);
            else if (!strcmp(cn,"ordbok.antall_tall")||!strcmp(cn,"builtin.ordbok.antall_tall")) fn_r = nc_int(narg>0 && cargs[0] && cargs[0]->type==NC_MAP ? cargs[0]->map->len : 0);
            else if (!strcmp(cn,"fil.skriv_fil") || !strcmp(cn,"builtin.fil.skriv_fil")) { if(narg>=2) nc_builtin_fil_skriv(cargs[0],cargs[1]); fn_r = nc_nil(); }
            else if (!strcmp(cn,"fil_skriv_binar") || !strcmp(cn,"builtin.fil_skriv_binar")) { if(narg>=2) nc_builtin_fil_skriv_binary(cargs[0],cargs[1]); fn_r = nc_nil(); }
            else if (!strcmp(cn,"lagring.sett_tekst") || !strcmp(cn,"builtin.lagring.sett_tekst")) { if(narg>=3) nc_index_set(cargs[0], cargs[1], cargs[2]); fn_r = nc_nil(); }
            else if (!strcmp(cn,"lagring.hent_tekst") || !strcmp(cn,"builtin.lagring.hent_tekst")) {
                NcVal *v = narg>=2 ? nc_index_get(cargs[0], cargs[1]) : nc_nil();
                fn_r = (v && v->type != NC_NIL) ? v : nc_str("");
            }
            /* Sti-hjelpere */
            else if (!strcmp(cn,"sti_join")||!strcmp(cn,"path.join"))     fn_r=nc_builtin_sti_join(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"sti_basename")||!strcmp(cn,"path.basename")) fn_r=nc_builtin_sti_basename(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"sti_dirname")||!strcmp(cn,"path.dirname"))   fn_r=nc_builtin_sti_dirname(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"sti_exists")||!strcmp(cn,"path.exists"))     fn_r=nc_builtin_sti_exists(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"sti_stem")||!strcmp(cn,"path.stem"))         fn_r=nc_builtin_sti_stem(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"miljo_sett")||!strcmp(cn,"env.sett"))        fn_r=nc_builtin_miljo_sett(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            /* Web-builtins */
            else if (!strcmp(cn,"ncb_route_handlers")) fn_r=nc_builtin_ncb_route_handlers(cargs,narg);
            else if (!strcmp(cn,"ncb_metadata"))       fn_r=nc_builtin_ncb_metadata(cargs,narg);
            else if (!strcmp(cn,"ncb_next_request_id"))fn_r=nc_builtin_ncb_next_request_id(cargs,narg);
            else if (!strcmp(cn,"ncb_call_fn")) {
                /* ncb_call_fn kan kaste unntak — bruk setjmp ved try_depth > 0 */
                if (try_depth > 0) {
                    jmp_buf _ncb_saved;
                    memcpy(&_ncb_saved, &g_err_jmp, sizeof(jmp_buf));
                    int _ncb_caught = 0;
                    if (setjmp(g_err_jmp)) {
                        memcpy(&g_err_jmp, &_ncb_saved, sizeof(jmp_buf));
                        if (try_depth > 0) {
                            const char *_em = g_err_msg;
                            if (strncmp(_em,"Norscode unntak: ",17)==0) _em+=17;
                            else if (strncmp(_em,"nc-vm feil: Norscode unntak: ",28)==0) _em+=28;
                            strncpy(last_exception, _em, sizeof(last_exception)-1);
                            g_err_msg[0] = 0;
                            const char *_cl = try_stack[try_depth-1].catch_lbl;
                            sp = try_stack[try_depth-1].sp_depth;
                            NcVal *_tgt = nc_index_get(label_map, nc_str(_cl));
                            if (_tgt && _tgt->type == NC_INT) { ip = (int)_tgt->i + 1; _ncb_caught = 1; }
                        }
                        fn_r = nc_nil();
                    } else {
                        fn_r = nc_builtin_ncb_call_fn(cargs, narg);
                        memcpy(&g_err_jmp, &_ncb_saved, sizeof(jmp_buf));
                    }
                    if (_ncb_caught) { free(cargs); free(callee); continue; }
                } else {
                    fn_r = nc_builtin_ncb_call_fn(cargs, narg);
                }
            }
            /* openapi_json er implementert i std/web.no — ikkje C-stub her */
            else if (cn[0]>='A'&&cn[0]<='Z')        fn_r=nc_map_new(); /* struct constructor */
            else {
                /* ── STDLIB DISPATCH (std.path.*, std.env.*, builtin.path.*, builtin.env.*) ── */
                if (!strncmp(cn, "std.path.basename", 17)) fn_r = nc_std_path_basename(cargs, narg);
                else if (!strncmp(cn, "std.path.dirname", 16)) fn_r = nc_std_path_dirname(cargs, narg);
                else if (!strncmp(cn, "std.path.stem", 13)) fn_r = nc_std_path_stem(cargs, narg);
                else if (!strncmp(cn, "std.path.join", 13)) fn_r = nc_std_path_join(cargs, narg);
                else if (!strncmp(cn, "std.path.exists", 15)) fn_r = nc_std_path_exists(cargs, narg);
                else if (!strncmp(cn, "std.env.sett", 12)) fn_r = nc_std_env_sett(cargs, narg);
                else if (!strncmp(cn, "std.env.hent", 12)) fn_r = nc_std_env_hent(cargs, narg);
                else if (!strncmp(cn, "std.env.finnes", 14)) fn_r = nc_std_env_finnes(cargs, narg);
                /* builtin.path.* dispatchers (same handlers as std.path.*) */
                else if (!strncmp(cn, "builtin.path.basename", 21)) fn_r = nc_std_path_basename(cargs, narg);
                else if (!strncmp(cn, "builtin.path.dirname", 20)) fn_r = nc_std_path_dirname(cargs, narg);
                else if (!strncmp(cn, "builtin.path.stem", 17)) fn_r = nc_std_path_stem(cargs, narg);
                else if (!strncmp(cn, "builtin.path.join", 17)) fn_r = nc_std_path_join(cargs, narg);
                else if (!strncmp(cn, "builtin.path.exists", 19)) fn_r = nc_std_path_exists(cargs, narg);
                else if (!strncmp(cn, "builtin.env.sett", 16)) fn_r = nc_std_env_sett(cargs, narg);
                else if (!strncmp(cn, "builtin.env.hent", 16)) fn_r = nc_std_env_hent(cargs, narg);
                else if (!strncmp(cn, "builtin.env.finnes", 18)) fn_r = nc_std_env_finnes(cargs, narg);
                else if (!strncmp(cn, "builtin.web.request_context", 27)) {
                    fn_r = nc_map_new();
                    nc_index_set(fn_r, nc_str("method"), narg > 0 ? cargs[0] : nc_str(""));
                    nc_index_set(fn_r, nc_str("path"), narg > 1 ? cargs[1] : nc_str(""));
                    nc_index_set(fn_r, nc_str("params"), narg > 2 ? cargs[2] : nc_map_new());
                    nc_index_set(fn_r, nc_str("headers"), narg > 3 ? cargs[3] : nc_map_new());
                    nc_index_set(fn_r, nc_str("body"), narg > 4 ? cargs[4] : nc_str(""));
                }

                else {
                NcVal *local_fn = nc_exec_find_fn(functions, callee);
                int dispatch_first = nc_exec_prefer_dispatch(callee) && !nc_exec_prefer_local(callee);
                NcVal *dispatch_r = NULL;

                /* Dispatch: bruk berre om det IKKJE finst ein lokal funksjon som matchar.
                 * Dette hindrar at dispatch-tabell sin fuzzy-match overstyrer
                 * eksplisitt bundla modular (t.d. trace.start vs selfhost.lexer.lexer_m1.start). */
                if (dispatch_first && !local_fn) {
                    dispatch_r = nc_dispatch_call(callee, cargs, narg);
                    if (dispatch_r != NULL) {
                        free(cargs); free(callee);
                        nc_push(&sp, stack_arr, dispatch_r); ip++;
                        continue;
                    }
                }
                if (!dispatch_first && !local_fn) {
                    dispatch_r = nc_dispatch_call(callee, cargs, narg);
                    if (dispatch_r != NULL) {
                        free(cargs); free(callee);
                        nc_push(&sp, stack_arr, dispatch_r); ip++;
                        continue;
                    }
                }

                /* Fang cross-function exceptions for alle lokale kall */
                int _had_try = try_depth > 0;
                /* nc_try_call: kall med unntak-fang når try_depth > 0
                 * TRY_END i catch-blokken fjernar try-ramma, ikkje THROW/setjmp. */
                int _nc_exn_caught = 0;
                if (_had_try) {
                    jmp_buf _saved_jmp;
                    memcpy(&_saved_jmp, &g_err_jmp, sizeof(jmp_buf));
                    if (setjmp(g_err_jmp)) {
                        memcpy(&g_err_jmp, &_saved_jmp, sizeof(jmp_buf));
                        free(cargs); free(callee);
                        if (try_depth > 0) {
                            { const char *_em = g_err_msg;
                              if (strncmp(_em,"Norscode unntak: ",17)==0) _em+=17;
                              else if (strncmp(_em,"nc-vm feil: Norscode unntak: ",28)==0) _em+=28;
                              strncpy(last_exception, _em, sizeof(last_exception)-1); }
                            g_err_msg[0] = 0;
                            const char *cl2 = try_stack[try_depth - 1].catch_lbl;
                            sp = try_stack[try_depth - 1].sp_depth;
                            NcVal *tgt2 = nc_index_get(label_map, nc_str(cl2));
                            if (tgt2 && tgt2->type == NC_INT) { ip = (int)tgt2->i + 1; _nc_exn_caught = 1; }
                        }
                        fn_r = nc_nil();
                    } else {
                        fn_r = nc_exec_call(functions, callee, cargs, narg, depth+1);
                        memcpy(&g_err_jmp, &_saved_jmp, sizeof(jmp_buf));
                    }
                } else {
                    fn_r = nc_exec_call(functions, callee, cargs, narg, depth+1);
                }
                if (_nc_exn_caught) continue;
                } /* close else for stdlib check */
            }
            free(cargs); free(callee);
            nc_push(&sp,stack_arr,fn_r); ip++;
        } else if (!strcmp(op, "THROW")) {
            NcVal *e = nc_pop(&sp,stack_arr); char *s=nc_to_str_raw(e);
            if (try_depth > 0) {
                strncpy(last_exception, s, sizeof(last_exception)-1);
                free(s);
                /* Hopp til catch-label — TRY_END i catch-blokken fjernar frå stacken */
                const char *cl = try_stack[try_depth - 1].catch_lbl;
                sp = try_stack[try_depth - 1].sp_depth;
                NcVal *tgt = nc_index_get(label_map, nc_str(cl));
                if (tgt && tgt->type == NC_INT) { ip = (int)tgt->i; continue; }
            }
            nc_throw(s); free(s); ip++;
        } else if (!strcmp(op, "TRY_BEGIN")) {
            if (instr->list->len >= 2 && try_depth < 32) {
                char *cl = nc_to_str_raw(instr->list->items[1]);
                try_stack[try_depth].catch_lbl = strdup(cl); free(cl);
                try_stack[try_depth].sp_depth = sp;
                try_depth++;
            }
            ip++;
        } else if (!strcmp(op, "TRY_END")) {
            if (try_depth > 0) try_depth--;
            ip++;
        } else if (!strcmp(op, "LOAD_EXCEPTION")) {
            nc_push(&sp, stack_arr, nc_str(last_exception[0] ? last_exception : "ukjent feil"));
            last_exception[0] = 0;
            ip++;
        } else {
            ip++;
        }
    }
done:
    free(stack_arr); free(vars_arr); free(varnames);
    return retval;
}

/* g_nc_closure defined via macro above */

/* Køyr lambda med closure-kontekst */
static NcVal *nc_exec_call_closure(NcVal *functions, const char *fn_name, NcVal **args, int nargs, NcVal *closure, int depth) {
    NcVal *saved = g_nc_closure;
    g_nc_closure = closure;
    NcVal *r = nc_exec_call(functions, fn_name, args, nargs, depth);
    g_nc_closure = saved;
    return r;
}

/* ── Wrap kompiler_fil via bootstrap-host dispatcher ── */
static NcVal *nc_native_kompiler(const char *src_path, const char *modul) {
    NcVal *src = nc_builtin_fil_les(nc_str(src_path));
    if (!src || src->type != NC_STR) return nc_nil();
    NcVal *src_v = nc_str(src->s);
    NcVal *mod = nc_str(modul);
    NcVal *args[] = {src_v, mod};
    return nc_dispatch_call("selfhost.kompiler.kompiler_fil", args, 2);
}


static void nc_merge_fns(NcVal *dst, NcVal *src) {
    if (!dst || dst->type != NC_MAP || !src || src->type != NC_MAP) return;
    for (int i = 0; i < src->map->len; i++) {
        NcVal *existing = nc_index_get(dst, nc_str(src->map->keys[i]));
        if (!existing || existing->type == NC_NIL)
            nc_index_set(dst, nc_str(src->map->keys[i]), src->map->vals[i]);
    }
}

/* ── Standard Library Dispatch Handlers (implementations) ── */
static NcVal *nc_std_path_basename(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_nil();
    const char *path = args[0]->s;
    const char *last = strrchr(path, '/');
    return nc_str(last ? last + 1 : path);
}

static NcVal *nc_std_path_dirname(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_nil();
    const char *path = args[0]->s;
    const char *last = strrchr(path, '/');
    if (!last) return nc_str(".");
    if (last == path) return nc_str("/");
    char buf[1024];
    snprintf(buf, (size_t)(last - path + 1), "%s", path);
    return nc_str(buf);
}

static NcVal *nc_std_path_stem(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_nil();
    const char *path = args[0]->s;
    const char *last_slash = strrchr(path, '/');
    const char *base = last_slash ? last_slash + 1 : path;
    const char *last_dot = strrchr(base, '.');
    if (!last_dot) return nc_str(base);
    char buf[1024];
    snprintf(buf, (size_t)(last_dot - base + 1), "%s", base);
    return nc_str(buf);
}

static NcVal *nc_std_path_join(NcVal **args, int na) {
    if (na < 2) return nc_nil();
    const char *a = (args[0] && args[0]->type == NC_STR) ? args[0]->s : "";
    const char *b = (args[1] && args[1]->type == NC_STR) ? args[1]->s : "";
    if (!*a) return nc_str(b);
    if (!*b) return nc_str(a);
    char buf[2048];
    snprintf(buf, sizeof(buf), "%s/%s", a, b);
    return nc_str(buf);
}

static NcVal *nc_std_path_exists(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_bool(0);
    FILE *f = fopen(args[0]->s, "r");
    if (f) { fclose(f); return nc_bool(1); }
    return nc_bool(0);
}

static NcVal *nc_std_env_sett(NcVal **args, int na) {
    if (na < 2) return nc_nil();
    if (!g_std_env) g_std_env = nc_map_new();
    const char *key = (args[0] && args[0]->type == NC_STR) ? args[0]->s : "";
    NcVal *val = args[1];
    nc_index_set(g_std_env, nc_str(key), val);
    return val;
}

static NcVal *nc_std_env_hent(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_nil();
    if (!g_std_env) return nc_nil();
    return nc_index_get(g_std_env, nc_str(args[0]->s));
}

static NcVal *nc_std_env_finnes(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_bool(0);
    if (!g_std_env) return nc_bool(0);
    NcVal *v = nc_index_get(g_std_env, nc_str(args[0]->s));
    return nc_bool(v && v->type != NC_NIL);
}

/* Initialize g_std_env */
static void nc_init_std_env() {
    if (!g_std_env) g_std_env = nc_map_new();
}

/* Host FFI: køyr ein namngitt funksjon i Gen1-NCB med to argument (for l5b bygg_bundle) */
NcVal *nc_fn_builtin_host_kall_bygg_bundle(NcVal **args, int na) {
    if (na < 3 || !args[0] || args[0]->type != NC_STR) return nc_int(1);
    NcVal *ncb = nc_builtin_json_parse_str(args[0]);
    if (!ncb || ncb->type != NC_MAP) return nc_int(1);
    NcVal *fns_v = nc_index_get(ncb, nc_str("functions"));
    if (!fns_v || fns_v->type != NC_MAP) {
        fprintf(stderr, "NCB manglar functions\n"); return nc_int(1);
    }
    if (!nc_exec_find_fn(fns_v, "selfhost.bundler.bygg_bundle")) {
        fprintf(stderr, "NCB manglar selfhost.bundler.bygg_bundle\n"); return nc_int(1);
    }
    NcVal *call_args[] = { args[1], args[2] };
    NcVal *r = nc_exec_call(fns_v, "selfhost.bundler.bygg_bundle", call_args, 2, 0);
    int code = nc_val_til_exit(r);
    return nc_int(code != 0 ? code : 0);
}

/* Global: route_handlers frå køyrande NCB; tilgjengeleg via builtin.ncb_route_handlers() */
static NcVal *g_current_route_handlers = NULL;
static NcVal *g_current_functions = NULL;

/* Ikkje static — norscode_generated.c kan ha forward-deklarasjonar for desse */
static NcVal *g_current_ncb = NULL;
static int g_request_counter = 0;

NcVal *nc_builtin_ncb_route_handlers(NcVal **args, int na) {
    (void)args; (void)na;
    return g_current_route_handlers ? g_current_route_handlers : nc_list_new();
}
NcVal *nc_builtin_ncb_metadata(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_list_new();
    if (!g_current_ncb) return nc_list_new();
    NcVal *v = nc_index_get(g_current_ncb, args[0]);
    return (v && v->type != NC_NIL) ? v : nc_list_new();
}
NcVal *nc_builtin_ncb_next_request_id(NcVal **args, int na) {
    (void)args; (void)na;
    g_request_counter++;
    char buf[32]; snprintf(buf, sizeof(buf), "req-%d", g_request_counter);
    return nc_str(buf);
}
NcVal *nc_builtin_ncb_call_fn(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_nil();
    if (!g_current_functions) return nc_nil();
    /* Pass all args except the first (fn name) */
    int nfn_args = na - 1;
    NcVal **fn_args = nfn_args > 0 ? &args[1] : NULL;
    return nc_exec_call(g_current_functions, args[0]->s, fn_args, nfn_args, 0);
}

/* Host FFI: køyr NCB via C-exec (same motor som standard run), brukt av selfhost.nc_main.no */
NcVal *nc_fn_builtin_host_exec_ncb_json(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_int(1);
    NcVal *ncb = nc_builtin_json_parse_str(args[0]);
    if (!ncb || ncb->type != NC_MAP) return nc_int(1);
    NcVal *fns_v = nc_index_get(ncb, nc_str("functions"));
    NcVal *entry_v = nc_index_get(ncb, nc_str("entry"));
    if (!fns_v || fns_v->type != NC_MAP) return nc_int(1);
    char *entry = nc_to_str_raw(entry_v);
    if (!entry || !entry[0]) { free(entry); return nc_int(1); }
    /* Lagre route_handlers, NCB og functions for std.web */
    NcVal *rh = nc_index_get(ncb, nc_str("route_handlers"));
    g_current_route_handlers = (rh && rh->type != NC_NIL) ? rh : nc_list_new();
    g_current_ncb = ncb;
    g_request_counter = 0;
    /* Ikkje la barne-NCB arve host NORSCODE_* (unngår driver-rekursjon ved nc run) */
    const char *env_keys[] = {
        "NORSCODE_CMD", "NORSCODE_FILE", "NORSCODE_OUTPUT",
        "NORSCODE_MODULE", NULL
    };
    char *saved[8];
    int nsaved = 0;
    for (int i = 0; env_keys[i]; i++) {
        const char *v = getenv(env_keys[i]);
        saved[nsaved] = v ? strdup(v) : NULL;
        nsaved++;
        unsetenv(env_keys[i]);
    }
    // TODO: Steg C - skip runtime-compilation av selfhost/common.no (infinite loop på stor fil)
    // Forutsetter at common-funksjonar allereie er i kompilert NCB
    // nc_ensure_sh_common();
    // if (g_sh_common_fns) nc_merge_fns(fns_v, g_sh_common_fns);
    g_current_functions = fns_v;
    NcVal *r = nc_exec_call(fns_v, entry, NULL, 0, 0);
    g_current_functions = NULL;
    for (int i = 0; i < nsaved; i++) {
        if (saved[i]) {
            setenv(env_keys[i], saved[i], 1);
            free(saved[i]);
        } else {
            unsetenv(env_keys[i]);
        }
    }
    free(entry);
    return nc_int(nc_val_til_exit(r));
}

static int nc_val_til_exit(NcVal *v) {
    if (v && v->type == NC_INT) return (int)v->i;
    return 0;
}

/* Køyr selfhost.nc_main.start; returnerer exit-kode eller -1 ved feil */
static int nc_try_nc_main_host(void) {
    const char *cmd = getenv("NORSCODE_CMD");
    if (cmd && !strcmp(cmd, "l5b-gen2")) {
        unsetenv("NORSCODE_FILE");
        unsetenv("NORSCODE_OUTPUT");
    }
    NcVal *r = nc_dispatch_call("selfhost.nc_main.start", NULL, 0);
    if (!r) return -1;
    return nc_val_til_exit(r);
}

/* ── main() — alle kommandoar delegert til selfhost.nc_main.no ── */
int main(int argc, char **argv) {
    if (setjmp(g_err_jmp)) {
        fprintf(stderr, "norscode: %s\n", g_err_msg);
        return 1;
    }

    const char *cmd = getenv("NORSCODE_CMD");
    if (!cmd) cmd = "selftest";

    int rc = nc_try_nc_main_host();
    if (rc >= 0) return rc;

    fprintf(stderr, "norscode: ukjend kommando: %s\n", cmd);
    return 1;
}
