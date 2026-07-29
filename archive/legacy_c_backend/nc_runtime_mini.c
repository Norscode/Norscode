#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif

/*
 * nc_runtime_mini.c — minimal Norscode C-runtime for maintainer-lanen.
 *
 * Brukt av generert C-kode frå `archive/legacy_c_backend/ncb_to_c.no`.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#if defined(_WIN32)
#include "nc_windows_compat.h"
#else
#include <pthread.h>
#endif
#include <stdarg.h>
#include <ctype.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#if defined(__linux__)
#include <sys/random.h>
#endif

/* ── Stage0-kjerne: verdi-layout og grunnstrukturar ── */
#define NC_NIL   0
#define NC_INT   1
#define NC_STR   2
#define NC_BOOL  3
#define NC_LIST  4
#define NC_MAP   5
#define NC_FLOAT 6
#define NC_BYTES 7
#define NC_GC_MAGIC 0x0147434eU
#define NC_GC_ACTIVE 1U
#define NC_GC_MARKED 2U

typedef struct NcVal NcVal;
typedef struct NcValArenaPage NcValArenaPage;
typedef struct NcGcEdge NcGcEdge;
typedef struct { NcVal **items; int len, cap; } NcList;
typedef struct { unsigned char *data; size_t len, cap; } NcBytes;
typedef struct {
    char **keys;
    NcVal **vals;
    uint64_t *hashes;
    int *buckets;
    int len, cap, bucket_cap;
    pthread_mutex_t mutex;
} NcMap;

struct NcVal {
    /* Eksakt norscode-memory-abi-v1 / NCG1-header (32 byte). */
    uint32_t gc_magic;
    uint8_t gc_flags;
    uint8_t gc_generation;
    uint8_t gc_age;
    uint8_t type;
    uint64_t gc_payload_size;
    uintptr_t gc_first_edge;
    uint64_t gc_ref_count;
    struct NcVal *gc_next;
    NcValArenaPage *gc_arena_page;
    union {
        long long i;
        double    f;
        char     *s;
        int       b;
        NcList   *list;
        NcMap    *map;
        NcBytes  *bytes;
    };
    size_t slen; /* cachet tekstlengd for NC_STR */
};

_Static_assert(offsetof(NcVal, gc_next) == 32, "NcVal must start with NCG1 header");

#define NC_VALS_PER_ARENA_PAGE 1024
struct NcValArenaPage {
    NcValArenaPage *next;
    NcVal values[NC_VALS_PER_ARENA_PAGE];
};

struct NcGcEdge {
    uint32_t magic;
    uint8_t flags;
    uint8_t generation;
    uint8_t age;
    uint8_t type;
    uint64_t payload_size;
    uintptr_t first_edge;
    uint64_t ref_count;
    NcVal *from;
    NcVal *to;
    NcGcEdge *next;
    NcGcEdge *registry_next;
};

_Static_assert(offsetof(NcGcEdge, from) == 32, "edge must start with NCG1 header");

typedef struct NcGcFrame {
    NcVal **stack;
    int *sp;
    NcVal **vars;
    int *nvars;
    _Atomic int *blocked;
    int pinned;
    pthread_t owner;
    struct NcGcFrame *next;
} NcGcFrame;

static pthread_mutex_t g_gc_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_gc_condition = PTHREAD_COND_INITIALIZER;
static NcVal *g_gc_values = NULL;
static NcVal *g_gc_free_values = NULL;
static NcValArenaPage *g_gc_arena_pages = NULL;
static size_t g_gc_arena_page_count = 0;
static size_t g_gc_arena_reused = 0;
static size_t g_gc_relocated = 0;
static _Atomic unsigned long long g_gc_relocation_epoch = 0;
static NcGcEdge *g_gc_edges = NULL;
static size_t g_gc_edge_count = 0;
static int g_gc_mark_use_edges = 0;
static NcGcFrame *g_gc_frames = NULL;
static size_t g_gc_allocated = 0;
static size_t g_gc_threshold = 4096;
static size_t g_gc_collections = 0;
static _Thread_local int g_trusted_file_resolution_depth = 0;
static size_t g_gc_minor_collections = 0;
static size_t g_gc_major_collections = 0;
static size_t g_gc_minor_since_major = 0;
static int g_gc_stop_requested = 0;
static pthread_t g_gc_collector;
static size_t g_gc_parked_threads = 0;
static _Thread_local int g_gc_thread_parked = 0;
static _Thread_local _Atomic int g_gc_thread_blocked = 0;

static uint64_t nc_map_hash(const char *key) {
    uint64_t hash = UINT64_C(1469598103934665603);
    for (const unsigned char *p = (const unsigned char *)key; *p; p++) {
        hash ^= *p;
        hash *= UINT64_C(1099511628211);
    }
    return hash ? hash : 1;
}

static void nc_map_rebuild_index_locked(NcMap *map, int minimum_entries) {
    int bucket_cap = 8;
    while (bucket_cap < minimum_entries * 2) bucket_cap *= 2;
    if (bucket_cap != map->bucket_cap) {
        free(map->buckets);
        map->buckets = calloc((size_t)bucket_cap, sizeof(int));
        if (!map->buckets) abort();
        map->bucket_cap = bucket_cap;
    } else {
        memset(map->buckets, 0, (size_t)bucket_cap * sizeof(int));
    }
    for (int i = 0; i < map->len; i++) {
        size_t slot = (size_t)map->hashes[i] & (size_t)(bucket_cap - 1);
        while (map->buckets[slot]) slot = (slot + 1) & (size_t)(bucket_cap - 1);
        map->buckets[slot] = i + 1;
    }
}

static void nc_map_index_insert_locked(NcMap *map, int index) {
    if (!map->buckets || map->bucket_cap < map->len * 2) {
        nc_map_rebuild_index_locked(map, map->len);
        return;
    }
    size_t slot = (size_t)map->hashes[index] & (size_t)(map->bucket_cap - 1);
    while (map->buckets[slot]) slot = (slot + 1) & (size_t)(map->bucket_cap - 1);
    map->buckets[slot] = index + 1;
}

static int nc_map_find_locked(NcMap *map, const char *key, uint64_t hash) {
    if (!map->buckets || map->bucket_cap == 0) return -1;
    size_t slot = (size_t)hash & (size_t)(map->bucket_cap - 1);
    for (int probes = 0; probes < map->bucket_cap; probes++) {
        int encoded = map->buckets[slot];
        if (!encoded) return -1;
        int index = encoded - 1;
        if (map->hashes[index] == hash && !strcmp(map->keys[index], key)) return index;
        slot = (slot + 1) & (size_t)(map->bucket_cap - 1);
    }
    return -1;
}

static NcVal *nc_map_get_cstr(NcVal *obj, const char *key) {
    if (!obj || obj->type != NC_MAP || !obj->map) return NULL;
    uint64_t hash = nc_map_hash(key);
    pthread_mutex_lock(&obj->map->mutex);
    int index = nc_map_find_locked(obj->map, key, hash);
    NcVal *result = index >= 0 ? obj->map->vals[index] : NULL;
    pthread_mutex_unlock(&obj->map->mutex);
    return result;
}

static void nc_val_arena_grow_locked(void) {
    NcValArenaPage *page = calloc(1, sizeof(NcValArenaPage));
    if (!page) abort();
    page->next = g_gc_arena_pages;
    g_gc_arena_pages = page;
    g_gc_arena_page_count++;
    for (int i = 0; i < NC_VALS_PER_ARENA_PAGE; i++) {
        page->values[i].gc_arena_page = page;
        page->values[i].gc_next = g_gc_free_values;
        g_gc_free_values = &page->values[i];
    }
}

static NcVal *nc_val_alloc(int type) {
    pthread_mutex_lock(&g_gc_mutex);
    if (!g_gc_free_values) nc_val_arena_grow_locked();
    NcVal *v = g_gc_free_values;
    g_gc_free_values = v->gc_next;
    NcValArenaPage *page = v->gc_arena_page;
    memset(v, 0, sizeof(NcVal));
    v->gc_arena_page = page;
    v->gc_magic = NC_GC_MAGIC;
    v->gc_flags = NC_GC_ACTIVE;
    v->type = type;
    v->gc_next = g_gc_values;
    g_gc_values = v;
    g_gc_allocated++;
    g_gc_arena_reused++;
    pthread_mutex_unlock(&g_gc_mutex);
    return v;
}

static void nc_gc_frame_enter(NcGcFrame *frame, NcVal **stack, int *sp,
                              NcVal **vars, int *nvars) {
    frame->stack = stack; frame->sp = sp;
    frame->vars = vars; frame->nvars = nvars;
    frame->blocked = &g_gc_thread_blocked;
    frame->pinned = 0;
    frame->owner = pthread_self();
    pthread_mutex_lock(&g_gc_mutex);
    while (g_gc_stop_requested && !pthread_equal(g_gc_collector, frame->owner))
        pthread_cond_wait(&g_gc_condition, &g_gc_mutex);
    frame->next = g_gc_frames;
    g_gc_frames = frame;
    pthread_mutex_unlock(&g_gc_mutex);
}

static void nc_gc_blocking_enter(void) {
    atomic_store_explicit(&g_gc_thread_blocked, 1, memory_order_release);
}

static void nc_gc_blocking_leave(void) {
    atomic_store_explicit(&g_gc_thread_blocked, 0, memory_order_release);
}

static void nc_gc_frame_leave(NcGcFrame *frame) {
    pthread_mutex_lock(&g_gc_mutex);
    NcGcFrame **cursor = &g_gc_frames;
    while (*cursor && *cursor != frame) cursor = &(*cursor)->next;
    if (*cursor == frame) *cursor = frame->next;
    pthread_cond_broadcast(&g_gc_condition);
    pthread_mutex_unlock(&g_gc_mutex);
}

static void nc_gc_unwind_thread_to(NcGcFrame *boundary) {
    pthread_t owner = pthread_self();
    pthread_mutex_lock(&g_gc_mutex);
    NcGcFrame **cursor = &g_gc_frames;
    while (*cursor && *cursor != boundary) {
        NcGcFrame *frame = *cursor;
        if (pthread_equal(frame->owner, owner)) {
            *cursor = frame->next;
        } else {
            cursor = &frame->next;
        }
    }
    pthread_cond_broadcast(&g_gc_condition);
    pthread_mutex_unlock(&g_gc_mutex);
}

static void nc_gc_mark_value(NcVal *v) {
    if (!v || (v->gc_flags & NC_GC_MARKED)) return;
    v->gc_flags |= NC_GC_MARKED;
    if (g_gc_mark_use_edges) {
        for (NcGcEdge *edge = (NcGcEdge *)v->gc_first_edge; edge; edge = edge->next)
            nc_gc_mark_value(edge->to);
    } else if (v->type == NC_LIST && v->list) {
        for (int i = 0; i < v->list->len; i++) nc_gc_mark_value(v->list->items[i]);
    } else if (v->type == NC_MAP && v->map) {
        pthread_mutex_lock(&v->map->mutex);
        for (int i = 0; i < v->map->len; i++) nc_gc_mark_value(v->map->vals[i]);
        pthread_mutex_unlock(&v->map->mutex);
    }
}

#if !defined(NC_RUNTIME_STANDALONE)
void nc_gc_host_mark_roots(void);
#endif

static void nc_gc_clear_edges_locked(void) {
    while (g_gc_edges) {
        NcGcEdge *next = g_gc_edges->registry_next;
        free(g_gc_edges);
        g_gc_edges = next;
    }
    g_gc_edge_count = 0;
    for (NcVal *v = g_gc_values; v; v = v->gc_next) {
        v->gc_first_edge = 0;
        v->gc_ref_count = 0;
    }
}

static void nc_gc_add_edge_locked(NcVal *from, NcVal *to) {
    if (!from || !to) return;
    NcGcEdge *edge = calloc(1, sizeof(NcGcEdge));
    if (!edge) abort();
    edge->magic = NC_GC_MAGIC;
    edge->flags = NC_GC_ACTIVE;
    edge->type = 250;
    edge->payload_size = 24;
    edge->from = from;
    edge->to = to;
    edge->next = (NcGcEdge *)from->gc_first_edge;
    edge->registry_next = g_gc_edges;
    from->gc_first_edge = (uintptr_t)edge;
    from->gc_ref_count++;
    g_gc_edges = edge;
    g_gc_edge_count++;
}

static void nc_gc_rebuild_edges_locked(void) {
    nc_gc_clear_edges_locked();
    for (NcVal *v = g_gc_values; v; v = v->gc_next) {
        if (v->type == NC_LIST && v->list) {
            for (int i = 0; i < v->list->len; i++) nc_gc_add_edge_locked(v, v->list->items[i]);
        } else if (v->type == NC_MAP && v->map) {
            pthread_mutex_lock(&v->map->mutex);
            for (int i = 0; i < v->map->len; i++) nc_gc_add_edge_locked(v, v->map->vals[i]);
            pthread_mutex_unlock(&v->map->mutex);
        }
    }
}

static void nc_gc_prune_edges_locked(void) {
    NcGcEdge **cursor = &g_gc_edges;
    while (*cursor) {
        NcGcEdge *edge = *cursor;
        if (!(edge->from->gc_flags & NC_GC_ACTIVE)) {
            *cursor = edge->registry_next;
            free(edge);
            g_gc_edge_count--;
        } else cursor = &edge->registry_next;
    }
}

static void nc_gc_destroy_value(NcVal *v) {
    if (v->type == NC_STR) free(v->s);
    else if (v->type == NC_LIST && v->list) {
        free(v->list->items); free(v->list);
    } else if (v->type == NC_MAP && v->map) {
        for (int i = 0; i < v->map->len; i++) free(v->map->keys[i]);
        free(v->map->keys); free(v->map->vals); free(v->map->hashes); free(v->map->buckets);
        pthread_mutex_destroy(&v->map->mutex); free(v->map);
    } else if (v->type == NC_BYTES && v->bytes) {
        free(v->bytes->data); free(v->bytes);
    }
    v->gc_flags = 0;
    v->gc_next = g_gc_free_values;
    g_gc_free_values = v;
}

typedef struct {
    NcVal *old_value;
    NcVal *new_value;
} NcGcRelocation;

#if !defined(NC_RUNTIME_STANDALONE)
void nc_gc_host_relocate(NcGcRelocation *table, size_t capacity);
#endif

static size_t nc_gc_pointer_hash(NcVal *value, size_t mask) {
    uintptr_t bits = (uintptr_t)value;
    bits ^= bits >> 17;
    bits *= UINT64_C(0xed5ad4bb);
    bits ^= bits >> 11;
    return (size_t)bits & mask;
}

