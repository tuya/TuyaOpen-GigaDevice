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

#ifndef __SEGGER_RTL_INT_H
#define __SEGGER_RTL_INT_H

/*********************************************************************
*
*       Pre-configuration section
*
**********************************************************************
*/

//
// Inform headers we are compiling the library proper
//
#define __SEGGER_RTL_COMPILING_LIBRARY  1

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "__SEGGER_RTL.h"
#include "stdint.h"
#include "limits.h"
#include "wchar.h"
#include "locale.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Inlining of functions is based on configuration of
// library optimization level.
//
#if __SEGGER_RTL_OPTIMIZE >= 2
  #define  __SEGGER_RTL_INLINE   __SEGGER_RTL_ALWAYS_INLINE
#elif __SEGGER_RTL_OPTIMIZE >= 1
  #define  __SEGGER_RTL_INLINE   __SEGGER_RTL_REQUEST_INLINE
#else
  #define  __SEGGER_RTL_INLINE   /*compiler makes its own decision on whether inlining is profitable*/
#endif

// Printer formatting flags.
#define FORMAT_LONG                 (1<<0)
#define FORMAT_LONG_LONG            (1<<1)
#define FORMAT_SHORT                (1<<2)
#define FORMAT_CHAR                 (1<<3)
#define FORMAT_LEFT_JUSTIFY         (1<<4)
#define FORMAT_SIGNED               (1<<5)
#define FORMAT_SPACE                (1<<6)
#define FORMAT_ALTERNATIVE          (1<<7)
#define FORMAT_HAVE_PRECISION       (1<<8)
#define FORMAT_ZERO_FILL            (1<<9)
#define FORMAT_FLOAT_E              (1<<10)
#define FORMAT_FLOAT_F              (1<<11)
#define FORMAT_ENGINEERING          (1<<12)
#define FORMAT_CAPITALS             (1<<13)
#define FORMAT_INPUT_SIGNED         (1<<14)
#define FORMAT_TICK                 (1<<15)   // POSIX.1 extension

// Combinations of flags used internally...
#define FORMAT_NEGATIVE             (FORMAT_SIGNED | FORMAT_SPACE)
#define FORMAT_FLOAT_G              (FORMAT_FLOAT_E | FORMAT_FLOAT_F)

#define M_PI        3.14159265358979323846   // Pi
#define M_PI_2      1.57079632679489661923   // Pi/2
#define M_PI_3      1.04719755119659774615   // Pi/3
#define M_PI_4      0.78539816339744830962   // Pi/4
#define M_PI_6      0.52359877559829887038   // Pi/6
#define M_SQRT_1_2  0.70710678118654752440   // Sqrt[0.5]

typedef struct { unsigned short min, max, map; } __SEGGER_RTL_unicode_map_bmp_range_t;
typedef struct { unsigned short cp, map; }       __SEGGER_RTL_unicode_map_bmp_singleton_t;
typedef struct { unsigned short min, max; }      __SEGGER_RTL_unicode_set_bmp_range_t;
typedef struct { long min, max; }                __SEGGER_RTL_unicode_set_nonbmp_range_t;

typedef void *ARGTYPE_PtrArg;
typedef char *ARGTYPE_CharPtrArg;

// We can't realistically read more characters than our total address
// space can hold, so use ptrdiff_t as the type needed to hold character
// counts.

#define charcount_t ptrdiff_t

#define ARGTYPE_PtrArg_NULL NULL

#define ARGTYPE __SEGGER_RTL_VA_LIST
#define ARGTYPE_setLongPtrArg(a, n) long *xp = va_arg(args, long *); *xp = n; 
#define ARGTYPE_setLongLongPtrArg(a, n) long long *xp = va_arg(args, long long *); *xp = n; 
#define ARGTYPE_setIntPtrArg(a, n) int *xp = va_arg(args, int *); *xp = n; 
#define ARGTYPE_setCharPtrArg(a, n) *(char *)va_arg(args, char *) = n; 
#define ARGTYPE_writePtrArgChar(a, ptr, v) *(char *)ptr = v
#define ARGTYPE_writePtrArgShort(a, ptr, v) *(short *)ptr = v
#define ARGTYPE_writePtrArgInt(a, ptr, v) *(int *)ptr = v
#define ARGTYPE_writePtrArgLong(a, ptr, v) *(long *)ptr = v
#define ARGTYPE_writePtrArgLongLong(a, ptr, v) *(int64_t *)ptr = v
#define ARGTYPE_writePtrArgUnsignedChar(a, ptr, v) *(unsigned char *)ptr = v
#define ARGTYPE_writePtrArgUnsignedShort(a, ptr, v) *(unsigned short *)ptr = v
#define ARGTYPE_writePtrArgUnsignedInt(a, ptr, v) *(unsigned int *)ptr = v
#define ARGTYPE_writePtrArgUnsignedLong(a, ptr, v) *(unsigned long *)ptr = v
#define ARGTYPE_writePtrArgUnsignedLongLong(a, ptr, v) *(uint64_t *)ptr = v
#define ARGTYPE_writePtrArgFloat(a, ptr, v) *(float *)ptr = v
#define ARGTYPE_writePtrArgDouble(a, ptr, v) *(double *)ptr = v
#define ARGTYPE_writePtrArgLongDouble(a, ptr, v) *(long double *)ptr = v

