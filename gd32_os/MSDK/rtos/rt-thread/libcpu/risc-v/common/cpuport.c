/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018/10/28     Bernard      The unify RISC-V porting code.
 * 2020/11/20     BalanceTWK   Add FPU support
 * 2023/01/04     WangShun     Adapt to CH32
 */

#include <rthw.h>
#include <rtthread.h>

#include "cpuport.h"
#include "rt_hw_stack_frame.h"
#include "gd32vw55x.h"

#define portINITIAL_MSTATUS                         ( MSTATUS_MPP | MSTATUS_MPIE | MSTATUS_FS_DIRTY)

/* Scheduler utilities. */
#define portYIELD()                                                             \
    {                                                                               \
        /* Set a software interrupt(SWI) request to request a context switch. */    \
        SysTimer_SetSWIRQ();                                                        \
        /* Barriers are normally not required but do ensure the code is completely  \
        within the specified behaviour for the architecture. */                     \
        __RWMB();                                                                   \
    }

/* Masks off all bits but the ECLIC MTH bits in the MTH register. */
#define portMTH_MASK                ( 0xFFUL )
static uint32_t rtCriticalNesting = 0;
static uint8_t rtMaxSysCallMTH = 255;
/* max syscall priority, a larger interrupt priorities value indicates a higher priority, priority support 0-15 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    10

#ifndef RT_USING_SMP
volatile rt_ubase_t  rt_interrupt_from_thread = 0;
volatile rt_ubase_t  rt_interrupt_to_thread   = 0;
volatile rt_uint32_t rt_thread_switch_interrupt_flag = 0;
#endif

/**
 * This function will initialize thread stack
 *
 * @param tentry the entry of thread
 * @param parameter the parameter of entry
 * @param stack_addr the beginning stack address
 * @param texit the function will be called when thread exit
 *
 * @return stack address
 */
rt_uint8_t *rt_hw_stack_init(void       *tentry,
                             void       *parameter,
                             rt_uint8_t *stack_addr,
                             void       *texit)
{
    struct rt_hw_stack_frame *frame;
    rt_uint8_t         *stk;
    int                i;

    stk  = stack_addr + sizeof(rt_ubase_t);
    stk  = (rt_uint8_t *)RT_ALIGN_DOWN((rt_ubase_t)stk, REGBYTES);
    stk -= sizeof(struct rt_hw_stack_frame);

    frame = (struct rt_hw_stack_frame *)stk;

    for (i = 0; i < sizeof(struct rt_hw_stack_frame) / sizeof(rt_ubase_t); i++)
    {
        ((rt_ubase_t *)frame)[i] = 0xdeadbeef;
    }

    frame->ra      = (rt_ubase_t)texit;
    frame->a0      = (rt_ubase_t)parameter;
    frame->epc     = (rt_ubase_t)tentry;

    /* force to machine mode(MPP=11) and set MPIE to 1 */
#ifdef __riscv_flen
    frame->mstatus = portINITIAL_MSTATUS;
#else
    frame->mstatus = 0x1880;
#endif
#ifdef __riscv_flen
    frame->fcsr = 0;
#endif

    return stk;
}
RTM_EXPORT(rt_hw_stack_init);

extern void SysTimer_SetSWIRQ(void);
rt_weak void rt_trigger_software_interrupt(void)
{
    while (0);
}

rt_weak void rt_hw_do_after_save_above(void)
{
    while (1);
}

/*
 * #ifdef RT_USING_SMP
 * void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread);
 * #else
 * void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to);
 * #endif
 */
#ifndef RT_USING_SMP
rt_weak void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to, rt_thread_t from_thread, rt_thread_t to_thread)
{
    if (rt_thread_switch_interrupt_flag == 0)
        rt_interrupt_from_thread = from;

    rt_interrupt_to_thread = to;
    rt_thread_switch_interrupt_flag = 1;

    //rt_trigger_software_interrupt();
    portYIELD();

    return ;
}
#endif /* end of RT_USING_SMP */

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to)
{
    rt_hw_context_switch_interrupt(from, to, NULL, NULL);
}

/** shutdown CPU */
void rt_hw_cpu_shutdown()
{
    rt_uint32_t level;
    rt_kprintf("shutdown...\n");

    level = rt_hw_interrupt_disable();
    while (level) {
        RT_ASSERT(0);
    }
}

void xPortTaskSwitch(void)
{
    /* Clear Software IRQ, A MUST */
    SysTimer_ClearSWIRQ();
    rt_thread_switch_interrupt_flag = 0;
    // make from thread to be to thread
    // If there is another swi interrupt triggered by other harts
    // not through rt_hw_context_switch or rt_hw_context_switch_interrupt
    // the task switch should just do a same task save and restore
    rt_interrupt_from_thread = rt_interrupt_to_thread;
}

rt_base_t rt_hw_interrupt_disable(void)
{
#if 1
    return vPortEnterCritical();
#else
    return __RV_CSR_READ_CLEAR(CSR_MSTATUS, MSTATUS_MIE);
#endif
}

void rt_hw_interrupt_enable(rt_base_t level)
{
#if 1
    return vPortExitCritical();
#else
    __RV_CSR_WRITE(CSR_MSTATUS, level);
#endif
}

static uint8_t prvCalcMaxSysCallMTH(uint8_t max_syscall_prio)
{
    uint8_t nlbits = __ECLIC_GetCfgNlbits();
    uint8_t intctlbits = __ECLIC_INTCTLBITS;
    uint8_t lvlbits, lfabits;
    uint8_t maxsyscallmth = 0;
    uint8_t temp;

    if (nlbits <= intctlbits) {
        lvlbits = nlbits;
    } else {
        lvlbits = intctlbits;
    }

    lfabits = 8 - lvlbits;

    temp = ((1 << lvlbits) - 1);
    if (max_syscall_prio > temp) {
        max_syscall_prio = temp;
    }

    maxsyscallmth = (max_syscall_prio << lfabits) | ((1 << lfabits) - 1);
    return maxsyscallmth;
}

void vPortCriticalInit(void)
{
    rtMaxSysCallMTH = prvCalcMaxSysCallMTH(configMAX_SYSCALL_INTERRUPT_PRIORITY);
}
RTM_EXPORT(vPortCriticalInit);

void vPortEnterCritical(void)
{
    ECLIC_SetMth(rtMaxSysCallMTH);
    __RWMB();

    rtCriticalNesting++;

    /* This is not the interrupt safe version of the enter critical function so
    assert() if it is being called from an interrupt context.  Only API
    functions that end in "FromISR" can be used in an interrupt.  Only assert if
    the critical nesting count is 1 to protect against recursive calls if the
    assert function also uses a critical section. */
    if (rtCriticalNesting == 1) {
        RT_ASSERT((__ECLIC_GetMth() & portMTH_MASK) == rtMaxSysCallMTH);
    }
}
RTM_EXPORT(vPortEnterCritical);

void vPortExitCritical(void)
{
    RT_ASSERT(rtCriticalNesting);
    rtCriticalNesting--;

    if (rtCriticalNesting == 0) {
        ECLIC_SetMth(0);
        __RWMB();
    }
}
RTM_EXPORT(vPortExitCritical);

uint32_t vPortInCritical(void)
{
    return rtCriticalNesting;
}
RTM_EXPORT(vPortInCritical);
