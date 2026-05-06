/*!
    \file    boot.h
    \brief   Definitions used for boot code.

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#ifndef _BOOT_H_
#define _BOOT_H_

/*
 * INCLUDE
 ****************************************************************************************
 */
// standard integer functions
//#include "co_int.h"
#include <stdint.h>
/*
 * DEFINES
 ****************************************************************************************
 */
// Stack initialization pattern
//#define STACK_INIT_PATTERN     0xF3F3F3F3
extern uint32_t _data[];
extern uint32_t _edata[];
extern uint32_t _rtos[];
extern uint32_t _ertos[];
extern uint32_t _trace[];
extern uint32_t _etrace[];

#define _sshram        ((uint32_t)_data)
#define _eshram        ((uint32_t)_edata)
#define __heap_bottom  ((uint32_t)_rtos)
#define __heap_top     ((uint32_t)_ertos)
#define __trace_start  ((uint32_t)_trace)
#define __trace_end    ((uint32_t)_etrace)

/*
 * LINKER VARIABLES
 ****************************************************************************************
 */

// Low/high boundaries of data sections (from linker script)
// start of uninitialized data section .sbss
//extern uint32_t _ssbss[];
// end of uninitialized data section .sbss
//extern uint32_t _esbss[];
// start of uninitialized data section .bss
//extern uint32_t _sbss[];
// end of uninitialized data section .bss
//extern uint32_t _ebss[];
// start of heap section
//extern uint32_t __heap_bottom[];
// end of heap section
//extern uint32_t __heap_top[];
// start of stack section
//extern uint32_t __stack_bottom[];
// end of stack section
//extern uint32_t __stack_top[];
// start of shared memory section
//extern uint32_t _sshram[];
// end of shared memory section
//extern uint32_t _eshram[];

#endif // _BOOT_H_
