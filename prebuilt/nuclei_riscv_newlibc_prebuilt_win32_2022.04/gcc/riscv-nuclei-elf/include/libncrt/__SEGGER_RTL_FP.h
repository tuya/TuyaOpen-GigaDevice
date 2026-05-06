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

#ifndef __SEGGER_RTL_FP_H
#define __SEGGER_RTL_FP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "__SEGGER_RTL_ConfDefaults.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

//
// Floating-point library version number
//
#define            __SEGGER_FPL_VERSION      20800

//
// Internal floating-point classification constants
//
#define            __SEGGER_RTL_FP_ZERO      0x00
#define            __SEGGER_RTL_FP_SUBNORMAL 0x01
#define            __SEGGER_RTL_FP_NORMAL    0x02
#define            __SEGGER_RTL_FP_INFINITE  0x03
#define            __SEGGER_RTL_FP_NAN       0x04

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

#if __SEGGER_RTL_INCLUDE_C_API

#define            FP_ZERO                   __SEGGER_RTL_FP_ZERO
#define            FP_SUBNORMAL              __SEGGER_RTL_FP_SUBNORMAL
#define            FP_NORMAL                 __SEGGER_RTL_FP_NORMAL
#define            FP_INFINITE               __SEGGER_RTL_FP_INFINITE
#define            FP_NAN                    __SEGGER_RTL_FP_NAN

#define            isinf(x)                  (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_isinf(x)    : __SEGGER_RTL_float64_isinf(x))
#define            isnan(x)                  (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_isnan(x)    : __SEGGER_RTL_float64_isnan(x))
#define            isfinite(x)               (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_isfinite(x) : __SEGGER_RTL_float64_isfinite(x))
#define            isnormal(x)               (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_isnormal(x) : __SEGGER_RTL_float64_isnormal(x))
#define            signbit(x)                (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_signbit(x)  : __SEGGER_RTL_float64_signbit(x))
#define            fpclassify(x)             (sizeof(x) == sizeof(float) ? __SEGGER_RTL_float32_classify(x) : __SEGGER_RTL_float64_classify(x))
#define            isgreater(x,y)            (!isunordered(x, y) && (x) > (y))
#define            isgreaterequal(x,y)       (!isunordered(x, y) && (x) >= (y))
#define            isless(x,y)               (!isunordered(x, y) && (x) < (y))
#define            islessequal(x,y)          (!isunordered(x, y) && (x) <= (y))
#define            islessgreater(x,y)        (!isunordered(x, y) && (x) != (y))
#define            isunordered(x,y)          (isnan(x) || isnan(y))

