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

#include <stdatomic.h>
#include "nc_metal_tensor.c"
#if defined(_WIN32)
#include "nc_windows_backend.h"
#else
#include <pthread.h>
#include <poll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#include <sys/mman.h>
#endif
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#if defined(__APPLE__)
#include <sandbox.h>
#include <libproc.h>
#include <sys/event.h>
#include <CommonCrypto/CommonKeyDerivation.h>
#endif
#if defined(__linux__)
#include <sys/prctl.h>
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#endif
#if defined(NC_ENABLE_OPENSSL)
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/ocsp.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/pem.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#endif
#if defined(NC_ENABLE_ZIG_ARGON2)
extern int norscode_argon2id(const unsigned char *password, size_t password_len,
                             const unsigned char *salt, size_t salt_len,
                             unsigned int memory_kib, unsigned int iterations,
                             unsigned int parallelism, unsigned char *out,
                             size_t out_len);
#endif

/* Forhandsdeklarasjonar — brukar nc_dispatch_call for dynamisk oppslag */
struct NcVal; typedef struct NcVal NcVal;
NcVal *nc_builtin_ncb_route_handlers(NcVal **args, int na);
NcVal *nc_builtin_ncb_metadata(NcVal **args, int na);
NcVal *nc_builtin_vm_function_info(NcVal **args, int na);
NcVal *nc_builtin_ncb_next_request_id(NcVal **args, int na);
NcVal *nc_builtin_ncb_call_fn(NcVal **args, int na);
NcVal *nc_fn_builtin_host_exec_ncb_json(NcVal **args, int na);
NcVal *nc_fn_builtin_host_kall_bygg_bundle(NcVal **args, int na);
static NcVal *nc_dispatch_call(const char *n, NcVal **a, int na);
NcVal *nc_fn_builtin_neste_token(NcVal **a, int na);
static NcVal *nc_builtin_thread_spawn(NcVal *request);
static NcVal *nc_builtin_thread_join(NcVal *request);
static NcVal *nc_builtin_thread_sync(NcVal *request);
static NcVal *nc_builtin_thread_pool(NcVal *request);
static NcVal *nc_builtin_pbkdf2_sha256(NcVal *password_v, NcVal *salt_v,
                                       NcVal *iterations_v, NcVal *length_v);
static NcVal *nc_builtin_argon2id(NcVal *password_v, NcVal *salt_v,
                                  NcVal *memory_kib_v, NcVal *iterations_v,
                                  NcVal *parallelism_v, NcVal *length_v);
static NcVal *nc_builtin_acme_sign(NcVal *algorithm_v, NcVal *private_key_v,
                                   NcVal *input_v);
static NcVal *nc_builtin_acme_verify(NcVal *algorithm_v, NcVal *public_key_v,
                                     NcVal *input_v, NcVal *signature_hex_v);
static NcVal *nc_builtin_dns_lookup(NcVal *host_v, NcVal *service_v);
static void nc_list_append_raw(NcVal *lst, NcVal *v);
static NcVal *g_current_route_handlers = NULL;
static NcVal *g_current_functions = NULL;
static NcVal *g_current_ncb = NULL;
static int g_request_counter = 0;

#define NC_ATOMIC_MAX 1024
typedef struct {
    _Atomic long long value;
    _Atomic unsigned long long version;
    _Atomic unsigned long long generation;
    _Atomic int active;
} NcAtomicCell;
static NcAtomicCell g_atomic_cells[NC_ATOMIC_MAX];

static NcVal *nc_atomic_result(const char *status) {
    NcVal *r = nc_map_new();
    nc_index_set(r, nc_str("abi"), nc_str("norscode-atomic-v1"));
    nc_index_set(r, nc_str("status"), nc_str(status));
    return r;
}

static memory_order nc_atomic_order(const char *name, int *ok) {
    *ok = 1;
    if (!strcmp(name, "relaxed")) return memory_order_relaxed;
    if (!strcmp(name, "acquire")) return memory_order_acquire;
    if (!strcmp(name, "release")) return memory_order_release;
    if (!strcmp(name, "acq_rel")) return memory_order_acq_rel;
    if (!strcmp(name, "seq_cst")) return memory_order_seq_cst;
    *ok = 0;
    return memory_order_seq_cst;
}

static int nc_atomic_failure_allowed(memory_order success, memory_order failure) {
    if (failure == memory_order_release || failure == memory_order_acq_rel) return 0;
    if (failure == memory_order_seq_cst) return success == memory_order_seq_cst;
    if (failure == memory_order_acquire)
        return success == memory_order_acquire || success == memory_order_acq_rel || success == memory_order_seq_cst;
    return 1;
}

static long long nc_atomic_int_field(NcVal *request, const char *key, long long fallback) {
    NcVal *v = nc_index_get(request, nc_str(key));
    if (v && v->type == NC_INT) return v->i;
    if (v && v->type == NC_STR) return strtoll(v->s, NULL, 10);
    return fallback;
}

static const char *nc_atomic_text_field(NcVal *request, const char *key, const char *fallback) {
    NcVal *v = nc_index_get(request, nc_str(key));
    return v && v->type == NC_STR ? v->s : fallback;
}

static NcVal *nc_process_result(const char *status, pid_t pid, int exit_code,
                                const char *out, const char *err, const char *error) {
    NcVal *r = nc_map_new();
    nc_index_set(r, nc_str("abi"), nc_str("norscode-process-spawn-v1"));
    nc_index_set(r, nc_str("status"), nc_str(status));
    nc_index_set(r, nc_str("pid"), nc_int((long long)pid));
    nc_index_set(r, nc_str("exit_code"), nc_int(exit_code));
    nc_index_set(r, nc_str("stdout"), nc_str(out ? out : ""));
    nc_index_set(r, nc_str("stderr"), nc_str(err ? err : ""));
    nc_index_set(r, nc_str("error"), nc_str(error ? error : ""));
    return r;
}

#define NC_JIT_MAX 128
#define NC_JIT_CODE_MAX 4096
typedef long long (*NcJitFn)(const long long *arguments);
typedef double (*NcJitFloatFn)(const double *arguments);
typedef uintptr_t (*NcJitTextFn)(NcVal *const *arguments);
typedef struct {
    int active;
    unsigned int generation;
    void *memory;
    size_t mapped_size;
    size_t code_size;
    int arity;
    int operation_count;
    int numeric_kind;
    int result_kind;
} NcJitSlot;
static NcJitSlot g_jit_slots[NC_JIT_MAX];
static pthread_mutex_t g_jit_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct { unsigned char data[NC_JIT_CODE_MAX]; size_t length; } NcJitBuffer;
typedef struct { char name[64]; size_t offset; int depth; } NcJitLabel;
typedef struct { char name[64]; size_t offset; int kind; int depth; } NcJitPatch;
static int nc_jit_u8(NcJitBuffer *buffer, unsigned int value) {
    if (buffer->length >= sizeof(buffer->data)) return 0;
    buffer->data[buffer->length++] = (unsigned char)value;
    return 1;
}
static int nc_jit_u32(NcJitBuffer *buffer, unsigned int value) {
    for (int i = 0; i < 4; i++) if (!nc_jit_u8(buffer, value >> (i * 8))) return 0;
    return 1;
}
static int nc_jit_u64(NcJitBuffer *buffer, unsigned long long value) {
    for (int i = 0; i < 8; i++) if (!nc_jit_u8(buffer, (unsigned int)(value >> (i * 8)))) return 0;
    return 1;
}
static int nc_jit_name(char destination[64], const char *source) {
    size_t length = source ? strlen(source) : 0;
    if (!length || length >= 64) return 0;
    for (size_t i = 0; i < length; i++)
        if (!(isalnum((unsigned char)source[i]) || source[i] == '_' || source[i] == '.')) return 0;
    memcpy(destination, source, length + 1);
    return 1;
}

static NcVal *nc_jit_result(const char *status, const char *error) {
    NcVal *result = nc_map_new();
    nc_index_set(result, nc_str("abi"), nc_str("norscode-jit-v1"));
    nc_index_set(result, nc_str("status"), nc_str(status));
    nc_index_set(result, nc_str("error"), nc_str(error ? error : ""));
#if defined(__aarch64__)
    nc_index_set(result, nc_str("backend"), nc_str("arm64"));
#elif defined(__x86_64__)
    nc_index_set(result, nc_str("backend"), nc_str("x86_64"));
#else
    nc_index_set(result, nc_str("backend"), nc_str("unsupported"));
#endif
    nc_index_set(result, nc_str("memory_policy"), nc_str("write-xor-execute"));
    return result;
}

#if defined(__aarch64__)
static int nc_jit_arm64_word(NcJitBuffer *buffer, unsigned int word) { return nc_jit_u32(buffer, word); }
static int nc_jit_arm64_const(NcJitBuffer *buffer, int reg, unsigned long long value) {
    if (!nc_jit_arm64_word(buffer, 0xD2800000u | ((unsigned int)(value & 0xffffu) << 5) | (unsigned int)reg)) return 0;
    for (int shift = 1; shift < 4; shift++) {
        unsigned int part = (unsigned int)((value >> (shift * 16)) & 0xffffu);
        if (part && !nc_jit_arm64_word(buffer, 0xF2800000u | ((unsigned int)shift << 21) | (part << 5) | (unsigned int)reg)) return 0;
    }
    return 1;
}
#endif

static long long nc_jit_text_eq_value(NcVal *left, NcVal *right) {
    return left && right && left->type == NC_STR && right->type == NC_STR &&
           !strcmp(left->s ? left->s : "", right->s ? right->s : "");
}

static long long nc_jit_text_ne_value(NcVal *left, NcVal *right) {
    return !nc_jit_text_eq_value(left, right);
}

static long long nc_jit_text_len_value(NcVal *value) {
    return value && value->type == NC_STR ? (long long)strlen(value->s ? value->s : "") : 0;
}

static uintptr_t nc_jit_text_concat_value(NcVal *left, NcVal *right) {
    if (!left || !right || left->type != NC_STR || right->type != NC_STR) return 0;
    const char *left_text = left->s ? left->s : "";
    const char *right_text = right->s ? right->s : "";
    size_t left_length = strlen(left_text), right_length = strlen(right_text);
    const size_t max_length = 16u * 1024u * 1024u;
    if (left_length > max_length || right_length > max_length - left_length) return 0;
    char *joined = malloc(left_length + right_length + 1);
    if (!joined) return 0;
    memcpy(joined, left_text, left_length);
    memcpy(joined + left_length, right_text, right_length + 1);
    return (uintptr_t)joined;
}

static int nc_jit_text_arg(const char *operation, int arity, int *index) {
    if (!operation || strncmp(operation, "arg:", 4)) return 0;
    char *end = NULL;
    long parsed = strtol(operation + 4, &end, 10);
    if (!end || *end || parsed < 0 || parsed >= arity) return 0;
    *index = (int)parsed;
    return 1;
}

static int nc_jit_compile_text(NcVal *operations, int arity, const char *result_kind,
                               NcJitBuffer *buffer, int *operation_count, const char **error) {
    if (!operations || operations->type != NC_LIST) {
        *error = "operations must be a list"; return 0;
    }
    if (arity < 1 || arity > 32) { *error = "JIT limits exceeded"; return 0; }
    int first = -1, second = -1, binary = 0;
    void *helper = NULL;
    if (operations->list->len == 4 &&
        operations->list->items[0] && operations->list->items[0]->type == NC_STR &&
        operations->list->items[1] && operations->list->items[1]->type == NC_STR &&
        operations->list->items[2] && operations->list->items[2]->type == NC_STR &&
        operations->list->items[3] && operations->list->items[3]->type == NC_STR &&
        nc_jit_text_arg(operations->list->items[0]->s, arity, &first) &&
        nc_jit_text_arg(operations->list->items[1]->s, arity, &second) &&
        !strcmp(operations->list->items[3]->s, "return") &&
        (!strcmp(operations->list->items[2]->s, "eq") || !strcmp(operations->list->items[2]->s, "ne") ||
         !strcmp(operations->list->items[2]->s, "concat"))) {
        binary = 1;
        if (!strcmp(operations->list->items[2]->s, "concat")) {
            if (strcmp(result_kind, "text")) { *error = "text concat must return text"; return 0; }
            helper = (void *)&nc_jit_text_concat_value;
        } else {
            if (strcmp(result_kind, "bool")) { *error = "text comparison must return bool"; return 0; }
            helper = !strcmp(operations->list->items[2]->s, "eq") ?
                (void *)&nc_jit_text_eq_value : (void *)&nc_jit_text_ne_value;
        }
    } else if (operations->list->len == 3 &&
               operations->list->items[0] && operations->list->items[0]->type == NC_STR &&
               operations->list->items[1] && operations->list->items[1]->type == NC_STR &&
               operations->list->items[2] && operations->list->items[2]->type == NC_STR &&
               nc_jit_text_arg(operations->list->items[0]->s, arity, &first) &&
               !strcmp(operations->list->items[1]->s, "text_len") &&
               !strcmp(operations->list->items[2]->s, "return")) {
        if (strcmp(result_kind, "int")) { *error = "text length must return int"; return 0; }
        helper = (void *)&nc_jit_text_len_value;
    } else {
        *error = "unsupported text JIT operation"; return 0;
    }
#if defined(__aarch64__)
    if (!nc_jit_arm64_word(buffer, 0xA9BF7BFDu) ||
        !nc_jit_arm64_word(buffer, 0x910003FDu) ||
        !nc_jit_arm64_word(buffer, 0xAA0003E8u) ||
        !nc_jit_arm64_word(buffer, 0xF9400000u | ((unsigned int)first << 10) | (8u << 5))) return 0;
    if (binary && !nc_jit_arm64_word(buffer, 0xF9400001u | ((unsigned int)second << 10) | (8u << 5))) return 0;
    if (!nc_jit_arm64_const(buffer, 16, (unsigned long long)(uintptr_t)helper) ||
        !nc_jit_arm64_word(buffer, 0xD63F0200u) ||
        !nc_jit_arm64_word(buffer, 0xA8C17BFDu) ||
        !nc_jit_arm64_word(buffer, 0xD65F03C0u)) return 0;
#elif defined(__x86_64__)
#if defined(_WIN32)
    if (!nc_jit_u8(buffer, 0x49) || !nc_jit_u8(buffer, 0x89) || !nc_jit_u8(buffer, 0xC8) ||
        !nc_jit_u8(buffer, 0x49) || !nc_jit_u8(buffer, 0x8B) || !nc_jit_u8(buffer, 0x88) ||
        !nc_jit_u32(buffer, (unsigned int)(first * 8))) return 0;
    if (binary && (!nc_jit_u8(buffer, 0x49) || !nc_jit_u8(buffer, 0x8B) || !nc_jit_u8(buffer, 0x90) ||
                   !nc_jit_u32(buffer, (unsigned int)(second * 8)))) return 0;
    if (!nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0xB8) || !nc_jit_u64(buffer, (uintptr_t)helper) ||
        !nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0x83) || !nc_jit_u8(buffer, 0xEC) || !nc_jit_u8(buffer, 0x28) ||
        !nc_jit_u8(buffer, 0xFF) || !nc_jit_u8(buffer, 0xD0) ||
        !nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0x83) || !nc_jit_u8(buffer, 0xC4) || !nc_jit_u8(buffer, 0x28) ||
        !nc_jit_u8(buffer, 0xC3)) return 0;
#else
    if (!nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0x89) || !nc_jit_u8(buffer, 0xFA) ||
        !nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0x8B) || !nc_jit_u8(buffer, 0xBA) ||
        !nc_jit_u32(buffer, (unsigned int)(first * 8))) return 0;
    if (binary && (!nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0x8B) || !nc_jit_u8(buffer, 0xB2) ||
                   !nc_jit_u32(buffer, (unsigned int)(second * 8)))) return 0;
    if (!nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0xB8) || !nc_jit_u64(buffer, (uintptr_t)helper) ||
        !nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0x83) || !nc_jit_u8(buffer, 0xEC) || !nc_jit_u8(buffer, 0x08) ||
        !nc_jit_u8(buffer, 0xFF) || !nc_jit_u8(buffer, 0xD0) ||
        !nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0x83) || !nc_jit_u8(buffer, 0xC4) || !nc_jit_u8(buffer, 0x08) ||
        !nc_jit_u8(buffer, 0xC3)) return 0;
#endif
#else
    *error = "unsupported architecture"; return 0;
#endif
    *operation_count = operations->list->len;
    return 1;
}

static int nc_jit_compile_integer(NcVal *operations, int arity, NcJitBuffer *buffer,
                                  int *operation_count, const char **error) {
    if (!operations || operations->type != NC_LIST || operations->list->len <= 0) {
        *error = "operations must be a non-empty list"; return 0;
    }
    if (arity < 0 || arity > 32 || operations->list->len > 128) {
        *error = "JIT limits exceeded"; return 0;
    }
    int depth = 0, return_count = 0;
    NcJitLabel labels[32]; int label_count = 0;
    NcJitPatch patches[64]; int patch_count = 0;
#if defined(__aarch64__)
    if (!nc_jit_arm64_word(buffer, 0xAA0003E8u)) { *error = "code buffer full"; return 0; } /* mov x8,x0 */
#endif
    for (int i = 0; i < operations->list->len; i++) {
        NcVal *item = operations->list->items[i];
        if (!item || item->type != NC_STR) { *error = "operation must be text"; return 0; }
        const char *op = item->s ? item->s : "";
        if (!strncmp(op, "arg:", 4)) {
            char *end = NULL; long index = strtol(op + 4, &end, 10);
            if (!end || *end || index < 0 || index >= arity) { *error = "invalid argument index"; return 0; }
#if defined(__aarch64__)
            if (depth >= 7) { *error = "JIT register stack overflow"; return 0; }
            int target = 9 + depth;
            if (!nc_jit_arm64_word(buffer, 0xF9400000u | ((unsigned int)index << 10) | (8u << 5) | (unsigned int)target)) return 0;
#elif defined(__x86_64__)
            if (!nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0x8B) || !nc_jit_u8(buffer,
#if defined(_WIN32)
                0x81
#else
                0x87
#endif
                ) ||
                !nc_jit_u32(buffer, (unsigned int)(index * 8)) || !nc_jit_u8(buffer, 0x50)) return 0;
#else
            *error = "unsupported architecture"; return 0;
#endif
            depth++;
        } else if (!strncmp(op, "const:", 6)) {
            char *end = NULL; long long value = strtoll(op + 6, &end, 10);
            if (!end || *end) { *error = "invalid integer constant"; return 0; }
#if defined(__aarch64__)
            if (depth >= 7) { *error = "JIT register stack overflow"; return 0; }
            if (!nc_jit_arm64_const(buffer, 9 + depth, (unsigned long long)value)) return 0;
#elif defined(__x86_64__)
            if (!nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0xB8) ||
                !nc_jit_u64(buffer, (unsigned long long)value) || !nc_jit_u8(buffer, 0x50)) return 0;
#else
            *error = "unsupported architecture"; return 0;
#endif
            depth++;
        } else if (!strcmp(op, "add") || !strcmp(op, "sub") || !strcmp(op, "mul")) {
            if (depth < 2) { *error = "JIT expression stack underflow"; return 0; }
#if defined(__aarch64__)
            int left = 9 + depth - 2, right = 9 + depth - 1;
            unsigned int word = !strcmp(op, "add") ? 0x8B000000u : !strcmp(op, "sub") ? 0xCB000000u : 0x9B007C00u;
            word |= ((unsigned int)right << 16) | ((unsigned int)left << 5) | (unsigned int)left;
            if (!nc_jit_arm64_word(buffer, word)) return 0;
#elif defined(__x86_64__)
            if (!nc_jit_u8(buffer, 0x59) || !nc_jit_u8(buffer, 0x58) || !nc_jit_u8(buffer, 0x48)) return 0;
            if (!strcmp(op, "add")) { if (!nc_jit_u8(buffer, 0x01) || !nc_jit_u8(buffer, 0xC8)) return 0; }
            else if (!strcmp(op, "sub")) { if (!nc_jit_u8(buffer, 0x29) || !nc_jit_u8(buffer, 0xC8)) return 0; }
            else { if (!nc_jit_u8(buffer, 0x0F) || !nc_jit_u8(buffer, 0xAF) || !nc_jit_u8(buffer, 0xC1)) return 0; }
            if (!nc_jit_u8(buffer, 0x50)) return 0;
#endif
            depth--;
        } else if (!strcmp(op, "neg")) {
            if (depth < 1) { *error = "JIT expression stack underflow"; return 0; }
#if defined(__aarch64__)
            int target = 9 + depth - 1;
            if (!nc_jit_arm64_word(buffer, 0xCB0003E0u | ((unsigned int)target << 16) | (unsigned int)target)) return 0;
#elif defined(__x86_64__)
            if (!nc_jit_u8(buffer, 0x58) || !nc_jit_u8(buffer, 0x48) || !nc_jit_u8(buffer, 0xF7) ||
                !nc_jit_u8(buffer, 0xD8) || !nc_jit_u8(buffer, 0x50)) return 0;
#endif
        } else if (!strcmp(op, "eq") || !strcmp(op, "ne") || !strcmp(op, "lt") ||
                   !strcmp(op, "le") || !strcmp(op, "gt") || !strcmp(op, "ge")) {
            if (depth < 2) { *error = "JIT comparison stack underflow"; return 0; }
#if defined(__aarch64__)
            int left = 9 + depth - 2, right = 9 + depth - 1;
            unsigned int condition = !strcmp(op,"eq") ? 0x1u : !strcmp(op,"ne") ? 0x0u :
                                     !strcmp(op,"lt") ? 0xAu : !strcmp(op,"le") ? 0xCu :
                                     !strcmp(op,"gt") ? 0xDu : 0xBu;
            if (!nc_jit_arm64_word(buffer, 0xEB00001Fu | ((unsigned int)right << 16) | ((unsigned int)left << 5)) ||
                !nc_jit_arm64_word(buffer, 0x9A9F07E0u | (condition << 12) | (unsigned int)left)) return 0;
#elif defined(__x86_64__)
            unsigned int setcc = !strcmp(op,"eq") ? 0x94u : !strcmp(op,"ne") ? 0x95u :
                                 !strcmp(op,"lt") ? 0x9Cu : !strcmp(op,"le") ? 0x9Eu :
                                 !strcmp(op,"gt") ? 0x9Fu : 0x9Du;
            if (!nc_jit_u8(buffer,0x59) || !nc_jit_u8(buffer,0x58) || !nc_jit_u8(buffer,0x48) ||
                !nc_jit_u8(buffer,0x39) || !nc_jit_u8(buffer,0xC8) || !nc_jit_u8(buffer,0x0F) ||
                !nc_jit_u8(buffer,setcc) || !nc_jit_u8(buffer,0xC0) || !nc_jit_u8(buffer,0x48) ||
                !nc_jit_u8(buffer,0x0F) || !nc_jit_u8(buffer,0xB6) || !nc_jit_u8(buffer,0xC0) ||
                !nc_jit_u8(buffer,0x50)) return 0;
#endif
            depth--;
        } else if (!strncmp(op, "label:", 6)) {
            if (label_count >= 32 || !nc_jit_name(labels[label_count].name, op + 6)) { *error = "invalid JIT label"; return 0; }
            for (int li = 0; li < label_count; li++) if (!strcmp(labels[li].name, labels[label_count].name)) { *error = "duplicate JIT label"; return 0; }
            labels[label_count].offset = buffer->length; labels[label_count].depth = depth; label_count++;
        } else if (!strncmp(op, "jump_if_false:", 14)) {
            if (depth < 1 || patch_count >= 64 || !nc_jit_name(patches[patch_count].name, op + 14)) { *error = "invalid conditional JIT jump"; return 0; }
            depth--;
#if defined(__aarch64__)
            int condition_reg = 9 + depth; patches[patch_count].offset = buffer->length; patches[patch_count].kind = 2;patches[patch_count].depth=depth;
            if (!nc_jit_arm64_word(buffer, 0xB4000000u | (unsigned int)condition_reg)) return 0;
#elif defined(__x86_64__)
            if (!nc_jit_u8(buffer,0x58) || !nc_jit_u8(buffer,0x48) || !nc_jit_u8(buffer,0x85) || !nc_jit_u8(buffer,0xC0) ||
                !nc_jit_u8(buffer,0x0F) || !nc_jit_u8(buffer,0x84)) return 0;
            patches[patch_count].offset = buffer->length; patches[patch_count].kind = 4;patches[patch_count].depth=depth;
            if (!nc_jit_u32(buffer,0)) return 0;
#endif
            patch_count++;
        } else if (!strncmp(op, "jump:", 5)) {
            if (patch_count >= 64 || !nc_jit_name(patches[patch_count].name, op + 5)) { *error = "invalid JIT jump"; return 0; }
#if defined(__aarch64__)
            patches[patch_count].offset = buffer->length; patches[patch_count].kind = 1;patches[patch_count].depth=depth;
            if (!nc_jit_arm64_word(buffer,0x14000000u)) return 0;
#elif defined(__x86_64__)
            if (!nc_jit_u8(buffer,0xE9)) return 0;
            patches[patch_count].offset = buffer->length; patches[patch_count].kind = 3;patches[patch_count].depth=depth;
            if (!nc_jit_u32(buffer,0)) return 0;
#endif
            patch_count++;
        } else if (!strcmp(op, "return")) {
            if (depth != 1) { *error = "JIT return requires one value"; return 0; }
#if defined(__aarch64__)
            if (!nc_jit_arm64_word(buffer,0x91000000u | (9u << 5)) || !nc_jit_arm64_word(buffer,0xD65F03C0u)) return 0;
#elif defined(__x86_64__)
            if (!nc_jit_u8(buffer,0x58) || !nc_jit_u8(buffer,0xC3)) return 0;
#endif
            depth = 0; return_count++;
        } else {
            *error = "unsupported JIT operation"; return 0;
        }
    }
    if (!return_count && depth != 1) { *error = "JIT expression must leave one value"; return 0; }
    if (!return_count) {
#if defined(__aarch64__)
    if (!nc_jit_arm64_word(buffer, 0x91000000u | (9u << 5)) ||
        !nc_jit_arm64_word(buffer, 0xD65F03C0u)) return 0;
#elif defined(__x86_64__)
    if (!nc_jit_u8(buffer, 0x58) || !nc_jit_u8(buffer, 0xC3)) return 0;
#endif
    }
    for (int pi = 0; pi < patch_count; pi++) {
        int found = -1; for (int li = 0; li < label_count; li++) if (!strcmp(patches[pi].name, labels[li].name)) { found = li; break; }
        if (found < 0) { *error = "unknown JIT label"; return 0; }
        if (patches[pi].depth != labels[found].depth) { *error = "JIT branch stack depth mismatch"; return 0; }
        long long target = (long long)labels[found].offset;
#if defined(__aarch64__)
        long long delta_words = (target - (long long)patches[pi].offset) / 4;
        if ((target - (long long)patches[pi].offset) % 4) { *error = "unaligned JIT branch"; return 0; }
        unsigned int word = 0; memcpy(&word, buffer->data + patches[pi].offset, 4);
        if (patches[pi].kind == 1) {
            if (delta_words < -(1LL<<25) || delta_words >= (1LL<<25)) { *error = "JIT branch out of range"; return 0; }
            word = (word & 0xFC000000u) | ((unsigned int)delta_words & 0x03FFFFFFu);
        } else {
            if (delta_words < -(1LL<<18) || delta_words >= (1LL<<18)) { *error = "JIT conditional branch out of range"; return 0; }
            word = (word & 0xFF00001Fu) | (((unsigned int)delta_words & 0x7FFFFu) << 5);
        }
        memcpy(buffer->data + patches[pi].offset, &word, 4);
#elif defined(__x86_64__)
        long long displacement = target - ((long long)patches[pi].offset + 4);
        if (displacement < INT32_MIN || displacement > INT32_MAX) { *error = "JIT branch out of range"; return 0; }
        int32_t relative = (int32_t)displacement; memcpy(buffer->data + patches[pi].offset, &relative, 4);
#endif
    }
    *operation_count = operations->list->len;
    return 1;
}

static int nc_jit_compile_float(NcVal *operations,int arity,NcJitBuffer *buffer,
                                int *operation_count,const char **error){
    if(!operations||operations->type!=NC_LIST||operations->list->len<=0){*error="operations must be a non-empty list";return 0;}
    if(arity<0||arity>32||operations->list->len>128){*error="JIT limits exceeded";return 0;}int depth=0,return_count=0;NcJitLabel labels[32];int label_count=0;NcJitPatch patches[64];int patch_count=0;
#if defined(__aarch64__)
    if(!nc_jit_arm64_word(buffer,0xAA0003E8u)){*error="code buffer full";return 0;}
#endif
    for(int i=0;i<operations->list->len;i++){
        NcVal *item=operations->list->items[i];if(!item||item->type!=NC_STR){*error="operation must be text";return 0;}const char *op=item->s?item->s:"";
        if(!strncmp(op,"arg:",4)){
            char *end=NULL;long index=strtol(op+4,&end,10);if(!end||*end||index<0||index>=arity){*error="invalid argument index";return 0;}
#if defined(__aarch64__)
            if(depth>=7){*error="JIT register stack overflow";return 0;}int target=9+depth;
            if(!nc_jit_arm64_word(buffer,0xFD400000u|((unsigned int)index<<10)|(8u<<5)|(unsigned int)target))return 0;
#elif defined(__x86_64__)
            if(!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x10)||!nc_jit_u8(buffer,
#if defined(_WIN32)
                0x81
#else
                0x87
#endif
                )||!nc_jit_u32(buffer,(unsigned int)(index*8))||!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0x83)||!nc_jit_u8(buffer,0xEC)||!nc_jit_u8(buffer,0x08)||!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x11)||!nc_jit_u8(buffer,0x04)||!nc_jit_u8(buffer,0x24))return 0;
#else
            *error="unsupported architecture";return 0;
#endif
            depth++;
        }else if(!strncmp(op,"const:",6)){
            char *end=NULL;double value=strtod(op+6,&end);if(!end||*end){*error="invalid float constant";return 0;}unsigned long long bits=0;memcpy(&bits,&value,sizeof(bits));
#if defined(__aarch64__)
            if(depth>=7){*error="JIT register stack overflow";return 0;}if(!nc_jit_arm64_const(buffer,16,bits)||!nc_jit_arm64_word(buffer,0x9E670000u|(16u<<5)|(unsigned int)(9+depth)))return 0;
#elif defined(__x86_64__)
            if(!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0xB8)||!nc_jit_u64(buffer,bits)||!nc_jit_u8(buffer,0x50))return 0;
#endif
            depth++;
        }else if(!strcmp(op,"add")||!strcmp(op,"sub")||!strcmp(op,"mul")||!strcmp(op,"div")){
            if(depth<2){*error="JIT expression stack underflow";return 0;}
#if defined(__aarch64__)
            int left=9+depth-2,right=9+depth-1;unsigned int word=!strcmp(op,"add")?0x1E602800u:!strcmp(op,"sub")?0x1E603800u:!strcmp(op,"mul")?0x1E600800u:0x1E601800u;
            if(!nc_jit_arm64_word(buffer,word|((unsigned int)right<<16)|((unsigned int)left<<5)|(unsigned int)left))return 0;
#elif defined(__x86_64__)
            unsigned int opcode=!strcmp(op,"add")?0x58u:!strcmp(op,"sub")?0x5Cu:!strcmp(op,"mul")?0x59u:0x5Eu;
            if(!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x10)||!nc_jit_u8(buffer,0x0C)||!nc_jit_u8(buffer,0x24)||!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0x83)||!nc_jit_u8(buffer,0xC4)||!nc_jit_u8(buffer,0x08)||!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x10)||!nc_jit_u8(buffer,0x04)||!nc_jit_u8(buffer,0x24)||!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,opcode)||!nc_jit_u8(buffer,0xC1)||!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x11)||!nc_jit_u8(buffer,0x04)||!nc_jit_u8(buffer,0x24))return 0;
#endif
            depth--;
        }else if(!strcmp(op,"neg")){
            if(depth<1){*error="JIT expression stack underflow";return 0;}
#if defined(__aarch64__)
            int target=9+depth-1;if(!nc_jit_arm64_word(buffer,0x1E614000u|((unsigned int)target<<5)|(unsigned int)target))return 0;
#elif defined(__x86_64__)
            if(!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0xB8)||!nc_jit_u64(buffer,0x8000000000000000ULL)||!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0x31)||!nc_jit_u8(buffer,0x04)||!nc_jit_u8(buffer,0x24))return 0;
#endif
        }else if(!strcmp(op,"eq")||!strcmp(op,"ne")||!strcmp(op,"lt")||!strcmp(op,"le")||!strcmp(op,"gt")||!strcmp(op,"ge")){
            if(depth<2){*error="JIT comparison stack underflow";return 0;}
#if defined(__aarch64__)
            int left=9+depth-2,right=9+depth-1;unsigned int condition=!strcmp(op,"eq")?0x1u:!strcmp(op,"ne")?0x0u:!strcmp(op,"lt")?0x5u:!strcmp(op,"le")?0x8u:!strcmp(op,"gt")?0xDu:0xBu;
            if(!nc_jit_arm64_word(buffer,0x1E602000u|((unsigned int)right<<16)|((unsigned int)left<<5))||!nc_jit_arm64_word(buffer,0x9A9F07F0u|(condition<<12))||!nc_jit_arm64_word(buffer,0x9E620200u|(16u<<5)|(unsigned int)left))return 0;
#elif defined(__x86_64__)
            unsigned int setcc=!strcmp(op,"eq")?0x94u:!strcmp(op,"ne")?0x95u:!strcmp(op,"lt")?0x92u:!strcmp(op,"le")?0x96u:!strcmp(op,"gt")?0x97u:0x93u;
            if(!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x10)||!nc_jit_u8(buffer,0x0C)||!nc_jit_u8(buffer,0x24)||!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0x83)||!nc_jit_u8(buffer,0xC4)||!nc_jit_u8(buffer,0x08)||!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x10)||!nc_jit_u8(buffer,0x04)||!nc_jit_u8(buffer,0x24)||!nc_jit_u8(buffer,0x66)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x2E)||!nc_jit_u8(buffer,0xC1)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,setcc)||!nc_jit_u8(buffer,0xC0))return 0;
            if(!strcmp(op,"eq")||!strcmp(op,"lt")||!strcmp(op,"le")){if(!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x9B)||!nc_jit_u8(buffer,0xC2)||!nc_jit_u8(buffer,0x20)||!nc_jit_u8(buffer,0xD0))return 0;}else if(!strcmp(op,"ne")){if(!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x9A)||!nc_jit_u8(buffer,0xC2)||!nc_jit_u8(buffer,0x08)||!nc_jit_u8(buffer,0xD0))return 0;}
            if(!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0xB6)||!nc_jit_u8(buffer,0xC0)||!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x2A)||!nc_jit_u8(buffer,0xC0)||!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x11)||!nc_jit_u8(buffer,0x04)||!nc_jit_u8(buffer,0x24))return 0;
#endif
            depth--;
        }else if(!strncmp(op,"label:",6)){
            if(label_count>=32||!nc_jit_name(labels[label_count].name,op+6)){*error="invalid JIT label";return 0;}for(int li=0;li<label_count;li++)if(!strcmp(labels[li].name,labels[label_count].name)){*error="duplicate JIT label";return 0;}labels[label_count].offset=buffer->length;labels[label_count].depth=depth;label_count++;
        }else if(!strncmp(op,"jump_if_false:",14)){
            if(depth<1||patch_count>=64||!nc_jit_name(patches[patch_count].name,op+14)){*error="invalid conditional JIT jump";return 0;}depth--;
#if defined(__aarch64__)
            int condition_reg=9+depth;if(!nc_jit_arm64_word(buffer,0x1E602008u|((unsigned int)condition_reg<<5)))return 0;patches[patch_count].offset=buffer->length;patches[patch_count].kind=2;patches[patch_count].depth=depth;if(!nc_jit_arm64_word(buffer,0x54000000u))return 0;
#elif defined(__x86_64__)
            if(!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x10)||!nc_jit_u8(buffer,0x04)||!nc_jit_u8(buffer,0x24)||!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0x83)||!nc_jit_u8(buffer,0xC4)||!nc_jit_u8(buffer,0x08)||!nc_jit_u8(buffer,0x66)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x57)||!nc_jit_u8(buffer,0xC9)||!nc_jit_u8(buffer,0x66)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x2E)||!nc_jit_u8(buffer,0xC1)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x84))return 0;patches[patch_count].offset=buffer->length;patches[patch_count].kind=4;patches[patch_count].depth=depth;if(!nc_jit_u32(buffer,0))return 0;
#endif
            patch_count++;
        }else if(!strncmp(op,"jump:",5)){
            if(patch_count>=64||!nc_jit_name(patches[patch_count].name,op+5)){*error="invalid JIT jump";return 0;}
#if defined(__aarch64__)
            patches[patch_count].offset=buffer->length;patches[patch_count].kind=1;patches[patch_count].depth=depth;if(!nc_jit_arm64_word(buffer,0x14000000u))return 0;
#elif defined(__x86_64__)
            if(!nc_jit_u8(buffer,0xE9))return 0;patches[patch_count].offset=buffer->length;patches[patch_count].kind=3;patches[patch_count].depth=depth;if(!nc_jit_u32(buffer,0))return 0;
#endif
            patch_count++;
        }else if(!strcmp(op,"return")){
            if(depth!=1){*error="JIT return requires one value";return 0;}
#if defined(__aarch64__)
            if(!nc_jit_arm64_word(buffer,0x1E604000u|(9u<<5))||!nc_jit_arm64_word(buffer,0xD65F03C0u))return 0;
#elif defined(__x86_64__)
            if(!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x10)||!nc_jit_u8(buffer,0x04)||!nc_jit_u8(buffer,0x24)||!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0x83)||!nc_jit_u8(buffer,0xC4)||!nc_jit_u8(buffer,0x08)||!nc_jit_u8(buffer,0xC3))return 0;
#endif
            depth=0;return_count++;
        }else{*error="unsupported float JIT operation";return 0;}
    }
    if(depth==1){
#if defined(__aarch64__)
        if(!nc_jit_arm64_word(buffer,0x1E604000u|(9u<<5))||!nc_jit_arm64_word(buffer,0xD65F03C0u))return 0;
#elif defined(__x86_64__)
        if(!nc_jit_u8(buffer,0xF2)||!nc_jit_u8(buffer,0x0F)||!nc_jit_u8(buffer,0x10)||!nc_jit_u8(buffer,0x04)||!nc_jit_u8(buffer,0x24)||!nc_jit_u8(buffer,0x48)||!nc_jit_u8(buffer,0x83)||!nc_jit_u8(buffer,0xC4)||!nc_jit_u8(buffer,0x08)||!nc_jit_u8(buffer,0xC3))return 0;
#endif
    }else if(depth!=0){*error="JIT expression must leave one value";return 0;}else if(!return_count){*error="JIT expression must return a value";return 0;}
    for(int pi=0;pi<patch_count;pi++){
        int found=-1;for(int li=0;li<label_count;li++)if(!strcmp(patches[pi].name,labels[li].name)){found=li;break;}if(found<0){*error="unknown JIT label";return 0;}if(patches[pi].depth!=labels[found].depth){*error="JIT branch stack depth mismatch";return 0;}long long target=(long long)labels[found].offset;
#if defined(__aarch64__)
        long long delta_words=(target-(long long)patches[pi].offset)/4;if((target-(long long)patches[pi].offset)%4){*error="unaligned JIT branch";return 0;}unsigned int word=0;memcpy(&word,buffer->data+patches[pi].offset,4);if(patches[pi].kind==1){if(delta_words<-(1LL<<25)||delta_words>=(1LL<<25)){*error="JIT branch out of range";return 0;}word=(word&0xFC000000u)|((unsigned int)delta_words&0x03FFFFFFu);}else{if(delta_words<-(1LL<<18)||delta_words>=(1LL<<18)){*error="JIT conditional branch out of range";return 0;}word=(word&0xFF00001Fu)|(((unsigned int)delta_words&0x7FFFFu)<<5);}memcpy(buffer->data+patches[pi].offset,&word,4);
#elif defined(__x86_64__)
        long long displacement=target-((long long)patches[pi].offset+4);if(displacement<INT32_MIN||displacement>INT32_MAX){*error="JIT branch out of range";return 0;}int32_t relative=(int32_t)displacement;memcpy(buffer->data+patches[pi].offset,&relative,4);
#endif
    }
    *operation_count=operations->list->len;return 1;
}

static int nc_jit_parse_handle(const char *handle, unsigned int *generation) {
    if (!handle || strncmp(handle, "jit:", 4)) return -1;
    char *end = NULL; long slot = strtol(handle + 4, &end, 10);
    if (!end || *end != ':' || slot <= 0 || slot >= NC_JIT_MAX) return -1;
    char *generation_end = NULL; unsigned long parsed = strtoul(end + 1, &generation_end, 10);
    if (!generation_end || *generation_end) return -1;
    *generation = (unsigned int)parsed;
    return (int)slot;
}

static NcVal *nc_builtin_jit_operation(NcVal *request) {
    if (!request || request->type != NC_MAP) return nc_jit_result("feil", "invalid request");
    const char *abi = nc_atomic_text_field(request, "abi", "");
    const char *operation = nc_atomic_text_field(request, "operation", "");
    if (strcmp(abi, "norscode-jit-v1")) return nc_jit_result("feil", "invalid ABI");
    if (!strcmp(operation, "status")) {
        NcVal *result = nc_jit_result("ferdig", ""); int active = 0;
        pthread_mutex_lock(&g_jit_lock);
        for (int i = 1; i < NC_JIT_MAX; i++) if (g_jit_slots[i].active) active++;
        pthread_mutex_unlock(&g_jit_lock);
        nc_index_set(result, nc_str("active"), nc_int(active));
        nc_index_set(result, nc_str("max_slots"), nc_int(NC_JIT_MAX - 1));
        return result;
    }
    if (!strcmp(operation, "compile")) {
        int arity = (int)nc_atomic_int_field(request, "arity", -1), operation_count = 0;
        const char *result_kind = nc_atomic_text_field(request, "result_kind", "int");
        const char *numeric_kind = nc_atomic_text_field(request, "numeric_kind", !strcmp(result_kind,"float")?"float":"int");
        if (strcmp(result_kind, "int") && strcmp(result_kind, "bool") && strcmp(result_kind,"float") && strcmp(result_kind,"text")) return nc_jit_result("feil", "invalid JIT result kind");
        if(strcmp(numeric_kind,"int")&&strcmp(numeric_kind,"float")&&strcmp(numeric_kind,"text"))return nc_jit_result("feil","invalid JIT numeric kind");
        if(!strcmp(numeric_kind,"float")&&!strcmp(result_kind,"int"))return nc_jit_result("feil","float JIT cannot return integer");
        NcVal *operations = nc_index_get(request, nc_str("operations"));
        NcJitBuffer buffer = {{0}, 0}; const char *error = "code generation failed";
        int float_code=!strcmp(numeric_kind,"float"),text_code=!strcmp(numeric_kind,"text"),float_result=!strcmp(result_kind,"float");
        int compiled = text_code ? nc_jit_compile_text(operations,arity,result_kind,&buffer,&operation_count,&error) :
                       float_code ? nc_jit_compile_float(operations,arity,&buffer,&operation_count,&error) :
                                    nc_jit_compile_integer(operations,arity,&buffer,&operation_count,&error);
        if (!compiled) return nc_jit_result("feil", error);
        long page_size = 4096;
#if !defined(_WIN32)
        page_size = sysconf(_SC_PAGESIZE); if (page_size <= 0) page_size = 4096;
#endif
        size_t mapped = (buffer.length + (size_t)page_size - 1) & ~((size_t)page_size - 1);
#if defined(_WIN32)
        void *memory = VirtualAlloc(NULL,mapped,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if(!memory)return nc_jit_result("feil","VirtualAlloc failed");
#else
        void *memory = mmap(NULL, mapped, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (memory == MAP_FAILED) return nc_jit_result("feil", strerror(errno));
#endif
        memcpy(memory, buffer.data, buffer.length);
#if defined(_WIN32)
        FlushInstructionCache(GetCurrentProcess(),memory,buffer.length);DWORD old_protection=0;
        if(!VirtualProtect(memory,mapped,PAGE_EXECUTE_READ,&old_protection)){VirtualFree(memory,0,MEM_RELEASE);return nc_jit_result("feil","VirtualProtect failed");}
#else
        __builtin___clear_cache((char *)memory, (char *)memory + buffer.length);
        if (mprotect(memory, mapped, PROT_READ | PROT_EXEC) != 0) { error = strerror(errno); munmap(memory, mapped); return nc_jit_result("feil", error); }
#endif
        pthread_mutex_lock(&g_jit_lock);
        int slot = 0; for (int i = 1; i < NC_JIT_MAX; i++) if (!g_jit_slots[i].active) { slot = i; break; }
        if (!slot) { pthread_mutex_unlock(&g_jit_lock);
#if defined(_WIN32)
            VirtualFree(memory,0,MEM_RELEASE);
#else
            munmap(memory,mapped);
#endif
            return nc_jit_result("feil", "JIT registry full"); }
        NcJitSlot *entry = &g_jit_slots[slot]; entry->generation++; if (!entry->generation) entry->generation = 1;
        entry->active = 1; entry->memory = memory; entry->mapped_size = mapped; entry->code_size = buffer.length;
        entry->arity = arity; entry->operation_count = operation_count;entry->numeric_kind=text_code?2:float_code?1:0; entry->result_kind = !strcmp(result_kind,"text")?3:float_result?2:!strcmp(result_kind,"bool")?1:0; unsigned int generation = entry->generation;
        pthread_mutex_unlock(&g_jit_lock);
        char handle[48]; snprintf(handle, sizeof(handle), "jit:%d:%u", slot, generation);
        NcVal *result = nc_jit_result("ferdig", "");
        nc_index_set(result, nc_str("handle"), nc_str(handle)); nc_index_set(result, nc_str("code_bytes"), nc_int((long long)buffer.length));
        nc_index_set(result, nc_str("arity"), nc_int(arity)); nc_index_set(result, nc_str("operations"), nc_int(operation_count));
        return result;
    }
    unsigned int generation = 0; int slot = nc_jit_parse_handle(nc_atomic_text_field(request, "handle", ""), &generation);
    if (slot < 0) return nc_jit_result("feil", "invalid JIT handle");
    pthread_mutex_lock(&g_jit_lock); NcJitSlot *entry = &g_jit_slots[slot];
    if (!entry->active || entry->generation != generation) { pthread_mutex_unlock(&g_jit_lock); return nc_jit_result("feil", "stale JIT handle"); }
    if (!strcmp(operation, "run")) {
        NcVal *arguments = nc_index_get(request, nc_str("arguments"));
        if (!arguments || arguments->type != NC_LIST || arguments->list->len != entry->arity) { pthread_mutex_unlock(&g_jit_lock); return nc_jit_result("feil", "invalid JIT arguments"); }
        long long values[32];double float_values[32],float_value=0.0;long long value=0;uintptr_t text_value=0;int output_kind=entry->result_kind,input_kind=entry->numeric_kind;
        for (int i = 0; i < entry->arity; i++) { NcVal *argument=arguments->list->items[i];if(entry->numeric_kind==2){if(!argument||argument->type!=NC_STR){pthread_mutex_unlock(&g_jit_lock);return nc_jit_result("feil","text JIT arguments must be text");}}else if(entry->numeric_kind==1){if(!argument||(argument->type!=NC_INT&&argument->type!=NC_FLOAT)){pthread_mutex_unlock(&g_jit_lock);return nc_jit_result("feil","float JIT arguments must be numeric");}float_values[i]=argument->type==NC_FLOAT?argument->f:(double)argument->i;}else{if(!argument||argument->type!=NC_INT){pthread_mutex_unlock(&g_jit_lock);return nc_jit_result("feil","JIT arguments must be integers");}values[i]=argument->i;}}
        if(entry->numeric_kind==2){text_value=((NcJitTextFn)entry->memory)(arguments->list->items);value=(long long)text_value;}else if(entry->numeric_kind==1)float_value=((NcJitFloatFn)entry->memory)(float_values);else value=((NcJitFn)entry->memory)(values);pthread_mutex_unlock(&g_jit_lock);
        if(output_kind==3&&!text_value)return nc_jit_result("feil","text JIT allocation or size limit failed");
        NcVal *result = nc_jit_result("ferdig", "");
        if(output_kind==3){nc_index_set(result,nc_str("value"),nc_str((const char *)text_value));free((void *)text_value);}
        else nc_index_set(result,nc_str("value"),output_kind==2?nc_float(float_value):output_kind==1?nc_bool(input_kind==1?float_value!=0.0:value!=0):nc_int(value)); return result;
    }
    if (!strcmp(operation, "free")) {
        void *memory = entry->memory; size_t mapped = entry->mapped_size; entry->active = 0; entry->memory = NULL; entry->mapped_size = 0;
        pthread_mutex_unlock(&g_jit_lock);
#if defined(_WIN32)
        VirtualFree(memory,0,MEM_RELEASE);
#else
        munmap(memory,mapped);
#endif
        return nc_jit_result("ferdig", "");
    }
    pthread_mutex_unlock(&g_jit_lock); return nc_jit_result("feil", "unknown JIT operation");
}

static long long nc_monotonic_ms(void) {
#if defined(_WIN32)
    return (long long)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
#endif
}

static void nc_fd_nonblocking(nc_socket_handle_t fd) {
#if defined(_WIN32)
    u_long enabled=1;(void)ioctlsocket(fd,FIONBIO,&enabled);
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

static void nc_close_inherited_fds(void) {
#if defined(_WIN32)
    return;
#else
    struct rlimit limit;
    rlim_t maximum = 1024;
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0 && limit.rlim_cur != RLIM_INFINITY)
        maximum = limit.rlim_cur;
    for (int fd = 3; (rlim_t)fd < maximum; fd++) close(fd);
#endif
}

static const char *nc_readiness_backend(void) {
#if defined(__APPLE__)
    return "kqueue";
#elif defined(__linux__)
    return "epoll";
#elif defined(_WIN32)
    return "wsapoll";
#else
    return "poll";
#endif
}

static int nc_poll_wait_fd(nc_socket_handle_t fd,int events,int timeout_ms,int *ready){
#if defined(_WIN32)
    WSAPOLLFD descriptor;memset(&descriptor,0,sizeof(descriptor));descriptor.fd=fd;
    if(events&1)descriptor.events|=POLLRDNORM;if(events&2)descriptor.events|=POLLWRNORM;
    int count=WSAPoll(&descriptor,1,timeout_ms);
    if(count>0){if(descriptor.revents&(POLLRDNORM|POLLIN))*ready|=1;if(descriptor.revents&(POLLWRNORM|POLLOUT))*ready|=2;if(descriptor.revents&POLLERR)*ready|=4;if(descriptor.revents&(POLLHUP|POLLNVAL))*ready|=8;}return count;
#else
    short wanted=0;if(events&1)wanted|=POLLIN;if(events&2)wanted|=POLLOUT;
    struct pollfd descriptor={fd,wanted,0};int count=poll(&descriptor,1,timeout_ms);
    if(count>0){if(descriptor.revents&POLLIN)*ready|=1;if(descriptor.revents&POLLOUT)*ready|=2;if(descriptor.revents&POLLERR)*ready|=4;if(descriptor.revents&(POLLHUP|POLLNVAL))*ready|=8;}return count;
#endif
}

static int nc_wait_fd(nc_socket_handle_t fd, int events, int timeout_ms, int *ready) {
    *ready = 0;
#if defined(__APPLE__)
    static _Thread_local int queue_fd = -1;
    if (queue_fd < 0) queue_fd = kqueue();
    if (queue_fd < 0) return -1;
    struct kevent changes[2], received[2];
    int change_count = 0;
    if (events & 1) EV_SET(&changes[change_count++], (uintptr_t)fd, EVFILT_READ,
                           EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, NULL);
    if (events & 2) EV_SET(&changes[change_count++], (uintptr_t)fd, EVFILT_WRITE,
                           EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, NULL);
    struct timespec timeout = {
        timeout_ms / 1000,
        (long)(timeout_ms % 1000) * 1000000L
    };
    int count = kevent(queue_fd, changes, change_count, received, 2, &timeout);
    if (count < 0) return -1;
    struct kevent deletions[2], deletion_receipts[2];
    int deletion_count = 0;
    if (events & 1) EV_SET(&deletions[deletion_count++], (uintptr_t)fd, EVFILT_READ,
                           EV_DELETE | EV_RECEIPT, 0, 0, NULL);
    if (events & 2) EV_SET(&deletions[deletion_count++], (uintptr_t)fd, EVFILT_WRITE,
                           EV_DELETE | EV_RECEIPT, 0, 0, NULL);
    struct timespec no_wait = {0, 0};
    (void)kevent(queue_fd, deletions, deletion_count, deletion_receipts, deletion_count, &no_wait);
    int matched = 0;
    for (int i = 0; i < count; i++) {
        if (received[i].ident != (uintptr_t)fd) continue;
        matched++;
        if ((received[i].flags & EV_ERROR) && received[i].data != 0) {
            errno = (int)received[i].data;
            return -1;
        }
        if (received[i].filter == EVFILT_READ) *ready |= 1;
        if (received[i].filter == EVFILT_WRITE) *ready |= 2;
        if (received[i].flags & EV_EOF) *ready |= 8;
    }
    return matched;
#elif defined(__linux__)
    static _Thread_local int epoll_fd = -1;
    if (epoll_fd < 0) epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) return -1;
    struct epoll_event registration;
    memset(&registration, 0, sizeof(registration));
    if (events & 1) registration.events |= EPOLLIN;
    if (events & 2) registration.events |= EPOLLOUT;
    registration.events |= EPOLLONESHOT | EPOLLERR | EPOLLHUP;
    registration.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &registration) != 0) {
        if (errno != ENOENT || epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &registration) != 0) {
            if (errno != EEXIST || epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &registration) != 0){int registration_error=errno;if(registration_error==EPERM)return nc_poll_wait_fd(fd,events,timeout_ms,ready);errno=registration_error;return -1;}
        }
    }
    struct epoll_event received;
    int count = epoll_wait(epoll_fd, &received, 1, timeout_ms);
    if (count > 0) {
        if (received.events & EPOLLIN) *ready |= 1;
        if (received.events & EPOLLOUT) *ready |= 2;
        if (received.events & EPOLLERR) *ready |= 4;
        if (received.events & (EPOLLHUP | EPOLLRDHUP)) *ready |= 8;
    }
    return count;
#else
    return nc_poll_wait_fd(fd,events,timeout_ms,ready);
#endif
}

#define NC_READINESS_BATCH_MAX 256
static int nc_wait_many_fds(const nc_socket_handle_t *fds, const int *events, int *ready,
                            int count, int timeout_ms) {
    if (!fds || !events || !ready || count < 0 || count > NC_READINESS_BATCH_MAX) {
        errno = EINVAL;
        return -1;
    }
    if (count == 0) return 0;
#if defined(_WIN32)
    WSAPOLLFD descriptors[NC_READINESS_BATCH_MAX];
#else
    struct pollfd descriptors[NC_READINESS_BATCH_MAX];
#endif
    memset(descriptors, 0, sizeof(descriptors));
    for (int i = 0; i < count; i++) {
        descriptors[i].fd = fds[i];
        if (events[i] & 1) descriptors[i].events |=
#if defined(_WIN32)
            POLLRDNORM;
#else
            POLLIN;
#endif
        if (events[i] & 2) descriptors[i].events |=
#if defined(_WIN32)
            POLLWRNORM;
#else
            POLLOUT;
#endif
        ready[i] = 0;
    }
    int result =
#if defined(_WIN32)
        WSAPoll(descriptors,(ULONG)count,timeout_ms);
#else
        poll(descriptors,(nfds_t)count,timeout_ms);
#endif
    if (result <= 0) return result;
    for (int i = 0; i < count; i++) {
        if (descriptors[i].revents & (POLLIN
#if defined(_WIN32)
            |POLLRDNORM
#endif
            )) ready[i] |= 1;
        if (descriptors[i].revents & (POLLOUT
#if defined(_WIN32)
            |POLLWRNORM
#endif
            )) ready[i] |= 2;
        if (descriptors[i].revents & POLLERR) ready[i] |= 4;
        if (descriptors[i].revents & (POLLHUP | POLLNVAL)) ready[i] |= 8;
    }
    return result;
}
#if !defined(_WIN32)
static int nc_apply_process_sandbox(const char *profile,long long timeout_ms,long long max_memory,long long max_files,int inherit_env);
static unsigned long long nc_process_resident_bytes(pid_t pid);
#endif

static NcVal *nc_builtin_process_spawn_argv(NcVal *request) {
    if (!request || request->type != NC_MAP)
        return nc_process_result("feil", -1, -1, "", "", "invalid request");
    const char *abi = nc_atomic_text_field(request, "abi", "");
    const char *executable = nc_atomic_text_field(request, "executable", "");
    const char *cwd = nc_atomic_text_field(request, "cwd", "");
    const char *stdin_text = nc_atomic_text_field(request, "stdin", "");
    long long timeout_ms = nc_atomic_int_field(request, "timeout_ms", 0);
    long long max_output = nc_atomic_int_field(request, "max_output_bytes", 0);
    const char *sandbox_profile = nc_atomic_text_field(request, "sandbox", "none");
    long long max_memory = nc_atomic_int_field(request, "max_memory_bytes", 536870912);
    long long max_files = nc_atomic_int_field(request, "max_open_files", 64);
    int inherit_env = nc_atomic_int_field(request, "inherit_env", 1) != 0;
    NcVal *args = nc_index_get(request, nc_str("args"));
    if (strcmp(abi, "norscode-process-spawn-v1") || !*executable ||
        timeout_ms <= 0 || max_output <= 0 || !args || args->type != NC_LIST)
        return nc_process_result("feil", -1, -1, "", "", "invalid request fields");
    if (max_output > 64LL * 1024LL * 1024LL) max_output = 64LL * 1024LL * 1024LL;

    int argc = args->list->len + 1;
    char **argv = calloc((size_t)argc + 1, sizeof(char *));
    if (!argv) return nc_process_result("feil", -1, -1, "", "", "out of memory");
    argv[0] = strdup(executable);
    int invalid_arg = 0;
    for (int i = 1; i < argc; i++) {
        NcVal *v = args->list->items[i - 1];
        if (!v || v->type != NC_STR) { invalid_arg = 1; break; }
        argv[i] = strdup(v->s ? v->s : "");
    }
    if (invalid_arg) {
        for (int i = 0; i < argc; i++) free(argv[i]);
        free(argv);
        return nc_process_result("feil", -1, -1, "", "", "argv must contain text");
    }

#if defined(_WIN32)
    (void)max_files; (void)inherit_env;
    NcwProcess process; char backend_error[NCW_ERROR_CAP] = "";
    if (!ncw_process_spawn(&process, executable, (const char *const *)argv, (size_t)argc,
                           cwd, stdin_text, strlen(stdin_text), (uint64_t)timeout_ms,
                           (uint64_t)(max_memory > 0 ? max_memory : 0),
                           sandbox_profile,
                           backend_error, sizeof(backend_error))) {
        for (int i = 0; i < argc; i++) free(argv[i]); free(argv);
        return nc_process_result("feil", -1, -1, "", "", backend_error);
    }
    for (int i = 0; i < argc; i++) free(argv[i]); free(argv);
    char *out = calloc((size_t)max_output + 1, 1), *err = calloc((size_t)max_output + 1, 1);
    if (!out || !err) { free(out); free(err); ncw_process_terminate(&process, 126, backend_error, sizeof(backend_error)); ncw_process_close(&process, backend_error, sizeof(backend_error)); return nc_process_result("feil", process.pid, 126, "", "", "out of memory"); }
    size_t out_len = 0, err_len = 0; int output_limit = 0;
    while (!process.exited) {
        for (int stream = 0; stream < 2; stream++) {
            char chunk[4096]; int64_t n;
            while ((n = ncw_process_read(&process, stream, chunk, sizeof(chunk), backend_error, sizeof(backend_error))) > 0) {
                size_t remaining = (size_t)max_output - out_len - err_len, take = (size_t)n < remaining ? (size_t)n : remaining;
                char *target = stream ? err : out; size_t *used = stream ? &err_len : &out_len;
                if (take) { memcpy(target + *used, chunk, take); *used += take; }
                if (take < (size_t)n || out_len + err_len >= (size_t)max_output) { output_limit = 1; ncw_process_terminate(&process, 125, backend_error, sizeof(backend_error)); break; }
            }
        }
        if (!process.exited && ncw_process_wait(&process, 10, backend_error, sizeof(backend_error)) < 0) break;
    }
    for (int stream = 0; stream < 2; stream++) { char chunk[4096]; int64_t n; while ((n = ncw_process_read(&process, stream, chunk, sizeof(chunk), backend_error, sizeof(backend_error))) > 0) { size_t remaining=(size_t)max_output-out_len-err_len,take=(size_t)n<remaining?(size_t)n:remaining;char *target=stream?err:out;size_t *used=stream?&err_len:&out_len;if(take){memcpy(target+*used,chunk,take);*used+=take;} } }
    out[out_len] = 0; err[err_len] = 0; DWORD pid = process.pid, exit_code = process.exit_code; int timed_out = process.timed_out;
    ncw_process_close(&process, backend_error, sizeof(backend_error));
    const char *status = timed_out ? "timeout" : output_limit ? "feil" : "ferdig";
    const char *error = timed_out ? "timeout" : output_limit ? "output limit" : "";
    NcVal *result = nc_process_result(status, pid, timed_out ? 124 : output_limit ? 125 : (int)exit_code, out, err, error);
    nc_index_set(result,nc_str("sandbox"),nc_str(sandbox_profile));
    nc_index_set(result,nc_str("sandbox_backend"),nc_str(process.appcontainer?"appcontainer+job-object":strcmp(sandbox_profile,"none")?"job-object":"none"));
    free(out); free(err); return result;
#else

    int in_pipe[2] = {-1, -1}, out_pipe[2] = {-1, -1}, err_pipe[2] = {-1, -1};
    if (pipe(in_pipe) || pipe(out_pipe) || pipe(err_pipe)) {
        for (int i = 0; i < argc; i++) free(argv[i]);
        free(argv);
        return nc_process_result("feil", -1, -1, "", "", "pipe failed");
    }

    pid_t pid = fork();
    if (pid == 0) {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        nc_close_inherited_fds();
        if (*cwd && chdir(cwd) != 0) {
            dprintf(STDERR_FILENO, "chdir failed: %s", strerror(errno));
            _exit(126);
        }
        if (!nc_apply_process_sandbox(sandbox_profile,timeout_ms,max_memory,max_files,inherit_env)) { dprintf(STDERR_FILENO,"sandbox setup failed"); _exit(126); }
        execv(executable, argv);
        dprintf(STDERR_FILENO, "execv failed: %s", strerror(errno));
        _exit(127);
    }
    for (int i = 0; i < argc; i++) free(argv[i]);
    free(argv);
    if (pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]); close(out_pipe[0]); close(out_pipe[1]); close(err_pipe[0]); close(err_pipe[1]);
        return nc_process_result("feil", -1, -1, "", "", "fork failed");
    }

    close(in_pipe[0]); close(out_pipe[1]); close(err_pipe[1]);
    nc_fd_nonblocking(in_pipe[1]); nc_fd_nonblocking(out_pipe[0]); nc_fd_nonblocking(err_pipe[0]);
    char *out = calloc((size_t)max_output + 1, 1);
    char *err = calloc((size_t)max_output + 1, 1);
    size_t out_len = 0, err_len = 0, stdin_off = 0, stdin_len = strlen(stdin_text);
    int in_open = 1, out_open = 1, err_open = 1, child_done = 0, wait_status = 0;
    int timed_out = 0, output_limit = 0, memory_limited = 0;
    long long deadline = nc_monotonic_ms() + timeout_ms;
    while (!child_done || out_open || err_open) {
        if (!child_done && nc_monotonic_ms() >= deadline) {
            timed_out = 1;
            kill(pid, SIGKILL);
        }
        if(!child_done&&!memory_limited&&max_memory>0){unsigned long long resident=nc_process_resident_bytes(pid);if(resident>(unsigned long long)max_memory){memory_limited=1;kill(pid,SIGKILL);}}
        struct pollfd fds[3];
        int nfds = 0, in_idx = -1, out_idx = -1, err_idx = -1;
        if (in_open) { in_idx = nfds; fds[nfds++] = (struct pollfd){in_pipe[1], POLLOUT, 0}; }
        if (out_open) { out_idx = nfds; fds[nfds++] = (struct pollfd){out_pipe[0], POLLIN | POLLHUP, 0}; }
        if (err_open) { err_idx = nfds; fds[nfds++] = (struct pollfd){err_pipe[0], POLLIN | POLLHUP, 0}; }
        (void)poll(fds, (nfds_t)nfds, 10);
        if (in_open && (stdin_off >= stdin_len || (in_idx >= 0 && (fds[in_idx].revents & (POLLERR | POLLHUP))))) {
            close(in_pipe[1]); in_open = 0;
        } else if (in_idx >= 0 && (fds[in_idx].revents & POLLOUT)) {
            ssize_t n = write(in_pipe[1], stdin_text + stdin_off, stdin_len - stdin_off);
            if (n > 0) stdin_off += (size_t)n;
        }
        int read_fds[2] = {out_pipe[0], err_pipe[0]};
        int *open_flags[2] = {&out_open, &err_open};
        char *buffers[2] = {out, err};
        size_t *lengths[2] = {&out_len, &err_len};
        int indexes[2] = {out_idx, err_idx};
        for (int s = 0; s < 2; s++) if (*open_flags[s] && indexes[s] >= 0 && fds[indexes[s]].revents) {
            char chunk[4096];
            for (;;) {
                ssize_t n = read(read_fds[s], chunk, sizeof(chunk));
                if (n > 0) {
                    size_t remaining = (size_t)max_output - out_len - err_len;
                    size_t take = (size_t)n < remaining ? (size_t)n : remaining;
                    if (take) { memcpy(buffers[s] + *lengths[s], chunk, take); *lengths[s] += take; }
                    if (take < (size_t)n || out_len + err_len >= (size_t)max_output) {
                        output_limit = 1; kill(pid, SIGKILL);
                    }
                } else if (n == 0) {
                    close(read_fds[s]); *open_flags[s] = 0; break;
                } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    close(read_fds[s]); *open_flags[s] = 0; break;
                } else break;
            }
        }
        if (!child_done) {
            pid_t waited = waitpid(pid, &wait_status, WNOHANG);
            if (waited == pid) child_done = 1;
        }
    }
    if (in_open) close(in_pipe[1]);
    if (!child_done) (void)waitpid(pid, &wait_status, 0);
    out[out_len] = 0; err[err_len] = 0;
    const char *status = "ferdig", *error = "";
    int exit_code = 0;
    if (memory_limited) { status = "feil"; error = "memory limit"; exit_code = 126; }
    else if (timed_out) { status = "timeout"; error = "timeout"; exit_code = 124; }
    else if (output_limit) { status = "feil"; error = "output limit"; exit_code = 125; }
    else if (WIFSIGNALED(wait_status)) { status = "signal"; exit_code = 128 + WTERMSIG(wait_status); }
    else if (WIFEXITED(wait_status)) exit_code = WEXITSTATUS(wait_status);
    NcVal *result = nc_process_result(status, pid, exit_code, out, err, error);
    nc_index_set(result,nc_str("sandbox"),nc_str(sandbox_profile));
#if defined(__linux__)
    nc_index_set(result,nc_str("sandbox_backend"),nc_str(strcmp(sandbox_profile,"none") ? "seccomp" : "none"));
#elif defined(__APPLE__)
    nc_index_set(result,nc_str("sandbox_backend"),nc_str(strcmp(sandbox_profile,"none") ? "seatbelt" : "none"));
#else
    nc_index_set(result,nc_str("sandbox_backend"),nc_str(strcmp(sandbox_profile,"none") ? "rlimit" : "none"));
#endif
    free(out); free(err);
    return result;
#endif
}

static int nc_atomic_handle_raw(const char *raw) {
    long id=0;unsigned long long generation=0;char trailing='\0';
    if(!raw||sscanf(raw,"atomic:%ld:%llu%c",&id,&generation,&trailing)!=2||id<=0||id>=NC_ATOMIC_MAX||generation==0)return -1;
    if(atomic_load_explicit(&g_atomic_cells[id].active,memory_order_acquire)!=1||atomic_load_explicit(&g_atomic_cells[id].generation,memory_order_acquire)!=generation)return -1;
    return (int)id;
}
static int nc_atomic_handle(NcVal *request){return nc_atomic_handle_raw(nc_atomic_text_field(request,"handle",""));}

static NcVal *nc_builtin_atomic_operation(NcVal *request) {
    if (!request || request->type != NC_MAP) return nc_atomic_result("invalid");
    const char *op = nc_atomic_text_field(request, "operation", "");
    const char *order_name = nc_atomic_text_field(request, "order", "seq_cst");
    int order_ok = 0;
    memory_order order = nc_atomic_order(order_name, &order_ok);
    if (!order_ok) return nc_atomic_result("invalid_order");

    if (!strcmp(op, "create")) {
        for (int id = 1; id < NC_ATOMIC_MAX; id++) {
            int expected=0;if(atomic_compare_exchange_strong_explicit(&g_atomic_cells[id].active,&expected,2,memory_order_acq_rel,memory_order_relaxed)) {
                atomic_init(&g_atomic_cells[id].value, nc_atomic_int_field(request, "value", 0));
                atomic_init(&g_atomic_cells[id].version, 0);
                unsigned long long generation=atomic_fetch_add_explicit(&g_atomic_cells[id].generation,1,memory_order_relaxed)+1;if(!generation){generation=atomic_fetch_add_explicit(&g_atomic_cells[id].generation,1,memory_order_relaxed)+1;}
                atomic_store_explicit(&g_atomic_cells[id].active,1,memory_order_release);
                NcVal *r = nc_atomic_result("ok");
                char handle[64]; snprintf(handle, sizeof(handle), "atomic:%d:%llu", id,generation);
                nc_index_set(r, nc_str("handle"), nc_str(handle));
                nc_index_set(r, nc_str("backend"), nc_str("native-c11"));
                return r;
            }
        }
        return nc_atomic_result("capacity");
    }
    if (!strcmp(op, "fence")) {
        atomic_thread_fence(order);
        return nc_atomic_result("ok");
    }

    int id = nc_atomic_handle(request);
    if (id < 0) return nc_atomic_result("invalid_handle");
    NcAtomicCell *cell = &g_atomic_cells[id];
    NcVal *r = nc_atomic_result("ok");
    long long observed = 0;

    if (!strcmp(op, "load")) {
        if (order == memory_order_release || order == memory_order_acq_rel)
            return nc_atomic_result("invalid_order");
        observed = atomic_load_explicit(&cell->value, order);
    } else if (!strcmp(op, "store")) {
        if (order == memory_order_acquire || order == memory_order_acq_rel)
            return nc_atomic_result("invalid_order");
        atomic_store_explicit(&cell->value, nc_atomic_int_field(request, "value", 0), order);
        atomic_fetch_add_explicit(&cell->version, 1, memory_order_relaxed);
        observed = atomic_load_explicit(&cell->value, memory_order_relaxed);
    } else if (!strcmp(op, "exchange")) {
        observed = atomic_exchange_explicit(&cell->value, nc_atomic_int_field(request, "value", 0), order);
        atomic_fetch_add_explicit(&cell->version, 1, memory_order_relaxed);
    } else if (!strcmp(op, "compare_exchange")) {
        long long expected = nc_atomic_int_field(request, "expected", 0);
        long long replacement = nc_atomic_int_field(request, "replacement", 0);
        const char *failure_name = nc_atomic_text_field(request, "failure_order", "acquire");
        int failure_ok = 0;
        memory_order failure = nc_atomic_order(failure_name, &failure_ok);
        if (!failure_ok || !nc_atomic_failure_allowed(order, failure))
            return nc_atomic_result("invalid_order");
        int success = atomic_compare_exchange_strong_explicit(&cell->value, &expected, replacement, order, failure);
        observed = expected;
        nc_index_set(r, nc_str("success"), nc_str(success ? "sann" : "usann"));
        if (success) atomic_fetch_add_explicit(&cell->version, 1, memory_order_relaxed);
    } else if (!strcmp(op, "fetch_add")) {
        observed = atomic_fetch_add_explicit(&cell->value, nc_atomic_int_field(request, "delta", 0), order);
        atomic_fetch_add_explicit(&cell->version, 1, memory_order_relaxed);
    } else if (!strcmp(op, "destroy")) {
        atomic_store_explicit(&cell->active,0,memory_order_release);
        return r;
    } else {
        return nc_atomic_result("unsupported");
    }

    nc_index_set(r, nc_str("observed"), nc_int(observed));
    nc_index_set(r, nc_str("value"), nc_int(atomic_load_explicit(&cell->value, memory_order_relaxed)));
    nc_index_set(r, nc_str("version"), nc_int((long long)atomic_load_explicit(&cell->version, memory_order_relaxed)));
    return r;
}

static _Atomic unsigned long long g_native_vm_instructions = 0;
static _Atomic unsigned long long g_native_vm_calls = 0;
static _Atomic unsigned long long g_native_vm_builtin_calls = 0;
static _Atomic unsigned long long g_native_vm_filesystem_calls = 0;
static _Atomic unsigned long long g_native_vm_io_calls = 0;
static _Atomic unsigned long long g_native_security_allowed = 0;
static _Atomic unsigned long long g_native_security_denied = 0;
static _Thread_local char g_native_last_security_denial[512];
static _Thread_local int g_native_app_security_active = 0;
static void nc_metric_set_u64(NcVal *map, const char *key, unsigned long long value);

#define NC_NATIVE_DEBUG_MAX_BREAKPOINTS 128
#define NC_NATIVE_DEBUG_MAX_WATCHES 128
typedef struct { char function[256]; int ip; int active; } NcNativeBreakpoint;
static pthread_mutex_t g_native_debug_mutex = PTHREAD_MUTEX_INITIALIZER;
static NcNativeBreakpoint g_native_breakpoints[NC_NATIVE_DEBUG_MAX_BREAKPOINTS];
static char g_native_debug_watches[NC_NATIVE_DEBUG_MAX_WATCHES][128];
static int g_native_debug_enabled = 0;
static int g_native_debug_step_pending = 0;
static int g_native_debug_suspend_on_hit = 0;
static unsigned long long g_native_debug_hit_count = 0;
static char g_native_debug_last_function[256];
static char g_native_debug_last_opcode[64];
static char g_native_debug_last_variables[2048];
static int g_native_debug_last_ip = -1;
static int g_native_debug_last_depth = 0;

static int nc_native_debug_breakpoint_matches(const char *function, int ip) {
    for (int i = 0; i < NC_NATIVE_DEBUG_MAX_BREAKPOINTS; i++)
        if (g_native_breakpoints[i].active && g_native_breakpoints[i].ip == ip &&
            !strcmp(g_native_breakpoints[i].function, function)) return 1;
    return 0;
}

static void nc_native_debug_observe(const char *function, int ip, const char *opcode,
                                    char **varnames, int nvars, int depth) {
    pthread_mutex_lock(&g_native_debug_mutex);
    int hit = g_native_debug_enabled &&
        (g_native_debug_step_pending || nc_native_debug_breakpoint_matches(function, ip));
    if (hit) {
        g_native_debug_step_pending = 0;
        g_native_debug_hit_count++;
        snprintf(g_native_debug_last_function, sizeof(g_native_debug_last_function), "%s", function);
        snprintf(g_native_debug_last_opcode, sizeof(g_native_debug_last_opcode), "%s", opcode);
        g_native_debug_last_ip = ip;
        g_native_debug_last_depth = depth + 1;
        g_native_debug_last_variables[0] = '\0';
        for (int i = 0; i < nvars; i++) {
            size_t used = strlen(g_native_debug_last_variables);
            if (used + strlen(varnames[i]) + 2 >= sizeof(g_native_debug_last_variables)) break;
            if (used) strcat(g_native_debug_last_variables, ",");
            strcat(g_native_debug_last_variables, varnames[i]);
        }
    }
    pthread_mutex_unlock(&g_native_debug_mutex);
}

static NcVal *nc_builtin_vm_debug_enable_native(void) {
    pthread_mutex_lock(&g_native_debug_mutex); g_native_debug_enabled = 1; pthread_mutex_unlock(&g_native_debug_mutex);
    return nc_bool(1);
}
static NcVal *nc_builtin_vm_debug_disable_native(void) {
    pthread_mutex_lock(&g_native_debug_mutex); g_native_debug_enabled = 0; g_native_debug_step_pending = 0; pthread_mutex_unlock(&g_native_debug_mutex);
    return nc_bool(1);
}
static NcVal *nc_builtin_vm_debug_breakpoint_add_native(NcVal **args, int nargs) {
    if (nargs < 2 || !args[0] || args[0]->type != NC_STR || !args[1] || args[1]->type != NC_INT) return nc_bool(0);
    int result = 0;
    pthread_mutex_lock(&g_native_debug_mutex);
    if (nc_native_debug_breakpoint_matches(args[0]->s, (int)args[1]->i)) result = 1;
    else for (int i = 0; i < NC_NATIVE_DEBUG_MAX_BREAKPOINTS; i++) if (!g_native_breakpoints[i].active) {
        g_native_breakpoints[i].active = 1;
        g_native_breakpoints[i].ip = (int)args[1]->i;
        snprintf(g_native_breakpoints[i].function, sizeof(g_native_breakpoints[i].function), "%s", args[0]->s);
        result = 1; break;
    }
    pthread_mutex_unlock(&g_native_debug_mutex);
    return nc_bool(result);
}
static NcVal *nc_builtin_vm_debug_breakpoint_remove_native(NcVal **args, int nargs) {
    int result = 0;
    if (nargs >= 2 && args[0] && args[0]->type == NC_STR && args[1] && args[1]->type == NC_INT) {
        pthread_mutex_lock(&g_native_debug_mutex);
        for (int i = 0; i < NC_NATIVE_DEBUG_MAX_BREAKPOINTS; i++) if (g_native_breakpoints[i].active &&
            g_native_breakpoints[i].ip == (int)args[1]->i && !strcmp(g_native_breakpoints[i].function, args[0]->s)) {
            g_native_breakpoints[i].active = 0; result = 1; break;
        }
        pthread_mutex_unlock(&g_native_debug_mutex);
    }
    return nc_bool(result);
}
static NcVal *nc_builtin_vm_debug_breakpoints_clear_native(void) {
    int count = 0; pthread_mutex_lock(&g_native_debug_mutex);
    for (int i = 0; i < NC_NATIVE_DEBUG_MAX_BREAKPOINTS; i++) if (g_native_breakpoints[i].active) { g_native_breakpoints[i].active = 0; count++; }
    pthread_mutex_unlock(&g_native_debug_mutex); return nc_int(count);
}
static NcVal *nc_builtin_vm_debug_watch_native(NcVal **args, int nargs, int add) {
    if (nargs < 1 || !args[0] || args[0]->type != NC_STR) return nc_bool(0);
    int result = 0; pthread_mutex_lock(&g_native_debug_mutex);
    for (int i = 0; i < NC_NATIVE_DEBUG_MAX_WATCHES; i++) if (g_native_debug_watches[i][0] && !strcmp(g_native_debug_watches[i], args[0]->s)) {
        if (!add) g_native_debug_watches[i][0] = '\0'; result = 1; goto done_watch;
    }
    if (add) for (int i = 0; i < NC_NATIVE_DEBUG_MAX_WATCHES; i++) if (!g_native_debug_watches[i][0]) {
        snprintf(g_native_debug_watches[i], sizeof(g_native_debug_watches[i]), "%s", args[0]->s); result = 1; break;
    }
done_watch: pthread_mutex_unlock(&g_native_debug_mutex); return nc_bool(result);
}
static NcVal *nc_builtin_vm_debug_watches_clear_native(void) {
    int count = 0; pthread_mutex_lock(&g_native_debug_mutex);
    for (int i = 0; i < NC_NATIVE_DEBUG_MAX_WATCHES; i++) if (g_native_debug_watches[i][0]) { g_native_debug_watches[i][0] = '\0'; count++; }
    pthread_mutex_unlock(&g_native_debug_mutex); return nc_int(count);
}
static NcVal *nc_builtin_vm_debug_snapshot_native(void) {
    char function[256], opcode[64], variables[2048]; int ip, depth, enabled; unsigned long long hits;
    pthread_mutex_lock(&g_native_debug_mutex);
    snprintf(function, sizeof(function), "%s", g_native_debug_last_function);
    snprintf(opcode, sizeof(opcode), "%s", g_native_debug_last_opcode);
    snprintf(variables, sizeof(variables), "%s", g_native_debug_last_variables);
    ip = g_native_debug_last_ip; depth = g_native_debug_last_depth; enabled = g_native_debug_enabled; hits = g_native_debug_hit_count;
    pthread_mutex_unlock(&g_native_debug_mutex);
    NcVal *snapshot = nc_map_new();
    nc_index_set(snapshot, nc_str("enabled"), nc_str(enabled ? "sann" : "usann"));
    nc_index_set(snapshot, nc_str("last_function"), nc_str(function));
    nc_index_set(snapshot, nc_str("last_opcode"), nc_str(opcode));
    nc_index_set(snapshot, nc_str("last_variables"), nc_str(variables));
    nc_index_set(snapshot, nc_str("last_frame_state"), nc_str("running"));
    nc_metric_set_u64(snapshot, "last_ip", (unsigned long long)(ip < 0 ? 0 : ip));
    nc_metric_set_u64(snapshot, "last_frame_depth", (unsigned long long)depth);
    nc_metric_set_u64(snapshot, "hit_count", hits);
    return snapshot;
}

static int nc_csv_has_token(const char *csv, const char *token) {
    if (!csv || !token || !token[0]) return 0;
    size_t token_len = strlen(token);
    const char *cursor = csv;
    while (*cursor) {
        while (*cursor == ' ' || *cursor == ',') cursor++;
        const char *end = cursor;
        while (*end && *end != ',') end++;
        const char *trimmed_end = end;
        while (trimmed_end > cursor && trimmed_end[-1] == ' ') trimmed_end--;
        if ((size_t)(trimmed_end - cursor) == token_len && !strncmp(cursor, token, token_len)) return 1;
        cursor = end;
    }
    return 0;
}

static int nc_native_disk_scope_allows(const char *path) {
    if (!path || !path[0] || strstr(path, "..")) return 0;
    const char *scope = getenv("NORSCODE_VM_DISK_ROOT");
    if (!scope || !scope[0] || !strcmp(scope, ".")) return path[0] != '/';
    const char *cursor = scope;
    while (*cursor) {
        while (*cursor == ' ' || *cursor == ',') cursor++;
        const char *end = cursor;
        while (*end && *end != ',') end++;
        size_t len = (size_t)(end - cursor);
        while (len && cursor[len - 1] == ' ') len--;
        if (len == 1 && cursor[0] == '.' && path[0] != '/') return 1;
        if (len && !strncmp(path, cursor, len) && (path[len] == '\0' || path[len] == '/')) return 1;
        cursor = end;
    }
    return 0;
}

static const char *nc_native_capability_for(const char *name) {
    if (!strcmp(name, "exec_prosess") || !strcmp(name, "process_spawn_argv") ||
        !strcmp(name, "process_operation") || !strcmp(name, "kompiler_fil")) return "process.exec";
    if (!strncmp(name, "thread_", 7) || !strcmp(name, "atomic_operation")) return "thread.spawn";
    if (!strcmp(name, "jit_operation") || !strcmp(name, "vm_jit_cleanup")) return "jit.execute";
    if (!strcmp(name, "miljo_hent")) return "env.read";
    if (!strcmp(name, "miljo_sett")) return "env.write";
    if (!strcmp(name, "fil_les") || !strcmp(name, "fil_les_binary") || !strcmp(name, "fil_les_binær") ||
        !strcmp(name, "fil_finnes") || !strcmp(name, "filesystem_read_operation")) return "disk.read";
    if (!strcmp(name, "fil_skriv") || !strcmp(name, "fil_slett") || !strcmp(name, "fil_append") ||
        !strcmp(name, "fil_skriv_binary") || !strcmp(name, "fil_skriv_binær") ||
        !strcmp(name, "mappe_opprett") || !strcmp(name, "mkdir") || !strcmp(name, "mkdir_p") ||
        !strcmp(name, "filesystem_write_operation")) return "disk.write";
    if (strstr(name, "socket_") || !strcmp(name, "network_operation")) return "net.tcp";
    if (!strcmp(name, "http_get") || !strcmp(name, "https_get")) return "net.http";
    if (!strcmp(name, "dns_lookup") || !strcmp(name, "resolve_host")) return "net.dns";
    return NULL;
}

static NcVal *nc_builtin_dns_lookup(NcVal *host_v, NcVal *service_v) {
    const char *host = (host_v && host_v->type == NC_STR) ? host_v->s : "";
    const char *service = (service_v && service_v->type == NC_STR) ? service_v->s : "0";
    NcVal *result = nc_map_new();
    NcVal *addresses = nc_list_new();
    nc_index_set(result, nc_str("abi"), nc_str("norscode-dns-v1"));
    nc_index_set(result, nc_str("host"), nc_str(host));
    nc_index_set(result, nc_str("service"), nc_str(service));
    nc_index_set(result, nc_str("addresses"), addresses);
    nc_index_set(result, nc_str("status"), nc_str("feil"));
    nc_index_set(result, nc_str("error"), nc_str(""));
    if (!host[0]) {
        nc_index_set(result, nc_str("error"), nc_str("host manglar"));
        return result;
    }
#if defined(_WIN32)
    (void)service;
    nc_index_set(result, nc_str("error"), nc_str("Windows DNS backend krev aktiv Winsock-promotering"));
    return result;
#else
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *list = NULL;
    int rc = getaddrinfo(host, service[0] ? service : NULL, &hints, &list);
    if (rc != 0) {
        nc_index_set(result, nc_str("error"), nc_str(gai_strerror(rc)));
        return result;
    }
    char numeric[INET6_ADDRSTRLEN];
    int count = 0;
    for (struct addrinfo *it = list; it && count < 32; it = it->ai_next) {
        if (getnameinfo(it->ai_addr, it->ai_addrlen, numeric, sizeof(numeric), NULL, 0, NI_NUMERICHOST) == 0) {
            nc_list_append_raw(addresses, nc_str(numeric));
            count++;
        }
    }
    freeaddrinfo(list);
    if (count == 0) {
        nc_index_set(result, nc_str("error"), nc_str("ingen numerisk adresse"));
        return result;
    }
    nc_index_set(result, nc_str("status"), nc_str("ok"));
    nc_index_set(result, nc_str("count"), nc_int(count));
    return result;
#endif
}

static const char *nc_native_security_check(const char *name, NcVal **args, int nargs) {
    if (!g_native_app_security_active) return NULL;
    if (!strcmp(name, "miljo_hent") && nargs > 0 && args[0] && args[0]->type == NC_STR) {
        const char *key = args[0]->s;
        if (!strncmp(key, "NORSCODE_VM_", 12) || !strcmp(key, "NORSCODE_PURE_NCB_FILE") ||
            !strcmp(key, "NORSCODE_NCB_FILE") || !strcmp(key, "NORSCODE_ROOT")) return NULL;
    }
    if (!strcmp(name, "fil_les") && nargs > 0 && args[0] && args[0]->type == NC_STR) {
        const char *trusted_ncb = getenv("NORSCODE_NCB_FILE");
        const char *trusted_pure_ncb = getenv("NORSCODE_PURE_NCB_FILE");
        if ((trusted_ncb && !strcmp(args[0]->s, trusted_ncb)) ||
            (trusted_pure_ncb && !strcmp(args[0]->s, trusted_pure_ncb))) return NULL;
    }
    const char *capability = nc_native_capability_for(name);
    if (!capability) return NULL;
    const char *caps = getenv("NORSCODE_VM_CAPABILITIES");
    if (!nc_csv_has_token(caps, capability)) {
        snprintf(g_native_last_security_denial, sizeof(g_native_last_security_denial),
                 "%s:denied", capability);
        return g_native_last_security_denial;
    }
    if ((!strcmp(capability, "disk.read") || !strcmp(capability, "disk.write")) &&
        nargs > 0 && args[0] && args[0]->type == NC_STR && !nc_native_disk_scope_allows(args[0]->s)) {
        snprintf(g_native_last_security_denial, sizeof(g_native_last_security_denial),
                 "%s:path outside disk scope", capability);
        return g_native_last_security_denial;
    }
    return NULL;
}

static void nc_metric_set_u64(NcVal *map, const char *key, unsigned long long value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%llu", value);
    nc_index_set(map, nc_str(key), nc_str(buffer));
}

static NcVal *nc_builtin_vm_memory_status_native(void) {
    size_t allocated, arena_reused, collections, minor_collections;
    size_t major_collections, relocated, arena_pages, edges, threshold;
    unsigned long long compactions;
    pthread_mutex_lock(&g_gc_mutex);
    allocated = g_gc_allocated;
    arena_reused = g_gc_arena_reused;
    collections = g_gc_collections;
    minor_collections = g_gc_minor_collections;
    major_collections = g_gc_major_collections;
    relocated = g_gc_relocated;
    arena_pages = g_gc_arena_page_count;
    edges = g_gc_edge_count;
    threshold = g_gc_threshold;
    compactions = atomic_load_explicit(&g_gc_relocation_epoch, memory_order_relaxed);
    pthread_mutex_unlock(&g_gc_mutex);

    NcVal *status = nc_map_new();
    nc_metric_set_u64(status, "allocated", (unsigned long long)allocated);
    nc_metric_set_u64(status, "alive", (unsigned long long)allocated);
    nc_metric_set_u64(status, "freed", (unsigned long long)arena_reused);
    nc_metric_set_u64(status, "collections", (unsigned long long)collections);
    nc_metric_set_u64(status, "minor_collections", (unsigned long long)minor_collections);
    nc_metric_set_u64(status, "major_collections", (unsigned long long)major_collections);
    nc_metric_set_u64(status, "compactions", compactions);
    nc_metric_set_u64(status, "relocated", (unsigned long long)relocated);
    nc_metric_set_u64(status, "arena_pages", (unsigned long long)arena_pages);
    nc_metric_set_u64(status, "edges", (unsigned long long)edges);
    nc_metric_set_u64(status, "threshold", (unsigned long long)threshold);
    nc_index_set(status, nc_str("backend"), nc_str("native-runtime-owned"));
    return status;
}

static NcVal *nc_builtin_vm_runtime_metrics_native(void) {
    NcVal *metrics = nc_builtin_vm_memory_status_native();
    unsigned long long instructions = atomic_load_explicit(&g_native_vm_instructions, memory_order_relaxed);
    nc_metric_set_u64(metrics, "instructions", instructions);
    nc_metric_set_u64(metrics, "calls", atomic_load_explicit(&g_native_vm_calls, memory_order_relaxed));
    nc_metric_set_u64(metrics, "builtin_calls", atomic_load_explicit(&g_native_vm_builtin_calls, memory_order_relaxed));
    nc_metric_set_u64(metrics, "filesystem_calls", atomic_load_explicit(&g_native_vm_filesystem_calls, memory_order_relaxed));
    nc_metric_set_u64(metrics, "io_calls", atomic_load_explicit(&g_native_vm_io_calls, memory_order_relaxed));
    nc_metric_set_u64(metrics, "gc_safe_points", instructions / 64U);
    nc_metric_set_u64(metrics, "gc_collections", (unsigned long long)g_gc_collections);
    nc_metric_set_u64(metrics, "gc_pause_ms", 0);
    nc_metric_set_u64(metrics, "wall_ms", 0);
    nc_metric_set_u64(metrics, "instructions_per_ms", instructions);
    nc_metric_set_u64(metrics, "heap_alive_bytes", (unsigned long long)g_gc_allocated * sizeof(NcVal));
    nc_metric_set_u64(metrics, "security_denied", atomic_load_explicit(&g_native_security_denied, memory_order_relaxed));
    nc_metric_set_u64(metrics, "security_allowed", atomic_load_explicit(&g_native_security_allowed, memory_order_relaxed));
    nc_index_set(metrics, nc_str("last_security_denial"), nc_str(g_native_last_security_denial));
    return metrics;
}
static NcVal *nc_builtin_json_parse_raw(NcVal *v);
static NcVal *nc_builtin_json_parse_str(NcVal *v);
static NcVal *nc_builtin_json_parse_norscode(NcVal *v);
static NcVal *nc_builtin_json_stringify(NcVal *v);
static NcVal *nc_builtin_json_stringify_smart(NcVal *v);
static void nc_merge_fns(NcVal *dst, NcVal *src);
static void nc_merge_imports_from_source(NcVal *bundle, const char *src_text);
static char *nc_module_to_path(const char *modul);
static void nc_list_append_raw(NcVal *lst, NcVal *v);

typedef int32_t NcTensorVec4 __attribute__((vector_size(16)));

static NcVal *nc_tensor_result(const char *status, const char *error) {
    NcVal *result = nc_map_new();
    nc_index_set(result, nc_str("abi"), nc_str("norscode-tensor-v1"));
    nc_index_set(result, nc_str("status"), nc_str(status));
    nc_index_set(result, nc_str("error"), nc_str(error ? error : ""));
    return result;
}

static int nc_tensor_simd_available(void) {
#if defined(__aarch64__)
    return 1;
#elif defined(__x86_64__) && (defined(__clang__) || defined(__GNUC__)) && !defined(_MSC_VER)
    unsigned int eax = 1, ebx = 0, ecx = 0, edx = 0;
    __asm__ volatile("cpuid" : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
    return (ecx & (1u << 19)) != 0;
#else
    return 0;
#endif
}

static const char *nc_tensor_backend(void) {
#if defined(__aarch64__)
    return "simd-arm64-128";
#elif defined(__x86_64__)
    return nc_tensor_simd_available() ? "simd-x86_64-sse4.1" : "scalar";
#else
    return "scalar";
#endif
}

#if defined(__x86_64__) && (defined(__clang__) || defined(__GNUC__))
__attribute__((target("sse4.1")))
#endif
static long long nc_tensor_dot_simd(NcVal *left, NcVal *right,
                                    int left_base, int column,
                                    int inner, int columns) {
    long long sum = 0;
    int k = 0;
    for (; k + 4 <= inner; k += 4) {
        NcTensorVec4 a = {
            (int32_t)left->list->items[left_base + k]->i,
            (int32_t)left->list->items[left_base + k + 1]->i,
            (int32_t)left->list->items[left_base + k + 2]->i,
            (int32_t)left->list->items[left_base + k + 3]->i
        };
        NcTensorVec4 b = {
            (int32_t)right->list->items[(k * columns) + column]->i,
            (int32_t)right->list->items[((k + 1) * columns) + column]->i,
            (int32_t)right->list->items[((k + 2) * columns) + column]->i,
            (int32_t)right->list->items[((k + 3) * columns) + column]->i
        };
        NcTensorVec4 product = a * b;
        sum += (long long)product[0] + product[1] + product[2] + product[3];
    }
    for (; k < inner; k++)
        sum += left->list->items[left_base + k]->i * right->list->items[(k * columns) + column]->i;
    return sum;
}

static long long nc_tensor_dot_scalar(NcVal *left, NcVal *right,
                                      int left_base, int column,
                                      int inner, int columns) {
    long long sum = 0;
    for (int k = 0; k < inner; k++)
        sum += left->list->items[left_base + k]->i * right->list->items[(k * columns) + column]->i;
    return sum;
}

static NcVal *nc_builtin_tensor_operation(NcVal *request) {
    if (!request || request->type != NC_MAP) return nc_tensor_result("feil", "request must be a map");
    if (strcmp(nc_atomic_text_field(request, "abi", ""), "norscode-tensor-v1"))
        return nc_tensor_result("feil", "invalid ABI");
    const char *operation = nc_atomic_text_field(request, "operation", "");
    if (!strcmp(operation, "status")) {
        NcVal *result = nc_tensor_result("ferdig", "");
        nc_index_set(result, nc_str("backend"), nc_str(nc_tensor_backend()));
        nc_index_set(result, nc_str("vector_lanes"), nc_int(nc_tensor_simd_available() ? 4 : 1));
        nc_index_set(result, nc_str("metal_available"), nc_bool(nc_metal_available()));
        return result;
    }
    if (strcmp(operation, "matmul_i32")) return nc_tensor_result("feil", "unsupported tensor operation");
    long long rows_ll = nc_atomic_int_field(request, "rows", -1);
    long long inner_ll = nc_atomic_int_field(request, "inner", -1);
    long long columns_ll = nc_atomic_int_field(request, "columns", -1);
    if (rows_ll <= 0 || inner_ll <= 0 || columns_ll <= 0 ||
        rows_ll > 4096 || inner_ll > 4096 || columns_ll > 4096 ||
        rows_ll * columns_ll > 1000000 || rows_ll * inner_ll * columns_ll > 20000000)
        return nc_tensor_result("feil", "tensor limits exceeded");
    int rows = (int)rows_ll, inner = (int)inner_ll, columns = (int)columns_ll;
    NcVal *left = nc_index_get(request, nc_str("left"));
    NcVal *right = nc_index_get(request, nc_str("right"));
    if (!left || left->type != NC_LIST || !right || right->type != NC_LIST ||
        left->list->len != rows * inner || right->list->len != inner * columns)
        return nc_tensor_result("feil", "tensor shape mismatch");
    for (int i = 0; i < left->list->len; i++) {
        NcVal *value = left->list->items[i];
        if (!value || value->type != NC_INT || value->i < -32768 || value->i > 32767)
            return nc_tensor_result("feil", "left value outside int16 range");
    }
    for (int i = 0; i < right->list->len; i++) {
        NcVal *value = right->list->items[i];
        if (!value || value->type != NC_INT || value->i < -32768 || value->i > 32767)
            return nc_tensor_result("feil", "right value outside int16 range");
    }
    const char *requested_backend = nc_atomic_text_field(request, "backend", "");
    if (!strcmp(requested_backend, "metal")) {
        size_t left_bytes = (size_t)rows * (size_t)inner * sizeof(int32_t);
        size_t right_bytes = (size_t)inner * (size_t)columns * sizeof(int32_t);
        size_t output_bytes = (size_t)rows * (size_t)columns * sizeof(int32_t);
        int32_t *left_values = (int32_t *)malloc(left_bytes);
        int32_t *right_values = (int32_t *)malloc(right_bytes);
        int32_t *output_values = (int32_t *)malloc(output_bytes);
        if (!left_values || !right_values || !output_values) {
            free(left_values); free(right_values); free(output_values);
            return nc_tensor_result("feil", "metal tensor allocation failed");
        }
        for (int i = 0; i < rows * inner; i++) left_values[i] = (int32_t)left->list->items[i]->i;
        for (int i = 0; i < inner * columns; i++) right_values[i] = (int32_t)right->list->items[i]->i;
        int metal_status = nc_metal_matmul_i32(left_values, right_values, output_values, rows, inner, columns);
        if (metal_status != 0) {
            free(left_values); free(right_values); free(output_values);
            return nc_tensor_result("feil", "metal backend unavailable");
        }
        NcVal *metal_data = nc_list_new();
        for (int i = 0; i < rows * columns; i++) nc_list_append_raw(metal_data, nc_int(output_values[i]));
        free(left_values); free(right_values); free(output_values);
        NcVal *metal_result = nc_tensor_result("ferdig", "");
        nc_index_set(metal_result, nc_str("data"), metal_data);
        nc_index_set(metal_result, nc_str("rows"), nc_int(rows));
        nc_index_set(metal_result, nc_str("columns"), nc_int(columns));
        nc_index_set(metal_result, nc_str("backend"), nc_str("metal"));
        return metal_result;
    }
    int use_simd = nc_tensor_simd_available();
    NcVal *data = nc_list_new();
    for (int row = 0; row < rows; row++) {
        for (int column = 0; column < columns; column++) {
            long long value = use_simd
                ? nc_tensor_dot_simd(left, right, row * inner, column, inner, columns)
                : nc_tensor_dot_scalar(left, right, row * inner, column, inner, columns);
            nc_list_append_raw(data, nc_int(value));
        }
    }
    NcVal *result = nc_tensor_result("ferdig", "");
    nc_index_set(result, nc_str("data"), data);
    nc_index_set(result, nc_str("rows"), nc_int(rows));
    nc_index_set(result, nc_str("columns"), nc_int(columns));
    nc_index_set(result, nc_str("backend"), nc_str(nc_tensor_backend()));
    return result;
}

static int nc_media_diffusion_clamp(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

static int nc_media_diffusion_noise(int seed, int x, int y, int channel, int round) {
    long long n = seed;
    n = (n + (long long)x * 374761393LL) % 2147483647LL;
    n = (n + (long long)y * 668265263LL) % 2147483647LL;
    n = (n + (long long)channel * 214013LL) % 2147483647LL;
    n = (n + (long long)round * 122949829LL) % 2147483647LL;
    return (int)(n % 511LL) - 255;
}

static NcVal *nc_builtin_media_diffusion_operation(NcVal *request) {
    if (!request || request->type != NC_MAP) return nc_tensor_result("feil", "request must be a map");
    if (strcmp(nc_atomic_text_field(request, "abi", ""), "norscode-media-diffusion-v1"))
        return nc_tensor_result("feil", "invalid ABI");
    const char *operation = nc_atomic_text_field(request, "operation", "");
    if (!strcmp(operation, "status")) {
        NcVal *result = nc_tensor_result("ferdig", "");
        nc_index_set(result, nc_str("backend"), nc_str("metal-diffusion-v1"));
        nc_index_set(result, nc_str("metal_available"), nc_bool(nc_metal_available()));
        return result;
    }
    if (strcmp(operation, "render_rgb")) return nc_tensor_result("feil", "unsupported media diffusion operation");
    long long width_ll = nc_atomic_int_field(request, "width", -1);
    long long height_ll = nc_atomic_int_field(request, "height", -1);
    long long steps_ll = nc_atomic_int_field(request, "steps", -1);
    long long seed_ll = nc_atomic_int_field(request, "seed", 0);
    if (width_ll <= 0 || height_ll <= 0 || steps_ll <= 0 || steps_ll > 100 ||
        width_ll > 2048 || height_ll > 2048 || width_ll * height_ll > 4194304LL)
        return nc_tensor_result("feil", "media diffusion limits exceeded");
    int width = (int)width_ll, height = (int)height_ll, steps = (int)steps_ll;
    int seed = (int)(seed_ll % 2147483647LL);
    if (seed < 0) seed = -seed;
    size_t count = (size_t)width * (size_t)height * 3u;
    int32_t *initial = (int32_t *)malloc(count * sizeof(int32_t));
    int32_t *output = (int32_t *)malloc(count * sizeof(int32_t));
    if (!initial || !output) {
        free(initial); free(output);
        return nc_tensor_result("feil", "media diffusion allocation failed");
    }
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int channel = 0; channel < 3; channel++) {
                int index = (y * width + x) * 3 + channel;
                initial[index] = nc_media_diffusion_clamp(
                    128 + nc_media_diffusion_noise(seed, x, y, channel, 0) / 4);
            }
        }
    }
    int metal_status = nc_metal_diffusion_rgb(initial, output, width, height, steps, seed);
    free(initial);
    if (metal_status != 0) {
        free(output);
        return nc_tensor_result("feil", "metal diffusion backend unavailable");
    }
    NcVal *data = nc_list_new();
    for (size_t i = 0; i < count; i++) nc_list_append_raw(data, nc_int(nc_media_diffusion_clamp(output[i])));
    free(output);
    NcVal *result = nc_tensor_result("ferdig", "");
    nc_index_set(result, nc_str("data"), data);
    nc_index_set(result, nc_str("width"), nc_int(width));
    nc_index_set(result, nc_str("height"), nc_int(height));
    nc_index_set(result, nc_str("channels"), nc_int(3));
    nc_index_set(result, nc_str("backend"), nc_str("metal-diffusion-v1"));
    return result;
}

static NcVal *nc_try_stage0_compiler_bundle(const char *src_text, const char *modul);

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
    if (!strncmp(name, "selfhost.json.", 14)) return 1;
    if (!strncmp(name, "std.path.", 9)) return 1;
    if (!strncmp(name, "std.env.", 8)) return 1;
    if (!strcmp(name, "std.web.request_context") || !strcmp(name, "std.web.handle_request") ||
        !strcmp(name, "std.web.response_status") || !strcmp(name, "std.web.response_body") ||
        !strcmp(name, "std.web.response_header") || !strcmp(name, "std.web.response_header_or") ||
        !strcmp(name, "std.web.response_builder") || !strcmp(name, "std.web.response_error") ||
        !strcmp(name, "std.web.response_json") || !strcmp(name, "std.web.response_file")) return 1;
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

/* Positiv cache for repeterte kvalifiserte/fuzzy funksjonsoppslag. */
#define NC_FN_CACHE_SIZE 2048
typedef struct {
    NcVal *functions;
    NcVal *result;
    uint64_t name_hash;
    unsigned long long relocation_epoch;
    char *name;
} NcFnCacheEntry;
static _Thread_local NcFnCacheEntry g_fn_cache[NC_FN_CACHE_SIZE];

static NcVal *nc_fn_cache_get(NcVal *functions, const char *name, uint64_t hash) {
    size_t slot = ((size_t)hash ^ ((uintptr_t)functions >> 4)) & (NC_FN_CACHE_SIZE - 1);
    NcFnCacheEntry *entry = &g_fn_cache[slot];
    unsigned long long epoch = atomic_load_explicit(&g_gc_relocation_epoch, memory_order_acquire);
    if (entry->relocation_epoch == epoch && entry->functions == functions &&
        entry->name_hash == hash && entry->name &&
        !strcmp(entry->name, name)) return entry->result;
    return NULL;
}

static void nc_fn_cache_put(NcVal *functions, const char *name, uint64_t hash, NcVal *result) {
    if (!result) return;
    size_t slot = ((size_t)hash ^ ((uintptr_t)functions >> 4)) & (NC_FN_CACHE_SIZE - 1);
    NcFnCacheEntry *entry = &g_fn_cache[slot];
    free(entry->name);
    entry->functions = functions;
    entry->result = result;
    entry->name_hash = hash;
    entry->relocation_epoch = atomic_load_explicit(&g_gc_relocation_epoch, memory_order_acquire);
    entry->name = strdup(name);
}

/* Finn funksjon med fuzzy matching */
static NcVal *nc_exec_find_fn(NcVal *functions, const char *name) {
    uint64_t name_hash = nc_map_hash(name);
    NcVal *cached = nc_fn_cache_get(functions, name, name_hash);
    if (cached) return cached;
    /* Direkte treff */
    NcVal *r = nc_map_get_cstr(functions, name);
    if (r && r->type != NC_NIL) {
        nc_fn_cache_put(functions, name, name_hash, r);
        return r;
    }
    /* Strip "builtin." og prøv direkte */
    const char *qname = name;
    if (!strncmp(name, "builtin.", 8)) qname = name + 8;
    if (qname != name) {
        NcVal *r2 = nc_map_get_cstr(functions, qname);
        if (r2 && r2->type != NC_NIL) {
            nc_fn_cache_put(functions, name, name_hash, r2);
            return r2;
        }
        /* builtin.* er eit reservert navnerom. Manglande eksakte treff skal
         * direkte til host-dispatch, ikkje skanne heile funksjonstabellen. */
        return NULL;
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
                    if (fn_pfx == pfx_len && !strncmp(fn, qname, pfx_len)) {
                        nc_fn_cache_put(functions, name, name_hash, functions->map->vals[i]);
                        return functions->map->vals[i];
                    }
                }
                if (!fuzzy_match) fuzzy_match = functions->map->vals[i];
            }
        }
    }
    nc_fn_cache_put(functions, name, name_hash, fuzzy_match);
    return fuzzy_match;
}

static NcVal *nc_dispatch_call(const char *n, NcVal **a, int na) {
    atomic_fetch_add_explicit(&g_native_vm_builtin_calls, 1, memory_order_relaxed);
    if (strstr(n, "fil_les") || strstr(n, "fil_skriv") || strstr(n, "filesystem_")) {
        atomic_fetch_add_explicit(&g_native_vm_filesystem_calls, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_native_vm_io_calls, 1, memory_order_relaxed);
    } else if (strstr(n, "socket") || strstr(n, "network_") || strstr(n, "process_")) {
        atomic_fetch_add_explicit(&g_native_vm_io_calls, 1, memory_order_relaxed);
    }
    if (!strcmp(n, "host_exec_ncb_json")) return nc_fn_builtin_host_exec_ncb_json(a, na);
    if (!strcmp(n, "host_kall_bygg_bundle")) return nc_fn_builtin_host_kall_bygg_bundle(a, na);
    const char *builtin = n;
    while (!strncmp(builtin, "builtin.", 8)) builtin += 8;
    if (!strcmp(builtin, "atomic_operation")) return nc_builtin_atomic_operation(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "jit_operation")) return nc_builtin_jit_operation(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "tensor_operation")) return nc_builtin_tensor_operation(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "media_diffusion_operation")) return nc_builtin_media_diffusion_operation(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "bytes_new")) return nc_builtin_bytes_new(na > 0 ? a[0] : nc_int(0), na > 1 ? a[1] : nc_int(0));
    if (!strcmp(builtin, "bytes_from_list")) return nc_builtin_bytes_from_list(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "bytes_to_list")) return nc_builtin_bytes_to_list(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "fil_les_binary") || !strcmp(builtin, "fil_les_binær")) return nc_builtin_fil_les_binary(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "process_spawn_argv")) return nc_builtin_process_spawn_argv(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "process_operation")) return nc_builtin_process_operation(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "filesystem_read_operation")) return nc_builtin_filesystem_read_operation(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "filesystem_write_operation")) return nc_builtin_filesystem_write_operation(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "native_mkdir_p")) return nc_builtin_mkdir_p(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "sti_mkdir_p")) return nc_builtin_mkdir_p(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "network_operation")) return nc_builtin_network_operation(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "dns_lookup") || !strcmp(builtin, "resolve_host")) return nc_builtin_dns_lookup(na > 0 ? a[0] : nc_nil(), na > 1 ? a[1] : nc_str("0"));
    if (!strcmp(builtin, "thread_spawn")) return nc_builtin_thread_spawn(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "thread_join")) return nc_builtin_thread_join(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "thread_sync")) return nc_builtin_thread_sync(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "thread_pool")) return nc_builtin_thread_pool(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "thread_current_id")) return nc_builtin_thread_current_id();
    if (!strcmp(builtin, "pbkdf2_sha256")) return nc_builtin_pbkdf2_sha256(na > 0 ? a[0] : nc_nil(), na > 1 ? a[1] : nc_nil(), na > 2 ? a[2] : nc_int(0), na > 3 ? a[3] : nc_int(0));
    if (!strcmp(builtin, "argon2id")) return nc_builtin_argon2id(na > 0 ? a[0] : nc_nil(), na > 1 ? a[1] : nc_nil(), na > 2 ? a[2] : nc_int(0), na > 3 ? a[3] : nc_int(0), na > 4 ? a[4] : nc_int(0), na > 5 ? a[5] : nc_int(0));
    if (!strcmp(builtin, "acme_sign")) return nc_builtin_acme_sign(na > 0 ? a[0] : nc_nil(), na > 1 ? a[1] : nc_nil(), na > 2 ? a[2] : nc_nil());
    if (!strcmp(builtin, "acme_verify")) return nc_builtin_acme_verify(na > 0 ? a[0] : nc_nil(), na > 1 ? a[1] : nc_nil(), na > 2 ? a[2] : nc_nil(), na > 3 ? a[3] : nc_nil());
    if (!strcmp(builtin, "heiltall") || !strcmp(builtin, "heiltall_fra_tekst") ||
        !strcmp(builtin, "heltall") || !strcmp(builtin, "heltall_fra_tekst")) return nc_builtin_heltall(na > 0 ? a[0] : nc_nil());
    if (!strcmp(builtin, "vm_memory_status")) return nc_builtin_vm_memory_status_native();
    if (!strcmp(builtin, "vm_runtime_metrics")) return nc_builtin_vm_runtime_metrics_native();
    if (!strcmp(builtin, "vm_debug_enable")) return nc_builtin_vm_debug_enable_native();
    if (!strcmp(builtin, "vm_debug_disable")) return nc_builtin_vm_debug_disable_native();
    if (!strcmp(builtin, "vm_debug_breakpoint_add")) return nc_builtin_vm_debug_breakpoint_add_native(a, na);
    if (!strcmp(builtin, "vm_debug_breakpoint_remove")) return nc_builtin_vm_debug_breakpoint_remove_native(a, na);
    if (!strcmp(builtin, "vm_debug_breakpoints_clear")) return nc_builtin_vm_debug_breakpoints_clear_native();
    if (!strcmp(builtin, "vm_debug_watch_add")) return nc_builtin_vm_debug_watch_native(a, na, 1);
    if (!strcmp(builtin, "vm_debug_watch_remove")) return nc_builtin_vm_debug_watch_native(a, na, 0);
    if (!strcmp(builtin, "vm_debug_watches_clear")) return nc_builtin_vm_debug_watches_clear_native();
    if (!strcmp(builtin, "vm_debug_suspend_on_hit")) { g_native_debug_suspend_on_hit = na > 0 && nc_truthy(a[0]); return nc_bool(1); }
    if (!strcmp(builtin, "vm_debug_continue") || !strcmp(builtin, "vm_debug_resume")) { g_native_debug_step_pending = 0; return nc_bool(1); }
    if (!strcmp(builtin, "vm_debug_step")) { g_native_debug_step_pending = 1; return nc_bool(1); }
    if (!strcmp(builtin, "vm_debug_snapshot")) return nc_builtin_vm_debug_snapshot_native();

    if (!strncmp(n, "builtin.", 8)) return nc_dispatch_call(n + 8, a, na);
    if (!strncmp(n, "__main__.", 9)) return nc_dispatch_call(n + 9, a, na);

    for (int i = 0; nc_dispatch[i].name; i++) {
        if (!strcmp(nc_dispatch[i].name, n)) return nc_dispatch[i].fn(a, na);
    }

    /* Kvalifiserte namn skal aldri kaprast av ein tilfeldig suffix-match. */
    if (strchr(n, '.')) return NULL;

    const char *last = strrchr(n, '.');
    last = last ? last + 1 : n;
    for (int i = 0; nc_dispatch[i].name; i++) {
        const char *fn_last = strrchr(nc_dispatch[i].name, '.');
        fn_last = fn_last ? fn_last + 1 : nc_dispatch[i].name;
        if (!strcmp(fn_last, last)) return nc_dispatch[i].fn(a, na);
    }

    char alias_buf[256];
    strncpy(alias_buf, last, sizeof(alias_buf) - 1);
    alias_buf[sizeof(alias_buf) - 1] = '\0';
    char *token_suffix = strstr(alias_buf, "_token");
    if (token_suffix) {
        *token_suffix = '\0';
        return nc_dispatch_call(alias_buf, a, na);
    }

    return NULL;
}

NcVal *nc_fn_builtin_neste_token(NcVal **a, int na) {
    return nc_dispatch_call("neste", a, na);
}

#define NC_NETWORK_MAX 256
typedef struct {
    int active,id,listener,connecting,datagram,tls_handshake_done,tls_server;
    nc_socket_handle_t fd;
    unsigned long long generation;
    int require_ocsp,ocsp_stapled;
    char ocsp_status[32];
    char tls_hostname[256];
    char tls_client_certificate_subject[256];
#if defined(_WIN32)
    NcwSocket iocp_socket;
    int iocp_owned;
    NcwSChannelClient *schannel;
    NcwSChannelServerCredential *schannel_server_credentials;
    NcwSChannelServer *schannel_server;
    int schannel_credentials_owner;
#endif
#if defined(NC_ENABLE_OPENSSL)
    SSL *ssl;
    SSL_CTX *ssl_ctx;
#endif
    pthread_mutex_t mutex;
} NcNetworkSlot;
static NcNetworkSlot g_network_slots[NC_NETWORK_MAX];
static pthread_mutex_t g_network_registry_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t g_network_once=PTHREAD_ONCE_INIT;
#if defined(_WIN32)
static NcwIocp g_network_iocp;
static int g_network_iocp_ready=0;
#endif
static int nc_socket_last_error(void){
#if defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}
static int nc_socket_would_block(int error){
#if defined(_WIN32)
    return error==WSAEWOULDBLOCK||error==WSAEINPROGRESS||error==WSAEALREADY;
#else
    return error==EAGAIN||error==EWOULDBLOCK||error==EINPROGRESS;
#endif
}
static const char *nc_socket_error_text(int error){
#if defined(_WIN32)
    static _Thread_local char buffer[256];DWORD flags=FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD length=FormatMessageA(flags,NULL,(DWORD)error,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),buffer,sizeof(buffer),NULL);
    if(!length)snprintf(buffer,sizeof(buffer),"Winsock error %d",error);else while(length&&(buffer[length-1]=='\r'||buffer[length-1]=='\n'))buffer[--length]=0;return buffer;
#else
    return strerror(error);
#endif
}
static int nc_hex_value(char c){
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}
static unsigned char *nc_hex_decode(const char *hex,size_t *out_len){
    size_t len=strlen(hex);if(!len||(len&1)||len>2097152)return NULL;
    unsigned char *out=malloc(len/2);if(!out)return NULL;
    for(size_t i=0;i<len/2;i++){int hi=nc_hex_value(hex[i*2]),lo=nc_hex_value(hex[i*2+1]);if(hi<0||lo<0){free(out);return NULL;}out[i]=(unsigned char)((hi<<4)|lo);}
    *out_len=len/2;return out;
}
static char *nc_hex_encode(const unsigned char *data,size_t len){
    static const char digits[]="0123456789abcdef";char *out=malloc(len*2+1);if(!out)return NULL;
    for(size_t i=0;i<len;i++){out[i*2]=digits[(data[i]>>4)&15];out[i*2+1]=digits[data[i]&15];}out[len*2]=0;return out;
}
static void nc_network_socket_close(nc_socket_handle_t fd){
#if defined(_WIN32)
    if(fd!=INVALID_SOCKET)closesocket(fd);
#else
    if(fd>=0)close(fd);
#endif
}
static void nc_network_init_once(void){
#if defined(_WIN32)
    WSADATA winsock_data;
    (void)WSAStartup(MAKEWORD(2,2),&winsock_data);
    char iocp_error[NCW_ERROR_CAP]="";
    g_network_iocp_ready=ncw_iocp_open(&g_network_iocp,iocp_error,sizeof(iocp_error));
#else
    signal(SIGPIPE,SIG_IGN);
#endif
#if defined(NC_ENABLE_OPENSSL)
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS|OPENSSL_INIT_LOAD_CRYPTO_STRINGS,NULL);
#endif
}

static NcVal *nc_network_result(const char *status,NcNetworkSlot *slot,const char *data,long long value,int port,const char *error){
    NcVal *r=nc_map_new();char handle[64]="";if(slot)snprintf(handle,sizeof(handle),"socket:%d:%llu",slot->id,slot->generation);
    nc_index_set(r,nc_str("abi"),nc_str("norscode-native-network-v1"));nc_index_set(r,nc_str("status"),nc_str(status));nc_index_set(r,nc_str("handle"),nc_str(handle));
    nc_index_set(r,nc_str("data"),nc_str(data?data:""));nc_index_set(r,nc_str("value"),nc_int(value));nc_index_set(r,nc_str("port"),nc_int(port));nc_index_set(r,nc_str("error"),nc_str(error?error:""));
    nc_index_set(r,nc_str("event_backend"),nc_str(nc_readiness_backend()));
#if defined(NC_ENABLE_OPENSSL)
    nc_index_set(r,nc_str("tls_version"),nc_str(slot&&slot->ssl&&slot->tls_handshake_done?SSL_get_version(slot->ssl):""));
    nc_index_set(r,nc_str("cipher"),nc_str(slot&&slot->ssl&&slot->tls_handshake_done?SSL_get_cipher_name(slot->ssl):""));
    X509 *peer=slot&&slot->ssl&&slot->tls_handshake_done?SSL_get1_peer_certificate(slot->ssl):NULL;
    int peer_verified=peer&&SSL_get_verify_result(slot->ssl)==X509_V_OK;char peer_subject[512]="";
    if(peer){X509_NAME_oneline(X509_get_subject_name(peer),peer_subject,sizeof(peer_subject));X509_free(peer);}
    nc_index_set(r,nc_str("peer_verified"),nc_bool(peer_verified));
    nc_index_set(r,nc_str("peer_cert_present"),nc_bool(peer!=NULL));
    nc_index_set(r,nc_str("peer_subject"),nc_str(peer_subject));
    nc_index_set(r,nc_str("ocsp_stapled"),nc_bool(slot&&slot->ocsp_stapled));
    nc_index_set(r,nc_str("ocsp_status"),nc_str(slot?slot->ocsp_status:""));
#elif defined(_WIN32)
    int client_ready=slot&&slot->schannel&&slot->schannel->handshake_done,server_ready=slot&&slot->schannel_server&&slot->schannel_server->handshake_done;ALG_ID cipher=client_ready?slot->schannel->cipher:server_ready?slot->schannel_server->cipher:0;int peer_present=client_ready?slot->schannel->peer_cert_present:server_ready?slot->schannel_server->peer_cert_present:0;char cipher_name[64]="";if(cipher)snprintf(cipher_name,sizeof(cipher_name),"ALG_ID:%lu",(unsigned long)cipher);
    nc_index_set(r,nc_str("tls_version"),nc_str(client_ready||server_ready?"TLSv1.3":""));nc_index_set(r,nc_str("cipher"),nc_str(cipher_name));
    nc_index_set(r,nc_str("peer_verified"),nc_bool(client_ready?peer_present:server_ready&&(!slot->schannel_server_credentials||!slot->schannel_server_credentials->require_client_cert||peer_present)));nc_index_set(r,nc_str("peer_cert_present"),nc_bool(peer_present));nc_index_set(r,nc_str("peer_subject"),nc_str(peer_present?"Windows certificate store":""));
    nc_index_set(r,nc_str("ocsp_stapled"),nc_bool(0));nc_index_set(r,nc_str("ocsp_status"),nc_str(client_ready||server_ready?"system-policy":""));
#else
    nc_index_set(r,nc_str("tls_version"),nc_str(""));nc_index_set(r,nc_str("cipher"),nc_str(""));nc_index_set(r,nc_str("peer_verified"),nc_bool(0));
    nc_index_set(r,nc_str("peer_cert_present"),nc_bool(0));nc_index_set(r,nc_str("peer_subject"),nc_str(""));
    nc_index_set(r,nc_str("ocsp_stapled"),nc_bool(0));nc_index_set(r,nc_str("ocsp_status"),nc_str(""));
#endif
    nc_index_set(r,nc_str("hostname"),nc_str(slot?slot->tls_hostname:""));return r;
}
static NcNetworkSlot *nc_network_slot(NcVal *request){
    const char *h=nc_atomic_text_field(request,"handle","");long id=0;unsigned long long generation=0;char trailing='\0';if(sscanf(h,"socket:%ld:%llu%c",&id,&generation,&trailing)!=2||id<=0||id>=NC_NETWORK_MAX||generation==0)return NULL;
    pthread_mutex_lock(&g_network_registry_lock);NcNetworkSlot *s=g_network_slots[id].active==1&&g_network_slots[id].generation==generation?&g_network_slots[id]:NULL;pthread_mutex_unlock(&g_network_registry_lock);return s;
}
static NcNetworkSlot *nc_network_adopt_fd(nc_socket_handle_t fd,int listener,int connecting){
    pthread_mutex_lock(&g_network_registry_lock);int id=0;NcNetworkSlot *s=NULL;for(int i=1;i<NC_NETWORK_MAX;i++)if(!g_network_slots[i].active){id=i;s=&g_network_slots[i];unsigned long long generation=s->generation+1;if(!generation)generation=1;memset(s,0,sizeof(*s));s->generation=generation;s->active=2;s->id=id;s->fd=fd;s->listener=listener;s->connecting=connecting;break;}pthread_mutex_unlock(&g_network_registry_lock);
    if(!id){nc_network_socket_close(fd);return NULL;}pthread_mutex_init(&s->mutex,NULL);
    pthread_mutex_lock(&g_network_registry_lock);s->active=1;pthread_mutex_unlock(&g_network_registry_lock);return s;
}
#if defined(_WIN32)
static NcNetworkSlot *nc_network_adopt_iocp_socket(NcwSocket socket,int listener,int connecting){
    NcNetworkSlot *slot=nc_network_adopt_fd(socket.socket,listener,connecting);if(slot){slot->iocp_socket=socket;slot->iocp_owned=1;}return slot;
}

#define NC_IOCP_OPERATION_MAX 512
typedef struct {
    int active,id,completed,kind;
    unsigned long long generation;
    NcwIoOperation operation;
    NcNetworkSlot *socket_slot;
    _Atomic int *pending_owner;
    char *buffer;
    size_t buffer_size;
} NcIocpOperationSlot;
static NcIocpOperationSlot g_iocp_operations[NC_IOCP_OPERATION_MAX];
static pthread_mutex_t g_iocp_operation_lock=PTHREAD_MUTEX_INITIALIZER;

static NcIocpOperationSlot *nc_iocp_operation_new(int kind,NcNetworkSlot *socket_slot,size_t buffer_size){
    pthread_mutex_lock(&g_iocp_operation_lock);NcIocpOperationSlot *entry=NULL;
    for(int i=1;i<NC_IOCP_OPERATION_MAX;i++)if(!g_iocp_operations[i].active){entry=&g_iocp_operations[i];unsigned long long generation=entry->generation+1;if(!generation)generation=1;memset(entry,0,sizeof(*entry));entry->generation=generation;entry->active=1;entry->id=i;entry->kind=kind;entry->socket_slot=socket_slot;break;}
    pthread_mutex_unlock(&g_iocp_operation_lock);if(entry&&buffer_size){entry->buffer=calloc(buffer_size+1,1);entry->buffer_size=buffer_size;if(!entry->buffer){pthread_mutex_lock(&g_iocp_operation_lock);entry->active=0;pthread_mutex_unlock(&g_iocp_operation_lock);return NULL;}}return entry;
}

static NcIocpOperationSlot *nc_iocp_operation_from_handle(const char *handle){
    long id=0;unsigned long long generation=0;char trailing='\0';if(!handle||sscanf(handle,"iocp-op:%ld:%llu%c",&id,&generation,&trailing)!=2||id<=0||id>=NC_IOCP_OPERATION_MAX||generation==0)return NULL;
    pthread_mutex_lock(&g_iocp_operation_lock);NcIocpOperationSlot *entry=g_iocp_operations[id].active&&g_iocp_operations[id].generation==generation?&g_iocp_operations[id]:NULL;pthread_mutex_unlock(&g_iocp_operation_lock);return entry;
}

static NcVal *nc_iocp_result(const char *status,NcIocpOperationSlot *entry,NcNetworkSlot *socket,const char *error){
    NcVal *result=nc_network_result(status,socket,entry&&(entry->kind==3||entry->kind==5)&&entry->completed&&entry->buffer?entry->buffer:"",entry&&entry->completed?(long long)entry->operation.transferred:0,0,error);char operation_handle[64]="";
    if(entry)snprintf(operation_handle,sizeof(operation_handle),"iocp-op:%d:%llu",entry->id,entry->generation);nc_index_set(result,nc_str("operation_handle"),nc_str(operation_handle));nc_index_set(result,nc_str("operation_kind"),nc_int(entry?entry->kind:0));nc_index_set(result,nc_str("event_backend"),nc_str("iocp"));return result;
}

static void nc_iocp_operation_release(NcIocpOperationSlot *entry){
    if(!entry)return;if(entry->pending_owner){atomic_fetch_sub_explicit(entry->pending_owner,1,memory_order_acq_rel);entry->pending_owner=NULL;}free(entry->buffer);entry->buffer=NULL;entry->buffer_size=0;pthread_mutex_lock(&g_iocp_operation_lock);entry->active=0;pthread_mutex_unlock(&g_iocp_operation_lock);
}

static NcVal *nc_builtin_iocp_operation(NcVal *request,const char *op){
    if(!g_network_iocp_ready)return nc_iocp_result("feil",NULL,NULL,"IOCP unavailable");char error[NCW_ERROR_CAP]="";
    if(!strcmp(op,"iocp_listen")){
        NcwSocket socket={INVALID_SOCKET};uint16_t port=0;const char *host=nc_atomic_text_field(request,"host","127.0.0.1");long long requested_port=nc_atomic_int_field(request,"port",0);
        if(requested_port<0||requested_port>65535||!ncw_socket_listen(&g_network_iocp,&socket,host,(uint16_t)requested_port,&port,error,sizeof(error)))return nc_iocp_result("feil",NULL,NULL,error);
        NcNetworkSlot *slot=nc_network_adopt_iocp_socket(socket,1,0);if(!slot){ncw_socket_close(&socket,error,sizeof(error));return nc_iocp_result("feil",NULL,NULL,"socket capacity");}NcVal *result=nc_iocp_result("listening",NULL,slot,"");nc_index_set(result,nc_str("port"),nc_int(port));return result;
    }
    if(!strcmp(op,"iocp_connect")){
        NcNetworkSlot *slot=nc_network_adopt_iocp_socket((NcwSocket){INVALID_SOCKET},0,1);if(!slot)return nc_iocp_result("feil",NULL,NULL,"socket capacity");NcIocpOperationSlot *entry=nc_iocp_operation_new(2,slot,0);if(!entry)return nc_iocp_result("feil",NULL,slot,"operation capacity");
        long long port=nc_atomic_int_field(request,"port",0);if(port<1||port>65535||!ncw_socket_connect_async(&g_network_iocp,&slot->iocp_socket,nc_atomic_text_field(request,"host",""),(uint16_t)port,&entry->operation,error,sizeof(error))){nc_iocp_operation_release(entry);pthread_mutex_lock(&g_network_registry_lock);slot->active=0;pthread_mutex_unlock(&g_network_registry_lock);return nc_iocp_result("feil",NULL,NULL,error);}slot->fd=slot->iocp_socket.socket;return nc_iocp_result("ventende",entry,slot,"");
    }
    if(!strcmp(op,"iocp_accept")){
        NcNetworkSlot *listener=nc_network_slot(request);if(!listener||!listener->iocp_owned||!listener->listener)return nc_iocp_result("feil",NULL,NULL,"invalid IOCP listener");NcNetworkSlot *accepted=nc_network_adopt_iocp_socket((NcwSocket){INVALID_SOCKET},0,1);if(!accepted)return nc_iocp_result("feil",NULL,NULL,"socket capacity");NcIocpOperationSlot *entry=nc_iocp_operation_new(1,accepted,0);if(!entry)return nc_iocp_result("feil",NULL,accepted,"operation capacity");
        if(!ncw_socket_accept_async(&g_network_iocp,&listener->iocp_socket,&accepted->iocp_socket,&entry->operation,error,sizeof(error))){nc_iocp_operation_release(entry);pthread_mutex_lock(&g_network_registry_lock);accepted->active=0;pthread_mutex_unlock(&g_network_registry_lock);return nc_iocp_result("feil",NULL,NULL,error);}accepted->fd=accepted->iocp_socket.socket;return nc_iocp_result("ventende",entry,accepted,"");
    }
    if(!strcmp(op,"iocp_read")||!strcmp(op,"iocp_write")){
        NcNetworkSlot *slot=nc_network_slot(request);if(!slot||!slot->iocp_owned)return nc_iocp_result("feil",NULL,NULL,"invalid IOCP socket");int kind=!strcmp(op,"iocp_read")?3:4;size_t size=kind==3?(size_t)nc_atomic_int_field(request,"max",65536):strlen(nc_atomic_text_field(request,"data",""));if(!size||size>1048576)return nc_iocp_result("feil",NULL,slot,"invalid IOCP buffer size");NcIocpOperationSlot *entry=nc_iocp_operation_new(kind,slot,size);if(!entry)return nc_iocp_result("feil",NULL,slot,"operation capacity");
        int submitted=kind==3?ncw_socket_read_async(&slot->iocp_socket,entry->buffer,size,&entry->operation,error,sizeof(error)):0;if(kind==4){memcpy(entry->buffer,nc_atomic_text_field(request,"data",""),size);submitted=ncw_socket_write_async(&slot->iocp_socket,entry->buffer,size,&entry->operation,error,sizeof(error));}if(!submitted){nc_iocp_operation_release(entry);return nc_iocp_result("feil",NULL,slot,error);}return nc_iocp_result("ventende",entry,slot,"");
    }
    if(!strcmp(op,"iocp_wait")){
        const char *wanted_handle=nc_atomic_text_field(request,"operation_handle","");NcIocpOperationSlot *wanted=*wanted_handle?nc_iocp_operation_from_handle(wanted_handle):NULL;if(*wanted_handle&&!wanted)return nc_iocp_result("feil",NULL,NULL,"invalid IOCP operation handle");
        if(wanted&&wanted->completed){const char *status=wanted->operation.error_code==ERROR_SUCCESS?"klar":wanted->operation.error_code==ERROR_OPERATION_ABORTED?"kansellert":"feil";const char *completion_error=wanted->operation.error_code==ERROR_SUCCESS?"":nc_socket_error_text((int)wanted->operation.error_code);NcVal *ready=nc_iocp_result(status,wanted,wanted->socket_slot,completion_error);nc_iocp_operation_release(wanted);return ready;}
        uint32_t timeout=(uint32_t)nc_atomic_int_field(request,"timeout_ms",0);NcwIoOperation *completed=NULL;int wait_result=ncw_iocp_wait(&g_network_iocp,&completed,timeout,error,sizeof(error));if(wait_result<0)return nc_iocp_result("feil",NULL,NULL,error);if(wait_result==0)return nc_iocp_result("ventende",wanted,wanted?wanted->socket_slot:NULL,"");NcIocpOperationSlot *entry=(NcIocpOperationSlot *)((char *)completed-offsetof(NcIocpOperationSlot,operation));if(!entry->active)return nc_iocp_result("feil",NULL,NULL,"stale IOCP completion");entry->completed=1;if(entry->socket_slot){entry->socket_slot->connecting=0;entry->socket_slot->fd=entry->socket_slot->iocp_socket.socket;}
        if(wanted&&entry!=wanted)return nc_iocp_result("ventende",wanted,wanted->socket_slot,"");const char *completion_status=entry->operation.error_code==ERROR_SUCCESS?"klar":entry->operation.error_code==ERROR_OPERATION_ABORTED?"kansellert":"feil";const char *completion_error=entry->operation.error_code==ERROR_SUCCESS?"":nc_socket_error_text((int)entry->operation.error_code);NcVal *ready=nc_iocp_result(completion_status,entry,entry->socket_slot,completion_error);nc_iocp_operation_release(entry);return ready;
    }
    if(!strcmp(op,"iocp_cancel")){
        NcIocpOperationSlot *entry=nc_iocp_operation_from_handle(nc_atomic_text_field(request,"operation_handle",""));if(!entry)return nc_iocp_result("feil",NULL,NULL,"invalid IOCP operation handle");if(!entry->operation.owner_handle)return nc_iocp_result("feil",entry,entry->socket_slot,"IOCP operation owner missing");if(!CancelIoEx(entry->operation.owner_handle,&entry->operation.overlapped)&&GetLastError()!=ERROR_NOT_FOUND)return nc_iocp_result("feil",entry,entry->socket_slot,nc_socket_error_text((int)GetLastError()));return nc_iocp_result("kansellert",entry,entry->socket_slot,"");
    }
    return nc_iocp_result("feil",NULL,NULL,"invalid IOCP operation");
}
#endif
static int nc_network_addr(const char *host,int port,struct sockaddr_in *addr){
    memset(addr,0,sizeof(*addr));addr->sin_family=AF_INET;addr->sin_port=htons((uint16_t)port);
    if(!strcmp(host,"localhost"))addr->sin_addr.s_addr=htonl(INADDR_LOOPBACK);else if(!strcmp(host,"0.0.0.0"))addr->sin_addr.s_addr=INADDR_ANY;else if(inet_pton(AF_INET,host,&addr->sin_addr)!=1)return 0;return 1;
}
#if defined(NC_ENABLE_OPENSSL)
typedef struct { unsigned char *data; int length; } NcOcspStaple;
static int g_nc_ocsp_ex_index=-1;static pthread_once_t g_nc_ocsp_ex_once=PTHREAD_ONCE_INIT;
static void nc_tls_ocsp_staple_free(void *parent,void *ptr,CRYPTO_EX_DATA *ad,int idx,long argl,void *argp){
    (void)parent;(void)ad;(void)idx;(void)argl;(void)argp;NcOcspStaple *staple=ptr;if(staple){OPENSSL_free(staple->data);free(staple);}
}
static void nc_tls_ocsp_ex_init(void){g_nc_ocsp_ex_index=SSL_CTX_get_ex_new_index(0,NULL,NULL,NULL,nc_tls_ocsp_staple_free);}
static int nc_tls_ocsp_server_callback(SSL *ssl,void *arg){
    NcOcspStaple *staple=arg;if(!staple||!staple->data||staple->length<=0)return SSL_TLSEXT_ERR_NOACK;
    unsigned char *copy=OPENSSL_memdup(staple->data,(size_t)staple->length);if(!copy)return SSL_TLSEXT_ERR_ALERT_FATAL;
    SSL_set_tlsext_status_ocsp_resp(ssl,copy,staple->length);return SSL_TLSEXT_ERR_OK;
}
static int nc_tls_configure_ocsp_staple(SSL_CTX *ctx,const char *path){
    if(!path||!*path)return 1;FILE *file=fopen(path,"rb");if(!file)return 0;
    if(fseek(file,0,SEEK_END)!=0){fclose(file);return 0;}long length=ftell(file);if(length<=0||length>1024*1024||fseek(file,0,SEEK_SET)!=0){fclose(file);return 0;}
    NcOcspStaple *staple=calloc(1,sizeof(*staple));if(!staple){fclose(file);return 0;}staple->data=OPENSSL_malloc((size_t)length);staple->length=(int)length;
    if(!staple->data||fread(staple->data,1,(size_t)length,file)!=(size_t)length){fclose(file);OPENSSL_free(staple->data);free(staple);return 0;}fclose(file);
    pthread_once(&g_nc_ocsp_ex_once,nc_tls_ocsp_ex_init);if(g_nc_ocsp_ex_index<0||SSL_CTX_set_ex_data(ctx,g_nc_ocsp_ex_index,staple)!=1){OPENSSL_free(staple->data);free(staple);return 0;}
    SSL_CTX_set_tlsext_status_cb(ctx,nc_tls_ocsp_server_callback);SSL_CTX_set_tlsext_status_arg(ctx,staple);return 1;
}
static int nc_tls_verify_ocsp(NcNetworkSlot *slot,char *error,size_t error_cap){
    const unsigned char *response_data=NULL;long response_length=SSL_get_tlsext_status_ocsp_resp(slot->ssl,&response_data);
    if(response_length<=0||!response_data){snprintf(slot->ocsp_status,sizeof(slot->ocsp_status),"missing");if(slot->require_ocsp)snprintf(error,error_cap,"OCSP staple required");return slot->require_ocsp?0:1;}
    slot->ocsp_stapled=1;const unsigned char *cursor=response_data;OCSP_RESPONSE *response=d2i_OCSP_RESPONSE(NULL,&cursor,response_length);
    if(!response||OCSP_response_status(response)!=OCSP_RESPONSE_STATUS_SUCCESSFUL){if(response)OCSP_RESPONSE_free(response);snprintf(slot->ocsp_status,sizeof(slot->ocsp_status),"invalid");snprintf(error,error_cap,"invalid OCSP response");return 0;}
    OCSP_BASICRESP *basic=OCSP_response_get1_basic(response);STACK_OF(X509) *chain=SSL_get_peer_cert_chain(slot->ssl);X509_STORE *store=SSL_CTX_get_cert_store(slot->ssl_ctx);
    if(!basic||OCSP_basic_verify(basic,NULL,store,0)!=1){if(basic)OCSP_BASICRESP_free(basic);OCSP_RESPONSE_free(response);snprintf(slot->ocsp_status,sizeof(slot->ocsp_status),"unverified");snprintf(error,error_cap,"unverified OCSP response");return 0;}
    X509 *peer=SSL_get1_peer_certificate(slot->ssl),*issuer=NULL;if(peer&&chain){for(int i=0;i<sk_X509_num(chain);i++){X509 *candidate=sk_X509_value(chain,i);if(X509_check_issued(candidate,peer)==X509_V_OK){issuer=candidate;break;}}}
    const STACK_OF(X509) *response_certs=OCSP_resp_get0_certs(basic);if(peer&&!issuer&&response_certs){for(int i=0;i<sk_X509_num(response_certs);i++){X509 *candidate=sk_X509_value(response_certs,i);if(X509_check_issued(candidate,peer)==X509_V_OK){issuer=candidate;break;}}}if(peer&&!issuer&&X509_check_issued(peer,peer)==X509_V_OK)issuer=peer;
    OCSP_CERTID *id=peer&&issuer?OCSP_cert_to_id(NULL,peer,issuer):NULL;int cert_status=V_OCSP_CERTSTATUS_UNKNOWN,reason=0;ASN1_GENERALIZEDTIME *revtime=NULL,*thisupd=NULL,*nextupd=NULL;
    int found=id&&OCSP_resp_find_status(basic,id,&cert_status,&reason,&revtime,&thisupd,&nextupd);int valid=found&&OCSP_check_validity(thisupd,nextupd,300,-1);
    if(id)OCSP_CERTID_free(id);if(peer)X509_free(peer);OCSP_BASICRESP_free(basic);OCSP_RESPONSE_free(response);
    if(!valid||cert_status!=V_OCSP_CERTSTATUS_GOOD){snprintf(slot->ocsp_status,sizeof(slot->ocsp_status),cert_status==V_OCSP_CERTSTATUS_REVOKED?"revoked":cert_status==V_OCSP_CERTSTATUS_GOOD?"stale":"unknown");snprintf(error,error_cap,"OCSP certificate status not good");return 0;}
    snprintf(slot->ocsp_status,sizeof(slot->ocsp_status),"good");return 1;
}
static int nc_tls_enable_crl(SSL_CTX *ctx,const char *crl_file){
    if(!crl_file||!*crl_file)return 1;X509_STORE *store=SSL_CTX_get_cert_store(ctx);if(!store)return 0;
    if(X509_STORE_load_locations(store,crl_file,NULL)!=1)return 0;
    return X509_STORE_set_flags(store,X509_V_FLAG_CRL_CHECK|X509_V_FLAG_CRL_CHECK_ALL)==1;
}
static SSL_CTX *nc_tls_server_ctx(const char *cert,const char *key,const char *client_ca,const char *client_crl,const char *ocsp_response,int require_client_cert){
    SSL_CTX *ctx=SSL_CTX_new(TLS_server_method());if(!ctx)return NULL;
    SSL_CTX_set_min_proto_version(ctx,TLS1_3_VERSION);SSL_CTX_set_max_proto_version(ctx,TLS1_3_VERSION);
    if(SSL_CTX_use_certificate_chain_file(ctx,cert)!=1||SSL_CTX_use_PrivateKey_file(ctx,key,SSL_FILETYPE_PEM)!=1||SSL_CTX_check_private_key(ctx)!=1){SSL_CTX_free(ctx);return NULL;}
    if(require_client_cert){
        if(!client_ca||!*client_ca||SSL_CTX_load_verify_locations(ctx,client_ca,NULL)!=1){SSL_CTX_free(ctx);return NULL;}
        if(!nc_tls_enable_crl(ctx,client_crl)){SSL_CTX_free(ctx);return NULL;}
        SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,NULL);
        STACK_OF(X509_NAME) *names=SSL_load_client_CA_file(client_ca);if(names)SSL_CTX_set_client_CA_list(ctx,names);
    }
    if(!nc_tls_configure_ocsp_staple(ctx,ocsp_response)){SSL_CTX_free(ctx);return NULL;}
    return ctx;
}
static int nc_tls_attach_client(NcNetworkSlot *s,const char *hostname,const char *cafile,const char *crl_file,const char *client_cert,const char *client_key,int require_ocsp){
    SSL_CTX *ctx=SSL_CTX_new(TLS_client_method());if(!ctx)return 0;SSL_CTX_set_min_proto_version(ctx,TLS1_3_VERSION);SSL_CTX_set_max_proto_version(ctx,TLS1_3_VERSION);SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,NULL);
    if(cafile&&*cafile){if(SSL_CTX_load_verify_locations(ctx,cafile,NULL)!=1){SSL_CTX_free(ctx);return 0;}}else if(SSL_CTX_set_default_verify_paths(ctx)!=1){SSL_CTX_free(ctx);return 0;}
    if(!nc_tls_enable_crl(ctx,crl_file)){SSL_CTX_free(ctx);return 0;}
    if((client_cert&&*client_cert)||(client_key&&*client_key)){
        if(!client_cert||!*client_cert||!client_key||!*client_key||SSL_CTX_use_certificate_chain_file(ctx,client_cert)!=1||SSL_CTX_use_PrivateKey_file(ctx,client_key,SSL_FILETYPE_PEM)!=1||SSL_CTX_check_private_key(ctx)!=1){SSL_CTX_free(ctx);return 0;}
    }
    SSL *ssl=SSL_new(ctx);if(!ssl){SSL_CTX_free(ctx);return 0;}SSL_set_fd(ssl,s->fd);SSL_set_connect_state(ssl);SSL_set_tlsext_host_name(ssl,hostname);SSL_set1_host(ssl,hostname);
    s->require_ocsp=require_ocsp;if(require_ocsp)SSL_set_tlsext_status_type(ssl,TLSEXT_STATUSTYPE_ocsp);
    s->ssl_ctx=ctx;s->ssl=ssl;snprintf(s->tls_hostname,sizeof(s->tls_hostname),"%s",hostname);return 1;
}
static int nc_tls_handshake_step(NcNetworkSlot *s,int *want_events,char *error,size_t error_cap){
    if(s->tls_handshake_done)return 1;int rc=SSL_do_handshake(s->ssl);if(rc==1){if(!s->tls_server&&!nc_tls_verify_ocsp(s,error,error_cap))return -1;s->tls_handshake_done=1;*want_events=0;return 1;}int e=SSL_get_error(s->ssl,rc);
    if(e==SSL_ERROR_WANT_READ){*want_events=1;return 0;}if(e==SSL_ERROR_WANT_WRITE){*want_events=2;return 0;}unsigned long oe=ERR_get_error();snprintf(error,error_cap,"%s",oe?ERR_error_string(oe,NULL):"TLS handshake failed");return -1;
}
#endif
static NcVal *nc_builtin_network_operation(NcVal *request){
    pthread_once(&g_network_once,nc_network_init_once);
    if(!request||request->type!=NC_MAP||strcmp(nc_atomic_text_field(request,"abi",""),"norscode-native-network-v1"))return nc_network_result("feil",NULL,"",-1,0,"invalid request");
    const char *op=nc_atomic_text_field(request,"operation","");
#if defined(_WIN32)
    if(!strncmp(op,"iocp_",5))return nc_builtin_iocp_operation(request,op);
#endif
    if(!strcmp(op,"poll_many")){
        NcVal *items=nc_index_get(request,nc_str("items"));
        int timeout_ms=(int)nc_atomic_int_field(request,"timeout_ms",0);
        if(!items||items->type!=NC_LIST||items->list->len>NC_READINESS_BATCH_MAX||timeout_ms<0){
            return nc_network_result("feil",NULL,"",-1,0,"invalid poll_many request");
        }
        int count=items->list->len;
        nc_socket_handle_t fds[NC_READINESS_BATCH_MAX];int events[NC_READINESS_BATCH_MAX],ready[NC_READINESS_BATCH_MAX];
        NcNetworkSlot *slots[NC_READINESS_BATCH_MAX];
        for(int i=0;i<count;i++){
            NcVal *item=items->list->items[i];
            slots[i]=nc_network_slot(item);
            events[i]=(int)nc_atomic_int_field(item,"events",1);
            if(!slots[i]||events[i]<1||events[i]>15){
                return nc_network_result("feil",NULL,"",-1,0,"invalid poll_many item");
            }
            fds[i]=slots[i]->fd;
        }
        int rc=nc_wait_many_fds(fds,events,ready,count,timeout_ms);
        NcVal *gc_roots[3]={0};int gc_sp=0,gc_root_count=0;NcGcFrame gc_frame;
        nc_gc_frame_enter(&gc_frame,gc_roots,&gc_sp,gc_roots,&gc_root_count);
        NcVal *result=nc_map_new(),*results=nc_list_new();
        gc_roots[gc_root_count++]=result;gc_roots[gc_root_count++]=results;
        int ready_count=0;
        for(int i=0;i<count;i++){
            NcVal *item_result=nc_map_new();
            gc_roots[gc_root_count++]=item_result;
            char handle[64];snprintf(handle,sizeof(handle),"socket:%d:%llu",slots[i]->id,slots[i]->generation);
            nc_index_set(item_result,nc_str("handle"),nc_str(handle));
            nc_index_set(item_result,nc_str("requested"),nc_int(events[i]));
            nc_index_set(item_result,nc_str("ready"),nc_int(rc<0?0:ready[i]));
            nc_index_set(item_result,nc_str("error_code"),nc_int(rc<0?errno:0));
            if(rc>=0&&ready[i]!=0)ready_count++;
            nc_builtin_legg_til(results,item_result);
            gc_root_count--;
        }
        nc_index_set(result,nc_str("abi"),nc_str("norscode-native-network-v1"));
        nc_index_set(result,nc_str("status"),nc_str(rc<0?"feil":ready_count?"klar":"ventende"));
        nc_index_set(result,nc_str("results"),results);
        nc_index_set(result,nc_str("ready_count"),nc_int(ready_count));
        nc_index_set(result,nc_str("error"),nc_str(rc<0?strerror(errno):""));
        nc_index_set(result,nc_str("event_backend"),nc_str(nc_readiness_backend()));
        nc_index_set(result,nc_str("batch_backend"),nc_str("poll"));
        nc_gc_frame_leave(&gc_frame);
        return result;
    }
#if !defined(NC_ENABLE_OPENSSL) && !defined(_WIN32)
    if(!strcmp(op,"tls_listen")||!strcmp(op,"tls_connect")||!strcmp(op,"tls_handshake"))return nc_network_result("feil",NULL,"",-1,0,"native TLS backend unavailable");
#endif
    if(!strcmp(op,"listen")||!strcmp(op,"tls_listen")){
        const char *host=nc_atomic_text_field(request,"host","127.0.0.1");int port=(int)nc_atomic_int_field(request,"port",0),backlog=(int)nc_atomic_int_field(request,"backlog",128);struct sockaddr_in addr;
        if(port<0||port>65535||!nc_network_addr(host,port,&addr))return nc_network_result("feil",NULL,"",-1,0,"invalid address");nc_socket_handle_t fd=socket(AF_INET,SOCK_STREAM,0);
#if defined(_WIN32)
        if(fd==INVALID_SOCKET)return nc_network_result("feil",NULL,"",-1,0,nc_socket_error_text(nc_socket_last_error()));
#else
        if(fd<0)return nc_network_result("feil",NULL,"",-1,0,nc_socket_error_text(nc_socket_last_error()));
#endif
        int yes=1;setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(const char*)&yes,sizeof(yes));nc_fd_nonblocking(fd);if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))||listen(fd,backlog)){int e=nc_socket_last_error();nc_network_socket_close(fd);return nc_network_result("feil",NULL,"",-1,0,nc_socket_error_text(e));}
        socklen_t alen=sizeof(addr);getsockname(fd,(struct sockaddr*)&addr,&alen);NcNetworkSlot *s=nc_network_adopt_fd(fd,1,0);
#if defined(NC_ENABLE_OPENSSL)
        if(s&&!strcmp(op,"tls_listen")){s->ssl_ctx=nc_tls_server_ctx(nc_atomic_text_field(request,"cert_file",""),nc_atomic_text_field(request,"key_file",""),nc_atomic_text_field(request,"client_ca_file",""),nc_atomic_text_field(request,"client_crl_file",""),nc_atomic_text_field(request,"ocsp_response_file",""),nc_atomic_int_field(request,"require_client_cert",0)!=0);s->tls_server=1;if(!s->ssl_ctx){nc_network_socket_close(s->fd);s->active=0;return nc_network_result("feil",NULL,"",-1,0,"invalid TLS certificate, key, client CA, CRL or OCSP response");}}
#elif defined(_WIN32)
        if(s&&!strcmp(op,"tls_listen")){const char *subject=nc_atomic_text_field(request,"certificate_subject","");if(!*subject){nc_network_socket_close(s->fd);s->active=0;return nc_network_result("feil",NULL,"",-1,0,"SChannel certificate_subject is required");}s->schannel_server_credentials=calloc(1,sizeof(NcwSChannelServerCredential));char tls_error[NCW_ERROR_CAP]="";if(!s->schannel_server_credentials||!ncw_schannel_server_credentials(s->schannel_server_credentials,subject,nc_atomic_int_field(request,"require_client_cert",0)!=0,tls_error,sizeof(tls_error))){free(s->schannel_server_credentials);s->schannel_server_credentials=NULL;nc_network_socket_close(s->fd);s->active=0;return nc_network_result("feil",NULL,"",-1,0,tls_error);}s->schannel_credentials_owner=1;s->tls_server=1;}
#endif
        return s?nc_network_result("listening",s,"",0,ntohs(addr.sin_port),""):nc_network_result("feil",NULL,"",-1,0,"socket capacity");
    }
    if(!strcmp(op,"udp_listen")||!strcmp(op,"udp_connect")){
        const char *host=nc_atomic_text_field(request,"host","127.0.0.1");int port=(int)nc_atomic_int_field(request,"port",0);struct sockaddr_in addr;
        if(port<0||port>65535||!nc_network_addr(host,port,&addr))return nc_network_result("feil",NULL,"",-1,0,"invalid address");
        nc_socket_handle_t fd=socket(AF_INET,SOCK_DGRAM,0);
#if defined(_WIN32)
        if(fd==INVALID_SOCKET)return nc_network_result("feil",NULL,"",-1,0,nc_socket_error_text(nc_socket_last_error()));
#else
        if(fd<0)return nc_network_result("feil",NULL,"",-1,0,nc_socket_error_text(nc_socket_last_error()));
#endif
        int bind_result=0;
        if(!strcmp(op,"udp_listen"))bind_result=bind(fd,(struct sockaddr*)&addr,sizeof(addr));
        else bind_result=connect(fd,(struct sockaddr*)&addr,sizeof(addr));
        if(bind_result){int e=nc_socket_last_error();nc_network_socket_close(fd);return nc_network_result("feil",NULL,"",-1,0,nc_socket_error_text(e));}
        nc_fd_nonblocking(fd);socklen_t alen=sizeof(addr);getsockname(fd,(struct sockaddr*)&addr,&alen);NcNetworkSlot *s=nc_network_adopt_fd(fd,!strcmp(op,"udp_listen"),0);
        if(s)s->datagram=1;
        return s?nc_network_result(!strcmp(op,"udp_listen")?"listening":"connected",s,"",0,ntohs(addr.sin_port),""):nc_network_result("feil",NULL,"",-1,0,"socket capacity");
    }
    if(!strcmp(op,"connect")||!strcmp(op,"tls_connect")){
        const char *host=nc_atomic_text_field(request,"host","");int port=(int)nc_atomic_int_field(request,"port",0);struct sockaddr_in addr;if(port<1||port>65535||!nc_network_addr(host,port,&addr))return nc_network_result("feil",NULL,"",-1,0,"invalid address");
        nc_socket_handle_t fd=socket(AF_INET,SOCK_STREAM,0);
#if defined(_WIN32)
        if(fd==INVALID_SOCKET)return nc_network_result("feil",NULL,"",-1,0,nc_socket_error_text(nc_socket_last_error()));
#else
        if(fd<0)return nc_network_result("feil",NULL,"",-1,0,nc_socket_error_text(nc_socket_last_error()));
#endif
        nc_fd_nonblocking(fd);int rc=connect(fd,(struct sockaddr*)&addr,sizeof(addr));int socket_error=rc<0?nc_socket_last_error():0;int connecting=rc<0&&nc_socket_would_block(socket_error);
        if(rc<0&&!connecting){nc_network_socket_close(fd);return nc_network_result("feil",NULL,"",-1,0,nc_socket_error_text(socket_error));}NcNetworkSlot *s=nc_network_adopt_fd(fd,0,connecting);
#if defined(NC_ENABLE_OPENSSL)
        if(s&&!strcmp(op,"tls_connect")&&!nc_tls_attach_client(s,nc_atomic_text_field(request,"hostname",host),nc_atomic_text_field(request,"ca_file",""),nc_atomic_text_field(request,"crl_file",""),nc_atomic_text_field(request,"client_cert_file",""),nc_atomic_text_field(request,"client_key_file",""),nc_atomic_int_field(request,"require_ocsp",0)!=0)){nc_network_socket_close(s->fd);s->active=0;return nc_network_result("feil",NULL,"",-1,0,"TLS trust, CRL, OCSP or client identity configuration failed");}
#elif defined(_WIN32)
        if(s&&!strcmp(op,"tls_connect")){
            if(*nc_atomic_text_field(request,"ca_file","")||*nc_atomic_text_field(request,"crl_file","")||*nc_atomic_text_field(request,"client_cert_file","")||*nc_atomic_text_field(request,"client_key_file","")||nc_atomic_int_field(request,"require_ocsp",0)!=0){nc_network_socket_close(s->fd);s->active=0;return nc_network_result("feil",NULL,"",-1,0,"SChannel custom PEM trust, CRL, OCSP and mTLS are not available");}
            s->schannel=calloc(1,sizeof(NcwSChannelClient));if(!s->schannel){nc_network_socket_close(s->fd);s->active=0;return nc_network_result("feil",NULL,"",-1,0,"out of memory");}snprintf(s->tls_hostname,sizeof(s->tls_hostname),"%s",nc_atomic_text_field(request,"hostname",host));snprintf(s->tls_client_certificate_subject,sizeof(s->tls_client_certificate_subject),"%s",nc_atomic_text_field(request,"client_certificate_subject",""));
        }
#endif
        return s?nc_network_result(connecting?"connecting":"connected",s,"",0,port,""):nc_network_result("feil",NULL,"",-1,0,"socket capacity");
    }
    NcNetworkSlot *s=nc_network_slot(request);if(!s)return nc_network_result("feil",NULL,"",-1,0,"invalid handle");pthread_mutex_lock(&s->mutex);
#if defined(NC_ENABLE_OPENSSL)
    if(!strcmp(op,"tls_handshake")){
        if(s->connecting){int ready=0;if(nc_wait_fd(s->fd,2,0,&ready)<=0||!(ready&2)){pthread_mutex_unlock(&s->mutex);return nc_network_result("ventende",s,"",2,0,"");}int err=0;socklen_t n=sizeof(err);getsockopt(s->fd,SOL_SOCKET,SO_ERROR,&err,&n);if(err){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,strerror(err));}s->connecting=0;}
        int wanted=0;char tls_error[256]="";int step=nc_tls_handshake_step(s,&wanted,tls_error,sizeof(tls_error));NcVal *r=step>0?nc_network_result("connected",s,"",0,0,""):step==0?nc_network_result("ventende",s,"",wanted,0,""):nc_network_result("feil",s,"",-1,0,tls_error);pthread_mutex_unlock(&s->mutex);return r;
    }
#elif defined(_WIN32)
    if(!strcmp(op,"tls_handshake")){
        if(!s->schannel&&!s->schannel_server){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,"SChannel state missing");}
        if(s->connecting){int ready=0;if(nc_wait_fd(s->fd,2,0,&ready)<=0||!(ready&2)){pthread_mutex_unlock(&s->mutex);return nc_network_result("ventende",s,"",2,0,"");}int socket_error=0;socklen_t length=sizeof(socket_error);getsockopt(s->fd,SOL_SOCKET,SO_ERROR,(char *)&socket_error,&length);if(socket_error){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,nc_socket_error_text(socket_error));}s->connecting=0;}
        if((s->schannel&&s->schannel->handshake_done)||(s->schannel_server&&s->schannel_server->handshake_done)){pthread_mutex_unlock(&s->mutex);return nc_network_result("connected",s,"",0,0,"");}char tls_error[NCW_ERROR_CAP]="";NcwSocket socket={s->fd};int ok=s->schannel?ncw_schannel_client_handshake(s->schannel,&socket,s->tls_hostname,s->tls_client_certificate_subject,tls_error,sizeof(tls_error)):ncw_schannel_server_handshake(s->schannel_server,s->schannel_server_credentials,&socket,tls_error,sizeof(tls_error));NcVal *result=ok?nc_network_result("connected",s,"",0,0,""):nc_network_result("feil",s,"",-1,0,tls_error);pthread_mutex_unlock(&s->mutex);return result;
    }
#endif
    if(!strcmp(op,"poll")){
        int events=(int)nc_atomic_int_field(request,"events",1);int timeout_ms=(int)nc_atomic_int_field(request,"timeout_ms",0);if(timeout_ms<0)timeout_ms=0;if(timeout_ms>60000)timeout_ms=60000;int ready=0;int rc=nc_wait_fd(s->fd,events,timeout_ms,&ready);
        if(s->connecting&&ready&2){int err=0;socklen_t n=sizeof(err);getsockopt(s->fd,SOL_SOCKET,SO_ERROR,(char*)&err,&n);if(err){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,nc_socket_error_text(err));}s->connecting=0;}
        NcVal *r=rc<0?nc_network_result("feil",s,"",-1,0,nc_socket_error_text(nc_socket_last_error())):nc_network_result(ready?"klar":"ventende",s,"",ready,0,"");pthread_mutex_unlock(&s->mutex);return r;
    }
    if(!strcmp(op,"accept")){nc_socket_handle_t fd=accept(s->fd,NULL,NULL);
#if defined(_WIN32)
        int accept_failed=fd==INVALID_SOCKET;
#else
        int accept_failed=fd<0;
#endif
        if(accept_failed){int error=nc_socket_last_error();NcVal *r=nc_socket_would_block(error)?nc_network_result("ventende",s,"",0,0,""):nc_network_result("feil",s,"",-1,0,nc_socket_error_text(error));pthread_mutex_unlock(&s->mutex);return r;}nc_fd_nonblocking(fd);NcNetworkSlot *child=nc_network_adopt_fd(fd,0,0);
#if defined(NC_ENABLE_OPENSSL)
        if(child&&s->ssl_ctx){SSL_CTX_up_ref(s->ssl_ctx);child->ssl_ctx=s->ssl_ctx;child->ssl=SSL_new(child->ssl_ctx);if(!child->ssl||SSL_set_fd(child->ssl,(int)child->fd)!=1){if(child->ssl)SSL_free(child->ssl);SSL_CTX_free(child->ssl_ctx);nc_network_socket_close(child->fd);pthread_mutex_lock(&g_network_registry_lock);child->active=0;pthread_mutex_unlock(&g_network_registry_lock);pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",NULL,"",-1,0,"TLS session initialization failed");}SSL_set_accept_state(child->ssl);child->tls_server=1;}
#elif defined(_WIN32)
        if(child&&s->schannel_server_credentials){child->schannel_server_credentials=s->schannel_server_credentials;InterlockedIncrement(&child->schannel_server_credentials->references);child->schannel_server=calloc(1,sizeof(NcwSChannelServer));child->tls_server=1;if(!child->schannel_server){InterlockedDecrement(&child->schannel_server_credentials->references);child->schannel_server_credentials=NULL;nc_network_socket_close(child->fd);pthread_mutex_lock(&g_network_registry_lock);child->active=0;pthread_mutex_unlock(&g_network_registry_lock);pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",NULL,"",-1,0,"out of memory");}}
#endif
        NcVal *r=child?nc_network_result("connected",child,"",0,0,""):nc_network_result("feil",NULL,"",-1,0,"socket capacity");pthread_mutex_unlock(&s->mutex);return r;}
    if(!strcmp(op,"recvfrom")){
        long long max=nc_atomic_int_field(request,"max",65536);
        if(max<1||max>1048576){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,"invalid datagram size");}
        unsigned char *buf=calloc((size_t)max,1);struct sockaddr_in peer;memset(&peer,0,sizeof(peer));socklen_t peer_len=sizeof(peer);
        ssize_t n=recvfrom(s->fd,(char*)buf,(size_t)max,0,(struct sockaddr*)&peer,&peer_len);int socket_error=n<0?nc_socket_last_error():0;
        if(n<0&&nc_socket_would_block(socket_error)){free(buf);pthread_mutex_unlock(&s->mutex);return nc_network_result("ventende",s,"",0,0,"");}
        if(n<0){free(buf);pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,nc_socket_error_text(socket_error));}
        char *hex=nc_hex_encode(buf,(size_t)n);free(buf);if(!hex){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,"out of memory");}
        char peer_text[INET6_ADDRSTRLEN]="";inet_ntop(AF_INET,&peer.sin_addr,peer_text,sizeof(peer_text));
        NcVal *r=nc_network_result("ok",s,"",n,ntohs(peer.sin_port),"");nc_index_set(r,nc_str("data_hex"),nc_str_own(hex));nc_index_set(r,nc_str("peer_host"),nc_str(peer_text));nc_index_set(r,nc_str("peer_port"),nc_int(ntohs(peer.sin_port)));pthread_mutex_unlock(&s->mutex);return r;
    }
    if(!strcmp(op,"sendto")){
        const char *hex=nc_atomic_text_field(request,"data_hex","");size_t data_len=0;unsigned char *data=nc_hex_decode(hex,&data_len);
        const char *host=nc_atomic_text_field(request,"peer_host","");int port=(int)nc_atomic_int_field(request,"peer_port",0);struct sockaddr_in peer;
        if(!data||data_len==0||((host[0] != '\0') && (port<1||port>65535||!nc_network_addr(host,port,&peer)))){free(data);pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,"invalid UDP peer or hex payload");}
        ssize_t n=host[0] == '\0' ? send(s->fd,(const char*)data,data_len,0) : sendto(s->fd,(const char*)data,data_len,0,(struct sockaddr*)&peer,sizeof(peer));int socket_error=n<0?nc_socket_last_error():0;free(data);
        NcVal *r=n<0&&nc_socket_would_block(socket_error)?nc_network_result("ventende",s,"",0,port,""):n<0?nc_network_result("feil",s,"",-1,0,nc_socket_error_text(socket_error)):nc_network_result("ok",s,"",n,port,"");pthread_mutex_unlock(&s->mutex);return r;
    }
    if(!strcmp(op,"read_hex")){long long max=nc_atomic_int_field(request,"max",65536);if(max<1||max>1048576){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,"invalid read size");}unsigned char *buf=calloc((size_t)max,1);ssize_t n=recv(s->fd,(char*)buf,(size_t)max,0);int socket_error=n<0?nc_socket_last_error():0;if(n<0&&nc_socket_would_block(socket_error)){free(buf);pthread_mutex_unlock(&s->mutex);return nc_network_result("ventende",s,"",0,0,"");}if(n<0){free(buf);pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,nc_socket_error_text(socket_error));}char *hex=nc_hex_encode(buf,(size_t)n);free(buf);if(!hex){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,"out of memory");}NcVal *r=nc_network_result(n==0?"eof":"ok",s,"",n,0,"");nc_index_set(r,nc_str("data_hex"),nc_str_own(hex));pthread_mutex_unlock(&s->mutex);return r;}
    if(!strcmp(op,"write_hex")){const char *hex=nc_atomic_text_field(request,"data_hex","");size_t data_len=0;unsigned char *data=nc_hex_decode(hex,&data_len);if(!data||data_len==0){free(data);pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,"invalid hex payload");}ssize_t n=send(s->fd,(const char*)data,data_len,0);int socket_error=n<0?nc_socket_last_error():0;free(data);NcVal *r=n<0&&nc_socket_would_block(socket_error)?nc_network_result("ventende",s,"",0,0,""):n<0?nc_network_result("feil",s,"",-1,0,nc_socket_error_text(socket_error)):nc_network_result("ok",s,"",n,0,"");pthread_mutex_unlock(&s->mutex);return r;}
    if(!strcmp(op,"read")){long long max=nc_atomic_int_field(request,"max",65536);if(max<1||max>1048576){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,"invalid read size");}char *buf=calloc((size_t)max+1,1);ssize_t n;
#if defined(NC_ENABLE_OPENSSL)
        if(s->ssl){n=SSL_read(s->ssl,buf,(int)max);if(n<=0){int e=SSL_get_error(s->ssl,(int)n);if(e==SSL_ERROR_WANT_READ||e==SSL_ERROR_WANT_WRITE){free(buf);pthread_mutex_unlock(&s->mutex);return nc_network_result("ventende",s,"",0,0,"");}n=-1;errno=EIO;}}
        else
#elif defined(_WIN32)
        if(s->schannel||s->schannel_server){char tls_error[NCW_ERROR_CAP]="";NcwSocket socket={s->fd};n=s->schannel?ncw_schannel_client_read(s->schannel,&socket,buf,(size_t)max,tls_error,sizeof(tls_error)):ncw_schannel_server_read(s->schannel_server,&socket,buf,(size_t)max,tls_error,sizeof(tls_error));if(n==-2){free(buf);pthread_mutex_unlock(&s->mutex);return nc_network_result("ventende",s,"",0,0,"");}if(n<0){free(buf);pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,tls_error);}}else
#endif
        n=recv(s->fd,buf,(size_t)max,0);int socket_error=n<0?nc_socket_last_error():0;NcVal *r=n<0&&nc_socket_would_block(socket_error)?nc_network_result("ventende",s,"",0,0,""):n<0?nc_network_result("feil",s,"",-1,0,nc_socket_error_text(socket_error)):nc_network_result(n==0?"eof":"ok",s,buf,n,0,"");free(buf);pthread_mutex_unlock(&s->mutex);return r;}
    if(!strcmp(op,"write")){const char *data=nc_atomic_text_field(request,"data","");ssize_t n;
#if defined(NC_ENABLE_OPENSSL)
        if(s->ssl){n=SSL_write(s->ssl,data,(int)strlen(data));if(n<=0){int e=SSL_get_error(s->ssl,(int)n);if(e==SSL_ERROR_WANT_READ||e==SSL_ERROR_WANT_WRITE){pthread_mutex_unlock(&s->mutex);return nc_network_result("ventende",s,"",0,0,"");}n=-1;errno=EIO;}}
        else
#elif defined(_WIN32)
        if(s->schannel||s->schannel_server){char tls_error[NCW_ERROR_CAP]="";NcwSocket socket={s->fd};n=s->schannel?ncw_schannel_client_write(s->schannel,&socket,data,strlen(data),tls_error,sizeof(tls_error)):ncw_schannel_server_write(s->schannel_server,&socket,data,strlen(data),tls_error,sizeof(tls_error));if(n<0){pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,tls_error);}}else
#endif
        n=send(s->fd,data,strlen(data),0);int socket_error=n<0?nc_socket_last_error():0;NcVal *r=n<0&&nc_socket_would_block(socket_error)?nc_network_result("ventende",s,"",0,0,""):n<0?nc_network_result("feil",s,"",-1,0,nc_socket_error_text(socket_error)):nc_network_result("ok",s,"",n,0,"");pthread_mutex_unlock(&s->mutex);return r;}
    if(!strcmp(op,"close")){
#if defined(NC_ENABLE_OPENSSL)
        if(s->ssl){SSL_shutdown(s->ssl);SSL_free(s->ssl);s->ssl=NULL;}if(s->ssl_ctx){SSL_CTX_free(s->ssl_ctx);s->ssl_ctx=NULL;}
#elif defined(_WIN32)
        if(s->schannel){char tls_error[NCW_ERROR_CAP]="";ncw_schannel_client_close(s->schannel,tls_error,sizeof(tls_error));free(s->schannel);s->schannel=NULL;}
        if(s->schannel_server){char tls_error[NCW_ERROR_CAP]="";ncw_schannel_server_close(s->schannel_server,tls_error,sizeof(tls_error));free(s->schannel_server);s->schannel_server=NULL;}
        if(s->schannel_server_credentials){NcwSChannelServerCredential *credentials=s->schannel_server_credentials;s->schannel_server_credentials=NULL;if(InterlockedDecrement(&credentials->references)==0){char tls_error[NCW_ERROR_CAP]="";ncw_schannel_server_credentials_close(credentials,tls_error,sizeof(tls_error));free(credentials);}s->schannel_credentials_owner=0;}
#endif
#if defined(_WIN32)
        if(s->iocp_owned){char close_error[NCW_ERROR_CAP]="";ncw_socket_close(&s->iocp_socket,close_error,sizeof(close_error));s->fd=INVALID_SOCKET;}else nc_network_socket_close(s->fd);
#else
        nc_network_socket_close(s->fd);
#endif
        NcVal *r=nc_network_result("closed",s,"",0,0,"");pthread_mutex_lock(&g_network_registry_lock);s->active=0;pthread_mutex_unlock(&g_network_registry_lock);pthread_mutex_unlock(&s->mutex);pthread_mutex_destroy(&s->mutex);return r;}
    pthread_mutex_unlock(&s->mutex);return nc_network_result("feil",s,"",-1,0,"invalid operation");
}

#define NC_FILESYSTEM_MAX 128
typedef struct {
    int active, id;
#if defined(_WIN32)
    NcwFile file;
#else
    int fd;
#endif
    unsigned long long generation; char mode[16]; _Atomic int pending_async; pthread_mutex_t mutex;
} NcFilesystemSlot;
static NcFilesystemSlot g_filesystem_slots[NC_FILESYSTEM_MAX];
static pthread_mutex_t g_filesystem_registry_lock = PTHREAD_MUTEX_INITIALIZER;

static NcVal *nc_filesystem_result(const char *status, NcFilesystemSlot *slot,
                                   const char *data, long long value, const char *error) {
    NcVal *r = nc_map_new(); char handle[64] = "";
    if (slot) snprintf(handle, sizeof(handle), "file:%d:%llu", slot->id, slot->generation);
    nc_index_set(r, nc_str("abi"), nc_str("norscode-native-filesystem-v1"));
    nc_index_set(r, nc_str("status"), nc_str(status)); nc_index_set(r, nc_str("handle"), nc_str(handle));
    nc_index_set(r, nc_str("data"), nc_str(data ? data : "")); nc_index_set(r, nc_str("value"), nc_int(value));
    nc_index_set(r, nc_str("error"), nc_str(error ? error : ""));
    nc_index_set(r, nc_str("event_backend"), nc_str(nc_readiness_backend())); return r;
}

static int nc_safe_component(const char *s) {
    size_t n = s ? strlen(s) : 0;
    return n > 0 && n <= 255 && strcmp(s, ".") && strcmp(s, "..") && !strchr(s, '\\') && !strchr(s, '\n') && !strchr(s, '\r');
}

#if !defined(_WIN32)
static int nc_open_root_dir(const char *root) {
    if (!root || !*root || root[0] == '/') { errno = EINVAL; return -1; }
    char *copy = strdup(root), *save = NULL, *part = strtok_r(copy, "/", &save);
    int fd = open(".", O_RDONLY | O_DIRECTORY);
    while (fd >= 0 && part) {
        if (!nc_safe_component(part)) { close(fd); fd = -1; errno = EINVAL; break; }
        int next = openat(fd, part, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
        close(fd); fd = next; part = strtok_r(NULL, "/", &save);
    }
    free(copy); return fd;
}

static int nc_open_beneath(const char *root, const char *relative, int flags, mode_t create_mode) {
    int dirfd = nc_open_root_dir(root);
    if (dirfd < 0 || !relative || !*relative || relative[0] == '/') { if (dirfd >= 0) close(dirfd); errno = EINVAL; return -1; }
    char *copy = strdup(relative), *save = NULL, *part = strtok_r(copy, "/", &save);
    if (!part) { close(dirfd); free(copy); errno = EINVAL; return -1; }
    for (;;) {
        if (!nc_safe_component(part)) { close(dirfd); free(copy); errno = EINVAL; return -1; }
        char *next_part = strtok_r(NULL, "/", &save);
        if (!next_part) {
            int fd = openat(dirfd, part, flags | O_NOFOLLOW, create_mode);
            close(dirfd); free(copy); return fd;
        }
        int next = openat(dirfd, part, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
        close(dirfd); dirfd = next;
        if (dirfd < 0) { free(copy); return -1; }
        part = next_part;
    }
}
#endif

static NcFilesystemSlot *nc_filesystem_slot(NcVal *request) {
    const char *handle = nc_atomic_text_field(request, "handle", "");
    if (strncmp(handle, "file:", 5)) return NULL;
    long id=0;unsigned long long generation=0;char trailing='\0';
    if(sscanf(handle,"file:%ld:%llu%c",&id,&generation,&trailing)!=2||id<=0||id>=NC_FILESYSTEM_MAX||generation==0)return NULL;
    pthread_mutex_lock(&g_filesystem_registry_lock);
    NcFilesystemSlot *slot = g_filesystem_slots[id].active == 1 && g_filesystem_slots[id].generation == generation ? &g_filesystem_slots[id] : NULL;
    pthread_mutex_unlock(&g_filesystem_registry_lock); return slot;
}

#if !defined(_WIN32)
static int nc_unlink_beneath(const char *root, const char *relative) {
    int dirfd=nc_open_root_dir(root); if(dirfd<0||!relative||!*relative||relative[0]=='/'){if(dirfd>=0)close(dirfd);errno=EINVAL;return -1;}
    char *copy=strdup(relative),*save=NULL,*part=strtok_r(copy,"/",&save); if(!part){close(dirfd);free(copy);errno=EINVAL;return -1;}
    for(;;){
        if(!nc_safe_component(part)){close(dirfd);free(copy);errno=EINVAL;return -1;}
        char *next_part=strtok_r(NULL,"/",&save);
        if(!next_part){int rc=unlinkat(dirfd,part,0);close(dirfd);free(copy);return rc;}
        int next=openat(dirfd,part,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);close(dirfd);dirfd=next;if(dirfd<0){free(copy);return -1;}part=next_part;
    }
}
#endif

static NcVal *nc_builtin_filesystem_operation(NcVal *request) {
    if (!request || request->type != NC_MAP || strcmp(nc_atomic_text_field(request, "abi", ""), "norscode-native-filesystem-v1")) return nc_filesystem_result("feil", NULL, "", -1, "invalid request");
    const char *operation = nc_atomic_text_field(request, "operation", "");
    if (!strcmp(operation, "mkdir_p")) {
        const char *root = nc_atomic_text_field(request, "root", ".");
        const char *relative = nc_atomic_text_field(request, "relative", "");
        if (!*root || !*relative || relative[0] == '/' || strstr(relative, "..")) return nc_filesystem_result("feil", NULL, "", -1, "invalid directory path");
        size_t root_len = strlen(root), relative_len = strlen(relative);
        char *path = malloc(root_len + relative_len + 2);
        if (!path) return nc_filesystem_result("feil", NULL, "", -1, "out of memory");
        snprintf(path, root_len + relative_len + 2, "%s/%s", root, relative);
        NcVal *created = nc_builtin_mkdir_p(nc_str_own(path));
        (void)created;
        return nc_filesystem_result("ok", NULL, "", 1, "");
    }
    if (!strcmp(operation,"delete")) {
        const char *root=nc_atomic_text_field(request,"root",""),*relative=nc_atomic_text_field(request,"relative","");
#if defined(_WIN32)
        char backend_error[NCW_ERROR_CAP]=""; int rc=ncw_file_delete(root,relative,backend_error,sizeof(backend_error)); return !rc?nc_filesystem_result("feil",NULL,"",-1,backend_error):nc_filesystem_result("deleted",NULL,"",0,"");
#else
        int rc=nc_unlink_beneath(root,relative); return rc?nc_filesystem_result("feil",NULL,"",-1,strerror(errno)):nc_filesystem_result("deleted",NULL,"",0,"");
#endif
    }
    if (!strcmp(operation, "open")) {
        const char *root = nc_atomic_text_field(request, "root", ""), *relative = nc_atomic_text_field(request, "relative", ""), *mode = nc_atomic_text_field(request, "mode", "");
        if (strcmp(mode,"read")&&strcmp(mode,"write")&&strcmp(mode,"append")&&strcmp(mode,"readwrite")) return nc_filesystem_result("feil", NULL, "", -1, "invalid mode");
        pthread_mutex_lock(&g_filesystem_registry_lock); int id=0; for(int i=1;i<NC_FILESYSTEM_MAX;i++) if(!g_filesystem_slots[i].active){id=i;g_filesystem_slots[i].generation++;if(!g_filesystem_slots[i].generation)g_filesystem_slots[i].generation=1;g_filesystem_slots[i].active=2;break;} pthread_mutex_unlock(&g_filesystem_registry_lock);
        if (!id) return nc_filesystem_result("feil", NULL, "", -1, "file capacity");
        NcFilesystemSlot *slot=&g_filesystem_slots[id]; slot->id=id;
#if defined(_WIN32)
        char backend_error[NCW_ERROR_CAP]=""; if(!ncw_file_open(&slot->file,root,relative,mode,backend_error,sizeof(backend_error))){pthread_mutex_lock(&g_filesystem_registry_lock);slot->active=0;pthread_mutex_unlock(&g_filesystem_registry_lock);return nc_filesystem_result("feil",NULL,"",-1,backend_error);}
#else
        int flags = !strcmp(mode,"read") ? O_RDONLY : !strcmp(mode,"write") ? O_WRONLY|O_CREAT|O_TRUNC : !strcmp(mode,"append") ? O_WRONLY|O_CREAT|O_APPEND : O_RDWR|O_CREAT;
        int fd = nc_open_beneath(root, relative, flags, 0600);
        if (fd < 0) { pthread_mutex_lock(&g_filesystem_registry_lock); slot->active=0; pthread_mutex_unlock(&g_filesystem_registry_lock); return nc_filesystem_result("feil", NULL, "", -1, strerror(errno)); }
        slot->fd=fd;
#endif
        snprintf(slot->mode,sizeof(slot->mode),"%s",mode); atomic_store_explicit(&slot->pending_async,0,memory_order_release); pthread_mutex_init(&slot->mutex,NULL);
        pthread_mutex_lock(&g_filesystem_registry_lock); slot->active=1; pthread_mutex_unlock(&g_filesystem_registry_lock); return nc_filesystem_result("open",slot,"",0,"");
    }
    NcFilesystemSlot *slot=nc_filesystem_slot(request); if(!slot) return nc_filesystem_result("feil",NULL,"",-1,"invalid handle");
    pthread_mutex_lock(&slot->mutex);
#if defined(_WIN32)
    if(!strcmp(operation,"read_async")||!strcmp(operation,"write_async")){
        pthread_once(&g_network_once,nc_network_init_once);if(!g_network_iocp_ready){pthread_mutex_unlock(&slot->mutex);return nc_filesystem_result("feil",slot,"",-1,"IOCP unavailable");}
        int kind=!strcmp(operation,"read_async")?5:6;long long offset=nc_atomic_int_field(request,"offset",0);const char *data=nc_atomic_text_field(request,"data","");long long requested=kind==5?nc_atomic_int_field(request,"max",65536):(long long)strlen(data);
        if(offset<0||requested<1||requested>1048576){pthread_mutex_unlock(&slot->mutex);return nc_filesystem_result("feil",slot,"",-1,"invalid async file request");}NcIocpOperationSlot *entry=nc_iocp_operation_new(kind,NULL,(size_t)requested);if(!entry){pthread_mutex_unlock(&slot->mutex);return nc_filesystem_result("feil",slot,"",-1,"operation capacity");}if(kind==6)memcpy(entry->buffer,data,(size_t)requested);char backend_error[NCW_ERROR_CAP]="";int submitted=kind==5?ncw_file_read_async(&g_network_iocp,&slot->file,(uint64_t)offset,entry->buffer,(size_t)requested,&entry->operation,backend_error,sizeof(backend_error)):ncw_file_write_async(&g_network_iocp,&slot->file,(uint64_t)offset,entry->buffer,(size_t)requested,&entry->operation,backend_error,sizeof(backend_error));
        entry->pending_owner=&slot->pending_async;atomic_fetch_add_explicit(&slot->pending_async,1,memory_order_acq_rel);if(!submitted){nc_iocp_operation_release(entry);pthread_mutex_unlock(&slot->mutex);return nc_filesystem_result("feil",slot,"",-1,backend_error);}char operation_handle[64];snprintf(operation_handle,sizeof(operation_handle),"iocp-op:%d:%llu",entry->id,entry->generation);NcVal *result=nc_filesystem_result("ventende",slot,"",0,"");nc_index_set(result,nc_str("operation_handle"),nc_str(operation_handle));nc_index_set(result,nc_str("operation_kind"),nc_int(kind));nc_index_set(result,nc_str("event_backend"),nc_str("iocp"));pthread_mutex_unlock(&slot->mutex);return result;
    }
    if(!strcmp(operation,"wait_async")||!strcmp(operation,"cancel_async")){
        NcVal *result=nc_builtin_iocp_operation(request,!strcmp(operation,"wait_async")?"iocp_wait":"iocp_cancel");nc_index_set(result,nc_str("abi"),nc_str("norscode-native-filesystem-v1"));char handle[64];snprintf(handle,sizeof(handle),"file:%d:%llu",slot->id,slot->generation);nc_index_set(result,nc_str("handle"),nc_str(handle));pthread_mutex_unlock(&slot->mutex);return result;
    }
#endif
    if (!strcmp(operation,"ready")) {
#if defined(_WIN32)
        NcVal *r=nc_filesystem_result("ok",slot,"",1,""); pthread_mutex_unlock(&slot->mutex); return r;
#else
        const char *events=nc_atomic_text_field(request,"events","read"); int ready_bits=0;
        int rc=nc_wait_fd(slot->fd,!strcmp(events,"write")?2:1,0,&ready_bits); long long ready=rc>0&&ready_bits!=0;
        NcVal *r=rc<0?nc_filesystem_result("feil",slot,"",-1,strerror(errno)):nc_filesystem_result("ok",slot,"",ready,""); pthread_mutex_unlock(&slot->mutex); return r;
#endif
    }
    if (!strcmp(operation,"read")) {
        long long max=nc_atomic_int_field(request,"max",65536); if(max<1||max>1048576){pthread_mutex_unlock(&slot->mutex);return nc_filesystem_result("feil",slot,"",-1,"invalid read size");}
        char *buf=calloc((size_t)max+1,1);
#if defined(_WIN32)
        char backend_error[NCW_ERROR_CAP]=""; int64_t n=ncw_file_read(&slot->file,buf,(size_t)max,backend_error,sizeof(backend_error)); NcVal *r=n<0?nc_filesystem_result("feil",slot,"",-1,backend_error):nc_filesystem_result("ok",slot,buf,n,"");
#else
        ssize_t n=read(slot->fd,buf,(size_t)max); NcVal *r=n<0?nc_filesystem_result("feil",slot,"",-1,strerror(errno)):nc_filesystem_result("ok",slot,buf,n,"");
#endif
        free(buf); pthread_mutex_unlock(&slot->mutex); return r;
    }
    if (!strcmp(operation,"write")) {
        const char *data=nc_atomic_text_field(request,"data","");
#if defined(_WIN32)
        char backend_error[NCW_ERROR_CAP]=""; int64_t n=ncw_file_write(&slot->file,data,strlen(data),backend_error,sizeof(backend_error)); NcVal *r=n<0?nc_filesystem_result("feil",slot,"",-1,backend_error):nc_filesystem_result("ok",slot,"",n,"");
#else
        ssize_t n=write(slot->fd,data,strlen(data)); NcVal *r=n<0?nc_filesystem_result("feil",slot,"",-1,strerror(errno)):nc_filesystem_result("ok",slot,"",n,"");
#endif
        pthread_mutex_unlock(&slot->mutex); return r;
    }
    if (!strcmp(operation,"seek")) {
        long long pos=nc_atomic_int_field(request,"position",-1);
#if defined(_WIN32)
        char backend_error[NCW_ERROR_CAP]=""; int64_t out=pos<0?-1:ncw_file_seek(&slot->file,pos,backend_error,sizeof(backend_error)); NcVal *r=out<0?nc_filesystem_result("feil",slot,"",-1,backend_error):nc_filesystem_result("ok",slot,"",out,"");
#else
        off_t out=pos<0?-1:lseek(slot->fd,(off_t)pos,SEEK_SET); NcVal *r=out<0?nc_filesystem_result("feil",slot,"",-1,strerror(errno)):nc_filesystem_result("ok",slot,"",out,"");
#endif
        pthread_mutex_unlock(&slot->mutex); return r;
    }
    if (!strcmp(operation,"flush")) {
#if defined(_WIN32)
        char backend_error[NCW_ERROR_CAP]=""; int rc=ncw_file_flush(&slot->file,backend_error,sizeof(backend_error)); NcVal *r=!rc?nc_filesystem_result("feil",slot,"",-1,backend_error):nc_filesystem_result("ok",slot,"",0,"");
#else
        int rc=fsync(slot->fd); NcVal *r=rc?nc_filesystem_result("feil",slot,"",-1,strerror(errno)):nc_filesystem_result("ok",slot,"",0,"");
#endif
        pthread_mutex_unlock(&slot->mutex); return r; }
    if (!strcmp(operation,"stat")) {
#if defined(_WIN32)
        char backend_error[NCW_ERROR_CAP]=""; int64_t size=ncw_file_size(&slot->file,backend_error,sizeof(backend_error)); NcVal *r=size<0?nc_filesystem_result("feil",slot,"",-1,backend_error):nc_filesystem_result("ok",slot,"",size,"");
#else
        struct stat st; int rc=fstat(slot->fd,&st); NcVal *r=rc?nc_filesystem_result("feil",slot,"",-1,strerror(errno)):nc_filesystem_result("ok",slot,"",(long long)st.st_size,"");
#endif
        pthread_mutex_unlock(&slot->mutex); return r; }
    if (!strcmp(operation,"close")) {
#if defined(_WIN32)
        if(atomic_load_explicit(&slot->pending_async,memory_order_acquire)>0){pthread_mutex_unlock(&slot->mutex);return nc_filesystem_result("feil",slot,"",-1,"file has pending async operations");}
        char backend_error[NCW_ERROR_CAP]=""; int rc=ncw_file_close(&slot->file,backend_error,sizeof(backend_error)); NcVal *r=!rc?nc_filesystem_result("feil",slot,"",-1,backend_error):nc_filesystem_result("closed",slot,"",0,"");
#else
        close(slot->fd); NcVal *r=nc_filesystem_result("closed",slot,"",0,"");
#endif
        pthread_mutex_lock(&g_filesystem_registry_lock);slot->active=0;pthread_mutex_unlock(&g_filesystem_registry_lock);pthread_mutex_unlock(&slot->mutex);pthread_mutex_destroy(&slot->mutex);return r; }
    pthread_mutex_unlock(&slot->mutex); return nc_filesystem_result("feil",slot,"",-1,"invalid operation");
}

static NcVal *nc_builtin_filesystem_read_operation(NcVal *request) {
    const char *operation=nc_atomic_text_field(request,"operation","");
    if (!strcmp(operation,"open") && strcmp(nc_atomic_text_field(request,"mode",""),"read")) return nc_filesystem_result("feil",NULL,"",-1,"write mode requires disk.write");
    if (strcmp(operation,"open") && strcmp(operation,"read") && strcmp(operation,"read_async") && strcmp(operation,"wait_async") && strcmp(operation,"cancel_async") && strcmp(operation,"ready") && strcmp(operation,"seek") && strcmp(operation,"stat") && strcmp(operation,"close")) return nc_filesystem_result("feil",NULL,"",-1,"operation requires disk.write");
    return nc_builtin_filesystem_operation(request);
}

static NcVal *nc_builtin_filesystem_write_operation(NcVal *request) {
    const char *operation=nc_atomic_text_field(request,"operation","");
    if (!strcmp(operation,"open") && !strcmp(nc_atomic_text_field(request,"mode",""),"read")) return nc_filesystem_result("feil",NULL,"",-1,"read mode requires disk.read");
    if (strcmp(operation,"open") && strcmp(operation,"write") && strcmp(operation,"write_async") && strcmp(operation,"wait_async") && strcmp(operation,"cancel_async") && strcmp(operation,"ready") && strcmp(operation,"seek") && strcmp(operation,"flush") && strcmp(operation,"close") && strcmp(operation,"delete") && strcmp(operation,"mkdir_p")) return nc_filesystem_result("feil",NULL,"",-1,"operation requires disk.read");
    return nc_builtin_filesystem_operation(request);
}

#if !defined(_WIN32)
#define NC_PROCESS_MAX 64
typedef struct {
    int active, id;
    unsigned long long generation;
    pid_t pid;
    int stdin_fd, stdout_fd, stderr_fd;
    char *stdin_data, *stdout_data, *stderr_data;
    size_t stdin_len, stdin_off, stdout_len, stderr_len, output_total, max_output;
    unsigned long long max_memory_bytes;
    long long deadline_ms;
    int exited, wait_status, timed_out, output_limited, memory_limited;
    char sandbox_profile[32];
    pthread_mutex_t mutex;
} NcProcessSlot;
static NcProcessSlot g_process_slots[NC_PROCESS_MAX];
static pthread_mutex_t g_process_registry_lock = PTHREAD_MUTEX_INITIALIZER;

static NcVal *nc_process_async_result(NcProcessSlot *slot, const char *error) {
    NcVal *r = nc_map_new();
    char handle[64] = "";
    const char *status = "feil";
    int exit_code = -1;
    if (slot) {
        snprintf(handle, sizeof(handle), "process:%d:%llu", slot->id, slot->generation);
        status = slot->exited ? "ferdig" : "kjorer";
        if (slot->memory_limited) { status = "feil"; exit_code = 126; }
        else if (slot->timed_out) { status = "timeout"; exit_code = 124; }
        else if (slot->output_limited) { status = "feil"; exit_code = 125; }
        else if (slot->exited && WIFSIGNALED(slot->wait_status)) { status = "signal"; exit_code = 128 + WTERMSIG(slot->wait_status); }
        else if (slot->exited && WIFEXITED(slot->wait_status)) exit_code = WEXITSTATUS(slot->wait_status);
    }
    nc_index_set(r, nc_str("abi"), nc_str("norscode-native-process-v1"));
    nc_index_set(r, nc_str("status"), nc_str(status));
    nc_index_set(r, nc_str("handle"), nc_str(handle));
    nc_index_set(r, nc_str("pid"), nc_int(slot ? (long long)slot->pid : -1));
    nc_index_set(r, nc_str("exit_code"), nc_int(exit_code));
    nc_index_set(r, nc_str("stdout"), nc_str(slot && slot->stdout_data ? slot->stdout_data : ""));
    nc_index_set(r, nc_str("stderr"), nc_str(slot && slot->stderr_data ? slot->stderr_data : ""));
    nc_index_set(r, nc_str("error"), nc_str(error && *error ? error : (slot && slot->memory_limited ? "memory limit" : slot && slot->output_limited ? "output limit" : slot && slot->timed_out ? "timeout" : "")));
    nc_index_set(r, nc_str("sandbox"), nc_str(slot ? slot->sandbox_profile : "none"));
#if defined(__linux__)
    nc_index_set(r, nc_str("sandbox_backend"), nc_str(slot && strcmp(slot->sandbox_profile, "none") ? "seccomp" : "none"));
#elif defined(__APPLE__)
    nc_index_set(r, nc_str("sandbox_backend"), nc_str(slot && strcmp(slot->sandbox_profile, "none") ? "seatbelt" : "none"));
#else
    nc_index_set(r, nc_str("sandbox_backend"), nc_str(slot && strcmp(slot->sandbox_profile, "none") ? "rlimit" : "none"));
#endif
    return r;
}

#if defined(__linux__)
static void nc_seccomp_deny(struct sock_filter *filter, size_t *count, int syscall_number) {
    filter[(*count)++] = (struct sock_filter)BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (unsigned int)syscall_number, 0, 1);
    filter[(*count)++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA));
}

static void nc_seccomp_deny_write_open(struct sock_filter *filter, size_t *count,
                                       int syscall_number, unsigned int flags_arg) {
    unsigned int write_flags = O_WRONLY | O_RDWR | O_CREAT | O_TRUNC | O_APPEND;
    filter[(*count)++] = (struct sock_filter)BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (unsigned int)syscall_number, 0, 3);
    filter[(*count)++] = (struct sock_filter)BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
        (unsigned int)(offsetof(struct seccomp_data, args) + sizeof(((struct seccomp_data *)0)->args[0]) * flags_arg));
    filter[(*count)++] = (struct sock_filter)BPF_STMT(BPF_ALU | BPF_AND | BPF_K, write_flags);
    filter[(*count)++] = (struct sock_filter)BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0, 1, 0);
    filter[(*count)++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EACCES & SECCOMP_RET_DATA));
}

static int nc_install_seccomp(const char *profile) {
    struct sock_filter filter[256];
    size_t count = 0;
#if defined(__x86_64__)
    const unsigned int audit_arch = AUDIT_ARCH_X86_64;
#elif defined(__aarch64__)
    const unsigned int audit_arch = AUDIT_ARCH_AARCH64;
#else
#error "Norscode seccomp requires a declared Linux audit architecture"
#endif
    filter[count++] = (struct sock_filter)BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch));
    filter[count++] = (struct sock_filter)BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, audit_arch, 1, 0);
    filter[count++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS);
    filter[count++] = (struct sock_filter)BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr));

#define NC_DENY_SYSCALL(name) do { \
    /* Each syscall is guarded because Linux architectures expose different sets. */ \
    nc_seccomp_deny(filter, &count, __NR_##name); \
} while (0)
#ifdef __NR_ptrace
    NC_DENY_SYSCALL(ptrace);
#endif
#ifdef __NR_bpf
    NC_DENY_SYSCALL(bpf);
#endif
#ifdef __NR_perf_event_open
    NC_DENY_SYSCALL(perf_event_open);
#endif
#ifdef __NR_kexec_load
    NC_DENY_SYSCALL(kexec_load);
#endif
#ifdef __NR_kexec_file_load
    NC_DENY_SYSCALL(kexec_file_load);
#endif
#ifdef __NR_init_module
    NC_DENY_SYSCALL(init_module);
#endif
#ifdef __NR_finit_module
    NC_DENY_SYSCALL(finit_module);
#endif
#ifdef __NR_delete_module
    NC_DENY_SYSCALL(delete_module);
#endif
#ifdef __NR_userfaultfd
    NC_DENY_SYSCALL(userfaultfd);
#endif
#ifdef __NR_keyctl
    NC_DENY_SYSCALL(keyctl);
#endif
#ifdef __NR_add_key
    NC_DENY_SYSCALL(add_key);
#endif
#ifdef __NR_request_key
    NC_DENY_SYSCALL(request_key);
#endif
#ifdef __NR_reboot
    NC_DENY_SYSCALL(reboot);
#endif
#ifdef __NR_swapon
    NC_DENY_SYSCALL(swapon);
#endif
#ifdef __NR_swapoff
    NC_DENY_SYSCALL(swapoff);
#endif
#ifdef __NR_setns
    NC_DENY_SYSCALL(setns);
#endif
#ifdef __NR_unshare
    NC_DENY_SYSCALL(unshare);
#endif
#ifdef __NR_mount
    NC_DENY_SYSCALL(mount);
#endif
#ifdef __NR_umount2
    NC_DENY_SYSCALL(umount2);
#endif
#ifdef __NR_pivot_root
    NC_DENY_SYSCALL(pivot_root);
#endif

    int deny_network = !strcmp(profile, "no-network") || !strcmp(profile, "pure");
    int deny_write = !strcmp(profile, "no-write") || !strcmp(profile, "pure");
    if (deny_network) {
#ifdef __NR_socket
        NC_DENY_SYSCALL(socket);
#endif
#ifdef __NR_socketpair
        NC_DENY_SYSCALL(socketpair);
#endif
#ifdef __NR_connect
        NC_DENY_SYSCALL(connect);
#endif
#ifdef __NR_bind
        NC_DENY_SYSCALL(bind);
#endif
#ifdef __NR_listen
        NC_DENY_SYSCALL(listen);
#endif
#ifdef __NR_accept
        NC_DENY_SYSCALL(accept);
#endif
#ifdef __NR_accept4
        NC_DENY_SYSCALL(accept4);
#endif
#ifdef __NR_sendto
        NC_DENY_SYSCALL(sendto);
#endif
#ifdef __NR_sendmsg
        NC_DENY_SYSCALL(sendmsg);
#endif
#ifdef __NR_sendmmsg
        NC_DENY_SYSCALL(sendmmsg);
#endif
#ifdef __NR_recvfrom
        NC_DENY_SYSCALL(recvfrom);
#endif
#ifdef __NR_recvmsg
        NC_DENY_SYSCALL(recvmsg);
#endif
#ifdef __NR_recvmmsg
        NC_DENY_SYSCALL(recvmmsg);
#endif
    }
    if (deny_write) {
#ifdef __NR_open
        nc_seccomp_deny_write_open(filter, &count, __NR_open, 1);
#endif
#ifdef __NR_openat
        nc_seccomp_deny_write_open(filter, &count, __NR_openat, 2);
#endif
#ifdef __NR_openat2
        NC_DENY_SYSCALL(openat2);
#endif
#ifdef __NR_creat
        NC_DENY_SYSCALL(creat);
#endif
#ifdef __NR_unlink
        NC_DENY_SYSCALL(unlink);
#endif
#ifdef __NR_unlinkat
        NC_DENY_SYSCALL(unlinkat);
#endif
#ifdef __NR_rename
        NC_DENY_SYSCALL(rename);
#endif
#ifdef __NR_renameat
        NC_DENY_SYSCALL(renameat);
#endif
#ifdef __NR_renameat2
        NC_DENY_SYSCALL(renameat2);
#endif
#ifdef __NR_mkdir
        NC_DENY_SYSCALL(mkdir);
#endif
#ifdef __NR_mkdirat
        NC_DENY_SYSCALL(mkdirat);
#endif
#ifdef __NR_rmdir
        NC_DENY_SYSCALL(rmdir);
#endif
#ifdef __NR_link
        NC_DENY_SYSCALL(link);
#endif
#ifdef __NR_linkat
        NC_DENY_SYSCALL(linkat);
#endif
#ifdef __NR_symlink
        NC_DENY_SYSCALL(symlink);
#endif
#ifdef __NR_symlinkat
        NC_DENY_SYSCALL(symlinkat);
#endif
#ifdef __NR_truncate
        NC_DENY_SYSCALL(truncate);
#endif
#ifdef __NR_ftruncate
        NC_DENY_SYSCALL(ftruncate);
#endif
#ifdef __NR_chmod
        NC_DENY_SYSCALL(chmod);
#endif
#ifdef __NR_fchmod
        NC_DENY_SYSCALL(fchmod);
#endif
#ifdef __NR_fchmodat
        NC_DENY_SYSCALL(fchmodat);
#endif
#ifdef __NR_chown
        NC_DENY_SYSCALL(chown);
#endif
#ifdef __NR_fchown
        NC_DENY_SYSCALL(fchown);
#endif
#ifdef __NR_lchown
        NC_DENY_SYSCALL(lchown);
#endif
#ifdef __NR_fchownat
        NC_DENY_SYSCALL(fchownat);
#endif
#ifdef __NR_mknod
        NC_DENY_SYSCALL(mknod);
#endif
#ifdef __NR_mknodat
        NC_DENY_SYSCALL(mknodat);
#endif
    }
#undef NC_DENY_SYSCALL
    filter[count++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW);
    struct sock_fprog program = { .len = (unsigned short)count, .filter = filter };
    return prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0;
}
#endif

static int nc_apply_process_sandbox(const char *profile,long long timeout_ms,long long max_memory,long long max_files,int inherit_env){
    if(!profile||!*profile||!strcmp(profile,"none"))return 1;
    struct rlimit lim;lim.rlim_cur=lim.rlim_max=0;if(setrlimit(RLIMIT_CORE,&lim)!=0){dprintf(2,"rlimit core: %s",strerror(errno));return 0;}
    rlim_t cpu=(rlim_t)(timeout_ms/1000+2);lim.rlim_cur=lim.rlim_max=cpu;if(setrlimit(RLIMIT_CPU,&lim)!=0){dprintf(2,"rlimit cpu: %s",strerror(errno));return 0;}
    if(max_memory>0){lim.rlim_cur=lim.rlim_max=(rlim_t)max_memory;
#if defined(__APPLE__)
        /* Darwin exposes RLIMIT_RSS but rejects practical per-child limits; Seatbelt and the other hard limits remain active. */
        (void)lim;
#else
        if(setrlimit(RLIMIT_AS,&lim)!=0){dprintf(2,"rlimit memory: %s",strerror(errno));return 0;}
#endif
    }
    if(max_files>0){lim.rlim_cur=lim.rlim_max=(rlim_t)max_files;if(setrlimit(RLIMIT_NOFILE,&lim)!=0){dprintf(2,"rlimit files: %s",strerror(errno));return 0;}}
    umask(077);
    if(!inherit_env){extern char **environ;static char *empty_environment[]={NULL};environ=empty_environment;setenv("PATH","/usr/bin:/bin",1);setenv("LANG","C",1);}
#if defined(__linux__)
    if(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)!=0)return 0;
    if(strcmp(profile,"restricted")&&strcmp(profile,"no-network")&&strcmp(profile,"no-write")&&strcmp(profile,"pure"))return 0;
    if(!nc_install_seccomp(profile)){dprintf(2,"seccomp: %s",strerror(errno));return 0;}
#endif
#if defined(__APPLE__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    const char *seatbelt=NULL;if(!strcmp(profile,"no-network"))seatbelt="(version 1) (allow default) (deny network*)";else if(!strcmp(profile,"no-write"))seatbelt="(version 1) (allow default) (deny file-write*)";else if(!strcmp(profile,"pure"))seatbelt="(version 1) (allow default) (deny network*) (deny file-write*)";else if(!strcmp(profile,"restricted"))seatbelt="(version 1) (allow default)";else return 0;
    char *error=NULL;if(sandbox_init(seatbelt,0,&error)!=0){dprintf(2,"seatbelt: %s",error?error:"unknown");if(error)sandbox_free_error(error);return 0;}
#pragma clang diagnostic pop
#elif !defined(__linux__)
    if(strcmp(profile,"restricted")&&strcmp(profile,"no-network"))return 0;
#endif
    return 1;
}

static NcProcessSlot *nc_process_slot_from_request(NcVal *request) {
    const char *handle = nc_atomic_text_field(request, "handle", "");
    long id=0;unsigned long long generation=0;char trailing='\0';
    if(sscanf(handle,"process:%ld:%llu%c",&id,&generation,&trailing)!=2||id<=0||id>=NC_PROCESS_MAX||generation==0)return NULL;
    pthread_mutex_lock(&g_process_registry_lock);
    NcProcessSlot *slot = g_process_slots[id].active == 1 && g_process_slots[id].generation == generation ? &g_process_slots[id] : NULL;
    pthread_mutex_unlock(&g_process_registry_lock);
    return slot;
}

static void nc_process_close_fd(int *fd) { if (*fd >= 0) { close(*fd); *fd = -1; } }

static void nc_process_append(NcProcessSlot *slot, int stream, const char *data, size_t len) {
    size_t remaining = slot->max_output > slot->output_total ? slot->max_output - slot->output_total : 0;
    size_t take = len < remaining ? len : remaining;
    char **buffer = stream == 1 ? &slot->stdout_data : &slot->stderr_data;
    size_t *used = stream == 1 ? &slot->stdout_len : &slot->stderr_len;
    if (take) {
        char *grown = realloc(*buffer, *used + take + 1);
        if (grown) { *buffer = grown; memcpy(*buffer + *used, data, take); *used += take; (*buffer)[*used] = 0; slot->output_total += take; }
    }
    if (take < len || slot->output_total >= slot->max_output) { slot->output_limited = 1; if (!slot->exited) kill(slot->pid, SIGKILL); }
}

static void nc_process_drain_fd(NcProcessSlot *slot, int stream) {
    int *fd = stream == 1 ? &slot->stdout_fd : &slot->stderr_fd;
    if (*fd < 0) return;
    char chunk[4096];
    for (;;) {
        ssize_t n = read(*fd, chunk, sizeof(chunk));
        if (n > 0) nc_process_append(slot, stream, chunk, (size_t)n);
        else if (n == 0) { nc_process_close_fd(fd); break; }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) break;
        else { nc_process_close_fd(fd); break; }
    }
}

static unsigned long long nc_process_resident_bytes(pid_t pid){
#if defined(__APPLE__)
    struct rusage_info_v2 info;memset(&info,0,sizeof(info));
    if(proc_pid_rusage(pid,RUSAGE_INFO_V2,(rusage_info_t *)&info)==0)return info.ri_resident_size;
#else
    (void)pid;
#endif
    return 0;
}

static void nc_process_refresh(NcProcessSlot *slot, int wait_ms) {
    if (!slot || !slot->active) return;
    if(!slot->exited&&!slot->memory_limited&&slot->max_memory_bytes>0){unsigned long long resident=nc_process_resident_bytes(slot->pid);if(resident>slot->max_memory_bytes){slot->memory_limited=1;kill(slot->pid,SIGKILL);}}
    if (!slot->exited && nc_monotonic_ms() >= slot->deadline_ms) { slot->timed_out = 1; kill(slot->pid, SIGKILL); }
    struct pollfd fds[3]; int nfds = 0, in_idx = -1, out_idx = -1, err_idx = -1;
    if (slot->stdin_fd >= 0) { in_idx = nfds; fds[nfds++] = (struct pollfd){slot->stdin_fd, POLLOUT, 0}; }
    if (slot->stdout_fd >= 0) { out_idx = nfds; fds[nfds++] = (struct pollfd){slot->stdout_fd, POLLIN | POLLHUP, 0}; }
    if (slot->stderr_fd >= 0) { err_idx = nfds; fds[nfds++] = (struct pollfd){slot->stderr_fd, POLLIN | POLLHUP, 0}; }
    (void)poll(fds, (nfds_t)nfds, wait_ms < 0 ? 0 : wait_ms);
    if (slot->stdin_fd >= 0) {
        if (slot->stdin_off >= slot->stdin_len || (in_idx >= 0 && (fds[in_idx].revents & (POLLERR | POLLHUP)))) nc_process_close_fd(&slot->stdin_fd);
        else if (in_idx >= 0 && (fds[in_idx].revents & POLLOUT)) { ssize_t n = write(slot->stdin_fd, slot->stdin_data + slot->stdin_off, slot->stdin_len - slot->stdin_off); if (n > 0) slot->stdin_off += (size_t)n; }
    }
    if (out_idx >= 0 && fds[out_idx].revents) nc_process_drain_fd(slot, 1);
    if (err_idx >= 0 && fds[err_idx].revents) nc_process_drain_fd(slot, 2);
    if (!slot->exited) { pid_t waited = waitpid(slot->pid, &slot->wait_status, WNOHANG); if (waited == slot->pid) slot->exited = 1; }
    if (slot->exited) { nc_process_drain_fd(slot, 1); nc_process_drain_fd(slot, 2); nc_process_close_fd(&slot->stdin_fd); }
}

static NcVal *nc_process_async_spawn(NcVal *request) {
    const char *executable = nc_atomic_text_field(request, "executable", "");
    const char *cwd = nc_atomic_text_field(request, "cwd", "");
    const char *stdin_text = nc_atomic_text_field(request, "stdin", "");
    long long timeout_ms = nc_atomic_int_field(request, "timeout_ms", 0);
    long long max_output = nc_atomic_int_field(request, "max_output_bytes", 0);
    const char *sandbox_profile = nc_atomic_text_field(request, "sandbox", "none");
    long long max_memory = nc_atomic_int_field(request, "max_memory_bytes", 536870912);
    long long max_files = nc_atomic_int_field(request, "max_open_files", 64);
    int inherit_env = nc_atomic_int_field(request, "inherit_env", 1) != 0;
    NcVal *args = nc_index_get(request, nc_str("args"));
    if (!*executable || timeout_ms <= 0 || max_output <= 0 || !args || args->type != NC_LIST) return nc_process_async_result(NULL, "invalid spawn request");
    if (max_output > 64LL * 1024LL * 1024LL) max_output = 64LL * 1024LL * 1024LL;
    int argc = args->list->len + 1;
    char **argv = calloc((size_t)argc + 1, sizeof(char *));
    if (!argv) return nc_process_async_result(NULL, "out of memory");
    argv[0] = strdup(executable);
    for (int i = 1; i < argc; i++) {
        NcVal *v = args->list->items[i - 1];
        if (!v || v->type != NC_STR) { for (int j = 0; j < argc; j++) free(argv[j]); free(argv); return nc_process_async_result(NULL, "argv must contain text"); }
        argv[i] = strdup(v->s ? v->s : "");
    }
    pthread_mutex_lock(&g_process_registry_lock);
    int id = 0; for (int i = 1; i < NC_PROCESS_MAX; i++) if (!g_process_slots[i].active) { id = i; g_process_slots[i].generation++; if(!g_process_slots[i].generation)g_process_slots[i].generation=1; g_process_slots[i].active = 2; break; }
    pthread_mutex_unlock(&g_process_registry_lock);
    if (!id) { for (int i = 0; i < argc; i++) free(argv[i]); free(argv); return nc_process_async_result(NULL, "process capacity"); }
    int in_pipe[2], out_pipe[2], err_pipe[2];
    if (pipe(in_pipe) || pipe(out_pipe) || pipe(err_pipe)) { pthread_mutex_lock(&g_process_registry_lock); g_process_slots[id].active = 0; pthread_mutex_unlock(&g_process_registry_lock); for (int i = 0; i < argc; i++) free(argv[i]); free(argv); return nc_process_async_result(NULL, "pipe failed"); }
    pid_t pid = fork();
    if (pid == 0) {
        dup2(in_pipe[0], STDIN_FILENO); dup2(out_pipe[1], STDOUT_FILENO); dup2(err_pipe[1], STDERR_FILENO);
        close(in_pipe[0]); close(in_pipe[1]); close(out_pipe[0]); close(out_pipe[1]); close(err_pipe[0]); close(err_pipe[1]);
        nc_close_inherited_fds();
        if (*cwd && chdir(cwd) != 0) { dprintf(STDERR_FILENO, "chdir failed: %s", strerror(errno)); _exit(126); }
        if (!nc_apply_process_sandbox(sandbox_profile,timeout_ms,max_memory,max_files,inherit_env)) { dprintf(STDERR_FILENO,"sandbox setup failed"); _exit(126); }
        execv(executable, argv); dprintf(STDERR_FILENO, "execv failed: %s", strerror(errno)); _exit(127);
    }
    for (int i = 0; i < argc; i++) free(argv[i]); free(argv);
    if (pid < 0) { pthread_mutex_lock(&g_process_registry_lock); g_process_slots[id].active = 0; pthread_mutex_unlock(&g_process_registry_lock); close(in_pipe[0]); close(in_pipe[1]); close(out_pipe[0]); close(out_pipe[1]); close(err_pipe[0]); close(err_pipe[1]); return nc_process_async_result(NULL, "fork failed"); }
    close(in_pipe[0]); close(out_pipe[1]); close(err_pipe[1]);
    nc_fd_nonblocking(in_pipe[1]); nc_fd_nonblocking(out_pipe[0]); nc_fd_nonblocking(err_pipe[0]);
    NcProcessSlot *slot = &g_process_slots[id]; unsigned long long generation=slot->generation; memset(slot, 0, sizeof(*slot));
    slot->generation=generation; slot->active = 2; slot->id = id; slot->pid = pid; slot->stdin_fd = in_pipe[1]; slot->stdout_fd = out_pipe[0]; slot->stderr_fd = err_pipe[0];
    slot->stdin_data = strdup(stdin_text); slot->stdin_len = strlen(stdin_text); slot->stdout_data = strdup(""); slot->stderr_data = strdup("");
    snprintf(slot->sandbox_profile,sizeof(slot->sandbox_profile),"%s",sandbox_profile);
    slot->max_output = (size_t)max_output;slot->max_memory_bytes=(unsigned long long)(max_memory>0?max_memory:0); slot->deadline_ms = nc_monotonic_ms() + timeout_ms; pthread_mutex_init(&slot->mutex, NULL);
    pthread_mutex_lock(&g_process_registry_lock); slot->active = 1; pthread_mutex_unlock(&g_process_registry_lock);
    return nc_process_async_result(slot, "");
}

static NcVal *nc_builtin_process_operation(NcVal *request) {
    if (!request || request->type != NC_MAP || strcmp(nc_atomic_text_field(request, "abi", ""), "norscode-native-process-v1")) return nc_process_async_result(NULL, "invalid request");
    const char *operation = nc_atomic_text_field(request, "operation", "");
    if (!strcmp(operation, "spawn_argv")) return nc_process_async_spawn(request);
    NcProcessSlot *slot = nc_process_slot_from_request(request);
    if (!slot) return nc_process_async_result(NULL, "invalid handle");
    pthread_mutex_lock(&slot->mutex);
    if (!strcmp(operation, "poll")) nc_process_refresh(slot, 0);
    else if (!strcmp(operation, "wait")) {
        long long wait_ms = nc_atomic_int_field(request, "wait_ms", -1);
        long long wait_deadline = wait_ms < 0 ? slot->deadline_ms : nc_monotonic_ms() + wait_ms;
        while (!slot->exited && nc_monotonic_ms() < wait_deadline) nc_process_refresh(slot, 10);
        nc_process_refresh(slot, 0);
    } else if (!strcmp(operation, "signal")) {
        int sig = (int)nc_atomic_int_field(request, "signal", SIGTERM);
        if (sig != SIGTERM && sig != SIGINT && sig != SIGKILL && sig != SIGHUP) { pthread_mutex_unlock(&slot->mutex); return nc_process_async_result(slot, "invalid signal"); }
        if (!slot->exited && kill(slot->pid, sig) != 0) { pthread_mutex_unlock(&slot->mutex); return nc_process_async_result(slot, "signal failed"); }
        nc_process_refresh(slot, 0);
    } else if (!strcmp(operation, "read")) {
        nc_process_refresh(slot, 0); NcVal *result = nc_process_async_result(slot, "");
        free(slot->stdout_data); free(slot->stderr_data); slot->stdout_data = strdup(""); slot->stderr_data = strdup(""); slot->stdout_len = 0; slot->stderr_len = 0;
        pthread_mutex_unlock(&slot->mutex); return result;
    } else if (!strcmp(operation, "close")) {
        nc_process_refresh(slot, 0);
        if (!slot->exited) { pthread_mutex_unlock(&slot->mutex); return nc_process_async_result(slot, "process still running"); }
        NcVal *result = nc_process_async_result(slot, ""); nc_process_close_fd(&slot->stdin_fd); nc_process_close_fd(&slot->stdout_fd); nc_process_close_fd(&slot->stderr_fd);
        free(slot->stdin_data); free(slot->stdout_data); free(slot->stderr_data);
        pthread_mutex_lock(&g_process_registry_lock); slot->active = 0; pthread_mutex_unlock(&g_process_registry_lock);
        pthread_mutex_unlock(&slot->mutex); pthread_mutex_destroy(&slot->mutex); return result;
    } else { pthread_mutex_unlock(&slot->mutex); return nc_process_async_result(slot, "invalid operation"); }
    NcVal *result = nc_process_async_result(slot, ""); pthread_mutex_unlock(&slot->mutex); return result;
}
#else
#define NC_PROCESS_MAX 64
typedef struct {
    int active, id;
    unsigned long long generation;
    NcwProcess process;
    char *stdout_data, *stderr_data;
    size_t stdout_len, stderr_len, output_total, max_output;
    int output_limited;
    char sandbox_profile[32];
    pthread_mutex_t mutex;
} NcProcessSlot;
static NcProcessSlot g_process_slots[NC_PROCESS_MAX];
static pthread_mutex_t g_process_registry_lock=PTHREAD_MUTEX_INITIALIZER;

static NcVal *nc_process_windows_result(NcProcessSlot *slot,const char *error){
    NcVal *r=nc_map_new();char handle[64]="";const char *status="feil";long long exit_code=-1;
    if(slot){snprintf(handle,sizeof(handle),"process:%d:%llu",slot->id,slot->generation);status=slot->process.exited?"ferdig":"kjorer";if(slot->process.timed_out){status="timeout";exit_code=124;}else if(slot->output_limited){status="feil";exit_code=125;}else if(slot->process.exited)exit_code=(long long)slot->process.exit_code;}
    nc_index_set(r,nc_str("abi"),nc_str("norscode-native-process-v1"));nc_index_set(r,nc_str("status"),nc_str(status));nc_index_set(r,nc_str("handle"),nc_str(handle));
    nc_index_set(r,nc_str("pid"),nc_int(slot?(long long)slot->process.pid:-1));nc_index_set(r,nc_str("exit_code"),nc_int(exit_code));
    nc_index_set(r,nc_str("stdout"),nc_str(slot&&slot->stdout_data?slot->stdout_data:""));nc_index_set(r,nc_str("stderr"),nc_str(slot&&slot->stderr_data?slot->stderr_data:""));
    nc_index_set(r,nc_str("error"),nc_str(error&&*error?error:slot&&slot->process.timed_out?"timeout":slot&&slot->output_limited?"output limit":""));
    nc_index_set(r,nc_str("sandbox"),nc_str(slot?slot->sandbox_profile:"none"));nc_index_set(r,nc_str("sandbox_backend"),nc_str(slot&&slot->process.appcontainer?"appcontainer+job-object":slot&&strcmp(slot->sandbox_profile,"none")?"job-object":"none"));return r;
}

static NcProcessSlot *nc_process_windows_slot(NcVal *request){
    const char *handle=nc_atomic_text_field(request,"handle","");long id=0;unsigned long long generation=0;char trailing='\0';
    if(sscanf(handle,"process:%ld:%llu%c",&id,&generation,&trailing)!=2||id<=0||id>=NC_PROCESS_MAX||generation==0)return NULL;
    pthread_mutex_lock(&g_process_registry_lock);NcProcessSlot *slot=g_process_slots[id].active==1&&g_process_slots[id].generation==generation?&g_process_slots[id]:NULL;pthread_mutex_unlock(&g_process_registry_lock);return slot;
}

static void nc_process_windows_append(NcProcessSlot *slot,int stream,const char *data,size_t length){
    size_t remaining=slot->max_output>slot->output_total?slot->max_output-slot->output_total:0,take=length<remaining?length:remaining;
    char **buffer=stream?&slot->stderr_data:&slot->stdout_data;size_t *used=stream?&slot->stderr_len:&slot->stdout_len;
    if(take){char *grown=realloc(*buffer,*used+take+1);if(grown){*buffer=grown;memcpy(*buffer+*used,data,take);*used+=take;(*buffer)[*used]=0;slot->output_total+=take;}else take=0;}
    if(take<length||slot->output_total>=slot->max_output){slot->output_limited=1;if(!slot->process.exited){char ignored[NCW_ERROR_CAP];ncw_process_terminate(&slot->process,125,ignored,sizeof(ignored));}}
}

static void nc_process_windows_refresh(NcProcessSlot *slot,uint64_t wait_ms,char *error,size_t error_cap){
    if(!slot||!slot->active)return;
    if(wait_ms) (void)ncw_process_wait(&slot->process,wait_ms,error,error_cap); else (void)ncw_process_poll(&slot->process,error,error_cap);
    for(int stream=0;stream<2;stream++){char chunk[4096];int64_t n;while((n=ncw_process_read(&slot->process,stream,chunk,sizeof(chunk),error,error_cap))>0)nc_process_windows_append(slot,stream,chunk,(size_t)n);}
}

static NcVal *nc_process_windows_spawn(NcVal *request){
    const char *executable=nc_atomic_text_field(request,"executable",""),*cwd=nc_atomic_text_field(request,"cwd",""),*stdin_text=nc_atomic_text_field(request,"stdin",""),*sandbox=nc_atomic_text_field(request,"sandbox","none");
    long long timeout_ms=nc_atomic_int_field(request,"timeout_ms",0),max_output=nc_atomic_int_field(request,"max_output_bytes",0),max_memory=nc_atomic_int_field(request,"max_memory_bytes",536870912);NcVal *args=nc_index_get(request,nc_str("args"));
    if(!*executable||timeout_ms<=0||max_output<=0||!args||args->type!=NC_LIST)return nc_process_windows_result(NULL,"invalid spawn request");if(max_output>64LL*1024LL*1024LL)max_output=64LL*1024LL*1024LL;
    int argc=args->list->len+1;char **argv=calloc((size_t)argc+1,sizeof(char *));if(!argv)return nc_process_windows_result(NULL,"out of memory");argv[0]=strdup(executable);
    for(int i=1;i<argc;i++){NcVal *value=args->list->items[i-1];if(!value||value->type!=NC_STR){for(int j=0;j<argc;j++)free(argv[j]);free(argv);return nc_process_windows_result(NULL,"argv must contain text");}argv[i]=strdup(value->s?value->s:"");}
    pthread_mutex_lock(&g_process_registry_lock);int id=0;for(int i=1;i<NC_PROCESS_MAX;i++)if(!g_process_slots[i].active){id=i;g_process_slots[i].generation++;if(!g_process_slots[i].generation)g_process_slots[i].generation=1;g_process_slots[i].active=2;break;}pthread_mutex_unlock(&g_process_registry_lock);
    if(!id){for(int i=0;i<argc;i++)free(argv[i]);free(argv);return nc_process_windows_result(NULL,"process capacity");}
    NcProcessSlot *slot=&g_process_slots[id];unsigned long long generation=slot->generation;memset(slot,0,sizeof(*slot));slot->generation=generation;slot->active=2;slot->id=id;slot->max_output=(size_t)max_output;slot->stdout_data=strdup("");slot->stderr_data=strdup("");snprintf(slot->sandbox_profile,sizeof(slot->sandbox_profile),"%s",sandbox);pthread_mutex_init(&slot->mutex,NULL);
    char backend_error[NCW_ERROR_CAP]="";int ok=ncw_process_spawn(&slot->process,executable,(const char *const *)argv,(size_t)argc,cwd,stdin_text,strlen(stdin_text),(uint64_t)timeout_ms,(uint64_t)(max_memory>0?max_memory:0),sandbox,backend_error,sizeof(backend_error));for(int i=0;i<argc;i++)free(argv[i]);free(argv);
    if(!ok){free(slot->stdout_data);free(slot->stderr_data);pthread_mutex_destroy(&slot->mutex);pthread_mutex_lock(&g_process_registry_lock);slot->active=0;pthread_mutex_unlock(&g_process_registry_lock);return nc_process_windows_result(NULL,backend_error);}
    pthread_mutex_lock(&g_process_registry_lock);slot->active=1;pthread_mutex_unlock(&g_process_registry_lock);return nc_process_windows_result(slot,"");
}

static NcVal *nc_builtin_process_operation(NcVal *request) {
    if(!request||request->type!=NC_MAP||strcmp(nc_atomic_text_field(request,"abi",""),"norscode-native-process-v1"))return nc_process_windows_result(NULL,"invalid request");const char *operation=nc_atomic_text_field(request,"operation","");if(!strcmp(operation,"spawn_argv"))return nc_process_windows_spawn(request);
    NcProcessSlot *slot=nc_process_windows_slot(request);if(!slot)return nc_process_windows_result(NULL,"invalid handle");pthread_mutex_lock(&slot->mutex);char backend_error[NCW_ERROR_CAP]="";
    if(!strcmp(operation,"poll"))nc_process_windows_refresh(slot,0,backend_error,sizeof(backend_error));
    else if(!strcmp(operation,"wait")){long long wait_ms=nc_atomic_int_field(request,"wait_ms",0);nc_process_windows_refresh(slot,(uint64_t)(wait_ms>0?wait_ms:0),backend_error,sizeof(backend_error));}
    else if(!strcmp(operation,"signal")){DWORD code=(DWORD)nc_atomic_int_field(request,"signal",143);if(!slot->process.exited&&!ncw_process_terminate(&slot->process,code,backend_error,sizeof(backend_error))){pthread_mutex_unlock(&slot->mutex);return nc_process_windows_result(slot,backend_error);}nc_process_windows_refresh(slot,0,backend_error,sizeof(backend_error));}
    else if(!strcmp(operation,"read")){nc_process_windows_refresh(slot,0,backend_error,sizeof(backend_error));NcVal *result=nc_process_windows_result(slot,backend_error);free(slot->stdout_data);free(slot->stderr_data);slot->stdout_data=strdup("");slot->stderr_data=strdup("");slot->stdout_len=slot->stderr_len=0;pthread_mutex_unlock(&slot->mutex);return result;}
    else if(!strcmp(operation,"close")){nc_process_windows_refresh(slot,0,backend_error,sizeof(backend_error));if(!slot->process.exited){pthread_mutex_unlock(&slot->mutex);return nc_process_windows_result(slot,"process still running");}NcVal *result=nc_process_windows_result(slot,backend_error);ncw_process_close(&slot->process,backend_error,sizeof(backend_error));free(slot->stdout_data);free(slot->stderr_data);pthread_mutex_lock(&g_process_registry_lock);slot->active=0;pthread_mutex_unlock(&g_process_registry_lock);pthread_mutex_unlock(&slot->mutex);pthread_mutex_destroy(&slot->mutex);return result;}
    else{pthread_mutex_unlock(&slot->mutex);return nc_process_windows_result(slot,"invalid operation");}
    NcVal *result=nc_process_windows_result(slot,backend_error);pthread_mutex_unlock(&slot->mutex);return result;
}
#endif

/* Forward decl */
static NcVal *g_nc_closure_fwd;
#define g_nc_closure g_nc_closure_fwd
static NcVal *nc_exec_call(NcVal *functions, const char *fn_name, NcVal **args, int nargs, int depth);

#define NC_THREAD_MAX 128
typedef struct {
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t condition;
    _Atomic int cancel_requested;
    int active;
    int finished;
    int joined;
    int id;
    unsigned long long generation;
    char fn[128];
    NcVal *args;
    NcVal *functions;
    char value[128];
    char error[256];
} NcThreadSlot;
static NcThreadSlot g_thread_slots[NC_THREAD_MAX];
static pthread_mutex_t g_thread_registry_lock=PTHREAD_MUTEX_INITIALIZER;
static _Thread_local int g_native_thread_id = 0;
static NcVal *nc_builtin_thread_current_id(void) { return nc_int(g_native_thread_id); }
static NcVal *nc_thread_result(const char *status, NcThreadSlot *slot,
                               const char *value, const char *error);

#define NC_THREAD_POOL_MAX 8
#define NC_THREAD_POOL_WORKERS_MAX 16
#define NC_THREAD_POOL_TASKS_MAX 256
typedef struct {
    int active,id,state,cancel_requested;
    unsigned int generation;
    char fn[128],value[128],error[256];
    NcVal *args,*functions;
} NcThreadPoolTask;
typedef struct {
    int active,id,shutdown,worker_count,queue_head,queue_tail,queue_count;
    unsigned int generation;
    unsigned int next_generation;
    pthread_t workers[NC_THREAD_POOL_WORKERS_MAX];
    pthread_mutex_t mutex;
    pthread_cond_t work_available;
    pthread_cond_t task_changed;
    int queue[NC_THREAD_POOL_TASKS_MAX];
    NcThreadPoolTask tasks[NC_THREAD_POOL_TASKS_MAX];
} NcThreadPool;
static NcThreadPool g_thread_pools[NC_THREAD_POOL_MAX];
static pthread_mutex_t g_thread_pool_registry_lock=PTHREAD_MUTEX_INITIALIZER;

#define NC_SYNC_MAX 256
typedef struct {
    int active;
    int type;
    unsigned long long generation;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} NcSyncSlot;
static NcSyncSlot g_sync_slots[NC_SYNC_MAX];
static pthread_mutex_t g_sync_registry_lock=PTHREAD_MUTEX_INITIALIZER;

static NcVal *nc_sync_result(const char *status, int id, const char *kind, const char *error) {
    NcVal *r = nc_thread_result(status, NULL, "", error);
    char handle[64] = "";
    if (id > 0) snprintf(handle, sizeof(handle), "%s:%d:%llu", kind, id,g_sync_slots[id].generation);
    nc_index_set(r, nc_str("handle"), nc_str(handle));
    return r;
}

static NcSyncSlot *nc_sync_slot(const char *handle, int expected_type) {
    const char *prefix = expected_type == 1 ? "mutex:" : "condition:";
    size_t prefix_len = strlen(prefix);
    if (strncmp(handle, prefix, prefix_len)) return NULL;
    long id=0;unsigned long long generation=0;char trailing='\0';
    if(sscanf(handle+prefix_len,"%ld:%llu%c",&id,&generation,&trailing)!=2||id<=0||id>=NC_SYNC_MAX||generation==0)return NULL;
    pthread_mutex_lock(&g_sync_registry_lock);NcSyncSlot *slot=g_sync_slots[id].active==1&&g_sync_slots[id].type==expected_type&&g_sync_slots[id].generation==generation?&g_sync_slots[id]:NULL;pthread_mutex_unlock(&g_sync_registry_lock);return slot;
}

static int nc_sync_slot_id(NcSyncSlot *slot) {
    return slot ? (int)(slot - g_sync_slots) : 0;
}

static NcVal *nc_thread_result(const char *status, NcThreadSlot *slot,
                               const char *value, const char *error) {
    NcVal *r = nc_map_new();
    char handle[32] = "";
    char thread_id[32] = "";
    if (slot) {
        snprintf(handle, sizeof(handle), "thread:%d:%llu", slot->id, slot->generation);
        snprintf(thread_id, sizeof(thread_id), "%d", slot->id);
    }
    nc_index_set(r, nc_str("abi"), nc_str("norscode-native-thread-v1"));
    nc_index_set(r, nc_str("status"), nc_str(status));
    nc_index_set(r, nc_str("handle"), nc_str(handle));
    nc_index_set(r, nc_str("thread_id"), nc_str(thread_id));
    nc_index_set(r, nc_str("value"), nc_str(value ? value : ""));
    nc_index_set(r, nc_str("error"), nc_str(error ? error : ""));
    return r;
}

static NcThreadSlot *nc_thread_slot_from_request(NcVal *request) {
    const char *handle = nc_atomic_text_field(request, "handle", "");
    long id=0;unsigned long long generation=0;char trailing='\0';
    if(sscanf(handle,"thread:%ld:%llu%c",&id,&generation,&trailing)!=2||id<=0||id>=NC_THREAD_MAX||generation==0)return NULL;
    pthread_mutex_lock(&g_thread_registry_lock);NcThreadSlot *slot=g_thread_slots[id].active&&g_thread_slots[id].generation==generation?&g_thread_slots[id]:NULL;pthread_mutex_unlock(&g_thread_registry_lock);return slot;
}

static void *nc_thread_worker(void *raw) {
    NcThreadSlot *slot = raw;
    NcVal *worker_roots[2] = {slot->args, slot->functions};
    int worker_sp = 0, worker_root_count = 2;
    NcGcFrame worker_frame;
    nc_gc_frame_enter(&worker_frame, worker_roots, &worker_sp, worker_roots, &worker_root_count);
    g_native_thread_id = slot->id;
    if (!strcmp(slot->fn, "thread.noop")) {
        snprintf(slot->value, sizeof(slot->value), "ok");
    } else if (!strcmp(slot->fn, "thread.atomic_increment")) {
        const char *handle = nc_atomic_text_field(slot->args, "handle", "");
        long atomic_id = nc_atomic_handle_raw(handle);
        long long iterations = nc_atomic_int_field(slot->args, "iterations", 1);
        if (atomic_id <= 0) {
            snprintf(slot->error, sizeof(slot->error), "invalid atomic handle");
        } else {
            long long completed = 0;
            while (completed < iterations && !atomic_load_explicit(&slot->cancel_requested, memory_order_acquire)) {
                atomic_fetch_add_explicit(&g_atomic_cells[atomic_id].value, 1, memory_order_seq_cst);
                atomic_fetch_add_explicit(&g_atomic_cells[atomic_id].version, 1, memory_order_relaxed);
                completed++;
            }
            snprintf(slot->value, sizeof(slot->value), "%lld", completed);
        }
    } else if (!strcmp(slot->fn, "thread.condition_signal")) {
        const char *condition_handle = nc_atomic_text_field(slot->args, "condition_handle", "");
        const char *mutex_handle = nc_atomic_text_field(slot->args, "mutex_handle", "");
        NcSyncSlot *condition = nc_sync_slot(condition_handle, 2);
        NcSyncSlot *mutex = nc_sync_slot(mutex_handle, 1);
        long long delay_ms = nc_atomic_int_field(slot->args, "delay_ms", 1);
        if (!condition || !mutex) {
            snprintf(slot->error, sizeof(slot->error), "invalid condition resources");
        } else {
            struct timespec delay = {delay_ms / 1000, (delay_ms % 1000) * 1000000};
            nanosleep(&delay, NULL);
            pthread_mutex_lock(&mutex->mutex);
            pthread_cond_signal(&condition->condition);
            pthread_mutex_unlock(&mutex->mutex);
            snprintf(slot->value, sizeof(slot->value), "signalled");
        }
    } else {
        NcVal *functions = slot->functions ? slot->functions : g_current_functions;
        NcVal *fn_def = functions ? nc_exec_find_fn(functions, slot->fn) : NULL;
        if (getenv("NORSCODE_THREAD_TRACE"))
            fprintf(stderr, "[nc-thread] generic id=%d functions=%p fn_def=%p\n",
                    slot->id, (void *)functions, (void *)fn_def);
        if (!fn_def) {
            snprintf(slot->error, sizeof(slot->error), "unknown thread function: %.190s", slot->fn);
        } else if (setjmp(g_err_jmp)) {
            snprintf(slot->error, sizeof(slot->error), "%.250s", g_err_msg[0] ? g_err_msg : "worker exception");
            g_err_msg[0] = 0;
        } else {
            NcVal *worker_args[1] = {slot->args ? slot->args : nc_map_new()};
            if (getenv("NORSCODE_THREAD_TRACE"))
                fprintf(stderr, "[nc-thread] generic id=%d execute\n", slot->id);
            NcVal *result = nc_exec_call(functions, slot->fn, worker_args, 1, 0);
            char *text = nc_to_str_raw(result);
            snprintf(slot->value, sizeof(slot->value), "%.127s", text ? text : "");
            free(text);
        }
    }

    if (getenv("NORSCODE_THREAD_TRACE")) {
        fprintf(stderr, "[nc-thread] id=%d fn=%s value=%s error=%s\n",
                slot->id, slot->fn, slot->value, slot->error);
    }
    pthread_mutex_lock(&slot->lock);
    slot->finished = 1;
    pthread_cond_broadcast(&slot->condition);
    pthread_mutex_unlock(&slot->lock);
    nc_gc_frame_leave(&worker_frame);
    g_native_thread_id = 0;
    return NULL;
}

static NcVal *nc_builtin_thread_spawn(NcVal *request) {
    if (!request || request->type != NC_MAP) return nc_thread_result("error", NULL, "", "invalid request");
    const char *fn = nc_atomic_text_field(request, "fn", "");
    long long stack_bytes = nc_atomic_int_field(request, "stack_bytes", 0);
    if (getenv("NORSCODE_THREAD_TRACE"))
        fprintf(stderr, "[nc-thread] spawn fn=%s stack=%lld\n", fn, stack_bytes);
    if (!*fn || stack_bytes < 65536) return nc_thread_result("error", NULL, "", "invalid spawn request");
    int native_special = !strcmp(fn, "thread.noop") || !strcmp(fn, "thread.atomic_increment") ||
        !strcmp(fn, "thread.condition_signal");
    NcVal *request_functions = nc_map_get_cstr(request, "__vm_functions");
    if (!request_functions || request_functions->type != NC_MAP) request_functions = g_current_functions;
    if (!native_special && (!request_functions || !nc_exec_find_fn(request_functions, fn)))
        return nc_thread_result("error", NULL, "", "unknown thread function");
    if (stack_bytes < 1048576) stack_bytes = 1048576;
    pthread_mutex_lock(&g_thread_registry_lock);int id = 0;
    for (int i = 1; i < NC_THREAD_MAX; i++) if (!g_thread_slots[i].active) { id = i;g_thread_slots[i].generation++;if(!g_thread_slots[i].generation)g_thread_slots[i].generation=1;g_thread_slots[i].active=2;break; }
    pthread_mutex_unlock(&g_thread_registry_lock);
    if (!id) return nc_thread_result("error", NULL, "", "thread capacity");

    NcThreadSlot *slot = &g_thread_slots[id];
    unsigned long long generation=slot->generation;memset(slot, 0, sizeof(*slot));slot->generation=generation;
    slot->active = 2;
    slot->id = id;
    slot->args = nc_index_get(request, nc_str("args"));
    slot->functions = request_functions;
    snprintf(slot->fn, sizeof(slot->fn), "%s", fn);
    pthread_mutex_init(&slot->lock, NULL);
    pthread_cond_init(&slot->condition, NULL);
    atomic_init(&slot->cancel_requested, 0);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, (size_t)stack_bytes);
    int rc = pthread_create(&slot->thread, &attr, nc_thread_worker, slot);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        pthread_mutex_lock(&g_thread_registry_lock);slot->active = 0;pthread_mutex_unlock(&g_thread_registry_lock);
        return nc_thread_result("error", slot, "", "pthread_create failed");
    }
    pthread_mutex_lock(&g_thread_registry_lock);slot->active=1;pthread_mutex_unlock(&g_thread_registry_lock);
    return nc_thread_result("started", slot, "", "");
}

static NcVal *nc_builtin_thread_join(NcVal *request) {
    NcThreadSlot *slot = nc_thread_slot_from_request(request);
    if (!slot) {
        if (getenv("NORSCODE_THREAD_TRACE"))
            fprintf(stderr, "[nc-thread] join invalid handle=%s\n", nc_atomic_text_field(request, "handle", ""));
        return nc_thread_result("error", NULL, "", "invalid handle");
    }
    long long timeout_ms = nc_atomic_int_field(request, "timeout_ms", -1);
    nc_gc_blocking_enter();
    pthread_mutex_lock(&slot->lock);
    int timed_out = 0;
    if (timeout_ms < 0) {
        while (!slot->finished) pthread_cond_wait(&slot->condition, &slot->lock);
    } else {
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += timeout_ms / 1000;
        deadline.tv_nsec += (timeout_ms % 1000) * 1000000;
        if (deadline.tv_nsec >= 1000000000) { deadline.tv_sec++; deadline.tv_nsec -= 1000000000; }
        while (!slot->finished) {
            int rc = pthread_cond_timedwait(&slot->condition, &slot->lock, &deadline);
            if (rc == ETIMEDOUT) { timed_out = 1; break; }
        }
    }
    pthread_mutex_unlock(&slot->lock);
    if (timed_out) {
        nc_gc_blocking_leave();
        return nc_thread_result("timeout", slot, "", "timeout");
    }
    if (!slot->joined) { pthread_join(slot->thread, NULL); slot->joined = 1; }
    nc_gc_blocking_leave();
    NcVal *result = nc_thread_result(slot->error[0] ? "error" : "finished", slot, slot->value, slot->error);
    pthread_mutex_destroy(&slot->lock);
    pthread_cond_destroy(&slot->condition);
    pthread_mutex_lock(&g_thread_registry_lock);slot->active = 0;pthread_mutex_unlock(&g_thread_registry_lock);
    return result;
}

static NcVal *nc_thread_pool_result(const char *status,NcThreadPool *pool,
                                    NcThreadPoolTask *task,const char *error){
    NcVal *r=nc_map_new();char pool_handle[64]="",task_handle[96]="";
    if(pool)snprintf(pool_handle,sizeof(pool_handle),"thread-pool:%d:%u",pool->id,pool->generation);
    if(pool&&task)snprintf(task_handle,sizeof(task_handle),"thread-task:%d:%u:%d:%u",pool->id,pool->generation,task->id,task->generation);
    nc_index_set(r,nc_str("abi"),nc_str("norscode-native-thread-pool-v1"));
    nc_index_set(r,nc_str("status"),nc_str(status));
    nc_index_set(r,nc_str("pool_handle"),nc_str(pool_handle));
    nc_index_set(r,nc_str("task_handle"),nc_str(task_handle));
    nc_index_set(r,nc_str("value"),nc_str(task?task->value:""));
    nc_index_set(r,nc_str("error"),nc_str(error&&*error?error:(task?task->error:"")));
    nc_index_set(r,nc_str("workers"),nc_int(pool?pool->worker_count:0));
    nc_index_set(r,nc_str("queued"),nc_int(pool?pool->queue_count:0));
    return r;
}

static NcThreadPool *nc_thread_pool_from_handle(const char *handle){
    long id=0;unsigned int generation=0;char trailing='\0';if(!handle||sscanf(handle,"thread-pool:%ld:%u%c",&id,&generation,&trailing)!=2||id<=0||id>=NC_THREAD_POOL_MAX||generation==0)return NULL;
    pthread_mutex_lock(&g_thread_pool_registry_lock);NcThreadPool *pool=g_thread_pools[id].active==1&&g_thread_pools[id].generation==generation?&g_thread_pools[id]:NULL;pthread_mutex_unlock(&g_thread_pool_registry_lock);return pool;
}

static NcThreadPoolTask *nc_thread_pool_task_from_handle(NcThreadPool *pool,const char *handle){
    long pool_id=0,task_id=0;unsigned int pool_generation=0,generation=0;char trailing='\0';
    if(!pool||!handle||sscanf(handle,"thread-task:%ld:%u:%ld:%u%c",&pool_id,&pool_generation,&task_id,&generation,&trailing)!=4||pool_id!=pool->id||pool_generation!=pool->generation||task_id<=0||task_id>=NC_THREAD_POOL_TASKS_MAX||generation==0)return NULL;
    NcThreadPoolTask *task=&pool->tasks[task_id];
    if(!task->active||task->generation!=generation)return NULL;
    return task;
}

static void nc_thread_pool_execute(NcThreadPool *pool,NcThreadPoolTask *task){
    NcVal *roots[2]={task->args,task->functions};int sp=0,root_count=2;NcGcFrame frame;
    nc_gc_frame_enter(&frame,roots,&sp,roots,&root_count);
#if defined(_WIN32)
    g_native_thread_id=(int)(GetCurrentThreadId()&0x7fffffffU);
#else
    g_native_thread_id=(int)((uintptr_t)pthread_self()&0x7fffffffU);
#endif
    if(task->cancel_requested){snprintf(task->error,sizeof(task->error),"cancelled");}
    else if(!strcmp(task->fn,"thread.noop")){snprintf(task->value,sizeof(task->value),"ok");}
    else if(!strcmp(task->fn,"thread.atomic_increment")){
        const char *handle=nc_atomic_text_field(task->args,"handle","");
        long atomic_id=nc_atomic_handle_raw(handle);
        long long iterations=nc_atomic_int_field(task->args,"iterations",1),completed=0;
        if(atomic_id<=0)
            snprintf(task->error,sizeof(task->error),"invalid atomic handle");
        else {
            while(completed<iterations&&!task->cancel_requested){
                atomic_fetch_add_explicit(&g_atomic_cells[atomic_id].value,1,memory_order_seq_cst);
                atomic_fetch_add_explicit(&g_atomic_cells[atomic_id].version,1,memory_order_relaxed);completed++;
            }
            snprintf(task->value,sizeof(task->value),"%lld",completed);
        }
    } else if(!task->functions||!nc_exec_find_fn(task->functions,task->fn)){
        snprintf(task->error,sizeof(task->error),"unknown pool function: %.190s",task->fn);
    } else if(setjmp(g_err_jmp)){
        snprintf(task->error,sizeof(task->error),"%.250s",g_err_msg[0]?g_err_msg:"pool worker exception");g_err_msg[0]=0;
    } else {
        NcVal *worker_args[1]={task->args?task->args:nc_map_new()};
        NcVal *result=nc_exec_call(task->functions,task->fn,worker_args,1,0);
        char *value=nc_to_str_raw(result);snprintf(task->value,sizeof(task->value),"%.127s",value?value:"");free(value);
    }
    g_native_thread_id=0;nc_gc_frame_leave(&frame);
}

static void *nc_thread_pool_worker(void *raw){
    NcThreadPool *pool=raw;
    for(;;){
        pthread_mutex_lock(&pool->mutex);
        while(pool->queue_count==0&&!pool->shutdown)pthread_cond_wait(&pool->work_available,&pool->mutex);
        if(pool->queue_count==0&&pool->shutdown){pthread_mutex_unlock(&pool->mutex);break;}
        int task_id=pool->queue[pool->queue_head];pool->queue_head=(pool->queue_head+1)%NC_THREAD_POOL_TASKS_MAX;pool->queue_count--;
        NcThreadPoolTask *task=&pool->tasks[task_id];
        if(task->cancel_requested){task->state=4;snprintf(task->error,sizeof(task->error),"cancelled");pthread_cond_broadcast(&pool->task_changed);pthread_mutex_unlock(&pool->mutex);continue;}
        task->state=2;pthread_mutex_unlock(&pool->mutex);
        nc_thread_pool_execute(pool,task);
        pthread_mutex_lock(&pool->mutex);task->state=task->cancel_requested&&task->error[0]?4:3;pthread_cond_broadcast(&pool->task_changed);pthread_mutex_unlock(&pool->mutex);
    }
    return NULL;
}

static NcVal *nc_builtin_thread_pool(NcVal *request){
    if(!request||request->type!=NC_MAP||strcmp(nc_atomic_text_field(request,"abi",""),"norscode-native-thread-pool-v1"))
        return nc_thread_pool_result("error",NULL,NULL,"invalid pool request");
    const char *op=nc_atomic_text_field(request,"operation","");
    if(!strcmp(op,"create")){
        int workers=(int)nc_atomic_int_field(request,"workers",0);
        if(workers<1||workers>NC_THREAD_POOL_WORKERS_MAX)return nc_thread_pool_result("error",NULL,NULL,"invalid worker count");
        pthread_mutex_lock(&g_thread_pool_registry_lock);int id=0;for(int i=1;i<NC_THREAD_POOL_MAX;i++)if(!g_thread_pools[i].active){id=i;g_thread_pools[i].generation++;if(!g_thread_pools[i].generation)g_thread_pools[i].generation=1;g_thread_pools[i].active=2;break;}pthread_mutex_unlock(&g_thread_pool_registry_lock);
        if(!id)return nc_thread_pool_result("error",NULL,NULL,"pool capacity");
        NcThreadPool *pool=&g_thread_pools[id];unsigned int generation=pool->generation;memset(pool,0,sizeof(*pool));pool->generation=generation;pool->active=2;pool->id=id;pool->worker_count=workers;pool->next_generation=1;
        pthread_mutex_init(&pool->mutex,NULL);pthread_cond_init(&pool->work_available,NULL);pthread_cond_init(&pool->task_changed,NULL);
        int created=0;for(;created<workers;created++)if(pthread_create(&pool->workers[created],NULL,nc_thread_pool_worker,pool)!=0)break;
        if(created!=workers){pthread_mutex_lock(&pool->mutex);pool->shutdown=1;pthread_cond_broadcast(&pool->work_available);pthread_mutex_unlock(&pool->mutex);for(int i=0;i<created;i++)pthread_join(pool->workers[i],NULL);pthread_mutex_lock(&g_thread_pool_registry_lock);pool->active=0;pthread_mutex_unlock(&g_thread_pool_registry_lock);return nc_thread_pool_result("error",pool,NULL,"pthread_create failed");}
        pthread_mutex_lock(&g_thread_pool_registry_lock);pool->active=1;pthread_mutex_unlock(&g_thread_pool_registry_lock);
        return nc_thread_pool_result("ready",pool,NULL,"");
    }
    NcThreadPool *pool=nc_thread_pool_from_handle(nc_atomic_text_field(request,"pool_handle",""));
    if(!pool)return nc_thread_pool_result("error",NULL,NULL,"invalid pool handle");
    if(!strcmp(op,"submit")){
        const char *fn=nc_atomic_text_field(request,"fn","");NcVal *functions=nc_map_get_cstr(request,"__vm_functions");
        if(!functions||functions->type!=NC_MAP)functions=g_current_functions;
        int native_special=!strcmp(fn,"thread.noop")||!strcmp(fn,"thread.atomic_increment");
        if(!*fn||(!native_special&&(!functions||!nc_exec_find_fn(functions,fn))))return nc_thread_pool_result("error",pool,NULL,"unknown pool function");
        pthread_mutex_lock(&pool->mutex);
        if(pool->shutdown||pool->queue_count>=NC_THREAD_POOL_TASKS_MAX-1){pthread_mutex_unlock(&pool->mutex);return nc_thread_pool_result("error",pool,NULL,"pool queue unavailable");}
        int id=0;for(int i=1;i<NC_THREAD_POOL_TASKS_MAX;i++)if(!pool->tasks[i].active){id=i;break;}
        if(!id){pthread_mutex_unlock(&pool->mutex);return nc_thread_pool_result("error",pool,NULL,"task capacity");}
        NcThreadPoolTask *task=&pool->tasks[id];memset(task,0,sizeof(*task));task->active=1;task->id=id;task->state=1;task->generation=pool->next_generation++;task->args=nc_index_get(request,nc_str("args"));task->functions=functions;snprintf(task->fn,sizeof(task->fn),"%s",fn);
        pool->queue[pool->queue_tail]=id;pool->queue_tail=(pool->queue_tail+1)%NC_THREAD_POOL_TASKS_MAX;pool->queue_count++;pthread_cond_signal(&pool->work_available);
        NcVal *result=nc_thread_pool_result("queued",pool,task,"");pthread_mutex_unlock(&pool->mutex);return result;
    }
    NcThreadPoolTask *task=nc_thread_pool_task_from_handle(pool,nc_atomic_text_field(request,"task_handle",""));
    if(!strcmp(op,"shutdown")){
        int cancel_pending=nc_atomic_int_field(request,"cancel_pending",0)!=0;
        pthread_mutex_lock(&pool->mutex);pool->shutdown=1;
        if(cancel_pending)for(int i=1;i<NC_THREAD_POOL_TASKS_MAX;i++)if(pool->tasks[i].active&&pool->tasks[i].state==1)pool->tasks[i].cancel_requested=1;
        pthread_cond_broadcast(&pool->work_available);pthread_mutex_unlock(&pool->mutex);
        nc_gc_blocking_enter();for(int i=0;i<pool->worker_count;i++)pthread_join(pool->workers[i],NULL);nc_gc_blocking_leave();
        NcVal *result=nc_thread_pool_result("finished",pool,NULL,"");
        pthread_mutex_destroy(&pool->mutex);pthread_cond_destroy(&pool->work_available);pthread_cond_destroy(&pool->task_changed);pthread_mutex_lock(&g_thread_pool_registry_lock);pool->active=0;pthread_mutex_unlock(&g_thread_pool_registry_lock);return result;
    }
    if(!task)return nc_thread_pool_result("error",pool,NULL,"invalid task handle");
    if(!strcmp(op,"cancel")){
        pthread_mutex_lock(&pool->mutex);if(task->state!=1){pthread_mutex_unlock(&pool->mutex);return nc_thread_pool_result("error",pool,task,"task already running");}task->cancel_requested=1;pthread_cond_broadcast(&pool->task_changed);NcVal *result=nc_thread_pool_result("cancelled",pool,task,"");pthread_mutex_unlock(&pool->mutex);return result;
    }
    if(!strcmp(op,"wait")||!strcmp(op,"status")){
        pthread_mutex_lock(&pool->mutex);long long timeout_ms=!strcmp(op,"wait")?nc_atomic_int_field(request,"timeout_ms",-1):0;int timed_out=0;
        if(!strcmp(op,"wait")&&task->state<3){
            if(timeout_ms<0)while(task->state<3)pthread_cond_wait(&pool->task_changed,&pool->mutex);
            else {struct timespec deadline;clock_gettime(CLOCK_REALTIME,&deadline);deadline.tv_sec+=timeout_ms/1000;deadline.tv_nsec+=(timeout_ms%1000)*1000000;if(deadline.tv_nsec>=1000000000){deadline.tv_sec++;deadline.tv_nsec-=1000000000;}while(task->state<3){int rc=pthread_cond_timedwait(&pool->task_changed,&pool->mutex,&deadline);if(rc==ETIMEDOUT){timed_out=1;break;}}}
        }
        const char *status=timed_out?"timeout":task->state==1?"queued":task->state==2?"running":task->state==4?"cancelled":task->error[0]?"error":"finished";
        NcVal *result=nc_thread_pool_result(status,pool,task,timed_out?"timeout":"");pthread_mutex_unlock(&pool->mutex);return result;
    }
    if(!strcmp(op,"release")){
        pthread_mutex_lock(&pool->mutex);if(task->state<3){pthread_mutex_unlock(&pool->mutex);return nc_thread_pool_result("error",pool,task,"task not finished");}NcVal *result=nc_thread_pool_result("finished",pool,task,"");memset(task,0,sizeof(*task));pthread_mutex_unlock(&pool->mutex);return result;
    }
    return nc_thread_pool_result("error",pool,task,"invalid pool operation");
}

static NcVal *nc_builtin_thread_sync(NcVal *request) {
    const char *op = nc_atomic_text_field(request, "operation", "");
    if (!strcmp(op, "cancel")) {
        NcThreadSlot *slot = nc_thread_slot_from_request(request);
        if (!slot) return nc_thread_result("error", NULL, "", "invalid handle");
        atomic_store_explicit(&slot->cancel_requested, 1, memory_order_release);
        return nc_thread_result("cancelled", slot, "", "");
    }

    if (!strcmp(op, "signal_block") || !strcmp(op, "signal_unblock") || !strcmp(op, "signal_query")) {
#if defined(_WIN32)
        return nc_thread_result("error", NULL, "", "POSIX signal masks are unavailable on Windows");
#else
        NcVal *signals = nc_index_get(request, nc_str("signals"));
        if (!signals || signals->type != NC_LIST) return nc_thread_result("error", NULL, "", "signals must be list");
        sigset_t set;
        sigemptyset(&set);
        for (int i = 0; i < signals->list->len; i++) {
            NcVal *name = signals->list->items[i];
            if (!name || name->type != NC_STR) return nc_thread_result("error", NULL, "", "invalid signal name");
            int signo = 0;
            if (!strcmp(name->s, "USR1")) signo = SIGUSR1;
            else if (!strcmp(name->s, "USR2")) signo = SIGUSR2;
            else if (!strcmp(name->s, "TERM")) signo = SIGTERM;
            else if (!strcmp(name->s, "INT")) signo = SIGINT;
            else return nc_thread_result("error", NULL, "", "unsupported signal name");
            sigaddset(&set, signo);
        }
        if (!strcmp(op, "signal_query")) {
            sigset_t current;
            pthread_sigmask(SIG_SETMASK, NULL, &current);
            int all_blocked = 1;
            for (int i = 0; i < signals->list->len; i++) {
                const char *name = signals->list->items[i]->s;
                int signo = !strcmp(name, "USR1") ? SIGUSR1 : !strcmp(name, "USR2") ? SIGUSR2 : !strcmp(name, "TERM") ? SIGTERM : SIGINT;
                if (sigismember(&current, signo) != 1) all_blocked = 0;
            }
            return nc_thread_result("ready", NULL, all_blocked ? "sann" : "usann", "");
        }
        int how = !strcmp(op, "signal_block") ? SIG_BLOCK : SIG_UNBLOCK;
        int rc = pthread_sigmask(how, &set, NULL);
        return nc_thread_result(rc == 0 ? "ready" : "error", NULL, "", rc == 0 ? "" : "pthread_sigmask failed");
#endif
    }

    if (!strcmp(op, "mutex_create") || !strcmp(op, "condition_create")) {
        int type = !strcmp(op, "mutex_create") ? 1 : 2;
        pthread_mutex_lock(&g_sync_registry_lock);int id = 0;
        for (int i = 1; i < NC_SYNC_MAX; i++) if (!g_sync_slots[i].active) { id = i;g_sync_slots[i].generation++;if(!g_sync_slots[i].generation)g_sync_slots[i].generation=1;g_sync_slots[i].active=2;break; }
        pthread_mutex_unlock(&g_sync_registry_lock);
        if (!id) return nc_sync_result("error", 0, type == 1 ? "mutex" : "condition", "sync capacity");
        NcSyncSlot *created = &g_sync_slots[id];
        unsigned long long generation=created->generation;memset(created, 0, sizeof(*created));created->generation=generation;
        created->active = 2;
        created->type = type;
        if (type == 1) pthread_mutex_init(&created->mutex, NULL);
        else pthread_cond_init(&created->condition, NULL);
        pthread_mutex_lock(&g_sync_registry_lock);created->active=1;pthread_mutex_unlock(&g_sync_registry_lock);return nc_sync_result("ready", id, type == 1 ? "mutex" : "condition", "");
    }

    const char *handle = nc_atomic_text_field(request, "handle", "");
    if (!strcmp(op, "mutex_lock") || !strcmp(op, "mutex_unlock") || !strcmp(op, "mutex_destroy")) {
        NcSyncSlot *mutex = nc_sync_slot(handle, 1);
        if (!mutex) return nc_sync_result("error", 0, "mutex", "invalid mutex handle");
        int id = nc_sync_slot_id(mutex);
        if (!strcmp(op, "mutex_lock")) {
            long long timeout = nc_atomic_int_field(request, "timeout_ms", -1);
            int rc = timeout == 0 ? pthread_mutex_trylock(&mutex->mutex) : pthread_mutex_lock(&mutex->mutex);
            if (rc == EBUSY) return nc_sync_result("timeout", id, "mutex", "timeout");
            return nc_sync_result(rc == 0 ? "ready" : "error", id, "mutex", rc == 0 ? "" : "mutex lock failed");
        }
        if (!strcmp(op, "mutex_unlock")) {
            int rc = pthread_mutex_unlock(&mutex->mutex);
            return nc_sync_result(rc == 0 ? "ready" : "error", id, "mutex", rc == 0 ? "" : "mutex unlock failed");
        }
        int rc = pthread_mutex_destroy(&mutex->mutex);
        if (rc == 0){pthread_mutex_lock(&g_sync_registry_lock);mutex->active = 0;pthread_mutex_unlock(&g_sync_registry_lock);}
        return nc_sync_result(rc == 0 ? "finished" : "blocked", id, "mutex", rc == 0 ? "" : "mutex busy");
    }

    if (!strcmp(op, "condition_wait") || !strcmp(op, "condition_signal") || !strcmp(op, "condition_broadcast") || !strcmp(op, "condition_destroy")) {
        NcSyncSlot *condition = nc_sync_slot(handle, 2);
        if (!condition) return nc_sync_result("error", 0, "condition", "invalid condition handle");
        int id = nc_sync_slot_id(condition);
        if (!strcmp(op, "condition_wait")) {
            NcSyncSlot *mutex = nc_sync_slot(nc_atomic_text_field(request, "mutex_handle", ""), 1);
            if (!mutex) return nc_sync_result("error", id, "condition", "invalid mutex handle");
            long long timeout = nc_atomic_int_field(request, "timeout_ms", -1);
            int rc = 0;
            nc_gc_blocking_enter();
            if (timeout < 0) rc = pthread_cond_wait(&condition->condition, &mutex->mutex);
            else {
                struct timespec deadline;
                clock_gettime(CLOCK_REALTIME, &deadline);
                deadline.tv_sec += timeout / 1000;
                deadline.tv_nsec += (timeout % 1000) * 1000000;
                if (deadline.tv_nsec >= 1000000000) { deadline.tv_sec++; deadline.tv_nsec -= 1000000000; }
                rc = pthread_cond_timedwait(&condition->condition, &mutex->mutex, &deadline);
            }
            nc_gc_blocking_leave();
            if (rc == ETIMEDOUT) return nc_sync_result("timeout", id, "condition", "timeout");
            return nc_sync_result(rc == 0 ? "ready" : "error", id, "condition", rc == 0 ? "" : "condition wait failed");
        }
        if (!strcmp(op, "condition_signal")) pthread_cond_signal(&condition->condition);
        else if (!strcmp(op, "condition_broadcast")) pthread_cond_broadcast(&condition->condition);
        else {
            int rc = pthread_cond_destroy(&condition->condition);
            if (rc == 0){pthread_mutex_lock(&g_sync_registry_lock);condition->active = 0;pthread_mutex_unlock(&g_sync_registry_lock);}
            return nc_sync_result(rc == 0 ? "finished" : "blocked", id, "condition", rc == 0 ? "" : "condition busy");
        }
        return nc_sync_result("ready", id, "condition", "");
    }
    return nc_thread_result("error", NULL, "", "unsupported sync operation");
}
static NcVal *nc_exec_call_closure(NcVal *functions, const char *fn_name, NcVal **args, int nargs, NcVal *closure, int depth);
static int nc_val_til_exit(NcVal *v);
static NcVal *nc_native_kompiler(const char *src_path, const char *modul);
static char *nc_shell_quote(const char *s);
static NcVal *nc_seed_kompiler(const char *src_path, const char *modul);
static NcVal *nc_builtin_koyr_funksjon_host(NcVal **args, int na);
static NcVal *nc_builtin_vm_sett_kontekst_host(NcVal **args, int na);
static NcVal *nc_builtin_koyr_med_kontekst_host(NcVal **args, int na);

/* ── SQLite3 ABI ── */
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef long long sqlite3_int64;
#define SQLITE_OK   0
#define SQLITE_ROW  100
#define SQLITE_DONE 101
#if defined(_WIN32)
/* Windows has a system winsqlite3.dll, so the native runtime must not require
 * a developer-only import library just to link. */
typedef int (*nc_sqlite3_open_fn)(const char *, sqlite3 **);
typedef int (*nc_sqlite3_close_fn)(sqlite3 *);
typedef int (*nc_sqlite3_exec_fn)(sqlite3 *, const char *, int (*)(void *, int, char **, char **), void *, char **);
typedef void (*nc_sqlite3_free_fn)(void *);
typedef const char *(*nc_sqlite3_errmsg_fn)(sqlite3 *);
typedef int (*nc_sqlite3_prepare_v2_fn)(sqlite3 *, const char *, int, sqlite3_stmt **, const char **);
typedef int (*nc_sqlite3_bind_text_fn)(sqlite3_stmt *, int, const char *, int, void (*)(void *));
typedef int (*nc_sqlite3_step_fn)(sqlite3_stmt *);
typedef int (*nc_sqlite3_finalize_fn)(sqlite3_stmt *);
typedef const unsigned char *(*nc_sqlite3_column_text_fn)(sqlite3_stmt *, int);
typedef sqlite3_int64 (*nc_sqlite3_column_int64_fn)(sqlite3_stmt *, int);
typedef int (*nc_sqlite3_changes_fn)(sqlite3 *);
static HMODULE g_nc_sqlite_module = NULL;
static nc_sqlite3_open_fn g_nc_sqlite3_open;
static nc_sqlite3_close_fn g_nc_sqlite3_close;
static nc_sqlite3_exec_fn g_nc_sqlite3_exec;
static nc_sqlite3_free_fn g_nc_sqlite3_free;
static nc_sqlite3_errmsg_fn g_nc_sqlite3_errmsg;
static nc_sqlite3_prepare_v2_fn g_nc_sqlite3_prepare_v2;
static nc_sqlite3_bind_text_fn g_nc_sqlite3_bind_text;
static nc_sqlite3_step_fn g_nc_sqlite3_step;
static nc_sqlite3_finalize_fn g_nc_sqlite3_finalize;
static nc_sqlite3_column_text_fn g_nc_sqlite3_column_text;
static nc_sqlite3_column_int64_fn g_nc_sqlite3_column_int64;
static nc_sqlite3_changes_fn g_nc_sqlite3_changes;

static int nc_sqlite_load(void) {
    if (g_nc_sqlite_module) return g_nc_sqlite3_open != NULL;
    g_nc_sqlite_module = LoadLibraryA("winsqlite3.dll");
    if (!g_nc_sqlite_module) g_nc_sqlite_module = LoadLibraryA("sqlite3.dll");
    if (!g_nc_sqlite_module) return 0;
#define NC_SQLITE_SYMBOL(name) g_nc_##name = (nc_##name##_fn)GetProcAddress(g_nc_sqlite_module, #name)
    NC_SQLITE_SYMBOL(sqlite3_open);
    NC_SQLITE_SYMBOL(sqlite3_close);
    NC_SQLITE_SYMBOL(sqlite3_exec);
    NC_SQLITE_SYMBOL(sqlite3_free);
    NC_SQLITE_SYMBOL(sqlite3_errmsg);
    NC_SQLITE_SYMBOL(sqlite3_prepare_v2);
    NC_SQLITE_SYMBOL(sqlite3_bind_text);
    NC_SQLITE_SYMBOL(sqlite3_step);
    NC_SQLITE_SYMBOL(sqlite3_finalize);
    NC_SQLITE_SYMBOL(sqlite3_column_text);
    NC_SQLITE_SYMBOL(sqlite3_column_int64);
    NC_SQLITE_SYMBOL(sqlite3_changes);
#undef NC_SQLITE_SYMBOL
    return g_nc_sqlite3_open && g_nc_sqlite3_close && g_nc_sqlite3_exec &&
        g_nc_sqlite3_free && g_nc_sqlite3_errmsg && g_nc_sqlite3_prepare_v2 &&
        g_nc_sqlite3_bind_text && g_nc_sqlite3_step && g_nc_sqlite3_finalize &&
        g_nc_sqlite3_column_text && g_nc_sqlite3_column_int64 && g_nc_sqlite3_changes;
}
static int sqlite3_open(const char *p, sqlite3 **db) { return nc_sqlite_load() ? g_nc_sqlite3_open(p, db) : -1; }
static int sqlite3_close(sqlite3 *db) { return nc_sqlite_load() ? g_nc_sqlite3_close(db) : -1; }
static int sqlite3_exec(sqlite3 *db, const char *sql, int (*cb)(void *, int, char **, char **), void *arg, char **err) { return nc_sqlite_load() ? g_nc_sqlite3_exec(db, sql, cb, arg, err) : -1; }
static void sqlite3_free(void *p) { if (nc_sqlite_load()) g_nc_sqlite3_free(p); }
static const char *sqlite3_errmsg(sqlite3 *db) { return nc_sqlite_load() ? g_nc_sqlite3_errmsg(db) : "SQLite runtime library not found"; }
static int sqlite3_prepare_v2(sqlite3 *db, const char *sql, int n, sqlite3_stmt **stmt, const char **tail) { return nc_sqlite_load() ? g_nc_sqlite3_prepare_v2(db, sql, n, stmt, tail) : -1; }
static int sqlite3_bind_text(sqlite3_stmt *stmt, int n, const char *data, int len, void (*destroy)(void *)) { return nc_sqlite_load() ? g_nc_sqlite3_bind_text(stmt, n, data, len, destroy) : -1; }
static int sqlite3_step(sqlite3_stmt *stmt) { return nc_sqlite_load() ? g_nc_sqlite3_step(stmt) : -1; }
static int sqlite3_finalize(sqlite3_stmt *stmt) { return nc_sqlite_load() ? g_nc_sqlite3_finalize(stmt) : -1; }
static const unsigned char *sqlite3_column_text(sqlite3_stmt *stmt, int col) { return nc_sqlite_load() ? g_nc_sqlite3_column_text(stmt, col) : NULL; }
static sqlite3_int64 sqlite3_column_int64(sqlite3_stmt *stmt, int col) { return nc_sqlite_load() ? g_nc_sqlite3_column_int64(stmt, col) : 0; }
static int sqlite3_changes(sqlite3 *db) { return nc_sqlite_load() ? g_nc_sqlite3_changes(db) : 0; }
#else
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
#endif
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

static NcVal *nc_load_precompiled_functions(const char *path, const char *label) {
    NcVal *ncb_file = nc_builtin_fil_les(nc_str(path));
    if (!ncb_file || ncb_file->type != NC_STR) {
        nc_throw(label);
        return NULL;
    }
    NcVal *ncb = nc_builtin_json_parse_str(ncb_file);
    if (!ncb || ncb->type != NC_MAP) {
        nc_throw(label);
        return NULL;
    }
    NcVal *fns = nc_index_get(ncb, nc_str("functions"));
    if (!fns || fns->type != NC_MAP) {
        nc_throw(label);
        return NULL;
    }
    return fns;
}

static NcVal *nc_try_stage0_compiler_bundle(const char *src_text, const char *modul) {
    const char *root = getenv("NORSCODE_ROOT");
    const char *paths[] = {
        "build/v9400/hybrid_compiler_bundle_v9400.ncb.json",
        "build/v6000/compiler_stage0_v6000.ncb.json",
        "build/nc-regen/selfhost_kompiler.ncb.json",
        "bootstrap/kompiler.ncb.json",
        NULL
    };
    for (int pi = 0; paths[pi]; pi++) {
        char rooted[4096];
        const char *chosen = NULL;
        FILE *probe = fopen(paths[pi], "r");
        if (probe) {
            fclose(probe);
            chosen = paths[pi];
        } else if (root && root[0] && paths[pi][0] != '/') {
            snprintf(rooted, sizeof(rooted), "%s/%s", root, paths[pi]);
            probe = fopen(rooted, "r");
            if (probe) {
                fclose(probe);
                chosen = rooted;
            }
        }
        if (!chosen) continue;
        NcVal *ncb_file = nc_builtin_fil_les(nc_str(chosen));
        if (!ncb_file || ncb_file->type != NC_STR) continue;
        NcVal *ncb = nc_builtin_json_parse_raw(ncb_file);
        if (!ncb || ncb->type != NC_MAP) continue;
        NcVal *fns_v = nc_index_get(ncb, nc_str("functions"));
        if (!fns_v || fns_v->type != NC_MAP) continue;
        if (!nc_exec_find_fn(fns_v, "selfhost.kompiler.kompiler_fil")) continue;

        NcVal *saved_functions = g_current_functions;
        NcVal *saved_ncb = g_current_ncb;
        NcVal *saved_handlers = g_current_route_handlers;
        int saved_request_counter = g_request_counter;

        g_current_functions = fns_v;
        g_current_ncb = ncb;
        NcVal *rh = nc_index_get(ncb, nc_str("route_handlers"));
        g_current_route_handlers = (rh && rh->type != NC_NIL) ? rh : nc_list_new();
        g_request_counter = 0;

        NcVal *args[] = { nc_str(src_text), nc_str(modul) };
        NcVal *res = nc_exec_call(fns_v, "selfhost.kompiler.kompiler_fil", args, 2, 0);

        g_current_functions = saved_functions;
        g_current_ncb = saved_ncb;
        g_current_route_handlers = saved_handlers;
        g_request_counter = saved_request_counter;

        if (res && res->type == NC_STR) return res;
    }
    return nc_nil();
}

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

void nc_gc_host_mark_roots(void) {
    nc_gc_mark_value(g_current_route_handlers);
    nc_gc_mark_value(g_current_functions);
    nc_gc_mark_value(g_current_ncb);
    nc_gc_mark_value(g_std_env);
    nc_gc_mark_value(g_nc_closure_fwd);
    nc_gc_mark_value(g_sh_common_fns);
    nc_gc_mark_value(g_sh_ir_contract_fns);
    for (int i = 1; i < NC_THREAD_MAX; i++) {
        if (!g_thread_slots[i].active) continue;
        nc_gc_mark_value(g_thread_slots[i].args);
        nc_gc_mark_value(g_thread_slots[i].functions);
    }
    for (int p = 1; p < NC_THREAD_POOL_MAX; p++) {
        if (!g_thread_pools[p].active) continue;
        for (int i = 1; i < NC_THREAD_POOL_TASKS_MAX; i++) {
            if (!g_thread_pools[p].tasks[i].active) continue;
            nc_gc_mark_value(g_thread_pools[p].tasks[i].args);
            nc_gc_mark_value(g_thread_pools[p].tasks[i].functions);
        }
    }
}

void nc_gc_host_relocate(NcGcRelocation *table, size_t capacity) {
    g_current_route_handlers = nc_gc_relocation_get(table, capacity, g_current_route_handlers);
    g_current_functions = nc_gc_relocation_get(table, capacity, g_current_functions);
    g_current_ncb = nc_gc_relocation_get(table, capacity, g_current_ncb);
    g_std_env = nc_gc_relocation_get(table, capacity, g_std_env);
    g_nc_closure_fwd = nc_gc_relocation_get(table, capacity, g_nc_closure_fwd);
    g_sh_common_fns = nc_gc_relocation_get(table, capacity, g_sh_common_fns);
    g_sh_ir_contract_fns = nc_gc_relocation_get(table, capacity, g_sh_ir_contract_fns);
    for (int i = 1; i < NC_THREAD_MAX; i++) {
        if (!g_thread_slots[i].active) continue;
        g_thread_slots[i].args = nc_gc_relocation_get(table, capacity, g_thread_slots[i].args);
        g_thread_slots[i].functions = nc_gc_relocation_get(table, capacity, g_thread_slots[i].functions);
    }
    for (int p = 1; p < NC_THREAD_POOL_MAX; p++) {
        if (!g_thread_pools[p].active) continue;
        for (int i = 1; i < NC_THREAD_POOL_TASKS_MAX; i++) {
            if (!g_thread_pools[p].tasks[i].active) continue;
            g_thread_pools[p].tasks[i].args = nc_gc_relocation_get(table, capacity, g_thread_pools[p].tasks[i].args);
            g_thread_pools[p].tasks[i].functions = nc_gc_relocation_get(table, capacity, g_thread_pools[p].tasks[i].functions);
        }
    }
}

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

static NcVal *nc_builtin_pbkdf2_sha256(NcVal *password_v, NcVal *salt_v,
                                       NcVal *iterations_v, NcVal *length_v) {
#if defined(__APPLE__)
    char *password = nc_to_str_raw(password_v);
    char *salt = nc_to_str_raw(salt_v);
    int iterations = iterations_v && iterations_v->type == NC_INT ? (int)iterations_v->i : 0;
    int length = length_v && length_v->type == NC_INT ? (int)length_v->i : 0;
    if (iterations < 1 || iterations > 10000000 || length < 1 || length > 4096) {
        free(password); free(salt);
        nc_throw("PBKDF2-parametre utanfor tillaten grense");
        return nc_nil();
    }
    unsigned char *derived = calloc((size_t)length, 1);
    if (!derived) {
        free(password); free(salt);
        nc_throw("PBKDF2 kunne ikkje allokere resultat");
        return nc_nil();
    }
    int rc = CCKeyDerivationPBKDF(kCCPBKDF2,
        password, strlen(password),
        (const uint8_t *)salt, strlen(salt),
        kCCPRFHmacAlgSHA256, (uint32_t)iterations,
        derived, (size_t)length);
    free(password); free(salt);
    if (rc != 0) {
        free(derived);
        nc_throw("PBKDF2 native backend feila");
        return nc_nil();
    }
    char *hex = malloc((size_t)length * 2 + 1);
    if (!hex) {
        free(derived);
        nc_throw("PBKDF2 kunne ikkje allokere hex-resultat");
        return nc_nil();
    }
    static const char digits[] = "0123456789abcdef";
    for (int i = 0; i < length; i++) {
        hex[i * 2] = digits[(derived[i] >> 4) & 0xf];
        hex[i * 2 + 1] = digits[derived[i] & 0xf];
    }
    hex[length * 2] = '\0';
    free(derived);
    return nc_str_own(hex);
#elif defined(NC_ENABLE_OPENSSL)
    char *password = nc_to_str_raw(password_v);
    char *salt = nc_to_str_raw(salt_v);
    int iterations = iterations_v && iterations_v->type == NC_INT ? (int)iterations_v->i : 0;
    int length = length_v && length_v->type == NC_INT ? (int)length_v->i : 0;
    if (iterations < 1 || iterations > 10000000 || length < 1 || length > 4096) {
        free(password); free(salt);
        nc_throw("PBKDF2-parametre utanfor tillaten grense");
        return nc_nil();
    }
    unsigned char *derived = calloc((size_t)length, 1);
    if (!derived) {
        free(password); free(salt);
        nc_throw("PBKDF2 kunne ikkje allokere resultat");
        return nc_nil();
    }
    int rc = PKCS5_PBKDF2_HMAC(password, (int)strlen(password),
        (const unsigned char *)salt, (int)strlen(salt), iterations,
        EVP_sha256(), length, derived);
    free(password); free(salt);
    if (rc != 1) {
        free(derived);
        nc_throw("PBKDF2 native backend feila");
        return nc_nil();
    }
    char *hex = malloc((size_t)length * 2 + 1);
    if (!hex) {
        free(derived);
        nc_throw("PBKDF2 kunne ikkje allokere hex-resultat");
        return nc_nil();
    }
    static const char digits[] = "0123456789abcdef";
    for (int i = 0; i < length; i++) {
        hex[i * 2] = digits[(derived[i] >> 4) & 0xf];
        hex[i * 2 + 1] = digits[derived[i] & 0xf];
    }
    hex[length * 2] = '\0';
    free(derived);
    return nc_str_own(hex);
#else
    (void)password_v; (void)salt_v; (void)iterations_v; (void)length_v;
    nc_throw("PBKDF2 native backend er ikkje bygd på denne plattforma");
    return nc_nil();
#endif
}

static NcVal *nc_builtin_argon2id(NcVal *password_v, NcVal *salt_v,
                                  NcVal *memory_kib_v, NcVal *iterations_v,
                                  NcVal *parallelism_v, NcVal *length_v) {
#if defined(NC_ENABLE_ZIG_ARGON2)
    char *password = nc_to_str_raw(password_v);
    char *salt = nc_to_str_raw(salt_v);
    uint32_t memory_kib = memory_kib_v && memory_kib_v->type == NC_INT ? (uint32_t)memory_kib_v->i : 0;
    uint32_t iterations = iterations_v && iterations_v->type == NC_INT ? (uint32_t)iterations_v->i : 0;
    uint32_t parallelism = parallelism_v && parallelism_v->type == NC_INT ? (uint32_t)parallelism_v->i : 0;
    int length = length_v && length_v->type == NC_INT ? (int)length_v->i : 0;
    if (strlen(salt) < 8 || memory_kib < 8 || memory_kib > 1048576 ||
        iterations < 1 || iterations > 32 || parallelism < 1 || parallelism > 16 ||
        length < 1 || length > 4096) {
        free(password); free(salt);
        nc_throw("Argon2id-parametre utanfor tillaten grense");
        return nc_nil();
    }
    unsigned char *derived = calloc((size_t)length, 1);
    int rc = derived ? norscode_argon2id((const unsigned char *)password, strlen(password),
                                        (const unsigned char *)salt, strlen(salt),
                                        memory_kib, iterations, parallelism,
                                        derived, (size_t)length) : -1;
    free(password); free(salt);
    if (rc != 0) {
        free(derived);
        nc_throw(rc == -2 ? "Argon2id-parametre utanfor tillaten grense" :
                 "Argon2id Zig-backend feila");
        return nc_nil();
    }
    static const char digits[] = "0123456789abcdef";
    char *hex = malloc((size_t)length * 2 + 1);
    if (!hex) {
        free(derived);
        nc_throw("Argon2id kunne ikkje allokere hex-resultat");
        return nc_nil();
    }
    for (int i = 0; i < length; i++) {
        hex[i * 2] = digits[(derived[i] >> 4) & 0xf];
        hex[i * 2 + 1] = digits[derived[i] & 0xf];
    }
    hex[length * 2] = '\0';
    free(derived);
    return nc_str_own(hex);
#elif defined(NC_ENABLE_OPENSSL)
#if !defined(OSSL_KDF_PARAM_ARGON2_LANES) || !defined(OSSL_KDF_PARAM_ARGON2_MEMCOST) || !defined(OSSL_KDF_PARAM_THREADS)
    (void)password_v; (void)salt_v; (void)memory_kib_v; (void)iterations_v;
    (void)parallelism_v; (void)length_v;
    nc_throw("Argon2id native backend krev OpenSSL med Argon2-stotte");
    return nc_nil();
#else
    char *password = nc_to_str_raw(password_v);
    char *salt = nc_to_str_raw(salt_v);
    uint32_t memory_kib = memory_kib_v && memory_kib_v->type == NC_INT ? (uint32_t)memory_kib_v->i : 0;
    uint32_t iterations = iterations_v && iterations_v->type == NC_INT ? (uint32_t)iterations_v->i : 0;
    uint32_t parallelism = parallelism_v && parallelism_v->type == NC_INT ? (uint32_t)parallelism_v->i : 0;
    int length = length_v && length_v->type == NC_INT ? (int)length_v->i : 0;
    if (strlen(salt) < 8 || memory_kib < 8 || memory_kib > 1048576 ||
        iterations < 1 || iterations > 32 || parallelism < 1 || parallelism > 16 ||
        length < 1 || length > 4096) {
        free(password); free(salt);
        nc_throw("Argon2id-parametre utanfor tillaten grense");
        return nc_nil();
    }
    EVP_KDF *kdf = EVP_KDF_fetch(NULL, "ARGON2ID", NULL);
    EVP_KDF_CTX *ctx = kdf ? EVP_KDF_CTX_new(kdf) : NULL;
    if (!kdf || !ctx) {
        if (ctx) EVP_KDF_CTX_free(ctx);
        if (kdf) EVP_KDF_free(kdf);
        free(password); free(salt);
        nc_throw("Argon2id native backend er ikkje tilgjengeleg");
        return nc_nil();
    }
    uint32_t threads = 1;
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ITER, &iterations),
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_LANES, &parallelism),
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_MEMCOST, &memory_kib),
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_THREADS, &threads),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, salt, strlen(salt)),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD, password, strlen(password)),
        OSSL_PARAM_construct_end()
    };
    unsigned char *derived = calloc((size_t)length, 1);
    int ok = derived && EVP_KDF_derive(ctx, derived, (size_t)length, params) == 1;
    EVP_KDF_CTX_free(ctx);
    EVP_KDF_free(kdf);
    free(password); free(salt);
    if (!ok) {
        free(derived);
        nc_throw("Argon2id native backend feila");
        return nc_nil();
    }
    static const char digits[] = "0123456789abcdef";
    char *hex = malloc((size_t)length * 2 + 1);
    if (!hex) {
        free(derived);
        nc_throw("Argon2id kunne ikkje allokere hex-resultat");
        return nc_nil();
    }
    for (int i = 0; i < length; i++) {
        hex[i * 2] = digits[(derived[i] >> 4) & 0xf];
        hex[i * 2 + 1] = digits[derived[i] & 0xf];
    }
    hex[length * 2] = '\0';
    free(derived);
    return nc_str_own(hex);
#endif
#else
    (void)password_v; (void)salt_v; (void)memory_kib_v; (void)iterations_v;
    (void)parallelism_v; (void)length_v;
    nc_throw("Argon2id native backend er ikkje bygd på denne plattforma");
    return nc_nil();
#endif
}

static NcVal *nc_builtin_acme_sign(NcVal *algorithm_v, NcVal *private_key_v,
                                   NcVal *input_v) {
#if defined(NC_ENABLE_OPENSSL)
    char *algorithm = nc_to_str_raw(algorithm_v);
    char *private_key = nc_to_str_raw(private_key_v);
    char *input = nc_to_str_raw(input_v);
    const EVP_MD *digest = NULL;
    int is_eddsa = !strcmp(algorithm, "EdDSA");
    if (!strcmp(algorithm, "RS256") || !strcmp(algorithm, "ES256")) digest = EVP_sha256();
    if ((!digest && !is_eddsa) || private_key[0] == '\0' || input[0] == '\0') {
        free(algorithm); free(private_key); free(input);
        nc_throw("ACME signering: ugyldig algoritme eller nøkkel");
        return nc_nil();
    }
    BIO *bio = BIO_new_mem_buf(private_key, -1);
    EVP_PKEY *pkey = bio ? PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL) : NULL;
    if (bio) BIO_free(bio);
    if (!pkey) {
        free(algorithm); free(private_key); free(input);
        nc_throw("ACME signering: privatnøkkel kunne ikkje lesast");
        return nc_nil();
    }
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    size_t siglen = 0;
    int ok = ctx && EVP_DigestSignInit(ctx, NULL, digest, NULL, pkey) == 1 &&
             EVP_DigestSignUpdate(ctx, input, strlen(input)) == 1 &&
             EVP_DigestSignFinal(ctx, NULL, &siglen) == 1;
    unsigned char *signature = ok ? malloc(siglen ? siglen : 1) : NULL;
    if (ok) ok = signature && EVP_DigestSignFinal(ctx, signature, &siglen) == 1;
    unsigned char raw[4096]; size_t rawlen = siglen;
    if (ok && !strcmp(algorithm, "ES256")) {
        const unsigned char *der = signature;
        ECDSA_SIG *ecdsa = d2i_ECDSA_SIG(NULL, &der, (long)siglen);
        const BIGNUM *r = NULL, *s = NULL;
        if (!ecdsa) ok = 0;
        if (ok) ECDSA_SIG_get0(ecdsa, &r, &s);
        if (ok && BN_bn2binpad(r, raw, 32) != 32) ok = 0;
        if (ok && BN_bn2binpad(s, raw + 32, 32) != 32) ok = 0;
        if (ok) { rawlen = 64; }
        if (ecdsa) ECDSA_SIG_free(ecdsa);
    } else if (ok && siglen <= sizeof(raw)) {
        memcpy(raw, signature, siglen);
        rawlen = siglen;
    } else if (ok) {
        ok = 0;
    }
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    free(signature); free(algorithm); free(private_key); free(input);
    if (!ok) {
        nc_throw("ACME signering feila");
        return nc_nil();
    }
    static const char digits[] = "0123456789abcdef";
    char *hex = malloc(rawlen * 2 + 1);
    if (!hex) { nc_throw("ACME signering: minnefeil"); return nc_nil(); }
    for (size_t i = 0; i < rawlen; i++) {
        hex[i * 2] = digits[(raw[i] >> 4) & 0xf];
        hex[i * 2 + 1] = digits[raw[i] & 0xf];
    }
    hex[rawlen * 2] = '\0';
    return nc_str_own(hex);
#else
    (void)algorithm_v; (void)private_key_v; (void)input_v;
    nc_throw("ACME signering native backend er ikkje bygd på denne plattforma");
    return nc_nil();
#endif
}

static NcVal *nc_builtin_acme_verify(NcVal *algorithm_v, NcVal *public_key_v,
                                     NcVal *input_v, NcVal *signature_hex_v) {
#if defined(NC_ENABLE_OPENSSL)
    char *algorithm = nc_to_str_raw(algorithm_v);
    char *public_key = nc_to_str_raw(public_key_v);
    char *input = nc_to_str_raw(input_v);
    char *signature_hex = nc_to_str_raw(signature_hex_v);
    const EVP_MD *digest = NULL;
    int is_eddsa = !strcmp(algorithm, "EdDSA");
    if (!strcmp(algorithm, "RS256") || !strcmp(algorithm, "ES256")) digest = EVP_sha256();
    size_t hex_len = strlen(signature_hex);
    if ((!digest && !is_eddsa) || public_key[0] == '\0' || input[0] == '\0' ||
        hex_len == 0 || (hex_len % 2) != 0 || hex_len > 8192) {
        free(algorithm); free(public_key); free(input); free(signature_hex);
        nc_throw("ACME verifisering: ugyldige parameter");
        return nc_nil();
    }
    size_t signature_len = hex_len / 2;
    unsigned char *signature = malloc(signature_len ? signature_len : 1);
    if (!signature) {
        free(algorithm); free(public_key); free(input); free(signature_hex);
        nc_throw("ACME verifisering: minnefeil");
        return nc_nil();
    }
    for (size_t i = 0; i < signature_len; i++) {
        int hi = 0, lo = 0;
        char a = signature_hex[i * 2], b = signature_hex[i * 2 + 1];
        if (a >= '0' && a <= '9') hi = a - '0';
        else if (a >= 'a' && a <= 'f') hi = a - 'a' + 10;
        else if (a >= 'A' && a <= 'F') hi = a - 'A' + 10;
        else hi = -1;
        if (b >= '0' && b <= '9') lo = b - '0';
        else if (b >= 'a' && b <= 'f') lo = b - 'a' + 10;
        else if (b >= 'A' && b <= 'F') lo = b - 'A' + 10;
        else lo = -1;
        if (hi < 0 || lo < 0) {
            free(signature); free(algorithm); free(public_key); free(input); free(signature_hex);
            nc_throw("ACME verifisering: signaturen er ikkje hex");
            return nc_nil();
        }
        signature[i] = (unsigned char)((hi << 4) | lo);
    }
    unsigned char *verify_signature = signature;
    size_t verify_length = signature_len;
    unsigned char *der_signature = NULL;
    if (!strcmp(algorithm, "ES256")) {
        if (signature_len != 64) {
            free(signature); free(algorithm); free(public_key); free(input); free(signature_hex);
            nc_throw("ACME ES256-verifisering krev 64-byte råsignatur");
            return nc_nil();
        }
        ECDSA_SIG *ecdsa = ECDSA_SIG_new();
        BIGNUM *r = BN_bin2bn(signature, 32, NULL);
        BIGNUM *s = BN_bin2bn(signature + 32, 32, NULL);
        if (!ecdsa || !r || !s || ECDSA_SIG_set0(ecdsa, r, s) != 1) {
            if (ecdsa) ECDSA_SIG_free(ecdsa);
            else { BN_free(r); BN_free(s); }
            free(signature); free(algorithm); free(public_key); free(input); free(signature_hex);
            nc_throw("ACME ES256-verifisering feila");
            return nc_nil();
        }
        int der_length = i2d_ECDSA_SIG(ecdsa, NULL);
        der_signature = der_length > 0 ? malloc((size_t)der_length) : NULL;
        if (!der_signature || der_length <= 0) {
            ECDSA_SIG_free(ecdsa); free(der_signature); free(signature);
            free(algorithm); free(public_key); free(input); free(signature_hex);
            nc_throw("ACME ES256-verifisering feila");
            return nc_nil();
        }
        unsigned char *der_cursor = der_signature;
        i2d_ECDSA_SIG(ecdsa, &der_cursor);
        ECDSA_SIG_free(ecdsa);
        verify_signature = der_signature;
        verify_length = (size_t)der_length;
    }
    BIO *bio = BIO_new_mem_buf(public_key, -1);
    EVP_PKEY *pkey = bio ? PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL) : NULL;
    if (bio) BIO_free(bio);
    if (!pkey) {
        bio = BIO_new_mem_buf(public_key, -1);
        X509 *certificate = bio ? PEM_read_bio_X509(bio, NULL, NULL, NULL) : NULL;
        if (bio) BIO_free(bio);
        if (certificate) {
            pkey = X509_get_pubkey(certificate);
            X509_free(certificate);
        }
    }
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    int ok = pkey && ctx && EVP_DigestVerifyInit(ctx, NULL, digest, NULL, pkey) == 1 &&
             EVP_DigestVerifyUpdate(ctx, input, strlen(input)) == 1 &&
             EVP_DigestVerifyFinal(ctx, verify_signature, verify_length) == 1;
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    free(der_signature); free(signature);
    free(algorithm); free(public_key); free(input); free(signature_hex);
    return nc_bool(ok ? 1 : 0);
#else
    (void)algorithm_v; (void)public_key_v; (void)input_v; (void)signature_hex_v;
    nc_throw("ACME verifisering native backend er ikkje bygd på denne plattforma");
    return nc_nil();
#endif
}


/* Kjøyr ein funksjon */
typedef struct { int sp, try_depth, ip, caught; } NcExecControl;
static NcVal *nc_exec_call(NcVal *functions, const char *fn_name, NcVal **args, int nargs, int depth) {
    atomic_fetch_add_explicit(&g_native_vm_calls, 1, memory_order_relaxed);
    /* Store Helpdesk-program kan ha mange legitime VM-kall under kompilering.
     * Grensa er framleis eksplisitt og kan senkast i test/sandbox-miljø. */
    int max_depth = 256;
    const char *max_depth_env = getenv("NORSCODE_NATIVE_MAX_DEPTH");
    if (max_depth_env && max_depth_env[0]) {
        long configured = strtol(max_depth_env, NULL, 10);
        if (configured >= 32 && configured <= 16384) max_depth = (int)configured;
    }
    NcVal *fn_def = nc_exec_find_fn(functions, fn_name);
    if (depth > max_depth) {
        long source_line = 0;
        long source_column = 0;
        if (fn_def && fn_def->type == NC_MAP) {
            NcVal *lines = nc_index_get(fn_def, nc_str("source_lines"));
            NcVal *columns = nc_index_get(fn_def, nc_str("source_columns"));
            if (lines && lines->type == NC_LIST && lines->list->len > 0 &&
                lines->list->items[0]->type == NC_INT) {
                source_line = lines->list->items[0]->i;
            }
            if (columns && columns->type == NC_LIST && columns->list->len > 0 &&
                columns->list->items[0]->type == NC_INT) {
                source_column = columns->list->items[0]->i;
            }
        }
        const char *source_file = getenv("NORSCODE_FILE");
        if (!source_file || !source_file[0]) source_file = "<ukjent fil>";
        char diagnostic[768];
        snprintf(diagnostic, sizeof(diagnostic),
                 "For djup rekursjon: fil=%s, funksjon=%s, linje=%ld, kolonne=%ld, "
                 "uttrykk=%s(), djup=%d, grense=%d; "
                 "sett NORSCODE_NATIVE_MAX_DEPTH for denne kjøringa",
                 source_file, fn_name ? fn_name : "<ukjent>", source_line,
                 source_column, fn_name ? fn_name : "<ukjent>", depth, max_depth);
        nc_throw(diagnostic);
        return nc_nil();
    }
    if (!fn_def) {
        const char *builtin_name = fn_name;
        while (!strncmp(builtin_name, "builtin.", 8)) builtin_name += 8;
        if (!strncmp(builtin_name, "tekst_fra_heiltall", 20)) {
            return nc_builtin_tekst_fra_heltall(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strncmp(builtin_name, "atomic", 6)) {
            return nc_builtin_atomic_operation(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "jit_operation")) {
            return nc_builtin_jit_operation(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "tensor_operation")) {
            return nc_builtin_tensor_operation(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "media_diffusion_operation")) {
            return nc_builtin_media_diffusion_operation(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "bytes_new")) return nc_builtin_bytes_new(nargs > 0 ? args[0] : nc_int(0), nargs > 1 ? args[1] : nc_int(0));
        if (!strcmp(builtin_name, "bytes_from_list")) return nc_builtin_bytes_from_list(nargs > 0 ? args[0] : nc_nil());
        if (!strcmp(builtin_name, "bytes_to_list")) return nc_builtin_bytes_to_list(nargs > 0 ? args[0] : nc_nil());
        if (!strcmp(builtin_name, "fil_les_binary") || !strcmp(builtin_name, "fil_les_binær")) return nc_builtin_fil_les_binary(nargs > 0 ? args[0] : nc_nil());
        if (!strcmp(builtin_name, "process_spawn_argv")) {
            return nc_builtin_process_spawn_argv(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "process_operation")) {
            return nc_builtin_process_operation(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "filesystem_read_operation")) {
            return nc_builtin_filesystem_read_operation(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "filesystem_write_operation")) {
            return nc_builtin_filesystem_write_operation(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "native_mkdir_p")) {
            return nc_builtin_mkdir_p(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "sti_mkdir_p")) {
            return nc_builtin_mkdir_p(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "network_operation")) {
            return nc_builtin_network_operation(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strncmp(builtin_name, "thread_spawn", 12)) {
            return nc_builtin_thread_spawn(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strncmp(builtin_name, "thread_join", 11)) {
            return nc_builtin_thread_join(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strncmp(builtin_name, "thread_sync", 11)) {
            return nc_builtin_thread_sync(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strcmp(builtin_name, "thread_pool")) {
            return nc_builtin_thread_pool(nargs > 0 ? args[0] : nc_nil());
        }
        if (!strncmp(builtin_name, "thread_current_id", 17)) {
            return nc_int(g_native_thread_id);
        }
        nc_panic("Ukjent funksjon: %s", fn_name);
        return nc_nil();
    }

    NcVal *params_v = nc_index_get(fn_def, nc_str("params"));
    NcVal *code_v   = nc_index_get(fn_def, nc_str("code"));

    NcVal **stack_arr = calloc(512, sizeof(NcVal*));
    NcExecControl *control = calloc(1, sizeof(NcExecControl));
    NcVal **vars_arr  = calloc(2048, sizeof(NcVal*));
    char **varnames   = calloc(2048, sizeof(char*)); int nvars = 0;
    NcGcFrame gc_frame;
    nc_gc_frame_enter(&gc_frame, stack_arr, &control->sp, vars_arr, &nvars);
    nc_store(vars_arr, varnames, &nvars, "__gc_functions", functions);
    nc_store(vars_arr, varnames, &nvars, "__gc_fn_def", fn_def);
    nc_store(vars_arr, varnames, &nvars, "__gc_params", params_v);
    nc_store(vars_arr, varnames, &nvars, "__gc_code", code_v);
    /* TRY/CATCH stack */
    struct { const char *catch_lbl; int sp_depth; } try_stack[32];
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
    nc_store(vars_arr, varnames, &nvars, "__gc_labels", label_map);
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
    if (!code_v || code_v->type != NC_LIST) {
        nc_gc_frame_leave(&gc_frame);
        for (int i = 0; i < nvars; i++) free(varnames[i]);
        free(control); free(stack_arr); free(vars_arr); free(varnames); return retval;
    }

#define sp (control->sp)
#define try_depth (control->try_depth)
#define ip (control->ip)
    while (1) {
        atomic_fetch_add_explicit(&g_native_vm_instructions, 1, memory_order_relaxed);
        nc_gc_opcode_safepoint();
        functions = nc_load(vars_arr, varnames, nvars, "__gc_functions");
        fn_def = nc_load(vars_arr, varnames, nvars, "__gc_fn_def");
        params_v = nc_load(vars_arr, varnames, nvars, "__gc_params");
        code_v = nc_load(vars_arr, varnames, nvars, "__gc_code");
        label_map = nc_load(vars_arr, varnames, nvars, "__gc_labels");
        if (!code_v || code_v->type != NC_LIST || ip >= code_v->list->len) break;
        NcVal *instr = code_v->list->items[ip];
        if (!instr || instr->type != NC_LIST || instr->list->len < 1) { ip++; continue; }
        NcVal *op_v = instr->list->items[0];
        if (!op_v || op_v->type != NC_STR) { ip++; continue; }
        const char *op = op_v->s;
        nc_native_debug_observe(fn_name, ip, op, varnames, nvars, depth);

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
            if (strstr(callee, "builtin.")) {
                atomic_fetch_add_explicit(&g_native_vm_builtin_calls, 1, memory_order_relaxed);
            }
            if (strstr(cn, "fil_les") || strstr(cn, "fil_skriv") || strstr(cn, "filesystem_")) {
                atomic_fetch_add_explicit(&g_native_vm_filesystem_calls, 1, memory_order_relaxed);
                atomic_fetch_add_explicit(&g_native_vm_io_calls, 1, memory_order_relaxed);
            } else if (strstr(cn, "socket") || strstr(cn, "network_") || strstr(cn, "process_")) {
                atomic_fetch_add_explicit(&g_native_vm_io_calls, 1, memory_order_relaxed);
            }
            const char *security_denial = nc_native_security_check(cn, cargs, narg);
            if (security_denial) {
                atomic_fetch_add_explicit(&g_native_security_denied, 1, memory_order_relaxed);
                char security_error[640];
                snprintf(security_error, sizeof(security_error), "Sikkerheitsfeil: manglar capability %s", security_denial);
                free(cargs); free(callee);
                if (try_depth > 0) {
                    strncpy(last_exception, security_error, sizeof(last_exception) - 1);
                    last_exception[sizeof(last_exception) - 1] = '\0';
                    const char *catch_label = try_stack[try_depth - 1].catch_lbl;
                    sp = try_stack[try_depth - 1].sp_depth;
                    NcVal *target = nc_index_get(label_map, nc_str(catch_label));
                    if (target && target->type == NC_INT) { ip = (int)target->i + 1; continue; }
                }
                nc_throw(security_error);
            } else if (g_native_app_security_active && nc_native_capability_for(cn)) {
                atomic_fetch_add_explicit(&g_native_security_allowed, 1, memory_order_relaxed);
            }
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
            if (try_depth == 0 && strncmp(callee, "builtin.", 8) != 0 &&
                !nc_exec_prefer_dispatch(callee)) {
                NcVal *early_local = nc_exec_find_fn(functions, callee);
                if (early_local && early_local->type != NC_NIL) {
                    NcVal *local_r = nc_exec_call(functions, callee, cargs, narg, depth+1);
                    free(cargs); free(callee);
                    nc_push(&sp, stack_arr, local_r); ip++;
                    continue;
                }
            }
            if (try_depth > 0 && strncmp(callee, "builtin.", 8) != 0 &&
                !nc_exec_prefer_dispatch(callee)) {
                NcVal *local_with_try = nc_exec_find_fn(functions, callee);
                if (local_with_try && local_with_try->type != NC_NIL) {
                    jmp_buf saved_call_jmp;
                    memcpy(&saved_call_jmp, &g_err_jmp, sizeof(jmp_buf));
                    NcGcFrame *saved_call_boundary = g_err_gc_boundary;
                    g_err_gc_boundary = &gc_frame;
                    control->caught = 0;
                    if (setjmp(g_err_jmp)) {
                        memcpy(&g_err_jmp, &saved_call_jmp, sizeof(jmp_buf));
                        g_err_gc_boundary = saved_call_boundary;
                        const char *message = g_err_msg;
                        if (!strncmp(message, "Norscode unntak: ", 17)) message += 17;
                        else if (!strncmp(message, "nc-vm feil: Norscode unntak: ", 28)) message += 28;
                        strncpy(last_exception, message, sizeof(last_exception) - 1);
                        last_exception[sizeof(last_exception) - 1] = '\0';
                        g_err_msg[0] = '\0';
                        label_map = nc_load(vars_arr, varnames, nvars, "__gc_labels");
                        const char *catch_label = try_stack[try_depth - 1].catch_lbl;
                        sp = try_stack[try_depth - 1].sp_depth;
                        NcVal *target = nc_index_get(label_map, nc_str(catch_label));
                        if (target && target->type == NC_INT) {
                            ip = (int)target->i + 1;
                            control->caught = 1;
                        }
                    } else {
                        NcVal *local_result = nc_exec_call(functions, callee, cargs, narg, depth + 1);
                        memcpy(&g_err_jmp, &saved_call_jmp, sizeof(jmp_buf));
                        g_err_gc_boundary = saved_call_boundary;
                        free(cargs); free(callee);
                        nc_push(&sp, stack_arr, local_result); ip++;
                        continue;
                    }
                    free(cargs); free(callee);
                    if (control->caught) continue;
                    nc_throw(last_exception[0] ? last_exception : "ukjent feil");
                }
            }
            NcVal *fn_r = nc_nil();
            /* Bootstrap bytecode may qualify builtins while older seeds expect
             * their short names. Normalize at the host ABI boundary. */
            while (!strncmp(cn, "builtin.", 8)) cn += 8;
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
            else if (!strcmp(cn,"heiltall")||!strcmp(cn,"heiltall_fra_tekst")||!strcmp(cn,"heltall")||!strcmp(cn,"heltall_fra_tekst")) fn_r=nc_builtin_heltall(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"tekst_fra_heltall"))fn_r=nc_builtin_tekst_fra_heltall(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"tekst")||!strcmp(cn,"til_tekst"))     fn_r=nc_to_str(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"random_hex"))        fn_r=nc_builtin_random_hex(narg>0?cargs[0]:nc_int(32));
            else if (!strcmp(cn,"pbkdf2_sha256"))     fn_r=nc_builtin_pbkdf2_sha256(narg>0?cargs[0]:nc_nil(), narg>1?cargs[1]:nc_nil(), narg>2?cargs[2]:nc_int(0), narg>3?cargs[3]:nc_int(0));
            else if (!strcmp(cn,"argon2id"))          fn_r=nc_builtin_argon2id(narg>0?cargs[0]:nc_nil(), narg>1?cargs[1]:nc_nil(), narg>2?cargs[2]:nc_int(0), narg>3?cargs[3]:nc_int(0), narg>4?cargs[4]:nc_int(0), narg>5?cargs[5]:nc_int(0));
            else if (!strcmp(cn,"acme_sign"))         fn_r=nc_builtin_acme_sign(narg>0?cargs[0]:nc_nil(), narg>1?cargs[1]:nc_nil(), narg>2?cargs[2]:nc_nil());
            else if (!strcmp(cn,"acme_verify"))       fn_r=nc_builtin_acme_verify(narg>0?cargs[0]:nc_nil(), narg>1?cargs[1]:nc_nil(), narg>2?cargs[2]:nc_nil(), narg>3?cargs[3]:nc_nil());
            else if (!strcmp(cn,"tid_ms")||!strcmp(cn,"now_ms")) fn_r=nc_builtin_tid_ms();
            else if (!strcmp(cn,"tid_no")||!strcmp(cn,"tid")||!strcmp(cn,"timestamp")||!strcmp(cn,"unix_timestamp")) fn_r=nc_builtin_tid_no();
            else if (!strcmp(cn,"now")||!strcmp(cn,"now_iso")) fn_r=nc_builtin_now_iso();
            else if (!strcmp(cn,"gc_collect"))        { nc_gc_collect(); fn_r=nc_bool(1); }
            else if (!strcmp(cn,"exec_prosess"))      fn_r=nc_builtin_exec_prosess(narg>0?cargs[0]:nc_str(""));
            else if (!strcmp(cn,"socket_new"))        fn_r=nc_builtin_socket_new(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"socket_connect"))    fn_r=nc_builtin_socket_connect(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil(),narg>2?cargs[2]:nc_nil());
            else if (!strcmp(cn,"socket_bind"))       fn_r=nc_builtin_socket_bind(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil(),narg>2?cargs[2]:nc_nil());
            else if (!strcmp(cn,"socket_listen"))     fn_r=nc_builtin_socket_listen(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"socket_accept"))     fn_r=nc_builtin_socket_accept(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"socket_send"))       fn_r=nc_builtin_socket_send(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"socket_send_bytes")) fn_r=nc_builtin_socket_send_bytes(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"socket_write"))      fn_r=nc_builtin_socket_write(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"socket_recv"))       fn_r=nc_builtin_socket_recv(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"socket_recv_bytes")) fn_r=nc_builtin_socket_recv_bytes(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"socket_read"))       fn_r=nc_builtin_socket_read(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            else if (!strcmp(cn,"socket_close"))      fn_r=nc_builtin_socket_close(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"socket_settimeout")) fn_r=nc_builtin_socket_settimeout(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
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
            else if (!strcmp(cn,"json_stringify") || !strcmp(cn,"std.json.stringify") || !strcmp(cn,"builtin.json.stringify"))
                fn_r=nc_builtin_json_stringify_smart(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"std.json.parse") || !strcmp(cn,"builtin.json.parse"))
                fn_r=nc_builtin_json_parse_raw(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"json_parse"))       fn_r=nc_builtin_json_parse_norscode(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"kompiler_fil"))     fn_r=nc_dispatch_call("selfhost.kompiler.kompiler_fil", cargs, narg);
            else if (!strcmp(cn,"fil_les")||!strcmp(cn,"fil_skriv")) {
                /* fil_les og fil_skriv kan kaste IOFeil — bruk setjmp */
                control->caught = 0;
                if (try_depth > 0) {
                    jmp_buf _fil_saved; memcpy(&_fil_saved,&g_err_jmp,sizeof(jmp_buf));
                    NcGcFrame *_fil_saved_boundary = g_err_gc_boundary;
                    g_err_gc_boundary = &gc_frame;
                    if (setjmp(g_err_jmp)) {
                        memcpy(&g_err_jmp,&_fil_saved,sizeof(jmp_buf));
                        g_err_gc_boundary = _fil_saved_boundary;
                        if (try_depth > 0) {
                            { const char *_em=g_err_msg;
                              if (strncmp(_em,"Norscode unntak: ",17)==0) _em+=17;
                              else if (strncmp(_em,"nc-vm feil: Norscode unntak: ",28)==0) _em+=28;
                              strncpy(last_exception,_em,sizeof(last_exception)-1); }
                            g_err_msg[0]=0;
                            const char *_cl=try_stack[try_depth-1].catch_lbl;
                            sp=try_stack[try_depth-1].sp_depth;
                            NcVal *_tgt=nc_index_get(label_map,nc_str(_cl));
                            if (_tgt&&_tgt->type==NC_INT){ip=(int)_tgt->i+1;control->caught=1;}
                        }
                        fn_r=nc_nil();
                    } else {
                        if (!strcmp(cn,"fil_les")) fn_r=nc_builtin_fil_les(narg>0?cargs[0]:nc_nil());
                        else if (narg>=2) nc_builtin_fil_skriv(cargs[0],cargs[1]);
                        memcpy(&g_err_jmp,&_fil_saved,sizeof(jmp_buf));
                        g_err_gc_boundary = _fil_saved_boundary;
                    }
                } else {
                    if (!strcmp(cn,"fil_les")) fn_r=nc_builtin_fil_les(narg>0?cargs[0]:nc_nil());
                    else if (narg>=2) nc_builtin_fil_skriv(cargs[0],cargs[1]);
                }
                if (control->caught) { free(cargs); free(callee); continue; }
            }
            else if (!strcmp(cn,"fil_skriv_bin\xc3\xa6r")||!strcmp(cn,"fil_skriv_binary")) { if(narg>=2) nc_builtin_fil_skriv_binary(cargs[0],cargs[1]); }
            else if (!strcmp(cn,"mkdir")||!strcmp(cn,"mkdir_p")||!strcmp(cn,"mappe_opprett")||!strcmp(cn,"builtin.mkdir")||!strcmp(cn,"builtin.mkdir_p")||!strcmp(cn,"builtin.mappe_opprett")) fn_r=nc_builtin_mkdir_p(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"char_code")) fn_r=nc_builtin_char_code(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"chr")) fn_r=nc_builtin_chr(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"atomic_operation")) fn_r=nc_builtin_atomic_operation(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"jit_operation")) fn_r=nc_builtin_jit_operation(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"tensor_operation")) fn_r=nc_builtin_tensor_operation(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"media_diffusion_operation")) fn_r=nc_builtin_media_diffusion_operation(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"process_spawn_argv")) fn_r=nc_builtin_process_spawn_argv(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"process_operation")) fn_r=nc_builtin_process_operation(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"filesystem_read_operation")) fn_r=nc_builtin_filesystem_read_operation(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"filesystem_write_operation")) fn_r=nc_builtin_filesystem_write_operation(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"native_mkdir_p")) fn_r=nc_builtin_mkdir_p(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"sti_mkdir_p")) fn_r=nc_builtin_mkdir_p(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"network_operation")) fn_r=nc_builtin_network_operation(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"dns_lookup") || !strcmp(cn,"resolve_host")) fn_r=nc_builtin_dns_lookup(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_str("0"));
            else if (!strcmp(cn,"thread_spawn")) fn_r=nc_builtin_thread_spawn(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"thread_join")) fn_r=nc_builtin_thread_join(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"thread_sync")) fn_r=nc_builtin_thread_sync(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"thread_pool")) fn_r=nc_builtin_thread_pool(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"thread_current_id")) fn_r=nc_int(g_native_thread_id);
            else if (!strcmp(cn,"json_parse_raw")) fn_r=nc_builtin_json_parse_raw(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"fil_finnes"))       fn_r=nc_builtin_fil_finnes(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"miljo_hent"))       fn_r=nc_builtin_miljo_hent(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"miljo_finnes"))     fn_r=nc_builtin_miljo_finnes(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"køyr_funksjon"))    fn_r=nc_builtin_koyr_funksjon_host(cargs, narg);
            else if (!strcmp(cn,"vm_sett_kontekst")) fn_r=nc_builtin_vm_sett_kontekst_host(cargs, narg);
            else if (!strcmp(cn,"køyr_med_kontekst")) fn_r=nc_builtin_koyr_med_kontekst_host(cargs, narg);
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
            else if (!strcmp(cn,"fil_slett") || !strcmp(cn,"builtin.fil_slett")) fn_r=nc_fn_builtin_fil_slett(cargs, narg);
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
                    NcGcFrame *_sh_saved_boundary = g_err_gc_boundary;
                    g_err_gc_boundary = &gc_frame;
                    control->caught = 0;
                    if (setjmp(g_err_jmp)) {
                        memcpy(&g_err_jmp, &_sh_saved_jmp, sizeof(jmp_buf));
                        g_err_gc_boundary = _sh_saved_boundary;
                        if (try_depth > 0) {
                            const char *_em = g_err_msg;
                            if (strncmp(_em,"Norscode unntak: ",17)==0) _em+=17;
                            else if (strncmp(_em,"nc-vm feil: Norscode unntak: ",28)==0) _em+=28;
                            strncpy(last_exception, _em, sizeof(last_exception)-1);
                            g_err_msg[0] = 0;
                            const char *_cl = try_stack[try_depth - 1].catch_lbl;
                            sp = try_stack[try_depth - 1].sp_depth;
                            NcVal *_tgt = nc_index_get(label_map, nc_str(_cl));
                            if (_tgt && _tgt->type == NC_INT) { ip = (int)_tgt->i + 1; control->caught = 1; }
                        }
                        fn_r = nc_nil();
                    } else {
                        /* Prøv bundle-funksjonen først, deretter sh-api */
                        NcVal *_local = nc_exec_find_fn(functions, callee);
                        if (_local) fn_r = nc_exec_call(functions, callee, cargs, narg, depth+1);
                        else fn_r = nc_call_sh_api(sh_cn, cargs, narg);
                        memcpy(&g_err_jmp, &_sh_saved_jmp, sizeof(jmp_buf));
                        g_err_gc_boundary = _sh_saved_boundary;
                    }
                    if (control->caught) { free(cargs); free(callee); continue; }
                } else {
                    /* Viktig: føretrekk lokale bundle-funksjonar også utan try/fang.
                     * Elles blir selfhost.parser / selfhost.compiler / selfhost.kompiler
                     * feilrouta til common.no-host-API, sjølv når ferske lokale
                     * funksjonar allereie finst i aktuell NCB-bundle. */
                    NcVal *_local = nc_exec_find_fn(functions, callee);
                    if (_local) fn_r = nc_exec_call(functions, callee, cargs, narg, depth+1);
                    else fn_r = nc_call_sh_api(sh_cn, cargs, narg);
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
            else if (!strcmp(cn,"sti_mkdir_p")||!strcmp(cn,"path.mkdir_p"))   fn_r=nc_builtin_mkdir_p(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"sti_stem")||!strcmp(cn,"path.stem"))         fn_r=nc_builtin_sti_stem(narg>0?cargs[0]:nc_nil());
            else if (!strcmp(cn,"miljo_sett")||!strcmp(cn,"env.sett"))        fn_r=nc_builtin_miljo_sett(narg>0?cargs[0]:nc_nil(),narg>1?cargs[1]:nc_nil());
            /* Web-builtins */
            else if (!strcmp(cn,"ncb_route_handlers")) fn_r=nc_builtin_ncb_route_handlers(cargs,narg);
            else if (!strcmp(cn,"ncb_metadata"))       fn_r=nc_builtin_ncb_metadata(cargs,narg);
            else if (!strcmp(cn,"vm_function_info"))  fn_r=nc_builtin_vm_function_info(cargs,narg);
            else if (!strcmp(cn,"ncb_next_request_id"))fn_r=nc_builtin_ncb_next_request_id(cargs,narg);
            else if (!strcmp(cn,"ncb_call_fn")) {
                /* ncb_call_fn kan kaste unntak — bruk setjmp ved try_depth > 0 */
                if (try_depth > 0) {
                    jmp_buf _ncb_saved;
                    memcpy(&_ncb_saved, &g_err_jmp, sizeof(jmp_buf));
                    NcGcFrame *_ncb_saved_boundary = g_err_gc_boundary;
                    g_err_gc_boundary = &gc_frame;
                    control->caught = 0;
                    if (setjmp(g_err_jmp)) {
                        memcpy(&g_err_jmp, &_ncb_saved, sizeof(jmp_buf));
                        g_err_gc_boundary = _ncb_saved_boundary;
                        if (try_depth > 0) {
                            const char *_em = g_err_msg;
                            if (strncmp(_em,"Norscode unntak: ",17)==0) _em+=17;
                            else if (strncmp(_em,"nc-vm feil: Norscode unntak: ",28)==0) _em+=28;
                            strncpy(last_exception, _em, sizeof(last_exception)-1);
                            g_err_msg[0] = 0;
                            const char *_cl = try_stack[try_depth-1].catch_lbl;
                            sp = try_stack[try_depth-1].sp_depth;
                            NcVal *_tgt = nc_index_get(label_map, nc_str(_cl));
                            if (_tgt && _tgt->type == NC_INT) { ip = (int)_tgt->i + 1; control->caught = 1; }
                        }
                        fn_r = nc_nil();
                    } else {
                        fn_r = nc_builtin_ncb_call_fn(cargs, narg);
                        memcpy(&g_err_jmp, &_ncb_saved, sizeof(jmp_buf));
                        g_err_gc_boundary = _ncb_saved_boundary;
                    }
                    if (control->caught) { free(cargs); free(callee); continue; }
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
                else if (!strncmp(cn, "std.lagring.finnes", 18) || !strncmp(cn, "builtin.lagring.finnes", 22)) {
                    fn_r = nc_builtin_fil_finnes(narg>0?cargs[0]:nc_nil());
                }
                /* builtin.path.* dispatchers (same handlers as std.path.*) */
                else if (!strncmp(cn, "builtin.path.basename", 21)) fn_r = nc_std_path_basename(cargs, narg);
                else if (!strncmp(cn, "builtin.path.dirname", 20)) fn_r = nc_std_path_dirname(cargs, narg);
                else if (!strncmp(cn, "builtin.path.stem", 17)) fn_r = nc_std_path_stem(cargs, narg);
                else if (!strncmp(cn, "builtin.path.join", 17)) fn_r = nc_std_path_join(cargs, narg);
                else if (!strncmp(cn, "builtin.path.exists", 19)) fn_r = nc_std_path_exists(cargs, narg);
                else if (!strncmp(cn, "builtin.env.sett", 16)) fn_r = nc_std_env_sett(cargs, narg);
                else if (!strncmp(cn, "builtin.env.hent", 16)) fn_r = nc_std_env_hent(cargs, narg);
                else if (!strncmp(cn, "builtin.env.finnes", 18)) fn_r = nc_std_env_finnes(cargs, narg);
                else if (strstr(cn, "request_context")) {
                    fn_r = nc_map_new();
                    NcVal *method = narg > 0 ? cargs[0] : nc_str("");
                    NcVal *path = narg > 1 ? cargs[1] : nc_str("");
                    NcVal *query = narg > 2 ? cargs[2] : nc_map_new();
                    NcVal *headers = narg > 3 ? cargs[3] : nc_map_new();
                    NcVal *body = narg > 4 ? cargs[4] : nc_str("");
                    nc_index_set(fn_r, nc_str("__method__"), method);
                    nc_index_set(fn_r, nc_str("__path__"), path);
                    nc_index_set(fn_r, nc_str("__query__"), query);
                    nc_index_set(fn_r, nc_str("__headers__"), headers);
                    nc_index_set(fn_r, nc_str("__body__"), body);
                    nc_index_set(fn_r, nc_str("__params__"), nc_map_new());
                    nc_index_set(fn_r, nc_str("method"), method);
                    nc_index_set(fn_r, nc_str("path"), path);
                    nc_index_set(fn_r, nc_str("query"), query);
                    nc_index_set(fn_r, nc_str("headers"), headers);
                    nc_index_set(fn_r, nc_str("body"), body);
                    nc_index_set(fn_r, nc_str("params"), nc_map_new());
                }
                else if (!strcmp(cn, "std.web.request_context")) {
                    fn_r = nc_map_new();
                    NcVal *method = narg > 0 ? cargs[0] : nc_str("");
                    NcVal *path = narg > 1 ? cargs[1] : nc_str("");
                    NcVal *query = narg > 2 ? cargs[2] : nc_map_new();
                    NcVal *headers = narg > 3 ? cargs[3] : nc_map_new();
                    NcVal *body = narg > 4 ? cargs[4] : nc_str("");
                    nc_index_set(fn_r, nc_str("__method__"), method);
                    nc_index_set(fn_r, nc_str("__path__"), path);
                    nc_index_set(fn_r, nc_str("__query__"), query);
                    nc_index_set(fn_r, nc_str("__headers__"), headers);
                    nc_index_set(fn_r, nc_str("__body__"), body);
                    nc_index_set(fn_r, nc_str("__params__"), nc_map_new());
                    nc_index_set(fn_r, nc_str("method"), method);
                    nc_index_set(fn_r, nc_str("path"), path);
                    nc_index_set(fn_r, nc_str("query"), query);
                    nc_index_set(fn_r, nc_str("headers"), headers);
                    nc_index_set(fn_r, nc_str("body"), body);
                    nc_index_set(fn_r, nc_str("params"), nc_map_new());
                }
                else if (strstr(cn, "handle_request")) {
                    fn_r = NULL;
                    NcVal *ctx = narg > 0 ? cargs[0] : nc_nil();
                    NcVal *method_v = (ctx && ctx->type == NC_MAP) ? nc_index_get(ctx, nc_str("__method__")) : NULL;
                    NcVal *path_v = (ctx && ctx->type == NC_MAP) ? nc_index_get(ctx, nc_str("__path__")) : NULL;
                    char *method = method_v ? nc_to_str_raw(method_v) : strdup("");
                    char *path = path_v ? nc_to_str_raw(path_v) : strdup("");
                    char *method_up = method ? strdup(method) : strdup("");
                    for (char *p = method_up; p && *p; ++p) if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 32);
                    NcVal *found = NULL;
                    NcVal *allowed = nc_map_new();
                    int path_matched = 0;
                    int matched_verb = 0;
                    int matched_allow = 0;
                    int i = 0;
                    if (!g_current_route_handlers || g_current_route_handlers->type != NC_LIST) {
                        fn_r = nc_nil();
                        free(method); free(path); free(method_up);
                        continue;
                    }
                    while (i < g_current_route_handlers->list->len) {
                        NcVal *handler = g_current_route_handlers->list->items[i];
                        if (handler && handler->type == NC_MAP) {
                            NcVal *spec_v = nc_index_get(handler, nc_str("spec"));
                            NcVal *fn_v = nc_index_get(handler, nc_str("function"));
                            char *spec = spec_v ? nc_to_str_raw(spec_v) : NULL;
                            char *fn_name = fn_v ? nc_to_str_raw(fn_v) : NULL;
                            if (spec && fn_name) {
                                char *spc = strdup(spec);
                                char *space = strchr(spc, ' ');
                                if (space) {
                                    *space = 0;
                                    char *spec_method = spc;
                                    char *spec_path = space + 1;
                                    char *spec_method_up = strdup(spec_method);
                                    for (char *p = spec_method_up; p && *p; ++p) if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 32);
                                    if (!strcmp(spec_method_up, method_up)) {
                                        /* exact eller enkel {id:int}-match */
                                        if (!strcmp(spec_path, path)) {
                                            path_matched = 1;
                                        } else {
                                            char *spec_copy = strdup(spec_path);
                                            char *pp = strdup(path);
                                            char *spec_save = NULL;
                                            char *path_save = NULL;
                                            char *sp_tok = strtok_r(spec_copy, "/", &spec_save);
                                            char *pp_tok = strtok_r(pp, "/", &path_save);
                                            int ok = 1;
                                            NcVal *params = nc_map_new();
                                            while (sp_tok || pp_tok) {
                                                if (!sp_tok || !pp_tok) { ok = 0; break; }
                                                if (sp_tok[0] == '{') {
                                                    char *colon = strchr(sp_tok, ':');
                                                    char *endb = strchr(sp_tok, '}');
                                                    if (!endb || endb[1] != '\0') { ok = 0; break; }
                                                    char *name_end = colon ? colon : endb;
                                                    size_t param_len = (size_t)(name_end - (sp_tok + 1));
                                                    char param_name[128];
                                                    if (param_len == 0 || param_len >= sizeof(param_name)) { ok = 0; break; }
                                                    memcpy(param_name, sp_tok + 1, param_len);
                                                    param_name[param_len] = '\0';
                                                    if (colon && !strncmp(colon + 1, "int}", 4)) {
                                                        for (const char *digit = pp_tok; *digit; digit++) {
                                                            if (*digit < '0' || *digit > '9') { ok = 0; break; }
                                                        }
                                                        if (!ok || !pp_tok[0]) break;
                                                    }
                                                    nc_index_set(params, nc_str(param_name), nc_str(pp_tok));
                                                } else if (strcmp(sp_tok, pp_tok) != 0) {
                                                    ok = 0; break;
                                                }
                                                sp_tok = strtok_r(NULL, "/", &spec_save);
                                                pp_tok = strtok_r(NULL, "/", &path_save);
                                            }
                                            if (ok && !sp_tok && !pp_tok) {
                                                path_matched = 1;
                                                if (ctx && ctx->type == NC_MAP) nc_index_set(ctx, nc_str("__params__"), params);
                                            }
                                            free(spec_copy); free(pp);
                                        }
                                        if (getenv("NORSCODE_WEB_DEBUG") && !strcmp(getenv("NORSCODE_WEB_DEBUG"), "1")) {
                                            fprintf(stderr, "[web-route] %s %s mot %s => %s\n",
                                                    method_up, path, spec_path, path_matched ? "treff" : "ikkje");
                                        }
                                        if (path_matched) {
                                            matched_allow = 1;
                                            found = handler;
                                            matched_verb = 1;
                                            NcVal *guards = nc_index_get(handler, nc_str("guards"));
                                            if (!guards || guards->type != NC_LIST) guards = nc_list_new();
                                            int gi = 0;
                                            int guard_ok = 1;
                                            while (gi < guards->list->len && guard_ok) {
                                                NcVal *g = guards->list->items[gi];
                                                char *gname = g ? nc_to_str_raw(g) : NULL;
                                                if (gname) {
                                                    char full[512];
                                                    snprintf(full, sizeof(full), "__main__.%s", gname);
                                                    NcVal *ga[] = { nc_str(full), ctx };
                                                    NcVal *gr = nc_builtin_ncb_call_fn(ga, 2);
                                                    if (!gr || !nc_truthy(gr)) guard_ok = 0;
                                                    free(gname);
                                                }
                                                gi++;
                                            }
                                            if (!guard_ok) {
                                                fn_r = nc_map_new();
                                                nc_index_set(fn_r, nc_str("__status__"), nc_int(403));
                                                nc_index_set(fn_r, nc_str("__headers__"), nc_map_new());
                                                nc_index_set(fn_r, nc_str("__body__"), nc_str("{\"error\":\"Forbudt\"}"));
                                                free(spc); free(spec_method_up); free(spec); free(fn_name);
                                                break;
                                            }
                                            NcVal *call_args[] = { nc_str(fn_name), ctx };
                                            NcVal *resp = nc_builtin_ncb_call_fn(call_args, 2);
                                            if (getenv("NORSCODE_WEB_DEBUG") && !strcmp(getenv("NORSCODE_WEB_DEBUG"), "1")) {
                                                NcVal *debug_status = resp && resp->type == NC_MAP ? nc_index_get(resp, nc_str("status")) : NULL;
                                                fprintf(stderr, "[web-route] handler=%s type=%d status_type=%d\n", fn_name,
                                                        resp ? resp->type : -1, debug_status ? debug_status->type : -1);
                                            }
                                            fn_r = resp ? resp : nc_nil();
                                            free(spc); free(spec_method_up); free(spec); free(fn_name);
                                            break;
                                        }
                                    }
                                    free(spec_method_up);
                                }
                                free(spc); free(spec); free(fn_name);
                            }
                        }
                        i++;
                    }
                    if (!fn_r) {
                        fn_r = nc_map_new();
                        if (path_matched || matched_verb || matched_allow) {
                            nc_index_set(fn_r, nc_str("__status__"), nc_int(405));
                            nc_index_set(fn_r, nc_str("__headers__"), nc_map_new());
                            nc_index_set(fn_r, nc_str("__body__"), nc_str("{\"error\":\"Metode ikkje tillatt\"}"));
                        } else {
                            nc_index_set(fn_r, nc_str("__status__"), nc_int(404));
                            nc_index_set(fn_r, nc_str("__headers__"), nc_map_new());
                            nc_index_set(fn_r, nc_str("__body__"), nc_str("{\"error\":\"Ikkje funne\"}"));
                        }
                    }
                    free(method); free(path); free(method_up);
                }
                else if (strstr(cn, "response_status")) {
                    NcVal *resp = narg > 0 ? cargs[0] : nc_nil();
                    if (resp && resp->type == NC_MAP) {
                        NcVal *v = nc_index_get(resp, nc_str("__status__"));
                        if (!v || v->type == NC_NIL) v = nc_index_get(resp, nc_str("status"));
                        fn_r = (v && v->type == NC_INT) ? nc_int((int)v->i) : nc_nil();
                    } else {
                        fn_r = nc_nil();
                    }
                }
                else if (strstr(cn, "response_body")) {
                    NcVal *resp = narg > 0 ? cargs[0] : nc_nil();
                    if (resp && resp->type == NC_MAP) {
                        NcVal *v = nc_index_get(resp, nc_str("__body__"));
                        if (!v || v->type == NC_NIL) v = nc_index_get(resp, nc_str("body"));
                        fn_r = (v && v->type == NC_STR) ? nc_str(v->s) : nc_str("");
                    } else {
                        fn_r = nc_str("");
                    }
                }
                else if (strstr(cn, "response_headers")) {
                    NcVal *resp = narg > 0 ? cargs[0] : nc_nil();
                    if (resp && resp->type == NC_MAP) {
                        NcVal *v = nc_index_get(resp, nc_str("__headers__"));
                        if (!v) v = nc_index_get(resp, nc_str("headers"));
                        fn_r = (v && v->type != NC_NIL) ? v : nc_map_new();
                    } else {
                        fn_r = nc_map_new();
                    }
                }
                else if (strstr(cn, "request_body")) {
                    NcVal *ctx = narg > 0 ? cargs[0] : nc_nil();
                    if (ctx && ctx->type == NC_MAP) {
                        NcVal *v = nc_index_get(ctx, nc_str("__body__"));
                        if (!v) v = nc_index_get(ctx, nc_str("body"));
                        fn_r = (v && v->type == NC_STR) ? nc_str(v->s) : nc_str("");
                    } else {
                        fn_r = nc_str("");
                    }
                }
                else if (strstr(cn, "request_header")) {
                    NcVal *ctx = narg > 0 ? cargs[0] : nc_nil();
                    NcVal *key = narg > 1 ? cargs[1] : nc_str("");
                    if (ctx && ctx->type == NC_MAP) {
                        NcVal *headers = nc_index_get(ctx, nc_str("__headers__"));
                        if (!headers || headers->type != NC_MAP) headers = nc_index_get(ctx, nc_str("headers"));
                        if (headers && headers->type == NC_MAP) {
                            NcVal *v = nc_index_get(headers, nc_str(key && key->type == NC_STR ? key->s : ""));
                            if (!v && key && key->type == NC_STR) {
                                char *low = nc_to_str_raw(key);
                                for (char *p = low; p && *p; ++p) if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
                                v = nc_index_get(headers, nc_str(low ? low : ""));
                                free(low);
                            }
                            fn_r = (v && v->type == NC_STR) ? nc_str(v->s) : nc_str("");
                        } else {
                            fn_r = nc_str("");
                        }
                    } else {
                        fn_r = nc_str("");
                    }
                }
                else if (strstr(cn, "request_query")) {
                    NcVal *ctx = narg > 0 ? cargs[0] : nc_nil();
                    NcVal *key = narg > 1 ? cargs[1] : nc_str("");
                    if (ctx && ctx->type == NC_MAP) {
                        NcVal *q = nc_index_get(ctx, nc_str("__query__"));
                        if (!q) q = nc_index_get(ctx, nc_str("query"));
                        if (q && q->type == NC_MAP && key && key->type == NC_STR) {
                            NcVal *v = nc_index_get(q, key);
                            fn_r = (v && v->type == NC_STR) ? nc_str(v->s) : nc_str("");
                        } else {
                            fn_r = nc_str("");
                        }
                    } else {
                        fn_r = nc_str("");
                    }
                }
                else if (strstr(cn, "request_method")) {
                    NcVal *ctx = narg > 0 ? cargs[0] : nc_nil();
                    if (ctx && ctx->type == NC_MAP) {
                        NcVal *v = nc_index_get(ctx, nc_str("__method__"));
                        if (!v) v = nc_index_get(ctx, nc_str("method"));
                        fn_r = (v && v->type == NC_STR) ? nc_str(v->s) : nc_str("");
                    } else {
                        fn_r = nc_str("");
                    }
                }
                else if (strstr(cn, "has_role")) {
                    NcVal *ctx = narg > 0 ? cargs[0] : nc_nil();
                    NcVal *rolle = narg > 1 ? cargs[1] : nc_str("");
                    int ok = 0;
                    if (ctx && ctx->type == NC_MAP && rolle && rolle->type == NC_STR) {
                        NcVal *headers = nc_index_get(ctx, nc_str("__headers__"));
                        if (!headers || headers->type != NC_MAP) headers = nc_index_get(ctx, nc_str("headers"));
                        if (headers && headers->type == NC_MAP) {
                            char *want = nc_to_str_raw(rolle);
                            NcVal *v = nc_index_get(headers, nc_str("x-role"));
                            if (!v) v = nc_index_get(headers, nc_str("x-roles"));
                            if (v && v->type == NC_STR) {
                                char *roles = nc_to_str_raw(v);
                                char *save = NULL;
                                for (char *tok = strtok_r(roles, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
                                    while (*tok == ' ' || *tok == '\t') tok++;
                                    if (!strcmp(tok, want)) { ok = 1; break; }
                                }
                                free(roles);
                            }
                            free(want);
                        }
                    }
                    fn_r = nc_bool(ok);
                }
                else if (strstr(cn, "has_permission")) {
                    NcVal *ctx = narg > 0 ? cargs[0] : nc_nil();
                    NcVal *perm = narg > 1 ? cargs[1] : nc_str("");
                    int ok = 0;
                    if (ctx && ctx->type == NC_MAP && perm && perm->type == NC_STR) {
                        NcVal *headers = nc_index_get(ctx, nc_str("__headers__"));
                        if (!headers || headers->type != NC_MAP) headers = nc_index_get(ctx, nc_str("headers"));
                        if (headers && headers->type == NC_MAP) {
                            char *want = nc_to_str_raw(perm);
                            NcVal *v = nc_index_get(headers, nc_str("x-permissions"));
                            if (v && v->type == NC_STR) {
                                char *perms = nc_to_str_raw(v);
                                char *save = NULL;
                                for (char *tok = strtok_r(perms, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
                                    while (*tok == ' ' || *tok == '\t') tok++;
                                    if (!strcmp(tok, want)) { ok = 1; break; }
                                }
                                free(perms);
                            }
                            free(want);
                        }
                    }
                    fn_r = nc_bool(ok);
                }
                else if (strstr(cn, "response_builder")) {
                    fn_r = nc_map_new();
                    NcVal *status = narg > 0 ? cargs[0] : nc_int(200);
                    NcVal *headers = narg > 1 ? cargs[1] : nc_map_new();
                    NcVal *body = narg > 2 ? cargs[2] : nc_str("");
                    nc_index_set(fn_r, nc_str("__status__"), status);
                    nc_index_set(fn_r, nc_str("__headers__"), headers);
                    nc_index_set(fn_r, nc_str("__body__"), body);
                }
                else if (strstr(cn, "response_error")) {
                    NcVal *status = narg > 0 ? cargs[0] : nc_int(500);
                    NcVal *melding = narg > 1 ? cargs[1] : nc_str("");
                    NcVal *headers = nc_map_new();
                    nc_index_set(headers, nc_str("content-type"), nc_str("application/json"));
                    char *msg = nc_to_str_raw(melding);
                    size_t need = strlen(msg) + 20;
                    char *json = (char *)malloc(need);
                    if (json) snprintf(json, need, "{\"error\":\"%s\"}", msg);
                    fn_r = nc_map_new();
                    nc_index_set(fn_r, nc_str("__status__"), status);
                    nc_index_set(fn_r, nc_str("__headers__"), headers);
                    nc_index_set(fn_r, nc_str("__body__"), json ? nc_str(json) : nc_str(""));
                    free(msg);
                    if (json) free(json);
                }
                else if (strstr(cn, "response_header")) {
                    NcVal *resp = narg > 0 ? cargs[0] : nc_nil();
                    NcVal *key = narg > 1 ? cargs[1] : nc_str("");
                    if (resp && resp->type == NC_MAP) {
                        NcVal *headers = nc_index_get(resp, nc_str("__headers__"));
                        if (!headers || headers->type != NC_MAP) headers = nc_index_get(resp, nc_str("headers"));
                        if (headers && headers->type == NC_MAP) {
                            NcVal *low_v = nc_builtin_lower(key);
                            char *low = low_v ? nc_to_str_raw(low_v) : NULL;
                            NcVal *v = nc_index_get(headers, nc_str(low ? low : ""));
                            if (!v) v = nc_index_get(headers, key);
                            fn_r = (v && v->type == NC_STR) ? nc_str(v->s) : nc_str("");
                            free(low);
                        } else {
                            fn_r = nc_str("");
                        }
                    } else {
                        fn_r = nc_str("");
                    }
                }
                else if (strstr(cn, "response_set_cookie")) {
                    NcVal *resp = narg > 0 ? cargs[0] : nc_nil();
                    NcVal *cookie = narg > 1 ? cargs[1] : nc_str("");
                    if (resp && resp->type == NC_MAP) {
                        NcVal *headers = nc_index_get(resp, nc_str("__headers__"));
                        if (!headers || headers->type != NC_MAP) headers = nc_map_new();
                        nc_index_set(headers, nc_str("set-cookie"), cookie);
                        nc_index_set(resp, nc_str("__headers__"), headers);
                        fn_r = resp;
                    } else {
                        fn_r = nc_nil();
                    }
                }
                else if (strstr(cn, "response_json")) {
                    NcVal *resp = narg > 0 ? cargs[0] : nc_nil();
                    if (resp && resp->type == NC_MAP) {
                        NcVal *v = nc_index_get(resp, nc_str("__body__"));
                        if (!v) v = nc_index_get(resp, nc_str("body"));
                        if (v && v->type == NC_STR) {
                            NcVal *parsed = nc_builtin_json_parse_raw(v);
                            fn_r = (parsed && parsed->type == NC_MAP) ? parsed : nc_map_new();
                        } else {
                            fn_r = nc_map_new();
                        }
                    } else {
                        fn_r = nc_map_new();
                    }
                }
                else if (strstr(cn, "response_file")) {
                    NcVal *path = narg > 0 ? cargs[0] : nc_str("");
                    NcVal *ct = narg > 1 ? cargs[1] : nc_str("application/octet-stream");
                    if (nc_builtin_fil_finnes(path) && path && path->type == NC_STR) {
                        NcVal *body = nc_builtin_fil_les(path);
                        fn_r = nc_map_new();
                        nc_index_set(fn_r, nc_str("__status__"), nc_int(200));
                        NcVal *headers = nc_map_new();
                        nc_index_set(headers, nc_str("content-type"), ct);
                        nc_index_set(fn_r, nc_str("__headers__"), headers);
                        nc_index_set(fn_r, nc_str("__body__"), body);
                    } else {
                        fn_r = nc_map_new();
                        nc_index_set(fn_r, nc_str("__status__"), nc_int(404));
                        NcVal *headers = nc_map_new();
                        nc_index_set(headers, nc_str("content-type"), nc_str("application/json"));
                        nc_index_set(fn_r, nc_str("__headers__"), headers);
                        nc_index_set(fn_r, nc_str("__body__"), nc_str("{\"error\":\"Fil ikkje funne\"}"));
                    }
                }
                else if (strstr(cn, ".web.route") || strstr(cn, ".web.router") || strstr(cn, ".web.subrouter") ||
                         strstr(cn, ".web.guard") || strstr(cn, ".web.dependency") || strstr(cn, ".web.use_dependency") ||
                         strstr(cn, ".web.use_guard") || strstr(cn, ".web.request_middleware") || strstr(cn, ".web.response_middleware") ||
                         strstr(cn, ".web.error_middleware") || strstr(cn, ".web.startup_hook") || strstr(cn, ".web.shutdown_hook") ||
                         strstr(cn, ".web.startup") || strstr(cn, ".web.shutdown")) {
                    fn_r = narg > 0 ? cargs[0] : nc_str("");
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
                control->caught = 0;
                if (_had_try) {
                    jmp_buf _saved_jmp;
                    memcpy(&_saved_jmp, &g_err_jmp, sizeof(jmp_buf));
                    NcGcFrame *_saved_gc_boundary = g_err_gc_boundary;
                    g_err_gc_boundary = &gc_frame;
                    if (setjmp(g_err_jmp)) {
                        memcpy(&g_err_jmp, &_saved_jmp, sizeof(jmp_buf));
                        g_err_gc_boundary = _saved_gc_boundary;
                        free(cargs); free(callee);
                        if (try_depth > 0) {
                            { const char *_em = g_err_msg;
                              if (strncmp(_em,"Norscode unntak: ",17)==0) _em+=17;
                              else if (strncmp(_em,"nc-vm feil: Norscode unntak: ",28)==0) _em+=28;
                              strncpy(last_exception, _em, sizeof(last_exception)-1); }
                            g_err_msg[0] = 0;
                            label_map = nc_load(vars_arr, varnames, nvars, "__gc_labels");
                            const char *cl2 = try_stack[try_depth - 1].catch_lbl;
                            sp = try_stack[try_depth - 1].sp_depth;
                            NcVal *tgt2 = nc_index_get(label_map, nc_str(cl2));
                            if (tgt2 && tgt2->type == NC_INT) { ip = (int)tgt2->i + 1; control->caught = 1; }
                        }
                        fn_r = nc_nil();
                    } else {
                        fn_r = nc_exec_call(functions, callee, cargs, narg, depth+1);
                        memcpy(&g_err_jmp, &_saved_jmp, sizeof(jmp_buf));
                        g_err_gc_boundary = _saved_gc_boundary;
                    }
                } else {
                    fn_r = nc_exec_call(functions, callee, cargs, narg, depth+1);
                }
                if (control->caught) continue;
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
    nc_gc_frame_leave(&gc_frame);
    for (int i = 0; i < nvars; i++) free(varnames[i]);
    free(control); free(stack_arr); free(vars_arr); free(varnames);
    return retval;
}
#undef sp
#undef try_depth
#undef ip

/* g_nc_closure defined via macro above */

/* Køyr lambda med closure-kontekst */
static NcVal *nc_exec_call_closure(NcVal *functions, const char *fn_name, NcVal **args, int nargs, NcVal *closure, int depth) {
    NcVal *saved = g_nc_closure;
    g_nc_closure = closure;
    NcVal *r = nc_exec_call(functions, fn_name, args, nargs, depth);
    g_nc_closure = saved;
    return r;
}

static char *nc_shell_quote(const char *s) {
    size_t len = 2;
    for (const char *p = s; *p; ++p) len += (*p == '\'') ? 4 : 1;
    char *out = malloc(len + 1);
    char *w = out;
    *w++ = '\'';
    for (const char *p = s; *p; ++p) {
        if (*p == '\'') {
            *w++ = '\'';
            *w++ = '\\';
            *w++ = '\'';
            *w++ = '\'';
        } else {
            *w++ = *p;
        }
    }
    *w++ = '\'';
    *w = 0;
    return out;
}

static NcVal *nc_seed_kompiler(const char *src_path, const char *modul) {
    const char *seed = getenv("NORSCODE_SEED_COMPILER_BIN");
    if (!seed || !seed[0]) seed = "build/v3002/bootstrap_compiler_seed_v3002";
    char rooted_seed[4096];
    FILE *probe = fopen(seed, "r");
    if (!probe) {
        const char *root = getenv("NORSCODE_ROOT");
        if (root && root[0] && seed[0] != '/') {
            snprintf(rooted_seed, sizeof(rooted_seed), "%s/%s", root, seed);
            seed = rooted_seed;
            probe = fopen(seed, "r");
        }
    }
    if (!probe) return nc_nil();
    fclose(probe);

    char tmp_tpl[] = "/tmp/nc_seed_compile_XXXXXX";
    int fd = mkstemp(tmp_tpl);
    if (fd < 0) return nc_nil();
    close(fd);

    char *q_seed = nc_shell_quote(seed);
    char *q_src = nc_shell_quote(src_path);
    char *q_mod = nc_shell_quote(modul);
    char *q_out = nc_shell_quote(tmp_tpl);
    size_t need = strlen(q_seed) + strlen(q_src) + strlen(q_mod) + strlen(q_out) + 256;
    char *cmd = malloc(need);
    snprintf(cmd, need,
             "NORSCODE_CMD=compile NORSCODE_FILE=%s NORSCODE_MODULE=%s NORSCODE_OUTPUT=%s %s >/dev/null 2>&1",
             q_src, q_mod, q_out, q_seed);
    int rc = system(cmd);
    free(q_seed); free(q_src); free(q_mod); free(q_out); free(cmd);
    if (rc != 0) {
        remove(tmp_tpl);
        return nc_nil();
    }
    NcVal *ncb_json = nc_builtin_fil_les(nc_str(tmp_tpl));
    remove(tmp_tpl);
    return ncb_json;
}

/* ── Wrap kompiler_fil via bootstrap-host dispatcher ── */
static _Thread_local char g_compile_import_stack[128][512];
static _Thread_local int g_compile_import_depth = 0;
static _Thread_local char g_compile_cache_modules[256][512];
static _Thread_local char *g_compile_cache_json[256];
static _Thread_local int g_compile_cache_count = 0;

static NcVal *nc_native_kompiler(const char *src_path, const char *modul) {
    NcVal *src = nc_builtin_fil_les(nc_str(src_path));
    if (!src || src->type != NC_STR) return nc_nil();
    if (g_compile_import_depth == 0) {
        for (int i = 0; i < g_compile_cache_count; i++) free(g_compile_cache_json[i]);
        g_compile_cache_count = 0;
    } else {
        for (int i = 0; i < g_compile_cache_count; i++) {
            if (!strcmp(g_compile_cache_modules[i], modul)) return nc_str(g_compile_cache_json[i]);
        }
    }
    for (int i = 0; i < g_compile_import_depth; i++) {
        if (!strcmp(g_compile_import_stack[i], modul)) return nc_nil();
    }
    if (g_compile_import_depth >= 128) return nc_nil();
    snprintf(g_compile_import_stack[g_compile_import_depth], sizeof(g_compile_import_stack[0]), "%s", modul);
    g_compile_import_depth++;
    NcVal *gc_roots[256] = {0}; int gc_root_count = 0, gc_stack_size = 0;
    NcGcFrame gc_frame;
    nc_gc_frame_enter(&gc_frame, gc_roots, &gc_stack_size, gc_roots, &gc_root_count);
    gc_frame.pinned = 1;
    gc_roots[gc_root_count++] = src;
    NcVal *result = NULL;
    g_trusted_file_resolution_depth++;
    NcVal *ncb_json = nc_try_stage0_compiler_bundle(src->s, modul);
    if (ncb_json) gc_roots[gc_root_count++] = ncb_json;
    if (!ncb_json || ncb_json->type != NC_STR) {
        ncb_json = nc_seed_kompiler(src_path, modul);
    }
    if (!ncb_json || ncb_json->type != NC_STR) {
        NcVal *src_v = nc_str(src->s);
        NcVal *mod = nc_str(modul);
        gc_roots[gc_root_count++] = src_v;
        gc_roots[gc_root_count++] = mod;
        NcVal *args[] = {src_v, mod};
        ncb_json = nc_dispatch_call("selfhost.kompiler.kompiler_fil", args, 2);
        if (ncb_json) gc_roots[gc_root_count++] = ncb_json;
        if (!ncb_json || ncb_json->type != NC_STR) { result = ncb_json; goto done; }
    }
    if (!ncb_json || ncb_json->type != NC_STR) { result = ncb_json; goto done; }
    NcVal *ncb = nc_builtin_json_parse_raw(ncb_json);
    if (ncb) gc_roots[gc_root_count++] = ncb;
    if (!ncb || ncb->type != NC_MAP) { result = ncb_json; goto done; }
    nc_merge_imports_from_source(ncb, src->s);
    NcVal *imports_v = nc_index_get(ncb, nc_str("imports"));
    if (imports_v && imports_v->type == NC_LIST) {
        for (int i = 0; i < imports_v->list->len; i++) {
            NcVal *imp = imports_v->list->items[i];
            if (!imp || imp->type != NC_STR) continue;
            const char *spec = imp->s;
            const char *eq = strchr(spec, '=');
            NcVal *imp_json = NULL;
            if (!eq) {
                char *path = nc_module_to_path(spec);
                if (!path) continue;
                if (access(path, R_OK) != 0) {
                    const char *slash = strrchr(src_path, '/');
                    if (slash) {
                        size_t dir_len = (size_t)(slash - src_path);
                        size_t path_len = strlen(path);
                        char *relative = malloc(dir_len + path_len + 2);
                        if (relative) {
                            memcpy(relative, src_path, dir_len);
                            relative[dir_len] = '/';
                            memcpy(relative + dir_len + 1, path, path_len + 1);
                            if (access(relative, R_OK) == 0) {
                                free(path);
                                path = relative;
                            } else {
                                free(relative);
                            }
                        }
                    }
                }
                imp_json = nc_native_kompiler(path, spec);
                free(path);
            } else {
                size_t alias_len = (size_t)(eq - spec);
                if (alias_len == 0 || alias_len > 255) continue;
                char alias_buf[256];
                memcpy(alias_buf, spec, alias_len);
                alias_buf[alias_len] = 0;
                const char *path = eq + 1;
                if (!path[0]) continue;
                NcVal *imp_src = nc_builtin_fil_les(nc_str(path));
                if (!imp_src || imp_src->type != NC_STR) continue;
                if (gc_root_count < 250) gc_roots[gc_root_count++] = imp_src;
                NcVal *imp_args[] = {nc_str(imp_src->s), nc_str(alias_buf)};
                if (gc_root_count < 249) { gc_roots[gc_root_count++] = imp_args[0]; gc_roots[gc_root_count++] = imp_args[1]; }
                imp_json = nc_dispatch_call("selfhost.kompiler.kompiler_fil", imp_args, 2);
            }
            if (!imp_json || imp_json->type != NC_STR) continue;
            if (gc_root_count < 250) gc_roots[gc_root_count++] = imp_json;
            NcVal *imp_ncb = nc_builtin_json_parse_raw(imp_json);
            if (!imp_ncb || imp_ncb->type != NC_MAP) continue;
            if (gc_root_count < 250) gc_roots[gc_root_count++] = imp_ncb;
            NcVal *dst_fns = nc_index_get(ncb, nc_str("functions"));
            NcVal *src_fns = nc_index_get(imp_ncb, nc_str("functions"));
            if (dst_fns && src_fns) nc_merge_fns(dst_fns, src_fns);
            NcVal *dst_rh = nc_index_get(ncb, nc_str("route_handlers"));
            NcVal *src_rh = nc_index_get(imp_ncb, nc_str("route_handlers"));
            if (dst_rh && src_rh && dst_rh->type == NC_LIST && src_rh->type == NC_LIST) {
                for (int ri = 0; ri < src_rh->list->len; ri++) {
                    nc_list_append_raw(dst_rh, src_rh->list->items[ri]);
                }
            }
        }
    }
    /* IR-emitteren produserer no ekte JSON-tal. Bevar tekst som "1", "sann"
     * og "null" i NCB-konstanttabellen i staden for å gjette type frå innhald. */
    result = nc_builtin_json_stringify(ncb);
done:
    g_trusted_file_resolution_depth--;
    if (result && result->type == NC_STR && g_compile_cache_count < 256) {
        snprintf(g_compile_cache_modules[g_compile_cache_count], sizeof(g_compile_cache_modules[0]), "%s", modul);
        g_compile_cache_json[g_compile_cache_count] = strdup(result->s);
        g_compile_cache_count++;
    }
    g_compile_import_depth--;
    nc_gc_frame_leave(&gc_frame);
    return result ? result : nc_nil();
}


static void nc_merge_fns(NcVal *dst, NcVal *src) {
    if (!dst || dst->type != NC_MAP || !src || src->type != NC_MAP) return;
    for (int i = 0; i < src->map->len; i++) {
        NcVal *existing = nc_index_get(dst, nc_str(src->map->keys[i]));
        if (!existing || existing->type == NC_NIL)
            nc_index_set(dst, nc_str(src->map->keys[i]), src->map->vals[i]);
    }
}

static int nc_is_builtin_import(const char *modul) {
    return !modul || !modul[0] || !strncmp(modul, "std.", 4) || !strncmp(modul, "builtin.", 8) || !strncmp(modul, "selfhost.", 9);
}

static char *nc_module_to_path(const char *modul) {
    if (!modul) return NULL;
    size_t n = strlen(modul);
    char *path = (char *)malloc(n + 4);
    if (!path) return NULL;
    for (size_t i = 0; i < n; i++) path[i] = (modul[i] == '.') ? '/' : modul[i];
    strcpy(path + n, ".no");
    return path;
}

static void nc_merge_imports_from_source(NcVal *bundle, const char *src_text) {
    if (!bundle || bundle->type != NC_MAP || !src_text) return;
    char *copy = strdup(src_text);
    if (!copy) return;
    char *save = NULL;
    for (char *line = strtok_r(copy, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        while (*line == ' ' || *line == '\t' || *line == '\r') line++;
        if (strncmp(line, "bruk ", 5) != 0) continue;
        char *som = strstr(line, " som ");
        if (!som) continue;
        size_t modul_len = (size_t)(som - (line + 5));
        if (modul_len == 0 || modul_len > 255) continue;
        char modul_buf[256];
        memcpy(modul_buf, line + 5, modul_len);
        modul_buf[modul_len] = 0;
        if (nc_is_builtin_import(modul_buf)) continue;
        char *path = nc_module_to_path(modul_buf);
        if (!path) continue;
        NcVal *imp_ncb_json = nc_native_kompiler(path, modul_buf);
        free(path);
        if (!imp_ncb_json || imp_ncb_json->type != NC_STR) continue;
        NcVal *imp_ncb = nc_builtin_json_parse_raw(imp_ncb_json);
        if (!imp_ncb || imp_ncb->type != NC_MAP) continue;
        NcVal *dst_fns = nc_index_get(bundle, nc_str("functions"));
        NcVal *src_fns = nc_index_get(imp_ncb, nc_str("functions"));
            if (dst_fns && src_fns) nc_merge_fns(dst_fns, src_fns);
            NcVal *dst_rh = nc_index_get(bundle, nc_str("route_handlers"));
            NcVal *src_rh = nc_index_get(imp_ncb, nc_str("route_handlers"));
            if (dst_rh && src_rh && dst_rh->type == NC_LIST && src_rh->type == NC_LIST) {
                for (int i = 0; i < src_rh->list->len; i++) {
                    nc_list_append_raw(dst_rh, src_rh->list->items[i]);
                }
            }
        }
    free(copy);
}

static void nc_list_append_raw(NcVal *lst, NcVal *v) {
    if (!lst || lst->type != NC_LIST || !v) return;
    if (lst->list->len >= lst->list->cap) {
        lst->list->cap = lst->list->cap ? lst->list->cap * 2 : 8;
        lst->list->items = realloc(lst->list->items, (size_t)lst->list->cap * sizeof(NcVal *));
    }
    lst->list->items[lst->list->len++] = v;
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
    NcVal *ncb = nc_builtin_json_parse_raw(args[0]);
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
NcVal *nc_builtin_vm_function_info(NcVal **args, int na) {
    NcVal *info = nc_map_new();
    const char *requested = (na > 0 && args[0] && args[0]->type == NC_STR) ? args[0]->s : "";
    NcVal *definition = (requested[0] && g_current_functions)
        ? nc_exec_find_fn(g_current_functions, requested) : NULL;
    nc_index_set(info, nc_str("finnes"), nc_str(definition ? "sann" : "usann"));
    nc_index_set(info, nc_str("namn"), nc_str(definition ? requested : ""));
    if (!definition || definition->type != NC_MAP) return info;

    NcVal *module = nc_map_get_cstr(definition, "module");
    NcVal *params = nc_map_get_cstr(definition, "params");
    NcVal *is_async = nc_map_get_cstr(definition, "is_async");
    NcVal *code = nc_map_get_cstr(definition, "code");
    nc_index_set(info, nc_str("module"), module ? module : nc_str(""));
    nc_index_set(info, nc_str("params"), params ? nc_builtin_json_stringify_smart(params) : nc_str("[]"));
    nc_index_set(info, nc_str("param_count"), nc_int(params && params->type == NC_LIST ? params->list->len : 0));
    nc_index_set(info, nc_str("is_async"), is_async ? is_async : nc_bool(0));
    nc_index_set(info, nc_str("instruction_count"), nc_int(code && code->type == NC_LIST ? code->list->len : 0));
    nc_index_set(info, nc_str("source_line"), nc_int(0));
    nc_index_set(info, nc_str("source_column"), nc_int(0));
    return info;
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
    NcVal *ncb = nc_builtin_json_parse_raw(args[0]);
    if (!ncb || ncb->type != NC_MAP) return nc_int(1);
    NcVal *fns_v = nc_index_get(ncb, nc_str("functions"));
    NcVal *entry_v = nc_index_get(ncb, nc_str("entry"));
    if (!fns_v || fns_v->type != NC_MAP) return nc_int(1);
    char *entry = nc_to_str_raw(entry_v);
    if (!entry || !entry[0]) { free(entry); return nc_int(1); }
    if (!nc_exec_find_fn(fns_v, entry)) {
        free(entry);
        entry = NULL;
        const char *fallbacks[] = {"__main__.start", "__main__.main", "__main__.test", NULL};
        for (int i = 0; fallbacks[i]; i++) {
            NcVal *candidate = nc_map_get_cstr(fns_v, fallbacks[i]);
            if (candidate && candidate->type != NC_NIL) { entry = strdup(fallbacks[i]); break; }
        }
        if (!entry && fns_v->map->len > 0) entry = strdup(fns_v->map->keys[0]);
        if (!entry) return nc_int(1);
    }
    NcVal *host_roots[4] = {ncb, fns_v, NULL, NULL};
    int host_sp = 0, host_root_count = 2;
    NcGcFrame host_frame;
    nc_gc_frame_enter(&host_frame, host_roots, &host_sp, host_roots, &host_root_count);
    NcVal *saved_functions = g_current_functions;
    NcVal *saved_ncb = g_current_ncb;
    NcVal *saved_handlers = g_current_route_handlers;
    int saved_request_counter = g_request_counter;
    int saved_security_active = g_native_app_security_active;
    /* Lagre route_handlers, NCB og functions for std.web */
    NcVal *rh = nc_index_get(ncb, nc_str("route_handlers"));
    g_current_route_handlers = (rh && rh->type != NC_NIL) ? rh : nc_list_new();
    host_roots[host_root_count++] = g_current_route_handlers;
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
    g_native_app_security_active = 1;
    NcVal *r = nc_exec_call(fns_v, entry, NULL, 0, 0);
    int exit_code = nc_val_til_exit(r);
    g_current_functions = saved_functions;
    g_current_ncb = saved_ncb;
    g_current_route_handlers = saved_handlers;
    g_request_counter = saved_request_counter;
    g_native_app_security_active = saved_security_active;
    for (int i = 0; i < nsaved; i++) {
        if (saved[i]) {
            setenv(env_keys[i], saved[i], 1);
            free(saved[i]);
        } else {
            unsetenv(env_keys[i]);
        }
    }
    free(entry);
    nc_gc_frame_leave(&host_frame);
    return nc_int(exit_code);
}

static int nc_val_til_exit(NcVal *v) {
    if (v && v->type == NC_INT) return (int)v->i;
    return 0;
}

static NcVal *nc_builtin_koyr_funksjon_host(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_nil();
    const char *fn_name = args[0]->s;
    NcVal *arg_list = (na >= 2) ? args[1] : nc_nil();
    NcVal *functions = (na >= 3 && args[2] && args[2]->type == NC_MAP) ? args[2] : g_current_functions;
    int depth = (na >= 4 && args[3] && args[3]->type == NC_INT) ? (int)args[3]->i : 0;
    if (!functions || functions->type != NC_MAP) return nc_nil();

    int nargs = 0;
    NcVal **call_args = NULL;
    if (arg_list && arg_list->type == NC_LIST && arg_list->list) {
        nargs = arg_list->list->len;
        if (nargs > 0) {
            call_args = calloc((size_t)nargs, sizeof(NcVal *));
            for (int i = 0; i < nargs; i++) call_args[i] = arg_list->list->items[i];
        }
    }

    NcVal *r = nc_exec_call(functions, fn_name, call_args, nargs, depth);
    free(call_args);
    return r ? r : nc_nil();
}

static NcVal *nc_builtin_vm_sett_kontekst_host(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_MAP) return nc_int(0);
    g_current_ncb = args[0];
    NcVal *fns = nc_index_get(args[0], nc_str("functions"));
    if (fns && fns->type == NC_MAP) g_current_functions = fns;
    NcVal *rhs = nc_index_get(args[0], nc_str("route_handlers"));
    if (rhs && rhs->type == NC_LIST) g_current_route_handlers = rhs;
    g_request_counter = 1;
    return nc_int(0);
}

static NcVal *nc_builtin_koyr_med_kontekst_host(NcVal **args, int na) {
    if (na < 1 || !args[0] || args[0]->type != NC_STR) return nc_nil();
    const char *fn_name = args[0]->s;
    NcVal *arg_list = (na >= 2) ? args[1] : nc_nil();
    int nargs = 0;
    NcVal **call_args = NULL;
    if (arg_list && arg_list->type == NC_LIST && arg_list->list) {
        nargs = arg_list->list->len;
        if (nargs > 0) {
            call_args = calloc((size_t)nargs, sizeof(NcVal *));
            for (int i = 0; i < nargs; i++) call_args[i] = arg_list->list->items[i];
        }
    }
    NcVal *r = nc_exec_call(g_current_functions, fn_name, call_args, nargs, 0);
    free(call_args);
    return r ? r : nc_nil();
}

/* Køyr selfhost.nc_main.start; returnerer exit-kode eller -1 ved feil */
static int nc_try_nc_main_host(void) {
    const char *cmd = getenv("NORSCODE_CMD");
    if (cmd && !strcmp(cmd, "compile")) {
        const char *src = getenv("NORSCODE_FILE");
        const char *mod = getenv("NORSCODE_MODULE");
        const char *out = getenv("NORSCODE_OUTPUT");
        if (!mod || !mod[0]) mod = "__main__";
        if (!src || !src[0]) return -1;
        NcVal *ncb_json = nc_native_kompiler(src, mod);
        if (!ncb_json || ncb_json->type != NC_STR) return -1;
        char out_buf[4096];
        if (!out || !out[0]) {
            snprintf(out_buf, sizeof(out_buf), "%s", src);
            size_t len = strlen(out_buf);
            if (len >= 3 && !strcmp(out_buf + len - 3, ".no")) {
                snprintf(out_buf + len - 3, sizeof(out_buf) - (len - 3), ".ncb.json");
            } else {
                strncat(out_buf, ".ncb.json", sizeof(out_buf) - strlen(out_buf) - 1);
            }
            out = out_buf;
        }
        nc_builtin_fil_skriv(nc_str(out), ncb_json);
        return 0;
    }
    if (cmd && !strcmp(cmd, "run")) {
        const char *src = getenv("NORSCODE_FILE");
        const char *mod = getenv("NORSCODE_MODULE");
        if (!mod || !mod[0]) mod = "__main__";
        if (!src || !src[0]) return -1;
        NcVal *ncb_json = nc_native_kompiler(src, mod);
        if (!ncb_json || ncb_json->type != NC_STR) return -1;
        NcVal *args[] = { ncb_json };
        NcVal *r = nc_fn_builtin_host_exec_ncb_json(args, 1);
        return nc_val_til_exit(r);
    }
    if(cmd&&!strcmp(cmd,"run-ncb")){
        const char *path=getenv("NORSCODE_FILE");if(!path||!*path)return -1;NcVal *ncb_json=nc_builtin_fil_les(nc_str(path));if(!ncb_json||ncb_json->type!=NC_STR)return -1;NcVal *args[]={ncb_json};NcVal *result=nc_fn_builtin_host_exec_ncb_json(args,1);return nc_val_til_exit(result);
    }
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
