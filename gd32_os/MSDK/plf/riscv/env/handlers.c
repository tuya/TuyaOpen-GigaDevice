/*!
    \file    handlers.c
    \brief   Interrupt and exception and NMI handling for GD32VW55x SDK.

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

#include "gd32vw55x.h"

#ifndef EXEC_USING_STD_PRINTF
#include "log_uart.h"
#include "rom_export.h"
#include "raw_flash_api.h"
#include "wrapper_os.h"
#define EXC_LOG_TO_FLASH    0
#endif

#define UART_CMD_IN_EXC     1

extern uint32_t _sp[];

typedef struct EXC_Frame {
#ifdef __riscv_flen
    unsigned long f0;
    unsigned long f1;
    unsigned long f2;
    unsigned long f3;
    unsigned long f4;
    unsigned long f5;
    unsigned long f6;
    unsigned long f7;
    unsigned long f10;
    unsigned long f11;
    unsigned long f12;
    unsigned long f13;
    unsigned long f14;
    unsigned long f15;
    unsigned long f16;
    unsigned long f17;
    unsigned long f28;
    unsigned long f29;
    unsigned long f30;
    unsigned long f31;
    unsigned long rcv;               /* pendding for 8-byte alignment */
    unsigned long fcsr;
#endif
    unsigned long ra;                /* ra: x1, return address for jump */
    unsigned long tp;                /* tp: x4, thread pointer */
    unsigned long t0;                /* t0: x5, temporary register 0 */
    unsigned long t1;                /* t1: x6, temporary register 1 */
    unsigned long t2;                /* t2: x7, temporary register 2 */
    unsigned long a0;                /* a0: x10, return value or function argument 0 */
    unsigned long a1;                /* a1: x11, return value or function argument 1 */
    unsigned long a2;                /* a2: x12, function argument 2 */
    unsigned long a3;                /* a3: x13, function argument 3 */
    unsigned long a4;                /* a4: x14, function argument 4 */
    unsigned long a5;                /* a5: x15, function argument 5 */
    unsigned long mcause;            /* mcause: machine cause csr register */
    unsigned long mepc;              /* mepc: machine exception program counter csr register */
    unsigned long msubm;             /* msubm: machine sub-mode csr register, nuclei customized */
#ifndef __riscv_32e
    unsigned long a6;                /* a6: x16, function argument 6 */
    unsigned long a7;                /* a7: x17, function argument 7 */
    unsigned long t3;                /* t3: x28, temporary register 3 */
    unsigned long t4;                /* t4: x29, temporary register 4 */
    unsigned long t5;                /* t5: x30, temporary register 5 */
    unsigned long t6;                /* t6: x31, temporary register 6 */
#endif
} EXC_Frame_Type;

/**
 * \defgroup  NMSIS_Core_IntExcNMI_Handling   Interrupt and Exception and NMI Handling
 * \brief Functions for interrupt, exception and nmi handle available in system_<device>.c.
 * \details
 * Nuclei provide a template for interrupt, exception and NMI handling. Silicon Vendor could adapat according
 * to their requirement. Silicon vendor could implement interface for different exception code and
 * replace current implementation.
 *
 * @{
 */
/** \brief Max exception handler number, don't include the NMI(0xFFF) one */
#define MAX_SYSTEM_EXCEPTION_NUM        12
/**
 * \brief      Store the exception handlers for each exception ID
 * \note
 * - This SystemExceptionHandlers are used to store all the handlers for all
 * the exception codes Nuclei N/NX core provided.
 * - Exception code 0 - 11, totally 12 exceptions are mapped to SystemExceptionHandlers[0:11]
 * - Exception for NMI is also re-routed to exception handling(exception code 0xFFF) in startup code configuration, the handler itself is mapped to SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM]
 */
static unsigned long SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM + 1];

