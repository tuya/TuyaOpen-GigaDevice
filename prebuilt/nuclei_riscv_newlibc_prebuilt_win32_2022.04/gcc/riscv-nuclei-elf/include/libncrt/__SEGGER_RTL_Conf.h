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

#ifndef __SEGGER_RTL_CONF_H
#define __SEGGER_RTL_CONF_H

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#if defined(__ARM_ARCH_ISA_ARM) || defined(__ARM_ARCH_ISA_THUMB)
  //
  // GNU C doesn't define __ARM_ACLE but does set the feature-test macros,
  // so use the ISA macros, one of which must be defined.
  //
  #include "__SEGGER_RTL_Arm_Conf.h"
#elif defined(__riscv)
  #include "__SEGGER_RTL_RISCV_Conf.h"
#elif defined(__XC16__)
  #include "__SEGGER_RTL_XC16_Conf.h"
#elif defined(_MSC_VER)
  #include "__SEGGER_RTL_MSVC_Conf.h"
#else
  #error Cannot determine configuration from standard definitions!
#endif

#endif

/*************************** End of file ****************************/