float              sinf                      (float  __x);
double             sin                       (double __x);
float              cosf                      (float  __x);
double             cos                       (double __x);
float              tanf                      (float  __x);
double             tan                       (double __x);
float              asinf                     (float  __x);
double             asin                      (double __x);
float              acosf                     (float  __x);
double             acos                      (double __x);
float              atanf                     (float  __x);
double             atan                      (double __x);
float              atan2f                    (float  __y, float  __x);
double             atan2                     (double __y, double __x);
float              frexpf                    (float  __x, int *__exp);
double             frexp                     (double __x, int *__exp);
float              ldexpf                    (float  __x, int __exp);
double             ldexp                     (double __x, int __exp);
float              scalbnf                   (float  __x, int __exp);
double             scalbn                    (double __x, int __exp);
float              logf                      (float  __x);
double             log                       (double __x);
float              log1pf                    (float  __x);
double             log1p                     (double __x);
float              log10f                    (float  __x);
double             log10                     (double __x);
float              log2f                     (float  __x);
double             log2                      (double __x);
int                ilogbf                    (float  __x);
int                ilogb                     (double __x);
float              fmodf                     (float  __x, float  __y);
double             fmod                      (double __x, double __y);
float              modff                     (float  __x, float  *__iptr);
double             modf                      (double __x, double *__iptr);
float              powf                      (float  __x, float  __y);
double             pow                       (double __x, double __y);
float              sqrtf                     (float  __x);
double             sqrt                      (double __x);
float              cbrtf                     (float  __x);
double             cbrt                      (double __x);
float              rsqrtf                    (float  __x);
double             rsqrt                     (double __x);
float              ceilf                     (float  __x);
double             ceil                      (double __x);
float              fabsf                     (float  __x);
double             fabs                      (double __x);
float              fminf                     (float  __x, float  __y);
double             fmin                      (double __x, double __y);
float              fmaxf                     (float  __x, float  __y);
double             fmax                      (double __x, double __y);
float              floorf                    (float  __x);
double             floor                     (double __x);
float              hypotf                    (float  __x, float  __y);
double             hypot                     (double __x, double __y);
float              sinhf                     (float  __x);
double             sinh                      (double __x);
float              coshf                     (float  __x);
double             cosh                      (double __x);
float              tanhf                     (float  __x);
double             tanh                      (double __x);
float              acoshf                    (float  __x);
double             acosh                     (double __x);
float              asinhf                    (float  __x);
double             asinh                     (double __x);
float              atanhf                    (float  __x);
double             atanh                     (double __x);
float              fmaf                      (float  __x, float  __y, float  __z);
double             fma                       (double __x, double __y, double __z);
float              expf                      (float  __x);
double             exp                       (double __x);
float              exp2f                     (float  __x);
double             exp2                      (double __x);
float              exp10f                    (float  __x);
double             exp10                     (double __x);

#endif

//
// SEGGER API
//

#if __SEGGER_RTL_INCLUDE_SEGGER_API

#define            SEGGER_FP_ZERO            __SEGGER_RTL_FP_ZERO
#define            SEGGER_FP_SUBNORMAL       __SEGGER_RTL_FP_SUBNORMAL
#define            SEGGER_FP_NORMAL          __SEGGER_RTL_FP_NORMAL
#define            SEGGER_FP_INFINITE        __SEGGER_RTL_FP_INFINITE
#define            SEGGER_FP_NAN             __SEGGER_RTL_FP_NAN

double             SEGGER_add                (double x, double y);
float              SEGGER_addf               (float  x, float  y);
double             SEGGER_sub                (double x, double y);
float              SEGGER_subf               (float  x, float  y);
double             SEGGER_mul                (double x, double y);
float              SEGGER_mulf               (float  x, float  y);
double             SEGGER_div                (double x, double y);
float              SEGGER_divf               (float  x, float  y);
double             SEGGER_neg                (double x);
float              SEGGER_negf               (float  x);
double             SEGGER_fma                (double x, double y, double z);
float              SEGGER_fmaf               (float  x, float  y, float  z);
double             SEGGER_fms                (double x, double y, double z);
float              SEGGER_fmsf               (float  x, float  y, float  z);

int                SEGGER_lt                 (double x, double y);
int                SEGGER_ltf                (float  x, float  y);
int                SEGGER_le                 (double x, double y);
int                SEGGER_lef                (float  x, float  y);
int                SEGGER_gt                 (double x, double y);
int                SEGGER_gtf                (float  x, float  y);
int                SEGGER_ge                 (double x, double y);
int                SEGGER_gef                (float  x, float  y);
int                SEGGER_eq                 (double x, double y);
int                SEGGER_eqf                (float  x, float  y);
int                SEGGER_ne                 (double x, double y);
int                SEGGER_nef                (float  x, float  y);

//
// Classification
//
int                SEGGER_isinff             (float);
int                SEGGER_isinf              (double);
int                SEGGER_isnanf             (float);
int                SEGGER_isnan              (double);
int                SEGGER_isfinitef          (float);
int                SEGGER_isfinite           (double);
int                SEGGER_isnormalf          (float);
int                SEGGER_isnormal           (double);
int                SEGGER_signbitf           (float);
int                SEGGER_signbit            (double);
int                SEGGER_classifyf          (float);
int                SEGGER_classify           (double);

