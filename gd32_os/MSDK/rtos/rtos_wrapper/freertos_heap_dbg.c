/*!
    \file    freertos_heap_dbg.c
    \brief   Heap check for GD32VW55x SDK

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

#include <dbg_print.h>
#include "wrapper_os.h"
#include "compiler.h"

#ifdef CFG_HEAP_MEM_CHECK
#include "ll.h"
#include "dlist.h"

#define MEMORY_CHK_HEAD_LEN           12
#define MEMORY_CHK_TOTAL_LEN          16
#define RET_ADDR_LEN                  4
#define MEMORY_SIZE_LEN               4
#define MAGIC_CODE_LEN                4

const static uint8_t magic_head[] = {0x74, 0x69, 0x6e, 0x79};
const static uint8_t magic_tail[] = {0x62, 0x69, 0x72, 0x64};
static bool init_done = false;

// memory content | return address(4 bytes) | size(4 bytes) | magic_head(4 bytes) | memory | magic_tail(4 bytes)
typedef struct
{
    dlist_t             list;
    void               *p_mem;    // point to return address
} mem_alloc_t;

/// BLE app taks callback list
static dlist_t heap_mem_list;

void mem_assert_err(void)
{
    //Make a exception
    *(uint8_t *)0xFFFF0001 = 1;
}

void sys_heap_malloc_dump(bool all)
{
    dlist_t *pos, *n;
    mem_alloc_t *p_mem;
    mem_alloc_t *p_former_mem = NULL;
    uint8_t *p;
    uint8_t idx = 0;
    size_t old_size;

    GLOBAL_INT_DISABLE();

    if (!init_done || list_empty(&heap_mem_list)) {
        GLOBAL_INT_RESTORE();
        return;
    }
    if (all)
        printf("sys_heap_malloc_dump: \r\n");

    list_for_each_safe(pos, n, &heap_mem_list) {
        p_mem = list_entry(pos, mem_alloc_t, list);
        p = (uint8_t *)p_mem->p_mem;
        old_size = *(uint32_t *)(p + RET_ADDR_LEN);
        p += (RET_ADDR_LEN + MEMORY_SIZE_LEN);    // point to magic code
        if(sys_memcmp(p, magic_head, MAGIC_CODE_LEN) != 0) {
            uint8_t *p_former = (uint8_t *)p_former_mem->p_mem;;
            if(p_former_mem) {
                printf("sys_heap_malloc_dump former return address 0x%x %p \r\n", *(uint32_t *)(p_former), p_former);
            }
            printf("sys_heap_malloc_dump return address 0x%x %p magic header damaged! \r\n", *(uint32_t *)(p - MEMORY_SIZE_LEN - RET_ADDR_LEN), p);
            //mem_assert_err();
        }
        else if(sys_memcmp((p + MAGIC_CODE_LEN + old_size), magic_tail, MAGIC_CODE_LEN) != 0) {
           printf("sys_heap_malloc_dump return address 0x%x %p magic tail damaged! \r\n", *(uint32_t *)(p - MEMORY_SIZE_LEN - RET_ADDR_LEN), p);
           //mem_assert_err();
        }
        else {
            if (all) {
                printf("ra 0x%x, buf %p, size %d; ", *(uint32_t *)(p - MEMORY_SIZE_LEN - RET_ADDR_LEN), p_mem, *(uint32_t *)(p - MEMORY_SIZE_LEN));
                idx = (idx + 1) % 4;
                if (idx == 0)
                    printf("\r\n");
            }
        }
        p_former_mem = p_mem;
    }
    printf("\r\n");
    GLOBAL_INT_RESTORE();
}

/***************** heap management implementation *****************/
/*!
    \brief      allocate a block of memory with a minimum of 'size' bytes.
    \param[in]  size: the minimum size of the requested block in bytes
    \param[out] none
    \retval     address to allocated memory, NULL pointer if there is an error
*/
void *sys_malloc(size_t size)
{
    mem_alloc_t *p_mem;
    register uint32_t value;

    {
        __asm volatile("mv %0, ra"            \
                      : "=r"(value)            \
                      :                        \
                      : "memory");             \
    }

    p_mem = (mem_alloc_t *)pvPortMalloc(size + sizeof(mem_alloc_t) + MEMORY_CHK_TOTAL_LEN);
    //co_printf("malloc %p, %u\r\n", p_mem, size);
    if (p_mem != NULL) {
        INIT_DLIST_HEAD(&p_mem->list);
        uint8_t *p = (uint8_t *)(p_mem + 1);
        p_mem->p_mem = p_mem + 1; // point to return address
        *(uint32_t *)p = value;
        p += RET_ADDR_LEN;
        *(uint32_t *)p = size;
        p += MEMORY_SIZE_LEN;
        sys_memcpy(p, magic_head, MAGIC_CODE_LEN);
        p += MAGIC_CODE_LEN;
        sys_memcpy(p + size, magic_tail, MAGIC_CODE_LEN);

        GLOBAL_INT_DISABLE();
        if (!init_done) {
            INIT_DLIST_HEAD(&heap_mem_list);
            init_done = true;
        }
        list_add_tail(&p_mem->list, &heap_mem_list);
        GLOBAL_INT_RESTORE();

        return (void *)p;
    }
    return NULL;
}

