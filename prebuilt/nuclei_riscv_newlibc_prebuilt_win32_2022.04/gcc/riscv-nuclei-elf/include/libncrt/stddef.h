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

#ifndef __SEGGER_RTL_STDDEF_H
#define __SEGGER_RTL_STDDEF_H

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "__SEGGER_RTL.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#ifndef NULL

/*********************************************************************
*
*       NULL
*
*  Description
*    Null-pointer constant.
*/
#define NULL 0            // Null pointer

#endif

/*********************************************************************
*
*       offsetof
*
*  Description
*    Calculate offset of member from start of structure.
*
*  Parameters
*    s - Type name of structure.
*    m - Name of member within structure.
*/
#define offsetof(s,m) ((size_t)&(((s *)0)->m))  // Offset of m within s

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

#ifndef __SEGGER_RTL_SIZE_T_DEFINED
#define __SEGGER_RTL_SIZE_T_DEFINED

/*********************************************************************
*
*       size_t
*
*  Description
*    Unsigned integral type returned by the sizeof operator.
*/
typedef __SEGGER_RTL_SIZE_T size_t;

#endif

#if !defined(__SEGGER_RTL_WCHAR_T_DEFINED)
#define __SEGGER_RTL_WCHAR_T_DEFINED

/*********************************************************************
*
*       wchar_t
*
*  Description
*    Integral type that can hold one wide character.
*/
typedef __SEGGER_RTL_WCHAR_T wchar_t;

#endif

/*********************************************************************
*
*       ptrdiff_t
*
*  Description
*    Signed integral type of the result of subtracting two pointers.
*/
typedef __SEGGER_RTL_PTRDIFF_T ptrdiff_t;

#endif

/*************************** End of file ****************************/
