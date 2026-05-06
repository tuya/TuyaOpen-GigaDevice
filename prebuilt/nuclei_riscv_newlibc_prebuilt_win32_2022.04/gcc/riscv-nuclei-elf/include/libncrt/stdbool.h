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

#ifndef __SEGGER_RTL_STDBOOL_H
#define __SEGGER_RTL_STDBOOL_H

#ifndef __cplusplus

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       bool
*
*  Description
*    Macros expanding to support the Boolean type.
*/
#define bool _Bool    // Underlying boolean type
#define true  1       // Boolean true value
#define false 0       // Boolean false value

/*********************************************************************
*
*       __bool_true_false_are_defined 
*
*  Description
*    This is required to be here for the C standard.
*/
#define __bool_true_false_are_defined 1

#endif

#endif

/*************************** End of file ****************************/
