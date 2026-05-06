
/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#include "riscv_encoding.h"

#ifndef __riscv_32e
#define portRegNum              29
#else
#define portRegNum              13
#endif

#define portCONTEXT_SIZE        ( portRegNum * REGBYTES )

#ifdef __riscv_flen
#define portFCONTEXT_SIZE       ( 32 * FPREGBYTES + REGBYTES )
#endif

.section    .text.entry
.align 8

.extern xPortTaskSwitch
.extern pxCurrentTCB
.global prvPortStartFirstTask

/* Start the first task.  This also clears the bit that indicates the FPU is
    in use in case the FPU was used before the scheduler was started - which
    would otherwise result in the unnecessary leaving of space in the stack
    for lazy saving of FPU registers. */
.align 3
prvPortStartFirstTask:
    /* Setup Interrupt Stack using
       The stack that was used by main()
       before the scheduler is started is
       no longer required after the scheduler is started.
       Interrupt stack pointer is stored in CSR_MSCRATCH */
    la t0, _sp
    csrw CSR_MSCRATCH, t0
    LOAD sp, pxCurrentTCB           /* Load pxCurrentTCB. */
    LOAD sp, 0x0(sp)                /* Read sp from first TCB member */

    /* Pop mstatus from stack and set it */
    LOAD t0,  0  * REGBYTES(sp)
    csrw CSR_MSTATUS, t0
    addi sp, sp, 1  * REGBYTES

    /* Pop PC from stack and set MEPC */
    LOAD t0,  0  * REGBYTES(sp)
    csrw CSR_MEPC, t0
    /* Interrupt still disable here */
    /* Restore Registers from Stack */
    LOAD x1,  1  * REGBYTES(sp)    /* RA */
    LOAD x10, 7  * REGBYTES(sp)
    addi sp, sp, portCONTEXT_SIZE

    mret

.align 2
.global eclic_msip_handler
eclic_msip_handler:
    addi sp, sp, -portCONTEXT_SIZE
    STORE x1,  1  * REGBYTES(sp)    /* RA */
    STORE x5,  2  * REGBYTES(sp)
    STORE x6,  3  * REGBYTES(sp)
    STORE x7,  4  * REGBYTES(sp)
    STORE x8,  5  * REGBYTES(sp)
    STORE x9,  6  * REGBYTES(sp)
    STORE x10, 7  * REGBYTES(sp)
    STORE x11, 8  * REGBYTES(sp)
    STORE x12, 9  * REGBYTES(sp)
    STORE x13, 10 * REGBYTES(sp)
    STORE x14, 11 * REGBYTES(sp)
    STORE x15, 12 * REGBYTES(sp)
#ifndef __riscv_32e
    STORE x16, 13 * REGBYTES(sp)
    STORE x17, 14 * REGBYTES(sp)
    STORE x18, 15 * REGBYTES(sp)
    STORE x19, 16 * REGBYTES(sp)
    STORE x20, 17 * REGBYTES(sp)
    STORE x21, 18 * REGBYTES(sp)
    STORE x22, 19 * REGBYTES(sp)
    STORE x23, 20 * REGBYTES(sp)
    STORE x24, 21 * REGBYTES(sp)
    STORE x25, 22 * REGBYTES(sp)
    STORE x26, 23 * REGBYTES(sp)
    STORE x27, 24 * REGBYTES(sp)
    STORE x28, 25 * REGBYTES(sp)
    STORE x29, 26 * REGBYTES(sp)
    STORE x30, 27 * REGBYTES(sp)
    STORE x31, 28 * REGBYTES(sp)
#endif

    /* Push mepc to stack */
    csrr t0, CSR_MEPC
    STORE t0, 0(sp)

    /* When mstatus->SD flag is set, the float register is dirty and need to be saved */
    csrr t0, CSR_MSTATUS
    bgez t0, _no_save_float_register

#ifdef __riscv_flen
    /* Save fp registers */
    addi sp, sp, -portFCONTEXT_SIZE
    csrr  t1, CSR_FCSR
    STORE t1, (portFCONTEXT_SIZE - REGBYTES)(sp)

    FPSTORE f0,  0  * FPREGBYTES(sp)
    FPSTORE f1,  1  * FPREGBYTES(sp)
    FPSTORE f2,  2  * FPREGBYTES(sp)
    FPSTORE f3,  3  * FPREGBYTES(sp)
    FPSTORE f4,  4  * FPREGBYTES(sp)
    FPSTORE f5,  5  * FPREGBYTES(sp)
    FPSTORE f6,  6  * FPREGBYTES(sp)
    FPSTORE f7,  7  * FPREGBYTES(sp)
    FPSTORE f8,  8  * FPREGBYTES(sp)
    FPSTORE f9,  9  * FPREGBYTES(sp)
    FPSTORE f10, 10 * FPREGBYTES(sp)
    FPSTORE f11, 11 * FPREGBYTES(sp)
    FPSTORE f12, 12 * FPREGBYTES(sp)
    FPSTORE f13, 13 * FPREGBYTES(sp)
    FPSTORE f14, 14 * FPREGBYTES(sp)
    FPSTORE f15, 15 * FPREGBYTES(sp)
    FPSTORE f16, 16 * FPREGBYTES(sp)
    FPSTORE f17, 17 * FPREGBYTES(sp)
    FPSTORE f18, 18 * FPREGBYTES(sp)
    FPSTORE f19, 19 * FPREGBYTES(sp)
    FPSTORE f20, 20 * FPREGBYTES(sp)
    FPSTORE f21, 21 * FPREGBYTES(sp)
    FPSTORE f22, 22 * FPREGBYTES(sp)
    FPSTORE f23, 23 * FPREGBYTES(sp)
    FPSTORE f24, 24 * FPREGBYTES(sp)
    FPSTORE f25, 25 * FPREGBYTES(sp)
    FPSTORE f26, 26 * FPREGBYTES(sp)
    FPSTORE f27, 27 * FPREGBYTES(sp)
    FPSTORE f28, 28 * FPREGBYTES(sp)
    FPSTORE f29, 29 * FPREGBYTES(sp)
    FPSTORE f30, 30 * FPREGBYTES(sp)
    FPSTORE f31, 31 * FPREGBYTES(sp)
