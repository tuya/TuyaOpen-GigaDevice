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

#ifndef __SEGGER_RTL_LIMITS_H
#define __SEGGER_RTL_LIMITS_H

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "__SEGGER_RTL_ConfDefaults.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       Character minima and maxima
*
*  Description
*    Minimum and maximum values for character types.
*/
#define CHAR_BIT    8                              // Number of bits for smallest object that is not a bit-field (byte).
#define CHAR_MIN    0                              // Minimum value of a plain character.
#define CHAR_MAX    255                            // Maximum value of a plain character.
#define SCHAR_MAX   127                            // Maximum value of a signed character.
#define SCHAR_MIN   (-128)                         // Minimum value of a signed character.
#define UCHAR_MAX   255                            // Maximum value of an unsigned character.

/*********************************************************************
*
*       Short integer minima and maxima
*
*  Description
*    Minimum and maximum values for short integer types.
*/
#define SHRT_MIN    (-32767 - 1)                   // Minimum value of a short integer.
#define SHRT_MAX    32767                          // Maximum value of a short integer.
#define USHRT_MAX   65535                          // Maximum value of an unsigned short integer.

/*********************************************************************
*
*       Integer minima and maxima
*
*  Description
*    Minimum and maximum values for integer types.
*/
#define INT_MIN	    (-2147483647 - 1)              // Minimum value of an integer.
#define INT_MAX     2147483647                     // Maximum value of an integer.
#define UINT_MAX    4294967295u                    // Maximum value of an unsigned integer.

#if __SEGGER_RTL_SIZEOF_LONG == 4
/*********************************************************************
*
*       Long integer minima and maxima (32-bit)
*
*  Description
*    Minimum and maximum values for long integer types.
*/
#define LONG_MIN    (-2147483647L - 1)             // Maximum value of a long integer.
#define LONG_MAX    2147483647L                    // Minimum value of a long integer.
#define ULONG_MAX   4294967295uL                   // Maximum value of an unsigned long integer.

#else

/*********************************************************************
*
*       Long integer minima and maxima (64-bit)
*
*  Description
*    Minimum and maximum values for long integer types.
*/
#define LONG_MIN   (-9223372036854775807L - 1)     // Minimum value of a long integer.
#define LONG_MAX   9223372036854775807L            // Maximum value of a long integer.
#define ULONG_MAX  18446744073709551615uL          // Maximum value of an unsigned long integer.

#endif

/*********************************************************************
*
*       Long long integer minima and maxima
*
*  Description
*    Minimum and maximum values for long integer types.
*/
#define LLONG_MIN   (-9223372036854775807LL - 1)   // Minimum value of a long long integer.
#define LLONG_MAX   9223372036854775807LL          // Maximum value of a long long integer.
#define ULLONG_MAX  18446744073709551615uLL        // Maximum value of an unsigned long long integer.

/*********************************************************************
*
*       Multibyte characters
*
*  Description
*    Maximum number of bytes in a multi-byte character.
*
*  Additional information
*    The maximum number of bytes in a multi-byte character for any
*    supported locale. Unicode (ISO 10646) characters between 0x000000
*    and 0x10FFFF inclusive are supported which convert to a maximum
*    of four bytes in the UTF-8 encoding.
*/
#define MB_LEN_MAX  4                              // Maximum

#endif

/*************************** End of file ****************************/
