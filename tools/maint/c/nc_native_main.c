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

/* Kompilator-pipeline i bundle skal bruke bootstrap-host-dispatch, ikkje bytecode frå Gen1-NCB */
static int nc_exec_prefer_dispatch(const char *name) {
    if (!strncmp(name, "selfhost.kompiler.", 18)) return 1;
    if (!strncmp(name, "selfhost.lexer.", 15)) return 1;
    if (!strncmp(name, "selfhost.parser.", 16)) return 1;
    if (!strncmp(name, "selfhost.compiler.", 18)) return 1;
    if (!strncmp(name, "selfhost.json.", 14)) return 1;
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
    /* Siste segment */
    const char *last = strrchr(name, '.'); if (last) last++;
    else last = name;
    if (functions->type == NC_MAP) {
        for (int i = 0; i < functions->map->len; i++) {
            const char *fn = functions->map->keys[i];
            const char *fn_last = strrchr(fn, '.'); fn_last = fn_last ? fn_last+1 : fn;
            if (!strcmp(fn_last, last)) return functions->map->vals[i];
            if (!strcmp(fn, last)) return functions->map->vals[i];
        }
    }
    return NULL;
}

/* Forward decl */
static NcVal *g_nc_closure_fwd;
#define g_nc_closure g_nc_closure_fwd
static NcVal *nc_exec_call(NcVal *functions, const char *fn_name, NcVal **args, int nargs, int depth);
static NcVal *nc_exec_call_closure(NcVal *functions, const char *fn_name, NcVal **args, int nargs, NcVal *closure, int depth);
static int nc_val_til_exit(NcVal *v);
static NcVal *nc_native_kompiler(const char *src_path, const char *modul);

/* Lazy-load selfhost/common.no for sh.* / selfhost.common.* / selfhost.compiler.* */
static NcVal *g_sh_common_fns = NULL;

