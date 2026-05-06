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

#ifndef __SEGGER_RTL_FENV_H
#define __SEGGER_RTL_FENV_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define FE_TONEAREST       0
#define FE_TOWARDZERO      1
#define FE_DOWNWARD        2
#define FE_UPWARD          3

#define FE_DFL_ENV         &__SEGGER_RTL_default_fenv

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
  int __control;
} fenv_t;

typedef struct {
  int __control;
} fexcept_t;

/*********************************************************************
*
*       External data
*
**********************************************************************
*/

extern const fenv_t __SEGGER_RTL_default_fenv;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

int feclearexcept   (int __excepts);
int feraiseexcept   (int __excepts);
int fetestexcept    (int __excepts);
int fegetexceptflag (fexcept_t *__flagp, int __excepts);
int fesetexceptflag (const fexcept_t *__flagp, int __excepts);
//
int fegetround      (void);
int fesetround      (int __round);
//
int fegetenv        (fenv_t *__envp);
int fesetenv        (const fenv_t *__envp);
int feholdexcept    (fenv_t *__envp);
int feupdateenv     (const fenv_t *__envp);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
