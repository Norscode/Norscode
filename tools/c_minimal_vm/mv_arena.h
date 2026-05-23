/* Bump allocator for freestanding compiler (no libc malloc). */
#ifndef MV_ARENA_H
#define MV_ARENA_H

#include <stddef.h>

#ifdef NORCODE_FREESTANDING
#define MV_USE_ARENA 1
#else
#define MV_USE_ARENA 0
#endif

void mv_arena_reset(void);
void *mv_arena_alloc(size_t size);
void *mv_arena_calloc(size_t nmemb, size_t size);

#endif
