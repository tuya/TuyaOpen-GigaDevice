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

#ifndef __SEGGER_RTL_FP_INT_H
#define __SEGGER_RTL_FP_INT_H

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

//
// Inlining of functions is based on configuration of
// library optimization level.
//
#if __SEGGER_RTL_OPTIMIZE >= 2
  #define  __SEGGER_RTL_INLINE   __SEGGER_RTL_ALWAYS_INLINE
#elif __SEGGER_RTL_OPTIMIZE >= 1
  #define  __SEGGER_RTL_INLINE   __SEGGER_RTL_REQUEST_INLINE
#else
  #define  __SEGGER_RTL_INLINE   /*compiler makes its own decision on whether inlining is profitable*/
#endif

#if defined(__riscv_flen) && (__riscv_flen >= 64) && (defined(__GNUC__) || defined(__clang__))
static __SEGGER_RTL_ALWAYS_INLINE unsigned __SEGGER_RTL_rv_float64_classify(double x) {
  unsigned cls;
  //
  __asm__("fclass.d %0,%1" : "=r"(cls) : "f"(x));
  //
  return cls;
}
#endif

/*********************************************************************
*
*       Supporting private data
*
**********************************************************************
*/

extern __SEGGER_RTL_RODATA __SEGGER_RTL_U8 __SEGGER_RTL_clz_lut[256];

/*********************************************************************
*
*       Supporting private prototypes
*
**********************************************************************
*/

int    __SEGGER_RTL_float32_isinf    (float);
int    __SEGGER_RTL_float64_isinf    (double);
int    __SEGGER_RTL_float32_isnan    (float);
int    __SEGGER_RTL_float64_isnan    (double);
int    __SEGGER_RTL_float32_isfinite (float);
int    __SEGGER_RTL_float64_isfinite (double);
int    __SEGGER_RTL_float32_isnormal (float);
int    __SEGGER_RTL_float64_isnormal (double);
int    __SEGGER_RTL_float32_signbit  (float);
int    __SEGGER_RTL_float64_signbit  (double);
int    __SEGGER_RTL_float32_classify (float);
int    __SEGGER_RTL_float64_classify (double);

#endif
