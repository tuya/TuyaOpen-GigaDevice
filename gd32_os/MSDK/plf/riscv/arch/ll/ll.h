/*!
    \file    ll.h
    \brief   Declaration of low level functions.

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

#ifndef LL_H_
#define LL_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

// Interrupt Enable bit in mstatus register
#define INTE_EN (1UL << (3))

/**
 ****************************************************************************************
 * @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 ****************************************************************************************
 */
#define GLOBAL_INT_START()                                                        \
do {                                                                              \
    int mstatus;                                                                  \
    __asm__ volatile ("csrrsi %0, mstatus, %1" : "=r" (mstatus) : "i" (INTE_EN)); \
} while(0)

/**
 ****************************************************************************************
 * @brief Disable interrupts globally in the system.
 * This macro must be used when the system wants to disable all the interrupt
 * it could handle.
 ****************************************************************************************
 */
#define GLOBAL_INT_STOP()                                                         \
do {                                                                              \
    int mstatus;                                                                  \
    __asm__ volatile ("csrrci %0, mstatus, %1" : "=r" (mstatus) : "i" (INTE_EN)); \
} while(0)

#ifdef CFG_RTOS
extern void sys_enter_critical(void);
extern void sys_exit_critical(void);

/**
 ****************************************************************************************
 * @brief Disable interrupts globally in the system.
 * This macro must be used in conjunction with the @ref GLOBAL_INT_RESTORE macro since
 * this last one will close the brace that the current macro opens.  This means that both
 * macros must be located at the same scope level.
 ****************************************************************************************
 */
#define GLOBAL_INT_DISABLE()         sys_enter_critical()

/**
 ****************************************************************************************
 * @brief Restore interrupts from the previous global disable.
 * @sa GLOBAL_INT_DISABLE
 ****************************************************************************************
 */
#define GLOBAL_INT_RESTORE()         sys_exit_critical()
#else
#define GLOBAL_INT_DISABLE()         GLOBAL_INT_START()
#define GLOBAL_INT_RESTORE()         GLOBAL_INT_STOP()
#endif

/**
 ****************************************************************************************
 * @brief Force a memory barrier to be inserted
 *
 ****************************************************************************************
 */
#define BARRIER()  __asm volatile("" : : : "memory");

/**
 ****************************************************************************************
 * @brief Wait For Interrupt
 ****************************************************************************************
 */
#define WFI()                                                                            \
do {                                                                                     \
    /* DBG_CPU_SLEEP_START(); */                                                         \
    __asm__ volatile ("wfi":::);                                                         \
    /* DBG_CPU_SLEEP_END(); */                                                           \
} while (0)

#endif // LL_H_
