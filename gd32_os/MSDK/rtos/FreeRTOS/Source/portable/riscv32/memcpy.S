/*!
    \file    memcpy.S
    \brief   Memory copy asm function for GD32VW55x SDK.

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

#define __ASM_STR(x)    x

#if __riscv_xlen == 64
#define __REG_SEL(a, b) __ASM_STR(a)
#elif __riscv_xlen == 32
#define __REG_SEL(a, b) __ASM_STR(b)
#else
#error "Unexpected __riscv_xlen"
#endif

#define REG_L   __REG_SEL(ld, lw)
#define REG_S   __REG_SEL(sd, sw)
#define SZREG   __REG_SEL(8, 4)


/* void *Mem_Copy(void *, const void *, size_t) */
Mem_Copy:
    move t6, a0  /* Preserve return value */

    /* Defer to byte-oriented copy for small sizes */
    sltiu a3, a2, 128
    bnez a3, 4f
    /* Use word-oriented copy only if low-order bits match */
    andi a3, t6, SZREG-1
    andi a4, a1, SZREG-1
    bne a3, a4, 4f

    beqz a3, 2f  /* Skip if already aligned */
    /*
     * Round to nearest double word-aligned address
     * greater than or equal to start address
     */
    andi a3, a1, ~(SZREG-1)
    addi a3, a3, SZREG
    /* Handle initial misalignment */
    sub a4, a3, a1
1:
    lb a5, 0(a1)
    addi a1, a1, 1
    sb a5, 0(t6)
    addi t6, t6, 1
    bltu a1, a3, 1b
    sub a2, a2, a4  /* Update count */

2:
    andi a4, a2, ~((16*SZREG)-1)
    beqz a4, 4f
    add a3, a1, a4
3:
    REG_L a4,       0(a1)
    REG_L a5,   SZREG(a1)
    REG_L a6, 2*SZREG(a1)
    REG_L a7, 3*SZREG(a1)
    REG_L t0, 4*SZREG(a1)
    REG_L t1, 5*SZREG(a1)
    REG_L t2, 6*SZREG(a1)
    REG_L t3, 7*SZREG(a1)
    REG_L t4, 8*SZREG(a1)
    REG_L t5, 9*SZREG(a1)
    REG_S a4,       0(t6)
    REG_S a5,   SZREG(t6)
    REG_S a6, 2*SZREG(t6)
    REG_S a7, 3*SZREG(t6)
    REG_S t0, 4*SZREG(t6)
    REG_S t1, 5*SZREG(t6)
    REG_S t2, 6*SZREG(t6)
    REG_S t3, 7*SZREG(t6)
    REG_S t4, 8*SZREG(t6)
    REG_S t5, 9*SZREG(t6)
    REG_L a4, 10*SZREG(a1)
    REG_L a5, 11*SZREG(a1)
    REG_L a6, 12*SZREG(a1)
    REG_L a7, 13*SZREG(a1)
    REG_L t0, 14*SZREG(a1)
    REG_L t1, 15*SZREG(a1)
    addi a1, a1, 16*SZREG
    REG_S a4, 10*SZREG(t6)
    REG_S a5, 11*SZREG(t6)
    REG_S a6, 12*SZREG(t6)
    REG_S a7, 13*SZREG(t6)
    REG_S t0, 14*SZREG(t6)
    REG_S t1, 15*SZREG(t6)
    addi t6, t6, 16*SZREG
    bltu a1, a3, 3b
    andi a2, a2, (16*SZREG)-1  /* Update count */

4:
    /* Handle trailing misalignment */
    beqz a2, 6f
    add a3, a1, a2

    /* Use word-oriented copy if co-aligned to word boundary */
    or a5, a1, t6
    or a5, a5, a3
    andi a5, a5, 3
    bnez a5, 5f
7:
    lw a4, 0(a1)
    addi a1, a1, 4
    sw a4, 0(t6)
    addi t6, t6, 4
    bltu a1, a3, 7b

    ret

5:
    lb a4, 0(a1)
    addi a1, a1, 1
    sb a4, 0(t6)
    addi t6, t6, 1
    bltu a1, a3, 5b
6:
    ret