enum {
  __SEGGER_RTL_WC_ALNUM = 1,
  __SEGGER_RTL_WC_ALPHA,
  __SEGGER_RTL_WC_CNTRL,
  __SEGGER_RTL_WC_DIGIT,
  __SEGGER_RTL_WC_GRAPH,
  __SEGGER_RTL_WC_LOWER,
  __SEGGER_RTL_WC_UPPER,
  __SEGGER_RTL_WC_SPACE,
  __SEGGER_RTL_WC_PRINT,
  __SEGGER_RTL_WC_PUNCT,
  __SEGGER_RTL_WC_BLANK,
  __SEGGER_RTL_WC_XDIGIT
};

enum {
  __SEGGER_RTL_WT_TOLOWER = 1,
  __SEGGER_RTL_WT_TOUPPER
};

/* Base classifications. */
#define __SEGGER_RTL_CTYPE_UPPER          0x01    /* upper case letter */
#define __SEGGER_RTL_CTYPE_LOWER          0x02    /* lower case letter */
#define __SEGGER_RTL_CTYPE_DIGIT          0x04    /* digit */
#define __SEGGER_RTL_CTYPE_SPACE          0x08    /* whitespace */
#define __SEGGER_RTL_CTYPE_PUNCT          0x10    /* punctuation character */
#define __SEGGER_RTL_CTYPE_CNTRL          0x20    /* control character */
#define __SEGGER_RTL_CTYPE_BLANK          0x40    /* space char */
#define __SEGGER_RTL_CTYPE_XDIGIT         0x80    /* hexadecimal digit */

/* Derived classifications. */
#define __SEGGER_RTL_CTYPE_ALPHA   (__SEGGER_RTL_CTYPE_UPPER | __SEGGER_RTL_CTYPE_LOWER)
#define __SEGGER_RTL_CTYPE_ALNUM   (__SEGGER_RTL_CTYPE_UPPER | __SEGGER_RTL_CTYPE_LOWER | __SEGGER_RTL_CTYPE_DIGIT)
#define __SEGGER_RTL_CTYPE_GRAPH   (__SEGGER_RTL_CTYPE_PUNCT | __SEGGER_RTL_CTYPE_UPPER | __SEGGER_RTL_CTYPE_LOWER | __SEGGER_RTL_CTYPE_DIGIT)
#define __SEGGER_RTL_CTYPE_PRINT   (__SEGGER_RTL_CTYPE_BLANK | __SEGGER_RTL_CTYPE_PUNCT | __SEGGER_RTL_CTYPE_UPPER | __SEGGER_RTL_CTYPE_LOWER | __SEGGER_RTL_CTYPE_DIGIT)

struct __SEGGER_RTL_locale_codeset_s {
  // Narrow character functions
  int (*__isctype)(int, int);
  int (*__toupper)(int);
  int (*__tolower)(int);

  // Wide character functions
  int                 (*__iswctype)(__SEGGER_RTL_WINT_T, int);
  __SEGGER_RTL_WINT_T (*__towupper)(__SEGGER_RTL_WINT_T);
  __SEGGER_RTL_WINT_T (*__towlower)(__SEGGER_RTL_WINT_T);

  // Conversion between multi-byte and wide characters.
  int (*__wctomb)(char *s, __SEGGER_RTL_WCHAR_T wc, struct __mbstate_s *state);
  int (*__mbtowc)(__SEGGER_RTL_WCHAR_T *pwc, const char *s, __SEGGER_RTL_SIZE_T len, struct __mbstate_s *state);
};

