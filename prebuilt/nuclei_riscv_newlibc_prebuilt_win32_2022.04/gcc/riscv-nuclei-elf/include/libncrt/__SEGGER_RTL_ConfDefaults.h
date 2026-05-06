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
*/

#ifndef __SEGGER_RTL_LIBC_CONF_DEFAULTS_H
#define __SEGGER_RTL_LIBC_CONF_DEFAULTS_H

#include "__SEGGER_RTL_Conf.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define __SEGGER_RTL_NAN_FORMAT_IEEE            0    // NaN is fully conformant IEEE NaN
#define __SEGGER_RTL_NAN_FORMAT_FAST            1    // NaN is AEABI-conformant IEEE NaN; 64-bit NaN can be distingushed from Inf using only the upper 32 bits
#define __SEGGER_RTL_NAN_FORMAT_COMPACT         2    // NaN is defined only by the top 16 bits of the floating value

#define __WIDTH_INT                             0
#define __WIDTH_LONG                            1
#define __WIDTH_LONG_LONG                       2

#define __WIDTH_NONE                            0
#define __WIDTH_FLOAT                           1
#define __WIDTH_DOUBLE                          2

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

//
// Configuration of internal implementation.
//
#ifndef   __SEGGER_RTL_SIDE_BY_SIDE
  #define __SEGGER_RTL_SIDE_BY_SIDE             0
#endif

#ifndef   __SEGGER_RTL_FORCE_SOFT_FLOAT
  #define __SEGGER_RTL_FORCE_SOFT_FLOAT         0
#endif

#ifndef   __SEGGER_RTL_CONFIG_CODE_COVERAGE
  #define __SEGGER_RTL_CONFIG_CODE_COVERAGE     0
#endif

#ifndef   __SEGGER_RTL_INCLUDE_AEABI_API
  #define __SEGGER_RTL_INCLUDE_AEABI_API        0
#endif

#ifndef   __SEGGER_RTL_INCLUDE_GNU_API
  #define __SEGGER_RTL_INCLUDE_GNU_API          0
#endif

#ifndef   __SEGGER_RTL_INCLUDE_GNU_FP16_API
  #define __SEGGER_RTL_INCLUDE_GNU_FP16_API     0
#endif

#ifndef   __SEGGER_RTL_INCLUDE_SEGGER_API
  #define __SEGGER_RTL_INCLUDE_SEGGER_API       0
#endif

#ifndef   __SEGGER_RTL_INCLUDE_C_API
  #define __SEGGER_RTL_INCLUDE_C_API            1
#endif

#if __SEGGER_RTL_TYPESET == 16
  //
  // Basic configuration of types for 16-bit targets
  //
  #define __SEGGER_RTL_U64                      unsigned long long
  #define __SEGGER_RTL_I64                      long long
  #define __SEGGER_RTL_U32                      unsigned long
  #define __SEGGER_RTL_I32                      long int
  #define __SEGGER_RTL_U16                      unsigned
  #define __SEGGER_RTL_I16                      int
  #define __SEGGER_RTL_U8                       unsigned char
  #define __SEGGER_RTL_I8                       signed char
  #define __SEGGER_RTL_LEAST_U32                unsigned long
  #define __SEGGER_RTL_LEAST_U16                unsigned
  #define __SEGGER_RTL_LEAST_I16                int
  #define __SEGGER_RTL_LEAST_U8                 unsigned
  #define __SEGGER_RTL_LEAST_I8                 int
  #define __SEGGER_RTL_I32_C(X)                 X##L
  #define __SEGGER_RTL_U32_C(X)                 X##uL
  #define __SEGGER_RTL_I64_C(X)                 X##LL
  #define __SEGGER_RTL_U64_C(X)                 X##uLL
  //
  #define __SEGGER_RTL_SIZEOF_INT               2
  #define __SEGGER_RTL_SIZEOF_LONG              4
  #define __SEGGER_RTL_SIZEOF_PTR               2 // probably

  #if defined(__GNUC__) || defined(__clang__)
    #define __SEGGER_RTL_PTRDIFF_T              __PTRDIFF_TYPE__
    #define __SEGGER_RTL_SIZE_T                 __SIZE_TYPE__
    #define __SEGGER_RTL_WINT_T                 __WINT_TYPE__
    #define __SEGGER_RTL_WCHAR_T                __WCHAR_TYPE__
    #define __SEGGER_RTL_SIZEOF_WCHAR_T         __SIZEOF_WCHAR_T__
  #else
    #define __SEGGER_RTL_PTRDIFF_T              unsigned
    #define __SEGGER_RTL_SIZE_T                 unsigned
  #endif
  //
