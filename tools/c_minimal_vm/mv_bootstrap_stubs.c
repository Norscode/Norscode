/* C stubs for bootstrap bundle external symbols (subset). */
#include "mv_bootstrap_stubs.h"

#include "mv_internal.h"
#include "mv_bootstrap_host_exec.h"
#include "mv_env.h"
#include "mv_io.h"
#include "ncbb_loader.h"

#ifdef NORCODE_EMBED_NATIVE_NCBB
#include "ncbb_dual_embedded.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if MV_USE_ARENA
#include "mv_arena.h"
#define MV_ALLOC(n) mv_arena_alloc(n)
#define MV_CALLOC(n, s) mv_arena_calloc(n, s)
#else
#define MV_ALLOC(n) malloc(n)
#define MV_CALLOC(n, s) calloc(n, s)
#endif

static int streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static const char *mv_str_arg(const mv_value_t *v) {
    if (v && v->tag == MV_VAL_STR && v->u.s) return v->u.s;
    return "";
}

static int mv_map_set_str(mv_value_t *mapv, const char *key, const char *val) {
    if (!mapv || mapv->tag != MV_VAL_MAP || !mapv->u.map) return -1;
    mv_map_t *m = mapv->u.map;
    for (size_t i = 0; i < m->len; i++) {
        if (m->entries[i].key.tag == MV_VAL_STR && m->entries[i].key.u.s &&
            strcmp(m->entries[i].key.u.s, key) == 0) {
            m->entries[i].val.tag = MV_VAL_STR;
            m->entries[i].val.u.s = val;
            return 0;
        }
    }
    if (m->len >= m->cap) return -1;
    m->entries[m->len].key.tag = MV_VAL_STR;
    m->entries[m->len].key.u.s = key;
    m->entries[m->len].val.tag = MV_VAL_STR;
    m->entries[m->len].val.u.s = val;
    m->len++;
    return 0;
}

static int mv_result_map(mv_runtime_t *rt, mv_value_t *out, int success, const char *msg, const char *exe) {
    mv_map_t *m = (mv_map_t *)mv_track(rt, MV_CALLOC(1, sizeof(mv_map_t)));
    if (!m) return MV_ERR_BUILTIN;
    m->cap = 8;
    m->entries = (mv_map_entry_t *)mv_track(rt, MV_CALLOC(m->cap, sizeof(mv_map_entry_t)));
    if (!m->entries) return MV_ERR_BUILTIN;
    m->len = 0;
    out->tag = MV_VAL_MAP;
    out->u.map = m;
    mv_map_set_str(out, "success", success ? "sann" : "usann");
    mv_map_set_str(out, "output", msg ? msg : "");
    mv_map_set_str(out, "executable", exe ? exe : "");
    return 0;
}

static int stub_init_runtime(mv_runtime_t *rt, mv_value_t *out) {
    (void)rt;
    *out = MV_NIL();
    return 0;
}

static int stub_init_stdlib(mv_runtime_t *rt, mv_value_t *out) {
    (void)rt;
    *out = MV_NIL();
    return 0;
}

static int stub_ny_command(
    mv_runtime_t *rt,
    const mv_value_t *args,
    uint8_t argc,
    mv_value_t *out) {
    mv_map_t *m = (mv_map_t *)mv_track(rt, MV_CALLOC(1, sizeof(mv_map_t)));
    if (!m) return MV_ERR_BUILTIN;
    m->cap = 8;
    m->entries = (mv_map_entry_t *)mv_track(rt, MV_CALLOC(m->cap, sizeof(mv_map_entry_t)));
    if (!m->entries) return MV_ERR_BUILTIN;
    m->len = 0;
    out->tag = MV_VAL_MAP;
    out->u.map = m;
    const char *navn = argc > 0 ? mv_str_arg(&args[0]) : "";
    const char *source = argc > 1 ? mv_str_arg(&args[1]) : "";
    const char *output = argc > 2 ? mv_str_arg(&args[2]) : "";
    const char *arch = argc > 3 ? mv_str_arg(&args[3]) : "aarch64";
    mv_map_set_str(out, "navn", navn);
    mv_map_set_str(out, "source", source);
    mv_map_set_str(out, "output", output);
    mv_map_set_str(out, "target_arch", arch);
    return 0;
}

static int stub_ny_compiler(mv_runtime_t *rt, mv_value_t *out) {
    mv_map_t *m = (mv_map_t *)mv_track(rt, MV_CALLOC(1, sizeof(mv_map_t)));
    if (!m) return MV_ERR_BUILTIN;
    m->cap = 4;
    m->entries = (mv_map_entry_t *)mv_track(rt, MV_CALLOC(m->cap, sizeof(mv_map_entry_t)));
    if (!m->entries) return MV_ERR_BUILTIN;
    m->len = 0;
    out->tag = MV_VAL_MAP;
    out->u.map = m;
    mv_map_set_str(out, "target_arch", "aarch64");
    return 0;
}

