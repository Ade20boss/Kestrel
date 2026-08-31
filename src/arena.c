#include "arena.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

KESTREL_STATIC_ASSERT((KESTREL_ALIGNMENT & (KESTREL_ALIGNMENT - 1)) == 0, "KESTREL_ALIGNMENT must be a power of two");

bool arena_init(arena_t *arena, size_t requested_capacity)
{

    if (arena == NULL)
    {
        fprintf(stderr, "arena_init: arena pointer is NULL\n");
        return false;
    }
    if (requested_capacity == 0)
    {
        fprintf(stderr, "arena_init: requested capacity is zero\n");
        return false;
    }
    /* Guarantees offset + (alignment - 1) can never overflow in arena_alloc,
       since offset <= capacity <= SIZE_MAX - (KESTREL_ALIGNMENT - 1). */
    if (requested_capacity > SIZE_MAX - (KESTREL_ALIGNMENT - 1))
    {
        fprintf(stderr,
                "arena_init: capacity %zu overflows when aligned to %d\n",
                requested_capacity,
                KESTREL_ALIGNMENT);
        return false;
    }

    size_t aligned_capacity = (requested_capacity + (KESTREL_ALIGNMENT - 1)) & ~(size_t)(KESTREL_ALIGNMENT - 1);

    arena->data = KESTREL_ALIGNED_ALLOC(KESTREL_ALIGNMENT, aligned_capacity);
    if (arena->data == NULL)
    {
        fprintf(stderr,
                "arena_init: failed to allocate %zu bytes aligned to %d\n",
                aligned_capacity,
                KESTREL_ALIGNMENT);
        return false;
    }

    arena->capacity = aligned_capacity;
    arena->offset = 0;
    return true;
}

void arena_destroy(arena_t *arena)
{
    /* Idempotent: destroying a NULL or already-destroyed arena is a no-op. */
    if (arena == NULL || arena->data == NULL)
    {
        return;
    }

    KESTREL_ALIGNED_FREE(arena->data);
    arena->data = NULL;
    arena->capacity = 0;
    arena->offset = 0;
}

void *arena_alloc(arena_t *arena, size_t size, size_t alignment)
{
    if (arena == NULL)
    {
        fprintf(stderr, "arena_alloc: arena pointer is NULL\n");
        return NULL;
    }
    if (arena->data == NULL)
    {
        fprintf(stderr, "arena_alloc: arena is uninitialised or destroyed\n");
        return NULL;
    }
    if (size == 0)
    {
        fprintf(stderr, "arena_alloc: requested size is zero\n");
        return NULL;
    }
    if (alignment == 0)
    {
        fprintf(stderr, "arena_alloc: requested alignment is zero\n");
        return NULL;
    }
    if ((alignment & (alignment - 1)) != 0)
    {
        fprintf(stderr, "arena_alloc: alignment %zu is not a power of two\n", alignment);
        return NULL;
    }
    if (alignment > KESTREL_ALIGNMENT)
    {
        fprintf(stderr, "arena_alloc: alignment %zu exceeds arena base alignment %d\n", alignment, KESTREL_ALIGNMENT);
        return NULL;
    }

    size_t aligned_offset = (arena->offset + (alignment - 1)) & ~(size_t)(alignment - 1);

    if (aligned_offset > arena->capacity || size > arena->capacity - aligned_offset)
    {
        fprintf(stderr,
                "arena_alloc: out of memory (need %zu bytes at offset %zu, capacity %zu)\n",
                size,
                aligned_offset,
                arena->capacity);
        return NULL;
    }

    arena->offset = aligned_offset + size;
    return (void *)(arena->data + aligned_offset);
}


arena_mark_t arena_get_mark(const arena_t *arena)
{
    KESTREL_ASSERT(arena != NULL);
    KESTREL_ASSERT(arena->data != NULL);

    return arena->offset;
}

void arena_pop_to_mark(arena_t *arena, arena_mark_t mark)
{
    KESTREL_ASSERT(arena != NULL);
    KESTREL_ASSERT(arena->data != NULL);
    /* mark <= offset implies mark <= capacity, since offset <= capacity is invariant. */
    KESTREL_ASSERT(mark <= arena->offset);

    arena->offset = mark;
}

void arena_reset(arena_t *arena)
{
    KESTREL_ASSERT(arena != NULL);
    KESTREL_ASSERT(arena->data != NULL);

    arena->offset = 0;
}