static void nc_gc_relocation_put(NcGcRelocation *table, size_t capacity,
                                 NcVal *old_value, NcVal *new_value) {
    size_t slot = nc_gc_pointer_hash(old_value, capacity - 1);
    while (table[slot].old_value) slot = (slot + 1) & (capacity - 1);
    table[slot].old_value = old_value;
    table[slot].new_value = new_value;
}

static NcVal *nc_gc_relocation_get(NcGcRelocation *table, size_t capacity,
                                   NcVal *old_value) {
    if (!old_value) return NULL;
    size_t slot = nc_gc_pointer_hash(old_value, capacity - 1);
    for (size_t probes = 0; probes < capacity; probes++) {
        if (!table[slot].old_value) return old_value;
        if (table[slot].old_value == old_value) return table[slot].new_value;
        slot = (slot + 1) & (capacity - 1);
    }
    return old_value;
}

static void nc_val_arena_compact_locked(void) {
    size_t live = g_gc_allocated;
    if (!live) {
        while (g_gc_arena_pages) {
            NcValArenaPage *next = g_gc_arena_pages->next;
            free(g_gc_arena_pages);
            g_gc_arena_pages = next;
        }
        g_gc_values = NULL;
        g_gc_free_values = NULL;
        g_gc_arena_page_count = 0;
        return;
    }

    size_t page_count = (live + NC_VALS_PER_ARENA_PAGE - 1) / NC_VALS_PER_ARENA_PAGE;
    g_gc_free_values = NULL;
    NcValArenaPage **page_cursor = &g_gc_arena_pages;
    size_t nonempty_pages = 0;
    while (*page_cursor) {
        NcValArenaPage *page = *page_cursor;
        size_t active = 0;
        for (size_t i = 0; i < NC_VALS_PER_ARENA_PAGE; i++)
            if (page->values[i].gc_flags & NC_GC_ACTIVE) active++;
        if (!active) {
            *page_cursor = page->next;
            free(page);
            continue;
        }
        nonempty_pages++;
        for (size_t i = 0; i < NC_VALS_PER_ARENA_PAGE; i++) {
            NcVal *value = &page->values[i];
            if (!(value->gc_flags & NC_GC_ACTIVE)) {
                value->gc_next = g_gc_free_values;
                g_gc_free_values = value;
            }
        }
        page_cursor = &page->next;
    }
    g_gc_arena_page_count = nonempty_pages;
    for (NcGcFrame *frame = g_gc_frames; frame; frame = frame->next)
        if (frame->pinned) return;
    size_t worthwhile_gap = page_count / 4;
    if (worthwhile_gap < 1) worthwhile_gap = 1;
    const char *force_compact = getenv("NORSCODE_GC_FORCE_COMPACT");
    if ((!force_compact || strcmp(force_compact, "1")) &&
        nonempty_pages <= page_count + worthwhile_gap) return;

    NcValArenaPage **pages = calloc(page_count, sizeof(NcValArenaPage *));
    size_t relocation_capacity = 8;
    while (relocation_capacity < live * 2) relocation_capacity *= 2;
    NcGcRelocation *relocations = calloc(relocation_capacity, sizeof(NcGcRelocation));
    if (!pages || !relocations) abort();

    NcValArenaPage *new_pages = NULL;
    for (size_t i = 0; i < page_count; i++) {
        pages[i] = calloc(1, sizeof(NcValArenaPage));
        if (!pages[i]) abort();
        pages[i]->next = new_pages;
        new_pages = pages[i];
    }

    NcVal *new_values = NULL;
    NcVal **new_tail = &new_values;
    size_t index = 0;
    for (NcVal *old = g_gc_values; old; old = old->gc_next) {
        NcValArenaPage *page = pages[index / NC_VALS_PER_ARENA_PAGE];
        NcVal *moved = &page->values[index % NC_VALS_PER_ARENA_PAGE];
        memcpy(moved, old, sizeof(NcVal));
        moved->gc_arena_page = page;
        moved->gc_next = NULL;
        nc_gc_relocation_put(relocations, relocation_capacity, old, moved);
        *new_tail = moved;
        new_tail = &moved->gc_next;
        index++;
    }

    for (NcVal *value = new_values; value; value = value->gc_next) {
        if (value->type == NC_LIST && value->list) {
            for (int i = 0; i < value->list->len; i++)
                value->list->items[i] = nc_gc_relocation_get(
                    relocations, relocation_capacity, value->list->items[i]);
        } else if (value->type == NC_MAP && value->map) {
            pthread_mutex_lock(&value->map->mutex);
            for (int i = 0; i < value->map->len; i++)
                value->map->vals[i] = nc_gc_relocation_get(
                    relocations, relocation_capacity, value->map->vals[i]);
            pthread_mutex_unlock(&value->map->mutex);
        }
    }
    for (NcGcFrame *frame = g_gc_frames; frame; frame = frame->next) {
        int sp = frame->sp ? *frame->sp : 0;
        int nvars = frame->nvars ? *frame->nvars : 0;
        for (int i = 0; i < sp; i++)
            frame->stack[i] = nc_gc_relocation_get(relocations, relocation_capacity, frame->stack[i]);
        for (int i = 0; i < nvars; i++)
            frame->vars[i] = nc_gc_relocation_get(relocations, relocation_capacity, frame->vars[i]);
    }
    for (NcGcEdge *edge = g_gc_edges; edge; edge = edge->registry_next) {
        edge->from = nc_gc_relocation_get(relocations, relocation_capacity, edge->from);
        edge->to = nc_gc_relocation_get(relocations, relocation_capacity, edge->to);
    }
#if !defined(NC_RUNTIME_STANDALONE)
    nc_gc_host_relocate(relocations, relocation_capacity);
#endif

    NcValArenaPage *old_pages = g_gc_arena_pages;
    g_gc_arena_pages = new_pages;
    g_gc_arena_page_count = page_count;
    g_gc_values = new_values;
    g_gc_free_values = NULL;
    for (size_t p = 0; p < page_count; p++) {
        size_t first_free = p * NC_VALS_PER_ARENA_PAGE;
        for (size_t i = 0; i < NC_VALS_PER_ARENA_PAGE; i++) {
            if (first_free + i >= live) {
                NcVal *free_value = &pages[p]->values[i];
                free_value->gc_arena_page = pages[p];
                free_value->gc_next = g_gc_free_values;
                g_gc_free_values = free_value;
            }
        }
    }
    while (old_pages) {
        NcValArenaPage *next = old_pages->next;
        free(old_pages);
        old_pages = next;
    }
    g_gc_relocated += live;
    atomic_fetch_add_explicit(&g_gc_relocation_epoch, 1, memory_order_release);
    free(relocations);
    free(pages);
}

static void nc_gc_collect_locked(int major) {
    g_gc_mark_use_edges = major;
    if (major) nc_gc_rebuild_edges_locked();
    if (!major) {
        /* Gamle objekt er konservative røter i minor-GC. */
        for (NcVal *v = g_gc_values; v; v = v->gc_next)
            if (v->gc_generation > 0) nc_gc_mark_value(v);
    }
#if !defined(NC_RUNTIME_STANDALONE)
    nc_gc_host_mark_roots();
#endif
    for (NcGcFrame *f = g_gc_frames; f; f = f->next) {
        int sp = f->sp ? *f->sp : 0;
        int nv = f->nvars ? *f->nvars : 0;
        for (int i = 0; i < sp; i++) nc_gc_mark_value(f->stack[i]);
        for (int i = 0; i < nv; i++) nc_gc_mark_value(f->vars[i]);
    }
    size_t survivors = 0;
    NcVal **cursor = &g_gc_values;
    while (*cursor) {
        NcVal *v = *cursor;
        if (!(v->gc_flags & NC_GC_MARKED)) {
            *cursor = v->gc_next;
            nc_gc_destroy_value(v);
        } else {
            v->gc_flags &= (uint8_t)~NC_GC_MARKED;
            if (v->gc_age < UINT8_MAX) v->gc_age++;
            if (v->gc_age >= 2) v->gc_generation = 1;
            if (v->type == NC_LIST && v->list) {
                v->gc_payload_size = sizeof(NcList) + (uint64_t)v->list->cap * sizeof(NcVal *);
                v->gc_ref_count = (uint64_t)v->list->len;
            } else if (v->type == NC_MAP && v->map) {
                v->gc_payload_size = sizeof(NcMap) + (uint64_t)v->map->cap *
                    (sizeof(char *) + sizeof(NcVal *) + sizeof(uint64_t)) +
                    (uint64_t)v->map->bucket_cap * sizeof(int);
                v->gc_ref_count = (uint64_t)v->map->len;
            }
            survivors++;
            cursor = &v->gc_next;
        }
    }
    g_gc_allocated = survivors;
    nc_gc_prune_edges_locked();
    if (major) nc_val_arena_compact_locked();
    g_gc_threshold = survivors < 1024 ? 4096 : survivors * 4;
    g_gc_collections++;
    if (major) { g_gc_major_collections++; g_gc_minor_since_major = 0; }
    else { g_gc_minor_collections++; g_gc_minor_since_major++; }
    if (getenv("NORSCODE_GC_TRACE"))
        fprintf(stderr, "[nc-gc] collection=%zu kind=%s live=%zu next=%zu\n",
                g_gc_collections, major ? "major" : "minor", survivors, g_gc_threshold);
    g_gc_mark_use_edges = 0;
}

static void nc_gc_collect(void) {
    pthread_mutex_lock(&g_gc_mutex);
    nc_gc_collect_locked(1);
    pthread_mutex_unlock(&g_gc_mutex);
}

static size_t nc_gc_other_active_threads(pthread_t collector) {
    pthread_t seen[128]; size_t seen_count = 0;
    for (NcGcFrame *f = g_gc_frames; f; f = f->next) {
        if (pthread_equal(f->owner, collector)) continue;
        if (f->blocked && atomic_load_explicit(f->blocked, memory_order_acquire)) continue;
        int duplicate = 0;
        for (size_t i = 0; i < seen_count; i++)
            if (pthread_equal(seen[i], f->owner)) { duplicate = 1; break; }
        if (!duplicate && seen_count < 128) seen[seen_count++] = f->owner;
    }
    return seen_count;
}

static void nc_gc_release_world_locked(void) {
    g_gc_stop_requested = 0;
    g_gc_parked_threads = 0;
    pthread_cond_broadcast(&g_gc_condition);
}

static void nc_gc_safepoint(void) {
#if defined(NC_DISABLE_GC)
    return;
#endif
    static int configured = 0;
    if (!configured) {
        const char *threshold = getenv("NORSCODE_GC_THRESHOLD");
        if (threshold && threshold[0]) {
            unsigned long long parsed = strtoull(threshold, NULL, 10);
            if (parsed >= 64) g_gc_threshold = (size_t)parsed;
        }
        configured = 1;
    }
    pthread_mutex_lock(&g_gc_mutex);
    pthread_t current = pthread_self();
    if (g_gc_stop_requested && !pthread_equal(g_gc_collector, current)) {
        if (!g_gc_thread_parked) {
            g_gc_thread_parked = 1;
            g_gc_parked_threads++;
            pthread_cond_broadcast(&g_gc_condition);
        }
        while (g_gc_stop_requested)
            pthread_cond_wait(&g_gc_condition, &g_gc_mutex);
        g_gc_thread_parked = 0;
        pthread_mutex_unlock(&g_gc_mutex);
        return;
    }
    if (g_gc_allocated < g_gc_threshold || g_gc_stop_requested) {
        pthread_mutex_unlock(&g_gc_mutex);
        return;
    }

    g_gc_stop_requested = 1;
    g_gc_collector = current;
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_nsec += 100000000L;
    if (deadline.tv_nsec >= 1000000000L) { deadline.tv_sec++; deadline.tv_nsec -= 1000000000L; }
    int timed_out = 0;
    while (g_gc_parked_threads < nc_gc_other_active_threads(current)) {
        int rc = pthread_cond_timedwait(&g_gc_condition, &g_gc_mutex, &deadline);
        if (rc == ETIMEDOUT) { timed_out = 1; break; }
    }
    if (!timed_out) nc_gc_collect_locked(g_gc_minor_since_major >= 7);
    else if (getenv("NORSCODE_GC_TRACE"))
        fprintf(stderr, "[nc-gc] stop-the-world timeout; collection deferred\n");
    nc_gc_release_world_locked();
    pthread_mutex_unlock(&g_gc_mutex);
}

static void nc_gc_opcode_safepoint(void) {
    static _Thread_local unsigned int opcode_ticks = 0;
    opcode_ticks++;
    if ((opcode_ticks & 63U) == 0) nc_gc_safepoint();
}

/* ── Stage0-kjerne: feilhandtering ── */
static _Thread_local jmp_buf g_err_jmp;
static _Thread_local char    g_err_msg[4096];
static _Thread_local NcGcFrame *g_err_gc_boundary = NULL;

static void nc_panic(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(g_err_msg, sizeof(g_err_msg), fmt, ap);
    va_end(ap);
    nc_gc_unwind_thread_to(g_err_gc_boundary);
    longjmp(g_err_jmp, 1);
}

/* ── Stage0-kjerne: konstruktørar ── */
static NcVal *nc_nil(void) {
    return nc_val_alloc(NC_NIL);
}
static NcVal *nc_int(long long i) {
    NcVal *v = nc_val_alloc(NC_INT); v->i = i; v->gc_payload_size = sizeof(i); return v;
}
static NcVal *nc_float(double f) {
    NcVal *v = nc_val_alloc(NC_FLOAT); v->f = f; v->gc_payload_size = sizeof(f); return v;
}
static NcVal *nc_bool(int b) {
    NcVal *v = nc_val_alloc(NC_BOOL); v->b = b; v->gc_payload_size = sizeof(b); return v;
}
static NcVal *nc_str(const char *s) {
    NcVal *v = nc_val_alloc(NC_STR);
    v->s = strdup(s ? s : ""); v->slen = strlen(v->s);
    v->gc_payload_size = v->slen + 1; return v;
}
static NcVal *nc_str_own(char *s) {
    NcVal *v = nc_val_alloc(NC_STR);
    v->s = s; v->slen = s ? strlen(s) : 0;
    v->gc_payload_size = v->slen + 1; return v;
}
static NcVal *nc_list_new(void) {
    NcVal *v = nc_val_alloc(NC_LIST);
    v->list = calloc(1, sizeof(NcList)); v->gc_payload_size = sizeof(NcList); return v;
}
static NcVal *nc_map_new(void) {
    NcVal *v = nc_val_alloc(NC_MAP);
    v->map = calloc(1, sizeof(NcMap));
    v->gc_payload_size = sizeof(NcMap);
    pthread_mutex_init(&v->map->mutex, NULL);
    return v;
}
static NcVal *nc_bytes_new(size_t len, unsigned char fill) {
    NcVal *v = nc_val_alloc(NC_BYTES);
    v->bytes = calloc(1, sizeof(NcBytes));
    v->bytes->len = len; v->bytes->cap = len;
    if (len > 0) {
        v->bytes->data = malloc(len);
        if (!v->bytes->data) nc_panic("bytes: minnefeil");
        memset(v->bytes->data, fill, len);
    }
    v->gc_payload_size = sizeof(NcBytes) + len;
    return v;
}