/*!
    \brief      allocate a certian chunks of memory with specified size
                Note: The allocated memory is filled with bytes of value zero.
                All chunks in the allocated memory are contiguous.
    \param[in]  count: multiple number of size want to malloc
    \param[in]  size:  number of size want to malloc
    \param[out] none
    \retval     address to allocated memory, NULL pointer if there is an error
*/
void *sys_calloc(size_t count, size_t size)
{
    mem_alloc_t *p_mem;
    register uint32_t value;

    {
        __asm volatile("mv %0, ra"            \
                      : "=r"(value)            \
                      :                        \
                      : "memory");             \
    }

    size = count * size;
    p_mem = (mem_alloc_t *)pvPortMalloc(size + sizeof(mem_alloc_t) + MEMORY_CHK_TOTAL_LEN);
    //co_printf("calloc %p, %u\r\n", p_mem, size);
    if (p_mem != NULL) {
        INIT_DLIST_HEAD(&p_mem->list);
        uint8_t *p = (uint8_t *)(p_mem + 1);
        p_mem->p_mem = p_mem + 1;   // point to return address
        *(uint32_t *)p = value;
        p += RET_ADDR_LEN;
        *(uint32_t *)p = size;
        p += MEMORY_SIZE_LEN;
        sys_memcpy(p, magic_head, MAGIC_CODE_LEN);
        p += MAGIC_CODE_LEN;

        sys_memset(p, 0, size);
        sys_memcpy(p + size, magic_tail, MAGIC_CODE_LEN);

        GLOBAL_INT_DISABLE();
        if (!init_done) {
            INIT_DLIST_HEAD(&heap_mem_list);
            init_done = true;
        }
        list_add_tail(&p_mem->list, &heap_mem_list);
        GLOBAL_INT_RESTORE();

        return (void *)p;
    }
    return NULL;
}