//
// Conversions to/from fixed-size integer types
//
__SEGGER_RTL_I32   SEGGER_float_to_int32     (float x);
__SEGGER_RTL_I32   SEGGER_double_to_int32    (double x);
float              SEGGER_int32_to_float     (__SEGGER_RTL_I32 x);
double             SEGGER_int32_to_double    (__SEGGER_RTL_I32 x);
//
__SEGGER_RTL_U32   SEGGER_float_to_uint32    (float x);
__SEGGER_RTL_U32   SEGGER_double_to_uint32   (double x);
float              SEGGER_uint32_to_float    (__SEGGER_RTL_U32 x);
double             SEGGER_uint32_to_double   (__SEGGER_RTL_U32 x);
//
__SEGGER_RTL_I64   SEGGER_float_to_int64     (float x);
__SEGGER_RTL_I64   SEGGER_double_to_int64    (double x);
float              SEGGER_int64_to_float     (__SEGGER_RTL_I64 x);
double             SEGGER_int64_to_double    (__SEGGER_RTL_I64 x);
//
__SEGGER_RTL_U64   SEGGER_float_to_uint64    (float x);
__SEGGER_RTL_U64   SEGGER_double_to_uint64   (double x);
float              SEGGER_uint64_to_float    (__SEGGER_RTL_U64 x);
double             SEGGER_uint64_to_double   (__SEGGER_RTL_U64 x);
//
float              SEGGER_uint64_to_float    (__SEGGER_RTL_U64 x);
double             SEGGER_uint64_to_double   (__SEGGER_RTL_U64 x);

//
// Conversions to/from standard integer types
//
float              SEGGER_int_to_float       (int x);
int                SEGGER_float_to_int       (float x);
double             SEGGER_int_to_double      (int x);
int                SEGGER_double_to_int      (double x);
float              SEGGER_uint_to_float      (unsigned x);
unsigned           SEGGER_float_to_uint      (float x);
double             SEGGER_uint_to_double     (unsigned x);
unsigned           SEGGER_double_to_uint     (double x);

float              SEGGER_long_to_float      (long x);
long               SEGGER_float_to_long      (float x);
double             SEGGER_long_to_double     (long x);
long               SEGGER_double_to_long     (double x);
float              SEGGER_ulong_to_float     (unsigned long x);
unsigned long      SEGGER_float_to_ulong     (float x);
double             SEGGER_ulong_to_double    (unsigned long x);
unsigned long      SEGGER_double_to_ulong    (double x);

float              SEGGER_llong_to_float     (long long x);
long long          SEGGER_float_to_llong     (float x);
double             SEGGER_llong_to_double    (long long x);
long long          SEGGER_double_to_llong    (double x);
float              SEGGER_ullong_to_float    (unsigned long long x);
unsigned long long SEGGER_float_to_ullong    (float x);
double             SEGGER_ullong_to_double   (unsigned long long x);
unsigned long long SEGGER_double_to_ullong   (double x);

double             SEGGER_float_to_double    (float x);
float              SEGGER_double_to_float    (double x);
double             SEGGER_ldouble_to_double  (long double x);
float              SEGGER_ldouble_to_float   (long double x);

float              SEGGER_ldexpf             (float x, int n);
double             SEGGER_ldexp              (double x, int n);
float              SEGGER_frexpf             (float x, int *exp);
double             SEGGER_frexp              (double x, int *exp);