#elif __SEGGER_RTL_TYPESET == 32
  //
  // Basic configuration of types for 32-bit targets
  //
  #define __SEGGER_RTL_U64                      unsigned long long
  #define __SEGGER_RTL_I64                      long long
  #define __SEGGER_RTL_U32                      unsigned
  #define __SEGGER_RTL_I32                      int
  #define __SEGGER_RTL_U16                      unsigned short
  #define __SEGGER_RTL_I16                      short
  #define __SEGGER_RTL_U8                       unsigned char
  #define __SEGGER_RTL_I8                       signed char
  #define __SEGGER_RTL_LEAST_U32                unsigned
  #define __SEGGER_RTL_LEAST_U16                unsigned
  #define __SEGGER_RTL_LEAST_I16                int
  #define __SEGGER_RTL_LEAST_U8                 unsigned
  #define __SEGGER_RTL_LEAST_I8                 int
  #define __SEGGER_RTL_I32_C(X)                 X
  #define __SEGGER_RTL_U32_C(X)                 X##u
  #define __SEGGER_RTL_I64_C(X)                 X##LL
  #define __SEGGER_RTL_U64_C(X)                 X##uLL
  //
  #define __SEGGER_RTL_SIZEOF_INT               4
  #define __SEGGER_RTL_SIZEOF_LONG              4
  #define __SEGGER_RTL_SIZEOF_PTR               4

  #if defined(__GNUC__) || defined(__clang__)
    #define __SEGGER_RTL_PTRDIFF_T              __PTRDIFF_TYPE__
    #define __SEGGER_RTL_SIZE_T                 __SIZE_TYPE__
    #define __SEGGER_RTL_WINT_T                 __WINT_TYPE__
    #define __SEGGER_RTL_WCHAR_T                __WCHAR_TYPE__
    #define __SEGGER_RTL_SIZEOF_WCHAR_T         __SIZEOF_WCHAR_T__
  #else
    #define __SEGGER_RTL_PTRDIFF_T              unsigned
    #define __SEGGER_RTL_SIZE_T                 unsigned
  #endif
  //
#elif __SEGGER_RTL_TYPESET == 64
  //
  // Basic configuration of types for 64-bit LP64 targets
  //
  #define __SEGGER_RTL_U64                      unsigned long long
  #define __SEGGER_RTL_I64                      long long
  #define __SEGGER_RTL_U32                      unsigned
  #define __SEGGER_RTL_I32                      int
  #define __SEGGER_RTL_U16                      unsigned short
  #define __SEGGER_RTL_I16                      short
  #define __SEGGER_RTL_U8                       unsigned char
  #define __SEGGER_RTL_I8                       signed char
  #define __SEGGER_RTL_LEAST_U32                unsigned long  // generates better code
  #define __SEGGER_RTL_LEAST_U16                unsigned
  #define __SEGGER_RTL_LEAST_I16                int
  #define __SEGGER_RTL_LEAST_U8                 unsigned
  #define __SEGGER_RTL_LEAST_I8                 int
  #define __SEGGER_RTL_I32_C(X)                 X
  #define __SEGGER_RTL_U32_C(X)                 X##u
  #define __SEGGER_RTL_I64_C(X)                 X##LL
  #define __SEGGER_RTL_U64_C(X)                 X##uLL
  //
  #define __SEGGER_RTL_SIZEOF_INT               8
  #define __SEGGER_RTL_SIZEOF_LONG              8
  #define __SEGGER_RTL_SIZEOF_PTR               8
  #define __SEGGER_RTL_PTRDIFF_T                __PTRDIFF_TYPE__
  #define __SEGGER_RTL_SIZE_T                   __SIZE_TYPE__
  //
  #if defined(__GNUC__) || defined(__clang__)
    #define __SEGGER_RTL_PTRDIFF_T              __PTRDIFF_TYPE__
    #define __SEGGER_RTL_SIZE_T                 __SIZE_TYPE__
    #define __SEGGER_RTL_WINT_T                 __WINT_TYPE__
    #define __SEGGER_RTL_WCHAR_T                __WCHAR_TYPE__
    #define __SEGGER_RTL_SIZEOF_WCHAR_T         __SIZEOF_WCHAR_T__
  #else
    #define __SEGGER_RTL_PTRDIFF_T              unsigned
    #define __SEGGER_RTL_SIZE_T                 unsigned
  #endif