typedef int (__SEGGER_RTL_mb_encode_t)(char *s, __SEGGER_RTL_WCHAR_T wc, struct __mbstate_s *codec);
typedef int (__SEGGER_RTL_mb_decode_t)(__SEGGER_RTL_WCHAR_T *pwc, const char *s, __SEGGER_RTL_SIZE_T len, struct __mbstate_s *codec);

typedef struct {
  __SEGGER_RTL_WCHAR_T * wide;
  char                 * narrow;
} __SEGGER_RTL_PRIN_STR;

typedef struct {
  char     * pdata;
  unsigned   index;
  unsigned   capacity;
} __SEGGER_RTL_PRIN_BUF;

typedef struct __SEGGER_RTL_prin_tag {
  __SEGGER_RTL_SIZE_T   charcount;
  __SEGGER_RTL_SIZE_T   maxchars;
  __SEGGER_RTL_PRIN_STR string;
  __SEGGER_RTL_PRIN_BUF buffer;
  int (* output_fn)(struct __SEGGER_RTL_prin_tag *ctx, const char *pData, unsigned DataLen);
} __SEGGER_RTL_prin_t;

typedef struct __SEGGER_RTL_scan_tag {
  const char  * narrow;
  int        (* getc_fn)  (struct __SEGGER_RTL_scan_tag *ctx);
  int        (* ungetc_fn)(struct __SEGGER_RTL_scan_tag *ctx, int ch);
} __SEGGER_RTL_scan_t;

typedef struct __SEGGER_RTL_wscan_tag {
  const __SEGGER_RTL_WCHAR_T * wide;
  int                       (* getc_fn)  (struct __SEGGER_RTL_wscan_tag *ctx);
  int                       (* ungetc_fn)(struct __SEGGER_RTL_wscan_tag *ctx, int ch);
} __SEGGER_RTL_wscan_t;

typedef struct __SEGGER_RTL_heap_tag {
  struct __SEGGER_RTL_heap_tag *next;
  unsigned size;
} __SEGGER_RTL_heap_t;

typedef struct {
  char      * buf;
  size_t      smax;
  mbstate_t   mbstate;
} __SEGGER_RTL_time_state_t;

typedef void (*__SEGGER_RTL_exit_func)(void);
typedef void (*__SEGGER_RTL_dtor_func)(void *);

#include "__SEGGER_RTL_FP_Int.h"

/*********************************************************************
*
*       Global data
*
**********************************************************************
*/

extern __SEGGER_RTL_RODATA char            __SEGGER_RTL_c_locale_abbrev_day_names   [];
extern __SEGGER_RTL_RODATA char            __SEGGER_RTL_c_locale_abbrev_month_names [];
extern __SEGGER_RTL_RODATA char            __SEGGER_RTL_hex_uc                      [];
extern __SEGGER_RTL_RODATA char            __SEGGER_RTL_hex_lc                      [];
extern __SEGGER_RTL_RODATA uint64_t        __SEGGER_RTL_ipow10                      [];
extern __SEGGER_RTL_RODATA __SEGGER_RTL_U8 __SEGGER_RTL_clz_lut                     [];

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

//
// Internal functions.
//