float              SEGGER_ceilf              (float x);
double             SEGGER_ceil               (double x);
float              SEGGER_floorf             (float x);
double             SEGGER_floor              (double x);
float              SEGGER_fabsf              (float x);
double             SEGGER_fabs               (double x);
float              SEGGER_fminf              (float x, float y);
double             SEGGER_fmin               (double x, double y);
float              SEGGER_fmaxf              (float x, float y);
double             SEGGER_fmax               (double x, double y);
double             SEGGER_sqrt               (double x);
float              SEGGER_sqrtf              (float x);
double             SEGGER_cbrt               (double x);
float              SEGGER_cbrtf              (float x);
double             SEGGER_rsqrt              (double x);
float              SEGGER_rsqrtf             (float x);
float              SEGGER_hypotf             (float  x, float  y);
double             SEGGER_hypot              (double x, double y);
double             SEGGER_sin                (double x);
float              SEGGER_sinf               (float x);
double             SEGGER_cos                (double x);
float              SEGGER_cosf               (float x);
double             SEGGER_tan                (double x);
float              SEGGER_tanf               (float x);
double             SEGGER_asin               (double x);
float              SEGGER_asinf              (float x);
double             SEGGER_acos               (double x);
float              SEGGER_acosf              (float x);
double             SEGGER_atan               (double x);
float              SEGGER_atanf              (float x);
float              SEGGER_atan2f             (float y, float x);
double             SEGGER_atan2              (double y, double x);
double             SEGGER_log                (double x);
float              SEGGER_logf               (float x);
double             SEGGER_log1p              (double x);
float              SEGGER_log1pf             (float x);
double             SEGGER_log2               (double x);
float              SEGGER_log2f              (float x);
double             SEGGER_log10              (double x);
float              SEGGER_log10f             (float x);
int                SEGGER_ilogb              (double x);
int                SEGGER_ilogbf             (float x);
float              SEGGER_fmodf              (float x, float y);
double             SEGGER_fmod               (double x, double y);
float              SEGGER_modff              (float x, float  *iptr);
double             SEGGER_modf               (double x, double *iptr);
double             SEGGER_exp                (double x);
float              SEGGER_expf               (float x);
float              SEGGER_exp2f              (float  x);
double             SEGGER_exp2               (double x);
float              SEGGER_exp10f             (float  x);
double             SEGGER_exp10              (double x);
float              SEGGER_powf               (float x, float y);
double             SEGGER_pow                (double x, double y);
double             SEGGER_sinh               (double x);
float              SEGGER_sinhf              (float x);
double             SEGGER_cosh               (double x);
float              SEGGER_coshf              (float x);
double             SEGGER_tanh               (double x);
float              SEGGER_tanhf              (float x);
double             SEGGER_asinh              (double x);
float              SEGGER_asinhf             (float x);
double             SEGGER_acosh              (double x);
float              SEGGER_acoshf             (float x);
double             SEGGER_atanh              (double x);
float              SEGGER_atanhf             (float x);

float              SEGGER_nextafterf         (float __x, float __y);
float              SEGGER_nexttowardf        (float __x, long double __y);

#endif

//
// GNU ABI
//

#if __SEGGER_RTL_INCLUDE_GNU_API

