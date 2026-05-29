/*
 * FreeRTOS FreeRTOS LTS Qualification Tests preview
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 */

/**
 * @file plaform_function.h
 * @brief Declaration of platform functions that users need to implement.
 */

#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "UNITY"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

#include "logging_stack.h"

#include "logging.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "platform_function.h"
#include "systime.h"
#include "wrapper_os.h"
#include <stdio.h>
#include "network_connection.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct TaskParam
{
    StaticSemaphore_t joinMutexBuffer;
    SemaphoreHandle_t joinMutexHandle;
    FRTestThreadFunction_t threadFunc;
    void * pParam;
    TaskHandle_t taskHandle;
} TaskParam_t;

/**
 * @brief Delay function to wait for at least specified amount of time.
 *
 * @param[in] delayMs Delay in milliseconds.
 */
void FRTest_TimeDelay( uint32_t delayMs )
{
    sys_ms_sleep(delayMs);
}

/**
 * @brief Function to get time elapsed in milliseconds since a given epoch.
 *
 * @note The timer should be a monotonic timer. It just needs to provide an
 * incrementing count of milliseconds elapsed since a given epoch.
 *
 * @return Time elapsed since the given epoch in milliseconds.
 */
uint32_t FRTest_GetTimeMs( void )
{
    return get_sys_local_time_ms();
}

static void ThreadWrapper( void * pParam )
{
    TaskParam_t * pTaskParam = pParam;

    if( ( pTaskParam != NULL ) && ( pTaskParam->threadFunc != NULL ) && ( pTaskParam->joinMutexHandle != NULL ) )
    {
        pTaskParam->threadFunc( pTaskParam->pParam );

        /* Give the mutex. */
        xSemaphoreGive( pTaskParam->joinMutexHandle );
    }

    vTaskDelete( NULL );
}

/**
 * @brief Thread create function for test application.
 *
 * @param[in] threadFunc The thread function to be executed in the created thread.
 * @param[in] pParam The pParam parameter passed to the thread function pParam parameter.
 *
 * @return NULL if create thread failed. Otherwise, return the handle of the created thread.
 */
FRTestThreadHandle_t FRTest_ThreadCreate( FRTestThreadFunction_t threadFunc, void * pParam )
{
    TaskParam_t * pTaskParam = NULL;
    FRTestThreadHandle_t threadHandle = NULL;
    BaseType_t xReturned;

    pTaskParam = sys_malloc( sizeof( TaskParam_t ) );
    configASSERT( pTaskParam != NULL );

    pTaskParam->joinMutexHandle = xSemaphoreCreateBinary();
//        xSemaphoreCreateBinaryStatic( &pTaskParam->joinMutexBuffer );
    configASSERT( pTaskParam->joinMutexHandle != NULL );

    pTaskParam->threadFunc = threadFunc;
    pTaskParam->pParam = pParam;

    xReturned = xTaskCreate( ThreadWrapper,    /* Task code. */
                             "ThreadWrapper",  /* All tasks have same name. */
                             4096,             /* Task stack size. */
                             pTaskParam,       /* Where the task writes its result. */
                             1, /* Task priority. */
                             &pTaskParam->taskHandle );
    configASSERT( xReturned == pdPASS );

    threadHandle = pTaskParam;

    return threadHandle;

}

/**
 * @brief Timed thread join function to wait for the created thread exit.
 *
 * @param[in] threadHandle The handle of the created thread to be waited.
 * @param[in] timeoutMs The timeout value of to wait for the created thread exit.
 *
 * @return 0 if the thread exits within timeoutMs. Other value will be regarded as error.
 */
int FRTest_ThreadTimedJoin( FRTestThreadHandle_t threadHandle, uint32_t timeoutMs )
{
    TaskParam_t * pTaskParam = threadHandle;
    BaseType_t xReturned;
    int retValue = 0;

    /* Check the parameters. */
    configASSERT( pTaskParam != NULL );
    configASSERT( pTaskParam->joinMutexHandle != NULL );

    /* Wait for the thread. */
    xReturned = xSemaphoreTake( pTaskParam->joinMutexHandle, pdMS_TO_TICKS( timeoutMs ) );

    if( xReturned != pdTRUE )
    {
        LogWarn( ( "Waiting thread exist failed after %u %d. Task abort.", timeoutMs, xReturned ) );

        /* Return negative value to indicate error. */
        retValue = -1;

        /* There may be used after free. Assert here to indicate error. */
        configASSERT( 0 );
    }

    sys_mfree( pTaskParam );

    return retValue;

}

/**
 * @brief Malloc function to allocate memory for test.
 *
 * @param[in] size Size in bytes.
 */
void * FRTest_MemoryAlloc( size_t size )
{
    return sys_malloc(size);
}

/**
 * @brief Free function to free memory allocated by FRTest_MemoryAlloc function.
 *
 * @param[in] size Size in bytes.
 */
void FRTest_MemoryFree( void * ptr )
{
    sys_mfree(ptr);
}

/**
 * @brief To generate random number in INT format.
 *
 * @return A random number.
 */
int FRTest_GenerateRandInt()
{
    return rand();
}

