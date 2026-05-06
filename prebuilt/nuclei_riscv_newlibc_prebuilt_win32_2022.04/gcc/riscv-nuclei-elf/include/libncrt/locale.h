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

#ifndef __SEGGER_RTL_LOCALE_H
#define __SEGGER_RTL_LOCALE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#ifndef NULL
#define NULL 0
#endif

#define LC_COLLATE    0
#define LC_CTYPE      1
#define LC_MONETARY   2
#define LC_NUMERIC    3
#define LC_TIME       4
#define LC_ALL        5

/*********************************************************************
*
*       Data types
*
**********************************************************************
*/

/*********************************************************************
*
*       lconv
*
*  Description
*    Formatting information for numeric values.
*
*  Additional information
*    Tthis structure holds formatting information on how numeric values
*    are to be written.  Note that the order of fields in this structure is not consistent
*    between implementations, nor is it consistent between C89 and C99 standards.
*    
*    The members decimal_point, grouping, and thousands_sep are controlled
*    by LC_NUMERIC, the remainder by LC_MONETARY.
*    
*    The members int_n_cs_precedes, int_n_sep_by_space, int_n_sign_posn,
*    int_p_cs_precedes}, int_p_sep_by_space, and int_p_sign_posn are added
*    by the C99 standard.
*    
*    This follows the ordering specified by the ARM EABI for the base of
*    this structure.  This ordering is neither that of C89 nor C99.
*/
typedef struct lconv {
  /* Numeric, non-monetary */
  char *decimal_point;          // Decimal point separator.
  char *thousands_sep;          // Separators used to delimit groups of digits to the left of the decimal point for non-monetary quantities.
  char *grouping;               // Specifies the amount of digits that form each of the groups to be separated by thousands_sep separator for non-monetary quantities.

  /* Monetary */
  char *int_curr_symbol;        // International currency symbol.
  char *currency_symbol;        // Local currency symbol.
  char *mon_decimal_point;      // Decimal-point separator used for monetary quantities.
  char *mon_thousands_sep;      // Separators used to delimit groups of digits to the left of the decimal point for monetary quantities.
  char *mon_grouping;           // Specifies the amount of digits that form each of the groups to be separated by mon_thousands_sep separator for monetary quantities.
  char *positive_sign;          // Sign to be used for nonnegative (positive or zero) monetary quantities.
  char *negative_sign;          // Sign to be used for negative monetary quantities.
  char int_frac_digits;         // Amount of fractional digits to the right of the decimal point for monetary quantities in the international format.
  char frac_digits;             // Amount of fractional digits to the right of the decimal point for monetary quantities in the local format.
  char p_cs_precedes;           // Whether the currency symbol should precede nonnegative (positive or zero) monetary quantities.
  char p_sep_by_space;          // Whether a space should appear between the currency symbol and nonnegative (positive or zero) monetary quantities.
  char n_cs_precedes;           // Whether the currency symbol should precede negative monetary quantities.
  char n_sep_by_space;          // Whether a space should appear between the currency symbol and negative monetary quantities.
  char p_sign_posn;             // Position of the sign for nonnegative (positive or zero) monetary quantities.
  char n_sign_posn;             // Position of the sign for negative monetary quantities.

  /* Extra monetary fields added by C99 */
  char int_p_cs_precedes;       // Whether int_curr_symbol precedes or succeeds the value for a nonnegative internationally formatted monetary quantity.
  char int_n_cs_precedes;       // Whether int_curr_symbol precedes or succeeds the value for a negative internationally formatted monetary quantity.
  char int_p_sep_by_space;      // Value indicating the separation of the int_curr_symbol, the sign string, and the value for a nonnegative internationally formatted monetary quantity.
  char int_n_sep_by_space;      // Value indicating the separation of the int_curr_symbol, the sign string, and the value for a negative internationally formatted monetary quantity.
  char int_p_sign_posn;         // Value indicating the positioning of the positive_sign for a nonnegative internationally formatted monetary quantity.
  char int_n_sign_posn;         // Value indicating the positioning of the positive_sign for a negative internationally formatted monetary quantity.
} __SEGGER_RTL_lconv;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

struct lconv * localeconv (void);
char         * setlocale  (int __category, const char *__loc);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
