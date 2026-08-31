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
} device_t;

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
