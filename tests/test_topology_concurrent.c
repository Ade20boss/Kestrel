#include "kestrel_core.h"
#include "kestrel_topology.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
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
        else                                                                                                           \
        {                                                                                                              \
            printf("  PASS: %s\n", msg);                                                                               \
        }                                                                                                              \
    } while (0)


typedef struct
{
    atomic_int ready_count;
    atomic_bool go;
    atomic_bool abort;
} start_gate;

typedef struct
{
    kestrel_topology_builder_t *builder;
    kestrel_status_t status;
    start_gate *gate;
} worker_context;


void *worker_func(void *arg)
{
    worker_context *data = arg;
    int previous = atomic_fetch_add(&data->gate->ready_count, 1);

    if (previous == 1)
    {
        atomic_store(&data->gate->go, true);
    }

    else
    {
        while (atomic_load(&data->gate->go) == false && atomic_load(&data->gate->abort) == false)
        {
        }
    }

    if (atomic_load(&data->gate->abort))
    {
        return NULL;
    }


    data->status = kestrel_topology_builder_create(&data->builder);
    return NULL;
}


int main(void)
{

    for (int i = 0; i < 1000; i++)
    {
        start_gate test_gate;

        atomic_init(&test_gate.ready_count, 0);
        atomic_init(&test_gate.go, false);
        atomic_init(&test_gate.abort, false);

        worker_context data_a = {0};
        worker_context data_b = {0};

        data_a.gate = &test_gate;
        data_b.gate = &test_gate;


        pthread_t thread_a;
        pthread_t thread_b;

        int result_a = pthread_create(&thread_a, NULL, worker_func, &data_a);
        int result_b = pthread_create(&thread_b, NULL, worker_func, &data_b);

        if (result_a != 0)
        {
            atomic_store(&data_a.gate->abort, true);
        }


        if (result_b != 0)
        {
            atomic_store(&data_b.gate->abort, true);
        }

        CHECK(result_a == 0 && result_b == 0, "Thread creation returns 0 on success");

        int join_a = -1;
        int join_b = -1;

        if (result_a == 0)
        {
            join_a = pthread_join(thread_a, NULL);
        }


        if (result_b == 0)
        {
            join_b = pthread_join(thread_b, NULL);
        }

        CHECK(join_a == 0 && join_b == 0, "Thread joining returns 0 on success");
        CHECK(atomic_load(&test_gate.ready_count) == 2, "Both workers reached the starting gate");


        if (join_a != 0 || join_b != 0)
        {
            printf("Concurrency iteration test stopped\n");
            printf("REPORT: Iteration %d failed", i + 1);

            if (join_a == 0)
            {
                if (data_a.builder != NULL)
                {
                    kestrel_topology_builder_destroy(&data_a.builder);
                    CHECK(data_a.builder == NULL, "LIVE builder becomes NULL after destruction");
                }
            }

            if (join_b == 0)
            {
                if (data_b.builder != NULL)
                {
                    kestrel_topology_builder_destroy(&data_b.builder);
                    CHECK(data_b.builder == NULL, "LIVE builder becomes NULL after destruction");
                }
            }

            break;
        }

        uint8_t check = 0;
        uint8_t test_builder = 0;


        if (data_a.status == KESTREL_STATUS_SUCCESS)
        {
            check += 1;
            test_builder = 1;
        }


        if (data_b.status == KESTREL_STATUS_SUCCESS)
        {
            check += 1;
            test_builder = 2;
        }


        CHECK(check == 1, "Builder create asserts only one active builder");

        if (test_builder == 1 && check == 1)
        {
            CHECK(data_b.builder == NULL, "Failed thread should have a NULL builder");
            CHECK(data_b.status == KESTREL_STATUS_INVALID_STATE, "Failed thread have invalid state status");
            CHECK(data_a.builder != NULL, "Successful thread should have a LIVE builder");
        }

        if (test_builder == 2 && check == 1)
        {
            CHECK(data_a.builder == NULL, "Failed thread should have a NULL builder");
            CHECK(data_a.status == KESTREL_STATUS_INVALID_STATE, "Failed thread have invalid state status");
            CHECK(data_b.builder != NULL, "Successful thread should have a LIVE builder");
        }


        if (data_a.builder != NULL)
        {
            kestrel_topology_builder_destroy(&data_a.builder);
            CHECK(data_a.builder == NULL, "LIVE builder becomes NULL after destruction");
        }


        if (data_b.builder != NULL)
        {
            kestrel_topology_builder_destroy(&data_b.builder);
            CHECK(data_b.builder == NULL, "LIVE builder becomes NULL after destruction");
        }


        kestrel_topology_builder_t *builder_test = NULL;

        CHECK(kestrel_topology_builder_create(&builder_test) == KESTREL_STATUS_SUCCESS,
              "builder create suceeds on a NULL builder pointer object after reservation has been released");
        kestrel_topology_builder_destroy(&builder_test);
        CHECK(builder_test == NULL, "After destruction, builder object is NULL");


        //Add checking fails logic here
        if (fails > 0)
        {
            printf("Concurrency iteration test stopped\n");
            printf("REPORT: Iteration %d failed", i + 1);
            break;
        }
    }


    printf("\nTotal: %d, Fails: %d\n", total, fails);
    if (fails == 0)
    {
        return 0;
    }
    return 1;
}