/* ── Forward-deklarasjonar for wrappers brukt av generert C ── */
static NcVal *nc_builtin_desimaltall(NcVal *v);
static NcVal *nc_builtin_bytes_new(NcVal *size_v, NcVal *fill_v);
static NcVal *nc_builtin_bytes_from_list(NcVal *list_v);
static NcVal *nc_builtin_bytes_to_list(NcVal *bytes_v);
static NcVal *nc_builtin_socket_listen(NcVal *host_v, NcVal *port_v);
static NcVal *nc_builtin_socket_accept(NcVal *srv_v);
static NcVal *nc_builtin_socket_read(NcVal *conn_v, NcVal *max_v);
static NcVal *nc_builtin_socket_write(NcVal *conn_v, NcVal *data_v);
static NcVal *nc_builtin_socket_read_bytes(NcVal *conn_v, NcVal *max_v);
static NcVal *nc_builtin_socket_write_bytes(NcVal *conn_v, NcVal *data_v);
static NcVal *nc_builtin_socket_close(NcVal *conn_v);
NcVal *nc_builtin_ncb_route_handlers(NcVal **args, int na);
NcVal *nc_builtin_ncb_metadata(NcVal **args, int na);
NcVal *nc_builtin_ncb_next_request_id(NcVal **args, int na);
NcVal *nc_builtin_ncb_call_fn(NcVal **args, int na);
#if defined(NC_RUNTIME_STANDALONE)
NcVal *nc_builtin_ncb_route_handlers(NcVal **args, int na) { (void)args; (void)na; return nc_nil(); }
NcVal *nc_builtin_ncb_metadata(NcVal **args, int na) { (void)args; (void)na; return nc_nil(); }
NcVal *nc_builtin_ncb_next_request_id(NcVal **args, int na) { (void)args; (void)na; return nc_nil(); }
NcVal *nc_builtin_ncb_call_fn(NcVal **args, int na) { (void)args; (void)na; return nc_nil(); }
#endif
static NcVal *nc_builtin_random_hex(NcVal *n_v);
static NcVal *nc_builtin_exec_prosess(NcVal *cmd_v);
static NcVal *nc_builtin_tid_ms(void);
static NcVal *nc_builtin_tid_no(void);
static NcVal *nc_builtin_now_iso(void);
/* Desse er leverte av nc_native_main.c i full host-kandidat. */
static NcVal *nc_builtin_atomic_operation(NcVal *request);
static NcVal *nc_builtin_process_spawn_argv(NcVal *request);
static NcVal *nc_builtin_process_operation(NcVal *request);
static NcVal *nc_builtin_filesystem_read_operation(NcVal *request);
static NcVal *nc_builtin_filesystem_write_operation(NcVal *request);
static NcVal *nc_builtin_network_operation(NcVal *request);
static NcVal *nc_builtin_thread_spawn(NcVal *request);
static NcVal *nc_builtin_thread_join(NcVal *request);
static NcVal *nc_builtin_thread_sync(NcVal *request);
static NcVal *nc_builtin_thread_current_id(void);

/* ── Wrappers brukt av generert C-dispatch ── */
static NcVal *nc_fn_builtin_desimaltall(NcVal **args, int na) {
    return nc_builtin_desimaltall(na > 0 ? args[0] : nc_nil());
}
static NcVal *nc_fn_builtin_socket_listen(NcVal **args, int na) {
    return nc_builtin_socket_listen(na > 0 ? args[0] : nc_nil(), na > 1 ? args[1] : nc_nil());
}
static NcVal *nc_fn_builtin_socket_accept(NcVal **args, int na) {
    return nc_builtin_socket_accept(na > 0 ? args[0] : nc_nil());
}
static NcVal *nc_fn_builtin_socket_read(NcVal **args, int na) {
    return nc_builtin_socket_read(na > 0 ? args[0] : nc_nil(), na > 1 ? args[1] : nc_nil());
}
static NcVal *nc_fn_builtin_socket_write(NcVal **args, int na) {
    return nc_builtin_socket_write(na > 0 ? args[0] : nc_nil(), na > 1 ? args[1] : nc_nil());
}
static NcVal *nc_fn_builtin_socket_close(NcVal **args, int na) {
    return nc_builtin_socket_close(na > 0 ? args[0] : nc_nil());
}
static NcVal *nc_builtin_socket_new(NcVal *kind_v, NcVal *mode_v) {
    (void)kind_v; (void)mode_v;
    return nc_nil();
}
static NcVal *nc_builtin_socket_connect(NcVal *sock_v, NcVal *host_v, NcVal *port_v) {
    (void)sock_v; (void)host_v; (void)port_v;
    nc_panic("socket_connect ikkje støtta i v3002 maintainer-kandidat");
    return nc_nil();
}
static NcVal *nc_builtin_socket_bind(NcVal *sock_v, NcVal *host_v, NcVal *port_v) {
    (void)sock_v;
    return nc_builtin_socket_listen(host_v, port_v);
}
static NcVal *nc_builtin_socket_send(NcVal *conn_v, NcVal *data_v) {
    return nc_builtin_socket_write(conn_v, data_v);
}
static NcVal *nc_builtin_socket_send_bytes(NcVal *conn_v, NcVal *data_v) {
    return nc_builtin_socket_write_bytes(conn_v, data_v);
}
static NcVal *nc_builtin_socket_recv(NcVal *conn_v, NcVal *max_v) {
    return nc_builtin_socket_read(conn_v, max_v);
}
static NcVal *nc_builtin_socket_recv_bytes(NcVal *conn_v, NcVal *max_v) {
    return nc_builtin_socket_read_bytes(conn_v, max_v);
}
static NcVal *nc_builtin_socket_settimeout(NcVal *sock_v, NcVal *timeout_v) {
    (void)sock_v; (void)timeout_v;
    return nc_bool(1);
}
static NcVal *nc_fn_builtin_random_hex(NcVal **args, int na) {
    return nc_builtin_random_hex(na > 0 ? args[0] : nc_int(32));
}
static NcVal *nc_fn_builtin_exec_prosess(NcVal **args, int na) {
    return nc_builtin_exec_prosess(na > 0 ? args[0] : nc_str(""));
}
static NcVal *nc_fn_builtin_tid_ms(NcVal **args, int na) {
    (void)args; (void)na; return nc_builtin_tid_ms();
}
static NcVal *nc_fn_builtin_tid_no(NcVal **args, int na) {
    (void)args; (void)na; return nc_builtin_tid_no();
}
static NcVal *nc_fn_builtin_now_iso(NcVal **args, int na) {
    (void)args; (void)na; return nc_builtin_now_iso();
}

/* ── Stage0-kjerne: stack ── */
static void nc_push(int *sp, NcVal **stack, NcVal *v) {
    if (*sp >= 8192) nc_panic("Stack overflow");
    stack[(*sp)++] = v ? v : nc_nil();
}
static NcVal *nc_pop(int *sp, NcVal **stack) {
    if (*sp <= 0) nc_panic("Stack underflow");
    return stack[--(*sp)];
}

/* ── Stage0-kjerne: variabeloppslag ── */
static void nc_store(NcVal **vars, char **varnames, int *nvars, const char *name, NcVal *val) {
    for (int i = 0; i < *nvars; i++) {
        if (!strcmp(varnames[i], name)) { vars[i] = val; return; }
    }
    if (*nvars >= 2048) nc_panic("For mange variablar");
    varnames[*nvars] = strdup(name);
    vars[(*nvars)++] = val;
}
static NcVal *nc_load(NcVal **vars, char **varnames, int nvars, const char *name) {
    for (int i = 0; i < nvars; i++) {
        if (!strcmp(varnames[i], name)) return vars[i];
    }
    if (!strcmp(name, "null") || !strcmp(name, "ingenting")) return nc_nil();
    if (!strcmp(name, "true") || !strcmp(name, "sann"))  return nc_bool(1);
    if (!strcmp(name, "false") || !strcmp(name, "usann")) return nc_bool(0);
    nc_panic("Ukjent variabel: %s", name);
    return nc_nil();
}

/* ── Stage0-kjerne: konvertering/truthiness ── */
static int nc_truthy(NcVal *v) {
    if (!v || v->type == NC_NIL) return 0;
    if (v->type == NC_BOOL) return v->b;
    if (v->type == NC_INT)  return v->i != 0;
    if (v->type == NC_FLOAT) return v->f != 0.0;
    if (v->type == NC_STR)  return v->s && v->s[0] != 0;
    if (v->type == NC_LIST) return v->list->len > 0;
    if (v->type == NC_BYTES) return v->bytes && v->bytes->len > 0;
    return 1;
}

static char *nc_to_str_raw(NcVal *v) {
    if (!v || v->type == NC_NIL) return strdup("ingenting");
    if (v->type == NC_STR)  {
        size_t len = v->slen;
        if (!len && v->s) len = strlen(v->s);
        char *out = malloc(len + 1);
        if (!out) return strdup("");
        if (v->s && len > 0) memcpy(out, v->s, len);
        out[len] = '\0';
        return out;
    }
    if (v->type == NC_BOOL) return strdup(v->b ? "sann" : "usann");
    if (v->type == NC_INT)  {
        char buf[32]; snprintf(buf, sizeof(buf), "%lld", v->i);
        return strdup(buf);
    }
    if (v->type == NC_FLOAT) {
        char buf[64]; snprintf(buf, sizeof(buf), "%.15g", v->f);
        return strdup(buf);
    }
    if (v->type == NC_BYTES) {
        char buf[64]; snprintf(buf, sizeof(buf), "[bytes %zu]", v->bytes ? v->bytes->len : 0);
        return strdup(buf);
    }
    return strdup("[verdi]");
}

/* ── Stage0-kjerne: aritmetikk ── */
static NcVal *nc_add(NcVal *a, NcVal *b) {
    if (a->type == NC_STR || b->type == NC_STR) {
        char *sa = nc_to_str_raw(a), *sb = nc_to_str_raw(b);
        char *r = malloc(strlen(sa)+strlen(sb)+1);
        strcpy(r, sa); strcat(r, sb);
        free(sa); free(sb);
        return nc_str_own(r);
    }
    if (a->type == NC_INT && b->type == NC_INT) return nc_int(a->i + b->i);
    if ((a->type == NC_INT || a->type == NC_FLOAT) && (b->type == NC_INT || b->type == NC_FLOAT))
        return nc_float((a->type == NC_FLOAT ? a->f : (double)a->i) + (b->type == NC_FLOAT ? b->f : (double)b->i));
    return nc_nil();
}
static NcVal *nc_sub(NcVal *a, NcVal *b) {
    if (a->type == NC_INT && b->type == NC_INT) return nc_int(a->i - b->i);
    if ((a->type == NC_INT || a->type == NC_FLOAT) && (b->type == NC_INT || b->type == NC_FLOAT))
        return nc_float((a->type == NC_FLOAT ? a->f : (double)a->i) - (b->type == NC_FLOAT ? b->f : (double)b->i));
    return nc_nil();
}
static NcVal *nc_mul(NcVal *a, NcVal *b) {
    if (a->type == NC_INT && b->type == NC_INT) return nc_int(a->i * b->i);
    if ((a->type == NC_INT || a->type == NC_FLOAT) && (b->type == NC_INT || b->type == NC_FLOAT))
        return nc_float((a->type == NC_FLOAT ? a->f : (double)a->i) * (b->type == NC_FLOAT ? b->f : (double)b->i));
    return nc_nil();
}
static NcVal *nc_div(NcVal *a, NcVal *b) {
    if (a->type == NC_INT && b->type == NC_INT) {
        if (b->i == 0) nc_panic("Divisjon med null");
        return nc_int(a->i / b->i);
    }
    if ((a->type == NC_INT || a->type == NC_FLOAT) && (b->type == NC_INT || b->type == NC_FLOAT)) {
        double divisor = b->type == NC_FLOAT ? b->f : (double)b->i;
        if (divisor == 0.0) nc_panic("Divisjon med null");
        return nc_float((a->type == NC_FLOAT ? a->f : (double)a->i) / divisor);
    }
    return nc_nil();
}
static NcVal *nc_mod(NcVal *a, NcVal *b) {
    if (a->type == NC_INT && b->type == NC_INT) {
        if (b->i == 0) nc_panic("Modulo med null");
        return nc_int(a->i % b->i);
    }
    return nc_nil();
}
static NcVal *nc_lshift(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av<<bv);
}
static NcVal *nc_rshift(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av>>bv);
}
static NcVal *nc_band(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av&bv);
}
static NcVal *nc_bor(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av|bv);
}
static NcVal *nc_bxor(NcVal *a, NcVal *b) {
    long long av=a&&a->type==NC_INT?a->i:0, bv=b&&b->type==NC_INT?b->i:0;
    return nc_int(av^bv);
}
static NcVal *nc_neg(NcVal *a) {
    if (a->type == NC_INT) return nc_int(-a->i);
    if (a->type == NC_FLOAT) return nc_float(-a->f);
    return nc_nil();
}

/* ── Stage0-kjerne: samanlikning ── */
static int nc_eq(NcVal *a, NcVal *b) {
    if (!a || !b) return a == b;
    if (a->type == NC_NIL && b->type == NC_NIL) return 1;
    if (a->type == NC_BOOL && b->type == NC_BOOL) return a->b == b->b;
    if (a->type == NC_INT  && b->type == NC_INT)  return a->i == b->i;
    if ((a->type == NC_INT || a->type == NC_FLOAT) && (b->type == NC_INT || b->type == NC_FLOAT))
        return (a->type == NC_FLOAT ? a->f : (double)a->i) == (b->type == NC_FLOAT ? b->f : (double)b->i);
    if (a->type == NC_STR  && b->type == NC_STR)  return !strcmp(a->s, b->s);
    if (a->type == NC_BYTES && b->type == NC_BYTES) {
        if (!a->bytes || !b->bytes || a->bytes->len != b->bytes->len) return 0;
        return a->bytes->len == 0 || memcmp(a->bytes->data, b->bytes->data, a->bytes->len) == 0;
    }
    return 0;
}
/* mode: -1=LT, 1=GT, -2=LE, 2=GE */
static NcVal *nc_cmp(NcVal *a, NcVal *b, int mode) {
    int r = 0;
    if (a->type == NC_INT && b->type == NC_INT) {
        r = (a->i < b->i) ? -1 : (a->i > b->i) ? 1 : 0;
    } else if ((a->type == NC_INT || a->type == NC_FLOAT) && (b->type == NC_INT || b->type == NC_FLOAT)) {
        double av = a->type == NC_FLOAT ? a->f : (double)a->i;
        double bv = b->type == NC_FLOAT ? b->f : (double)b->i;
        r = (av < bv) ? -1 : (av > bv) ? 1 : 0;
    } else if (a->type == NC_STR && b->type == NC_STR) {
        r = strcmp(a->s, b->s);
    }
    int ok = (mode == -1) ? r < 0 : (mode == 1) ? r > 0 : (mode == -2) ? r <= 0 : r >= 0;
    return nc_bool(ok);
}

