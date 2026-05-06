/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************

----------------------------------------------------------------------
Licensing information
Licensor:                 SEGGER Software GmbH
Licensed to:              Nuclei System Technology Co., Ltd., Room 101, No. 500 Bibo Road, Pilot Free Trade Zone, Shanghai, P. R. China
Licensed SEGGER software: emRun RISC-V
License number:           RTL-00126
License model:            License and Service Agreement, signed August 27, 2021
Licensed platform:        RISC-V based Processor Units designed, manufactured marketed and branded by LICENSEE based on the RV32 architecture
----------------------------------------------------------------------
Support and Update Agreement (SUA)
SUA period:               2021-09-09 - 2022-09-09
Contact to extend SUA:    sales@segger.com
-------------------------- END-OF-HEADER -----------------------------

Purpose: Inline functions used by integer and floating-point C code.

*/

/*********************************************************************
*
*       Inline functions
*
**********************************************************************
*/

/*********************************************************************
*
*       __SEGGER_RTL_CLZ_U32_compare_alg_inline()
*
*  Function description
*    Count leading zeroes in a 32-bit integer, use compares.
*
*  Parameters
*    x - Value to count leading zeros; must not be zero.
*
*  Return value
*    Number of leading zeros.
*
*  Additional information
*    This implementation of CLZ will never be called with x==0,
*    and therefore this particular case is not catered for.
*/
static __SEGGER_RTL_ALWAYS_INLINE int __SEGGER_RTL_CLZ_U32_compare_alg_inline(__SEGGER_RTL_U32 x) {
  unsigned n;
  //
  n = 0;
  //
  if (x <= __SEGGER_RTL_U32_C(0x0000FFFF)) { n += 16; x <<= 16; }
  if (x <= __SEGGER_RTL_U32_C(0x00FFFFFF)) { n += 8;  x <<=  8; }
  if (x <= __SEGGER_RTL_U32_C(0x0FFFFFFF)) { n += 4;  x <<=  4; }
  if (x <= __SEGGER_RTL_U32_C(0x3FFFFFFF)) { n += 2;  x <<=  2; }
  if (x <= __SEGGER_RTL_U32_C(0x7FFFFFFF)) { n += 1;            }
  //
  return n;
}

/*********************************************************************
*
*       __SEGGER_RTL_CLZ_U32_shift_alg_inline()
*
*  Function description
*    Count leading zeros in a 32-bit integer, use shifts.
*
*  Parameters
*    x - Value to count leading zeros; must not be zero.
*
*  Return value
*    Number of leading zeros.
*
*  Additional information
*    This implementation of CLZ will never be called with x==0,
*    and therefore this particular case is not catered for.
*/
static __SEGGER_RTL_ALWAYS_INLINE int __SEGGER_RTL_CLZ_U32_shift_alg_inline(__SEGGER_RTL_U32 x) {
  unsigned n;
  //
  n = 0;
  //
  if ((x >> 16) == 0) { n += 16; x <<= 16; }
  if ((x >> 24) == 0) { n += 8;  x <<=  8; }
  if ((x >> 28) == 0) { n += 4;  x <<=  4; }
  if ((x >> 30) == 0) { n += 2;  x <<=  2; }
  if ((x >> 31) == 0) { n += 1;            }
  //
  return n;
}

/*********************************************************************
*
*       __SEGGER_RTL_CLZ_U32_lut_alg_inline()
*
*  Function description
*    Count leading zeros in a 32-bit integer, use LUT.
*
*  Parameters
*    x - Value to count leading zeros.
*
*  Return value
*    Number of leading zeros.
*/
static __SEGGER_RTL_ALWAYS_INLINE int __SEGGER_RTL_CLZ_U32_lut_alg_inline(__SEGGER_RTL_U32 x) {
  unsigned n;
  //
  n = 0;
  //
  if ((x >> 16) == 0) { n += 16; x <<= 16; }
  if ((x >> 24) == 0) { n +=  8; x <<=  8; }
  //
  return n + __SEGGER_RTL_clz_lut[x >> 24];
}

