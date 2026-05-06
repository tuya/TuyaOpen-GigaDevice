/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018/10/28     Bernard      The unify RISC-V porting implementation
 * 2018/12/27     Jesven       Add SMP support
 * 2020/11/20     BalanceTWK   Add FPU support
 * 2022/12/28     WangShun     Add macro to distinguish whether FPU is supported
 * 2023/03/19     Flyingcys    Add riscv_32e support
 */

#define __ASSEMBLY__
//#include "cpuport.h"

#ifdef RT_USING_SMP
#define rt_hw_interrupt_disable rt_hw_local_irq_disable
#define rt_hw_interrupt_enable  rt_hw_local_irq_enable
#endif

#if 0
/*
 * rt_base_t rt_hw_interrupt_disable(void);
 */
    .globl rt_hw_interrupt_disable
rt_hw_interrupt_disable:
    csrrci a0, mstatus, 8
    ret

/*
 * void rt_hw_interrupt_enable(rt_base_t level);
 */
    .globl rt_hw_interrupt_enable
rt_hw_interrupt_enable:
    csrw mstatus, a0
    ret

/*
 * #ifdef RT_USING_SMP
 * void rt_hw_context_switch_to(rt_ubase_t to, stuct rt_thread *to_thread);
 * #else
 * void rt_hw_context_switch_to(rt_ubase_t to);
 * #endif
 * a0 --> to
 * a1 --> to_thread
 */
    .globl rt_hw_context_switch_to
rt_hw_context_switch_to:
    la t0, __rt_rvstack
    csrw mscratch,t0

    LOAD sp, (a0)

#ifdef RT_USING_SMP
    mv   a0,   a1
    call  rt_cpus_lock_status_restore
#endif
    LOAD a0,   2 * REGBYTES(sp)
    csrw mstatus, a0
    j    rt_hw_context_switch_exit

/*
 * #ifdef RT_USING_SMP
 * void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread);
 * #else
 * void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
 * #endif
 *
 * a0 --> from
 * a1 --> to
 * a2 --> to_thread
 */
    .globl rt_hw_context_switch
rt_hw_context_switch:
    /* saved from thread context
     *     x1/ra       -> sp(0)
     *     x1/ra       -> sp(1)
     *     mstatus.mie -> sp(2)
     *     x(i)        -> sp(i-4)
     */
#ifdef ARCH_RISCV_FPU
    addi    sp, sp, -32 * FREGBYTES

    FSTORE  f0, 0 * FREGBYTES(sp)
    FSTORE  f1, 1 * FREGBYTES(sp)
    FSTORE  f2, 2 * FREGBYTES(sp)
    FSTORE  f3, 3 * FREGBYTES(sp)
    FSTORE  f4, 4 * FREGBYTES(sp)
    FSTORE  f5, 5 * FREGBYTES(sp)
    FSTORE  f6, 6 * FREGBYTES(sp)
    FSTORE  f7, 7 * FREGBYTES(sp)
    FSTORE  f8, 8 * FREGBYTES(sp)
    FSTORE  f9, 9 * FREGBYTES(sp)
    FSTORE  f10, 10 * FREGBYTES(sp)
    FSTORE  f11, 11 * FREGBYTES(sp)
    FSTORE  f12, 12 * FREGBYTES(sp)
    FSTORE  f13, 13 * FREGBYTES(sp)
    FSTORE  f14, 14 * FREGBYTES(sp)
    FSTORE  f15, 15 * FREGBYTES(sp)
    FSTORE  f16, 16 * FREGBYTES(sp)
    FSTORE  f17, 17 * FREGBYTES(sp)
    FSTORE  f18, 18 * FREGBYTES(sp)
    FSTORE  f19, 19 * FREGBYTES(sp)
    FSTORE  f20, 20 * FREGBYTES(sp)
    FSTORE  f21, 21 * FREGBYTES(sp)
    FSTORE  f22, 22 * FREGBYTES(sp)
    FSTORE  f23, 23 * FREGBYTES(sp)
    FSTORE  f24, 24 * FREGBYTES(sp)
    FSTORE  f25, 25 * FREGBYTES(sp)
    FSTORE  f26, 26 * FREGBYTES(sp)
    FSTORE  f27, 27 * FREGBYTES(sp)
    FSTORE  f28, 28 * FREGBYTES(sp)
    FSTORE  f29, 29 * FREGBYTES(sp)
    FSTORE  f30, 30 * FREGBYTES(sp)
    FSTORE  f31, 31 * FREGBYTES(sp)

#endif
#ifndef __riscv_32e
    addi  sp,  sp, -32 * REGBYTES
#else
    addi  sp,  sp, -16 * REGBYTES
#endif

    STORE sp,  (a0)

    STORE x1,   0 * REGBYTES(sp)
    STORE x1,   1 * REGBYTES(sp)

    csrr a0, mstatus
    andi a0, a0, 8
    beqz a0, save_mpie
    li   a0, 0x80
