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

/* Forhandsdeklarasjonar for genererte funksjonar */
static NcVal *nc_fn_selfhost_kompiler_kompiler_fil(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_kompiler_kompiler_fil_til_disk(NcVal **args, int nargs);

/* ── NC executor: kjøyr eit NCB via C (ikkje selfhost/vm.no) ── */

typedef struct NcExecCtx {
    NcVal *functions; /* NC_MAP: fn_namn → fn_def */
} NcExecCtx;

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
static NcVal *nc_exec_call(NcVal *functions, const char *fn_name, NcVal **args, int nargs, int depth);

/* Kjøyr ein funksjon */
static NcVal *nc_exec_call(NcVal *functions, const char *fn_name, NcVal **args, int nargs, int depth) {
    if (depth > 500) { nc_throw("For djup rekursjon"); return nc_nil(); }

    NcVal *fn_def = nc_exec_find_fn(functions, fn_name);
    if (!fn_def) {
        /* Prøv innebygd */
        nc_panic("Ukjent funksjon: %s", fn_name);
        return nc_nil();
    }

    NcVal *params_v = nc_index_get(fn_def, nc_str("params"));
    NcVal *code_v   = nc_index_get(fn_def, nc_str("code"));

    NcVal **stack_arr = calloc(512, sizeof(NcVal*)); int sp = 0;
    NcVal **vars_arr  = calloc(128, sizeof(NcVal*));
    char **varnames   = calloc(128, sizeof(char*)); int nvars = 0;

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
                nc_push(&sp, stack_arr, nc_load(vars_arr, varnames, nvars, n)); free(n);
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
            /* Push lambda-funksjonsnamnet som ein streng-verdi */
            if (instr->list->len >= 2) {
                nc_push(&sp, stack_arr, instr->list->items[1]);
            } else {
                nc_push(&sp, stack_arr, nc_nil());
            }
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
            /* Variabel-kall: sjekk om cn er ein variabel som held eit fn-namn */
            NcVal *var_fn = nc_nil();
            for (int _vi=0; _vi<nvars; _vi++) {
                if (!strcmp(varnames[_vi], cn)) { var_fn = vars_arr[_vi]; break; }
            }
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
            else if (!strcmp(cn,"fjern_siste"))      fn_r = nc_builtin_fjern_siste(narg>0?cargs[0]:nc_nil());
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
            else if (!strcmp(cn,"fil_finnes"))       fn_r=nc_builtin_fil_finnes(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"miljo_hent"))       fn_r=nc_builtin_miljo_hent(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"miljo_finnes"))     fn_r=nc_builtin_miljo_finnes(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"bool"))             fn_r=nc_builtin_bool(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"feil"))             fn_r=nc_builtin_feil(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"type")||!strcmp(cn,"type_av")) fn_r=nc_builtin_type(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"index_of"))         fn_r=nc_builtin_index_of(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"upper"))            fn_r=nc_builtin_upper(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"lower"))            fn_r=nc_builtin_lower(narg>0?cargs[0]:nc_nil());
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
            /* env.*, json.*, t.* aliases */
            else if (!strcmp(cn,"env.finnes")||!strcmp(cn,"env_finnes")) fn_r=nc_builtin_miljo_finnes(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"env.hent")||!strcmp(cn,"env_hent"))     fn_r=nc_builtin_miljo_hent(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json.parse")||!strcmp(cn,"json_parse")) fn_r=nc_builtin_json_parse_norscode(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json.stringify")||!strcmp(cn,"json_stringify")) fn_r=nc_builtin_json_stringify_smart(narg>0?cargs[0]:nc_nil());
            else if (cn[0]>='A'&&cn[0]<='Z')        fn_r=nc_map_new(); /* struct constructor */
            else fn_r = nc_exec_call(functions, callee, cargs, narg, depth+1);
            free(cargs); free(callee);
            nc_push(&sp,stack_arr,fn_r); ip++;
        } else if (!strcmp(op, "THROW")) {
            NcVal *e = nc_pop(&sp,stack_arr); char *s=nc_to_str_raw(e);
            nc_throw(s); free(s); ip++;
        } else if (!strcmp(op, "TRY_BEGIN")) {
            /* Minimal TRY: lagre catch-label */
            if (instr->list->len >= 2) {
                NcVal *catch_lbl = instr->list->items[1];
                nc_push(&sp, stack_arr, catch_lbl); /* marker på stack */
            }
            ip++;
        } else if (!strcmp(op, "TRY_END")) {
            /* Fjern TRY-marker frå stack */
            ip++;
        } else if (!strcmp(op, "LOAD_EXCEPTION")) {
            /* Last unntak-meldinga */
            nc_push(&sp, stack_arr, nc_str(g_err_msg[0] ? g_err_msg : "ukjent feil"));
            ip++;
        } else {
            ip++;
        }
    }