#endif

_no_save_float_register:
    /* Push mstatus to stack */
    addi sp, sp, -REGBYTES
    STORE t0,  0  * REGBYTES(sp)

    /* Store sp to task stack */
    LOAD t0, pxCurrentTCB
    STORE sp, 0(t0)

    call xPortTaskSwitch

    /* Switch task context */
    LOAD t0, pxCurrentTCB           /* Load pxCurrentTCB. */
    LOAD sp, 0x0(t0)                /* Read sp from first TCB member */

    /* Pop mstatus from stack and set it */
    LOAD t0,  0  * REGBYTES(sp)
    csrw CSR_MSTATUS, t0
    addi sp, sp, REGBYTES
    /* When mstatus->SD flag is set, the float register has been saved and need to be reloaded */
    csrr t0, CSR_MSTATUS
    bgez t0, _no_restore_float_register

#ifdef __riscv_flen
    /* Restore fp registers */
    FPLOAD f0,  0  * FPREGBYTES(sp)
    FPLOAD f1,  1  * FPREGBYTES(sp)
    FPLOAD f2,  2  * FPREGBYTES(sp)
    FPLOAD f3,  3  * FPREGBYTES(sp)
    FPLOAD f4,  4  * FPREGBYTES(sp)
    FPLOAD f5,  5  * FPREGBYTES(sp)
    FPLOAD f6,  6  * FPREGBYTES(sp)
    FPLOAD f7,  7  * FPREGBYTES(sp)
    FPLOAD f8,  8  * FPREGBYTES(sp)
    FPLOAD f9,  9  * FPREGBYTES(sp)
    FPLOAD f10, 10 * FPREGBYTES(sp)
    FPLOAD f11, 11 * FPREGBYTES(sp)
    FPLOAD f12, 12 * FPREGBYTES(sp)
    FPLOAD f13, 13 * FPREGBYTES(sp)
    FPLOAD f14, 14 * FPREGBYTES(sp)
    FPLOAD f15, 15 * FPREGBYTES(sp)
    FPLOAD f16, 16 * FPREGBYTES(sp)
    FPLOAD f17, 17 * FPREGBYTES(sp)
    FPLOAD f18, 18 * FPREGBYTES(sp)
    FPLOAD f19, 19 * FPREGBYTES(sp)
    FPLOAD f20, 20 * FPREGBYTES(sp)
    FPLOAD f21, 21 * FPREGBYTES(sp)
    FPLOAD f22, 22 * FPREGBYTES(sp)
    FPLOAD f23, 23 * FPREGBYTES(sp)
    FPLOAD f24, 24 * FPREGBYTES(sp)
    FPLOAD f25, 25 * FPREGBYTES(sp)
    FPLOAD f26, 26 * FPREGBYTES(sp)
    FPLOAD f27, 27 * FPREGBYTES(sp)
    FPLOAD f28, 28 * FPREGBYTES(sp)
    FPLOAD f29, 29 * FPREGBYTES(sp)
    FPLOAD f30, 30 * FPREGBYTES(sp)
    FPLOAD f31, 31 * FPREGBYTES(sp)

    LOAD t0, (portFCONTEXT_SIZE - REGBYTES)(sp)
    csrw CSR_FCSR, t0
    addi sp, sp, portFCONTEXT_SIZE
#endif

_no_restore_float_register:
    /* Pop PC from stack and set MEPC */
    LOAD t0,  0  * REGBYTES(sp)
    csrw CSR_MEPC, t0

    /* Interrupt still disable here */
    /* Restore Registers from Stack */
    LOAD x1,  1  * REGBYTES(sp)    /* RA */
    LOAD x5,  2  * REGBYTES(sp)
    LOAD x6,  3  * REGBYTES(sp)
    LOAD x7,  4  * REGBYTES(sp)
    LOAD x8,  5  * REGBYTES(sp)
    LOAD x9,  6  * REGBYTES(sp)
    LOAD x10, 7  * REGBYTES(sp)
    LOAD x11, 8  * REGBYTES(sp)
    LOAD x12, 9  * REGBYTES(sp)
    LOAD x13, 10 * REGBYTES(sp)
    LOAD x14, 11 * REGBYTES(sp)
    LOAD x15, 12 * REGBYTES(sp)
#ifndef __riscv_32e
    LOAD x16, 13 * REGBYTES(sp)
    LOAD x17, 14 * REGBYTES(sp)
    LOAD x18, 15 * REGBYTES(sp)
    LOAD x19, 16 * REGBYTES(sp)
    LOAD x20, 17 * REGBYTES(sp)
    LOAD x21, 18 * REGBYTES(sp)
    LOAD x22, 19 * REGBYTES(sp)
    LOAD x23, 20 * REGBYTES(sp)
    LOAD x24, 21 * REGBYTES(sp)
    LOAD x25, 22 * REGBYTES(sp)
    LOAD x26, 23 * REGBYTES(sp)
    LOAD x27, 24 * REGBYTES(sp)
    LOAD x28, 25 * REGBYTES(sp)
    LOAD x29, 26 * REGBYTES(sp)
    LOAD x30, 27 * REGBYTES(sp)
    LOAD x31, 28 * REGBYTES(sp)
#endif
    addi sp, sp, portCONTEXT_SIZE
    mret
