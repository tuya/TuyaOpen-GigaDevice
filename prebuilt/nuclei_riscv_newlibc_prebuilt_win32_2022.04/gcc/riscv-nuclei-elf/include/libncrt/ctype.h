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

#ifndef __SEGGER_RTL_CTYPE_H
#define __SEGGER_RTL_CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

int isalpha (int __c);
int isupper (int __c);
int islower (int __c);
int isdigit (int __c);
int isxdigit(int __c);
int isspace (int __c);
int ispunct (int __c);
int isalnum (int __c);
int isprint (int __c);
int isgraph (int __c);
int iscntrl (int __c);
int toupper (int __c);
int tolower (int __c);
int isblank (int __c);  /* C99 */

#ifndef __EXCLUDE_POSIX

#include "__SEGGER_RTL.h"

#ifndef __SEGGER_RTL_LOCALE_T_DEFINED
#define __SEGGER_RTL_LOCALE_T_DEFINED
typedef struct __SEGGER_RTL_POSIX_locale_s *locale_t;
#endif

int isalpha_l (int __c, locale_t __loc);
int isupper_l (int __c, locale_t __loc);
int islower_l (int __c, locale_t __loc);
int isdigit_l (int __c, locale_t __loc);
int isxdigit_l(int __c, locale_t __loc);
int isspace_l (int __c, locale_t __loc);
int ispunct_l (int __c, locale_t __loc);
int isalnum_l (int __c, locale_t __loc);
int isprint_l (int __c, locale_t __loc);
int isgraph_l (int __c, locale_t __loc);
int iscntrl_l (int __c, locale_t __loc);
int toupper_l (int __c, locale_t __loc);
int tolower_l (int __c, locale_t __loc);
int isblank_l (int __c, locale_t __loc);  /* C99 */

#endif

#ifdef __cplusplus
}
#endif

#endif