/**
 * \brief      Exception Handler Function Typedef
 * \note
 * This typedef is only used internal in this system_<Device>.c file.
 * It is used to do type conversion for registered exception handler before calling it.
 */
typedef void (*EXC_HANDLER)(unsigned long mcause, unsigned long sp);

void Exception_DumpFrame(unsigned long sp);

#ifndef EXEC_USING_STD_PRINTF
static int8_t __hex_to_val(uint8_t c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    } else if(c >= 'a' && c <= 'f') {
        return (c - 'a') + 10;
    } else if(c >= 'A' && c <= 'F') {
        return (c - 'A') + 10;
    } else {
        return -1;
    }
}

static void __print32(uint32_t val)
{
    int8_t i;
    uint8_t c;

    for (i = 7; i >= 0; i --) {
        c = (val >> (i << 2)) & 0xf;
        if (c >= 10)
            c += 'a' - 10;
        else
            c += '0';
        log_uart_putc_noint(c);
    }
}

static void __print_str(const char* s)
{
    char *str = (char *)s;

    while(*str) {
        log_uart_putc_noint(*str);
        str++;
    }
}

void print_reg(const char *title, uint32_t val)
{
    __print_str(title);
    __print_str(":      0x");
    __print32(val);
    __print_str("\r\n");
}

void dumpStackInfo(unsigned long sp, int32_t num)
{
    uint32_t i = 0;
    int8_t j = 0;
    char c;

    while(num > 0)
    {
        uint8_t val = *(uint8_t *)sp;
        for (j = 4; j >= 0; j -= 4) {
            c = ((val >> j ) & 0xF);
            if (c >= 10)
                c += 'a' - 10;
            else
                c += '0';
            log_uart_putc_noint(c);
        }


        i++;
        sp++;
        if ((i % 16) == 0) {
            __print_str("\r\n");
        }
        else {
            log_uart_putc_noint(',');
            log_uart_putc_noint(' ');
        }

        num--;
    }
}

void xCurrentTaskDumpInfo(unsigned long sp)
{
    int8_t j = 0;
    char *pcTaskName = sys_task_name_get(NULL);
    int32_t num = sys_current_task_stack_depth(sp);
#if (configRECORD_STACK_HIGH_ADDRESS == 0)
    __print_str("set configRECORD_STACK_HIGH_ADDRESS=1 to get precise data\r\n");
#endif
    __print_str((const char *)pcTaskName);

    __print_str(" Stack Dump:\r\n");

    dumpStackInfo(sp, num);
}


