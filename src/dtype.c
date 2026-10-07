#include "dtype.h"
#include "kestrel_core.h"


size_t kestrel_dtype_size(kestrel_dtype_t dtype)
{
    switch (dtype)
    {
    case KESTREL_DTYPE_INT8:
    case KESTREL_DTYPE_UINT8:
    case KESTREL_DTYPE_BOOL:
        return 1;

    case KESTREL_DTYPE_FP16:
    case KESTREL_DTYPE_BF16:
    case KESTREL_DTYPE_INT16:
    case KESTREL_DTYPE_UINT16:
        return 2;

    case KESTREL_DTYPE_FP32:
    case KESTREL_DTYPE_INT32:
    case KESTREL_DTYPE_UINT32:
        return 4;

    case KESTREL_DTYPE_FP64:
    case KESTREL_DTYPE_INT64:
    case KESTREL_DTYPE_UINT64:
        return 8;

    default:
        /* KESTREL_DTYPE_INVALID, KESTREL_DTYPE_COUNT, or memory corruption */
        return 0;
    }
}

kestrel_dtype_t kestrel_dtype_promote(kestrel_dtype_t first, kestrel_dtype_t second)
{
    if (kestrel_dtype_is_valid(first) == false || kestrel_dtype_is_valid(second) == false)
    {
        return KESTREL_DTYPE_INVALID;
    }

    if (kestrel_dtype_is_bool(first) == true || kestrel_dtype_is_bool(second) == true)
    {
        return KESTREL_DTYPE_INVALID;
    }

    if (first == second)
    {
        return first;
    }


    /* --- SWAP PHASE --- */
    kestrel_dtype_t temp;
    /* If we have a mix of Float and Integer, force the Float to always be 'first' */
    if (kestrel_dtype_is_integer(first) && kestrel_dtype_is_float(second))
    {
        temp = first;
        first = second;
        second = temp;
    }

    /* If we have a mix of Signed and Unsigned ints, force the Signed one to always be 'first' */
    else if (kestrel_dtype_is_unsigned_integer(first) && kestrel_dtype_is_signed_integer(second))
    {
        temp = first;
        first = second;
        second = temp;
    }


    /* 1. Float + Float */

    if (kestrel_dtype_is_float(first) && kestrel_dtype_is_float(second))
    {
        if (first == KESTREL_DTYPE_FP64 || second == KESTREL_DTYPE_FP64)
        {
            return KESTREL_DTYPE_FP64;
        }

        return KESTREL_DTYPE_FP32;
    }


    /*2. Float + Integer */
    if (kestrel_dtype_is_float(first) && kestrel_dtype_is_integer(second))
    {
        size_t int_size = kestrel_dtype_size(second);

        switch (int_size)
        {
        case 1:
            /* 8-bit integer: Every currently supported floating dtype can exactly represent the full domain of an 8-bit integer. */
            return first;

        case 2:
            /* 16-bit integer: Pushes FP16/BF16 to FP32. Otherwise, Float wins. */
            if (first == KESTREL_DTYPE_FP16 || first == KESTREL_DTYPE_BF16)
            {
                return KESTREL_DTYPE_FP32;
            }
            return first;

        case 4:
            /* 32-bit integer: Pushes everything to FP64. */
            return KESTREL_DTYPE_FP64;

        default:
            return KESTREL_DTYPE_INVALID;
        }
    }


    /* 3. Integer + Integer */
    if (kestrel_dtype_is_integer(first) && kestrel_dtype_is_integer(second))
    {
        size_t size1 = kestrel_dtype_size(first);
        size_t size2 = kestrel_dtype_size(second);


        /* Same signedness: Just return the wider one */
        if (kestrel_dtype_is_signed_integer(first) == kestrel_dtype_is_signed_integer(second))
        {
            if (size1 > size2)
            {
                return first;
            }
            else
            {
                return second;
            }
        }

        /*
        * Mixed signedness: Because of the swap phase, we mathematically know:
        * 'first' is Signed, 'second' is Unsigned.
        */

        /* Rule 1: If the signed type is already wider, it safely absorbs the unsigned type */
        if (size1 > size2)
        {
            return first;
        }

        /* Rule 2: If unsigned is equal or wider, we need a signed type strictly wider than the unsigned type */
        size_t target_size = size2 * 2;

        if (target_size == 2)
        {
            return KESTREL_DTYPE_INT16;
        }

        if (target_size == 4)
        {
            return KESTREL_DTYPE_INT32;
        }

        if (target_size == 8)
        {
            return KESTREL_DTYPE_INT64;
        }
    }
    return KESTREL_DTYPE_INVALID;
}

kestrel_dtype_t kestrel_dtype_accumulate(kestrel_dtype_t dtype)
{
    switch (dtype)
    {
    case KESTREL_DTYPE_FP16:
    case KESTREL_DTYPE_BF16:
    case KESTREL_DTYPE_FP32:
        return KESTREL_DTYPE_FP32;
    case KESTREL_DTYPE_FP64:
        return KESTREL_DTYPE_FP64;
    case KESTREL_DTYPE_INT16:
    case KESTREL_DTYPE_INT32:
    case KESTREL_DTYPE_INT64:
        return KESTREL_DTYPE_INT64;
    case KESTREL_DTYPE_INT8:
        return KESTREL_DTYPE_INT32;
    case KESTREL_DTYPE_UINT16:
    case KESTREL_DTYPE_UINT32:
    case KESTREL_DTYPE_UINT64:
        return KESTREL_DTYPE_UINT64;
    case KESTREL_DTYPE_UINT8:
        return KESTREL_DTYPE_UINT32;
    default:
        return KESTREL_DTYPE_INVALID;
    }
}
