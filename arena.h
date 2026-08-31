#ifndef ARENA_H_
#define ARENA_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "kestrel_core.h"


#if defined(_MSC_VER)
#include <malloc.h>
#define KESTREL_ALIGNED_ALLOC(align, size) _aligned_malloc((size), (align))
#define KESTREL_ALIGNED_FREE(ptr) _aligned_free((ptr))

#else
#define KESTREL_ALIGNED_ALLOC(align, size) aligned_alloc((align), (size))
#define KESTREL_ALIGNED_FREE(ptr) free((ptr))
#endif

#if defined(_MSC_VER) && !defined(static_assert)
#define KESTREL_STATIC_ASSERT(c, m) typedef char kestrel_sa_[(c) ? 1 : -1]
#else
#define KESTREL_STATIC_ASSERT(c, m) _Static_assert(c, m)
#endif



typedef struct
{
    uint8_t *data;
    size_t capacity;
    size_t offset;
} arena_t;

typedef size_t arena_mark_t;

bool arena_init(arena_t *arena, size_t requested_capacity);
void arena_destroy(arena_t *arena);
void *arena_alloc(arena_t *arena, size_t size, size_t alignment);
arena_mark_t arena_get_mark(const arena_t *arena);
void arena_pop_to_mark(arena_t *arena, arena_mark_t mark);
void arena_reset(arena_t *arena);

#endif //ARENA_H_