#else
  //
  #error Expect __SEGGER_RTL_TYPESET to be set!
  //
#endif

//
// Configuration of long double size; default to binary64.
//
#ifndef   __SEGGER_RTL_SIZEOF_LDOUBLE
  #define __SEGGER_RTL_SIZEOF_LDOUBLE           8
#endif

// -2 - Favor size at the expense of speed
// -1 - Favor size over speed
//  0 - Balanced
// +1 - Favor speed over size
// +2 - Favor speed at the expense of size
//
#ifndef   __SEGGER_RTL_OPTIMIZE
  #define __SEGGER_RTL_OPTIMIZE                 0
#endif

#ifndef   __SEGGER_RTL_FORMAT_INT_WIDTH
  #define __SEGGER_RTL_FORMAT_INT_WIDTH         __WIDTH_LONG_LONG
#endif

#ifndef   __SEGGER_RTL_FORMAT_FLOAT_WIDTH
  #define __SEGGER_RTL_FORMAT_FLOAT_WIDTH       __WIDTH_DOUBLE
#endif

#ifndef   __SEGGER_RTL_FORMAT_WCHAR
  #define __SEGGER_RTL_FORMAT_WCHAR             1
#endif

#ifndef   __SEGGER_RTL_FORMAT_WIDTH_PRECISION
  #define __SEGGER_RTL_FORMAT_WIDTH_PRECISION   1
#endif

#ifndef   __SEGGER_RTL_FORMAT_CHAR_CLASS
  #define __SEGGER_RTL_FORMAT_CHAR_CLASS        1
#endif

#if __SEGGER_RTL_FORMAT_FLOAT_WIDTH
  #undef  __SEGGER_RTL_FORMAT_INT_WIDTH
  #define __SEGGER_RTL_FORMAT_INT_WIDTH         __WIDTH_LONG_LONG
  #undef  __SEGGER_RTL_FORMAT_WIDTH_PRECISION
  #define __SEGGER_RTL_FORMAT_WIDTH_PRECISION   1
#endif

#ifndef   __SEGGER_RTL_MINIMAL_LOCALE
  #define __SEGGER_RTL_MINIMAL_LOCALE           0
#endif

#ifndef   __SEGGER_RTL_HEAP_SIZE
  #define __SEGGER_RTL_HEAP_SIZE                1024
#endif

#ifndef   __SEGGER_RTL_ATEXIT_COUNT
  #define __SEGGER_RTL_ATEXIT_COUNT             1
#endif

#ifndef   __SEGGER_RTL_STDOUT_BUFFER_LEN
  #define __SEGGER_RTL_STDOUT_BUFFER_LEN        64
#endif

#ifndef   __SEGGER_RTL_THREAD
  #define __SEGGER_RTL_THREAD
#endif

#ifndef  __SEGGER_RTL_BYTE_ORDER
  #error __SEGGER_RTL_BYTE_ORDER is not configured
#endif

#ifndef  __SEGGER_RTL_SIZE_T
  #error __SEGGER_RTL_SIZE_T is not configured
#endif

#ifndef  __SEGGER_RTL_MAX_ALIGN
  #error __SEGGER_RTL_MAX_ALIGN is not configured
#endif

#ifndef  __SEGGER_RTL_WCHAR_T
  #error __SEGGER_RTL_WCHAR_T is not configured
#endif

#if !defined(__SEGGER_RTL_WINT_T)
  #error __SEGGER_RTL_WINT_T is not configured
#endif

#ifndef  __SEGGER_RTL_FP_ABI
  #error __SEGGER_RTL_FP_ABI is not configured
#endif

#ifndef   __SEGGER_RTL_FP_HW
  #define __SEGGER_RTL_FP_HW                    0
#endif

#ifndef   __SEGGER_RTL_NAN_FORMAT
  #define __SEGGER_RTL_NAN_FORMAT               __SEGGER_RTL_NAN_FORMAT_IEEE
#endif

#if !defined(__SEGGER_RTL_SIZEOF_LONG) || (__SEGGER_RTL_SIZEOF_LONG != 4 && __SEGGER_RTL_SIZEOF_LONG != 8)
  #error __SEGGER_RTL_SIZEOF_LONG is not configured or not configured correctly
