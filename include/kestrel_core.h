#ifndef KESTREL_CORE_H
#define KESTREL_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* --- Semantic Memory Macros --- */

#define KILOBYTES(n) ((n) * 1024ULL)
#define MEGABYTES(n) (KILOBYTES(n) * 1024ULL)
#define GIGABYTES(n) (MEGABYTES(n) * 1024ULL)

/* --- Framework Constants --- */
#define KESTREL_ALIGNMENT 64
#define KESTREL_MAX_DIMS 6

/* --- Hardware Targets --- */
typedef enum
{
    DEV_CPU = 0,
    DEV_CUDA = 1 /* Reserved for your future CUDA backend */
} device_t;      //pending removal in KES-004

/*Supported dtype set */
typedef enum
{
    KESTREL_DTYPE_INVALID = 0,
    KESTREL_DTYPE_FP16,
    KESTREL_DTYPE_BF16,
    KESTREL_DTYPE_FP32,
    KESTREL_DTYPE_FP64,
    KESTREL_DTYPE_INT8,
    KESTREL_DTYPE_UINT8,
    KESTREL_DTYPE_INT16,
    KESTREL_DTYPE_UINT16,
    KESTREL_DTYPE_INT32,
    KESTREL_DTYPE_UINT32,
    KESTREL_DTYPE_INT64,
    KESTREL_DTYPE_UINT64,
    KESTREL_DTYPE_BOOL,
    KESTREL_DTYPE_COUNT
} kestrel_dtype_t;

typedef int kestrel_status_t;

enum
{
    KESTREL_STATUS_SUCCESS = 0,
    KESTREL_STATUS_INVALID_ARGUMENT,
    KESTREL_STATUS_INVALID_STATE,
    KESTREL_STATUS_OUT_OF_MEMORY
};

/* --- Assertion Framework --- */
/*
 * In release mode (compiled with -DNDEBUG), asserts compile down to literally nothing.
 * In debug mode, they crash the program and print the exact file and line number.
 */
#ifdef NDEBUG
#define KESTREL_ASSERT(cond) ((void)0)
#else
#include <stdio.h>
#define KESTREL_ASSERT(cond)                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(cond))                                                                                                   \
        {                                                                                                              \
            fprintf(stderr, "ASSERT FAILED: %s\nFile: %s\nLine: %d\n", #cond, __FILE__, __LINE__);                     \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)
#endif

#endif /* KESTREL_CORE_H */
