#include "kestrel_core.h"
#include "kestrel_topology.h"
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
        else                                                                                                           \
        {                                                                                                              \
            printf("  PASS: %s\n", msg);                                                                               \
        }                                                                                                              \
    } while (0)


int main(void)
{
    kestrel_topology_builder_t *builder = NULL;
    CHECK(kestrel_topology_builder_create(&builder) == KESTREL_STATUS_OUT_OF_MEMORY,
          "Builder create fails on allocation failure");

    CHECK(builder == NULL, "Builder should remain NULL after create failure");

    CHECK(kestrel_topology_builder_create(&builder) == KESTREL_STATUS_SUCCESS,
          "builder create suceeds on a NULL builder pointer object");

    CHECK(builder != NULL, "Builder should not be NULL after builder create success");

    kestrel_topology_builder_destroy(&builder);

    CHECK(builder == NULL, "After destruction of NULL object, builder object is NULL");


    printf("\nTotal: %d, Fails: %d\n", total, fails);
    if (fails == 0)
    {
        return 0;
    }
    return 1;
}
