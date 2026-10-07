#ifndef DTYPE_H_
#define DTYPE_H_

#include "kestrel_core.h"
#include <stdbool.h>
#include <stddef.h>


size_t kestrel_dtype_size(kestrel_dtype_t dtype);
kestrel_dtype_t kestrel_dtype_promote(kestrel_dtype_t first, kestrel_dtype_t second);
kestrel_dtype_t kestrel_dtype_accumulate(kestrel_dtype_t dtype);

static inline bool kestrel_dtype_is_float(kestrel_dtype_t dtype)
{
    switch (dtype)
    {
    case KESTREL_DTYPE_BF16:
    case KESTREL_DTYPE_FP16:
    case KESTREL_DTYPE_FP32:
    case KESTREL_DTYPE_FP64:
        return true;
    default:
        return false;
    }
}

static inline bool kestrel_dtype_is_integer(kestrel_dtype_t dtype)
{
    switch (dtype)
    {
    case KESTREL_DTYPE_INT8:
    case KESTREL_DTYPE_UINT8:
    case KESTREL_DTYPE_INT16:
    case KESTREL_DTYPE_UINT16:
    case KESTREL_DTYPE_INT32:
    case KESTREL_DTYPE_UINT32:
    case KESTREL_DTYPE_INT64:
    case KESTREL_DTYPE_UINT64:
        return true;
    default:
        return false;
    }
}

static inline bool kestrel_dtype_is_bool(kestrel_dtype_t dtype)
{
    return dtype == KESTREL_DTYPE_BOOL;
}


static inline bool kestrel_dtype_is_signed_integer(kestrel_dtype_t dtype)
{
    switch (dtype)
    {
    case KESTREL_DTYPE_INT8:
    case KESTREL_DTYPE_INT16:
    case KESTREL_DTYPE_INT32:
    case KESTREL_DTYPE_INT64:
        return true;
    default:
        return false;
    }
}


static inline bool kestrel_dtype_is_unsigned_integer(kestrel_dtype_t dtype)
{
    switch (dtype)
    {
    case KESTREL_DTYPE_UINT8:
    case KESTREL_DTYPE_UINT16:
    case KESTREL_DTYPE_UINT32:
    case KESTREL_DTYPE_UINT64:
        return true;
    default:
        return false;
    }
}


static inline bool kestrel_dtype_is_valid(kestrel_dtype_t dtype)
{
    switch (dtype)
    {
    case KESTREL_DTYPE_INT8:
    case KESTREL_DTYPE_UINT8:
    case KESTREL_DTYPE_INT16:
    case KESTREL_DTYPE_UINT16:
    case KESTREL_DTYPE_INT32:
    case KESTREL_DTYPE_UINT32:
    case KESTREL_DTYPE_INT64:
    case KESTREL_DTYPE_UINT64:
    case KESTREL_DTYPE_BF16:
    case KESTREL_DTYPE_FP16:
    case KESTREL_DTYPE_FP32:
    case KESTREL_DTYPE_FP64:
    case KESTREL_DTYPE_BOOL:
        return true;
    default:
        return false;
    }
}


#endif //DTYPE_H_
