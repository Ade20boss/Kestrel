#include "../src/kestrel_topology.c"
#include "kestrel_core.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


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
    int *num_arr = malloc(sizeof(int) * 4);
    CHECK(num_arr != NULL, "Malloc succeeds");
    if (num_arr != NULL)
    {
        num_arr[0] = 10;
        num_arr[1] = 20;
        num_arr[2] = 30;
        num_arr[3] = 40;

        growth_output result = kestrel_growth_helper(num_arr, 4, 3, sizeof(int));

        CHECK(result.status == KESTREL_STATUS_SUCCESS, "Growth succeeds on valid arguments");

        CHECK(result.data == num_arr, "Data pointer remains the same");

        CHECK(result.capacity == 4, "Capacity remains unchanged");


        growth_output result2 = kestrel_growth_helper(num_arr, 4, 5, sizeof(int));

        CHECK(result2.status == KESTREL_STATUS_OUT_OF_MEMORY, "Growth fails on on realloc failure");

        CHECK(result2.data == NULL, "Data pointer is NULL");

        CHECK(result2.capacity == 0, "Capacity is 0");


        if (result2.status == KESTREL_STATUS_OUT_OF_MEMORY && result2.data == NULL)
        {
            CHECK(num_arr[0] == 10, "Data in original array remains unchanged");
            CHECK(num_arr[1] == 20, "Data in original array remains unchanged");
            CHECK(num_arr[2] == 30, "Data in original array remains unchanged");
            CHECK(num_arr[3] == 40, "Data in original array remains unchanged");

            growth_output result3 = kestrel_growth_helper(num_arr, 4, 5, sizeof(int));

            CHECK(result3.status == KESTREL_STATUS_SUCCESS, "Growth succeeds on valid arguments");

            CHECK(result3.data != NULL, "Data pointer is non-NULL on growth success");

            CHECK(result3.capacity == 8, "Capacity Doubles");

            if (result3.status == KESTREL_STATUS_SUCCESS && result3.data != NULL)
            {
                int *grown_arr = result3.data;
                CHECK(grown_arr[0] == 10, "Data in original array remains unchanged");
                CHECK(grown_arr[1] == 20, "Data in original array remains unchanged");
                CHECK(grown_arr[2] == 30, "Data in original array remains unchanged");
                CHECK(grown_arr[3] == 40, "Data in original array remains unchanged");

                free(result3.data);
            }

            else
            {
                free(num_arr);
            }
        }

        else
        {
            free(result2.data);
        }
    }

    printf("\n%d/%d passed, %d failed\n", total - fails, total, fails);
    if (fails == 0)
    {
        return 0;
    }
    return 1;
}
