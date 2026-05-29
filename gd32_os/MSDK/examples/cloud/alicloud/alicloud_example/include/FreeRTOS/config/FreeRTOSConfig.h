/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 */
#include "wrapper_os_config.h"

/*----------------------------------------------------------*/
#define configUSE_PREEMPTION            1
#define configUSE_IDLE_HOOK             1
#define configUSE_TICK_HOOK             0
#define configCPU_CLOCK_HZ              OS_CPU_CLOCK_HZ
#define configTICK_RATE_HZ              OS_TICK_RATE_HZ
#define configMAX_PRIORITIES            OS_TASK_PRIO_MAX
#define configMINIMAL_STACK_SIZE        ( ( unsigned short ) 224 ) //256
#define configMAX_TASK_NAME_LEN         ( 16 )
#define configUSE_16_BIT_TICKS          0
#define configIDLE_SHOULD_YIELD         0
#define configUSE_MALLOC_FAILED_HOOK    0
#if 1//def RTOS_STACK_CHECK
#define configCHECK_FOR_STACK_OVERFLOW  2//1//2
#else
#define configCHECK_FOR_STACK_OVERFLOW  0
#endif /* RTOS_CHECK_STACK */
#define configUSE_TRACE_FACILITY        1
#if (configUSE_TRACE_FACILITY == 1)
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1
#define configGENERATE_RUN_TIME_STATS   1
#endif
#if (configGENERATE_RUN_TIME_STATS == 1)
#define portGET_RUN_TIME_COUNTER_VALUE()    xTickCount
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#endif
#define configUSE_MUTEXES               1

#define configUSE_TIMERS                1
//  <o>Timer task stack depth [words]
#define configTIMER_TASK_STACK_DEPTH    512
//  <o>Timer task priority, default lowest priority
#define configTIMER_TASK_PRIORITY       OS_TASK_PRIORITY(3)
//  <o>Timer queue length
#define configTIMER_QUEUE_LENGTH        (5)

#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 1

#define configUSE_TICKLESS_IDLE         1

#define configUSE_COUNTING_SEMAPHORES   1

#if configUSE_TICKLESS_IDLE
extern void freertos_pre_sleep_processing(unsigned long long *expected_idle_time);
extern void freertos_post_sleep_processing(unsigned long long *expected_idle_time);
extern int  freertos_ready_to_sleep(void);

#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP   5

#define configPRE_SLEEP_PROCESSING( x )         ( freertos_pre_sleep_processing((unsigned long long *)&x) )

#define configPOST_SLEEP_PROCESSING( x )        ( freertos_post_sleep_processing((unsigned long long *)&x) )

#endif

#define USE_HEAP_SECTION 1

#if USE_HEAP_SECTION
#include "boot.h"
/* use .heap section for HEAP.
   .heap section is defined to use all the remaining data memory after the link */
#define configAPPLICATION_ALLOCATED_HEAP 1
#define configTOTAL_HEAP_SIZE            ((uint32_t)__heap_top - (uint32_t)__heap_bottom)
#else
/* use constant size table for HEAP */
#define configTOTAL_HEAP_SIZE            ( ( size_t ) ( 80 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP 0
#endif

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES           0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function.
We use --gc-sections when linking, so there is no harm is setting all of these to 1 */

#define INCLUDE_vTaskPrioritySet        1
#define INCLUDE_uxTaskPriorityGet       1
#define INCLUDE_vTaskDelete             1
#define INCLUDE_vTaskCleanUpResources   1
#define INCLUDE_vTaskSuspend            1
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelay              1
#define INCLUDE_eTaskGetState           1
#define INCLUDE_xQueueGetMutexHolder    1
#define INCLUDE_xTaskGetIdleTaskHandle  1
#define INCLUDE_xTaskGetHandle          1

#define configENABLE_FPU                1
#define configENABLE_MPU                0
#define configENABLE_TRUSTZONE          0

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
    /* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
    #define configPRIO_BITS             __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS             4        /* 15 priority levels */
#endif

#define configRECORD_STACK_HIGH_ADDRESS 0

/* sw and timer interrupt priority */
#define configKERNEL_INTERRUPT_PRIORITY         0
/* max syscall priority, a larger interrupt priorities value indicates a higher priority, priority support 0-15 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    10

#ifdef CONFIG_AZURE_IOT_SUPPORT
#define configSUPPORT_STATIC_ALLOCATION         1

extern void vLoggingPrintf( const char * pcFormatString,
                            ... );
#define configPRINTF( X )    vLoggingPrintf X
#define configUSE_DAEMON_TASK_STARTUP_HOOK      1

#define configASSERT( x )                                                    \
    if( ( x ) == 0 )                                                         \
    {                                                                        \
        vLoggingPrintf( "[FATAL] [%s:%d] %s\r\n", __func__, __LINE__, # x ); \
        do {                                                                        \
            int mstatus;                                                            \
            __asm__ volatile ("csrrci %0, mstatus, %1" : "=r" (mstatus) : "i" (8)); \
        } while(0);                                                         \
        for( ; ; ) {; };                                                      \
    }

#endif /* CONFIG_AZURE_IOT_SUPPORT */

#define configUSE_RECURSIVE_MUTEXES 1

#endif /* FREERTOS_CONFIG_H */