static bool parseUartCmd(char *p_buf, uint8_t buf_len, unsigned long sp) {
    uint8_t idx = 0;
    uint8_t state = 0;
    uint32_t start_addr = 0;
    uint32_t len = 0;
    int32_t i;
    uint32_t val;
    int8_t val8;
    char cmdr[] = {'r', 'm', 'e', 'm', ' '};
    char cmdd[] = {'d', 'u', 'm', 'p'};
    char cmdh[] = {'h', 'e', 'l', 'p'};

    while(idx < buf_len) {
        switch (state) {
        case 0:
            if (p_buf[idx] == 'r' && (buf_len - idx) > 5) {
                for (i = 0; i < 5; i++) {
                    if (p_buf[idx] != cmdr[i]) {
                        return false;
                    }
                    idx++;
                }

                state = 1;
                continue;
            }
            else if (p_buf[idx] == 'd' && (buf_len - idx) >= 4) {
                for (i = 0; i < 4; i++) {
                    if (p_buf[idx] != cmdd[i]) {
                        return false;
                    }
                    idx++;
                }
                Exception_DumpFrame(sp);
                // Trap from task
                if ((MSUBM_PTYP & __RV_CSR_READ(CSR_MSUBM)) == 0) {
                    print_reg("SP     ", sp);
                    xCurrentTaskDumpInfo(sp);
                }
                // Trap from interrupt
                else if ((MSUBM_PTYP & __RV_CSR_READ(CSR_MSUBM)) == 0x0100) {
                    unsigned long int_sp = __RV_CSR_READ(CSR_MSCRATCH);
                    print_reg("SP     ", int_sp);
#ifndef __riscv_32e
                    int32_t num = 80;
#else
                    int32_t num = 64;
#endif

#ifdef __riscv_flen
                    num += 80;
#endif
                    __print_str("Interrupt Stack Dump:\r\n");
                    dumpStackInfo(sp, num);
                    num = (uint32_t)_sp - int_sp;
                    dumpStackInfo(int_sp, num);
                }

                __print_str("Please use bloodhound tool to parse log above!\r\n");
                return true;
            }
            else if (p_buf[idx] == 'h' && (buf_len - idx) >= 4) {
                for (i = 0; i < 4; i++) {
                    if (p_buf[idx] != cmdh[i]) {
                        return false;
                    }
                    idx++;
                }
                __print_str("rmem ADDR(Hex 4bytes align) Len(Hex length of 4 bytes)\r\n");
                __print_str("dump\r\n");

                return true;
            }
            break;
        case 1:
            if (p_buf[idx] == ' ') {
                state = 2;
                break;
            }

            val8 = __hex_to_val(p_buf[idx]);

            if (val8 < 0)
                return false;
            else
                start_addr = (start_addr << 4) + val8;

            break;
        case 2:
            if (p_buf[idx] == ' ') {
                break;
            }

            val8 = __hex_to_val(p_buf[idx]);
            if (val8 < 0)
                return false;
            else
                len = (len << 4) + val8;

        default:
            break;
        }
        idx++;
    }

    // we need to check read addr is ok
    i = 0;
    do {
        val = *(uint32_t *)start_addr;

        if ((i % 4) == 0) {
            __print_str("\r\n");
            __print32(start_addr);
            log_uart_putc_noint(':');
        }

        log_uart_putc_noint(' ');
        __print32(val);

        start_addr = start_addr + 4;
    } while(++i < len);

    return true;
}
#endif

#if EXC_LOG_TO_FLASH
#define WR_REGION_SPAGE    ((uint32_t)0x003FE000U)       /*!< start page of write protection region: page1022 */
#define WR_REGION_EPAGE    ((uint32_t)0x003FF000U)       /*!< end page of write protection region: page1023 */

static void dump_info_2_flash(unsigned long sp)
{
    char prefix[3];
    uint32_t reg_val = 0;
    uint32_t offset = WR_REGION_SPAGE;

    if (raw_flash_erase(WR_REGION_SPAGE, FLASH_PAGE_SIZE))
        return;

    prefix[0] = 20;
    prefix[1] = 0;
    prefix[2] = 0x01;   //register value
    raw_flash_write(offset, (void *)&prefix, 3);
    offset += 3;

    reg_val = __RV_CSR_READ(CSR_MCAUSE);
    raw_flash_write(offset, (void *)&reg_val, 4);
    offset += 4;

    reg_val = __RV_CSR_READ(CSR_MDCAUSE);
    raw_flash_write(offset, (void *)&reg_val, 4);
    offset += 4;

    reg_val = __RV_CSR_READ(CSR_MEPC);
    raw_flash_write(offset, (void *)&reg_val, 4);
    offset += 4;

    reg_val = __RV_CSR_READ(CSR_MTVAL);
    raw_flash_write(offset, (void *)&reg_val, 4);
    offset += 4;

    reg_val = __RV_CSR_READ(CSR_MSUBM);
    raw_flash_write(offset, (void *)&reg_val, 4);
    offset += 4;

    prefix[2] = 0x02;   //stack type
    // Trap from task
    if ((MSUBM_PTYP & __RV_CSR_READ(CSR_MSUBM)) == 0) {
        int32_t num = sys_current_task_stack_depth(sp);
        if ((offset + num + 6) > WR_REGION_EPAGE) {
            num = WR_REGION_EPAGE - offset - 6;
        }
        prefix[0] = num & 0xFF;
        prefix[1] = (num >> 8) & 0xFF;
        raw_flash_write(offset, (void *)prefix, 3);
        offset += 3;
        raw_flash_write(offset, (void *)sp, num);
        offset += num;
    }
    // Trap from interrupt
    else if ((MSUBM_PTYP & __RV_CSR_READ(CSR_MSUBM)) == 0x0100) {
          unsigned long int_sp = __RV_CSR_READ(CSR_MSCRATCH);
          int32_t sk_num = (uint32_t)_sp - int_sp;
#ifndef __riscv_32e
        int32_t num = 80;
#else
        int32_t num = 64;
#endif

#ifdef __riscv_flen
        num += 80;
#endif

        if ((offset + num + sk_num + 6) > WR_REGION_EPAGE) {
            sk_num = WR_REGION_EPAGE - offset - 6 - num;
        }

        num += sk_num;
        prefix[0] = num & 0xFF;
        prefix[1] = (num >> 8) & 0xFF;
        raw_flash_write(offset, (void *)prefix, 3);
        offset += 3;

        num -= sk_num;
        raw_flash_write(offset, (void *)sp, num);
        offset += num;

        num = sk_num;
        raw_flash_write(offset, (void *)int_sp, num);
        offset += num;
    }

    prefix[0] = 0;
    prefix[1] = 0;
    raw_flash_write(offset, (void *)&prefix, 3);
}
#endif