/* ── Stage0-kjerne: liste/ordbok-bygging ── */
static NcVal *nc_build_list(int *sp, NcVal **stack, int n) {
    NcVal *lst = nc_list_new();
    lst->list->items = calloc(n, sizeof(NcVal*));
    lst->list->cap = n;
    NcVal **tmp = malloc(n * sizeof(NcVal*));
    for (int i = n-1; i >= 0; i--) tmp[i] = nc_pop(sp, stack);
    for (int i = 0; i < n; i++) {
        lst->list->items[lst->list->len++] = tmp[i];
    }
    free(tmp);
    return lst;
}
static NcVal *nc_build_map(int *sp, NcVal **stack, int n) {
    NcVal *m = nc_map_new();
    for (int i = 0; i < n; i++) {
        NcVal *v = nc_pop(sp, stack), *k = nc_pop(sp, stack);
        char *ks = nc_to_str_raw(k);
        /* veks kapasiteten ved behov */
        if (m->map->len >= m->map->cap) {
            m->map->cap = m->map->cap ? m->map->cap*2 : 4;
            m->map->keys = realloc(m->map->keys, m->map->cap*sizeof(char*));
            m->map->vals = realloc(m->map->vals, m->map->cap*sizeof(NcVal*));
            m->map->hashes = realloc(m->map->hashes, m->map->cap*sizeof(uint64_t));
        }
        m->map->keys[m->map->len] = ks;
        m->map->vals[m->map->len] = v;
        m->map->hashes[m->map->len] = nc_map_hash(ks);
        m->map->len++;
        nc_map_index_insert_locked(m->map, m->map->len - 1);
    }
    return m;
}

/* ── Stage0-kjerne: indeksering ── */
static NcVal *nc_index_get(NcVal *obj, NcVal *key) {
    if (!obj) return nc_nil();
    if (obj->type == NC_LIST) {
        long long idx = (key->type == NC_INT) ? key->i : atoll(key->s);
        if (idx < 0) idx = obj->list->len + idx;
        if (idx < 0 || idx >= obj->list->len) nc_panic("Indeks utanfor: %lld", idx);
        return obj->list->items[idx];
    }
    if (obj->type == NC_MAP) {
        char *owned_key = NULL;
        const char *ks = (key && key->type == NC_STR) ? key->s : (owned_key = nc_to_str_raw(key));
        uint64_t hash = nc_map_hash(ks);
        pthread_mutex_lock(&obj->map->mutex);
        int index = nc_map_find_locked(obj->map, ks, hash);
        if (index >= 0) {
            NcVal *result = obj->map->vals[index];
            pthread_mutex_unlock(&obj->map->mutex);
            free(owned_key); return result;
        }
        pthread_mutex_unlock(&obj->map->mutex);
        free(owned_key); return nc_nil();
    }
    if (obj->type == NC_STR) {
        long long idx = (key->type == NC_INT) ? key->i : atoll(key->s);
        long long slen = (long long)(obj->slen ? obj->slen : (obj->slen = strlen(obj->s)));
        if (idx < 0) idx = slen + idx;
        if (idx < 0 || idx >= slen) return nc_str("");
        char buf[2] = {obj->s[idx], 0};
        return nc_str(buf);
    }
    if (obj->type == NC_BYTES) {
        long long idx = key && key->type == NC_INT ? key->i : -1;
        if (idx < 0) idx += (long long)obj->bytes->len;
        if (idx < 0 || (size_t)idx >= obj->bytes->len) nc_panic("Bytes-indeks utanfor: %lld", idx);
        return nc_int(obj->bytes->data[idx]);
    }
    return nc_nil();
}
static void nc_index_set(NcVal *obj, NcVal *key, NcVal *val) {
    if (!obj) return;
    if (obj->type == NC_BYTES) {
        long long idx = key && key->type == NC_INT ? key->i : -1;
        if (idx < 0) idx += (long long)obj->bytes->len;
        if (idx < 0 || (size_t)idx >= obj->bytes->len) nc_panic("Bytes-indeks utanfor: %lld", idx);
        if (!val || val->type != NC_INT || val->i < 0 || val->i > 255) nc_panic("Bytes-verdi må vere 0..255");
        obj->bytes->data[idx] = (unsigned char)val->i;
        return;
    }
    if (obj->type == NC_LIST) {
        long long idx = (key->type == NC_INT) ? key->i : atoll(key->s);
        if (idx < 0) idx = obj->list->len + idx;
        if (idx >= obj->list->len) {
            while (obj->list->len <= idx) {
                if (obj->list->len >= obj->list->cap) {
                    obj->list->cap = obj->list->cap ? obj->list->cap*2 : 8;
                    obj->list->items = realloc(obj->list->items, obj->list->cap*sizeof(NcVal*));
                }
                obj->list->items[obj->list->len++] = nc_nil();
            }
        }
        obj->list->items[idx] = val;
        return;
    }
    if (obj->type == NC_MAP) {
        char *owned_key = NULL;
        const char *ks = (key && key->type == NC_STR) ? key->s : (owned_key = nc_to_str_raw(key));
        uint64_t hash = nc_map_hash(ks);
        pthread_mutex_lock(&obj->map->mutex);
        int index = nc_map_find_locked(obj->map, ks, hash);
        if (index >= 0) {
            obj->map->vals[index] = val;
            pthread_mutex_unlock(&obj->map->mutex);
            free(owned_key); return;
        }
        if (obj->map->len >= obj->map->cap) {
            obj->map->cap = obj->map->cap ? obj->map->cap*2 : 4;
            obj->map->keys = realloc(obj->map->keys, obj->map->cap*sizeof(char*));
            obj->map->vals = realloc(obj->map->vals, obj->map->cap*sizeof(NcVal*));
            obj->map->hashes = realloc(obj->map->hashes, obj->map->cap*sizeof(uint64_t));
        }
        obj->map->keys[obj->map->len] = owned_key ? owned_key : strdup(ks);
        obj->map->vals[obj->map->len] = val;
        obj->map->hashes[obj->map->len] = hash;
        obj->map->len++;
        nc_map_index_insert_locked(obj->map, obj->map->len - 1);
        pthread_mutex_unlock(&obj->map->mutex);
    }
}

/* ── Stage0-kjerne: unntak ── */
static char g_throw_msg[4096];
static void nc_throw(const char *msg) {
    strncpy(g_throw_msg, msg ? msg : "ukjent feil", sizeof(g_throw_msg)-1);
    nc_panic("Norscode unntak: %s", g_throw_msg);
}

/*
 * Hjelpelag over stage0-kjerna.
 */
/* ── Hjelpelag: innebygde funksjonar ── */
static NcVal *nc_builtin_lengde(NcVal *v) {
    if (!v) return nc_int(0);
    if (v->type == NC_STR)  return nc_int((long long)(v->slen ? v->slen : (v->slen=strlen(v->s))));
    if (v->type == NC_LIST) return nc_int(v->list->len);
    if (v->type == NC_BYTES) return nc_int(v->bytes ? (long long)v->bytes->len : 0);
    if (v->type == NC_MAP)  {
        pthread_mutex_lock(&v->map->mutex);
        int len = v->map->len;
        pthread_mutex_unlock(&v->map->mutex);
        return nc_int(len);
    }
    return nc_int(0);
}
static NcVal *nc_builtin_legg_til(NcVal *lst, NcVal *v) {
    if (!lst || lst->type != NC_LIST) return nc_nil();
    if (lst->list->len >= lst->list->cap) {
        lst->list->cap = lst->list->cap ? lst->list->cap*2 : 8;
        lst->list->items = realloc(lst->list->items, lst->list->cap*sizeof(NcVal*));
    }
    lst->list->items[lst->list->len++] = v;
    return nc_nil();
}
static NcVal *nc_builtin_fjern_siste(NcVal *lst) {
    if (!lst || lst->type != NC_LIST || lst->list->len == 0) return nc_nil();
    return lst->list->items[--lst->list->len];
}
static NcVal *nc_builtin_fjern(NcVal *lst, NcVal *idx_v) {
    if (!lst || lst->type != NC_LIST) return nc_nil();
    long long idx = idx_v->type == NC_INT ? idx_v->i : 0;
    if (idx < 0 || idx >= lst->list->len) return nc_nil();
    NcVal *r = lst->list->items[idx];
    memmove(&lst->list->items[idx], &lst->list->items[idx+1], (lst->list->len-idx-1)*sizeof(NcVal*));
    lst->list->len--;
    return r;
}
static NcVal *nc_builtin_slice(NcVal *v, NcVal *a_v, NcVal *b_v) {
    if (!v) return nc_str("");
    long long a = (a_v && a_v->type==NC_INT) ? a_v->i : 0;
    if (v->type == NC_LIST) {
        long long len = v->list->len;
        long long b = (b_v && b_v->type==NC_INT && b_v->i != -1) ? b_v->i : len;
        if (a < 0) a = 0; if (b < 0 || b > len) b = len; if (b < a) b = a;
        NcVal *r = nc_list_new();
        for (long long i=a; i<b; i++) nc_builtin_legg_til(r, v->list->items[i]);
        return r;
    }
    if (v->type != NC_STR) return nc_str("");
    long long len = (long long)(v->slen ? v->slen : (v->slen=strlen(v->s)));
    long long b = (b_v && b_v->type==NC_INT && b_v->i != -1) ? b_v->i : len;
    if (a < 0) a = 0; if (a > len) a = len;
    if (b < a) b = a; if (b > len) b = len;
    char *r = malloc(b-a+1);
    memcpy(r, v->s+a, b-a); r[b-a] = 0;
    return nc_str_own(r);
}
static NcVal *nc_builtin_skriv(NcVal *v) {
    char *s = nc_to_str_raw(v); fputs(s, stdout); free(s); fflush(stdout);
    return nc_nil();
}
static NcVal *nc_builtin_skriv_linje(NcVal *v) {
    NcVal *r = nc_builtin_skriv(v);
    fputc('\n', stdout); fflush(stdout);
    return r;
}
static NcVal *nc_builtin_vent_sov(NcVal *millis_v) {
    long long millis = millis_v && millis_v->type == NC_INT ? millis_v->i : 0;
    if (millis < 0) millis = 0;
    struct timespec req = { millis / 1000, (millis % 1000) * 1000000L };
    while (nanosleep(&req, &req) != 0 && errno == EINTR) { }
    return nc_nil();
}
/* Fil/miljø-bridge mot host-OS via `nc_host_*`. */
/* ── Hjelpelag: fil/miljø-host-bridge ── */
static FILE *nc_host_fopen(const char *path,const char *mode){
#if defined(_WIN32)
    if(!path||!mode)return NULL;int path_len=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,NULL,0);int mode_len=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,mode,-1,NULL,0);
    if(path_len<=0||mode_len<=0)return NULL;wchar_t *wide_path=malloc((size_t)path_len*sizeof(wchar_t)),*wide_mode=malloc((size_t)mode_len*sizeof(wchar_t));
    if(!wide_path||!wide_mode){free(wide_path);free(wide_mode);return NULL;}MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide_path,path_len);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,mode,-1,wide_mode,mode_len);
    FILE *file=_wfopen(wide_path,wide_mode);free(wide_path);free(wide_mode);return file;
#else
    return fopen(path,mode);
#endif
}
static int nc_host_file_exists(const char *path) {
    FILE *f = nc_host_fopen(path, "rb");
    int ok = f != NULL;
    if (f) fclose(f);
    return ok;
}
static char *nc_host_read_text_path(const char *path) {
    FILE *f = nc_host_fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    return buf;
}
static int nc_host_write_text_path(const char *path, const char *data) {
    FILE *f = nc_host_fopen(path, "wb");
    if (!f) return 0;
    fputs(data, f);
    fclose(f);
    return 1;
}
static int nc_host_write_binary_path(const char *path, NcVal *list_v) {
    FILE *f = nc_host_fopen(path, "wb");
    if (!f) return 0;
    if (list_v && list_v->type == NC_BYTES) {
        if (list_v->bytes->len > 0) fwrite(list_v->bytes->data, 1, list_v->bytes->len, f);
    } else if (list_v && list_v->type == NC_LIST) {
        for (int i = 0; i < list_v->list->len; i++) {
            NcVal *bv = list_v->list->items[i];
            unsigned char byte = (unsigned char)(bv && bv->type == NC_INT ? (int)bv->i : 0);
            fwrite(&byte, 1, 1, f);
        }
    }
    fclose(f);
    return 1;
}
static int nc_host_append_text_path(const char *path, const char *data) {
    FILE *f = nc_host_fopen(path, "ab");
    if (!f) return 0;
    fputs(data, f);
    fclose(f);
    return 1;
}
static int nc_host_append_binary_path(const char *path, NcVal *data_v) {
    FILE *f = nc_host_fopen(path, "ab");
    if (!f) return 0;
    if (data_v && data_v->type == NC_BYTES && data_v->bytes && data_v->bytes->len > 0) {
        fwrite(data_v->bytes->data, 1, data_v->bytes->len, f);
    } else if (data_v && data_v->type == NC_LIST) {
        for (int i = 0; i < data_v->list->len; i++) {
            NcVal *item = data_v->list->items[i];
            unsigned char byte = (unsigned char)(item && item->type == NC_INT ? item->i : 0);
            fwrite(&byte, 1, 1, f);
        }
    }
    fclose(f);
    return 1;
}
static const char *nc_host_getenv_raw(const char *key, int *exists) {
    const char *v = getenv(key);
    if (exists) *exists = v != NULL;
    return v;
}
static char *nc_host_getenv_text(const char *key) {
    const char *v = nc_host_getenv_raw(key, NULL);
    return strdup(v ? v : "");
}
static NcVal *nc_env_get_text(const char *key) {
    return nc_str_own(nc_host_getenv_text(key));
}
static NcVal *nc_env_exists(const char *key) {
    int exists = 0;
    nc_host_getenv_raw(key, &exists);
    return nc_bool(exists);
}
static NcVal *nc_env_set_text(const char *key, const char *val) {
    setenv(key, val, 1);
    return nc_env_get_text(key);
}
static NcVal *nc_host_file_error_nil(const char *op, const char *path) {
    char msg[512];
    snprintf(msg, sizeof(msg), "Kan ikkje %s fil: %s", op, path);
    nc_throw(msg);
    return nc_nil();
}
static NcVal *nc_host_file_write_result(int ok, const char *op, const char *path) {
    return ok ? nc_nil() : nc_host_file_error_nil(op, path);
}
static NcVal *nc_file_read_text(const char *path) {
    char *buf = nc_host_read_text_path(path);
    return buf ? nc_str_own(buf) : nc_host_file_error_nil("opne", path);
}
static NcVal *nc_file_write_text(const char *path, const char *data) {
    return nc_host_file_write_result(nc_host_write_text_path(path, data), "skrive", path);
}
static NcVal *nc_file_write_binary(const char *path, NcVal *list_v) {
    return nc_host_file_write_result(nc_host_write_binary_path(path, list_v), "skrive", path);
}
static NcVal *nc_file_append_text(const char *path, const char *data) {
    return nc_host_file_write_result(nc_host_append_text_path(path, data), "append til", path);
}
static NcVal *nc_file_exists_value(const char *path) {
    return nc_bool(nc_host_file_exists(path));
}

