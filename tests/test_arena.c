#include "arena.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fails = 0, total = 0;
#define CHECK(cond, msg)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        total++;                                                                                                       \
        if (!(cond))                                                                                                   \
        {                                                                                                              \
            fails++;                                                                                                   \
            printf("  FAIL: %s (line %d)\n", msg, __LINE__);                                                           \
        }                                                                                                              \
    } while (0)

int main(void)
{
    arena_t a = {0};

    /* --- init --- */
    CHECK(arena_init(&a, 4096), "init 4096 succeeds");
    CHECK(a.capacity == 4096, "capacity rounds to 4096");
    CHECK(a.offset == 0, "offset starts at 0");
    CHECK(((uintptr_t)a.data % KESTREL_ALIGNMENT) == 0, "base is 64-aligned");

    /* capacity rounding */
    arena_t r = {0};
    CHECK(arena_init(&r, 100), "init 100 succeeds");
    CHECK(r.capacity == 128, "100 rounds up to 128");
    arena_destroy(&r);

    /* --- every alloc is aligned --- */
    printf("(expect no errors above/below unless marked)\n");
    for (int i = 1; i <= 20; i++)
    {
        void *p = arena_alloc(&a, (size_t)i * 7, 64);
        CHECK(p != NULL, "alloc succeeds");
        CHECK(((uintptr_t)p % 64) == 0, "alloc is 64-aligned");
    }

    /* smaller alignment still aligned */
    void *p32 = arena_alloc(&a, 13, 32);
    CHECK(((uintptr_t)p32 % 32) == 0, "32-alignment honoured");
    void *p1 = arena_alloc(&a, 1, 1);
    CHECK(p1 != NULL, "alignment 1 works");

    /* --- mark / pop --- */
    arena_reset(&a);
    void *first = arena_alloc(&a, 100, 64);
    arena_mark_t m = arena_get_mark(&a);
    arena_alloc(&a, 200, 64);
    arena_alloc(&a, 300, 64);
    arena_pop_to_mark(&a, m);
    CHECK(arena_get_mark(&a) == m, "pop restores offset");
    void *after = arena_alloc(&a, 200, 64);
    CHECK((uintptr_t)after == (uintptr_t)first + 128, "reuse after pop");

    /* --- write to memory, confirm no overlap --- */
    arena_reset(&a);
    char *b1 = arena_alloc(&a, 64, 64);
    char *b2 = arena_alloc(&a, 64, 64);
    memset(b1, 0xAA, 64);
    memset(b2, 0xBB, 64);
    CHECK(b1[0] == (char)0xAA && b1[63] == (char)0xAA, "b1 intact");
    CHECK(b2[0] == (char)0xBB, "b2 intact, no overlap");

    /* --- failure cases: fully transactional guarantees (Blocker 2) --- */
    printf("--- expected error messages below ---\n");
    arena_reset(&a);
    arena_alloc(&a, 64, 64);

    /* Snapshot the entire state */
    void *before_data = a.data;
    size_t before_cap = a.capacity;
    size_t before_offset = a.offset;

    CHECK(arena_alloc(&a, 999999, 64) == NULL, "oversized returns NULL");
    CHECK(a.data == before_data && a.capacity == before_cap && a.offset == before_offset,
          "data, capacity, and offset completely unchanged after OOM");

    CHECK(arena_alloc(&a, 16, 0) == NULL, "alignment 0 rejected");
    CHECK(a.data == before_data && a.capacity == before_cap && a.offset == before_offset,
          "state unchanged after align-0");

    CHECK(arena_alloc(&a, 16, 128) == NULL, "alignment > base rejected");
    CHECK(a.data == before_data && a.capacity == before_cap && a.offset == before_offset,
          "state unchanged after align > base");

    CHECK(arena_alloc(&a, 16, 48) == NULL, "non-power-of-2 rejected");
    CHECK(a.data == before_data && a.capacity == before_cap && a.offset == before_offset,
          "state unchanged after non-power-of-2");

    CHECK(arena_alloc(&a, 0, 64) == NULL, "size 0 rejected");
    CHECK(a.data == before_data && a.capacity == before_cap && a.offset == before_offset,
          "state unchanged after size 0");

    CHECK(arena_alloc(NULL, 16, 64) == NULL, "null arena rejected");


    /* --- KES-001 Explicit Mark & Reset Tests (Blocker 3) --- */

    /* Mark == current offset is a valid no-op */
    arena_mark_t current_m = arena_get_mark(&a);
    arena_pop_to_mark(&a, current_m);
    CHECK(a.offset == current_m, "pop to current mark is a valid no-op");

    /* Reset preserves data/capacity and reuses base memory */
    void *reset_data_snap = a.data;
    size_t reset_cap_snap = a.capacity;

    arena_reset(&a);
    CHECK(a.offset == 0, "reset successfully zeroes offset");
    CHECK(a.data == reset_data_snap && a.capacity == reset_cap_snap,
          "reset perfectly preserves data pointer and capacity");

    /* Prove reallocation reuses the absolute beginning of the arena */
    void *reused_base = arena_alloc(&a, 16, KESTREL_ALIGNMENT);
    CHECK(reused_base == reset_data_snap, "allocation immediately after reset reuses the base pointer");
    /* exact fit */
    arena_t e = {0};
    arena_init(&e, 128);
    CHECK(arena_alloc(&e, 128, 64) != NULL, "exact capacity fits");
    CHECK(arena_alloc(&e, 1, 64) == NULL, "one byte past capacity fails");
    arena_destroy(&e);

    /* init rejections */
    arena_t bad = {0};
    CHECK(arena_init(&bad, 0) == false, "zero capacity rejected");
    CHECK(arena_init(NULL, 64) == false, "null arena init rejected");
    CHECK(arena_init(&bad, SIZE_MAX) == false, "SIZE_MAX rejected");
    printf("--- end expected errors ---\n");


    /* --- KES-001 Lifecycle & Release Defense Tests --- */

    /* 1. Failed init preserves exactly ZERO */
    arena_t zero_test = {0};
    CHECK(arena_init(&zero_test, SIZE_MAX) == false, "impossible init fails");
    CHECK(zero_test.data == NULL && zero_test.capacity == 0 && zero_test.offset == 0,
          "failed init leaves arena in exact zero state");

    /* 2. Destroy returns exact ZERO */
    arena_t destroy_test = {0};
    arena_init(&destroy_test, 1024);
    arena_destroy(&destroy_test);
    CHECK(destroy_test.data == NULL && destroy_test.capacity == 0 && destroy_test.offset == 0,
          "destroy returns arena to exact zero state");

    /* 3. Re-init after destroy is legal */
    CHECK(arena_init(&destroy_test, 2048) == true, "re-init of a destroyed arena works");
    arena_destroy(&destroy_test);

#ifdef NDEBUG
    /* --- Release Mode Contract Violation Attacks --- */
    printf("--- expected error messages below (Release Mode Defenses) ---\n");

    /* Attack 1: Corrupted / INVALID State Initialization */
    arena_t invalid = {0};
    invalid.data = (void *)0xDEADBEEF;
    invalid.capacity = 0; /* INVALID: Non-NULL data with zero capacity */
    invalid.offset = 50;

    bool invalid_res = arena_init(&invalid, 1024);
    CHECK(invalid_res == false, "NDEBUG: invalid/corrupted arena state rejected");
    CHECK(invalid.data == (void *)0xDEADBEEF && invalid.capacity == 0 && invalid.offset == 50,
          "NDEBUG: invalid state remains completely untouched");

    /* Attack 2: Live Re-initialization (The Leak Hazard) */
    arena_t leak_test = {0};
    arena_init(&leak_test, 1024);
    void *original_data = leak_test.data;
    size_t original_cap = leak_test.capacity;

    bool leak_res = arena_init(&leak_test, 2048);
    CHECK(leak_res == false, "NDEBUG: live re-initialization rejected");
    CHECK(leak_test.data == original_data && leak_test.capacity == original_cap,
          "NDEBUG: existing live allocation was not leaked or overwritten");

    /* Attack 3: Stale / Future Mark Popping */
    arena_mark_t current = arena_get_mark(&leak_test);
    arena_pop_to_mark(&leak_test, current + 9999);
    CHECK(arena_get_mark(&leak_test) == current, "NDEBUG: popping to a future mark was safely ignored");

    arena_destroy(&leak_test);
    printf("--- end expected errors (Release Mode Defenses) ---\n");
#endif

    /* --- double destroy --- */
    arena_destroy(&a);
    arena_destroy(&a);
    arena_destroy(NULL);
    CHECK(a.data == NULL && a.capacity == 0 && a.offset == 0, "destroy clears state to exact ZERO");

    printf("\n%d/%d passed, %d failed\n", total - fails, total, fails);
    return fails != 0;
}