static int stub_compile_source_for_target(
    mv_runtime_t *rt,
    const mv_value_t *args,
    uint8_t argc,
    mv_value_t *out) {
    if (argc < 4) return MV_ERR_BUILTIN;
    const char *source = mv_str_arg(&args[1]);
    const char *output = mv_str_arg(&args[2]);
    const char *arch = mv_str_arg(&args[3]);
    ncbb_bundle_t native;
#ifdef NORCODE_EMBED_NATIVE_NCBB
    if (norcode_ncbb_data_size < 4 || memcmp(norcode_ncbb_data, "NCBB", 4) != 0) {
        return MV_ERR_BUILTIN;
    }
    if (ncbb_load(norcode_ncbb_data, norcode_ncbb_data_size, &native) != 0) {
        return MV_ERR_BUILTIN;
    }
#else
    char ncbb_path_buf[512];
    const char *ncbb_path = "build/native_elf_compiler_bundle.ncb.bin";
    if (mv_env_get("NORCODE_NATIVE_NCBB", ncbb_path_buf, sizeof(ncbb_path_buf)) == 0) {
        ncbb_path = ncbb_path_buf;
    }
    if (ncbb_load_file(ncbb_path, &native) != 0) return MV_ERR_BUILTIN;
#endif

    mv_runtime_t nrt;
    if (mv_runtime_init(&nrt, &native) != 0) {
        ncbb_free(&native);
        return MV_ERR_BUILTIN;
    }

    mv_value_t call_args[2];
    call_args[0].tag = MV_VAL_STR;
    call_args[0].u.s = source;
    call_args[1].tag = MV_VAL_STR;
    call_args[1].u.s = arch;
    mv_value_t elf_list = MV_NIL();
    int rc = mv_call(&nrt, "__main__.kompiler_native_elf_bytes", call_args, 2, &elf_list);
    if (rc != 0) {
        mv_runtime_free(&nrt);
        ncbb_free(&native);
        return rc;
    }

    if (elf_list.tag != MV_VAL_LIST || !elf_list.u.list) {
        mv_runtime_free(&nrt);
        ncbb_free(&native);
        return MV_ERR_TYPE;
    }

    FILE *f = fopen(output, "wb");
    if (!f) {
        mv_runtime_free(&nrt);
        ncbb_free(&native);
        return MV_ERR_BUILTIN;
    }
    for (size_t i = 0; i < elf_list.u.list->len; i++) {
        if (elf_list.u.list->items[i].tag != MV_VAL_INT) continue;
        unsigned char b = (unsigned char)(elf_list.u.list->items[i].u.i & 0xFF);
        if (fputc((int)b, f) == EOF) {
            fclose(f);
            mv_runtime_free(&nrt);
            ncbb_free(&native);
            return MV_ERR_BUILTIN;
        }
    }
    fclose(f);
    mv_runtime_free(&nrt);
    ncbb_free(&native);
    return mv_result_map(rt, out, 1, "compile OK", output);
}

static int stub_les_fil(mv_runtime_t *rt, const mv_value_t *args, uint8_t argc, mv_value_t *out) {
    if (argc < 1) return MV_ERR_BUILTIN;
    const char *path = mv_str_arg(&args[0]);
    FILE *f = fopen(path, "rb");
    if (!f) {
        mv_map_t *m = (mv_map_t *)mv_track(rt, MV_CALLOC(1, sizeof(mv_map_t)));
        if (!m) return MV_ERR_BUILTIN;
        m->cap = 4;
        m->entries = (mv_map_entry_t *)mv_track(rt, MV_CALLOC(m->cap, sizeof(mv_map_entry_t)));
        if (!m->entries) return MV_ERR_BUILTIN;
        m->len = 0;
        out->tag = MV_VAL_MAP;
        out->u.map = m;
        mv_map_set_str(out, "success", "usann");
        mv_map_set_str(out, "data", "");
        return 0;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return MV_ERR_BUILTIN;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return MV_ERR_BUILTIN;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return MV_ERR_BUILTIN;
    }
    char *buf = (char *)mv_track(rt, MV_ALLOC((size_t)sz + 1));
    if (!buf) {
        fclose(f);
        return MV_ERR_BUILTIN;
    }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f);
        return MV_ERR_BUILTIN;
    }
    buf[sz] = 0;
    fclose(f);
    mv_map_t *m = (mv_map_t *)mv_track(rt, MV_CALLOC(1, sizeof(mv_map_t)));
    if (!m) return MV_ERR_BUILTIN;
    m->cap = 4;
    m->entries = (mv_map_entry_t *)mv_track(rt, MV_CALLOC(m->cap, sizeof(mv_map_entry_t)));
    if (!m->entries) return MV_ERR_BUILTIN;
    m->len = 0;
    out->tag = MV_VAL_MAP;
    out->u.map = m;
    mv_map_set_str(out, "success", "sann");
    mv_map_set_str(out, "data", buf);
    return 0;
}

static const char *proc_map_str(const mv_value_t *proc, const char *key) {
    if (!proc || proc->tag != MV_VAL_MAP || !proc->u.map) return "";
    mv_map_t *m = proc->u.map;
    for (size_t i = 0; i < m->len; i++) {
        if (m->entries[i].key.tag == MV_VAL_STR && m->entries[i].key.u.s &&
            strcmp(m->entries[i].key.u.s, key) == 0) {
            if (m->entries[i].val.tag == MV_VAL_STR && m->entries[i].val.u.s) {
                return m->entries[i].val.u.s;
            }
            return "";
        }
    }
    return "";
}

