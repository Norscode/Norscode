/* NCBB v1 loader — mirrors norcode/ncb_binary.py layout. */
#include "ncbb_loader.h"
#include "mv_arena.h"

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

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int read_string_pool(const uint8_t *blob, size_t blen, uint32_t count, const char ***out) {
    if (count == 0) {
        *out = NULL;
        return 0;
    }
    const char **pool = (const char **)MV_CALLOC(count, sizeof(char *));
    if (!pool) return -1;

    size_t pos = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (pos >= blen) {
            free(pool);
            return -2;
        }
        pool[i] = (const char *)(blob + pos);
        while (pos < blen && blob[pos] != 0) pos++;
        if (pos >= blen) {
            free(pool);
            return -3;
        }
        pos++;
    }
    *out = pool;
    return 0;
}

static void ncbb_clear(ncbb_bundle_t *out) {
#if !MV_USE_ARENA
    free(out->owned_data);
    free((void *)out->strings);
    free((void *)out->builtins);
    free(out->functions);
#endif
    memset(out, 0, sizeof(*out));
}

int ncbb_load(const uint8_t *data, size_t size, ncbb_bundle_t *out) {
    if (!data || !out) return -1;
    memset(out, 0, sizeof(*out));

    if (size < NCBB_HEADER_SIZE) return -10;
    if (data[0] != NCBB_MAGIC0 || data[1] != NCBB_MAGIC1 || data[2] != NCBB_MAGIC2 ||
        data[3] != NCBB_MAGIC3) {
        return -11;
    }

    uint32_t version = rd32(data + 4);
    uint32_t n_fn = rd32(data + 8);
    uint32_t n_str = rd32(data + 12);
    uint32_t n_bi = rd32(data + 16);
    uint32_t off_fn = rd32(data + 20);
    uint32_t off_bi = rd32(data + 24);
    uint32_t off_str = rd32(data + 28);
    uint32_t off_code = rd32(data + 32);

    if (version != NCBB_VERSION) return -12;
    if (off_fn < NCBB_HEADER_SIZE || off_bi < off_fn || off_str < off_bi || off_code < off_str ||
        off_code > size) {
        return -13;
    }

    const uint8_t *fn_blob = data + off_fn;
    size_t fn_len = (size_t)(off_bi - off_fn);
    const uint8_t *bi_blob = data + off_bi;
    size_t bi_len = (size_t)(off_str - off_bi);
    const uint8_t *str_blob = data + off_str;
    size_t str_len = (size_t)(off_code - off_str);
    const uint8_t *code_blob = data + off_code;
    size_t code_len = size - off_code;

    const char **strings = NULL;
    const char **builtins = NULL;
    ncbb_function_t *functions = NULL;

    if (read_string_pool(str_blob, str_len, n_str, &strings) != 0) return -20;
    if (read_string_pool(bi_blob, bi_len, n_bi, &builtins) != 0) {
        free((void *)strings);
        return -21;
    }

    functions = (ncbb_function_t *)MV_CALLOC(n_fn, sizeof(ncbb_function_t));
    if (!functions) {
        free((void *)strings);
        free((void *)builtins);
        return -22;
    }

    size_t cursor = 0;
    for (uint32_t i = 0; i < n_fn; i++) {
        if (cursor + 16 > fn_len) {
            free(functions);
            free((void *)strings);
            free((void *)builtins);
            return -30;
        }
        uint32_t name_id = rd32(fn_blob + cursor);
        uint16_t n_params = rd16(fn_blob + cursor + 4);
        uint16_t n_slots = rd16(fn_blob + cursor + 6);
        uint32_t code_off = rd32(fn_blob + cursor + 8);
        uint32_t code_len_fn = rd32(fn_blob + cursor + 12);
        cursor += 16;

        if (name_id >= n_str) {
            free(functions);
            free((void *)strings);
            free((void *)builtins);
            return -31;
        }
        if (cursor + (size_t)n_slots * 4 > fn_len) {
            free(functions);
            free((void *)strings);
            free((void *)builtins);
            return -32;
        }
        cursor += (size_t)n_slots * 4;

        if (code_off + code_len_fn > code_len) {
            free(functions);
            free((void *)strings);
            free((void *)builtins);
            return -33;
        }

        functions[i].name = strings[name_id];
        functions[i].n_params = n_params;
        functions[i].n_slots = n_slots;
        functions[i].code = code_blob + code_off;
        functions[i].code_len = code_len_fn;
    }

    out->data = data;
    out->size = size;
    out->strings = strings;
    out->n_strings = n_str;
    out->builtins = builtins;
    out->n_builtins = n_bi;
    out->functions = functions;
    out->n_functions = n_fn;
    return 0;
}

int ncbb_load_file(const char *path, ncbb_bundle_t *out) {
    if (!path || !out) return -1;
    memset(out, 0, sizeof(*out));

    FILE *f = fopen(path, "rb");
    if (!f) return -40;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -41;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return -42;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -43;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) {
        fclose(f);
        return -44;
    }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf);
        fclose(f);
        return -45;
    }
    fclose(f);

    int rc = ncbb_load(buf, (size_t)sz, out);
    if (rc != 0) {
        free(buf);
        return rc;
    }
    out->owned_data = buf;
    out->data = buf;
    return 0;
}

void ncbb_free(ncbb_bundle_t *bundle) {
    if (!bundle) return;
    ncbb_clear(bundle);
}

const ncbb_function_t *ncbb_find_function(const ncbb_bundle_t *bundle, const char *name) {
    if (!bundle || !name || !bundle->functions) return NULL;
    for (uint32_t i = 0; i < bundle->n_functions; i++) {
        if (bundle->functions[i].name && strcmp(bundle->functions[i].name, name) == 0) {
            return &bundle->functions[i];
        }
    }
    return NULL;
}

int ncbb_find_builtin(const ncbb_bundle_t *bundle, const char *name) {
    if (!bundle || !name || !bundle->builtins) return -1;
    for (uint32_t i = 0; i < bundle->n_builtins; i++) {
        if (bundle->builtins[i] && strcmp(bundle->builtins[i], name) == 0) return (int)i;
    }
    return -1;
}
