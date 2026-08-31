#include "tensor.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


tensor_t tensor_alloc(arena_t *arena, size_t ndim, const size_t *shape)
{
    /* 1. Programming Error: Fatal logic bug, crash immediately. */
    KESTREL_ASSERT(shape != NULL);

    tensor_t invalid = {0};


    /* 2. Runtime Errors: Bad environment or dead arena, return sentinel. */
    if (arena == NULL || arena->data == NULL)
    {
        fprintf(stderr, "tensor_alloc: arena or arena data is null\n");
        return invalid;
    }

    if (ndim == 0)
    {
        fprintf(stderr, "tensor_alloc: ndim cannot be zero\n");
        return invalid;
    }

    if (ndim > KESTREL_MAX_DIMS)
    {
        fprintf(stderr, "tensor_alloc: ndim cannot be greater than KESTREL_MAX_DIMS\n");
        return invalid;
    }


    tensor_t allocated = {0};
    allocated.ndim = ndim;

    size_t count = 1;
    for (size_t i = KESTREL_MAX_DIMS; i-- > 0;)
    {
        if (i >= ndim)
        {
            allocated.shape[i] = 0;
            allocated.strides[i] = 0;
            continue;
        }


        if (shape[i] == 0)
        {
            fprintf(stderr, "tensor_alloc: shape[%zu] is zero\n", i);
            return invalid;
        }

        if (count > SIZE_MAX / shape[i])
        {
            fprintf(stderr, "tensor_alloc: count overflow on shape[%zu]\n", i);
            return invalid;
        }

        allocated.shape[i] = shape[i];
        allocated.strides[i] = count;

        count *= shape[i]; /* now guaranteed safe */
    }

    if (count > SIZE_MAX / sizeof(float))
    {
        fprintf(stderr, "tensor_alloc: count overflow when calculating number of bytes for allocation\n");
        return invalid;
    }

    size_t allocation_size = count * sizeof(float);

    allocated.count = count;

    allocated.data = arena_alloc(arena, allocation_size, KESTREL_ALIGNMENT);

    if (allocated.data == NULL)
    {
        fprintf(stderr, "tensor_alloc: arena allocation failed\n");
        return invalid;
    }

    allocated.device = DEV_CPU;

    return allocated;
}


tensor_t tensor_zeros(arena_t *arena, size_t ndim, const size_t *shape)
{
    tensor_t allocated = tensor_alloc(arena, ndim, shape);

    /*Check if allocation failed (checking if the returned struct is "zero")*/
    if (allocated.data == NULL || allocated.ndim == 0)
    {
        /*
         * tensor_alloc already printed the specific error message,
         * so we can just return the invalid struct directly.
         */

        return allocated;
    }

    /*
     * memset the allocated data buffer to zero
     * (allocated.count * sizeof(float) gives the total size in bytes)
     */
    memset(allocated.data, 0, allocated.count * sizeof(float));

    return allocated;
}