save_mpie:
    STORE a0,   2 * REGBYTES(sp)

    STORE x4,   4 * REGBYTES(sp)
    STORE x5,   5 * REGBYTES(sp)
    STORE x6,   6 * REGBYTES(sp)
    STORE x7,   7 * REGBYTES(sp)
    STORE x8,   8 * REGBYTES(sp)
    STORE x9,   9 * REGBYTES(sp)
    STORE x10, 10 * REGBYTES(sp)
    STORE x11, 11 * REGBYTES(sp)
    STORE x12, 12 * REGBYTES(sp)
    STORE x13, 13 * REGBYTES(sp)
    STORE x14, 14 * REGBYTES(sp)
    STORE x15, 15 * REGBYTES(sp)
#ifndef __riscv_32e
    STORE x16, 16 * REGBYTES(sp)
    STORE x17, 17 * REGBYTES(sp)
    STORE x18, 18 * REGBYTES(sp)
    STORE x19, 19 * REGBYTES(sp)
    STORE x20, 20 * REGBYTES(sp)
    STORE x21, 21 * REGBYTES(sp)
    STORE x22, 22 * REGBYTES(sp)
    STORE x23, 23 * REGBYTES(sp)
    STORE x24, 24 * REGBYTES(sp)
    STORE x25, 25 * REGBYTES(sp)
    STORE x26, 26 * REGBYTES(sp)
    STORE x27, 27 * REGBYTES(sp)
    STORE x28, 28 * REGBYTES(sp)
    STORE x29, 29 * REGBYTES(sp)
    STORE x30, 30 * REGBYTES(sp)
    STORE x31, 31 * REGBYTES(sp)
#endif
    /* restore to thread context
     * sp(0) -> epc;
     * sp(1) -> ra;
     * sp(i) -> x(i+2)
     */
    LOAD sp,  (a1)

#ifdef RT_USING_SMP
    mv   a0,   a2
    call  rt_cpus_lock_status_restore
#endif /*RT_USING_SMP*/

    j rt_hw_context_switch_exit

#ifdef RT_USING_SMP
/*
 * void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread);
 *
 * a0 --> context
 * a1 --> from
 * a2 --> to
 * a3 --> to_thread
 */
    .globl rt_hw_context_switch_interrupt
rt_hw_context_switch_interrupt:

    STORE a0, 0(a1)

    LOAD  sp, 0(a2)
    move  a0, a3
    call rt_cpus_lock_status_restore

    j rt_hw_context_switch_exit

#endif

.global rt_hw_context_switch_exit
rt_hw_context_switch_exit:
#ifdef RT_USING_SMP
#ifdef RT_USING_SIGNALS
    mv a0, sp

    csrr  t0, mhartid
    /* switch interrupt stack of current cpu */
    la    sp, __stack_start__
    addi  t1, t0, 1
    li    t2, __STACKSIZE__
    mul   t1, t1, t2
    add   sp, sp, t1 /* sp = (cpuid + 1) * __STACKSIZE__ + __stack_start__ */

    call rt_signal_check
    mv sp, a0
#endif
#endif
    /* resw ra to mepc */
    LOAD a0,   0 * REGBYTES(sp)
    csrw mepc, a0

    LOAD x1,   1 * REGBYTES(sp)
#ifdef ARCH_RISCV_FPU
    li    t0, 0x7800
#else
    li    t0, 0x1800
#endif
    csrw  mstatus, t0
    LOAD a0,   2 * REGBYTES(sp)
    csrs mstatus, a0

    LOAD x4,   4 * REGBYTES(sp)
    LOAD x5,   5 * REGBYTES(sp)
    LOAD x6,   6 * REGBYTES(sp)
    LOAD x7,   7 * REGBYTES(sp)
    LOAD x8,   8 * REGBYTES(sp)
    LOAD x9,   9 * REGBYTES(sp)
    LOAD x10, 10 * REGBYTES(sp)
    LOAD x11, 11 * REGBYTES(sp)
    LOAD x12, 12 * REGBYTES(sp)
    LOAD x13, 13 * REGBYTES(sp)
    LOAD x14, 14 * REGBYTES(sp)
    LOAD x15, 15 * REGBYTES(sp)
#ifndef __riscv_32e
    LOAD x16, 16 * REGBYTES(sp)
    LOAD x17, 17 * REGBYTES(sp)
    LOAD x18, 18 * REGBYTES(sp)
    LOAD x19, 19 * REGBYTES(sp)
    LOAD x20, 20 * REGBYTES(sp)
    LOAD x21, 21 * REGBYTES(sp)
    LOAD x22, 22 * REGBYTES(sp)
    LOAD x23, 23 * REGBYTES(sp)
    LOAD x24, 24 * REGBYTES(sp)
    LOAD x25, 25 * REGBYTES(sp)
    LOAD x26, 26 * REGBYTES(sp)
    LOAD x27, 27 * REGBYTES(sp)
    LOAD x28, 28 * REGBYTES(sp)
    LOAD x29, 29 * REGBYTES(sp)
    LOAD x30, 30 * REGBYTES(sp)
    LOAD x31, 31 * REGBYTES(sp)

    addi sp,  sp, 32 * REGBYTES