done:
    free(stack_arr); free(vars_arr); free(varnames);
    return retval;
}

/* ── Wrap kompiler_fil som NcExecCtx-kall ── */
static NcVal *nc_native_kompiler(const char *src_path, const char *modul) {
    NcVal *src = nc_builtin_fil_les(nc_str(src_path));
    NcVal *mod = nc_str(modul);
    NcVal *args[] = {src, mod};
    return nc_fn_selfhost_kompiler_kompiler_fil(args, 2);
}

/* ── Overskriv main() ── */
int main(int argc, char **argv) {
    if (setjmp(g_err_jmp)) {
        fprintf(stderr, "norscode: %s\n", g_err_msg);
        return 1;
    }

    const char *cmd     = getenv("NORSCODE_CMD");
    const char *src     = getenv("NORSCODE_FILE");
    const char *out     = getenv("NORSCODE_OUTPUT");
    const char *modul   = getenv("NORSCODE_MODULE");

    if (!cmd)   cmd   = "selftest";
    if (!modul) modul = "__main__";

    /* ── compile ── */
    if (!strcmp(cmd, "compile")) {
        if (!src) { fprintf(stderr, "NORSCODE_FILE ikkje sett\n"); return 1; }
        printf("Kompilerer %s...\n", src);
        NcVal *ncb_json = nc_native_kompiler(src, modul);
        if (!ncb_json || ncb_json->type != NC_STR) {
            fprintf(stderr, "Kompilering feila\n"); return 1;
        }
        const char *outpath = out ? out : "/dev/stdout";
        nc_builtin_fil_skriv(nc_str(outpath), ncb_json);
        if (out) printf("NCB skrive til %s (%d bytes)\n", out, (int)strlen(ncb_json->s));
        return 0;
    }

    /* ── run ── */
    if (!strcmp(cmd, "run")) {
        if (!src) { fprintf(stderr, "NORSCODE_FILE ikkje sett\n"); return 1; }
        NcVal *ncb_json = nc_native_kompiler(src, modul);
        if (!ncb_json || ncb_json->type != NC_STR) {
            fprintf(stderr, "Kompilering feila\n"); return 1;
        }
        /* Parse NCB med C JSON parser */
        NcVal *ncb = nc_builtin_json_parse_str(ncb_json);
        /* Køyr entry via C executor */
        NcVal *fns_v = nc_index_get(ncb, nc_str("functions"));
        NcVal *entry_v = nc_index_get(ncb, nc_str("entry"));
        char *entry = nc_to_str_raw(entry_v);
        if (!fns_v || !strcmp(entry, "")) {
            fprintf(stderr, "Ingen entry-funksjon funnen\n"); free(entry); return 1;
        }
        nc_exec_call(fns_v, entry, NULL, 0, 0);
        free(entry);
        return 0;
    }

    /* ── selftest ── */
    if (!strcmp(cmd, "selftest")) {
        printf("=== norscode_native selftest ===\n");
        NcVal *ncb_json = nc_native_kompiler("/tmp/hei_test.no", "__main__");
        /* Skriv ein test-fil */
        nc_builtin_fil_skriv(nc_str("/tmp/hei_test.no"),
            nc_str("funksjon start() { skriv(\"selftest OK\\n\") }\n"));
        ncb_json = nc_native_kompiler("/tmp/hei_test.no", "__main__");
        if (ncb_json && ncb_json->type == NC_STR && strlen(ncb_json->s) > 10) {
            printf("Kompilering: OK (%d bytes NCB)\n", (int)strlen(ncb_json->s));
        } else {
            fprintf(stderr, "Kompilering feila\n"); return 1;
        }
        return 0;
    }

    fprintf(stderr, "Ukjent kommando: %s\n", cmd);
    return 1;
}
