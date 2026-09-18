#ifndef ARENA_H_
#define ARENA_H_

#include "kestrel_core.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


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

/**
 * @struct arena_t
 * @brief Aligned bump allocator with strict lifecycle invariants.
 *
 * INVARIANTS:
 * - ZERO (Non-Live): data == NULL, capacity == 0, offset == 0.
 * - LIVE: data != NULL, capacity > 0, offset <= capacity.
 * Any other combination of state is INVALID and triggers fatal contract violations.
 */
typedef struct
{
    uint8_t *data;
    size_t capacity;
    size_t offset;
} arena_t;

typedef size_t arena_mark_t;

/**
 * @brief Initializes a new arena.
 *
 * PRECONDITION: The caller must explicitly zero-initialize the arena struct
 * before calling this function (establishing the ZERO state). Passing an
 * already-LIVE or INVALID arena is a fatal programming error.
 *
 * FAILURE GUARANTEE: If allocation fails, the arena state remains exactly in
 * the ZERO state. No intermediate states are created.
 */
bool arena_init(arena_t *arena, size_t requested_capacity);

/**
 * @brief Destroys a live arena and returns it to the ZERO state.
 *
 * SEMANTICS: This function is perfectly idempotent. Calling it on a NULL
 * pointer, a ZERO arena, or an already-destroyed arena is a safe no-op.
 * Upon successful execution, data is freed, and capacity/offset are zeroed.
 */
void arena_destroy(arena_t *arena);

/**
 * @brief Allocates aligned memory from a LIVE arena.
 *
 * TRANSACTIONAL FAILURE: If the allocation is rejected (e.g., due to OOM,
 * zero size, or invalid alignment), the arena's data, capacity, and offset
 * remain completely untouched.
 */
void *arena_alloc(arena_t *arena, size_t size, size_t alignment);

/**
 * @brief Snapshots the current allocation boundary.
 *
 * VALIDITY: Returns the current offset. The mark is only valid as long as
 * it remains less than or equal to the arena's current offset.
 */
arena_mark_t arena_get_mark(const arena_t *arena);

/**
 * @brief Rewinds the arena to a previously captured mark.
 *
 * STALENESS CONTRACT: A mark becomes "stale" if the arena is popped or reset
 * to an offset lower than the mark. It is a fatal programming error to use a
 * stale mark to attempt to move the arena's offset forward.
 */
void arena_pop_to_mark(arena_t *arena, arena_mark_t mark);

/**
 * @brief Logically invalidates all allocations while keeping the memory alive.
 *
 * SEMANTICS: Sets offset to 0 while leaving data and capacity completely unchanged.
 * Calling arena_reset() explicitly expresses the intent to discard all arena
 * allocations. However, arena_pop_to_mark(arena, 0) remains a valid operation
 * if 0 was captured as a legitimate mark. Any nonzero marks created before a
 * reset immediately become stale.
 */
void arena_reset(arena_t *arena);

#endif //ARENA_H_
