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
    if (!tensor_is_valid(&allocated))
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

bool tensor_same_shape(const tensor_t *tensor_a, const tensor_t *tensor_b)
{
    KESTREL_ASSERT(tensor_a != NULL);
    KESTREL_ASSERT(tensor_b != NULL);

    if (!tensor_is_valid(tensor_a) || !tensor_is_valid(tensor_b))
    {
        return false;
    }

    if (tensor_a->ndim != tensor_b->ndim)
    {
        return false;
    }

    for (size_t i = 0; i < tensor_a->ndim; i++)
    {
        if (tensor_a->shape[i] != tensor_b->shape[i])
        {
            return false;
        }
    }

    return true;
}

bool tensor_is_contiguous(const tensor_t *tensor)
{
    KESTREL_ASSERT(tensor != NULL);

    if (!tensor_is_valid(tensor))
    {
        return false;
    }
    size_t expected_stride = 1;
    for (size_t i = tensor->ndim; i-- > 0;)
    {
        if (tensor->strides[i] != expected_stride)
        {
            return false;
        }

        expected_stride *= tensor->shape[i];
    }

    return true;
}

tensor_t tensor_transpose(const tensor_t *tensor)
{
    KESTREL_ASSERT(tensor != NULL);
    tensor_t invalid = {0};

    if (!tensor_is_valid(tensor) || tensor->ndim < 2)
    {
        return invalid;
    }

    tensor_t view = *tensor;
    //transpose magic, swap last two dimensions(since last two dimensions is always the rows and columns) and swap the last two strides(since they represent the row and column strides)
    //
    size_t temp1 = view.shape[view.ndim - 1];
    view.shape[view.ndim - 1] = view.shape[view.ndim - 2];
    view.shape[view.ndim - 2] = temp1;

    size_t temp2 = view.strides[view.ndim - 1];
    view.strides[view.ndim - 1] = view.strides[view.ndim - 2];
    view.strides[view.ndim - 2] = temp2;

    return view;
}


size_t tensor_offset(const tensor_t *tensor, const size_t *idx)
{
    KESTREL_ASSERT(tensor != NULL);
    KESTREL_ASSERT(idx != NULL);

    size_t offset = 0;

    for (size_t i = 0; i < tensor->ndim; i++)
    {
        KESTREL_ASSERT(idx[i] < tensor->shape[i]);
        offset += (idx[i] * tensor->strides[i]);
    }

    return offset;
}