/*!
    \brief      change the size of a previously allocated memory block.
    \param[in]  mem: address to the old buffer
    \param[in]  size: number of the new buffer size
    \param[out] none
    \retval     address to allocated memory, NULL pointer if there is an error
*/
void *sys_realloc(void *mem, size_t size)
{
    void *mem_ptr = NULL;
    mem_alloc_t * p_mem;
    mem_alloc_t * p_old_mem = NULL;
    register uint32_t value;

    {
        __asm volatile("mv %0, ra"            \
                      : "=r"(value)            \
                      :                        \
                      : "memory");             \
    }

    GLOBAL_INT_DISABLE();
    if (!init_done) {
        INIT_DLIST_HEAD(&heap_mem_list);
        init_done = true;
    }
    GLOBAL_INT_RESTORE();

    if(mem != NULL) {
        uint8_t *p = (uint8_t *)mem;
        mem_ptr = (void *)(p - MEMORY_CHK_HEAD_LEN - sizeof(mem_alloc_t));
        size_t old_size = *(uint32_t *)(p - MAGIC_CODE_LEN - MEMORY_SIZE_LEN);
        if(sys_memcmp((p - MAGIC_CODE_LEN), magic_head, MAGIC_CODE_LEN) != 0 ||
           sys_memcmp((p + old_size), magic_tail, MAGIC_CODE_LEN) != 0) {
           printf("sys_realloc return address 0x%x %p damaged!\r\n", *(uint32_t *)(p - MEMORY_CHK_HEAD_LEN), mem_ptr);
           mem_assert_err();
        }
        p_old_mem = (mem_alloc_t *)mem_ptr;
        GLOBAL_INT_DISABLE();
        list_del(&p_old_mem->list);
        GLOBAL_INT_RESTORE();
    }

    p_mem = (mem_alloc_t *)pvPortReAlloc(mem_ptr, size + sizeof(mem_alloc_t) + MEMORY_CHK_TOTAL_LEN);
    //co_printf("realloc %p, %u\r\n", p_mem, size);
    if (p_mem != NULL) {
        uint8_t *p = (uint8_t *)(p_mem + 1);
        INIT_DLIST_HEAD(&p_mem ->list);
        p_mem->p_mem = p_mem + 1; // point to return address
        *(uint32_t *)p = value;
        p += RET_ADDR_LEN;
        *(uint32_t *)p = size;
        p += MEMORY_SIZE_LEN;
        sys_memcpy(p, magic_head, MAGIC_CODE_LEN);
        p += MAGIC_CODE_LEN;
        sys_memcpy(p + size, magic_tail, MAGIC_CODE_LEN);

        GLOBAL_INT_DISABLE();
        list_add_tail(&p_mem->list, &heap_mem_list);
        GLOBAL_INT_RESTORE();

        return (void *)p;
    }
    // realloc fail , re-insert old mem
    else if(p_old_mem != NULL){
        GLOBAL_INT_DISABLE();
        list_add_tail(&p_old_mem->list, &heap_mem_list);
        GLOBAL_INT_RESTORE();
    }

    return NULL;
}

/*!
    \brief      free a memory to the heap
    \param[in]  ptr: pointer to the address want to free
    \param[out] none
    \retval     none
*/
void sys_mfree(void *ptr)
{

    mem_alloc_t * p_mem;

    if(ptr != NULL) {
        uint8_t *p = (uint8_t *)ptr;
        size_t old_size = *(uint32_t *)(p - MAGIC_CODE_LEN - MEMORY_SIZE_LEN);
        ptr = (p - MEMORY_CHK_HEAD_LEN - sizeof(mem_alloc_t));
        //co_printf("free %p\r\n", ptr);
        if (sys_memcmp((p - MAGIC_CODE_LEN), magic_head, MAGIC_CODE_LEN) != 0) {
            printf("sys_mfree return address 0x%x %p, header damaged!\r\n",
                  *(uint32_t *)(p - MEMORY_CHK_HEAD_LEN),
                    (p - MEMORY_CHK_HEAD_LEN - sizeof(mem_alloc_t)));
            sys_heap_malloc_dump(true);
            mem_assert_err();
        } else if (sys_memcmp((p + old_size), magic_tail, MAGIC_CODE_LEN) != 0) {
            printf("sys_mfree return address 0x%x %p, tail damaged!\r\n",
                  *(uint32_t *)(p - MEMORY_CHK_HEAD_LEN),
                    (p - MEMORY_CHK_HEAD_LEN - sizeof(mem_alloc_t)));
            mem_assert_err();
        }
        else {
            //ptr = (p - MEMORY_CHK_HEAD_LEN - sizeof(mem_alloc_t));
        }

        p_mem = (mem_alloc_t *)ptr;
        GLOBAL_INT_DISABLE();
        if (!init_done) {
            INIT_DLIST_HEAD(&heap_mem_list);
            init_done = true;
        }
        list_del(&p_mem->list);
        GLOBAL_INT_RESTORE();
        sys_memset(p_mem, 0, sizeof(mem_alloc_t));
    }else {
        co_printf("!!!!free 0!!!!!\r\n");
    }
    vPortFree(ptr);
}
#endif