static int stub_load_executable(
    mv_runtime_t *rt,
    const mv_value_t *args,
    uint8_t argc,
    mv_value_t *out) {
    (void)rt;
    const char *path = argc > 1 ? mv_str_arg(&args[1]) : (argc > 0 ? mv_str_arg(&args[0]) : "");
    mv_map_t *m = (mv_map_t *)mv_track(rt, MV_CALLOC(1, sizeof(mv_map_t)));
    if (!m) return MV_ERR_BUILTIN;
    m->cap = 8;
    m->entries = (mv_map_entry_t *)mv_track(rt, MV_CALLOC(m->cap, sizeof(mv_map_entry_t)));
    if (!m->entries) return MV_ERR_BUILTIN;
    m->len = 0;
    out->tag = MV_VAL_MAP;
    out->u.map = m;
    mv_map_set_str(out, "pid", "1");
    mv_map_set_str(out, "path", path);
    mv_map_set_str(out, "executable", path);
    mv_map_set_str(out, "running", "usann");
    return 0;
}

static int stub_execute_process(mv_runtime_t *rt, const mv_value_t *args, uint8_t argc, mv_value_t *out) {
    (void)rt;
    if (argc < 1) return MV_ERR_BUILTIN;
    const char *path = proc_map_str(&args[0], "executable");
    if (!path[0]) path = proc_map_str(&args[0], "path");
    if (!path[0]) return MV_ERR_BUILTIN;

    int exit_code = 0;
    if (mv_bootstrap_host_exec_path(path, &exit_code) != 0) {
        mv_write_stderr("bootstrap: execute_process failed for ");
        mv_write_stderr(path);
        mv_write_stderr("\n");
        return MV_ERR_BUILTIN;
    }
    *out = MV_INT(exit_code);
    return 0;
}

static int stub_skriv_linje(mv_runtime_t *rt, const mv_value_t *args, uint8_t argc, mv_value_t *out) {
    (void)rt;
    *out = MV_NIL();
    if (argc >= 1) {
        const char *line = mv_str_arg(&args[0]);
        mv_write_stderr(line);
        if (line[0] && line[strlen(line) - 1] != '\n') mv_write_stderr("\n");
    }
    return 0;
}

static int stub_noop_true(mv_runtime_t *rt, mv_value_t *out) {
    (void)rt;
    *out = MV_BOOL(1);
    return 0;
}

int mv_bootstrap_stub_call(
    mv_runtime_t *rt,
    const char *name,
    const mv_value_t *args,
    uint8_t argc,
    mv_value_t *out) {
    (void)argc;
    if (streq(name, "__main__.init_runtime")) return stub_init_runtime(rt, out);
    if (streq(name, "__main__.init_stdlib")) return stub_init_stdlib(rt, out);
    if (streq(name, "__main__.ny_compiler")) return stub_ny_compiler(rt, out);
    if (streq(name, "__main__.compiler_driver_ny_compiler_context")) return stub_ny_compiler(rt, out);
    if (streq(name, "__main__.compiler_driver_ny_command")) {
        return stub_ny_command(rt, args, argc, out);
    }
    if (streq(name, "__main__.ny_command")) return stub_ny_command(rt, args, argc, out);
    if (streq(name, "__main__.compile_source_for_target") ||
        streq(name, "__main__.compile_source_aarch64_subset")) {
        return stub_compile_source_for_target(rt, args, argc, out);
    }
    if (streq(name, "__main__.les_fil")) return stub_les_fil(rt, args, argc, out);
    if (streq(name, "__main__.load_executable")) return stub_load_executable(rt, args, argc, out);
    if (streq(name, "__main__.execute_process")) return stub_execute_process(rt, args, argc, out);
    if (streq(name, "__main__.skriv_linje")) return stub_skriv_linje(rt, args, argc, out);
    if (streq(name, "__main__.runtime_panic")) {
        mv_write_stderr("bootstrap stub: runtime_panic\n");
        return MV_ERR_BUILTIN;
    }
    if (streq(name, "__main__.ny_package_registry") || streq(name, "__main__.ny_loader") ||
        streq(name, "__main__.runtime_leak_check") || streq(name, "__main__.verify_phase1_report") ||
        streq(name, "__main__.fullfor_fase1_native_backend")) {
        return stub_noop_true(rt, out);
    }
/* BEGIN GENERATED MANIFEST STUB DISPATCH */
#include "mv_bootstrap_stub_manifest_dispatch.inc"
/* END GENERATED MANIFEST STUB DISPATCH */
    if (streq(name, "__main__.ingen")) {
        *out = MV_NIL();
        return 0;
    }
    /* Default: no-op success for unimplemented externals during probe. */
    if (strncmp(name, "__main__.", 7) == 0) return stub_noop_true(rt, out);
    return MV_ERR_FN;
}
