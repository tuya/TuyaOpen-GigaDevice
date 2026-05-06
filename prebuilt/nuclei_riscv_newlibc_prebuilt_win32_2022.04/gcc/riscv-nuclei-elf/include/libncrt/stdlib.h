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

#ifndef __SEGGER_RTL_STDLIB_H
#define __SEGGER_RTL_STDLIB_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "__SEGGER_RTL.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defined, fixed
*
**********************************************************************
*/

#ifndef NULL
#define NULL 0
#endif


#ifndef __SEGGER_RTL_SIZE_T_DEFINED
#define __SEGGER_RTL_SIZE_T_DEFINED
typedef __SEGGER_RTL_SIZE_T size_t;
#endif

#if !defined(__cplusplus) || !defined(__clang__)
#ifndef __SEGGER_RTL_WCHAR_T_DEFINED
#define __SEGGER_RTL_WCHAR_T_DEFINED
typedef __SEGGER_RTL_WCHAR_T wchar_t;
#endif
#endif

#ifndef __SEGGER_RTL_LOCALE_T_DEFINED
#define __SEGGER_RTL_LOCALE_T_DEFINED
typedef struct __SEGGER_RTL_POSIX_locale_s *locale_t;
#endif

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1
#define RAND_MAX    32767
#define MB_CUR_MAX  __SEGGER_RTL_mb_max(&__SEGGER_RTL_global_locale)

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
  int quot;
  int rem;
} div_t;

typedef struct {
  long quot;
  long rem;
} ldiv_t;

typedef struct {
  long long quot;
  long rem;
} lldiv_t;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

int                      abs        (int __j);
long int                 labs       (long int __j);
long long int            llabs      (long long int __j);
div_t                    div        (int __numer, int __denom);
ldiv_t                   ldiv       (long int __numer, long int __denom);
lldiv_t                  lldiv      (long long int __numer, long long int __denom);
void                   * malloc     (size_t __size);
void                   * calloc     (size_t __nobj, size_t __size);
void                   * realloc    (void *p, size_t __size);
void                     free       (void *__p);
double                   atof       (const char *__nptr);
double                   strtod     (const char *__nptr, char **__endptr);
float                    strtof     (const char *__nptr, char **__endptr);
long double              strtold    (const char *__nptr, char **__endptr);
int                      atoi       (const char *__nptr);
long int                 atol       (const char *__nptr);
long long int            atoll      (const char *__nptr);
long int                 strtol     (const char *__nptr, char **__endptr, int __base);
long long int            strtoll    (const char *__nptr, char **__endptr, int __base);
unsigned long int        strtoul    (const char *__nptr, char **__endptr, int __base);
unsigned long long int   strtoull   (const char *__nptr, char **__endptr, int __base);
int                      rand       (void);
void                     srand      (unsigned int __seed);
void                   * bsearch    (const void *__key, const void *__buf, size_t __num, size_t __size, int (*__compare)(const void *, const void *));
void                     qsort      (void *__buf, size_t __num, size_t __size, int (*__compare)(const void *, const void *));
void                     abort      (void);
void                     exit       (int __exit_code);
int                      atexit     (void (*__func)(void));
char                   * getenv     (const char *__name);
int                      system     (const char *__command);
char                   * itoa       (int __val, char *__buf, int __radix);
char                   * utoa       (unsigned val, char *buf, int radix);
char                   * ltoa       (long __val, char *__buf, int __radix);
char                   * ultoa      (unsigned long __val, char *__buf, int __radix);
char                   * lltoa      (long long __val, char *__buf, int __radix);
char                   * ulltoa     (unsigned long long __val, char *__buf, int __radix);
int                      mblen      (const char *__s, size_t __n);
int                      mblen_l    (const char *__s, size_t __n, locale_t __loc);
size_t                   mbstowcs   (wchar_t *__pwcs, const char *__s, size_t __n);
size_t                   mbstowcs_l (wchar_t *__pwcs, const char *__s, size_t __n, locale_t __loc);
int                      mbtowc     (wchar_t *__pwc, const char *__s, size_t __n);
int                      mbtowc_l   (wchar_t *__pwc, const char *__s, size_t __n, locale_t __loc);
int                      wctomb     (char *__s, wchar_t __wc);
int                      wctomb_l   (char *__s, wchar_t __wc, locale_t __loc);
size_t                   wcstombs   (char *__s, const wchar_t *__pwcs, size_t __n);
size_t                   wcstombs_l (char *__s, const wchar_t *__pwcs, size_t __n, locale_t __loc);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
