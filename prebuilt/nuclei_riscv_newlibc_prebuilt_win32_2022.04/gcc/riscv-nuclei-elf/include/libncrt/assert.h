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

#ifndef __SEGGER_RTL_ASSERT_H
#define __SEGGER_RTL_ASSERT_H

#include "__SEGGER_RTL.h"

#undef assert

#ifdef NDEBUG
  #define assert(ignore) ((void)0)
#else
  #define assert(e) ((e) ? (void)0 : __SEGGER_RTL_X_assert(#e, __FILE__, __LINE__))
#endif

#if defined __GNUC__ && __STDC_VERSION__ >= 201112L && !defined __cplusplus
#define static_assert _Static_assert
#endif

#endif
