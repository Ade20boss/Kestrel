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
    void *owned_data = NULL;

    growth_output result1 = kestrel_growth_helper(NULL, 0, 1, sizeof(int));

    CHECK(result1.status == KESTREL_STATUS_SUCCESS,
          "Growth is successful on valid arguments, NULL, 0, 1 and sizeof(int) are valid arguments");

    CHECK(result1.data != NULL, "Successful growth initializes empty array");

    CHECK(result1.capacity == 4, "For zero capacoty array, growth helper initializes to 4");


    if (result1.status == KESTREL_STATUS_SUCCESS && result1.data != NULL)
    {
        owned_data = result1.data;

        growth_output result2 = kestrel_growth_helper(result1.data, result1.capacity, 3, sizeof(int));

        CHECK(result2.status == KESTREL_STATUS_SUCCESS, "Growth is successful on valid arguments");

        CHECK(result2.data == result1.data, "Data pointer remains the same on smaller capacity requirements");

        CHECK(result2.capacity == 4, "Capacity remain the same");

        if (result2.status == KESTREL_STATUS_SUCCESS && result2.data != NULL)
        {
            owned_data = result2.data;

            growth_output result3 = kestrel_growth_helper(result2.data, result2.capacity, 5, sizeof(int));

            CHECK(result3.status == KESTREL_STATUS_SUCCESS, "Growth is successful and doubles capacity");

            CHECK(result3.data != NULL, "Doubling returns a valid allocation");
            CHECK(result3.capacity == 8, "Capacity Doubles");


            if (result3.status == KESTREL_STATUS_SUCCESS && result3.data != NULL)
            {
                owned_data = result3.data;
            }
        }
    }


    growth_output result4 = kestrel_growth_helper(NULL, 0, 12, sizeof(int));

    CHECK(result4.status == KESTREL_STATUS_SUCCESS, "Growth is successful and uses headroom mechanism");

    CHECK(result4.data != NULL, "Successful growth initializes empty array");

    CHECK(result4.capacity == 15, "Capacity is equal to required_count + (required_count/4)");


    growth_output result5 = kestrel_growth_helper(NULL, 0, 0, sizeof(int));

    CHECK(result5.status == KESTREL_STATUS_SUCCESS, "Growth is successful on valid arguments");

    CHECK(result5.data == NULL, "Data pointer returns NULL since no allocation occured");

    CHECK(result5.capacity == 0, "Capacity returns the zero since no allocation happened");


    growth_output result6 = kestrel_growth_helper(NULL, 0, 12, 0);

    CHECK(result6.status == KESTREL_STATUS_INVALID_ARGUMENT, "Growth fails on invalid arguments");
    CHECK(result6.data == NULL, "Data pointer returns NULL due to invalid argument");
    CHECK(result6.capacity == 0, "capacity returns 0 due to invalid argument");


    growth_output result7 = kestrel_growth_helper(NULL, 15, 20, sizeof(int));

    CHECK(result7.status == KESTREL_STATUS_INVALID_STATE, "Growth fails on invalid arguments");
    CHECK(result7.data == NULL, "Data pointer remains NULL due to invalid argument");
    CHECK(result7.capacity == 0, "capacity returns 0 due to invalid argument");

    int dummy = 0;

    growth_output result8 = kestrel_growth_helper(&dummy, 0, 20, sizeof(int));

    CHECK(result8.status == KESTREL_STATUS_INVALID_STATE, "Growth fails on invalid arguments");
    CHECK(result8.data == NULL, "Data pointer remains NULL due to invalid argument");
    CHECK(result8.capacity == 0, "capacity returns 0 due to invalid argument");


    growth_output result9 = kestrel_growth_helper(NULL, 0, 1, ((SIZE_MAX / 4) + 7));

    CHECK(result9.status == KESTREL_STATUS_INVALID_ARGUMENT, "Growth fails on invalid arguments");
    CHECK(result9.data == NULL, "Data pointer remains NULL due to invalid argument");
    CHECK(result9.capacity == 0, "capacity returns 0 due to invalid argument");


    int *arr = malloc(5 * sizeof(int));

    CHECK(arr != NULL, "Malloc allocation succeeds");


    if (arr != NULL)
    {

        for (int i = 0; i < 5; i++)
        {
            arr[i] = i * 10;
        }

        growth_output result10 = kestrel_growth_helper(arr, 5, 10, sizeof(int));

        CHECK(result10.status == KESTREL_STATUS_SUCCESS, "Growth is successful and doubles capacity");
        CHECK(result10.data != NULL, "Doubling returns a valid allocation");

        CHECK(result10.capacity == 10, "Capacity Doubles");


        if (result10.status == KESTREL_STATUS_SUCCESS && result10.data != NULL)
        {
            int *new_arr = (int *)result10.data;
            for (int i = 0; i < 5; i++)
            {
                CHECK(new_arr[i] == (i * 10), "Previous array occupants remain unchanged after growth");
            }
        }

        else
        {
            free(arr);
        }

        free(result10.data);
    }


    free(owned_data);
    free(result4.data);


    printf("\n%d/%d passed, %d failed\n", total - fails, total, fails);
    if (fails == 0)
    {
        return 0;
    }
    return 1;
}
