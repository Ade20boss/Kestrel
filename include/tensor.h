#ifndef TENSOR_H_
#define TENSOR_H_

#include "arena.h"
#include "kestrel_core.h"
#include <stdbool.h>

/*
 * ARCHITECTURAL JUSTIFICATIONS
 * 1. Total Element Count: Caching the total element count prevents the engine from
 *    recalculating shape products during flat, 1D mathematical loops (like ReLU or
 *    element-wise addition), saving CPU cycles in the hottest execution paths.
 * 2. OOM Failure State: Because the API returns by value, standard null-pointer
 *    checks fail. If the arena is full, constructors return a sentinel struct where
 *    `.data = NULL`. Callers must explicitly verify the payload pointer before execution.
 * 3. KESTREL_MAX_DIMS = 6: The deepest shape on the roadmap is 4D —
 *    {batch, channels, height, width} for convolutions, and
 *    {batch, heads, sequence, head_dim} for attention. Six is 4 plus
 *    headroom, chosen so an unforeseen use case doesn't require touching
 *    every function. The cost is 32 bytes of unused shape and stride slots
 *    per tensor (128 bytes total, versus 88 at MAX_DIMS = 4). Cheap enough
 *    not to optimise for.
 * 4. By-Value Return: Returning the struct by value flattens Kestrel's memory topology.
 *    Graph nodes can physically embed the tensor metadata, eliminating an arena allocation
 *    per tensor and destroying a layer of pointer-chasing latency during reverse
 *    topological sorting.
 */


/*
 * TENSOR STRUCT INVARIANTS
 * - Strides: Strides are measured in elements (floats), not physical bytes.
 * - Trailing Dimensions: Any slots in the shape and strides arrays at an index >= ndim
 *   are guaranteed to be zeroed and must be ignored by all math loops.
 * - Hardware Locality: The device field dictates physical memory space. Dereferencing
 *   data from host C code when device == DEV_CUDA will immediately trigger a
 *   Segmentation Fault.
 */
typedef struct
{
    float *data;
    size_t ndim;
    size_t shape[KESTREL_MAX_DIMS];
    size_t strides[KESTREL_MAX_DIMS];
    size_t count;
    device_t device;
} tensor_t;

/*
 * Action: Allocates a contiguous, dense N-dimensional tensor.
 * Allocation: Yes. Allocates count * sizeof(float) from the arena aligned to 64 bytes.
 * Aliasing: No. Not a view — data points to a fresh arena allocation. Lifetime is the arena's.
 * Failure: Returns a struct with .data = NULL if the arena is exhausted or inputs are invalid.
 */
tensor_t tensor_alloc(arena_t *arena, size_t ndim, const size_t *shape);

/*
 * Action: Allocates a contiguous, dense N-dimensional tensor and initializes the payload to 0.0f.
 * Allocation: Yes. Allocates count * sizeof(float) from the arena aligned to 64 bytes.
 * Aliasing: No. Not a view — data points to a fresh arena allocation. Lifetime is the arena's.
 * Failure: Returns a struct with .data = NULL if the arena is exhausted or inputs are invalid.
 */
tensor_t tensor_zeros(arena_t *arena, size_t ndim, const size_t *shape);

/*
 * Action: Swaps the two innermost dimensions of the tensor by reversing their strides and also swaps their stride metadata.
 * Allocation: No. Memory topology is unchanged.
 * Aliasing: YES. The returned tensor is a view. Mutating the payload of the returned tensor will silently mutate the original tensor.
 * Failure: Returns a struct with .data = NULL if the input is not at least 2D.
 */
tensor_t tensor_transpose(const tensor_t *tensor);

/*
 * Action: Converts an N-dimensional index into a flat element offset
 *        using the tensor's strides.
 * Allocation: No.
 * Aliasing: N/A (Read-only).
 * Failure: Out-of-bounds indices are a programming error —
 *          asserted in debug builds, undefined in release.
 */
size_t tensor_offset(const tensor_t *tensor, const size_t *idx);

/*
 * Action: Compares ndim and all active shape elements for strict equality.
 * Allocation: No.
 * Aliasing: N/A (Read-only).
 * Failure: Returns false if shapes mismatch or if either tensor is invalid.
 */
bool tensor_same_shape(const tensor_t *tensor_a, const tensor_t *tensor_b);

/*
 * Action: Mathematically verifies that the tensor has no memory gaps and has not been transposed or sliced, meaning it can be processed as a flat 1D array.
 * Allocation: No.
 * Aliasing: N/A (Read-only).
 * Failure: Returns false if the stride math does not perfectly match a dense layout.
 */
bool tensor_is_contiguous(const tensor_t *tensor);

/*
 * Action: Validates that the tensor's data pointer is not NULL.
 * Allocation: No.
 * Aliasing: N/A (Read-only).
 * Failure: Returns false if .data is NULL.
 */
bool tensor_is_valid(const tensor_t *tensor);

#endif //TENSOR_H_