#endif

#if !defined(__SEGGER_RTL_SIZEOF_PTR) || (__SEGGER_RTL_SIZEOF_PTR != 2 && __SEGGER_RTL_SIZEOF_PTR != 4 && __SEGGER_RTL_SIZEOF_PTR != 8)
  #error __SEGGER_RTL_SIZEOF_PTR is not configured or not configured correctly
#endif

#if !defined(__SEGGER_RTL_SIZEOF_WCHAR_T) || (__SEGGER_RTL_SIZEOF_WCHAR_T != 2 && __SEGGER_RTL_SIZEOF_WCHAR_T != 4)
  #error __SEGGER_RTL_SIZEOF_WCHAR_T is not configured or not configured correctly
#endif

#if __SEGGER_RTL_FORMAT_FLOAT_WIDTH != __WIDTH_NONE && __SEGGER_RTL_FORMAT_FLOAT_WIDTH != __WIDTH_DOUBLE && __SEGGER_RTL_FORMAT_FLOAT_WIDTH != __WIDTH_FLOAT
  #error __SEGGER_RTL_FORMAT_FLOAT_WIDTH is not configured or not configured correctly
#endif

#if __SEGGER_RTL_FORMAT_INT_WIDTH  < __WIDTH_INT || __WIDTH_LONG_LONG < __SEGGER_RTL_FORMAT_INT_WIDTH
  #error __SEGGER_RTL_FORMAT_INT_WIDTH is not configured or not configured correctly
#endif

#ifndef   __SEGGER_RTL_BitcastToU32
  #if defined(__clang) || defined(__SEGGER_CC)
    #define __SEGGER_RTL_BitcastToU32(X)        __builtin_bit_cast(__SEGGER_RTL_U32, (float)(X))
  #else
    #define __SEGGER_RTL_BitcastToU32(X)        __SEGGER_RTL_BitcastToU32_inline(X)
  #endif
#endif

#ifndef   __SEGGER_RTL_BitcastToF32
  #if defined(__clang) || defined(__SEGGER_CC)
    #define __SEGGER_RTL_BitcastToF32(X)        __builtin_bit_cast(float, (__SEGGER_RTL_U32)(X))
  #else
    #define __SEGGER_RTL_BitcastToF32(X)        __SEGGER_RTL_BitcastToF32_inline(X)
  #endif
#endif

#ifndef   __SEGGER_RTL_BitcastToU64
  #if defined(__clang) || defined(__SEGGER_CC)
    #define __SEGGER_RTL_BitcastToU64(X)        __builtin_bit_cast(__SEGGER_RTL_U64, (double)(X))
  #else
    #define __SEGGER_RTL_BitcastToU64(X)        __SEGGER_RTL_BitcastToU64_inline(X)
  #endif
#endif

#ifndef   __SEGGER_RTL_BitcastToF64
  #if defined(__clang) || defined(__SEGGER_CC)
    #define __SEGGER_RTL_BitcastToF64(X)        __builtin_bit_cast(double, (__SEGGER_RTL_U64)(X))
  #else
    #define __SEGGER_RTL_BitcastToF64(X)        __SEGGER_RTL_BitcastToF64_inline(X)
  #endif
#endif

#ifndef   __SEGGER_RTL_SMULL_X
  #define __SEGGER_RTL_SMULL_X(X, Y)            __SEGGER_RTL_SMULL_X_func((X), (Y))
  #define __SEGGER_RTL_SMULL_X_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_UMULL_X
  #define __SEGGER_RTL_UMULL_X(X, Y)            __SEGGER_RTL_UMULL_X_func((X), (Y))
  #define __SEGGER_RTL_UMULL_X_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_SMULL_HI
  #define __SEGGER_RTL_SMULL_HI(X, Y)           __SEGGER_RTL_SMULL_HI_func((X), (Y))
  #define __SEGGER_RTL_SMULL_HI_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_UMULL_HI
  #define __SEGGER_RTL_UMULL_HI(X, Y)           __SEGGER_RTL_UMULL_HI_func((X), (Y))
  #define __SEGGER_RTL_UMULL_HI_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_SMULL
  #define __SEGGER_RTL_SMULL(L,H,X,Y)           __SEGGER_RTL_SMULL_func(&(L), &(H), (X), (Y))
  #define __SEGGER_RTL_SMULL_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_UMULL
  #define __SEGGER_RTL_UMULL(L,H,X,Y)           __SEGGER_RTL_UMULL_func(&(L), &(H), (X), (Y))
  #define __SEGGER_RTL_UMULL_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_SMLAL
  #define __SEGGER_RTL_SMLAL(L,H,X,Y)           __SEGGER_RTL_SMLAL_func(&(L), &(H), (X), (Y))
  #define __SEGGER_RTL_SMLAL_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_UMLAL
  #define __SEGGER_RTL_UMLAL(L,H,X,Y)           __SEGGER_RTL_UMLAL_func(&(L), &(H), (X), (Y))
  #define __SEGGER_RTL_UMLAL_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_FLT_SELECT
  #define __SEGGER_RTL_FLT_SELECT(HEX, DEC)     DEC
