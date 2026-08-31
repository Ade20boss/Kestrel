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
    arena_t a;

    /* --- init --- */
    CHECK(arena_init(&a, 4096), "init 4096 succeeds");
    CHECK(a.capacity == 4096, "capacity rounds to 4096");
    CHECK(a.offset == 0, "offset starts at 0");
    CHECK(((uintptr_t)a.data % KESTREL_ALIGNMENT) == 0, "base is 64-aligned");

    /* capacity rounding */
    arena_t r;
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

    /* --- failure cases: offset must not move --- */
    printf("--- expected error messages below ---\n");
    arena_reset(&a);
    arena_alloc(&a, 64, 64);
    size_t before = a.offset;

    CHECK(arena_alloc(&a, 999999, 64) == NULL, "oversized returns NULL");
    CHECK(a.offset == before, "offset unchanged after OOM");
    CHECK(arena_alloc(&a, 16, 0) == NULL, "alignment 0 rejected");
    CHECK(a.offset == before, "offset unchanged after align-0");
    CHECK(arena_alloc(&a, 16, 128) == NULL, "alignment > base rejected");
    CHECK(arena_alloc(&a, 16, 48) == NULL, "non-power-of-2 rejected");
    CHECK(arena_alloc(&a, 0, 64) == NULL, "size 0 rejected");
    CHECK(a.offset == before, "offset unchanged after all rejects");
    CHECK(arena_alloc(NULL, 16, 64) == NULL, "null arena rejected");

    /* exact fit */
    arena_t e;
    arena_init(&e, 128);
    CHECK(arena_alloc(&e, 128, 64) != NULL, "exact capacity fits");
    CHECK(arena_alloc(&e, 1, 64) == NULL, "one byte past capacity fails");
    arena_destroy(&e);

    /* init rejections */
    arena_t bad;
    CHECK(arena_init(&bad, 0) == false, "zero capacity rejected");
    CHECK(arena_init(NULL, 64) == false, "null arena init rejected");
    CHECK(arena_init(&bad, SIZE_MAX) == false, "SIZE_MAX rejected");
    printf("--- end expected errors ---\n");

    /* --- double destroy --- */
    arena_destroy(&a);
    arena_destroy(&a);
    arena_destroy(NULL);
    CHECK(a.data == NULL && a.capacity == 0, "destroy clears state");

    printf("\n%d/%d passed, %d failed\n", total - fails, total, fails);
    return fails != 0;
}
