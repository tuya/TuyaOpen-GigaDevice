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

#ifndef __SEGGER_RTL_WCTYPE_H
#define __SEGGER_RTL_WCTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       #include Section
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

#define WEOF ((wint_t)~0u)

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

#ifndef __SEGGER_RTL_WINT_T_DEFINED
#define __SEGGER_RTL_WINT_T_DEFINED
typedef __SEGGER_RTL_WINT_T wint_t;
#endif

#ifndef __SEGGER_RTL_LOCALE_T_DEFINED
#define __SEGGER_RTL_LOCALE_T_DEFINED
typedef struct __SEGGER_RTL_POSIX_locale_s *locale_t;
#endif

typedef int wctrans_t;
typedef int wctype_t;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

int       iswalpha    (wint_t __c);
int       iswalpha_l  (wint_t __c, locale_t __loc);
int       iswalnum    (wint_t __c);
int       iswalnum_l  (wint_t __c, locale_t __loc);
int       iswblank    (wint_t __c);
int       iswblank_l  (wint_t __c, locale_t __loc);
int       iswcntrl    (wint_t __c);
int       iswcntrl_l  (wint_t __c, locale_t __loc);
int       iswdigit    (wint_t __c);
int       iswdigit_l  (wint_t __c, locale_t __loc);
int       iswgraph    (wint_t __c);
int       iswgraph_l  (wint_t __c, locale_t __loc);
int       iswlower    (wint_t __c);
int       iswlower_l  (wint_t __c, locale_t __loc);
int       iswprint    (wint_t __c);
int       iswprint_l  (wint_t __c, locale_t __loc);
int       iswpunct    (wint_t __c);
int       iswpunct_l  (wint_t __c, locale_t __loc);
int       iswspace    (wint_t __c);
int       iswspace_l  (wint_t __c, locale_t __loc);
int       iswupper    (wint_t __c);
int       iswupper_l  (wint_t __c, locale_t __loc);
int       iswxdigit   (wint_t __c);
int       iswxdigit_l (wint_t __c, locale_t __loc);
wint_t    towlower    (wint_t __c);
wint_t    towlower_l  (wint_t __c, locale_t __loc);
wint_t    towupper    (wint_t __c);
wint_t    towupper_l  (wint_t __c, locale_t __loc);
wint_t    towctrans   (wint_t __c, wctrans_t __t);
wint_t    towctrans_l (wint_t __c, wctrans_t __t, locale_t __loc);
int       iswctype    (wint_t __c, wctype_t __t);
int       iswctype_l  (wint_t __c, wctype_t __t, locale_t __loc);
wctrans_t wctrans     (const char *property);
wctrans_t wctrans_l   (const char *property, locale_t loc);
wctype_t  wctype      (const char *property);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