/**
 * \brief      System Default Exception Handler
 * \details
 * This function provided a default exception and NMI handling code for all exception ids.
 * By default, It will just print some information for debug, Vendor can customize it according to its requirements.
 */
static void system_default_exception_handler(unsigned long mcause, unsigned long sp)
{
    /* TODO: Uncomment this if you have implement printf function */
#ifdef EXEC_USING_STD_PRINTF
    printf("MCAUSE : 0x%lx\r\n", mcause);
    printf("MDCAUSE: 0x%lx\r\n", __RV_CSR_READ(CSR_MDCAUSE));
    printf("MEPC   : 0x%lx\r\n", __RV_CSR_READ(CSR_MEPC));
    printf("MTVAL  : 0x%lx\r\n", __RV_CSR_READ(CSR_MTVAL));
#else
    char ch;
    char uart_buf[64];
    uint8_t uart_index = 0;

    __print_str("System Default Exception \r\n");
    print_reg("MCAUSE ", mcause);
    print_reg("MDCAUSE", __RV_CSR_READ(CSR_MDCAUSE));
    print_reg("MEPC   ", __RV_CSR_READ(CSR_MEPC));
    print_reg("MTVAL  ", __RV_CSR_READ(CSR_MTVAL));
    print_reg("MSUBM  ", __RV_CSR_READ(CSR_MSUBM));
#endif
    Exception_DumpFrame(sp);

#ifndef EXEC_USING_STD_PRINTF
    // Trap from task
    if ((MSUBM_PTYP & __RV_CSR_READ(CSR_MSUBM)) == 0) {
        print_reg("SP     ", sp);
        xCurrentTaskDumpInfo(sp);
    }
    // Trap from interrupt
    else if ((MSUBM_PTYP & __RV_CSR_READ(CSR_MSUBM)) == 0x0100) {
        unsigned long int_sp = __RV_CSR_READ(CSR_MSCRATCH);
        print_reg("SP     ", int_sp);
#ifndef __riscv_32e
        int32_t num = 80;
#else
        int32_t num = 64;
#endif

#ifdef __riscv_flen
        num += 84;
#endif
        __print_str("Interrupt Stack Dump:\r\n");
        dumpStackInfo(sp, num);
        num = (uint32_t)_sp - int_sp;
        dumpStackInfo(int_sp, num);
    }
    __print_str("Please use bloodhound tool to parse log above!\r\n");

#if EXC_LOG_TO_FLASH
    dump_info_2_flash(sp);
#endif

#if (UART_CMD_IN_EXC == 1)
    uart_config(LOG_UART, DEFAULT_LOG_BAUDRATE, false, false, false);
    __print_str("\r\n");
    __print_str("print help to get cmd\r\n");
    while (1) {
        // We should have chance to check overflow error
        // Otherwise it may cause dead loop handle rx interrupt
        if (RESET != usart_flag_get(LOG_UART, USART_FLAG_ORERR)) {
            usart_flag_clear(LOG_UART, USART_FLAG_ORERR);
        }

        if ((RESET != usart_flag_get(LOG_UART, USART_FLAG_RBNE))) {
            ch = (char)usart_data_receive(LOG_UART);
        }
        else {
            continue;
        }

        if (ch == '\r' || ch == '\n') { /* putty doesn't transmit '\n' */
            __print_str("\r\n");
            if (uart_index > 0) {
                parseUartCmd(uart_buf, uart_index, sp);
            }
            log_uart_putc_noint('#');
            log_uart_putc_noint(' ');
            uart_index = 0;
        } else if (ch == '\b') { /* non-destructive backspace */
            if (uart_index > 0) {
                uart_buf[--uart_index] = '\0';
            }
        }
        else {
            uart_buf[uart_index++] = ch;
            if (uart_index >= 64) {
                uart_index = 0;
            }
            log_uart_putc_noint(ch);
        }
    }
#endif
#endif
    while (1);
}