double                        __SEGGER_RTL_pow10                               (int);
float                         __SEGGER_RTL_pow10f                              (int);
int                           __SEGGER_RTL_digit                               (int ch, int radix);
void                          __SEGGER_RTL_init_prin                           (__SEGGER_RTL_prin_t *ctx);
void                          __SEGGER_RTL_putc                                (__SEGGER_RTL_prin_t *ctx, int ch);
int                           __SEGGER_RTL_prin                                (__SEGGER_RTL_prin_t *ctx, const char *fmt, ARGTYPE args);
void                          __SEGGER_RTL_prin_flush                          (__SEGGER_RTL_prin_t *ctx);
int                           __SEGGER_RTL_scan                                (__SEGGER_RTL_scan_t *ctx, const unsigned char *fmt, ARGTYPE argv);
void                          __SEGGER_RTL_print_padding                       (__SEGGER_RTL_prin_t *ctx, int ch, int n);
void                          __SEGGER_RTL_pre_padding                         (__SEGGER_RTL_prin_t *ctx, int flags, int width);
void                          __SEGGER_RTL_print_wide_string                   (__SEGGER_RTL_prin_t *ctx, const wchar_t *wstr, unsigned max_bytes);
__SEGGER_RTL_WINT_T           __SEGGER_RTL_unicode_towupper                    (__SEGGER_RTL_WINT_T ch);
__SEGGER_RTL_WINT_T           __SEGGER_RTL_unicode_towlower                    (__SEGGER_RTL_WINT_T ch);
int                           __SEGGER_RTL_unicode_iswctype                    (wint_t ch, int ty);
int                           __SEGGER_RTL_unicode_map_range_search            (const void *k0, const void *k1);
int                           __SEGGER_RTL_unicode_map_singleton_search        (const void *k0, const void *k1);
int                           __SEGGER_RTL_unicode_set_bmp_range_search        (const void *k0, const void *k1);
int                           __SEGGER_RTL_unicode_set_nonbmp_range_search     (const void *k0, const void *k1);
int                           __SEGGER_RTL_unicode_set_bmp_singleton_search    (const void *k0, const void *k1);
int                           __SEGGER_RTL_unicode_set_nonbmp_singleton_search (const void *k0, const void *k1);
const char *                  __SEGGER_RTL_string_list_decode                  (const char *str, int index);
int                           __SEGGER_RTL_string_list_encode                  (const char *list, const char *str);
void                          __SEGGER_RTL_init_mbstate                        (struct __mbstate_s *state);
int                           __SEGGER_RTL_ascii_wctomb                        (char *s, __SEGGER_RTL_WCHAR_T wc, struct __mbstate_s *state);
int                           __SEGGER_RTL_ascii_mbtowc                        (__SEGGER_RTL_WCHAR_T *pwc, const char *s, __SEGGER_RTL_SIZE_T len, struct __mbstate_s *state);
int                           __SEGGER_RTL_utf8_wctomb                         (char *s, __SEGGER_RTL_WCHAR_T wc, struct __mbstate_s *state);
int                           __SEGGER_RTL_utf8_mbtowc                         (__SEGGER_RTL_WCHAR_T *pwc, const char *s, __SEGGER_RTL_SIZE_T len, struct __mbstate_s *state);
int                           __SEGGER_RTL_mb_max                              (const struct __SEGGER_RTL_POSIX_locale_s *loc);
int                           __SEGGER_RTL_compare_locale_name                 (const char *str0, const char *str1);
void                          __SEGGER_RTL_compute_wide_metrics                (const wchar_t *wstr, unsigned max_bytes, int *n_bytes);
const __SEGGER_RTL_locale_t * __SEGGER_RTL_find_locale                         (const char *locale);
const __SEGGER_RTL_locale_t * __SEGGER_RTL_global_locale_category              (int __category);
const __SEGGER_RTL_locale_t * __SEGGER_RTL_locale_category                     (locale_t __locale, int __category);

//
// AEABI functions.
//

#if __SEGGER_RTL_INCLUDE_AEABI_API
unsigned long long            __aeabi_llsl                                     (unsigned long long x, int n);
unsigned long long            __aeabi_llsr                                     (unsigned long long x, int n);
long long                     __aeabi_lasr                                     (long long x, int n);
long long                     __aeabi_lmul                                     (long long x, long long y);
int                           __aeabi_idiv                                     (int x, int y);
int                           __aeabi_idivmod                                  (int x, int y);                                // Remainder returned as well, but inaccessible from C
unsigned                      __aeabi_uidiv                                    (unsigned x, unsigned y);
unsigned                      __aeabi_uidivmod                                 (unsigned x, unsigned y);                      // Remainder returned as well, but inaccessible from C
long long                     __aeabi_ldivmod                                  (long long x, long long y);                    // Remainder returned as well, but inaccessible from C
unsigned long long            __aeabi_uldivmod                                 (unsigned long long x, unsigned long long y);  // Remainder returned as well, but inaccessible from C
int                           __aeabi_lcmp                                     (long long x, long long y);
int                           __aeabi_ulcmp                                    (unsigned long long x, unsigned long long y);

//
// These are unofficial SEGGER extensions to the Arm Rutime ABI that enable
// direct testing of __aeabi_idivmod(), __aeabi_uidivmod(), __aeabi_ldivmod(),
// and __aeabi_uldivmod().
//
int                           __aeabi_imod                                     (int x, int y);
unsigned                      __aeabi_uimod                                    (unsigned x, unsigned y);
long long                     __aeabi_lmod                                     (long long x, long long y);
unsigned long long            __aeabi_ulmod                                    (unsigned long long x, unsigned long long y);

#endif

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
