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

#ifndef __SEGGER_RTL_SETJMP_H
#define __SEGGER_RTL_SETJMP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

#if defined(__riscv_xlen) && __riscv_xlen == 32 && defined(__riscv_abi_rve)
  typedef struct {
    long ireg[4];
  } jmp_buf[1];
#elif defined(__riscv_xlen)
  #if !defined(__riscv_flen)
    typedef struct {
      long  ireg[14];
    } jmp_buf[1];
  #elif __riscv_flen == 32
    typedef struct {
      long  ireg[14];
      float freg[12];
    } jmp_buf[1];
  #elif __riscv_flen == 64
    typedef struct {
      long   ireg[14];
      double freg[12];
    } jmp_buf[1];
  #endif
#elif defined(__ARM_ARCH_VFP__) || defined(__ARM_ARCH_VFP3_D16__) || defined(__ARM_ARCH_VFP3_D32__) || defined(__ARM_ARCH_VFPV4_D16__) || defined(__ARM_ARCH_FPV4_SP_D16__) || defined(__ARM_ARCH_FPV5_SP_D16__) || defined(__ARM_ARCH_FPV5_D16__)
  typedef unsigned long long jmp_buf[14];  // R4-R14, D8-D15
#else
  typedef unsigned long jmp_buf[11];  // R4-R14
#endif

/*********************************************************************
*
*       Functions
*
**********************************************************************
*/

int  setjmp  (jmp_buf __env);

__attribute__((noreturn))
void longjmp (jmp_buf __env, int __val);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
