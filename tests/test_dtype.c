#include "dtype.h"
#include "kestrel_core.h"
#include <stdio.h>


static int fails = 0, total = 0;
#define CHECK(cond, msg)                                                                                               \
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

    /*Validity check */
    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_FP32) == true, "validity check for FP32 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_FP16) == true, "validity check for FP16 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_FP64) == true, "validity check for FP64 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_BF16) == true, "validity check for BF16 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_INT8) == true, "validity check for INT8 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_UINT8) == true, "validity check for UINT8 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_INT16) == true, "validity check for INT16 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_UINT16) == true, "validity check for UINT16 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_INT32) == true, "validity check for INT32 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_UINT32) == true, "validity check for UINT32 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_INT64) == true, "validity check for INT64 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_UINT64) == true, "validity check for UINT64 returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_BOOL) == true, "validity check for BOOL returns true");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_INVALID) == false, "validity check for DTYPE_INVALID returns false");

    CHECK(kestrel_dtype_is_valid(KESTREL_DTYPE_COUNT) == false, "validity check for DTYPE_COUNT returns false");

    CHECK(kestrel_dtype_is_valid((kestrel_dtype_t)999) == false,
          "validity check for out-of-range positive enum returns false");

    CHECK(kestrel_dtype_is_valid((kestrel_dtype_t)-1) == false,
          "validity check for out-of-range negative enum returns false");

    /*Size check */


    CHECK(kestrel_dtype_size(KESTREL_DTYPE_FP32) == 4, "size check for FP32 returns 4bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_FP16) == 2, "size check for FP16 returns 2bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_FP64) == 8, "size check for FP64 returns 8bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_BF16) == 2, "size check for BF16 returns 2bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_INT8) == 1, "size check for INT8 returns 1bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_UINT8) == 1, "size check for UINT8 returns 1bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_INT16) == 2, "size check for INT16 returns 2bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_UINT16) == 2, "size check for UINT16 returns 2bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_INT32) == 4, "size check for INT32 returns 4bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_UINT32) == 4, "size check for UINT32 returns 4bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_INT64) == 8, "size check for INT64 returns 8bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_UINT64) == 8, "size check for UINT64 returns 8bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_BOOL) == 1, "size check for BOOL returns 1bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_INVALID) == 0, "size check for DTYPE_INVALID returns 0bytes");

    CHECK(kestrel_dtype_size(KESTREL_DTYPE_COUNT) == 0, "size check for DTYPE_COUNT returns 0bytes");


    /*classification tests - is_float*/

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_FP32) == true, "is_float check for FP32 returns true");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_FP16) == true, "is_float check for FP16 returns true");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_FP64) == true, "is_float check for FP64 returns true");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_BF16) == true, "is_float check for BF16 returns true");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_INT8) == false, "is_float check for INT8 returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_UINT8) == false, "is_float check for UINT8 returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_INT16) == false, "is_float check for INT16 returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_UINT16) == false, "is_float check for UINT16 returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_INT32) == false, "is_float check for INT32 returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_UINT32) == false, "is_float check for UINT32 returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_INT64) == false, "is_float check for INT64 returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_UINT64) == false, "is_float check for UINT64 returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_BOOL) == false, "is_float check for BOOL returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_INVALID) == false, "is_float check for DTYPE_INVALID returns false");

    CHECK(kestrel_dtype_is_float(KESTREL_DTYPE_COUNT) == false, "is_float check for DTYPE_COUNT returns false");


    /*classification tests - is_integer */

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_FP32) == false, "is_integer check for FP32 returns false");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_FP16) == false, "is_integer check for FP16 returns false");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_FP64) == false, "is_integer check for FP64 returns false");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_BF16) == false, "is_integer check for BF16 returns false");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_INT8) == true, "is_integer check for INT8 returns true");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_UINT8) == true, "is_integer check for UINT8 returns true");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_INT16) == true, "is_integer check for INT16 returns true");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_UINT16) == true, "is_integer check for UINT16 returns true");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_INT32) == true, "is_integer check for INT32 returns true");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_UINT32) == true, "is_integer check for UINT32 returns true");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_INT64) == true, "is_integer check for INT64 returns true");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_UINT64) == true, "is_integer check for UINT64 returns true");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_BOOL) == false, "is_integer check for BOOL returns false");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_INVALID) == false, "is_integer check for DTYPE_INVALID returns false");

    CHECK(kestrel_dtype_is_integer(KESTREL_DTYPE_COUNT) == false, "is_integer check for DTYPE_COUNT returns false");


    /*classification tests - is_bool */

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_FP32) == false, "is_bool check for FP32 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_FP16) == false, "is_bool check for FP16 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_FP64) == false, "is_bool check for FP64 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_BF16) == false, "is_bool check for BF16 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_INT8) == false, "is_bool check for INT8 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_UINT8) == false, "is_bool check for UINT8 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_INT16) == false, "is_bool check for INT16 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_UINT16) == false, "is_bool check for UINT16 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_INT32) == false, "is_bool check for INT32 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_UINT32) == false, "is_bool check for UINT32 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_INT64) == false, "is_bool check for INT64 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_UINT64) == false, "is_bool check for UINT64 returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_BOOL) == true, "is_bool check for BOOL returns true");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_INVALID) == false, "is_bool check for DTYPE_INVALID returns false");

    CHECK(kestrel_dtype_is_bool(KESTREL_DTYPE_COUNT) == false, "is_bool check for DTYPE_COUNT returns false");


    /*classification tests - is_signed_integer */

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_FP32) == false,
          "is_signed_integer check for FP32 returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_FP16) == false,
          "is_signed_integer check for FP16 returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_FP64) == false,
          "is_signed_integer check for FP64 returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_BF16) == false,
          "is_signed_integer check for BF16 returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_INT8) == true, "is_signed_integer check for INT8 returns true");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_UINT8) == false,
          "is_signed_integer check for UINT8 returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_INT16) == true,
          "is_signed_integer check for INT16 returns true");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_UINT16) == false,
          "is_signed_integer check for UINT16 returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_INT32) == true,
          "is_signed_integer check for INT32 returns true");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_UINT32) == false,
          "is_signed_integer check for UINT32 returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_INT64) == true,
          "is_signed_integer check for INT64 returns true");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_UINT64) == false,
          "is_signed_integer check for UINT64 returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_BOOL) == false,
          "is_signed_integer check for BOOL returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_INVALID) == false,
          "is_signed_integer check for DTYPE_INVALID returns false");

    CHECK(kestrel_dtype_is_signed_integer(KESTREL_DTYPE_COUNT) == false,
          "is_signed_integer check for DTYPE_COUNT returns false");


    /*classification tests - is_unsigned_integer */

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_FP32) == false,
          "is_unsigned_integer check for FP32 returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_FP16) == false,
          "is_unsigned_integer check for FP16 returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_FP64) == false,
          "is_unsigned_integer check for FP64 returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_BF16) == false,
          "is_unsigned_integer check for BF16 returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_INT8) == false,
          "is_unsigned_integer check for INT8 returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_UINT8) == true,
          "is_unsigned_integer check for UINT8 returns true");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_INT16) == false,
          "is_unsigned_integer check for INT16 returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_UINT16) == true,
          "is_unsigned_integer check for UINT16 returns true");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_INT32) == false,
          "is_unsigned_integer check for INT32 returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_UINT32) == true,
          "is_unsigned_integer check for UINT32 returns true");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_INT64) == false,
          "is_unsigned_integer check for INT64 returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_UINT64) == true,
          "is_unsigned_integer check for UINT64 returns true");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_BOOL) == false,
          "is_unsigned_integer check for BOOL returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_INVALID) == false,
          "is_unsigned_integer check for DTYPE_INVALID returns false");

    CHECK(kestrel_dtype_is_unsigned_integer(KESTREL_DTYPE_COUNT) == false,
          "is_unsigned_integer check for DTYPE_COUNT returns false");


    /* --- Promotion Tests: 13x13 Symmetry Matrix --- */
    bool symmetry_passed = true;
    for (int i = 1; i < KESTREL_DTYPE_COUNT; i++)
    {
        for (int j = 1; j < KESTREL_DTYPE_COUNT; j++)
        {
            kestrel_dtype_t a = (kestrel_dtype_t)i;
            kestrel_dtype_t b = (kestrel_dtype_t)j;

            if (kestrel_dtype_promote(a, b) != kestrel_dtype_promote(b, a))
            {
                symmetry_passed = false;
                printf("  FAIL: Asymmetry detected between %d and %d\n", a, b);
            }
        }
    }
    CHECK(symmetry_passed == true, "promote(A, B) == promote(B, A) holds for all 169 operand pairs");


    /* --- Explicit Boundary Traps --- */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_BOOL, KESTREL_DTYPE_BOOL) == KESTREL_DTYPE_INVALID,
          "BOOL + BOOL -> INVALID");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INVALID, KESTREL_DTYPE_FP32) == KESTREL_DTYPE_INVALID,
          "INVALID + FP32 -> INVALID");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_COUNT, KESTREL_DTYPE_INT8) == KESTREL_DTYPE_INVALID,
          "COUNT + INT8 -> INVALID");


    /* --- 1. Float Promotion (Complete Matrix) --- */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_FP16, KESTREL_DTYPE_FP16) == KESTREL_DTYPE_FP16, "FP16 + FP16 -> FP16");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_FP16, KESTREL_DTYPE_BF16) == KESTREL_DTYPE_FP32, "FP16 + BF16 -> FP32");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_FP16, KESTREL_DTYPE_FP32) == KESTREL_DTYPE_FP32, "FP16 + FP32 -> FP32");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_FP16, KESTREL_DTYPE_FP64) == KESTREL_DTYPE_FP64, "FP16 + FP64 -> FP64");

    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_BF16, KESTREL_DTYPE_BF16) == KESTREL_DTYPE_BF16, "BF16 + BF16 -> BF16");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_BF16, KESTREL_DTYPE_FP32) == KESTREL_DTYPE_FP32, "BF16 + FP32 -> FP32");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_BF16, KESTREL_DTYPE_FP64) == KESTREL_DTYPE_FP64, "BF16 + FP64 -> FP64");

    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_FP32, KESTREL_DTYPE_FP32) == KESTREL_DTYPE_FP32, "FP32 + FP32 -> FP32");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_FP32, KESTREL_DTYPE_FP64) == KESTREL_DTYPE_FP64, "FP32 + FP64 -> FP64");

    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_FP64, KESTREL_DTYPE_FP64) == KESTREL_DTYPE_FP64, "FP64 + FP64 -> FP64");


    /* --- 2. Integer Promotion (Rule Boundaries) --- */
    /* Same Signedness */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT8, KESTREL_DTYPE_INT64) == KESTREL_DTYPE_INT64,
          "INT8 + INT64 -> INT64");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_UINT8, KESTREL_DTYPE_UINT64) == KESTREL_DTYPE_UINT64,
          "UINT8 + UINT64 -> UINT64");

    /* Signed Wider */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT16, KESTREL_DTYPE_UINT8) == KESTREL_DTYPE_INT16,
          "INT16 + UINT8 -> INT16");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT32, KESTREL_DTYPE_UINT16) == KESTREL_DTYPE_INT32,
          "INT32 + UINT16 -> INT32");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT64, KESTREL_DTYPE_UINT32) == KESTREL_DTYPE_INT64,
          "INT64 + UINT32 -> INT64");

    /* Equal Width Mixed */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT8, KESTREL_DTYPE_UINT8) == KESTREL_DTYPE_INT16,
          "INT8 + UINT8 -> INT16");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT16, KESTREL_DTYPE_UINT16) == KESTREL_DTYPE_INT32,
          "INT16 + UINT16 -> INT32");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT32, KESTREL_DTYPE_UINT32) == KESTREL_DTYPE_INT64,
          "INT32 + UINT32 -> INT64");

    /* Unsigned Wider */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT8, KESTREL_DTYPE_UINT16) == KESTREL_DTYPE_INT32,
          "INT8 + UINT16 -> INT32");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT16, KESTREL_DTYPE_UINT32) == KESTREL_DTYPE_INT64,
          "INT16 + UINT32 -> INT64");

    /* No Representable Result */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT8, KESTREL_DTYPE_UINT64) == KESTREL_DTYPE_INVALID,
          "INT8 + UINT64 -> INVALID");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT16, KESTREL_DTYPE_UINT64) == KESTREL_DTYPE_INVALID,
          "INT16 + UINT64 -> INVALID");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT32, KESTREL_DTYPE_UINT64) == KESTREL_DTYPE_INVALID,
          "INT32 + UINT64 -> INVALID");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT64, KESTREL_DTYPE_UINT64) == KESTREL_DTYPE_INVALID,
          "INT64 + UINT64 -> INVALID");


    /* --- 3. Mixed Integer/Float (Width Classes) --- */
    /* 8-bit */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_UINT8, KESTREL_DTYPE_BF16) == KESTREL_DTYPE_BF16, "UINT8 + BF16 -> BF16");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT8, KESTREL_DTYPE_FP64) == KESTREL_DTYPE_FP64, "INT8 + FP64 -> FP64");

    /* 16-bit */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT16, KESTREL_DTYPE_BF16) == KESTREL_DTYPE_FP32, "INT16 + BF16 -> FP32");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_UINT16, KESTREL_DTYPE_FP32) == KESTREL_DTYPE_FP32,
          "UINT16 + FP32 -> FP32");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT16, KESTREL_DTYPE_FP64) == KESTREL_DTYPE_FP64, "INT16 + FP64 -> FP64");

    /* 32-bit */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT32, KESTREL_DTYPE_FP16) == KESTREL_DTYPE_FP64, "INT32 + FP16 -> FP64");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_UINT32, KESTREL_DTYPE_BF16) == KESTREL_DTYPE_FP64,
          "UINT32 + BF16 -> FP64");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_UINT32, KESTREL_DTYPE_FP64) == KESTREL_DTYPE_FP64,
          "UINT32 + FP64 -> FP64");

    /* 64-bit */
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_INT64, KESTREL_DTYPE_FP16) == KESTREL_DTYPE_INVALID,
          "INT64 + FP16 -> INVALID");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_UINT64, KESTREL_DTYPE_FP32) == KESTREL_DTYPE_INVALID,
          "UINT64 + FP32 -> INVALID");
    CHECK(kestrel_dtype_promote(KESTREL_DTYPE_UINT64, KESTREL_DTYPE_FP64) == KESTREL_DTYPE_INVALID,
          "UINT64 + FP64 -> INVALID");


    /* --- 4. Accumulation Mappings (Complete List) --- */
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_FP16) == KESTREL_DTYPE_FP32, "Accumulate FP16 -> FP32");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_BF16) == KESTREL_DTYPE_FP32, "Accumulate BF16 -> FP32");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_FP32) == KESTREL_DTYPE_FP32, "Accumulate FP32 -> FP32");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_FP64) == KESTREL_DTYPE_FP64, "Accumulate FP64 -> FP64");

    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_INT8) == KESTREL_DTYPE_INT32, "Accumulate INT8 -> INT32");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_INT16) == KESTREL_DTYPE_INT64, "Accumulate INT16 -> INT64");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_INT32) == KESTREL_DTYPE_INT64, "Accumulate INT32 -> INT64");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_INT64) == KESTREL_DTYPE_INT64, "Accumulate INT64 -> INT64");

    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_UINT8) == KESTREL_DTYPE_UINT32, "Accumulate UINT8 -> UINT32");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_UINT16) == KESTREL_DTYPE_UINT64, "Accumulate UINT16 -> UINT64");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_UINT32) == KESTREL_DTYPE_UINT64, "Accumulate UINT32 -> UINT64");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_UINT64) == KESTREL_DTYPE_UINT64, "Accumulate UINT64 -> UINT64");

    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_BOOL) == KESTREL_DTYPE_INVALID, "Accumulate BOOL -> INVALID");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_INVALID) == KESTREL_DTYPE_INVALID, "Accumulate INVALID -> INVALID");
    CHECK(kestrel_dtype_accumulate(KESTREL_DTYPE_COUNT) == KESTREL_DTYPE_INVALID, "Accumulate COUNT -> INVALID");


    printf("\n%d/%d passed, %d failed\n", total - fails, total, fails);
    return fails != 0;
}