static NcVal *nc_builtin_fil_les(NcVal *path_v) {
    char *path = nc_to_str_raw(path_v);
    if (g_trusted_file_resolution_depth > 0 && path[0] != '/' &&
        (!strncmp(path, "std/", 4) || !strncmp(path, "selfhost/", 9) ||
         !strncmp(path, "bootstrap/", 10)) && access(path, R_OK) != 0) {
        const char *root = getenv("NORSCODE_ROOT");
        if (root && root[0]) {
            size_t needed = strlen(root) + strlen(path) + 2;
            char *rooted = malloc(needed);
            if (rooted) {
                snprintf(rooted, needed, "%s/%s", root, path);
                if (access(rooted, R_OK) == 0) { free(path); path = rooted; }
                else free(rooted);
            }
        }
    }
    /* Older embedded compiler bundles prefer .nors for imported modules.
     * Accept the current .no source extension without requiring duplicate files. */
    if (access(path, R_OK) != 0) {
        size_t n = strlen(path);
        if (n > 5 && !strcmp(path + n - 5, ".nors")) {
            char *fallback = malloc(n);
            if (fallback) {
                memcpy(fallback, path, n - 5);
                memcpy(fallback + n - 5, ".no", 4);
                if (access(fallback, R_OK) == 0) {
                    free(path);
                    path = fallback;
                } else {
                    free(fallback);
                }
            }
        }
    }
    NcVal *r = nc_file_read_text(path);
    free(path);
    return r;
}
static NcVal *nc_builtin_fil_les_binary(NcVal *path_v) {
    char *path = nc_to_str_raw(path_v);
    FILE *f = nc_host_fopen(path, "rb");
    if (!f) { NcVal *r = nc_host_file_error_nil("opne", path); free(path); return r; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); free(path); nc_throw("Kan ikkje lese binær filstorleik"); }
    long size = ftell(f);
    if (size < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); free(path); nc_throw("Kan ikkje lese binær fil"); }
    NcVal *out = nc_bytes_new((size_t)size, 0);
    size_t read_count = size > 0 ? fread(out->bytes->data, 1, (size_t)size, f) : 0;
    fclose(f); free(path);
    if (read_count != (size_t)size) nc_throw("Ufullstendig binær fillesing");
    return out;
}
static NcVal *nc_builtin_fil_skriv(NcVal *path_v, NcVal *data_v) {
    char *path = nc_to_str_raw(path_v), *data = nc_to_str_raw(data_v);
    NcVal *r = nc_file_write_text(path, data);
    free(path);
    free(data);
    return r;
}
static NcVal *nc_builtin_fil_skriv_binary(NcVal *path_v, NcVal *list_v) {
    char *path = nc_to_str_raw(path_v);
    NcVal *r = nc_file_write_binary(path, list_v);
    free(path);
    return r;
}
static NcVal *nc_builtin_fil_finnes(NcVal *path_v) {
    char *path = nc_to_str_raw(path_v);
    NcVal *r = nc_file_exists_value(path);
    free(path);
    return r;
}
static NcVal *nc_builtin_fil_sett_kjorbar(NcVal *path_v) {
    char *path = nc_to_str_raw(path_v);
#if defined(_WIN32)
    /* Windows PE-køyrbarheit ligg i filformatet; fjern berre read-only. */
    int rc = _chmod(path, _S_IREAD | _S_IWRITE);
#else
    int rc = chmod(path, 0755);
#endif
    free(path);
    return nc_bool(rc == 0);
}
static NcVal *nc_builtin_miljo_hent(NcVal *k_v) {
    char *k = nc_to_str_raw(k_v);
    NcVal *r = nc_env_get_text(k);
    free(k);
    return r;
}
static NcVal *nc_builtin_miljo_finnes(NcVal *k_v) {
    char *k = nc_to_str_raw(k_v);
    NcVal *r = nc_env_exists(k);
    free(k);
    return r;
}

/* ── Socket builtins for maintainer-kandidat ──────────────────────────────── */
#if !defined(_WIN32)
typedef int nc_socket_handle_t;
#endif

static int nc_socket_invalid(nc_socket_handle_t fd) {
#if defined(_WIN32)
    return fd == INVALID_SOCKET;
#else
    return fd < 0;
#endif
}

static int nc_socket_close(nc_socket_handle_t fd) {
#if defined(_WIN32)
    return closesocket(fd);
#else
    return close(fd);
#endif
}

static NcVal *nc_builtin_socket_listen(NcVal *host_v, NcVal *port_v) {
#if defined(_WIN32)
    if (!nc_windows_socket_startup()) nc_panic("socket_listen: WSAStartup feila");
#endif
    char *host = nc_to_str_raw(host_v);
    int port = (int)((port_v && port_v->type == NC_INT) ? port_v->i : atoll(nc_to_str_raw(port_v)));
    nc_socket_handle_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (nc_socket_invalid(fd)) {
        free(host);
        nc_panic("socket_listen: socket feila");
    }
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (!host || !strcmp(host, "") || !strcmp(host, "0.0.0.0")) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else if (!strcmp(host, "localhost")) {
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        nc_socket_close(fd);
        free(host);
        nc_panic("socket_listen: ugyldig host");
    }
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        nc_socket_close(fd);
        free(host);
        nc_panic("socket_listen: bind feila");
    }
    if (listen(fd, 128) != 0) {
        nc_socket_close(fd);
        free(host);
        nc_panic("socket_listen: listen feila");
    }
    free(host);
    return nc_int((long long)(uintptr_t)fd);
}

static NcVal *nc_builtin_socket_accept(NcVal *srv_v) {
    nc_socket_handle_t srv = (nc_socket_handle_t)(uintptr_t)((srv_v && srv_v->type == NC_INT) ? srv_v->i : -1);
    nc_socket_handle_t fd = accept(srv, NULL, NULL);
    if (nc_socket_invalid(fd)) nc_panic("socket_accept: accept feila");
    return nc_int((long long)(uintptr_t)fd);
}

static NcVal *nc_builtin_socket_read(NcVal *conn_v, NcVal *max_v) {
    nc_socket_handle_t fd = (nc_socket_handle_t)(uintptr_t)((conn_v && conn_v->type == NC_INT) ? conn_v->i : -1);
    long long max_len = (max_v && max_v->type == NC_INT) ? max_v->i : 8192;
    if (max_len < 1) max_len = 8192;
    if (max_len > 1048576) max_len = 1048576;
    char *buf = calloc((size_t)max_len + 1, 1);
    if (!buf) nc_panic("socket_read: minnefeil");
    ssize_t n = recv(fd, buf, (int)max_len, 0);
    if (n < 0) {
        free(buf);
        nc_panic("socket_read: recv feila");
    }
    buf[n] = 0;
    return nc_str_own(buf);
}

static NcVal *nc_builtin_socket_read_bytes(NcVal *conn_v, NcVal *max_v) {
    nc_socket_handle_t fd = (nc_socket_handle_t)(uintptr_t)((conn_v && conn_v->type == NC_INT) ? conn_v->i : -1);
    long long max_len = (max_v && max_v->type == NC_INT) ? max_v->i : 8192;
    if (max_len < 1) max_len = 8192;
    if (max_len > 1048576) max_len = 1048576;
    NcVal *out = nc_bytes_new((size_t)max_len, 0);
    ssize_t n = recv(fd, (char *)out->bytes->data, (int)max_len, 0);
    if (n < 0) nc_panic("socket_read_bytes: recv feila");
    out->bytes->len = (size_t)n;
    out->gc_payload_size = sizeof(NcBytes) + (size_t)n;
    return out;
}

static NcVal *nc_builtin_socket_write(NcVal *conn_v, NcVal *data_v) {
    nc_socket_handle_t fd = (nc_socket_handle_t)(uintptr_t)((conn_v && conn_v->type == NC_INT) ? conn_v->i : -1);
    char *data = nc_to_str_raw(data_v);
    size_t len = strlen(data);
    size_t off = 0;
    while (off < len) {
        ssize_t n = send(fd, data + off, (int)(len - off), 0);
        if (n < 0) {
            free(data);
            nc_panic("socket_write: send feila");
        }
        off += (size_t)n;
    }
    free(data);
    return nc_int((long long)off);
}

static NcVal *nc_builtin_socket_write_bytes(NcVal *conn_v, NcVal *data_v) {
    nc_socket_handle_t fd = (nc_socket_handle_t)(uintptr_t)((conn_v && conn_v->type == NC_INT) ? conn_v->i : -1);
    if (data_v && data_v->type == NC_LIST) data_v = nc_builtin_bytes_from_list(data_v);
    if (!data_v || data_v->type != NC_BYTES) nc_throw("socket_send_bytes krev bytes eller byte-liste");
    size_t off = 0, len = data_v->bytes->len;
    while (off < len) {
        ssize_t n = send(fd, (const char *)data_v->bytes->data + off, (int)(len - off), 0);
        if (n < 0) nc_panic("socket_write_bytes: send feila");
        off += (size_t)n;
    }
    return nc_int((long long)off);
}

static NcVal *nc_builtin_socket_close(NcVal *conn_v) {
    nc_socket_handle_t fd = (nc_socket_handle_t)(uintptr_t)((conn_v && conn_v->type == NC_INT) ? conn_v->i : -1);
    if (!nc_socket_invalid(fd)) nc_socket_close(fd);
    return nc_bool(1);
}

/* ── Runtime-gap builtins: random, tid og prosess ── */
static int nc_secure_random_bytes(unsigned char *buf, size_t n) {
    if (!buf) return 0;
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    arc4random_buf(buf, n);
    return 1;
#elif defined(__linux__)
    size_t off = 0;
    while (off < n) {
        ssize_t r = getrandom(buf + off, n - off, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        off += (size_t)r;
    }
    if (off == n) return 1;
#endif
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return 0;
    size_t got = fread(buf, 1, n, f);
    fclose(f);
    return got == n;
}

static NcVal *nc_builtin_random_hex(NcVal *n_v) {
    long long n = (n_v && n_v->type == NC_INT) ? n_v->i : 32;
    if (n < 1 || n > 4096) nc_panic("random_hex: ugyldig lengde");
    unsigned char *bytes = calloc((size_t)n, 1);
    if (!bytes) nc_panic("random_hex: minnefeil");
    if (!nc_secure_random_bytes(bytes, (size_t)n)) {
        free(bytes);
        nc_panic("random_hex: trygg random ikkje tilgjengeleg");
    }
    static const char hexdigits[] = "0123456789abcdef";
    char *out = malloc((size_t)n * 2 + 1);
    if (!out) {
        free(bytes);
        nc_panic("random_hex: minnefeil");
    }
    for (long long i = 0; i < n; i++) {
        out[i * 2] = hexdigits[(bytes[i] >> 4) & 0xf];
        out[i * 2 + 1] = hexdigits[bytes[i] & 0xf];
    }
    out[(size_t)n * 2] = 0;
    free(bytes);
    return nc_str_own(out);
}

static long long nc_unix_ms_now(void) {
    struct timespec ts;
#if defined(CLOCK_REALTIME)
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return (long long)ts.tv_sec * 1000LL + (long long)(ts.tv_nsec / 1000000LL);
    }
#endif
    return (long long)time(NULL) * 1000LL;
}

static NcVal *nc_builtin_tid_ms(void) {
    return nc_int(nc_unix_ms_now());
}

static NcVal *nc_builtin_tid_no(void) {
    return nc_int(nc_unix_ms_now() / 1000LL);
}

static NcVal *nc_builtin_now_iso(void) {
    time_t t = (time_t)(nc_unix_ms_now() / 1000LL);
    struct tm tmv;
#if defined(_WIN32)
    gmtime_s(&tmv, &t);
#else
    gmtime_r(&t, &tmv);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmv);
    return nc_str(buf);
}

