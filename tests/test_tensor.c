#include "kestrel_core.h"
#include "tensor.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


static int fails = 0, total = 0;
#define TENSOR_CHECK(cond, msg)                                                                                        \
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
    arena_init(&a, MEGABYTES(1));

    /* --- OOM and Rejection Tests --- */
    printf("--- expected error messages below (OOM & Invalid inputs) ---\n");

    /* Record the exact state of the arena before malicious allocations */
    size_t offset_before = a.offset;

    /* 1. Exhausted Arena (OOM) Test */
    /* Requesting 1024^3 floats (approx 4 Gigabytes) against a 1 Megabyte arena */
    size_t massive_shape[] = {1024, 1024, 1024};
    tensor_t oom_test = tensor_alloc(&a, 3, massive_shape);

    TENSOR_CHECK(oom_test.data == NULL, "OOM tensor returns NULL data pointer");
    TENSOR_CHECK(oom_test.ndim == 0, "OOM tensor returns zero-initialized sentinel");
    TENSOR_CHECK(a.offset == offset_before, "arena offset remains completelyunchanged after OOM");


    /* 2. Validation Rejection Tests */
    size_t valid_shape[] = {2, 2};
    size_t zero_shape[] = {2, 0, 2};
    size_t overflow_shape1[] = {SIZE_MAX, 2};

    /* Forces passage through shape multiplication, but fails byte conversion */
    size_t overflow_shape2[] = {(SIZE_MAX / sizeof(float)) + 1};

    tensor_t rej_ndim0 = tensor_alloc(&a, 0, valid_shape);
    TENSOR_CHECK(rej_ndim0.data == NULL && a.offset == offset_before, "rejects ndim == 0 without moving offset");

    tensor_t rej_ndim_max = tensor_alloc(&a, KESTREL_MAX_DIMS + 1, valid_shape);
    TENSOR_CHECK(rej_ndim_max.data == NULL && a.offset == offset_before,
                 "rejects ndim > MAX_DIMS without moving offset");

    tensor_t rej_zero_dim = tensor_alloc(&a, 3, zero_shape);
    TENSOR_CHECK(rej_zero_dim.data == NULL && a.offset == offset_before,
                 "rejects shape containing 0 without moving offset");

    tensor_t rej_overflow1 = tensor_alloc(&a, 2, overflow_shape1);
    TENSOR_CHECK(rej_overflow1.data == NULL && a.offset == offset_before,
                 "rejects shape multiplication overflow without moving offset");

    tensor_t rej_overflow2 = tensor_alloc(&a, 1, overflow_shape2);
    TENSOR_CHECK(rej_overflow2.data == NULL && a.offset == offset_before,
                 "rejects physical byte overflow without moving offset");

    printf("--- end expected errors ---\n");

    /*test arena_reset */
    arena_reset(&a);
    TENSOR_CHECK(a.offset == 0, "arena offset is 0 after reset");

    /*stride test*/
    size_t shape1[] = {32, 28, 28};
    tensor_t test1 = tensor_alloc(&a, 3, shape1);
    TENSOR_CHECK(test1.strides[0] == 784 && test1.strides[1] == 28 && test1.strides[2] == 1, "strides {784, 28, 1}");

    size_t shape2[] = {2, 3};
    tensor_t test2 = tensor_alloc(&a, 2, shape2);
    TENSOR_CHECK(test2.strides[0] == 3 && test2.strides[1] == 1, "strides {3, 1}");

    size_t shape3[] = {32, 3, 28, 28};
    tensor_t test3 = tensor_alloc(&a, 4, shape3);
    TENSOR_CHECK(test3.strides[0] == 2352 && test3.strides[1] == 784 && test3.strides[2] == 28 && test3.strides[3] == 1,
                 "strides {2352, 784, 28, 1}");

    size_t shape4[] = {3};
    tensor_t test4 = tensor_alloc(&a, 1, shape4);
    TENSOR_CHECK(test4.strides[0] == 1, "strides {1}");


    /*count test, stride and count zero out of bounds test*/
    size_t correct_count1 = 1;
    for (size_t i = 0; i < KESTREL_MAX_DIMS; i++)
    {
        if (i >= test1.ndim)
        {
            TENSOR_CHECK(test1.shape[i] == 0, "shape should be zero");
            TENSOR_CHECK(test1.strides[i] == 0, "stride should be zero");
            continue;
        }
        correct_count1 *= test1.shape[i];
    }

    size_t correct_count2 = 1;
    for (size_t i = 0; i < KESTREL_MAX_DIMS; i++)
    {
        if (i >= test2.ndim)
        {
            TENSOR_CHECK(test2.shape[i] == 0, "shape should be zero");
            TENSOR_CHECK(test2.strides[i] == 0, "stride should be zero");
            continue;
        }
        correct_count2 *= test2.shape[i];
    }

    size_t correct_count3 = 1;
    for (size_t i = 0; i < KESTREL_MAX_DIMS; i++)
    {
        if (i >= test3.ndim)
        {
            TENSOR_CHECK(test3.shape[i] == 0, "shape should be zero");
            TENSOR_CHECK(test3.strides[i] == 0, "stride should be zero");
            continue;
        }
        correct_count3 *= test3.shape[i];
    }

    size_t correct_count4 = 1;
    for (size_t i = 0; i < KESTREL_MAX_DIMS; i++)
    {
        if (i >= test4.ndim)
        {
            TENSOR_CHECK(test4.shape[i] == 0, "shape should be zero");
            TENSOR_CHECK(test4.strides[i] == 0, "stride should be zero");
            continue;
        }
        correct_count4 *= test4.shape[i];
    }


    TENSOR_CHECK(test1.count == correct_count1, "count is not correct");
    TENSOR_CHECK(test2.count == correct_count2, "count is not correct");
    TENSOR_CHECK(test3.count == correct_count3, "count is not correct");
    TENSOR_CHECK(test4.count == correct_count4, "count is not correct");


    /*alignment test */
    TENSOR_CHECK(((uintptr_t)test1.data % KESTREL_ALIGNMENT) == 0, "base is 64-aligned");
    TENSOR_CHECK(((uintptr_t)test2.data % KESTREL_ALIGNMENT) == 0, "base is 64-aligned");
    TENSOR_CHECK(((uintptr_t)test3.data % KESTREL_ALIGNMENT) == 0, "base is 64-aligned");
    TENSOR_CHECK(((uintptr_t)test4.data % KESTREL_ALIGNMENT) == 0, "base is 64-aligned");

    /*tensor zeros test*/
    size_t shape5[] = {32, 28, 28};
    tensor_t test5 = tensor_zeros(&a, 3, shape5);
    bool all_zero = true;
    for (size_t i = 0; i < test5.count; i++)
    {
        if (test5.data[i] != 0.0f)
        {
            all_zero = false;
            break;
        }
    }
    TENSOR_CHECK(all_zero, "tensor_zeros initializes all payload elements to exactly 0.0f");

    /*DEV_CPU check*/
    TENSOR_CHECK(test1.device == DEV_CPU, ".device is DEV_CPU by default");

    TENSOR_CHECK(test2.device == DEV_CPU, ".device is DEV_CPU by default");

    TENSOR_CHECK(test3.device == DEV_CPU, ".device is DEV_CPU by default");

    TENSOR_CHECK(test4.device == DEV_CPU, ".device is DEV_CPU by default");

    TENSOR_CHECK(test5.device == DEV_CPU, ".device is DEV_CPU by default");

    /*tensor_is_valid tests*/
    size_t shape6[] = {2, 2};
    tensor_t test6 = tensor_zeros(&a, 2, shape6);

    TENSOR_CHECK(tensor_is_valid(&test6) == true, "tensor_is_valid is true for valid tensors");

    size_t shape7[] = {5, 5};
    tensor_t test7 = tensor_zeros(&a, 0, shape7);
    TENSOR_CHECK(tensor_is_valid(&test7) == false, "tensor_is_valid is false for invalid tensors");

    /*tensor_is_contiguous tests */
    TENSOR_CHECK(tensor_is_contiguous(&test4) == true, "1D tensor is contiguous");
    TENSOR_CHECK(tensor_is_contiguous(&test2) == true, "2D tensor is contiguous");
    TENSOR_CHECK(tensor_is_contiguous(&test1) == true, "3D tensor is contiguous");
    TENSOR_CHECK(tensor_is_contiguous(&test3) == true, "4D tensor is contiguous");
    TENSOR_CHECK(tensor_is_contiguous(&test7) == false, "Invalid tensor is not contiguous");


    /*tensor_transpose test*/
    tensor_t view2d = tensor_transpose(&test2);
    TENSOR_CHECK(view2d.shape[0] == 3u && view2d.shape[1] == 2u, "tensor_transpose produces a swapped shape");

    TENSOR_CHECK(view2d.strides[0] == 1u && view2d.strides[1] == 3u, "tensor_transpose produces swapped strides");

    TENSOR_CHECK(view2d.ndim == test2.ndim, "tensor_transpose does not alter .ndims");
    TENSOR_CHECK(view2d.count == test2.count, "tensor_transpose does not alter .count");
    TENSOR_CHECK(view2d.data == test2.data, "tensor_transpose performs no allocation");

    /*implicit tensor_is_contiguous on transposed view */
    TENSOR_CHECK(tensor_is_contiguous(&view2d) == false, "tensor_is_contiguous returns false on transposed view");

    /*double view test */
    tensor_t double_view = tensor_transpose(&view2d);
    TENSOR_CHECK(double_view.shape[0] == 2u && double_view.shape[1] == 3u, "tensor_transpose produces a swapped shape");
    TENSOR_CHECK(double_view.strides[0] == 3u && double_view.strides[1] == 1u,
                 "tensor_transpose produces swapped strides");


    /*3D view */
    tensor_t view3d = tensor_transpose(&test1);
    TENSOR_CHECK(view3d.shape[0] == 32u && view3d.shape[1] == 28u && view3d.shape[2] == 28u,
                 "tensor_transpose produces a swapped shape");

    TENSOR_CHECK(view3d.strides[0] == 784u && view3d.strides[1] == 1u && view3d.strides[2] == 28u,
                 "tensor_transpose produces swapped strides");

    /*alias proof */
    view2d.data[0] = 42.0f;
    TENSOR_CHECK(test2.data[0] == 42.0f, "Writing through transposed view alters the original");

    /*1D rejection */
    tensor_t view1d = tensor_transpose(&test4);
    TENSOR_CHECK(tensor_is_valid(&view1d) == false, "tensor_is_valid returns false on invalid tensors");


    /*tensor_same_shape tests*/
    size_t shape8[] = {2, 3};
    tensor_t test8 = tensor_zeros(&a, 2, shape8);

    size_t shape9[] = {2, 3};
    tensor_t test9 = tensor_zeros(&a, 2, shape9);

    size_t shape10[] = {3, 2};
    tensor_t test10 = tensor_zeros(&a, 2, shape10);

    size_t shape11[] = {2, 3, 1};
    tensor_t test11 = tensor_zeros(&a, 3, shape11);

    /*same shape test */
    TENSOR_CHECK(tensor_same_shape(&test8, &test9) == true, "tensor_same_shape is true for tensors with same shape");

    /*different shape test */
    TENSOR_CHECK(tensor_same_shape(&test8, &test10) == false,
                 "tensor_same_shape is false for tensors with different shapes");

    /*different rank test */
    TENSOR_CHECK(tensor_same_shape(&test8, &test11) == false,
                 "tensor_same_shape is false for tensors with different dimensions");


    /*tensor offset tests */
    size_t idx2[] = {1, 2};
    TENSOR_CHECK(tensor_offset(&test2, idx2) == 5, "2D offset math is correct");

    size_t idx3[] = {2, 14, 7};
    TENSOR_CHECK(tensor_offset(&test1, idx3) == 1967, "3D offset math is correct");


    arena_destroy(&a);

    printf("\n%d/%d passed, %d failed\n", total - fails, total, fails);
    return fails != 0;
}
