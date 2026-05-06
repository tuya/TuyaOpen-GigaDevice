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

#ifndef __SEGGER_RTL_STDARG_H
#define __SEGGER_RTL_STDARG_H

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "__SEGGER_RTL.h"

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

typedef __SEGGER_RTL_VA_LIST va_list;

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define va_start(v,l) \
    __builtin_va_start((v), l) 

#define va_arg \
    __builtin_va_arg

#define va_copy(d,s) \
    __builtin_va_copy((d), (s))

#define va_end(ap) \
    __builtin_va_end(ap)

#endif

/*************************** End of file ****************************/