#endif

#ifndef   __SEGGER_RTL_DIVMOD_U32
  #define __SEGGER_RTL_DIVMOD_U32(Q, R, N, D)   do { Q = N / D; R = N - Q*D; } while (0)
#endif

#ifndef   __SEGGER_RTL_DIVMOD_U64
  #define __SEGGER_RTL_DIVMOD_U64(Q, R, N, D)   do { Q = N / D; R = N - Q*D; } while (0)
#endif

#ifndef   __SEGGER_RTL_NEVER_INLINE
  #define __SEGGER_RTL_NEVER_INLINE
#endif

#ifndef   __SEGGER_RTL_ALWAYS_INLINE
  #define __SEGGER_RTL_ALWAYS_INLINE
#endif

#ifndef   __SEGGER_RTL_REQUEST_INLINE
  #define __SEGGER_RTL_REQUEST_INLINE
#endif

#ifndef   __SEGGER_RTL_UNLIKELY
  #define __SEGGER_RTL_UNLIKELY(X)              (X)
#endif

#ifndef   __SEGGER_RTL_LIKELY
  #define __SEGGER_RTL_LIKELY(X)                (X)
#endif

#ifndef   __SEGGER_RTL_RODATA_IS_RW
  #define __SEGGER_RTL_RODATA_IS_RW             0
#endif

#ifndef   __SEGGER_RTL_USE_PARA                             // Some compiler complain about unused parameters.
  #define __SEGGER_RTL_USE_PARA(Para)           (void)Para  // This works for most compilers.
#endif

#ifndef   __SEGGER_RTL_SIDE_BY_SIZE
  #define __SEGGER_RTL_SIDE_BY_SIZE             0
#endif

#ifndef   __SEGGER_RTL_SPECIALIZE_COMPARES
  #define __SEGGER_RTL_SPECIALIZE_COMPARES      1
#endif

#ifndef   __SEGGER_RTL_PUBLIC_API
  #define __SEGGER_RTL_PUBLIC_API
#endif

#ifndef   __SEGGER_RTL_CLZ_U32
  #define __SEGGER_RTL_CLZ_U32(X)               __SEGGER_RTL_CLZ_U32_inline(X)
  #define __SEGGER_RTL_CLZ_U32_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_CLZ_U64
  #define __SEGGER_RTL_CLZ_U64(X)               __SEGGER_RTL_CLZ_U64_inline(X)
  #define __SEGGER_RTL_CLZ_U64_SYNTHESIZED
#endif

#ifndef   __SEGGER_RTL_FAST_CODE_SECTION
  #define __SEGGER_RTL_FAST_CODE_SECTION(X)
#endif

#ifndef   __SEGGER_RTL_SCALED_INTEGER
  #define __SEGGER_RTL_SCALED_INTEGER           0
#endif

#ifndef   __SEGGER_RTL_FILE_IMPL
  #define __SEGGER_RTL_FILE_IMPL                __SEGGER_RTL_FILE_impl
#endif

#ifndef   __SEGGER_RTL_U64_H
  #define __SEGGER_RTL_U64_H(X)                 ((__SEGGER_RTL_U32)((__SEGGER_RTL_U64)(X) >> 32))
#endif
#ifndef   __SEGGER_RTL_U64_L
  #define __SEGGER_RTL_U64_L(X)                 ((__SEGGER_RTL_U32)(__SEGGER_RTL_U64)(X))
#endif
#ifndef   __SEGGER_RTL_U64_MK
  #define __SEGGER_RTL_U64_MK(H, L)             (((__SEGGER_RTL_U64)(__SEGGER_RTL_U32)(H) << 32) + (__SEGGER_RTL_U32)(L))
