/* Minimal bytecode VM — 26-opcode subset over NCBB (P3b C port). */
#ifndef MINIMAL_VM_H
#define MINIMAL_VM_H

#include "ncbb_loader.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    OP_LABEL = 0,
    OP_PUSH_CONST,
    OP_LOAD_NAME,
    OP_STORE_NAME,
    OP_POP,
    OP_BUILD_LIST,
    OP_BUILD_MAP,
    OP_INDEX_GET,
    OP_INDEX_SET,
    OP_UNARY_NEG,
    OP_BINARY_ADD,
    OP_BINARY_SUB,
    OP_BINARY_MUL,
    OP_BINARY_DIV,
    OP_BINARY_MOD,
    OP_COMPARE_EQ,
    OP_COMPARE_NE,
    OP_COMPARE_GT,
    OP_COMPARE_LT,
    OP_COMPARE_GE,
    OP_COMPARE_LE,
    OP_CALL,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_RETURN,
    OP_UNARY_NOT,
    OP_COUNT
} mv_opcode_t;

#define MV_STACK_MAX 1024
#define MV_LOCALS_MAX 256
#define MV_CALL_DEPTH_MAX 128
#define MV_MAX_STEPS_DEFAULT 50000000

typedef enum {
    MV_VAL_NIL = 0,
    MV_VAL_INT,
    MV_VAL_BOOL,
    MV_VAL_STR,
    MV_VAL_LIST,
    MV_VAL_MAP,
} mv_tag_t;

typedef struct mv_list mv_list_t;
typedef struct mv_map mv_map_t;

typedef struct mv_value {
    mv_tag_t tag;
    union {
        int64_t i;
        int b;
        const char *s;
        mv_list_t *list;
        mv_map_t *map;
    } u;
} mv_value_t;

struct mv_list {
    mv_value_t *items;
    size_t len;
    size_t cap;
};

typedef struct mv_map_entry {
    mv_value_t key;
    mv_value_t val;
} mv_map_entry_t;

struct mv_map {
    mv_map_entry_t *entries;
    size_t len;
    size_t cap;
};

typedef struct mv_runtime {
    const ncbb_bundle_t *bundle;
    int64_t max_steps;
    int64_t steps;
    int depth;
    int last_error;
} mv_runtime_t;

#define MV_INT(v) ((mv_value_t){MV_VAL_INT, .u.i = (int64_t)(v)})
#define MV_BOOL(v) ((mv_value_t){MV_VAL_BOOL, .u.b = (v) ? 1 : 0})
#define MV_STR(s) ((mv_value_t){MV_VAL_STR, .u.s = (const char *)(s)})
#define MV_NIL() ((mv_value_t){MV_VAL_NIL, .u.i = 0})

int mv_opcode_from_name(const char *name);

void mv_value_free(mv_runtime_t *rt, mv_value_t *v);
int mv_value_dup(mv_runtime_t *rt, const mv_value_t *src, mv_value_t *dst);
int mv_value_truthy(const mv_value_t *v);

int mv_runtime_init(mv_runtime_t *rt, const ncbb_bundle_t *bundle);
void mv_runtime_free(mv_runtime_t *rt);

int mv_call(mv_runtime_t *rt, const char *name, const mv_value_t *args, size_t argc, mv_value_t *out);

/* Legacy integer smoke program (PUSH/ADD/RETURN). */
int mv_run_int_program(const uint8_t *program, size_t len, int64_t *out_result);

#endif
