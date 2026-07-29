#ifndef KESTREL_H_
#define KESTREL_H_


#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#ifndef KESTREL_ASSERT
#include <assert.h>
#define KESTREL_ASSERT assert
#endif // KESTREL_ASSERT

#define MATRIX_PRINT(m) print_matrix(m, #m)
#define MAT_POS(m, r, c) ((m).es[(r) * (m).stride + (c)])
#define ARRAY_LEN(array) sizeof(array) / sizeof(array[0])

float rand_float(void);
float sigmoid(float value);


typedef struct
{
    size_t offset;
    size_t capacity;
    uint8_t *data;

} Chunk_memory;

uint8_t *arena_init(Chunk_memory *arena);
void *custom_alloc(size_t size, Chunk_memory *arena);
int arena_reset(Chunk_memory *arena);
int custom_free(Chunk_memory *arena);

typedef struct
{
    size_t rows;
    size_t cols;
    size_t stride;
    float *es;

} matrix;

matrix matrix_allocate(Chunk_memory *arena, size_t rows, size_t cols);
void matrix_randomize(matrix matrix, float low, float high);
void matrix_fill(matrix matrix, float value);
void matrix_dotproduct(matrix destination, matrix source_a, matrix source_b);
void matrix_activate(matrix matrix);
matrix row_matricize(matrix matrix, size_t row_num);
void matrix_copy(matrix destination, matrix source);
void print_matrix(matrix matrix, const char *name);


typedef struct
{
    size_t count;
    matrix *weights;
    matrix *biases;
    matrix *inputs; // The amount of outputs + 1(original input)
} Neural_network;


Neural_network nn_allocate(size_t *architecture, size_t arch_count, Chunk_memory *arena);
void print_Nn(Neural_network *nn, const char *name);
#define NN_PRINT(nn) print_Nn(&nn, #nn)
#define NN_INPUT(nn) (nn)->inputs[0]
#define NN_OUTPUT(nn) (nn)->inputs[(nn)->count]

void Nn_randomize(Neural_network *nn, float low, float high);
void Nn_forward_pass(Neural_network *nn);
double nn_cost(Neural_network *nn, matrix inputs, matrix outputs);
void nn_fdiff(Neural_network *nn, Neural_network *gradient, float epsilon, matrix inputs, matrix outputs);
void nn_learn(Neural_network *nn, Neural_network *gradient, float rate);
void gradient_descent(Neural_network *nn,
                      Neural_network *gradient,
                      Neural_network *best_model,
                      float eps,
                      matrix inputs,
                      matrix outputs,
                      float rate,
                      size_t iterations,
                      size_t checkpoints);

#endif //KESTREL_H_


#ifdef KESTREL_CODE

uint8_t *arena_init(Chunk_memory *arena)
{
    if (arena->data == NULL)
    {
        arena->data = (uint8_t *)malloc(sizeof(uint8_t) * arena->capacity);
        arena->offset = 0;

        if (arena->data == NULL)
        {
            return NULL;
        }
    }
    return arena->data;
}


void *custom_alloc(size_t size, Chunk_memory *arena)
{
    if (arena->offset + size > arena->capacity)
    {
        return NULL;
    }

    uint8_t *ptr = arena->data + arena->offset;
    arena->offset += size;
    return (void *)ptr;
}

int arena_reset(Chunk_memory *arena)
{
    if (arena->data == NULL)
    {
        return 1;
    }

    else
    {
        arena->offset = 0;
        return 0;
    }
}

int custom_free(Chunk_memory *arena)
{
    if (arena->data == NULL)
    {
        return 1;
    }
    else
    {
        free(arena->data);
        arena->data = NULL;
        arena->offset = 0;
        arena->capacity = 0;
    }
    return 0;
}


float rand_float()
{
    return (float)rand() / (float)RAND_MAX;
}