/**
 * \brief      NMI Handler
 * \details
 * This function provided a NMI handling code for NMI_EXCn.
 * By default, It will just print some information for debug, Vendor can customize it according to its requirements.
 */
void nmi_handler(unsigned long mcause, unsigned long sp)
{
    /* TODO: Uncomment this if you have implement printf function */
#ifdef EXEC_USING_STD_PRINTF
    printf("NMI \r\n");
    printf("MCAUSE : 0x%lx\r\n", mcause);
    printf("MDCAUSE: 0x%lx\r\n", __RV_CSR_READ(CSR_MDCAUSE));
    printf("MEPC   : 0x%lx\r\n", __RV_CSR_READ(CSR_MEPC));
    printf("MTVAL  : 0x%lx\r\n", __RV_CSR_READ(CSR_MTVAL));
#else
    __print_str("NMI \r\n");
    print_reg("MCAUSE ", mcause);
    print_reg("MDCAUSE", __RV_CSR_READ(CSR_MDCAUSE));
    print_reg("MEPC   ", __RV_CSR_READ(CSR_MEPC));
    print_reg("MTVAL  ", __RV_CSR_READ(CSR_MTVAL));
    print_reg("MSUBM  ", __RV_CSR_READ(CSR_MSUBM));
#endif
    Exception_DumpFrame(sp);

    while (1);
}

/**
 * \brief      Initialize all the default core exception handlers
 * \details
 * The core exception handler for each exception id will be initialized to \ref system_default_exception_handler.
 * \note
 * Called in \ref _init function, used to initialize default exception handlers for all exception IDs
 */
void Exception_Init(void)
{
    for (int i = 0; i < MAX_SYSTEM_EXCEPTION_NUM + 1; i++) {
        SystemExceptionHandlers[i] = (unsigned long)system_default_exception_handler;
    }
}

/**
 * \brief       Register an exception handler for exception code EXCn
 * \details
 * * For EXCn < \ref MAX_SYSTEM_EXCEPTION_NUM, it will be registered into SystemExceptionHandlers[EXCn-1].
 * * For EXCn == NMI_EXCn, it will be registered into SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM].
 * \param   EXCn    See \ref EXCn_Type
 * \param   exc_handler     The exception handler for this exception code EXCn
 */
