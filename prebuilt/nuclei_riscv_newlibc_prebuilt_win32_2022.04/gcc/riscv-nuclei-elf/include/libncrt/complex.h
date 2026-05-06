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

#ifndef __SEGGER_RTL_COMPLEX_H
#define __SEGGER_RTL_COMPLEX_H

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "__SEGGER_RTL_FP.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define complex                  _Complex

/*********************************************************************
*
*       Prototypes (internal API)
*
**********************************************************************
*/

__SEGGER_RTL_FLOAT64_C_COMPLEX   __SEGGER_RTL_float64_cmplx(double x, double y);

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

float                            crealf  (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
float                            cimagf  (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
float                            cabsf   (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   conjf   (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   csqrtf  (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   cexpf   (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   clogf   (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   csinf   (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   ccosf   (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   ctanf   (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   casinf  (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   cacosf  (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   catanf  (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   csinhf  (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   ccoshf  (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   ctanhf  (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   casinhf (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   cacoshf (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);
__SEGGER_RTL_FLOAT32_C_COMPLEX   catanhf (__SEGGER_RTL_FLOAT32_C_COMPLEX __x);


double                           creal   (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
double                           cimag   (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
double                           cabs    (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   conj    (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   csqrt   (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   cexp    (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   clog    (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   csin    (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   ccos    (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   ctan    (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   casin   (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   cacos   (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   catan   (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   csinh   (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   ccosh   (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   ctanh   (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   casinh  (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   cacosh  (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);
__SEGGER_RTL_FLOAT64_C_COMPLEX   catanh  (__SEGGER_RTL_FLOAT64_C_COMPLEX __x);

long double                      creall  (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
long double                      cimagl  (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
long double                      cabsl   (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   conjl   (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   csqrtl  (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   cexpl   (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   clogl   (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   csinl   (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   ccosl   (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   ctanl   (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   casinl  (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   cacosl  (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   catanl  (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   csinhl  (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   ccoshl  (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   ctanhl  (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   casinhl (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   cacoshl (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);
__SEGGER_RTL_LDOUBLE_C_COMPLEX   catanhl (__SEGGER_RTL_LDOUBLE_C_COMPLEX __x);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
