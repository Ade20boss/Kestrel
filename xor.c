#include <stddef.h>
#define KESTREL_CODE
#include "kestrel.h"
#define NO_OF_ITERATIONS 1000000
#define CHECK 1000
Chunk_memory arena = {.offset = 0, .data = NULL, .capacity = 100000000};


float data[] = {
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    0,
    1,
    1,
    1,
    0,
};


int main(void)
{


    arena_init(&arena);
    srand(time(0));

    size_t stride = 3;
    size_t n = sizeof(data) / sizeof(data[0]) / stride;

    matrix data_inputs = {.rows = n, .cols = 2, .stride = stride, .es = data};

    matrix data_outputs = {.rows = n, .cols = 1, .stride = stride, .es = &data[2]};

    size_t arch[] = {2, 2, 1};
    Neural_network test = nn_allocate(arch, ARRAY_LEN(arch), &arena);
    Neural_network best = nn_allocate(arch, ARRAY_LEN(arch), &arena);
    Neural_network test_gradient = nn_allocate(arch, ARRAY_LEN(arch), &arena);
    Nn_randomize(&test, 0, 1);

    float eps = 1.0e-1;
    float rate = 1.0e-1;

    gradient_descent(&test, &test_gradient, &best, eps, data_inputs, data_outputs, rate, NO_OF_ITERATIONS, CHECK);

    for (size_t i = 0; i < 4; i++)
    {
        matrix train_input = row_matricize(data_inputs, i);
        matrix_copy(NN_INPUT(&best), train_input);
        Nn_forward_pass(&best);

        MATRIX_PRINT(train_input);
        printf("   ");
        printf("%f", MAT_POS(NN_OUTPUT(&best), 0, 0));
        printf("\n\n");
    }

    custom_free(&arena);
    return 0;
}
