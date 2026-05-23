#include "mv_arena.h"

#include <string.h>

#if MV_USE_ARENA

/* 96 MiB static arena — enough for hello compile (~464k VM steps). */
#define MV_ARENA_BYTES ((size_t)(96ULL * 1024 * 1024))

static unsigned char mv_arena_storage[MV_ARENA_BYTES];
static size_t mv_arena_off;

void mv_arena_reset(void) {
    mv_arena_off = 0;
}

void *mv_arena_alloc(size_t size) {
    if (size == 0) return NULL;
    size_t align = 16;
    size_t pad = (align - (mv_arena_off % align)) % align;
    if (mv_arena_off + pad + size > MV_ARENA_BYTES) return NULL;
    mv_arena_off += pad;
    void *p = mv_arena_storage + mv_arena_off;
    mv_arena_off += size;
    return p;
}

void *mv_arena_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) return NULL;
    size_t total = nmemb * size;
    void *p = mv_arena_alloc(total);
    if (p) memset(p, 0, total);
    return p;
}

#else

void mv_arena_reset(void) {}
void *mv_arena_alloc(size_t size) {
    (void)size;
    return NULL;
}
void *mv_arena_calloc(size_t nmemb, size_t size) {
    (void)nmemb;
    (void)size;
    return NULL;
}

#endif
