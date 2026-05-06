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

#ifndef __SEGGER_RTL_STDINT_H
#define __SEGGER_RTL_STDINT_H

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "__SEGGER_RTL.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       Signed integer minima and maxima
*
*  Description
*    Minimum and maximum values for signed integer types.
*/
#define INT8_MIN         (-128)                         // Minimum value of int8_t
#define INT8_MAX         127                            // Maximum value of int8_t
#define INT16_MIN        (-32767-1)                     // Minimum value of int16_t
#define INT16_MAX        32767                          // Maximum value of int16_t
#define INT32_MIN        (-2147483647L-1)               // Minimum value of int32_t
#define INT32_MAX        2147483647L                    // Maximum value of int32_t
#define INT64_MIN        (-9223372036854775807LL-1)     // Minimum value of int64_t
#define INT64_MAX        9223372036854775807LL          // Maximum value of int64_t

/*********************************************************************
*
*       Unsigned integer minima and maxima
*
*  Description
*    Minimum and maximum values for unsigned integer types.
*/
#define UINT8_MAX        255                            // Maximum value of uint8_t
#define UINT16_MAX       65535                          // Maximum value of uint16_t
#define UINT32_MAX       4294967295UL                   // Maximum value of uint32_t
#define UINT64_MAX       18446744073709551615ULL        // Maximum value of uint64_t

/*********************************************************************
*
*       Maximal integer minima and maxima
*
*  Description
*    Minimum and maximum values for signed and unsigned
*    maximal-integer types.
*/
#define INTMAX_MIN       INT64_MIN                      // Minimum value of intmax_t
#define INTMAX_MAX       INT64_MAX                      // Maximum value of intmax_t
#define UINTMAX_MAX      UINT64_MAX                     // Maximum value of uintmax_t

/*********************************************************************
*
*       Least integer minima and maxima
*
*  Description
*    Minimum and maximum values for signed and unsigned
*    least-integer types.
*/
#define INT_LEAST8_MIN   INT8_MIN     // Minimum value of int_least8_t
#define INT_LEAST8_MAX   INT8_MAX     // Maximum value of int_least8_t
#define INT_LEAST16_MIN  INT16_MIN    // Minimum value of int_least16_t
#define INT_LEAST16_MAX  INT16_MAX    // Maximum value of int_least16_t
#define INT_LEAST32_MIN  INT32_MIN    // Minimum value of int_least32_t
#define INT_LEAST32_MAX  INT32_MAX    // Maximum value of int_least32_t
#define INT_LEAST64_MIN  INT64_MIN    // Minimum value of int_least64_t
#define INT_LEAST64_MAX  INT64_MAX    // Maximum value of int_least64_t
#define UINT_LEAST8_MAX  UINT8_MAX    // Maximum value of uint_least8_t
#define UINT_LEAST16_MAX UINT16_MAX   // Maximum value of uint_least16_t
#define UINT_LEAST32_MAX UINT32_MAX   // Maximum value of uint_least32_t
#define UINT_LEAST64_MAX UINT64_MAX   // Maximum value of uint_least64_t

/*********************************************************************
*
*       Fast integer minima and maxima
*
*  Description
*    Minimum and maximum values for signed and unsigned
*    fast-integer types.
*/
#define INT_FAST8_MIN    INT8_MIN     // Minimum value of int_fast8_t
#define INT_FAST8_MAX    INT8_MAX     // Maximum value of int_fast8_t
#define INT_FAST16_MIN   INT32_MIN    // Minimum value of int_fast16_t
#define INT_FAST16_MAX   INT32_MAX    // Maximum value of int_fast16_t
#define INT_FAST32_MIN   INT32_MIN    // Minimum value of int_fast32_t
#define INT_FAST32_MAX   INT32_MAX    // Maximum value of int_fast32_t
#define INT_FAST64_MIN   INT64_MIN    // Minimum value of int_fast64_t
#define INT_FAST64_MAX   INT64_MAX    // Maximum value of int_fast64_t
#define UINT_FAST8_MAX   UINT8_MAX    // Maximum value of uint_fast8_t
#define UINT_FAST16_MAX  UINT32_MAX   // Maximum value of uint_fast16_t
#define UINT_FAST32_MAX  UINT32_MAX   // Maximum value of uint_fast32_t
#define UINT_FAST64_MAX  UINT64_MAX   // Maximum value of uint_fast64_t