static NcVal *nc_builtin_exec_prosess(NcVal *cmd_v) {
    const char *enabled = getenv("NORSCODE_ENABLE_EXEC_PROSESS");
    if (!enabled || strcmp(enabled, "1")) {
        nc_panic("exec_prosess deaktivert: sett NORSCODE_ENABLE_EXEC_PROSESS=1 i DEV");
    }
    char *cmd = nc_to_str_raw(cmd_v);
    FILE *p = popen(cmd, "r");
    NcVal *res = nc_list_new();
    nc_builtin_legg_til(res, nc_int(1));
    if (!p) {
        nc_builtin_legg_til(res, nc_str(""));
        free(cmd);
        return res;
    }
    size_t cap = 4096, len = 0;
    char *out = calloc(cap, 1);
    char buf[512];
    while (fgets(buf, sizeof(buf), p)) {
        size_t bl = strlen(buf);
        if (len + bl + 1 > cap) {
            cap *= 2;
            if (cap > 1024 * 1024) break;
            out = realloc(out, cap);
        }
        memcpy(out + len, buf, bl);
        len += bl;
        out[len] = 0;
    }
    int status = pclose(p);
    int code = status;
#if defined(WIFEXITED)
    if (WIFEXITED(status)) code = WEXITSTATUS(status);
#endif
    char code_text[32]; snprintf(code_text, sizeof(code_text), "%d", code);
    res->list->items[0] = nc_str(code_text);
    nc_builtin_legg_til(res, nc_str_own(out));
    free(cmd);
    return res;
}
/* ── Hjelpelag: samlingshjelparar over liste/map-kjerna ── */
static int nc_coll_find_key_index(NcVal *m, const char *k) {
    if (!m || m->type != NC_MAP) return -1;
    for (int i = 0; i < m->map->len; i++) {
        if (!strcmp(m->map->keys[i], k)) return i;
    }
    return -1;
}
static NcVal *nc_coll_collect_list(NcVal *m, int values) {
    NcVal *lst = nc_list_new();
    if (!m || m->type != NC_MAP) return lst;
    for (int i = 0; i < m->map->len; i++) {
        nc_builtin_legg_til(lst, values ? m->map->vals[i] : nc_str(m->map->keys[i]));
    }
    return lst;
}
static NcVal *nc_builtin_finnes_nokkel(NcVal *m, NcVal *k_v) {
    char *k = nc_to_str_raw(k_v);
    if (m && m->type == NC_MAP) pthread_mutex_lock(&m->map->mutex);
    int ok = nc_coll_find_key_index(m, k) >= 0;
    if (m && m->type == NC_MAP) pthread_mutex_unlock(&m->map->mutex);
    free(k);
    return nc_bool(ok);
}
static NcVal *nc_builtin_nokler(NcVal *m) {
    return nc_coll_collect_list(m, 0);
}
static NcVal *nc_builtin_verdier(NcVal *m) {
    return nc_coll_collect_list(m, 1);
}
/* ── Hjelpelag: teksthjelparar over streng- og listekjerna ── */
static NcVal *nc_text_split_to_list(const char *s, const char *sep) {
    NcVal *lst = nc_list_new();
    size_t seplen = strlen(sep);
    if (seplen == 0) {
        for (size_t i = 0; s[i]; i++) {
            char buf[2] = {s[i], 0};
            nc_builtin_legg_til(lst, nc_str(buf));
        }
        return lst;
    }
    char *cur = (char *)s;
    char *found;
    while ((found = strstr(cur, sep)) != NULL) {
        size_t partlen = found - cur;
        char *part = malloc(partlen + 1);
        memcpy(part, cur, partlen);
        part[partlen] = 0;
        nc_builtin_legg_til(lst, nc_str_own(part));
        cur = found + seplen;
    }
    nc_builtin_legg_til(lst, nc_str(cur));
    return lst;
}
static char *nc_text_join_list(NcVal *lst, const char *sep) {
    size_t seplen = strlen(sep);
    int n = lst->list->len;
    size_t total = 0;
    char **parts = malloc(n * sizeof(char*));
    size_t *lens = malloc(n * sizeof(size_t));
    for (int i = 0; i < n; i++) {
        parts[i] = nc_to_str_raw(lst->list->items[i]);
        lens[i] = strlen(parts[i]);
        total += lens[i];
    }
    if (n > 1) total += seplen * (n - 1);
    char *r = malloc(total + 1);
    char *wp = r;
    for (int i = 0; i < n; i++) {
        if (i > 0) { memcpy(wp, sep, seplen); wp += seplen; }
        memcpy(wp, parts[i], lens[i]); wp += lens[i];
        free(parts[i]);
    }
    *wp = 0;
    free(parts);
    free(lens);
    return r;
}
static char *nc_text_replace_copy(const char *s, const char *old, const char *newstr) {
    size_t olen = strlen(old), nlen = strlen(newstr);
    if (olen == 0) return strdup(s);
    int cnt = 0;
    char *p = (char *)s;
    while ((p = strstr(p, old)) != NULL) { cnt++; p += olen; }
    char *r = malloc(strlen(s) + cnt * (nlen > olen ? nlen - olen : 0) + 1);
    char *wp = r;
    p = (char *)s;
    char *found;
    while ((found = strstr(p, old)) != NULL) {
        memcpy(wp, p, found - p); wp += found - p;
        memcpy(wp, newstr, nlen); wp += nlen;
        p = found + olen;
    }
    strcpy(wp, p);
    return r;
}
static int nc_text_index_of_raw(const char *s, const char *p) {
    char *found = strstr(s, p);
    return found ? (int)(found - s) : -1;
}
static int nc_text_starts_with_raw(const char *s, const char *p) {
    return strncmp(s, p, strlen(p)) == 0;
}
static int nc_text_ends_with_raw(const char *s, const char *p) {
    size_t sl = strlen(s), pl = strlen(p);
    return sl >= pl && strcmp(s + sl - pl, p) == 0;
}
static int nc_text_contains_raw(const char *s, const char *p) {
    return strstr(s, p) != NULL;
}
static NcVal *nc_builtin_starts_with(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    NcVal *r = nc_bool(nc_text_starts_with_raw(s, p)); free(s); free(p);
    return r;
}
static NcVal *nc_builtin_ends_with(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    NcVal *r = nc_bool(nc_text_ends_with_raw(s, p)); free(s); free(p);
    return r;
}
static NcVal *nc_builtin_contains(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    NcVal *r = nc_bool(nc_text_contains_raw(s, p)); free(s); free(p);
    return r;
}
static NcVal *nc_builtin_split(NcVal *s_v, NcVal *sep_v) {
    char *s = nc_to_str_raw(s_v), *sep = nc_to_str_raw(sep_v);
    NcVal *lst = nc_text_split_to_list(s, sep);
    free(s); free(sep);
    return lst;
}
static NcVal *nc_builtin_join(NcVal *lst, NcVal *sep_v) {
    if (!lst || lst->type != NC_LIST) return nc_str("");
    char *sep = nc_to_str_raw(sep_v);
    NcVal *r = nc_str_own(nc_text_join_list(lst, sep));
    free(sep);
    return r;
}
static NcVal *nc_builtin_trim(NcVal *s_v) {
    char *s = nc_to_str_raw(s_v);
    int a = 0, b = (int)strlen(s);
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    char *rtext = malloc(b - a + 1);
    memcpy(rtext, s + a, b - a);
    rtext[b - a] = 0;
    NcVal *r = nc_str_own(rtext);
    free(s); return r;
}
static NcVal *nc_builtin_replace(NcVal *s_v, NcVal *old_v, NcVal *new_v) {
    char *s = nc_to_str_raw(s_v), *old = nc_to_str_raw(old_v), *newstr = nc_to_str_raw(new_v);
    NcVal *r = nc_str_own(nc_text_replace_copy(s, old, newstr));
    free(s); free(old); free(newstr);
    return r;
}
static NcVal *nc_builtin_chr(NcVal *v) {
    int code = v && v->type == NC_INT ? (int)v->i : 0;
    char buf[2] = {(char)(unsigned char)code, 0};
    return nc_str(buf);
}
static NcVal *nc_builtin_char_code(NcVal *v) {
    if (!v || v->type != NC_STR || !v->s || !v->s[0]) return nc_int(0);
    return nc_int((unsigned char)v->s[0]);
}
static NcVal *nc_builtin_tekst_fra_heltall(NcVal *v) {
    char buf[32];
    if (v->type == NC_INT) snprintf(buf, sizeof(buf), "%lld", v->i);
    else snprintf(buf, sizeof(buf), "%s", nc_to_str_raw(v));
    return nc_str(buf);
}
static NcVal *nc_builtin_tekst(NcVal *v) {
    char *s = nc_to_str_raw(v);
    NcVal *r = nc_str(s);
    free(s);
    return r;
}
static NcVal *nc_builtin_heltall(NcVal *v) {
    if (v->type == NC_INT) return v;
    if (v->type == NC_STR) return nc_int(strtoll(v->s, NULL, 0));
    if (v->type == NC_BOOL) return nc_int(v->b);
    if (v->type == NC_FLOAT) return nc_int((long long)v->f);
    return nc_int(0);
}
static NcVal *nc_json_stringify_list(NcVal *v, int smart);
static NcVal *nc_json_stringify_map(NcVal *v, int smart);
static char *nc_json_escape_string_copy(const char *s);
static int nc_json_looks_like_nonstring(const char *s);
static NcVal *nc_json_stringify_primitive(NcVal *v) {
    if (!v || v->type == NC_NIL) return nc_str("null");
    if (v->type == NC_BOOL) return nc_str(v->b ? "true" : "false");
    if (v->type == NC_INT) { char buf[32]; snprintf(buf,sizeof(buf),"%lld",v->i); return nc_str(buf); }
    if (v->type == NC_FLOAT) { char buf[64]; snprintf(buf,sizeof(buf),"%.15g",v->f); return nc_str(buf); }
    return NULL;
}
static NcVal *nc_json_stringify_compound(NcVal *v, int smart) {
    if (v->type == NC_LIST) {
        return nc_json_stringify_list(v, smart);
    }
    if (v->type == NC_MAP) {
        return nc_json_stringify_map(v, smart);
    }
    return nc_str("null");
}
static NcVal *nc_json_stringify_string(NcVal *v, int smart) {
    if (smart && nc_json_looks_like_nonstring(v->s)) return nc_str(v->s);
    return nc_str_own(nc_json_escape_string_copy(v->s));
}
static NcVal *nc_json_stringify_any(NcVal *v, int smart) {
    NcVal *prim = nc_json_stringify_primitive(v);
    if (prim) return prim;
    if (v->type == NC_STR) {
        return nc_json_stringify_string(v, smart);
    }
    return nc_json_stringify_compound(v, smart);
}

/* ── Hjelpelag: diverse convenience-builtins ── */
static NcVal *nc_builtin_bool(NcVal *v) { return nc_bool(nc_truthy(v)); }
static NcVal *nc_builtin_assert(NcVal *v) {
    if (!nc_truthy(v)) nc_throw("assert feilet");
    return nc_nil();
}
static NcVal *nc_builtin_assert_eq(NcVal *a, NcVal *b) {
    if (!nc_eq(a, b)) {
        char *av = nc_to_str_raw(a), *bv = nc_to_str_raw(b);
        char message[4096];
        snprintf(message, sizeof(message), "assert_eq feilet: %s != %s", av, bv);
        free(av); free(bv);
        nc_throw(message);
    }
    return nc_nil();
}
static NcVal *nc_builtin_bytes_new(NcVal *size_v, NcVal *fill_v) {
    long long size = size_v && size_v->type == NC_INT ? size_v->i : -1;
    long long fill = fill_v && fill_v->type == NC_INT ? fill_v->i : 0;
    if (size < 0 || size > 1073741824LL) nc_throw("bytes-storleik utanfor");
    if (fill < 0 || fill > 255) nc_throw("bytes-fyll må vere 0..255");
    return nc_bytes_new((size_t)size, (unsigned char)fill);
}
static NcVal *nc_builtin_bytes_from_list(NcVal *list_v) {
    if (!list_v || list_v->type != NC_LIST) nc_throw("bytes_from_list krev liste");
    NcVal *out = nc_bytes_new((size_t)list_v->list->len, 0);
    for (int i = 0; i < list_v->list->len; i++) {
        NcVal *item = list_v->list->items[i];
        if (!item || item->type != NC_INT || item->i < 0 || item->i > 255) nc_throw("bytes-verdi må vere 0..255");
        out->bytes->data[i] = (unsigned char)item->i;
    }
    return out;
}
static NcVal *nc_builtin_bytes_to_list(NcVal *bytes_v) {
    if (!bytes_v || bytes_v->type != NC_BYTES) nc_throw("bytes_to_list krev bytes");
    NcVal *out = nc_list_new();
    for (size_t i = 0; i < bytes_v->bytes->len; i++) nc_builtin_legg_til(out, nc_int(bytes_v->bytes->data[i]));
    return out;
}
static NcVal *nc_builtin_bytes_to_text(NcVal *bytes_v) {
    if (!bytes_v || bytes_v->type != NC_BYTES) nc_throw("bytes_to_text krev bytes");
    char *text = (char *)malloc(bytes_v->bytes->len + 1);
    if (!text) nc_throw("bytes_to_text tom for minne");
    memcpy(text, bytes_v->bytes->data, bytes_v->bytes->len);
    text[bytes_v->bytes->len] = '\0';
    NcVal *out = nc_str(text);
    free(text);
    return out;
}
// Fast, bounds-checked conversion for binary-backed text payloads. Unlike a
// whole-buffer conversion this can select the JSON slice after a Mach-O/ELF
// prefix without ever copying embedded NUL bytes.
static NcVal *nc_builtin_bytes_slice_to_text(NcVal *bytes_v, NcVal *start_v, NcVal *len_v) {
    if (!bytes_v || bytes_v->type != NC_BYTES) nc_throw("bytes_slice_to_text krev bytes");
    long long start = start_v && start_v->type == NC_INT ? start_v->i : -1;
    long long len = len_v && len_v->type == NC_INT ? len_v->i : -1;
    if (start < 0 || len < 0 || (size_t)start > bytes_v->bytes->len || (size_t)len > bytes_v->bytes->len - (size_t)start) {
        nc_throw("bytes_slice_to_text utanfor buffer");
    }
    char *text = (char *)malloc((size_t)len + 1);
    if (!text) nc_throw("bytes_slice_to_text tom for minne");
    memcpy(text, bytes_v->bytes->data + (size_t)start, (size_t)len);
    text[len] = '\0';
    NcVal *out = nc_str(text);
    free(text);
    return out;
}
static NcVal *nc_builtin_feil(NcVal *v) { char *s = nc_to_str_raw(v); nc_throw(s); free(s); return nc_nil(); }
static NcVal *nc_builtin_n(NcVal *v) { return v ? v : nc_nil(); }
static NcVal *nc_builtin_desimaltall(NcVal *v) {
    if (!v) return nc_float(0.0);
    if (v->type == NC_FLOAT) return v;
    if (v->type == NC_INT) return nc_float((double)v->i);
    if (v->type == NC_BOOL) return nc_float(v->b ? 1.0 : 0.0);
    if (v->type == NC_STR) { char *end = NULL; double value = strtod(v->s, &end); return nc_float(end && *end == '\0' ? value : 0.0); }
    return nc_float(0.0);
}
static NcVal *nc_builtin_index_of(NcVal *s_v, NcVal *p_v) {
    char *s = nc_to_str_raw(s_v), *p = nc_to_str_raw(p_v);
    NcVal *r = nc_int(nc_text_index_of_raw(s, p));
    free(s); free(p); return r;
}
static NcVal *nc_builtin_lower(NcVal *v) {
    char *s = nc_to_str_raw(v);
    char *rtext = strdup(s);
    for (int i = 0; rtext[i]; i++) rtext[i] = tolower((unsigned char)rtext[i]);
    NcVal *r = nc_str_own(rtext);
    free(s); return r;
}
static NcVal *nc_builtin_upper(NcVal *v) {
    char *s = nc_to_str_raw(v);
    char *rtext = strdup(s);
    for (int i = 0; rtext[i]; i++) rtext[i] = toupper((unsigned char)rtext[i]);
    NcVal *r = nc_str_own(rtext);
    free(s); return r;
}
static NcVal *nc_builtin_type(NcVal *v) {
    if (!v || v->type==NC_NIL) return nc_str("ingenting");
    if (v->type==NC_INT)  return nc_str("heltall");
    if (v->type==NC_FLOAT) return nc_str("desimaltall");
    if (v->type==NC_STR)  return nc_str("tekst");
    if (v->type==NC_BOOL) return nc_str("boolsk");
    if (v->type==NC_LIST) return nc_str("liste");
    if (v->type==NC_MAP)  return nc_str("ordbok");
    if (v->type==NC_BYTES) return nc_str("bytes");
    return nc_str("ukjent");
}
static NcVal *nc_builtin_fjern_nokkel(NcVal *m, NcVal *k_v) {
    if (!m || m->type!=NC_MAP) return nc_nil();
    char *k = nc_to_str_raw(k_v);
    pthread_mutex_lock(&m->map->mutex);
    int i = nc_coll_find_key_index(m, k);
    if (i >= 0) {
        free(m->map->keys[i]);
        memmove(&m->map->keys[i],&m->map->keys[i+1],(m->map->len-i-1)*sizeof(char*));
        memmove(&m->map->vals[i],&m->map->vals[i+1],(m->map->len-i-1)*sizeof(NcVal*));
        memmove(&m->map->hashes[i],&m->map->hashes[i+1],(m->map->len-i-1)*sizeof(uint64_t));
        m->map->len--;
        nc_map_rebuild_index_locked(m->map, m->map->len);
        pthread_mutex_unlock(&m->map->mutex);
        free(k); return nc_nil();
    }
    pthread_mutex_unlock(&m->map->mutex);
    free(k); return nc_nil();
}
static NcVal *nc_builtin_fil_append(NcVal *path_v, NcVal *data_v) {
    char *path = nc_to_str_raw(path_v), *data = nc_to_str_raw(data_v);
    NcVal *r = nc_file_append_text(path, data);
    free(path);
    free(data); return r;
}
static NcVal *nc_builtin_fil_append_binary(NcVal *path_v, NcVal *data_v) {
    char *path = nc_to_str_raw(path_v);
    NcVal *r = nc_host_file_write_result(nc_host_append_binary_path(path, data_v), "append til", path);
    free(path); return r;
}
static NcVal *nc_builtin_exit(NcVal *code_v) {
    exit(code_v && code_v->type==NC_INT ? (int)code_v->i : 0);
    return nc_nil();
}

