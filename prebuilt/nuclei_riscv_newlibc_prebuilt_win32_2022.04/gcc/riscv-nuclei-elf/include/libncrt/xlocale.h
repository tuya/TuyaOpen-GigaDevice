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

Description: IEEE Std 1003.1-2008 (POSIX.1) extended locales.

*/


#ifndef __SEGGER_RTL_XLOCALE_H
#define __SEGGER_RTL_XLOCALE_H

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

#define LC_COLLATE_MASK   (1 << LC_COLLATE)
#define LC_CTYPE_MASK     (1 << LC_CTYPE)
#define LC_MONETARY_MASK  (1 << LC_MONETARY)
#define LC_NUMERIC_MASK   (1 << LC_NUMERIC)
#define LC_TIME_MASK      (1 << LC_TIME)
#define LC_MESSAGES_MASK  0                 // Not yet supported (1 << LC_MESSAGES)

#define LC_ALL_MASK        ((int)(~0u))

#define LC_GLOBAL_LOCALE  &__SEGGER_RTL_global_locale

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

#ifndef __SEGGER_RTL_LOCALE_T_DEFINED
#define __SEGGER_RTL_LOCALE_T_DEFINED
typedef struct __SEGGER_RTL_POSIX_locale_s *locale_t;
#endif

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

locale_t       newlocale    (int category_mask, const char *locale, locale_t base);
locale_t       duplocale    (locale_t loc);
int            freelocale   (locale_t loc);
struct lconv * localeconv_l (locale_t loc);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