matrix matrix_allocate(Chunk_memory *arena, size_t rows, size_t cols)
{
    matrix m;
    m.rows = rows;
    m.cols = cols;
    m.stride = cols;
    m.es = custom_alloc(rows * cols * sizeof(*m.es), arena);
    KESTREL_ASSERT(m.es != NULL);
    return m;
}


void matrix_fill(matrix matrix, float value)
{
    for (size_t i = 0; i < matrix.rows; i++)
    {
        for (size_t j = 0; j < matrix.cols; j++)
        {
            MAT_POS(matrix, i, j) = value;
        }
    }
}


void matrix_dotproduct(matrix destination, matrix source_a, matrix source_b)
{
    // for matrix multiplication to work, columns of a must equal rows of b
    // otherwise the elements don't line up and the whole thing falls apart
    KESTREL_ASSERT(source_a.cols == source_b.rows);

    // the resulting matrix always has the same rows as a and same cols as b
    // if destination isn't that shape, we're writing into the wrong place
    KESTREL_ASSERT(destination.rows == source_a.rows && destination.cols == source_b.cols);

    // zero out destination first, because we're using +=
    // if we don't, we're just accumulating on top of garbage
    matrix_fill(destination, 0.0f);

    // locks us into each row of the result, one at a time
    for (size_t i = 0; i < destination.rows; i++)
    {
        // locks us into each column of the result
        // at this point (i, j) is the exact cell we're about to compute
        for (size_t j = 0; j < destination.cols; j++)
        {
            // this is the actual accumulation engine
            // once we're locked into the right row and the right element,
            // k does two things at once — it's the column index for source_a (source_a[i][k])
            // and the row index for source_b (source_b[k][j]) at the same time
            // so as k increases, it walks across the columns of source_a
            // while simultaneously walking down the rows of source_b
            // multiplying the matching pair at each step and accumulating into destination[i][j]
            for (size_t k = 0; k < source_a.cols; k++)
            {
                // destination[i][j] += source_a[i][k] * source_b[k][j]
                // each matrix uses its own stride because they can have different layouts in memory
                MAT_POS(destination, i, j) += MAT_POS(source_a, i, k) * MAT_POS(source_b, k, j);
            }
        }
    }
}


matrix row_matricize(matrix m, size_t row_num)
{
    KESTREL_ASSERT(row_num < m.rows);
    return (matrix){.rows = 1, .cols = m.cols, .stride = m.stride, .es = &MAT_POS(m, row_num, 0)};
}


void matrix_copy(matrix destination, matrix source)
{
    KESTREL_ASSERT(destination.rows == source.rows && destination.cols == source.cols);
    for (size_t i = 0; i < destination.rows; i++)
    {
        for (size_t j = 0; j < destination.cols; j++)
        {
            MAT_POS(destination, i, j) = MAT_POS(source, i, j);
        }
    }
}

void matrix_sum_bias(matrix destination, matrix source)
{
    /*
    So what we're doing here essentially is adding the bias to the weighted sum
    When the layer computes the result, we just add the bias to every computed value
    */
    KESTREL_ASSERT(destination.cols == source.cols && source.rows == 1);
    for (size_t i = 0; i < destination.rows; i++)
    {
        for (size_t j = 0; j < destination.cols; j++)
        {
            MAT_POS(destination, i, j) += MAT_POS(source, 0, j);
        }
    }
}

void print_matrix(matrix m, const char *name)
{
    printf("%s", name);
    printf("\n");
    for (size_t i = 0; i < m.rows; i++)
    {
        printf("    [");
        for (size_t j = 0; j < m.cols; j++)
        {
            printf("%f", MAT_POS(m, i, j));

            if (j < m.cols - 1)
            {
                printf("    ");
            }
        }
        printf("]\n");
    }
}


void matrix_randomize(matrix matrix, float low, float high)
{

    for (size_t i = 0; i < matrix.rows; i++)
    {
        for (size_t j = 0; j < matrix.cols; j++)
        {
            MAT_POS(matrix, i, j) = rand_float() * (high - low) + low;
        }
    }
}