/*********************************************************************
*
*       __SEGGER_RTL_CLZ_U32_inline()
*
*  Function description
*    Count leading zeros in a 32-bit integer, best method.
*
*  Parameters
*    x - Value to count leading zeros.
*
*  Return value
*    Number of leading zeros.
*/
static __SEGGER_RTL_ALWAYS_INLINE int __SEGGER_RTL_CLZ_U32_inline(__SEGGER_RTL_U32 x) {
  //
#if __SEGGER_RTL_OPTIMIZE <= 0
  return __SEGGER_RTL_CLZ_U32_compare_alg_inline(x);
#elif __SEGGER_RTL_OPTIMIZE == 1
  return __SEGGER_RTL_CLZ_U32_shift_alg_inline(x);
#else
  return __SEGGER_RTL_CLZ_U32_lut_alg_inline(x);
#endif
}

/*********************************************************************
*
*       __SEGGER_RTL_CLZ_U64_inline()
*
*  Function description
*    Count leading zeros in a 64-bit integer, best method.
*
*  Parameters
*    x - Value to count leading zeros.
*
*  Return value
*    Number of leading zeros.
*/
static __SEGGER_RTL_ALWAYS_INLINE int __SEGGER_RTL_CLZ_U64_inline(__SEGGER_RTL_U64 x) {
  if (__SEGGER_RTL_U64_H(x) == 0) {
    return 32 + __SEGGER_RTL_CLZ_U32(__SEGGER_RTL_U64_L(x));
  } else {
    return __SEGGER_RTL_CLZ_U32(__SEGGER_RTL_U64_H(x));
  }
}

/*********************************************************************
*
*       __SEGGER_RTL_SMULL_X_func()
*
*  Function description
*    Signed 32-by-32-to-64 multiplication.
*
*  Parameters
*    x - Multiplicand.
*    y - Multiplier.
*
*  Return value
*    Product.
*/
static __SEGGER_RTL_ALWAYS_INLINE __SEGGER_RTL_I64 __SEGGER_RTL_SMULL_X_func(__SEGGER_RTL_I32 x, __SEGGER_RTL_I32 y) {
  return (__SEGGER_RTL_I64)x * (__SEGGER_RTL_I64)y;
}

/*********************************************************************
*
*       __SEGGER_RTL_UMULL_X_func()
*
*  Function description
*    Unsigned 32-by-32-to-64 multiplication.
*
*  Parameters
*    x - Multiplicand.
*    y - Multiplier.
*
*  Return value
*    Product.
*/
static __SEGGER_RTL_ALWAYS_INLINE __SEGGER_RTL_U64 __SEGGER_RTL_UMULL_X_func(__SEGGER_RTL_U32 x, __SEGGER_RTL_U32 y) {
  return (__SEGGER_RTL_U64)x * (__SEGGER_RTL_U64)y;
}

/*********************************************************************
*
*       __SEGGER_RTL_SMULL_HI_func()
*
*  Function description
*    Signed high-order 32-by-32 multiplication.
*
*  Parameters
*    x - Multiplicand.
*    y - Multiplier.
*
*  Return value
*    Product of x and y divided by 2^32.
*/
static __SEGGER_RTL_ALWAYS_INLINE __SEGGER_RTL_I32 __SEGGER_RTL_SMULL_HI_func(__SEGGER_RTL_I32 x, __SEGGER_RTL_I32 y) {
  return __SEGGER_RTL_I64_H(__SEGGER_RTL_SMULL_X(x, y));
}

/*********************************************************************
*
*       __SEGGER_RTL_UMULL_HI_func()
*
*  Function description
*    Unsigned high-order 32-by-32 multiplication.
*
*  Parameters
*    x - Multiplicand.
*    y - Multiplier.
*
*  Return value
*    Product of x and y divided by 2^32.
*/
static __SEGGER_RTL_ALWAYS_INLINE __SEGGER_RTL_U32 __SEGGER_RTL_UMULL_HI_func(__SEGGER_RTL_U32 x, __SEGGER_RTL_U32 y) {
  return __SEGGER_RTL_U64_H(__SEGGER_RTL_UMULL_X(x, y));
}