float              __addsf3                  (float __x, float __y);
double             __adddf3                  (double __x, double __y);
float              __subsf3                  (float __x, float __y);
double             __subdf3                  (double __x, double __y);
float              __mulsf3                  (float __x, float __y);
double             __muldf3                  (double __x, double __y);
float              __divsf3                  (float __x, float __y);
double             __divdf3                  (double __x, double __y);
int                __ltsf2                   (float __x, float __y);
int                __ltdf2                   (double __x, double __y);
int                __lesf2                   (float __x, float __y);
int                __ledf2                   (double __x, double __y);
int                __gtsf2                   (float __x, float __y);
int                __gtdf2                   (double __x, double __y);
int                __gesf2                   (float __x, float __y);
int                __gedf2                   (double __x, double __y);
int                __eqsf2                   (float __x, float __y);
int                __eqdf2                   (double __x, double __y);
int                __nesf2                   (float __x, float __y);
int                __nedf2                   (double __x, double __y);
int                __unordsf2                (float __x, float __y);
int                __unorddf2                (double __x, double __y);
int                __fixsfsi                 (float __x);
int                __fixdfsi                 (double __x);
long long          __fixsfdi                 (float f);
long long          __fixdfdi                 (double __x);
unsigned long long __fixunsdfdi              (double __x);
unsigned           __fixunssfsi              (float __x);
unsigned           __fixunsdfsi              (double __x);
float              __floatsisf               (__SEGGER_RTL_I32 __x);
double             __floatsidf               (__SEGGER_RTL_I32 __x);
float              __floatunsisf             (__SEGGER_RTL_U32 __x);
unsigned long long __fixunssfdi              (float f);
double             __floatunsidf             (__SEGGER_RTL_U32 __x);
float              __floatdisf               (__SEGGER_RTL_I64 __x);
double             __floatdidf               (__SEGGER_RTL_I64 __x);
float              __floatundisf             (__SEGGER_RTL_U64 __x);
double             __floatundidf             (__SEGGER_RTL_U64 __x);
float              __truncdfsf2              (double __x);
double             __extendsfdf2             (float __x);
float              __gnu_h2f_ieee            (__SEGGER_RTL_U16 __x);
double             __gnu_h2d_ieee            (__SEGGER_RTL_U16 __x);
unsigned short     __gnu_f2h_ieee            (float __x);
unsigned short     __gnu_d2h_ieee            (double __x);

#endif

//
// ARM EAEBI
//

#if __SEGGER_RTL_INCLUDE_AEABI_API

//
// Incoming and outgoing floats and doubles are always confined to
// integer registers, even when the floating ABI uses hard-floating
// registers.  As such, all functions are prototyped to use integer
// types to ensure that arguments are passed in the correct registers.
//