#endif
#ifndef   __SEGGER_RTL_I64_H
  #define __SEGGER_RTL_I64_H(X)                 ((__SEGGER_RTL_I32)((__SEGGER_RTL_I64)(X) >> 32))
#endif
#ifndef   __SEGGER_RTL_I64_L
  #define __SEGGER_RTL_I64_L(X)                 ((__SEGGER_RTL_U32)(__SEGGER_RTL_U64)(X))
#endif

//
// If complex types are not supported by the compiler, synthesize them
// with a pseudo-complex represented as a structure.
//
#ifndef   __SEGGER_RTL_FLOAT32_C_COMPLEX
  #define __SEGGER_RTL_FLOAT32_C_COMPLEX        __SEGGER_RTL_FLOAT32_PSEUDO_COMPLEX
#endif

#ifndef   __SEGGER_RTL_FLOAT64_C_COMPLEX
  #define __SEGGER_RTL_FLOAT64_C_COMPLEX        __SEGGER_RTL_FLOAT64_PSEUDO_COMPLEX
#endif

//
// GCC and clang provide a built-in support for some math constants.
//
#if defined(__GNUC__) || defined(__clang__)
  #ifndef   __SEGGER_RTL_INFINITY
    #define __SEGGER_RTL_INFINITY               __builtin_inff()
  #endif
  
  #ifndef   __SEGGER_RTL_NAN 
    #define __SEGGER_RTL_NAN                    __builtin_nanf("0x7fc00000")
  #endif
  
  #ifndef   __SEGGER_RTL_HUGE_VAL
    #define __SEGGER_RTL_HUGE_VAL               __builtin_huge_val()
  #endif
  
  #ifndef   __SEGGER_RTL_HUGE_VALF
    #define __SEGGER_RTL_HUGE_VALF              __builtin_huge_valf()
  #endif
#endif

//
// Sanity checks
//
#if (SEGGER_RTL_INCLUDE_GNU_API == 2) && (defined(__ARM_ARCH_ISA_ARM) || defined(__ARM_ARCH_ISA_THUMB))
  #error The GNU API is not supported for the Arm architecture with assembly speedups
#endif

#if (__SEGGER_RTL_SIDE_BY_SIDE > 0) && (__SEGGER_RTL_INCLUDE_SEGGER_API == 0)
  #error __SEGGER_RTL_SIDE_BY_SIDE selected requires __SEGGER_RTL_INCLUDE_SEGGER_API selected
#endif

// Private macros.
#define __SEGGER_RTL_HIDE(X)                    __c_##X

#if __SEGGER_RTL_RODATA_IS_RW
  #define __SEGGER_RTL_RODATA
#else
  #define __SEGGER_RTL_RODATA                   const
#endif

// Private configuration.  This is work in progress and cannot be
// otherwise configured.
#define __SEGGER_RTL_SUBNORMALS_READ_AS_ZERO    1  // Incoming subnormals are read as a signed zero.
#define __SEGGER_RTL_SUBNORMALS_FLUSH_TO_ZERO   1  // Outgoing subnormals are generated as a signed zero.

#if __SEGGER_RTL_STDOUT_BUFFER_LEN <= 0
  #error Bad configuration of __SEGGER_RTL_STDOUT_BUFFER_LEN
#endif

#if __SEGGER_RTL_ATEXIT_COUNT <= 0
  #error Bad configuration of __SEGGER_RTL_ATEXIT_COUNT
#endif

#ifndef   __SEGGER_RTL_CORE_HAS_UDIV_X
  #define __SEGGER_RTL_CORE_HAS_UDIV_X          0
#endif

#ifndef   __SEGGER_RTL_CORE_HAS_IDIV_X
  #define __SEGGER_RTL_CORE_HAS_IDIV_X          0
#endif

#ifndef   __SEGGER_RTL_CORE_HAS_UDIVM_X
  #define __SEGGER_RTL_CORE_HAS_UDIVM_X         0
#endif

#ifndef   __SEGGER_RTL_CORE_HAS_IDIVM_X
  #define __SEGGER_RTL_CORE_HAS_IDIVM_X         0
#endif

#ifndef   __SEGGER_RTL_NO_BUILTIN
  #error  __SEGGER_RTL_NO_BUILTIN must be defined for your target compiler!
#endif

#endif

/*************************** End of file ****************************/