static void nc_ensure_sh_common(void) {
    if (g_sh_common_fns) return;
    NcVal *ncb_json = nc_native_kompiler("selfhost/common.no", "selfhost.common");
    if (!ncb_json || ncb_json->type != NC_STR) {
        nc_throw("Kunne ikkje kompilere selfhost/common.no");
        return;
    }
    NcVal *ncb = nc_builtin_json_parse_str(ncb_json);
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
    if (!nc_exec_find_fn(g_sh_common_fns, full) && !nc_exec_find_fn(g_sh_common_fns, short_fn)) {
        nc_panic("Ukjent selfhost API: %s", cn);
        return nc_nil();
    }
    return nc_exec_call(g_sh_common_fns, full, args, nargs, 0);
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
            else if (!strcmp(cn,"finnes_nøkkel")||!strcmp(cn,"har_nokkel")) fn_r=nc_builtin_finnes_nokkel(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"nøkler"))           fn_r=nc_builtin_nokler(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"verdier"))          fn_r=nc_builtin_verdier(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"fjern_nokkel"))     { if(narg>=2) nc_builtin_fjern_nokkel(cargs[0],cargs[1]); }
            else if (!strcmp(cn,"json_stringify"))   fn_r=nc_builtin_json_stringify_smart(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json_parse"))       fn_r=nc_builtin_json_parse_norscode(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"fil_les"))          fn_r=nc_builtin_fil_les(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"fil_skriv"))        { if(narg>=2) nc_builtin_fil_skriv(cargs[0],cargs[1]); }
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
            else if (!strcmp(cn,"math.pluss")||!strcmp(cn,"std.math.pluss"))   fn_r=nc_add(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.minus")||!strcmp(cn,"std.math.minus"))   fn_r=nc_sub(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.gange")||!strcmp(cn,"std.math.gange"))   fn_r=nc_mul(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.dele")||!strcmp(cn,"std.math.dele"))     fn_r=nc_div(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.rest")||!strcmp(cn,"std.math.rest"))     fn_r=nc_mod(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"math.abs")||!strcmp(cn,"std.math.abs")) {
                NcVal *v=narg>0?cargs[0]:nc_nil();
                fn_r = (v->type==NC_INT && v->i<0) ? nc_int(-v->i) : v;
            }
            else if (!strcmp(cn,"math.min")||!strcmp(cn,"std.math.min"))   fn_r=nc_truthy(nc_cmp(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil(),-1))?cargs[0]:cargs[1];
            else if (!strcmp(cn,"math.maks")||!strcmp(cn,"std.math.maks")) fn_r=nc_truthy(nc_cmp(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil(),1))?cargs[0]:cargs[1];
            else if (!strcmp(cn,"assert")) {
                if (narg>0 && !nc_truthy(cargs[0])) {
                    char *msg = narg>1 ? nc_to_str_raw(cargs[1]) : strdup("assert feilet");
                    nc_throw(msg); free(msg);
                }
            }
            else if (!strcmp(cn,"assert_eq")) {
                if (narg>=2 && !nc_eq(cargs[0],cargs[1])) {
                    char *a=nc_to_str_raw(cargs[0]),*b=nc_to_str_raw(cargs[1]);
                    char msg[512]; snprintf(msg,sizeof(msg),"assert_eq feilet: %s != %s",a,b);
                    free(a); free(b); nc_throw(msg);
                }
            }
            else if (!strcmp(cn,"assert_ne")) {
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
            /* sh.* / selfhost.common.* / selfhost.compiler.* (lazy common.no) */
            else if (nc_is_sh_api(cn)) {
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
                        else fn_r = nc_call_sh_api(cn, cargs, narg);
                        memcpy(&g_err_jmp, &_sh_saved_jmp, sizeof(jmp_buf));
                    }
                    if (_sh_caught) { free(cargs); free(callee); continue; }
                } else {
                    fn_r = nc_call_sh_api(cn, cargs, narg);
                }
            }
            /* Globale assert-helpers for testar */
            else if (!strcmp(cn,"assert_starter_med")) fn_r=nc_stub_assert_starter_med(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"assert_slutter_med")) fn_r=nc_stub_assert_slutter_med(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"assert_inneholder")) {
                NcVal *s2=narg>0?cargs[0]:nc_nil(), *sub=narg>1?cargs[1]:nc_nil();
                if (!nc_truthy(nc_builtin_contains(s2,sub))) {
                    char *sv=nc_to_str_raw(s2), *pv=nc_to_str_raw(sub);
                    char msg[512]; snprintf(msg,sizeof(msg),"assert_inneholder: '%s' inneheld ikkje '%s'",sv,pv);
                    free(sv); free(pv); nc_throw(msg);
                }
                fn_r = nc_nil();
            }
            else if (!strcmp(cn,"path.join"))          fn_r=nc_stub_path_join(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"web_escape_html")||!strcmp(cn,"html.escape")) fn_r=nc_stub_web_escape_html(narg>0?cargs[0]:nc_nil());
            /* env.*, json.*, t.* aliases */
            else if (!strcmp(cn,"env.finnes")||!strcmp(cn,"env_finnes")) fn_r=nc_builtin_miljo_finnes(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"env.hent")||!strcmp(cn,"env_hent"))     fn_r=nc_builtin_miljo_hent(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json.parse")||!strcmp(cn,"json_parse")) fn_r=nc_builtin_json_parse_norscode(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json.parse_raw")||!strcmp(cn,"json_parse_raw")) fn_r=nc_builtin_json_parse_raw(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json.stringify")||!strcmp(cn,"json_stringify")) fn_r=nc_builtin_json_stringify_smart(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json_skriv")) fn_r=nc_builtin_json_stringify(narg>0?cargs[0]:nc_nil());
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
                NcVal *local_fn = nc_exec_find_fn(functions, callee);
                int dispatch_first = nc_exec_prefer_dispatch(callee) && !nc_exec_prefer_local(callee);
                NcVal *dispatch_r = NULL;

                /* Dispatch-only kall (ingen lokal fn, dispatch_first) kan skippa try-wrapping */
                if (dispatch_first) {
                    dispatch_r = nc_dispatch_call(callee, cargs, narg);
                    if (dispatch_r != NULL) {
                        free(cargs); free(callee);
                        nc_push(&sp, stack_arr, dispatch_r); ip++;
                        continue;
                    }
                }
                if (!dispatch_first) {
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
    NcVal *ctx = na > 1 ? args[1] : nc_nil();
    NcVal *call_args[] = { ctx };
    if (!g_current_functions) return nc_nil();
    return nc_exec_call(g_current_functions, args[0]->s, call_args, 1, 0);
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
    nc_ensure_sh_common();
    if (g_sh_common_fns) nc_merge_fns(fns_v, g_sh_common_fns);
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
