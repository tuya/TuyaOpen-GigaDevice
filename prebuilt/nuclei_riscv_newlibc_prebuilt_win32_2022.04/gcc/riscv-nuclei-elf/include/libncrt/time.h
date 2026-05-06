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

#ifndef __SEGGER_RTL_TIME_H
#define __SEGGER_RTL_TIME_H

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

#ifndef __SEGGER_RTL_SIZE_T_DEFINED
#define __SEGGER_RTL_SIZE_T_DEFINED
typedef __SEGGER_RTL_SIZE_T size_t;
#endif

#ifndef __SEGGER_RTL_LOCALE_T_DEFINED
#define __SEGGER_RTL_LOCALE_T_DEFINED
typedef struct __SEGGER_RTL_POSIX_locale_s *locale_t;
#endif

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000
#endif

#ifndef NULL
#define NULL 0
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef long clock_t;
typedef long time_t;

struct tm {
  int tm_sec;     // seconds after the minute - [0, 59]
  int tm_min;     // minutes after the hour - [0, 59]
  int tm_hour;    // hours since midnight - [0, 23]
  int tm_mday;    // day of the month - [1, 31]
  int tm_mon;     // months since January - [0, 11]
  int tm_year;    // years since 1900
  int tm_wday;    // days since Sunday - [0, 6]
  int tm_yday;    // days since January 1 - [0, 365]
  int tm_isdst;   // daylight savings time flag
};

struct timeval {
  long tv_sec;    // Seconds since the Epoch
  long tv_usec;   // Microseconds
};

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

int         gettimeofday (struct timeval *__tp, void *__tzp);
int         settimeofday (const struct timeval *__tp, const void *__tzp);
clock_t     clock        (void);
time_t      time         (time_t *__tp);
double      difftime     (time_t __time2, time_t __time1);
time_t      mktime       (struct tm *__tp);
char      * asctime      (const struct tm *__tp);
char      * asctime_r    (const struct tm *__tp, char *__buf);
char      * ctime        (const time_t *__tp);
char      * ctime_r      (const time_t *__tp, char *__buf);
struct tm * gmtime       (const time_t *__tp);
struct tm * gmtime_r     (const time_t *__tp, struct tm *__result);
struct tm * localtime    (const time_t *__tp);
struct tm * localtime_r  (const time_t *__tp, struct tm *__result);
size_t      strftime     (char *__s, size_t __smax, const char *__fmt, const struct tm *__tp);
size_t      strftime_l   (char *__s, size_t __smax, const char *__fmt, const struct tm *__tp, locale_t __loc);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