float sigmoid(float x)
{
    return 1.f / (1.f + expf(-x));
}


void matrix_activate(matrix matrix)
{
    for (size_t i = 0; i < matrix.rows; i++)
    {
        for (size_t j = 0; j < matrix.cols; j++)
        {
            MAT_POS(matrix, i, j) = sigmoid(MAT_POS(matrix, i, j));
        }
    }
}


Neural_network nn_allocate(size_t *architecture, size_t arch_count, Chunk_memory *arena)
{
    KESTREL_ASSERT(arch_count > 1);
    Neural_network nn;
    nn.count = arch_count - 1;

    nn.weights = custom_alloc(sizeof(*nn.weights) * nn.count, arena);
    KESTREL_ASSERT(nn.weights != NULL);

    nn.biases = custom_alloc(sizeof(*nn.biases) * nn.count, arena);
    KESTREL_ASSERT(nn.biases != NULL);

    nn.inputs = custom_alloc(sizeof(*nn.inputs) * (nn.count + 1), arena);
    KESTREL_ASSERT(nn.inputs != NULL);

    for (size_t i = 0; i < arch_count; i++)
    {
        nn.inputs[i] = matrix_allocate(arena, 1, architecture[i]);
    }

    for (size_t i = 0; i < nn.count; i++)
    {
        nn.weights[i] = matrix_allocate(arena, nn.inputs[i].cols, architecture[i + 1]);
        nn.biases[i] = matrix_allocate(arena, 1, architecture[i + 1]);
    }

    return nn;
}

void print_Nn(Neural_network *nn, const char *name)
{
    printf("%s = [\n\n", name);

    for (size_t i = 0; i < nn->count; i++)
    {
        printf("LAYER %zu:\n", i + 1);
        printf("    ");
        print_matrix(nn->weights[i], "weights");
        printf("\n\n");
        printf("    ");
        print_matrix(nn->biases[i], "biases");
        printf("\n\n");
    }
    printf("]\n");
}

void Nn_randomize(Neural_network *nn, float low, float high)
{
    for (size_t i = 0; i < nn->count; i++)
    {
        matrix_randomize(nn->weights[i], low, high);
        matrix_randomize(nn->biases[i], low, high);
    }
}

void Nn_forward_pass(Neural_network *nn)
{
    for (size_t i = 0; i < nn->count; i++)
    {
        matrix_dotproduct(nn->inputs[i + 1], nn->inputs[i], nn->weights[i]);
        matrix_sum_bias(nn->inputs[i + 1], nn->biases[i]);
        matrix_activate(nn->inputs[i + 1]);
    }
}


double nn_cost(Neural_network *nn, matrix inputs, matrix outputs)
{
    KESTREL_ASSERT(inputs.rows == outputs.rows);
    KESTREL_ASSERT(NN_OUTPUT(nn).rows == 1);
    KESTREL_ASSERT(NN_OUTPUT(nn).cols == outputs.cols);

    double total_cost = 0.0f;

    for (size_t i = 0; i < inputs.rows; i++)
    {
        matrix train_input = row_matricize(inputs, i);
        matrix train_output = row_matricize(outputs, i);
        matrix_copy(NN_INPUT(nn), train_input);

        Nn_forward_pass(nn);

        for (size_t j = 0; j < train_output.cols; j++)
        {
            float difference = MAT_POS(NN_OUTPUT(nn), 0, j) - MAT_POS(train_output, 0, j);
            total_cost += (difference * difference);
        }
    }
    total_cost /= inputs.rows;
    return total_cost;
}


