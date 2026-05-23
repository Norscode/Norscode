/* Freestanding P3b compiler: embedded NCBB + VM, no libc (Linux). */
/* Markers for norcode.native_elf_service (must stay in rodata). */
const char norcode_p3b_freestanding_marker[] = "NORCODE_P3B_FREESTANDING=1\n";

#include "minimal_vm.h"
#include "mv_arena.h"
#include "mv_env.h"
#include "mv_internal.h"
#include "mv_io.h"
#include "ncbb_loader.h"

#ifdef NORCODE_DUAL_NCBB
#include "ncbb_dual_embedded.h"
#else
#include "ncbb_embedded.h"
#endif

#ifdef NORCODE_BOOTSTRAP_STUBS
#include "mv_bootstrap_stubs.h"
#endif

#include <stdio.h>
#include <string.h>

#ifdef NORCODE_DUAL_NCBB
#define BOOTSTRAP_ARG_MAX 16

static int load_bootstrap_bundle(ncbb_bundle_t *bundle, mv_runtime_t *rt) {
    if (norcode_bootstrap_ncbb_data_size < 4 ||
        memcmp(norcode_bootstrap_ncbb_data, "NCBB", 4) != 0) {
        mv_write_stderr("norcode: bad embedded bootstrap NCBB\n");
        return 10;
    }
    if (ncbb_load(norcode_bootstrap_ncbb_data, norcode_bootstrap_ncbb_data_size, bundle) != 0) {
        mv_write_stderr("norcode: bootstrap ncbb_load failed\n");
        return 13;
    }
    if (mv_runtime_init(rt, bundle) != 0) {
        ncbb_free(bundle);
        return 14;
    }
    rt->max_steps = 80000000;
    return 0;
}

static int bootstrap_exit_from_value(const mv_value_t *out) {
    if (out->tag == MV_VAL_INT) return (int)out->u.i;
    if (out->tag == MV_VAL_BOOL) return out->u.b ? 0 : 1;
    return 0;
}

static int run_bootstrap_cli(void) {
    ncbb_bundle_t bundle;
    mv_runtime_t rt;
    int lr = load_bootstrap_bundle(&bundle, &rt);
    if (lr != 0) return lr;

    int argc = mv_env_argc();
    if (argc < 0) argc = 0;
    if (argc > BOOTSTRAP_ARG_MAX) argc = BOOTSTRAP_ARG_MAX;

    mv_list_t *argv = mv_list_new(&rt, (size_t)(argc > 0 ? argc : 1));
    if (!argv) {
        mv_runtime_free(&rt);
        ncbb_free(&bundle);
        return 15;
    }

    char arg_bufs[BOOTSTRAP_ARG_MAX][512];
    mv_value_t items[BOOTSTRAP_ARG_MAX];
    for (int i = 0; i < argc; i++) {
        if (mv_env_arg(i, arg_bufs[i], sizeof(arg_bufs[i])) != 0) {
            arg_bufs[i][0] = 0;
        }
        items[i].tag = MV_VAL_STR;
        items[i].u.s = arg_bufs[i];
        if (mv_list_append(argv, &items[i]) != 0) {
            mv_runtime_free(&rt);
            ncbb_free(&bundle);
            return 15;
        }
    }

    mv_value_t args[1];
    args[0].tag = MV_VAL_LIST;
    args[0].u.list = argv;

    mv_value_t out = MV_NIL();
    int rc = mv_call(&rt, "__main__.cli_main", args, 1, &out);
    int exit_code = rc != 0 ? (rc < 0 ? 16 : rc) : bootstrap_exit_from_value(&out);
    mv_runtime_free(&rt);
    ncbb_free(&bundle);
    if (rc != 0) mv_write_stderr("norcode: bootstrap cli_main failed\n");
    return exit_code;
}

static int run_bootstrap_compiler(void) {
    char cmd[32];
    char src[4096];
    char out[4096];
    char arch[32];
    if (mv_env_get("NORCODE_SRC", src, sizeof(src)) != 0) {
        mv_write_stderr("norcode: NORCODE_SRC missing\n");
        return 11;
    }
    if (mv_env_get("NORCODE_OUT", out, sizeof(out)) != 0) {
        mv_write_stderr("norcode: NORCODE_OUT missing\n");
        return 12;
    }
    if (mv_env_get("NORCODE_ARCH", arch, sizeof(arch)) != 0) {
        snprintf(arch, sizeof(arch), "%s", "aarch64");
    }
    if (mv_env_get("NORCODE_CMD", cmd, sizeof(cmd)) != 0) {
        snprintf(cmd, sizeof(cmd), "%s", "build");
    }

    ncbb_bundle_t bundle;
    mv_runtime_t rt;
    int lr = load_bootstrap_bundle(&bundle, &rt);
    if (lr != 0) return lr;

    mv_list_t *argv = mv_list_new(&rt, 4);
    if (!argv) {
        mv_runtime_free(&rt);
        ncbb_free(&bundle);
        return 15;
    }
    mv_value_t items[4];
    items[0].tag = MV_VAL_STR;
    items[0].u.s = cmd;
    items[1].tag = MV_VAL_STR;
    items[1].u.s = src;
    items[2].tag = MV_VAL_STR;
    items[2].u.s = out;
    items[3].tag = MV_VAL_STR;
    items[3].u.s = arch;
    for (int i = 0; i < 4; i++) {
        if (mv_list_append(argv, &items[i]) != 0) {
            mv_runtime_free(&rt);
            ncbb_free(&bundle);
            return 15;
        }
    }

    mv_value_t args[1];
    args[0].tag = MV_VAL_LIST;
    args[0].u.list = argv;

    mv_value_t result = MV_NIL();
    int rc = mv_call(&rt, "__main__.compiler_driver_compiler_main", args, 1, &result);
    mv_runtime_free(&rt);
    ncbb_free(&bundle);

    if (rc != 0) {
        mv_write_stderr("norcode: bootstrap driver failed\n");
        return rc < 0 ? 16 : rc;
    }
    return 0;
}
#endif

static int run_native_compiler(void) {
    if (norcode_ncbb_data_size < 4 || memcmp(norcode_ncbb_data, "NCBB", 4) != 0) {
        mv_write_stderr("norcode: bad embedded NCBB\n");
        return 1;
    }

    ncbb_bundle_t bundle;
    if (ncbb_load(norcode_ncbb_data, norcode_ncbb_data_size, &bundle) != 0) {
        mv_write_stderr("norcode: ncbb_load failed\n");
        return 2;
    }

    mv_runtime_t rt;
    mv_runtime_init(&rt, &bundle);
    rt.max_steps = 80000000;

    mv_value_t out = MV_NIL();
    int rc = mv_call(&rt, "__main__.start", NULL, 0, &out);
    mv_runtime_free(&rt);
    ncbb_free(&bundle);

    if (rc != 0) {
        mv_write_stderr("norcode: compile failed\n");
        return rc < 0 ? 3 : rc;
    }
    if (out.tag != MV_VAL_INT) {
        mv_write_stderr("norcode: bad return type\n");
        return 4;
    }
    return (int)out.u.i;
}

int norcode_compiler_main(void) {
    mv_arena_reset();

#ifdef NORCODE_DUAL_NCBB
    if (mv_env_flag("NORCODE_BOOTSTRAP_CLI")) {
        return run_bootstrap_cli();
    }
    if (mv_env_flag("NORCODE_BOOTSTRAP_VM")) {
        return run_bootstrap_compiler();
    }
#endif

    return run_native_compiler();
}