/* ── Hjelpelag: JSON-parsing/-stringify-lag ── */
/* Enkel JSON-parser for direkte parse-entrypoint. */
typedef struct { const char *p; } JP2;
static NcVal *jp2_parse(JP2 *j);
static NcVal *nc_json_parse_string(JP2 *j);
static NcVal *nc_json_parse_object(JP2 *j);
static NcVal *nc_json_parse_array(JP2 *j);
static NcVal *nc_json_parse_true(JP2 *j);
static NcVal *nc_json_parse_false(JP2 *j);
static NcVal *nc_json_parse_null(JP2 *j);
static NcVal *nc_json_parse_number(JP2 *j);
static NcVal *nc_json_parse_unknown(JP2 *j);
static NcVal *nc_json_parse_primitive(JP2 *j);
static int nc_json_is_token_boundary(char c);
static int nc_json_consume_keyword(JP2 *j, const char *kw, int len);
static void nc_json_unescape_append(char *buf, size_t *bi, char c) {
    switch (c) {
        case '"': buf[(*bi)++] = '"'; break;
        case '\\': buf[(*bi)++] = '\\'; break;
        case 'n': buf[(*bi)++] = '\n'; break;
        case 'r': buf[(*bi)++] = '\r'; break;
        case 't': buf[(*bi)++] = '\t'; break;
        default:
            buf[(*bi)++] = '\\';
            buf[(*bi)++] = c;
            break;
    }
}
static void nc_json_skip_ws(JP2 *j) { while (*j->p==' '||*j->p=='\t'||*j->p=='\n'||*j->p=='\r') j->p++; }
static char *jp2_str(JP2 *j) {
    j->p++;
    /* brukar heapbuffer for store JSON-strengar */
    size_t cap = 4096, bi = 0;
    char *buf = malloc(cap);
    while (*j->p && *j->p!='"') {
        if (bi + 4 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        if (*j->p=='\\') { j->p++;
            nc_json_unescape_append(buf, &bi, *j->p);
        } else buf[bi++]=*j->p;
        j->p++;
    }
    if (*j->p == '"') {
        j->p++;
    } else {
        free(buf);
        return NULL;
    }
    buf[bi]=0;
    char *result = strdup(buf);
    free(buf);
    return result;
}
static NcVal *nc_json_parse_from_ncval(NcVal *v) {
    char *s = nc_to_str_raw(v);
    JP2 j = {s};
    NcVal *r = jp2_parse(&j);
    nc_json_skip_ws(&j);
    if (r && *j.p != '\0') r = nc_nil();
    free(s);
    return r ? r : nc_nil();
}
static NcVal *nc_json_parse_object(JP2 *j) {
    j->p++;
    NcVal *m = nc_map_new();
    nc_json_skip_ws(j);
    if (*j->p == '}') { j->p++; return m; }
    while (*j->p) {
        if (*j->p == ',') { return nc_nil(); }
        if (*j->p != '"') { return nc_nil(); }
        char *k = jp2_str(j);
        if (!k) { return nc_nil(); }
        nc_json_skip_ws(j);
        if (*j->p != ':') { free(k); return nc_nil(); }
        j->p++;
        nc_json_skip_ws(j);
        NcVal *v = jp2_parse(j);
        if (!v) { free(k); return nc_nil(); }
        nc_json_skip_ws(j);
        nc_index_set(m, nc_str_own(k), v);
        if (*j->p == ',') {
            j->p++;
            nc_json_skip_ws(j);
            if (*j->p == '}' || *j->p == ',') return nc_nil();
            if (*j->p == '\0') return nc_nil();
            continue;
        }
        if (*j->p == '}') { j->p++; return m; }
        return nc_nil();
    }
    return nc_nil();
}
static NcVal *nc_json_parse_array(JP2 *j) {
    j->p++;
    NcVal *lst = nc_list_new();
    nc_json_skip_ws(j);
    if (*j->p == ']') { j->p++; return lst; }
    while (*j->p) {
        if (*j->p == ',') { return nc_nil(); }
        nc_builtin_legg_til(lst, jp2_parse(j));
        nc_json_skip_ws(j);
        if (*j->p == ',') {
            j->p++;
            nc_json_skip_ws(j);
            if (*j->p == ']' || *j->p == ',') return nc_nil();
            continue;
        }
        if (*j->p == ']') { j->p++; return lst; }
        return nc_nil();
    }
    return nc_nil();
}
static NcVal *nc_json_parse_string(JP2 *j) {
    char *s = jp2_str(j);
    if (!s) return nc_nil();
    return nc_str_own(s);
}
static NcVal *nc_json_parse_true(JP2 *j) {
    (void)j;
    return nc_bool(1);
}
static NcVal *nc_json_parse_false(JP2 *j) {
    (void)j;
    return nc_bool(0);
}
static NcVal *nc_json_parse_null(JP2 *j) {
    (void)j;
    return nc_nil();
}
static int nc_json_is_token_boundary(char c) {
    return c == '\0' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',' || c == '}' || c == ']' || c == ':';
}
static int nc_json_consume_keyword(JP2 *j, const char *kw, int len) {
    if (strncmp(j->p, kw, len) != 0) return 0;
    if (!nc_json_is_token_boundary(j->p[len])) return 0;
    j->p += len;
    return 1;
}
static NcVal *nc_json_parse_number(JP2 *j) {
    const char *start = j->p;
    const char *cursor = start;
    if (*cursor == '-') cursor++;
    if (*cursor == '+') return NULL;
    if (!isdigit((unsigned char)*cursor)) return NULL;
    // Norscode-kjelde kan ha 0xNN-literalar i eldre bundle-cache. Dei må
    // normaliserast til eit ekte heiltal før JSON-serialisering; elles kan
    // ein multi-modul bundle ende med ugyldig JSON-token som 0x7F.
    if (*cursor == '0' && (cursor[1] == 'x' || cursor[1] == 'X')) {
        const char *hex = cursor + 2;
        if (!isxdigit((unsigned char)*hex)) return NULL;
        while (isxdigit((unsigned char)*hex)) hex++;
        if (!nc_json_is_token_boundary(*hex)) return NULL;
        errno = 0; unsigned long long raw = strtoull(cursor + 2, NULL, 16);
        if (errno == ERANGE || raw > (unsigned long long)LLONG_MAX) return NULL;
        j->p = hex;
        return nc_int((long long)raw);
    }
    if (*cursor == '0' && isdigit((unsigned char)cursor[1])) return NULL;

    int floating = 0;
    if (*cursor == '0') cursor++; else while (isdigit((unsigned char)*cursor)) cursor++;
    if (*cursor == '.') {
        floating = 1; cursor++;
        if (!isdigit((unsigned char)*cursor)) return NULL;
        while (isdigit((unsigned char)*cursor)) cursor++;
    }
    if (*cursor == 'e' || *cursor == 'E') {
        floating = 1; cursor++;
        if (*cursor == '+' || *cursor == '-') cursor++;
        if (!isdigit((unsigned char)*cursor)) return NULL;
        while (isdigit((unsigned char)*cursor)) cursor++;
    }
    if (!nc_json_is_token_boundary(*cursor)) return NULL;
    char *end = NULL;
    if (floating) {
        errno = 0; double value = strtod(start, &end);
        if (end != cursor || errno == ERANGE) return NULL;
        j->p = cursor; return nc_float(value);
    }
    errno = 0; long long value = strtoll(start, &end, 10);
    if (end != cursor || errno == ERANGE) return NULL;
    j->p = cursor; return nc_int(value);
}
static NcVal *nc_json_parse_unknown(JP2 *j) {
    while (*j->p && !nc_json_is_token_boundary(*j->p)) {
        j->p++;
    }
    return nc_nil();
}
static NcVal *nc_json_parse_primitive(JP2 *j) {
    if (nc_json_consume_keyword(j, "true", 4)) return nc_json_parse_true(j);
    if (nc_json_consume_keyword(j, "false", 5)) return nc_json_parse_false(j);
    if (nc_json_consume_keyword(j, "null", 4)) return nc_json_parse_null(j);
    NcVal *number = nc_json_parse_number(j);
    if (number) return number;
    return nc_json_parse_unknown(j);
}
static NcVal *jp2_parse(JP2 *j) {
    nc_json_skip_ws(j);
    if (*j->p=='"') return nc_json_parse_string(j);
    if (*j->p=='{') return nc_json_parse_object(j);
    if (*j->p=='[') return nc_json_parse_array(j);
    return nc_json_parse_primitive(j);
}
static NcVal *nc_to_str(NcVal *v) { return nc_str_own(nc_to_str_raw(v)); }
/* Host-/generated hook: levert utanfrå denne runtimefila. */
NcVal *nc_fn_builtin_neste_token(NcVal **args, int nargs);
static NcVal *nc_builtin_jit_operation(NcVal *request);

/* Smart JSON-stringify for verdiar som allereie kan likne rå JSON. */
static int nc_json_looks_like_nonstring(const char *s) {
    if (!s || !*s) return 0;
    if (!strcmp(s,"true")||!strcmp(s,"false")||!strcmp(s,"null")) return 1;
    size_t sl = strlen(s);
    if (s[0]=='{' && s[sl-1]=='}' && sl>=2) return 1;
    if (s[0]=='[' && s[sl-1]==']' && sl>=2) return 1;
    char *end; strtoll(s,&end,10);
    if (*end==0 && end!=s) return 1;
    return 0;
}
static void nc_json_append(char **buf, size_t *length, size_t *capacity,
                            const char *part, size_t part_length) {
    size_t need = *length + part_length + 1;
    if (need > *capacity) {
        size_t next = *capacity ? *capacity : 64;
        while (next < need) next *= 2;
        char *grown = realloc(*buf, next);
        if (!grown) abort();
        *buf = grown;
        *capacity = next;
    }
    memcpy(*buf + *length, part, part_length);
    *length += part_length;
    (*buf)[*length] = '\0';
}

static void nc_json_append_cstr(char **buf, size_t *length, size_t *capacity,
                                const char *part) {
    nc_json_append(buf, length, capacity, part, strlen(part));
}

static NcVal *nc_json_stringify_list(NcVal *v, int smart) {
    size_t length = 0, capacity = 64;
    char *r = malloc(capacity); if (!r) abort(); r[0] = '\0';
    nc_json_append_cstr(&r, &length, &capacity, "[");
    int first = 1;
    for (int i = 0; i < v->list->len; i++) {
        NcVal *item_json = smart
            ? nc_json_stringify_any(v->list->items[i], 1)
            : nc_json_stringify_any(v->list->items[i], 0);
        if (!first) nc_json_append_cstr(&r, &length, &capacity, ",");
        nc_json_append_cstr(&r, &length, &capacity, item_json->s); first = 0;
    }
    nc_json_append_cstr(&r, &length, &capacity, "]");
    return nc_str_own(r);
}
static int nc_json_map_is_dense_array_keys(NcVal *v) {
    if (!v || v->type != NC_MAP) return 0;
    for (int i = 0; i < v->map->len; i++) {
        char keybuf[32];
        snprintf(keybuf, sizeof(keybuf), "%d", i);
        if (strcmp(v->map->keys[i], keybuf) != 0) return 0;
    }
    return 1;
}
static NcVal *nc_json_stringify_map(NcVal *v, int smart) {
    int arrayish = nc_json_map_is_dense_array_keys(v);
    size_t length = 0, capacity = 64;
    char *r = malloc(capacity); if (!r) abort(); r[0] = '\0';
    nc_json_append_cstr(&r, &length, &capacity, "{");
    int first = 1;
    for (int i = 0; i < v->map->len; i++) {
        NcVal *kj = nc_json_stringify_any(nc_str(v->map->keys[i]), 0);
        int value_smart = smart;
        if (arrayish && v->map->vals[i] && v->map->vals[i]->type == NC_STR) {
            value_smart = 0;
        }
        NcVal *vj = value_smart
            ? nc_json_stringify_any(v->map->vals[i], 1)
            : nc_json_stringify_any(v->map->vals[i], 0);
        if (!first) nc_json_append_cstr(&r, &length, &capacity, ",");
        nc_json_append_cstr(&r, &length, &capacity, kj->s);
        nc_json_append_cstr(&r, &length, &capacity, ":");
        nc_json_append_cstr(&r, &length, &capacity, vj->s); first = 0;
    }
    nc_json_append_cstr(&r, &length, &capacity, "}");
    return nc_str_own(r);
}
static char *nc_json_escape_string_copy(const char *s) {
    size_t len = strlen(s);
    char *r = malloc(len * 2 + 3);
    char *wp = r;
    *wp++ = '"';
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"') { *wp++='\\'; *wp++='"'; }
        else if (c == '\\') { *wp++='\\'; *wp++='\\'; }
        else if (c == '\n') { *wp++='\\'; *wp++='n'; }
        else if (c == '\r') { *wp++='\\'; *wp++='r'; }
        else if (c == '\t') { *wp++='\\'; *wp++='t'; }
        else *wp++ = c;
    }
    *wp++ = '"';
    *wp = 0;
    return r;
}
static void nc_json_store_stringified_value(NcVal *m, NcVal *key, NcVal *v) {
    if (v && v->type == NC_FLOAT) {
        nc_index_set(m, key, v);
        return;
    }
    nc_index_set(m, key, (v && v->type != NC_STR) ? nc_json_stringify_any(v, 1) : v);
}
static void nc_json_stringify_map_values_in_place(NcVal *m) {
    for (int i=0; i<m->map->len; i++) {
        nc_json_store_stringified_value(m, nc_str(m->map->keys[i]), m->map->vals[i]);
    }
}
static NcVal *nc_json_stringify_list_as_map(NcVal *lst) {
    NcVal *m = nc_map_new();
    for (int i=0; i<lst->list->len; i++) {
        char keybuf[32]; snprintf(keybuf, sizeof(keybuf), "%d", i);
        nc_json_store_stringified_value(m, nc_str(keybuf), lst->list->items[i]);
    }
    return m;
}
/* Norscode-variant av json.parse med tekstifisering av ikkje-streng-verdiar. */
static NcVal *nc_builtin_json_parse_norscode(NcVal *v) {
    NcVal *r = nc_json_parse_from_ncval(v);
    /* MAP: stringify non-string values */
    if (r->type == NC_MAP) {
        nc_json_stringify_map_values_in_place(r);
        return r;
    }
    /* LIST: konverter til string-keyed map */
    if (r->type == NC_LIST) {
        return nc_json_stringify_list_as_map(r);
    }
    /* Scalars keep std.json semantics: non-string values become their text form. */
    if (r->type != NC_STR) {
        return nc_json_stringify_any(r, 1);
    }
    return r;
}
static NcVal *nc_builtin_json_parse_str(NcVal *v) {
    return nc_json_parse_from_ncval(v);
}
static NcVal *nc_builtin_json_parse_raw(NcVal *v) {
    return nc_json_parse_from_ncval(v);
}
static NcVal *nc_builtin_json_stringify(NcVal *v) {
    return nc_json_stringify_any(v, 0);
}
static NcVal *nc_builtin_json_stringify_smart(NcVal *v) {
    return nc_json_stringify_any(v, 1);
}