void Exception_Register_EXC(uint32_t EXCn, unsigned long exc_handler)
{
    if ((EXCn < MAX_SYSTEM_EXCEPTION_NUM) && (EXCn >= 0)) {
        SystemExceptionHandlers[EXCn] = exc_handler;
    } else if (EXCn == NMI_EXCn) {
        SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM] = exc_handler;
    }
}

/**
 * \brief       Get current exception handler for exception code EXCn
 * \details
 * * For EXCn < \ref MAX_SYSTEM_EXCEPTION_NUM, it will return SystemExceptionHandlers[EXCn-1].
 * * For EXCn == NMI_EXCn, it will return SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM].
 * \param   EXCn    See \ref EXCn_Type
 * \return  Current exception handler for exception code EXCn, if not found, return 0.
 */
unsigned long Exception_Get_EXC(uint32_t EXCn)
{
    if ((EXCn < MAX_SYSTEM_EXCEPTION_NUM) && (EXCn >= 0)) {
        return SystemExceptionHandlers[EXCn];
    } else if (EXCn == NMI_EXCn) {
        return SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM];
    } else {
        return 0;
    }
}

/**
 * \brief      Common NMI and Exception handler entry
 * \details
 * This function provided a command entry for NMI and exception. Silicon Vendor could modify
 * this template implementation according to requirement.
 * \remarks
 * - RISCV provided common entry for all types of exception. This is proposed code template
 *   for exception entry function, Silicon Vendor could modify the implementation.
 * - For the core_exception_handler template, we provided exception register function \ref Exception_Register_EXC
 *   which can help developer to register your exception handler for specific exception number.
 */
uint32_t core_exception_handler(unsigned long mcause, unsigned long sp)
{
    uint32_t EXCn = (uint32_t)(mcause & 0X00000fff);
    EXC_HANDLER exc_handler;

    if ((EXCn < MAX_SYSTEM_EXCEPTION_NUM) && (EXCn >= 0)) {
        exc_handler = (EXC_HANDLER)SystemExceptionHandlers[EXCn];
    } else if (EXCn == NMI_EXCn) {
        exc_handler = (EXC_HANDLER)SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM];
    } else {
        exc_handler = (EXC_HANDLER)system_default_exception_handler;
    }
    if (exc_handler != NULL) {
        exc_handler(mcause, sp);
    }
    return 0;
}
/** @} */ /* End of Doxygen Group NMSIS_Core_ExceptionAndNMI */

/**
 * \brief      Dump Exception Frame
 * \details
 * This function provided feature to dump exception frame stored in stack.
 */
