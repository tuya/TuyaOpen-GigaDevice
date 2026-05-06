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

#ifndef __SEGGER_RTL_MATH_H
#define __SEGGER_RTL_MATH_H

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "__SEGGER_RTL_FP.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define MATH_ERRNO        1
#define MATH_ERREXCEPT    2

// Indicate math calls do not set errno and do not raise exceptions.
#define math_errhandling  0

#define FP_ILOGB0           (int)((~0u >> 1) + 1u)   // INT_MIN
#define FP_ILOGBNAN         (int)(~0u >> 1)          // INT_MAX

#ifdef __cplusplus
extern "C" {
#endif

#define FP_ZERO             __SEGGER_RTL_FP_ZERO
#define FP_SUBNORMAL        __SEGGER_RTL_FP_SUBNORMAL
#define FP_NORMAL           __SEGGER_RTL_FP_NORMAL
#define FP_INFINITE         __SEGGER_RTL_FP_INFINITE
#define FP_NAN              __SEGGER_RTL_FP_NAN

#define INFINITY            __SEGGER_RTL_INFINITY
#define NAN                 __SEGGER_RTL_NAN
#define HUGE_VAL            __SEGGER_RTL_HUGE_VAL
#define HUGE_VALF           __SEGGER_RTL_HUGE_VALF

#define isinf(x)            (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_isinf(x)    : __SEGGER_RTL_float64_isinf(x))
#define isnan(x)            (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_isnan(x)    : __SEGGER_RTL_float64_isnan(x))
#define isfinite(x)         (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_isfinite(x) : __SEGGER_RTL_float64_isfinite(x))
#define isnormal(x)         (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_isnormal(x) : __SEGGER_RTL_float64_isnormal(x))
#define signbit(x)          (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_signbit(x)  : __SEGGER_RTL_float64_signbit(x))
#define fpclassify(x)       (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_classify(x) : __SEGGER_RTL_float64_classify(x))

#define isgreater(x,y)      (!isunordered(x, y) && (x) > (y))
#define isgreaterequal(x,y) (!isunordered(x, y) && (x) >= (y))
#define isless(x,y)         (!isunordered(x, y) && (x) < (y))
#define islessequal(x,y)    (!isunordered(x, y) && (x) <= (y))
#define islessgreater(x,y)  (!isunordered(x, y) && (x) != (y))
#define isunordered(x,y)    (isnan(x) || isnan(y))

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

typedef float  float_t;
typedef double double_t;

/*********************************************************************
*
*       Prototypes (internal API)
*
**********************************************************************
*/

int         __SEGGER_RTL_float32_isinf    (float);
int         __SEGGER_RTL_float64_isinf    (double);
int         __SEGGER_RTL_float32_isnan    (float);
int         __SEGGER_RTL_float64_isnan    (double);
int         __SEGGER_RTL_float32_isfinite (float);
int         __SEGGER_RTL_float64_isfinite (double);
int         __SEGGER_RTL_float32_isnormal (float);
int         __SEGGER_RTL_float64_isnormal (double);
int         __SEGGER_RTL_float32_signbit  (float);
int         __SEGGER_RTL_float64_signbit  (double);
int         __SEGGER_RTL_float32_classify (float);
int         __SEGGER_RTL_float64_classify (double);

/*********************************************************************
*
*       Prototypes (public API, float)
*
**********************************************************************
*/

/*********************************************************************
*
*       Circular functions, float.
*/
float       cosf       (float __x);
float       sinf       (float __x);
float       tanf       (float __x);
float       asinf      (float __x);
float       acosf      (float __x);
float       atanf      (float __x);
float       atan2f     (float __y, float __x);
void        sincosf    (float __x, float *pSin, float *pCos);

/*********************************************************************
*
*       Hyperbolic functions, float.
*/
float       sinhf      (float __x);
float       coshf      (float __x);
float       tanhf      (float __x);
float       acoshf     (float __x);
float       asinhf     (float __x);
float       atanhf     (float __x);

/*********************************************************************
*
*       Logarithms, exponential, and power functions, float.
*/
float       logf       (float __x);
float       log2f      (float __x);
float       log10f     (float __x);
float       logbf      (float __x);
float       log1pf     (float __x);
int         ilogbf     (float __x);
float       expf       (float __x);
float       exp2f      (float __x);
float       exp10f     (float __x);
float       expm1f     (float __x);
float       sqrtf      (float __x);
float       cbrtf      (float __x);
float       rsqrtf     (float __x);
float       hypotf     (float __x, float __y);
float       powf       (float __x, float __y);

/*********************************************************************
*
*       Minimum, maximum, and positive difference functions, float.
*/
float       fminf      (float __x, float __y);
float       fmaxf      (float __x, float __y);
float       fdimf      (float __x, float __y);
float       fabsf      (float __x);

/*********************************************************************
*
*       Special functions, float.
*/
float       tgammaf    (float __x);
float       lgammaf    (float __x);
float       erff       (float __x);
float       erfcf      (float __x);

/*********************************************************************
*
*       Truncation functions, float.
*/
float       ceilf      (float __x);
float       floorf     (float __x);
float       truncf     (float __x);
float       rintf      (float __x);
long        lrintf     (float __x);
long long   llrintf    (float __x);
float       roundf     (float __x);
long        lroundf    (float __x);
long long   llroundf   (float __x);
float       nearbyintf (float __x);

/*********************************************************************
*
*       Manipulation functions, float.
*/
float       fmodf      (float __x, float __y);
float       remainderf (float __x, float __y);
float       remquof    (float __x, float __y, int *__quo);
float       modff      (float __x, float  *__iptr);
float       frexpf     (float __x, int *__exp);
float       ldexpf     (float __x, int __exp);
float       scalbnf    (float __x, int __exp);
float       scalblnf   (float __x, long __exp);
float       copysignf  (float __x, float __y);
float       nextafterf (float __x, float __y);
float       nexttowardf(float __x, long double __y);
float       nanf       (const char *__tag);

/*********************************************************************
*
*       Prototypes (public API, double)
*
**********************************************************************
*/

/*********************************************************************
*
*       Circular functions, double.
*/
double      sin        (double __x);
double      cos        (double __x);
double      tan        (double __x);
double      asin       (double __x);
double      acos       (double __x);
double      atan       (double __x);
double      atan2      (double __y, double __x);
void        sincos     (double __x, double *pSin, double *pCos);

/*********************************************************************
*
*       Hyperbolic functions, double.
*/
double      sinh       (double __x);
double      cosh       (double __x);
double      tanh       (double __x);
double      asinh      (double __x);
double      acosh      (double __x);
double      atanh      (double __x);

/*********************************************************************
*
*       Logarithms, exponential, and power functions, double.
*/
double      log        (double __x);
double      log2       (double __x);
double      log10      (double __x);
double      logb       (double __x);
double      log1p      (double __x);
int         ilogb      (double __x);
double      exp        (double __x);
double      exp2       (double __x);
double      exp10      (double __x);
double      expm1      (double __x);
double      sqrt       (double __x);
double      cbrt       (double __x);
double      rsqrt      (double __x);
double      hypot      (double __x, double __y);
double      pow        (double __x, double __y);

/*********************************************************************
*
*       Minimum, maximum, and positive difference functions, double.
*/
double      fmin       (double __x, double __y);
double      fmax       (double __x, double __y);
double      fdim       (double __x, double __y);
double      fabs       (double __x);

/*********************************************************************
*
*       Special functions, double.
*/
double      tgamma     (double __x);
double      lgamma     (double __x);
double      erf        (double __x);
double      erfc       (double __x);

/*********************************************************************
*
*       Truncation functions, double.
*/
double      ceil       (double __x);
double      floor      (double __x);
double      trunc      (double __x);
double      rint       (double __x);
long        lrint      (double __x);
long long   llrint     (double __x);
double      round      (double __x);
long        lround     (double __x);
long long   llround    (double __x);
double      nearbyint  (double __x);

/*********************************************************************
*
*       Manipulation functions, double.
*/
double      fmod       (double __x, double __y);
double      remainder  (double __x, double __y);
double      remquo     (double __x, double __y, int *__quo);
double      modf       (double __x, double *__iptr);
double      frexp      (double __x, int *__exp);
double      ldexp      (double __x, int __exp);
double      scalbn     (double __x, int __exp);
double      scalbln    (double __x, long __exp);
double      copysign   (double __x, double __y);
double      nextafter  (double __x, double __y);
double      nexttoward (double __x, long double __y);
double      nan        (const char *__tag);

/*********************************************************************
*
*       Prototypes (public API, double)
*
**********************************************************************
*/

/*********************************************************************
*
*       Circular functions, long double.
*/
long double sinl       (long double __x);
long double cosl       (long double __x);
long double tanl       (long double __x);
long double asinl      (long double __x);
long double acosl      (long double __x);
long double atanl      (long double __x);
long double atan2l     (long double __y, long double __x);
void        sincosl    (long double __x, long double *pSin, long double *pCos);

/*********************************************************************
*
*       Hyperbolic functions, long double.
*/
long double sinhl      (long double __x);
long double coshl      (long double __x);
long double tanhl      (long double __x);
long double asinhl     (long double __x);
long double acoshl     (long double __x);
long double atanhl     (long double __x);

/*********************************************************************
*
*       Logarithms, exponential, and power functions, long double.
*/
long double logl       (long double __x);
long double log2l      (long double __x);
long double log10l     (long double __x);
long double logbl      (long double __x);
long double log1pl     (long double __x);
int         ilogbl     (long double __x);
long double expl       (long double __x);
long double exp2l      (long double __x);
long double exp10l     (long double __x);
long double expm1l     (long double __x);
long double sqrtl      (long double __x);
long double cbrtl      (long double __x);
long double rsqrtl     (long double __x);
long double hypotl     (long double __x, long double __y);
long double powl       (long double __x, long double __y);

/*********************************************************************
*
*       Minimum, maximum, and positive difference functions, long double.
*/
long double fminl      (long double __x, long double __y);
long double fmaxl      (long double __x, long double __y);
long double fdiml      (long double __x, long double __y);
long double fabsl      (long double __x);

/*********************************************************************
*
*       Special functions, long double.
*/
long double tgammal    (long double __x);
long double lgammal    (long double __x);
long double erfl       (long double __x);
long double erfcl      (long double __x);

/*********************************************************************
*
*       Truncation functions, long double.
*/
long double ceill      (long double __x);
long double floorl     (long double __x);
long double truncl     (long double __x);
long double rintl      (long double __x);
long        lrintl     (long double __x);
long long   llrintl    (long double __x);
long double roundl     (long double __x);
long        lroundl    (long double __x);
long long   llroundl   (long double __x);
long double nearbyintl (long double __x);

/*********************************************************************
*
*       Manipulation functions, long double.
*/
long double fmodl      (long double __x, long double __y);
long double remainderl (long double __x, long double __y);
long double remquol    (long double __x, long double __y, int *__quo);
long double modfl      (long double __x, long double *__iptr);
long double frexpl     (long double __x, int *__exp);
long double ldexpl     (long double __x, int __exp);
long double scalbnl    (long double __x, int __exp);
long double scalblnl   (long double __x, long __exp);
long double copysignl  (long double __x, long double __y);
long double nextafterl (long double __x, long double __y);
long double nexttowardl(long double __x, long double __y);
long double nanl       (const char *__tag);

float       fmaf       (float  __x, float  __y, float  __z);
double      fma        (double __x, double __y, double __z);
long double fmal       (long double __x, long double __y, long double __z);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