/* ─── Sti-hjelpere ─────────────────────────────────────────────────────────── */
static char *nc_path_join_copy(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *r = malloc(la + lb + 2);
    strcpy(r, a);
    if (la > 0 && a[la-1] != '/' && lb > 0) strcat(r, "/");
    strcat(r, b);
    return r;
}
static char *nc_path_basename_ptr(char *path) {
    char *last = strrchr(path, '/');
    return last ? last + 1 : path;
}
static char *nc_path_dirname_copy(char *path) {
    char *last = strrchr(path, '/');
    if (!last) return strdup(".");
    if (last == path) return strdup("/");
    *last = 0;
    return strdup(path);
}
static int nc_path_exists_raw(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}
static int nc_path_mkdir_p_raw(const char *path) {
    if (!path || !*path) return 0;
    char *tmp = strdup(path);
    if (!tmp) return 0;
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
        len--;
    }
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0777) != 0 && errno != EEXIST) {
                free(tmp);
                return 0;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0777) != 0 && errno != EEXIST) {
        free(tmp);
        return 0;
    }
    free(tmp);
    return 1;
}
static char *nc_path_stem_copy(char *path) {
    char *base = nc_path_basename_ptr(path);
    char *dot = strrchr(base, '.');
    if (dot && dot != base) {
        char tmp = *dot;
        *dot = 0;
        char *r = strdup(base);
        *dot = tmp;
        return r;
    }
    return strdup(base);
}
static NcVal *nc_builtin_sti_join(NcVal *a, NcVal *b) {
    char *sa = nc_to_str_raw(a), *sb = nc_to_str_raw(b);
    char *r = nc_path_join_copy(sa, sb);
    free(sa); free(sb);
    return nc_str_own(r);
}
static NcVal *nc_builtin_sti_basename(NcVal *v) {
    char *s = nc_to_str_raw(v);
    NcVal *r = nc_str(nc_path_basename_ptr(s));
    free(s); return r;
}
static NcVal *nc_builtin_sti_dirname(NcVal *v) {
    char *s = nc_to_str_raw(v);
    NcVal *r = nc_str_own(nc_path_dirname_copy(s));
    free(s); return r;
}
static NcVal *nc_builtin_sti_exists(NcVal *v) {
    char *s = nc_to_str_raw(v);
    int ok = nc_path_exists_raw(s);
    free(s); return nc_bool(ok);
}
static NcVal *nc_builtin_mkdir_p(NcVal *v) {
    char *s = nc_to_str_raw(v);
    int ok = nc_path_mkdir_p_raw(s);
    free(s);
    if (!ok) nc_panic("Kan ikkje opprette mappe");
    return nc_bool(1);
}
static NcVal *nc_builtin_sti_stem(NcVal *v) {
    char *s = nc_to_str_raw(v);
    NcVal *r = nc_str_own(nc_path_stem_copy(s));
    free(s); return r;
}

/* ─── Hjelpelag: sti- og miljøoperasjonar ─────────────────────────────────── */
static NcVal *nc_builtin_miljo_sett(NcVal *key, NcVal *val) {
    char *k = nc_to_str_raw(key), *v = nc_to_str_raw(val);
    NcVal *r = nc_env_set_text(k, v);
    free(k); free(v); return r;
}

/* Små compat-restar frå eldre generated namn. */
/* ─── Compat-wrapperar for eldre builtin-symbol ─────────────────────────────── */
/* Desse symbola kan liggje i eldre bootstrap-NCB-ar som eksplisitte
 * `builtin.*`-kall. Dei må finnast i den sjølvstendige C-runtimen sjølv om
 * den aktuelle plattforma ikkje eksporterer ein separat host-dispatch. */
NcVal *nc_fn_builtin_native_mkdir_p(NcVal **a, int na) {
    return nc_builtin_mkdir_p(na > 0 ? a[0] : nc_nil());
}
NcVal *nc_fn_builtin_tensor_operation(NcVal **a, int na) {
    (void)a; (void)na;
    return nc_nil();
}
NcVal *nc_fn_builtin_heiltall(NcVal **a, int na) {
    return nc_builtin_heltall(na > 0 ? a[0] : nc_nil());
}
NcVal *nc_fn_builtin_tekst_erstatt(NcVal **a, int na) { return nc_builtin_replace(na>0?a[0]:nc_nil(),na>1?a[1]:nc_nil(),na>2?a[2]:nc_nil()); }
NcVal *nc_fn_builtin_tekst_starter_med(NcVal **a, int na) { return nc_builtin_starts_with(na>0?a[0]:nc_nil(),na>1?a[1]:nc_nil()); }
NcVal *nc_fn_builtin_tekst_til_liten(NcVal **a, int na) { return nc_builtin_lower(na>0?a[0]:nc_nil()); }
NcVal *nc_fn_builtin_tekst_til_store(NcVal **a, int na) { return nc_builtin_upper(na>0?a[0]:nc_nil()); }
NcVal *nc_fn_builtin_web_request_query_param(NcVal **a, int na) {
    if (na < 2) return nc_str("");
    NcVal *query = nc_index_get(a[0], nc_str("query"));
    if (!query || query->type == NC_NIL) return nc_str("");
    NcVal *v = nc_index_get(query, a[1]);
    if (!v || v->type == NC_NIL) return nc_str("");
    return v;
}
NcVal *nc_fn_builtin_web_request_header(NcVal **a, int na) {
    if (na < 2) return nc_str("");
    NcVal *headers = nc_index_get(a[0], nc_str("headers"));
    if (!headers || headers->type == NC_NIL) return nc_str("");
    NcVal *v = nc_index_get(headers, a[1]);
    if (!v || v->type == NC_NIL) return nc_str("");
    return v;
}
NcVal *nc_fn_builtin_web_request_json(NcVal **a, int na) {
    if (na < 1) return nc_map_new();
    NcVal *body = nc_index_get(a[0], nc_str("body"));
    if (!body || body->type == NC_NIL) return nc_map_new();
    return nc_builtin_json_parse_str(body);
}
NcVal *nc_fn_builtin_web_request_json_field(NcVal **a, int na) {
    if (na < 2) return nc_str("");
    NcVal *json_args[1] = { a[0] };
    NcVal *obj = nc_fn_builtin_web_request_json(json_args, 1);
    NcVal *v = nc_index_get(obj, a[1]);
    if (!v || v->type == NC_NIL) return nc_str("");
    return v;
}
NcVal *nc_fn_builtin_web_request_cookie(NcVal **a, int na) {
    if (na < 2) return nc_str("");
    NcVal *header_args[2] = { a[0], nc_str("Cookie") };
    NcVal *cookie = nc_fn_builtin_web_request_header(header_args, 2);
    char *raw = nc_to_str_raw(cookie);
    char *key = nc_to_str_raw(a[1]);
    size_t key_len = strlen(key);
    NcVal *out = nc_str("");
    char *p = raw;
    while (p && *p) {
        while (*p == ' ' || *p == ';') p++;
        if (!strncmp(p, key, key_len) && p[key_len] == '=') {
            char *start = p + key_len + 1;
            char *end = strchr(start, ';');
            size_t len = end ? (size_t)(end - start) : strlen(start);
            char *buf = malloc(len + 1);
            memcpy(buf, start, len);
            buf[len] = 0;
            out = nc_str_own(buf);
            break;
        }
        p = strchr(p, ';');
        if (p) p++;
    }
    free(raw); free(key);
    return out;
}
NcVal *nc_fn_builtin_web_auth_header(NcVal **a, int na) {
    if (na < 1) return nc_str("");
    NcVal *header_args[2] = { a[0], nc_str("Authorization") };
    return nc_fn_builtin_web_request_header(header_args, 2);
}
NcVal *nc_fn_builtin_web_bearer_token(NcVal **a, int na) {
    NcVal *auth = nc_fn_builtin_web_auth_header(a, na);
    char *raw = nc_to_str_raw(auth);
    const char *prefix = "Bearer ";
    NcVal *out = nc_str("");
    if (!strncmp(raw, prefix, strlen(prefix))) out = nc_str(raw + strlen(prefix));
    free(raw);
    return out;
}
NcVal *nc_fn_builtin_tekst_til_heltall(NcVal **a, int na) { return nc_builtin_heltall(na>0?a[0]:nc_nil()); }
NcVal *nc_fn_builtin_web_response_body(NcVal **a, int na) {
    if (na < 1) return nc_str("");
    NcVal *v = nc_index_get(a[0], nc_str("__body__"));
    if (v && v->type != NC_NIL) return v;
    v = nc_index_get(a[0], nc_str("body"));
    if (v && v->type != NC_NIL) return v;
    return nc_str("");
}
NcVal *nc_fn_builtin_web_response_builder(NcVal **a, int na) {
    NcVal *m = nc_map_new();
    nc_index_set(m, nc_str("status"), na>0?a[0]:nc_int(200));
    nc_index_set(m, nc_str("headers"), na>1?a[1]:nc_map_new());
    nc_index_set(m, nc_str("body"), na>2?a[2]:nc_str(""));
    return m;
}
NcVal *nc_fn_builtin_web_response_error(NcVal **a, int na) {
    NcVal *headers = nc_map_new();
    nc_index_set(headers, nc_str("content-type"), nc_str("text/plain; charset=utf-8"));
    NcVal *args[3] = { na>0?a[0]:nc_int(500), headers, na>1?a[1]:nc_str("Feil") };
    return nc_fn_builtin_web_response_builder(args, 3);
}
NcVal *nc_fn_builtin_web_has_role(NcVal **a, int na) {
    if (na < 2) return nc_bool(0);
    NcVal *roles = nc_index_get(a[0], nc_str("roles"));
    if (!roles || roles->type == NC_NIL) return nc_bool(0);
    char *wanted = nc_to_str_raw(a[1]);
    int ok = 0;
    if (roles->type == NC_LIST) {
        for (int i = 0; i < roles->list->len; i++) {
            char *r = nc_to_str_raw(roles->list->items[i]);
            if (!strcmp(r, wanted)) ok = 1;
            free(r);
            if (ok) break;
        }
    } else {
        char *r = nc_to_str_raw(roles);
        ok = strstr(r, wanted) != NULL;
        free(r);
    }
    free(wanted);
    return nc_bool(ok);
}
NcVal *nc_fn_builtin_ncb_route_handlers(NcVal **a, int na) { return nc_builtin_ncb_route_handlers(a, na); }
NcVal *nc_fn_builtin_ncb_metadata(NcVal **a, int na) { return nc_builtin_ncb_metadata(a, na); }
NcVal *nc_fn_builtin_ncb_next_request_id(NcVal **a, int na) { return nc_builtin_ncb_next_request_id(a, na); }
NcVal *nc_fn_builtin_ncb_call_fn(NcVal **a, int na) { return nc_builtin_ncb_call_fn(a, na); }
NcVal *nc_fn_builtin_fil_slett(NcVal **a, int na) {
    if (na < 1) return nc_bool(0);
    char *p = nc_to_str_raw(a[0]);
    int rc = remove(p);
    free(p);
    return nc_bool(rc == 0);
}
NcVal *nc_fn_builtin_semantic_analyser(NcVal **a, int na) { return na>0?a[0]:nc_nil(); }
NcVal *nc_fn_builtin_semantic_analyser_rent(NcVal **a, int na) { return na>0?a[0]:nc_nil(); }
NcVal *nc_fn_builtin_semantic_ok(NcVal **a, int na) { (void)a; (void)na; return nc_bool(1); }
NcVal *nc_fn_builtin_semantic_rapport(NcVal **a, int na) { (void)a; (void)na; return nc_str(""); }
NcVal *nc_fn_builtin_semantic_validate_program(NcVal **a, int na) { return na>0?a[0]:nc_nil(); }
NcVal *nc_fn_builtin_semantic_has_errors(NcVal **a, int na) { (void)a; (void)na; return nc_bool(0); }
NcVal *nc_fn_builtin_wasm_selftest(NcVal **a, int na) { (void)a; (void)na; return nc_bool(1); }
NcVal *nc_fn_builtin_sett_inn(NcVal **a, int na) {
    if (na < 3) return nc_nil();
    nc_index_set(a[0], a[1], a[2]);
    return nc_nil();
}
NcVal *nc_fn_builtin_miljo_sett(NcVal **a, int na) {
    return nc_builtin_miljo_sett(na > 0 ? a[0] : nc_nil(), na > 1 ? a[1] : nc_nil());
}
NcVal *nc_fn_builtin__rt_env_finnes(NcVal **a, int na) {
    return nc_builtin_miljo_finnes(na > 0 ? a[0] : nc_nil());
}