/*********************************************************************
*
*       __SEGGER_RTL_SMULL_func()
*
*  Function description
*    Split signed 32-by-32-to-64 multiplication.
*
*  Parameters
*    pLo - Pointer to object that receives the low 32 bits of the product.
*    pHi - Pointer to object that receives the high 32 bits of the product.
*    x   - Multiplicand.
*    y   - Multiplier.
*/
static __SEGGER_RTL_ALWAYS_INLINE void __SEGGER_RTL_SMULL_func(__SEGGER_RTL_U32 *pLo,
                                                               __SEGGER_RTL_U32 *pHi,
                                                               __SEGGER_RTL_U32 x,
                                                               __SEGGER_RTL_U32 y) {
  __SEGGER_RTL_U64 u;
  //
  u    = __SEGGER_RTL_SMULL_X(x, y);
  *pLo = __SEGGER_RTL_I64_L(u);
  *pHi = __SEGGER_RTL_I64_H(u);
}

/*********************************************************************
*
*       __SEGGER_RTL_UMULL_func()
*
*  Function description
*    Split unsigned 32-by-32-to-64 multiplication.
*
*  Parameters
*    pLo - Pointer to object that receives the low 32 bits of the product.
*    pHi - Pointer to object that receives the high 32 bits of the product.
*    x   - Multiplicand.
*    y   - Multiplier.
*/
static __SEGGER_RTL_ALWAYS_INLINE void __SEGGER_RTL_UMULL_func(__SEGGER_RTL_U32 *pLo,
                                                               __SEGGER_RTL_U32 *pHi,
                                                               __SEGGER_RTL_U32 x,
                                                               __SEGGER_RTL_U32 y) {
  __SEGGER_RTL_U64 u;
  //
  u    = __SEGGER_RTL_UMULL_X(x, y);
  *pLo = __SEGGER_RTL_U64_L(u);
  *pHi = __SEGGER_RTL_U64_H(u);
}

/*********************************************************************
*
*       __SEGGER_RTL_SMLAL_func()
*
*  Function description
*    Signed multiply and accumulate, long.
*
*  Parameters
*    pLo - Pointer to object that accumulates the low 32 bits of the product.
*    pHi - Pointer to object that accumulates the high 32 bits of the product.
*    x   - Multiplicand.
*    y   - Multiplier.
*
*  Additional information
*    Computes hi:lo += (x * y) where x*y is a 64-bit product and hi:lo is
*    a 64-bit accumulator in two halves.
*/
static __SEGGER_RTL_ALWAYS_INLINE void __SEGGER_RTL_SMLAL_func(__SEGGER_RTL_U32 *pLo,
                                                               __SEGGER_RTL_U32 *pHi,
                                                               __SEGGER_RTL_U32  x,
                                                               __SEGGER_RTL_U32  y) {
  __SEGGER_RTL_U64 u;
  //
  u    = __SEGGER_RTL_SMULL_X(x, y);
  *pLo += __SEGGER_RTL_U64_L(u);
  *pHi += __SEGGER_RTL_U64_H(u);
  *pHi += (*pLo) < __SEGGER_RTL_U64_L(u);
}

/*********************************************************************
*
*       __SEGGER_RTL_UMLAL_func()
*
*/
static __SEGGER_RTL_ALWAYS_INLINE void __SEGGER_RTL_UMLAL_func(__SEGGER_RTL_U32 *lo,
                                                               __SEGGER_RTL_U32 *hi,
                                                               __SEGGER_RTL_U32 x,
                                                               __SEGGER_RTL_U32 y) {
  __SEGGER_RTL_U64 u;
  //
  u    = __SEGGER_RTL_UMULL_X(x, y);
  *lo += __SEGGER_RTL_U64_L(u);
  *hi += __SEGGER_RTL_U64_H(u);
  *hi += (*lo) < __SEGGER_RTL_U64_L(u);
}

/*************************** End of file ****************************/