void nn_fdiff(Neural_network *nn, Neural_network *gradient, float epsilon, matrix inputs, matrix outputs)
{

    float saved;
    double c = nn_cost(nn, inputs, outputs);
    for (size_t i = 0; i < nn->count; i++)
    {
        for (size_t j = 0; j < nn->weights[i].rows; j++)
        {
            for (size_t k = 0; k < nn->weights[i].cols; k++)
            {
                saved = MAT_POS(nn->weights[i], j, k);

                MAT_POS(nn->weights[i], j, k) += epsilon;

                MAT_POS(gradient->weights[i], j, k) = (nn_cost(nn, inputs, outputs) - c) / epsilon;

                MAT_POS(nn->weights[i], j, k) = saved;
            }
        }

        for (size_t j = 0; j < nn->biases[i].rows; j++)
        {
            for (size_t k = 0; k < nn->biases[i].cols; k++)
            {
                saved = MAT_POS(nn->biases[i], j, k);

                MAT_POS(nn->biases[i], j, k) += epsilon;

                MAT_POS(gradient->biases[i], j, k) = (nn_cost(nn, inputs, outputs) - c) / epsilon;

                MAT_POS(nn->biases[i], j, k) = saved;
            }
        }
    }
}

void nn_learn(Neural_network *nn, Neural_network *gradient, float rate)
{
    for (size_t i = 0; i < nn->count; i++)
    {
        for (size_t j = 0; j < nn->weights[i].rows; j++)
        {
            for (size_t k = 0; k < nn->weights[i].cols; k++)
            {
                MAT_POS(nn->weights[i], j, k) -= rate * MAT_POS(gradient->weights[i], j, k);
            }
        }

        for (size_t j = 0; j < nn->biases[i].rows; j++)
        {
            for (size_t k = 0; k < nn->biases[i].cols; k++)
            {
                MAT_POS(nn->biases[i], j, k) -= rate * MAT_POS(gradient->biases[i], j, k);
            }
        }
    }
}

int NN_compare(Neural_network *a, Neural_network *b)
{
    if (a->count != b->count)
    {
        return 1;
    }

    for (size_t i = 0; i < a->count + 1; i++)
    {
        if ((a->inputs[i].rows != b->inputs[i].rows) || (a->inputs[i].cols != b->inputs[i].cols))
        {
            return 1;
        }
    }

    for (size_t j = 0; j < a->count; j++)
    {
        if ((a->weights[j].rows != b->weights[j].rows) || (a->weights[j].cols != b->weights[j].cols))
        {
            return 1;
        }

        if ((a->biases[j].rows != b->biases[j].rows) || (a->biases[j].cols != b->biases[j].cols))
        {
            return 1;
        }
    }

    return 0;
}

void NN_copy(Neural_network *destination, Neural_network *source)
{
    KESTREL_ASSERT(NN_compare(destination, source) == 0);
    for (size_t j = 0; j < destination->count; j++)
    {
        matrix_copy(destination->weights[j], source->weights[j]);
        matrix_copy(destination->biases[j], source->biases[j]);
    }
}

void gradient_descent(Neural_network *nn,
                      Neural_network *gradient,
                      Neural_network *best_model,
                      float eps,
                      matrix inputs,
                      matrix outputs,
                      float rate,
                      size_t iterations,
                      size_t checkpoints)

{
    print_Nn(nn, "Untrained network");
    double best_cost = INFINITY;
    for (size_t i = 0; i < iterations; i++)
    {
        nn_fdiff(nn, gradient, eps, inputs, outputs);
        nn_learn(nn, gradient, rate);
        double new_cost = nn_cost(nn, inputs, outputs);

        if (new_cost < best_cost)
        {
            best_cost = new_cost;
            NN_copy(best_model, nn);
        }

        if (i % checkpoints == 0)
        {
            printf("iteration_no: %zu   cost: %f\n", i, new_cost);
        }
    }
    printf("FINAL COST: %lf\n", nn_cost(nn, inputs, outputs));
    printf("\n\n\n");
    printf("BEST MODEL COST: %lf\n", nn_cost(best_model, inputs, outputs));

    print_Nn(best_model, "BEST MODEL");
    printf("\n\n\n");
}


#endif //KESTREL_CODE