/*********************************************************************
*
*       Pointer types minima and maxima
*
*  Description
*    Minimum and maximum values for pointer-related types.
*/
#if __SEGGER_RTL_SIZEOF_PTR == 4
#define PTRDIFF_MIN      INT32_MIN    // Minimum value of ptrdiff_t
#define PTRDIFF_MAX      INT32_MAX    // Maximum value of ptrdiff_t
#define SIZE_MAX         INT32_MAX    // Maximum value of size_t
#define INTPTR_MIN       INT32_MIN    // Minimum value of intptr_t
#define INTPTR_MAX       INT32_MAX    // Maximum value of intptr_t
#define UINTPTR_MAX      UINT32_MAX   // Maximum value of uintptr_t
#elif __SEGGER_RTL_SIZEOF_PTR == 8
#define PTRDIFF_MIN      INT64_MIN    // Minimum value of ptrdiff_t
#define PTRDIFF_MAX      INT64_MAX    // Maximum value of ptrdiff_t
#define SIZE_MAX         INT64_MAX    // Maximum value of size_t
#define INTPTR_MIN       INT64_MIN    // Minimum value of intptr_t
#define INTPTR_MAX       INT64_MAX    // Maximum value of intptr_t
#define UINTPTR_MAX      UINT64_MAX   // Maximum value of uintptr_t
#endif

/*********************************************************************
*
*       Wide integer minima and maxima
*
*  Description
*    Minimum and maximum values for the wint_t type.
*/
#define WINT_MIN   (-2147483647L-1)   // Minimum value of wint_t
#define WINT_MAX   2147483647L        // Maximum value of wint_t

/*********************************************************************
*
*       Signed integer construction macros
*
*  Description
*    Macros that create constants of type intx_t.
*/
#define INT8_C(x)        (x)          // Create constant of type int8_t
#define INT16_C(x)       (x)          // Create constant of type int16_t
#define INT32_C(x)       (x)          // Create constant of type int32_t
#define INT64_C(x)       (x##LL)      // Create constant of type int64_t

/*********************************************************************
*
*       Unsigned integer construction macros
*
*  Description
*    Macros that create constants of type uintx_t.
*/
#define UINT8_C(x)       (x##u)       // Create constant of type uint8_t
#define UINT16_C(x)      (x##u)       // Create constant of type uint16_t
#define UINT32_C(x)      (x##u)       // Create constant of type uint32_t
#define UINT64_C(x)      (x##uLL)     // Create constant of type uint64_t

/*********************************************************************
*
*       Maximal integer construction macros
*
*  Description
*    Macros that create constants of type intmax_t and uintmax_t.
*/
#define INTMAX_C(x)      (x##LL)      // Create constant of type intmax_t
#define UINTMAX_C(x)     (x##uLL)     // Create constant of type uintmax_t

/*********************************************************************
*
*       Wide character minima and maxima
*
*  Description
*    Minimum and maximum values for the wchar_t type.
*/
#if __SEGGER_RTL_SIZEOF_WCHAR_T == 2

#define WCHAR_MIN  0u
#define WCHAR_MAX  65535u

#else

#define WCHAR_MIN  (-2147483647L-1)
#define WCHAR_MAX  2147483647L

#endif

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

typedef signed   char      int8_t;
typedef unsigned char      uint8_t;

typedef signed   short     int16_t;
typedef unsigned short     uint16_t;
typedef signed   int       int32_t;
typedef unsigned int       uint32_t;

typedef signed   long long int64_t;
typedef unsigned long long uint64_t;

typedef int8_t             int_least8_t;
typedef int16_t            int_least16_t;
typedef int32_t            int_least32_t;
typedef int64_t            int_least64_t;

typedef uint8_t            uint_least8_t;
typedef uint16_t           uint_least16_t;
typedef uint32_t           uint_least32_t;
typedef uint64_t           uint_least64_t;

typedef int32_t            int_fast8_t;
typedef int32_t            int_fast16_t;
typedef int32_t            int_fast32_t;
typedef int64_t            int_fast64_t;
    
typedef uint32_t           uint_fast8_t;
typedef uint32_t           uint_fast16_t;
typedef uint32_t           uint_fast32_t;
typedef uint64_t           uint_fast64_t;

#if __SEGGER_RTL_SIZEOF_PTR == 4
typedef int32_t            intptr_t;
typedef uint32_t           uintptr_t;
#elif __SEGGER_RTL_SIZEOF_PTR == 8
typedef int64_t            intptr_t;
typedef uint64_t           uintptr_t;
#endif

typedef int64_t            intmax_t;
typedef uint64_t           uintmax_t;

#endif

/*************************** End of file ****************************/