__SEGGER_RTL_U32   __aeabi_fadd              (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_U32   __aeabi_fsub              (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_U32   __aeabi_frsub             (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_U32   __aeabi_fmul              (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_U32   __aeabi_fdiv              (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_I32   __aeabi_fcmplt            (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_I32   __aeabi_fcmple            (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_I32   __aeabi_fcmpgt            (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_I32   __aeabi_fcmpge            (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_I32   __aeabi_fcmpeq            (__SEGGER_RTL_U32, __SEGGER_RTL_U32);
__SEGGER_RTL_U64   __aeabi_dadd              (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_U64   __aeabi_dsub              (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_U64   __aeabi_drsub             (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_U64   __aeabi_dmul              (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_U64   __aeabi_ddiv              (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_I32   __aeabi_dcmplt            (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_I32   __aeabi_dcmple            (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_I32   __aeabi_dcmpgt            (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_I32   __aeabi_dcmpge            (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_I32   __aeabi_dcmpeq            (__SEGGER_RTL_U64, __SEGGER_RTL_U64);
__SEGGER_RTL_I32   __aeabi_f2iz              (__SEGGER_RTL_U32);
__SEGGER_RTL_U32   __aeabi_f2uiz             (__SEGGER_RTL_U32);
__SEGGER_RTL_I64   __aeabi_f2lz              (__SEGGER_RTL_U32);
__SEGGER_RTL_U64   __aeabi_f2ulz             (__SEGGER_RTL_U32);
__SEGGER_RTL_U32   __aeabi_i2f               (__SEGGER_RTL_I32);
__SEGGER_RTL_U32   __aeabi_ui2f              (__SEGGER_RTL_U32);
__SEGGER_RTL_U32   __aeabi_l2f               (__SEGGER_RTL_I64);
__SEGGER_RTL_U32   __aeabi_ul2f              (__SEGGER_RTL_U64);
__SEGGER_RTL_I32   __aeabi_d2iz              (__SEGGER_RTL_U64);
__SEGGER_RTL_I64   __aeabi_d2lz              (__SEGGER_RTL_U64);
__SEGGER_RTL_U32   __aeabi_d2uiz             (__SEGGER_RTL_U64);
__SEGGER_RTL_U64   __aeabi_d2ulz             (__SEGGER_RTL_U64);
__SEGGER_RTL_U64   __aeabi_i2d               (__SEGGER_RTL_I32);
__SEGGER_RTL_U64   __aeabi_ui2d              (__SEGGER_RTL_U32);
__SEGGER_RTL_U64   __aeabi_l2d               (__SEGGER_RTL_I64);
__SEGGER_RTL_U64   __aeabi_ul2d              (__SEGGER_RTL_U64);
__SEGGER_RTL_U64   __aeabi_f2d               (__SEGGER_RTL_U32);
__SEGGER_RTL_U32   __aeabi_d2f               (__SEGGER_RTL_U64);
__SEGGER_RTL_U16   __aeabi_f2h               (__SEGGER_RTL_U32);
__SEGGER_RTL_U16   __aeabi_d2h               (__SEGGER_RTL_U64);
__SEGGER_RTL_U32   __aeabi_h2f               (__SEGGER_RTL_U16);
__SEGGER_RTL_U64   __aeabi_h2d               (__SEGGER_RTL_U16);

#endif

/*********************************************************************
*
*       Supporting private prototypes
*
**********************************************************************
*/

int                __SEGGER_RTL_float32_isinf    (float);
int                __SEGGER_RTL_float64_isinf    (double);
int                __SEGGER_RTL_float32_isnan    (float);
int                __SEGGER_RTL_float64_isnan    (double);
int                __SEGGER_RTL_float32_isfinite (float);
int                __SEGGER_RTL_float64_isfinite (double);
int                __SEGGER_RTL_float32_isnormal (float);
int                __SEGGER_RTL_float64_isnormal (double);
int                __SEGGER_RTL_float32_signbit  (float);
int                __SEGGER_RTL_float64_signbit  (double);
int                __SEGGER_RTL_float32_classify (float);
int                __SEGGER_RTL_float64_classify (double);

#ifdef __cplusplus
}
#endif

typedef struct {
  float Re;
  float Im;
} __SEGGER_RTL_FLOAT32_PSEUDO_COMPLEX;

typedef struct {
  union {
    __SEGGER_RTL_FLOAT32_PSEUDO_COMPLEX part;   // Real + imaginary parts as a structure
#if defined(__SEGGER_RTL_FLOAT32_C_COMPLEX)
    __SEGGER_RTL_FLOAT32_C_COMPLEX      value;  // C API presentation of a complex value
#endif
  } u;
} __SEGGER_RTL_FLOAT32_COMPLEX;

typedef struct {
  double Re;
  double Im;
} __SEGGER_RTL_FLOAT64_PSEUDO_COMPLEX;

typedef struct {
  union {
    __SEGGER_RTL_FLOAT64_PSEUDO_COMPLEX part;   // Real + imaginary parts as a structure
#if defined(__SEGGER_RTL_FLOAT64_C_COMPLEX)
    __SEGGER_RTL_FLOAT64_C_COMPLEX      value;  // C API presentation of a complex value
#endif
  } u;
} __SEGGER_RTL_FLOAT64_COMPLEX;

typedef struct {
  long double Re;
  long double Im;
} __SEGGER_RTL_LDOUBLE_PSEUDO_COMPLEX;

typedef struct {
  union {
    __SEGGER_RTL_LDOUBLE_PSEUDO_COMPLEX part;   // Real + imaginary parts as a structure
#if defined(__SEGGER_RTL_LDOUBLE_C_COMPLEX)
    __SEGGER_RTL_LDOUBLE_C_COMPLEX      value;  // C API presentation of a complex value
#endif
  } u;
} __SEGGER_RTL_LDOUBLE_COMPLEX;

#endif

/*************************** End of file ****************************/
