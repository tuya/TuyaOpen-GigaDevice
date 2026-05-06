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

#ifndef __SEGGER_RTL_FLOAT_H
#define __SEGGER_RTL_FLOAT_H

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       Common parameters
*
*  Description
*    Applies to single-precision and double-precision formats.
*/
#define FLT_ROUNDS                1  // Rounding mode of floating-point addition is round to nearest.
#define FLT_EVAL_METHOD           0  // All operations and constants are evaluated to the range and precision of the type.
#define FLT_RADIX                 2  // Radix of the exponent representation.
#define DECIMAL_DIG              17  // Number of decimal digits that can be rounded to a floating-point number without change to the value.

/*********************************************************************
*
*       Float parameters
*
*  Description
*    IEEE 32-bit single-precision floating format parameters.
*/
#define FLT_MANT_DIG             24  // Number of base FLT_RADIX digits in the mantissa part of a float.
#define FLT_EPSILON 1.19209290E-07f  // Minimum positive number such that 1.0f + FLT_EPSILON != 1.0f.
#define FLT_DIG                   6  // Number of decimal digits of precision of a float.
#define FLT_MIN_EXP            -125  // Minimum value of base FLT_RADIX in the exponent part of a float.
#define FLT_MIN     1.17549435E-38f  // Minimum value of a float.
#define FLT_MIN_10_EXP          -37  // Minimum value in base 10 of the exponent part of a float.
#define FLT_MAX_EXP            +128  // Maximum value of base FLT_RADIX in the exponent part of a float.
#define FLT_MAX     3.40282347E+38f  // Maximum value of a float.
#define FLT_MAX_10_EXP          +38  // Maximum value in base 10 of the exponent part of a float.

/*********************************************************************
*
*       Double parameters
*
*  Description
*    IEEE 64-bit double-precision floating format parameters.
*/
#define DBL_MANT_DIG                    53  // Number of base DBL_RADIX digits in the mantissa part of a double.
#define DBL_EPSILON 2.2204460492503131E-16  // Minimum positive number such that 1.0 + DBL_EPSILON != 1.0.
#define DBL_DIG                         15  // Number of decimal digits of precision of a double.
#define DBL_MIN_EXP                  -1021  // Minimum value of base DBL_RADIX in the exponent part of a double.
#define DBL_MIN    2.2250738585072014E-308  // Minimum value of a double.
#define DBL_MIN_10_EXP                -307  // Minimum value in base 10 of the exponent part of a double.
#define DBL_MAX_EXP                  +1024  // Maximum value of base DBL_RADIX in the exponent part of a double.
#define DBL_MAX    1.7976931348623157E+308  // Maximum value of a double.
#define DBL_MAX_10_EXP                +308  // Maximum value in base 10 of the exponent part of a double.

/*********************************************************************
*
*       Long-double parameters
*
*  Description
*    Choose between 64-bit and 128-bit long double.
*/
#if __SEGGER_RTL_SIZEOF_LDOUBLE == 8
#define LDBL_MANT_DIG                    53  // Number of base LDBL_RADIX digits in the mantissa part of a long double.
#define LDBL_EPSILON 2.2204460492503131E-16  // Minimum positive number such that 1.0 + LDBL_EPSILON != 1.0.
#define LDBL_DIG                         15  // Number of decimal digits of precision of a long double.
#define LDBL_MIN_EXP                  -1021  // Minimum value of base LDBL_RADIX in the exponent part of a long double.
#define LDBL_MIN    2.2250738585072014E-308  // Minimum value of a long double.
#define LDBL_MIN_10_EXP                -307  // Minimum value in base 10 of the exponent part of a long double.
#define LDBL_MAX_EXP                  +1024  // Maximum value of base LDBL_RADIX in the exponent part of a long double.
#define LDBL_MAX    1.7976931348623157E+308  // Maximum value of a long double.
#define LDBL_MAX_10_EXP                +308  // Maximum value in base 10 of the exponent part of a long double.
#else
#define LDBL_MANT_DIG                                       113   // Number of base LDBL_RADIX digits in the mantissa part of a long double.
#define LDBL_EPSILON 1. 92592994438723585305597794258492732e-34L  // Minimum positive number such that 1.0 + LDBL_EPSILON != 1.0.
#define LDBL_DIG                                             33   // Number of decimal digits of precision of a long double.
#define LDBL_MIN_EXP                                    (-16381)  // Minimum value of base LDBL_RADIX in the exponent part of a long double.
#define LDBL_MIN    3.36210314311209350626267781732175260e-4932L  // Minimum value of a long double.
#define LDBL_MIN_10_EXP                                  (-4931)  // Minimum value in base 10 of the exponent part of a long double.
#define LDBL_MAX_EXP                                      16384   // Maximum value of base LDBL_RADIX in the exponent part of a long double.
#define LDBL_MAX    1.18973149535723176508575932662800702e+4932L  // Maximum value of a long double.
#define LDBL_MAX_10_EXP                                    4932   // Maximum value in base 10 of the exponent part of a long double.
#endif

#endif

/*************************** End of file ****************************/
