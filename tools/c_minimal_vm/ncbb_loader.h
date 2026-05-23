/* NCBB v1 frozen bundle loader (points into blob; no JSON/zlib). */
#ifndef NCBB_LOADER_H
#define NCBB_LOADER_H

#include <stddef.h>
#include <stdint.h>

#define NCBB_MAGIC0 'N'
#define NCBB_MAGIC1 'C'
#define NCBB_MAGIC2 'B'
#define NCBB_MAGIC3 'B'
#define NCBB_VERSION 1
#define NCBB_HEADER_SIZE 36

typedef struct {
    const char *name;
    uint16_t n_params;
    uint16_t n_slots;
    const uint8_t *code;
    uint32_t code_len;
} ncbb_function_t;

typedef struct {
    uint8_t *owned_data;
    const uint8_t *data;
    size_t size;
    const char **strings;
    uint32_t n_strings;
    const char **builtins;
    uint32_t n_builtins;
    ncbb_function_t *functions;
    uint32_t n_functions;
} ncbb_bundle_t;

/* Load bundle from memory; caller must ncbb_free when done. */
int ncbb_load(const uint8_t *data, size_t size, ncbb_bundle_t *out);

/* Read file into owned buffer then load. */
int ncbb_load_file(const char *path, ncbb_bundle_t *out);

void ncbb_free(ncbb_bundle_t *bundle);

/* Linear search by function name (sorted table, not hashed). */
const ncbb_function_t *ncbb_find_function(const ncbb_bundle_t *bundle, const char *name);

int ncbb_find_builtin(const ncbb_bundle_t *bundle, const char *name);

#endif