#else
    addi sp,  sp, 16 * REGBYTES
#endif

#ifdef ARCH_RISCV_FPU
    FLOAD   f0, 0 * FREGBYTES(sp)
    FLOAD   f1, 1 * FREGBYTES(sp)
    FLOAD   f2, 2 * FREGBYTES(sp)
    FLOAD   f3, 3 * FREGBYTES(sp)
    FLOAD   f4, 4 * FREGBYTES(sp)
    FLOAD   f5, 5 * FREGBYTES(sp)
    FLOAD   f6, 6 * FREGBYTES(sp)
    FLOAD   f7, 7 * FREGBYTES(sp)
    FLOAD   f8, 8 * FREGBYTES(sp)
    FLOAD   f9, 9 * FREGBYTES(sp)
    FLOAD   f10, 10 * FREGBYTES(sp)
    FLOAD   f11, 11 * FREGBYTES(sp)
    FLOAD   f12, 12 * FREGBYTES(sp)
    FLOAD   f13, 13 * FREGBYTES(sp)
    FLOAD   f14, 14 * FREGBYTES(sp)
    FLOAD   f15, 15 * FREGBYTES(sp)
    FLOAD   f16, 16 * FREGBYTES(sp)
    FLOAD   f17, 17 * FREGBYTES(sp)
    FLOAD   f18, 18 * FREGBYTES(sp)
    FLOAD   f19, 19 * FREGBYTES(sp)
    FLOAD   f20, 20 * FREGBYTES(sp)
    FLOAD   f21, 21 * FREGBYTES(sp)
    FLOAD   f22, 22 * FREGBYTES(sp)
    FLOAD   f23, 23 * FREGBYTES(sp)
    FLOAD   f24, 24 * FREGBYTES(sp)
    FLOAD   f25, 25 * FREGBYTES(sp)
    FLOAD   f26, 26 * FREGBYTES(sp)
    FLOAD   f27, 27 * FREGBYTES(sp)
    FLOAD   f28, 28 * FREGBYTES(sp)
    FLOAD   f29, 29 * FREGBYTES(sp)
    FLOAD   f30, 30 * FREGBYTES(sp)
    FLOAD   f31, 31 * FREGBYTES(sp)

    addi    sp, sp, 32 * FREGBYTES
#endif

    mret

#else
#include "riscv_encoding.h"

#ifndef __riscv_32e
#define portRegNum          29
#else
#define portRegNum          13
#endif

#define portCONTEXT_SIZE        ( portRegNum * REGBYTES )

#ifdef __riscv_flen
#define portFCONTEXT_SIZE       ( 32 * FPREGBYTES + REGBYTES )
#endif

    .extern rt_interrupt_from_thread
    .extern rt_interrupt_to_thread


.section    .text
/*
 * void rt_hw_context_switch_to(rt_ubase_t to);
 * a0 --> to_thread
 */
    .globl rt_hw_context_switch_to

/* Start the first task.  This also clears the bit that indicates the FPU is
    in use in case the FPU was used before the scheduler was started - which
    would otherwise result in the unnecessary leaving of space in the stack
    for lazy saving of FPU registers. */
.align 3
rt_hw_context_switch_to:
    /* Setup Interrupt Stack using
       The stack that was used by main()
       before the scheduler is started is
       no longer required after the scheduler is started.
       Interrupt stack pointer is stored in CSR_MSCRATCH */
    la t0, _sp
    csrw CSR_MSCRATCH, t0
    LOAD sp, 0x0(a0)           /* Read sp from first TCB member(a0) */

    /* Pop mstatus from stack and set it */
    LOAD t0,  0  * REGBYTES(sp)
    csrw CSR_MSTATUS, t0
    addi sp, sp, 1  * REGBYTES

#ifdef __riscv_flen
    addi sp, sp, 32 * FPREGBYTES    /* f0-f31 */
    LOAD t0, 0 * REGBYTES(sp)
    csrw CSR_FCSR, t0
    addi sp, sp, 1  * REGBYTES      /* FCSR */
    /* Write mstatus->FS as clean to clear mstatus->SD flag */
    lui t0, 0x2
    csrc CSR_MSTATUS, t0
#endif

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
    STORE x10, 7  * REGBYTES(sp)   /* a0 */
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
    LOAD t0, rt_interrupt_from_thread
    STORE sp, 0(t0)

    jal xPortTaskSwitch

    /* Switch task context */
    LOAD t0, rt_interrupt_to_thread
    LOAD sp, 0x0(t0)

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

    /* Write mstatus->FS as clean to clear mstatus->SD flag */
    lui t0, 0x2
    csrc CSR_MSTATUS, t0
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

#endif
