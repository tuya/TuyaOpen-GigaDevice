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

#ifndef __SEGGER_RTL_ERRNO_H
#define __SEGGER_RTL_ERRNO_H

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "__SEGGER_RTL.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#ifdef __ARM_EABI__

extern const int __aeabi_EDOM;
extern const int __aeabi_EILSEQ;
extern const int __aeabi_ERANGE;

#define EDOM     (__aeabi_EDOM)
#define EILSEQ   (__aeabi_EILSEQ)
#define ERANGE   (__aeabi_ERANGE)

#define EHEAP     0x04
#define ENOMEM    0x05
#define EINVAL    0x06
#define ESPIPE    0x07

#else

/*********************************************************************
*
*       Error names
*
*  Description
*    Symbolic error names for raised errors.
*/
#define EDOM      0x01   // Domain error
#define EILSEQ    0x02   // Illegal multibyte sequence in conversion
#define ERANGE    0x03   // Range error
#define EHEAP     0x04   // Heap is corrupt
#define ENOMEM    0x05   // Out of memory
#define EINVAL    0x06   // Invalid parameter
#define ESPIPE    0x07   // Invalid seek (POSIX.1)

#endif

/*********************************************************************
*
*       errno
*
*  Description
*    Macro returning the current error.
*
*  Additional information
*    The value in errno is significant only when the return value of the
*    call indicated an error.  A function that succeeds is allowed to
*    change errno.  The value of errno is never set to zero by a library
*    function.
*/
#define errno   (*__SEGGER_RTL_X_errno_addr())

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