void Exception_DumpFrame(unsigned long sp)
{
    EXC_Frame_Type *exc_frame = (EXC_Frame_Type *)sp;
#ifndef __riscv_32e
    /* TODO: Uncomment this if you have implement printf function */
#ifdef EXEC_USING_STD_PRINTF
    printf("ra: 0x%x, tp: 0x%x, t0: 0x%x, t1: 0x%x, t2: 0x%x, t3: 0x%x, t4: 0x%x, t5: 0x%x, t6: 0x%x\n" \
           "a0: 0x%x, a1: 0x%x, a2: 0x%x, a3: 0x%x, a4: 0x%x, a5: 0x%x, a6: 0x%x, a7: 0x%x\n" \
           "mcause: 0x%x, mepc: 0x%x, msubm: 0x%x\n", exc_frame->ra, exc_frame->tp, exc_frame->t0, \
           exc_frame->t1, exc_frame->t2, exc_frame->t3, exc_frame->t4, exc_frame->t5, exc_frame->t6, \
           exc_frame->a0, exc_frame->a1, exc_frame->a2, exc_frame->a3, exc_frame->a4, exc_frame->a5, \
           exc_frame->a6, exc_frame->a7, exc_frame->mcause, exc_frame->mepc, exc_frame->msubm);
#else
    print_reg("ra     ", exc_frame->ra);
    print_reg("tp     ", exc_frame->tp);
    print_reg("t0     ", exc_frame->t0);
    print_reg("t1     ", exc_frame->t1);
    print_reg("t2     ", exc_frame->t2);
    print_reg("t3     ", exc_frame->t3);
    print_reg("t4     ", exc_frame->t4);
    print_reg("t5     ", exc_frame->t5);
    print_reg("t6     ", exc_frame->t6);

    print_reg("a0     ", exc_frame->a0);
    print_reg("a1     ", exc_frame->a1);
    print_reg("a2     ", exc_frame->a2);
    print_reg("a3     ", exc_frame->a3);
    print_reg("a4     ", exc_frame->a4);
    print_reg("a5     ", exc_frame->a5);
    print_reg("a6     ", exc_frame->a6);
    print_reg("mcause ", exc_frame->mcause);
    print_reg("mepc   ", exc_frame->mepc);
    print_reg("msubm  ", exc_frame->msubm);
#endif
#else /* __riscv_32e */
    /* TODO: Uncomment this if you have implement printf function */
    printf("ra: 0x%x, tp: 0x%x, t0: 0x%x, t1: 0x%x, t2: 0x%x\n" \
           "a0: 0x%x, a1: 0x%x, a2: 0x%x, a3: 0x%x, a4: 0x%x, a5: 0x%x\n" \
           "mcause: 0x%x, mepc: 0x%x, msubm: 0x%x\n", exc_frame->ra, exc_frame->tp, exc_frame->t0, \
           exc_frame->t1, exc_frame->t2, exc_frame->a0, exc_frame->a1, exc_frame->a2, exc_frame->a3, \
           exc_frame->a4, exc_frame->a5, exc_frame->mcause, exc_frame->mepc, exc_frame->msubm);
#endif /* __riscv_32e */
}

#if 0
/**
 * \brief  Initialize a specific IRQ and register the handler
 * \details
 * This function set vector mode, trigger mode and polarity, interrupt level and priority,
 * assign handler for specific IRQn.
 * \param [in]  IRQn        NMI interrupt handler address
 * \param [in]  shv         \ref ECLIC_NON_VECTOR_INTERRUPT means non-vector mode, and \ref ECLIC_VECTOR_INTERRUPT is vector mode
 * \param [in]  trig_mode   see \ref ECLIC_TRIGGER_Type
 * \param [in]  lvl         interupt level
 * \param [in]  priority    interrupt priority
 * \param [in]  handler     interrupt handler, if NULL, handler will not be installed
 * \return       -1 means invalid input parameter. 0 means successful.
 * \remarks
 * - This function use to configure specific eclic interrupt and register its interrupt handler and enable its interrupt.
 * - If the vector table is placed in read-only section(FLASHXIP mode), handler could not be installed
 */
int32_t ECLIC_Register_IRQ(IRQn_Type IRQn, uint8_t shv, ECLIC_TRIGGER_Type trig_mode, uint8_t lvl, uint8_t priority, void* handler)
{
    if ((IRQn > SOC_INT_MAX) || (shv > ECLIC_VECTOR_INTERRUPT) \
        || (trig_mode > ECLIC_NEGTIVE_EDGE_TRIGGER)) {
        return -1;
    }

    /* set interrupt vector mode */
    ECLIC_SetShvIRQ(IRQn, shv);
    /* set interrupt trigger mode and polarity */
    ECLIC_SetTrigIRQ(IRQn, trig_mode);
    /* set interrupt level */
    ECLIC_SetLevelIRQ(IRQn, lvl);
    /* set interrupt priority */
    ECLIC_SetPriorityIRQ(IRQn, priority);
    if (handler != NULL) {
        /* set interrupt handler entry to vector table */
        ECLIC_SetVector(IRQn, (rv_csr_t)handler);
    }
    /* enable interrupt */
    ECLIC_EnableIRQ(IRQn);
    return 0;
}
#endif
