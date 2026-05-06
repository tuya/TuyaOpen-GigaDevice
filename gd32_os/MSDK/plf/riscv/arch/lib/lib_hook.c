/*!
    \file    lib_hook.c
    \brief   lib hook functions for GD32VW55x SDK.

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

#include <stddef.h>
#include <stdio.h>
#include "app_cfg.h"
#include "wrapper_os.h"

__attribute__ ((used)) void *_sbrk(ptrdiff_t incr)
{
    extern char _end[];
    extern char _heap_end[];
    static char *curbrk = _end;

    if ((curbrk + incr < _end) || (curbrk + incr > _heap_end)) {
        printf("error: alloc or free heap memory out of bounds\r\n");
        return NULL - 1;
    }

    curbrk += incr;
    return curbrk - incr;
}

__attribute__ ((used)) int _open(int fd)
{
    return -1;
}

__attribute__ ((used)) int _close(int fd)
{
    return -1;
}

__attribute__ ((used)) int _fstat(int fd)
{
    return -1;
}

__attribute__ ((used)) void _exit(int code)
{
    printf("\nProgram has exited with code:%d\r\n",code);
    for (;;);
}

__attribute__ ((used)) int _isatty(int fd)
{
    return 0;
}

__attribute__ ((used)) off_t _lseek(int fd, off_t ptr, int dir)
{
    return -1;
}

__attribute__ ((used)) ssize_t _read(int fd, void* ptr, size_t len)
{
    return -1;
}

__attribute__ ((used)) int _kill(int fd)
{
    return -1;
}

__attribute__ ((used)) int _getpid(int fd)
{
    return -1;
}

/* __malloc_lock and __malloc_unlock functions ensure the safety of heap under multi-tasking during memory distribution */
__attribute__ ((used)) void __malloc_lock(void)
{
    sys_sched_lock();
}

__attribute__ ((used)) void __malloc_unlock(void)
{
    sys_sched_unlock();
}

#ifdef TUYAOS_SUPPORT
__attribute__ ((used)) void *malloc(size_t size)
{
    return sys_malloc(size);
}

__attribute__ ((used)) void *malloc_r(void *p, size_t size)
{

    return sys_malloc(size);
}

__attribute__ ((used)) void free(void *pv)
{
    sys_mfree(pv);
}

__attribute__ ((used)) void *calloc(size_t a, size_t b)
{
    return sys_calloc(a, b);
}

__attribute__ ((used)) void *realloc(void* pv, size_t size)
{
    return sys_realloc(pv, size);
}

__attribute__ ((used)) void free_r(void *p, void *x)
{
   sys_mfree(x);
}

__attribute__ ((used)) void *realloc_r(void *p, void* x, size_t sz)
{
  return sys_realloc(x, sz);
}
#endif
