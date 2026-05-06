/*!
    \file    debug_print.c
    \brief   debug print for GD32VW55x SDK.

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

#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include "debug_print.h"
#include "wrapper_os.h"
#include "trace_ext.h"
#include "util.h"
#include "log_uart.h"
#include "plf_assert.h"

// whether short type is supported
#define FLAG_SHORT_SUPPORTED

//#define CONFIG_PRINT_IN_SEQUENCE

// States values
enum
{
    S_COPY,         // Initial state; copy chars of the format str
    S_PERCENT,      // just read '%'
    S_FLAGS,        // just read flag character
    S_WIDTH,        // just read width specifier
    S_DOT,          // just read '.'
    S_PRECIS,       // just read field_precision specifier
    S_SIZE,         // just read size specifier
    S_TYPE,         // just read type specifier
    S_MAX,
};

// character type values
enum
{
    C_OTHER,        // character with no special meaning
    C_PERCENT,      // '%'
    C_DOT,          // '.'
    C_ZERO,         // '0'
    C_DIGIT,        // '1'..'9'
    C_FLAG,         // ' ', '+', '-',
    C_SIZE,         // 'h', 'l', 'L'
    C_TYPE,         // type specifying character
    C_MAX,
};

// field_flags used to store the format information
enum
{
    FLAG_SHORT      = (1 << 0),   // short value
    FLAG_LONG       = (1 << 1),   // long value
    FLAG_SIGNED     = (1 << 2),   // signed value
    FLAG_SIGN       = (1 << 3),   // Add a - or + in the front of the field
    FLAG_SIGN_SPACE = (1 << 4),   // Add a space or - in the front of the field
    FLAG_LEFT       = (1 << 5),   // left justify
    FLAG_LEAD_ZERO  = (1 << 6),   // padding with 0
    FLAG_NEGATIVE   = (1 << 7),   // the value is negative
};

// Transition table
static const uint8_t transition_table[S_MAX][C_MAX] =
{   //OTHER  PERCENT    DOT     ZERO      DIGIT     FLAG     SIZE    TYPE
    {S_COPY, S_PERCENT, S_COPY, S_COPY,   S_COPY,   S_COPY,  S_COPY, S_COPY}, // COPY
    {S_COPY, S_COPY,    S_DOT,  S_FLAGS,  S_WIDTH,  S_FLAGS, S_SIZE, S_TYPE}, // PERCENT
    {S_COPY, S_COPY,    S_DOT,  S_FLAGS,  S_WIDTH,  S_FLAGS, S_SIZE, S_TYPE}, // FLAGS
    {S_COPY, S_COPY,    S_DOT,  S_WIDTH,  S_WIDTH,  S_COPY,  S_SIZE, S_TYPE}, // WIDTH
    {S_COPY, S_COPY,    S_COPY, S_PRECIS, S_PRECIS, S_COPY,  S_SIZE, S_TYPE}, // DOT
    {S_COPY, S_COPY,    S_COPY, S_PRECIS, S_PRECIS, S_COPY,  S_SIZE, S_TYPE}, // PRECIS
    {S_COPY, S_COPY,    S_COPY, S_COPY,   S_COPY,   S_COPY,  S_COPY, S_TYPE}, // SIZE
    {S_COPY, S_PERCENT, S_COPY, S_COPY,   S_COPY,   S_COPY,  S_COPY, S_COPY}, // TYPE
};

// Upper case hexadecimal table
static const char hex_upper_table[] = "0123456789ABCDEF";
#ifdef SUPPORT_LOWER_CASE_HEX
// Lower case hexadecimal table
static const char hex_lower_table[] = "0123456789abcdef";
#endif


#ifdef CONFIG_PRINT_IN_SEQUENCE
#define MAX_BUF_LEN            8192
static char print_buf[MAX_BUF_LEN];
static os_sema_t print_sema;
static int print_task_init = 0;
static int w_point = 0, r_point = 0, used_len = 0;
#endif

static void printchar(char **str, int c, int *space)
{
    if (!str) {
        log_uart_putc_noint((char)c);
    } else if ((uint32_t)str < USART_BASE) {
        if (*space > 0) {
            **str = c;
            ++(*str);
            *space -= 1;
        }
    } else {
        uint32_t uartx = (uint32_t)str;
        if ((uartx == USART0) || (uartx == UART1) || (uartx == UART2))
            uart_putc_noint(uartx, (char)c);
    }
}

#define PAD_WITH_RIGHT 1
#define PAD_WITH_ZERO 2

static int prints(char **out, const char *string, int wdt, int pad_data_format, int *space)
{
    register int pc = 0, padchar = ' ';

    if (wdt > 0) {
        register int len = 0;
        register const char *ptr;

        for (ptr = string; *ptr; ++ptr) {
            ++len;
        }

        if (len >= wdt) {
            wdt = 0;
        }
        else {
            wdt -= len;
        }

        if (pad_data_format & PAD_WITH_ZERO) {
            padchar = '0';
        }
    }

    if (!(pad_data_format & PAD_WITH_RIGHT)) {
        for ( ; wdt > 0; --wdt) {
            printchar (out, padchar, space);
            ++pc;
        }
    }

    for ( ; *string ; ++string) {
        printchar (out, *string, space);
        ++pc;
    }

    for ( ; wdt > 0; --wdt) {
        printchar (out, padchar, space);
        ++pc;
    }

    return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int wdt, int pad_data_format, int letbase, int *space)
{
    char print_buf[PRINT_BUF_LEN];
    register char *s;
    register int t, neg = 0, pc = 0;
    register unsigned int u = i;

    if (i == 0) {
        print_buf[0] = '0';
        print_buf[1] = '\0';
        return prints (out, print_buf, wdt, pad_data_format, space);
    }

    if (sg && b == 10 && i < 0) {
        neg = 1;
        u = -i;
    }

    s = print_buf + PRINT_BUF_LEN-1;
    *s = '\0';

    while (u) {
        t = u % b;

        if( t >= 10 ) {
            t += letbase - '0' - 10;
        }

        *--s = t + '0';
        u /= b;
    }

    if (neg) {
        if( wdt && (pad_data_format & PAD_WITH_ZERO) ) {
            printchar (out, '-', space);
            ++pc;
            --wdt;
        }
        else {
            *--s = '-';
        }
    }

    return pc + prints (out, s, wdt, pad_data_format, space);
}

int print(char **out, const char *format, va_list args, int space)
{
    register int width, pad_data_format;
    register int pc = 0;
    char scr[2];

    // not enough space
    if (out  && ((uint32_t)out < USART_BASE) && space < 1)
        return 0;

    space -= 1;     // last byte for '\0'

    for (; *format != 0; ++format) {
        if (*format == '%') {
            ++format;
            width = pad_data_format = 0;

            if (*format == '\0') {
                break;
            }

            if (*format == '%') {
                goto out;
            }

            if (*format == '-') {
                ++format;
                pad_data_format = PAD_WITH_RIGHT;
            }

            while (*format == '0') {
                ++format;
                pad_data_format |= PAD_WITH_ZERO;
            }

            for ( ; *format >= '0' && *format <= '9'; ++format) {
                width *= 10;
                width += *format - '0';
            }

            if (*format == 's') {
                register char *s = (char *)va_arg(args, int);
                pc += prints(out, s ? s : "(null)", width, pad_data_format, &space);
                continue;
            }

            if (*format == 'd') {
                pc += printi(out, va_arg(args, int), 10, 1, width, pad_data_format, 'a', &space);
                continue;
            }

            if (*format == 'p') {
                register unsigned char *addr;
                register unsigned char i;
                switch (*(++format)) {
                case 'M':
                    width = 2;
                    pad_data_format |= PAD_WITH_ZERO;
                    addr = (unsigned char *)va_arg(args, int);
                    for (i = 0; i < 5; i++) {
                        pc += printi(out, addr[i], 16, 1, width, pad_data_format, 'a', &space);
                        printchar(out, ':', &space);
                        pc++;
                    }
                    pc += printi(out, addr[i], 16, 1, width, pad_data_format, 'a', &space);
                    continue;
                case 'I':
                    addr = (unsigned char *)va_arg(args, int);
                    for (i = 0; i < 3; i++) {
                        pc += printi(out, addr[i], 10, 1, width, pad_data_format, 'a', &space);
                        printchar(out, '.', &space);
                        pc++;
                    }
                    pc += printi(out, addr[i], 10, 1, width, pad_data_format, 'a', &space);
                    continue;
                default:
                    format--;
                    pc += printi(out, va_arg(args, int), 16, 0, width, pad_data_format, 'a', &space);
                    continue;
                }
            }

            if ((*format == 'x')) {
                pc += printi(out, va_arg(args, int), 16, 0, width, pad_data_format, 'a', &space);
                continue;
            }

            if (*format == 'X') {
                pc += printi(out, va_arg(args, int), 16, 0, width, pad_data_format, 'A', &space);
                continue;
            }

            if (*format == 'z') {
                ++format;
            }

            if (*format == 'u') {
                pc += printi(out, va_arg(args, int), 10, 0, width, pad_data_format, 'a', &space);
                continue;
            }

            if (*format == 'c') {
                /* char are converted to int then pushed on the stack */
                scr[0] = (char)va_arg(args, int);
                scr[1] = '\0';
                pc += prints (out, scr, width, pad_data_format, &space);
                continue;
            }
        } else {
out:
            printchar(out, *format, &space);
            ++pc;
        }
    }

    if (out) {
        **out = '\0';
    }

    return pc;
}

#ifdef CONFIG_PRINT_IN_SEQUENCE
static void print_task_handle(void *argv)
{
    do {
        sys_sema_down(&print_sema, 0);
        while ( used_len > 0) {
            log_uart_putc_noint(print_buf[r_point]);
            r_point ++;
            if (r_point >= MAX_BUF_LEN)
                r_point -= MAX_BUF_LEN;
            sys_enter_critical();
            used_len--;
            sys_exit_critical();
        }
    } while(1);
}
#endif


int co_printf(const char *format, ...)
{
#if (!defined(CONFIG_PRINT_IN_SEQUENCE) | !defined(LOG_UART))
    int ret;
#ifndef LOG_UART
    char out[1024], *pout = &out[0];
#endif
    va_list args;

    va_start(args, format);
#ifndef LOG_UART
    ret = print(&pout, format, args, 1024);
    trace_console(ret, (uint8_t *)out);
#else
    ret = print(0, format, args, 0);
#endif
    va_end(args);

    return ret;
#else
    char out[1024], *pout = &out[0];
    char len = 0;
    va_list args;
    int cur_wp = 0;
    int pc = 0;

    if (print_task_init == 0) {
        sys_enter_critical();
        if (print_task_init == 0 ) {
            sys_sema_init(&print_sema, 0);
            sys_task_create_dynamic((const uint8_t *)"Print", 512,
                 (OS_TASK_PRIO_IDLE + TASK_PRIO_HIGHER(1)), print_task_handle, NULL);
            print_task_init = 1;
        }
        sys_exit_critical();
    }

    va_start(args, format);
    pc = print(&pout, format, args, 1024);
    va_end(args);

    len = strlen(out);
    if (len < (MAX_BUF_LEN - used_len)) {
        /* Remaining print buffer is enough for this print. */
        sys_enter_critical();
        cur_wp = w_point;
        used_len += len;
        w_point += len;
        w_point = (w_point >= MAX_BUF_LEN) ? (w_point - MAX_BUF_LEN) : w_point;
        sys_exit_critical();
        if (cur_wp >= r_point) {
            if ((MAX_BUF_LEN - cur_wp) >= len) {
                sys_memcpy(&print_buf[cur_wp], out, len);
            } else {
                sys_memcpy(&print_buf[cur_wp], out, (MAX_BUF_LEN - cur_wp));
                sys_memcpy(&print_buf[0], &out[MAX_BUF_LEN - cur_wp], (len - (MAX_BUF_LEN - cur_wp)));
            }
        } else {
            sys_memcpy(&print_buf[cur_wp], out, len);
        }
        /* Inform print task to read. */
        sys_sema_up(&print_sema);
    }else {
        /* The print buffer is full. Ignore the new message. */
    }
    return pc;
#endif    // CONFIG_PRINT_IN_SEQUENCE end
}

int co_snprintf(char *out, int space, const char *format, ...)
{
    int ret = 0;
    if (out) {
        va_list args;

        va_start(args, format);
        ret = print(&out, format, args, space);
        va_end(args);
    }

    return ret;
}

#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
int print_buffer(unsigned long addr, void *data, unsigned long width, unsigned long count, unsigned long linelen)
{
#ifdef LOG_UART
    unsigned char linebuf[MAX_LINE_LENGTH_BYTES];
    unsigned long *uip = (void *)linebuf;
    unsigned short *usp = (void *)linebuf;
    unsigned char *ucp = (void *)linebuf;
    unsigned char *pdata = (unsigned char *)data;
    int i;

    if (linelen*width > MAX_LINE_LENGTH_BYTES)
        linelen = MAX_LINE_LENGTH_BYTES / width;
    if (linelen < 1)
        linelen = DEFAULT_LINE_LENGTH_BYTES / width;

      while (count) {
        co_printf("%08x:", addr);

        /* check for overflow condition */
        if (count < linelen)
            linelen = count;

        /* Copy from memory into linebuf and print hex values */
        for (i = 0; i < linelen; i++) {
            if (width == 4) {
                uip[i] = *(volatile unsigned long *)pdata;
                co_printf(" %08x", uip[i]);
            } else if (width == 2) {
                usp[i] = *(volatile unsigned short *)pdata;
                co_printf(" %04x", usp[i]);
            } else {
                ucp[i] = *(volatile unsigned char *)pdata;
                co_printf(" %02x", ucp[i]);
            }
            pdata += width;
        }

#if 0
        /* Print data in ASCII characters */
        prints(NULL, "    ", 0, 0);
        for (i = 0; i < linelen * width; i++)
            printchar(NULL, isprint(ucp[i]) && (ucp[i] < 0x80) ? ucp[i] : '.');
#endif

        printchar(NULL, '\n', 0);

        /* update references */
        addr += linelen * width;
        count -= linelen;
    }
#else
    char out[512], *pout = &out[0];
    unsigned char *pdata = (unsigned char *)data;
    int i, space;
    int ret = 0;

    if ((linelen * width) > MAX_LINE_LENGTH_BYTES)
        linelen = MAX_LINE_LENGTH_BYTES / width;
    if (linelen < 1)
        linelen = DEFAULT_LINE_LENGTH_BYTES / width;


      while (count) {
        pout = &out[0];
        space = 512 - 1;

        ret = co_snprintf(pout, space, "%08x:", addr);
        space -= ret;
        pout += ret;

        /* check for overflow condition */
        if (count < linelen)
            linelen = count;

        /* Copy from memory into linebuf and print hex values */
        for (i = 0; i < linelen; i++) {
            if (width == 4) {
                ret = co_snprintf(pout, space, " %08x", *(((volatile unsigned long *)pdata) + i));
                space -= ret;
                pout += ret;
            } else if (width == 2) {
                ret = co_snprintf(pout, space, " %08x", *(((volatile unsigned short *)pdata) + i));
                space -= ret;
                pout += ret;
            } else {
                ret = co_snprintf(pout, space, " %08x", *(((volatile unsigned char *)pdata) + i));
                space -= ret;
                pout += ret;
            }
            pdata += width;
        }

#if 0
        /* Print data in ASCII characters */
        prints(NULL, "    ", 0, 0);
        for (i = 0; i < linelen * width; i++)
            printchar(NULL, isprint(ucp[i]) && (ucp[i] < 0x80) ? ucp[i] : '.');
#endif
        *pout = '\n';
        space = 512 - space;
        trace_console(space, (uint8_t *)out);

        /* update references */
        addr += linelen * width;
        count -= linelen;
    }
#endif

    return 0;
}

void debug_print_dump_data(char *title, char *mem, int mem_size)
{
    if (title)
        co_printf("=== %s ===\r\n", title);
    if (mem_size == 0) return;
    print_buffer((unsigned long)mem, mem, 1, mem_size, 0);
}

int str2hex(char *input, int input_len, unsigned char *output, int output_len)
{
    int index = 0;
    char iter_char = 0;

    if (input == NULL || input_len <= 0 || input_len % 2 != 0 ||
        output == NULL || output_len < input_len / 2) {
        return -1;
    }

    sys_memset(output, 0, output_len);

    for (index = 0; index < input_len; index += 2) {
        if (input[index] >= '0' && input[index] <= '9') {
            iter_char = input[index] - '0';
        } else if (input[index] >= 'A' && input[index] <= 'F') {
            iter_char = input[index] - 'A' + 0x0A;
        } else if (input[index] >= 'a' && input[index] <= 'f') {
            iter_char = input[index] - 'a' + 0x0A;
        } else {
            return -2;
        }
        output[index / 2] |= (iter_char << 4) & 0xF0;

        if (input[index + 1] >= '0' && input[index + 1] <= '9') {
            iter_char = input[index + 1] - '0';
        } else if (input[index + 1] >= 'A' && input[index + 1] <= 'F') {
            iter_char = input[index + 1] - 'A' + 0x0A;
        } else if (input[index + 1] >= 'a' && input[index + 1] <= 'f') {
            iter_char = input[index + 1] - 'a' + 0x0A;
        } else {
            printf("c:%c\r\n",iter_char);
            return -3;
        }
        output[index / 2] |= (iter_char) & 0x0F;
    }

    return 0;
}


/**
 ****************************************************************************************
 * @brief Function to read a particular character and map its type
 *
 * This function is called to read a particular character and fetch its type
 *
 * @param[in] c Input character
 *
 * @return Type of the character
 ****************************************************************************************
 */
static uint32_t type_get(char c)
{
    uint32_t res;

    switch (c)
    {
    case '%':
        res =  C_PERCENT;
        break;

    case '.':
        res =  C_DOT;
        break;

    case '0':
        res =  C_ZERO;
        break;

    case ' ':
    case '+':
    case '-':
        res =  C_FLAG;
        break;

    case 'h':
    case 'l':
        res =  C_SIZE;
        break;

    case 'x':
    case 'X':
    case 'd':
    case 'b':
    case 'i':
    case 'c':
    case 'u':
    case 's':
    case 'm':
    case 'M':
    case 'a':
    case 'A':
    case 'p':
    case 'P':
        res =  C_TYPE;
        break;

    case '*':
        res =  C_DIGIT;
        break;

    default:
        if (('1' <= c) && (c <= '9'))
        {
            res =  C_DIGIT;
        }
        else
        {
            res =  C_OTHER;
        }
        break;
    }

    return res;
}

uint32_t dbg_vsnprintf_offset(char *buffer, uint32_t size, uint32_t offset,
                              const char *fmt, va_list args)
{
    uint32_t state_current = S_COPY;      // Initial state
    char c;

    char *fmt_field_ptr = NULL;
    int fmt_field_size = 0;

    char buffer_tmp[64];

    int field_width = 0;
    char field_flags = 0;
    int field_precision = 0;
    int field_padding = 0;

    int32_t value;
    char *tmp_ptr = NULL;

    uint32_t res = 0, remain;

    #define WRITE_CHAR(_c) if (remain && res >= offset) \
        {                                               \
            *buffer++ = (_c);remain--;                  \
        }                                               \
        res++

    // Check parameters: If buffer is NULL then size MUST be 0.
    if (buffer == NULL)
        size = 0;

    remain = size;

    // For each char in format string
    while ((c = *fmt++) != 0) {
        state_current =  transition_table[state_current][type_get(c)];

        switch (state_current)
        {
        case S_COPY:
            WRITE_CHAR(c);
            break;

        case S_PERCENT:
            // Assign default value for the conversion parameters
            field_width = 0;
            field_flags = 0;
            field_precision = -1;
            break;

        case S_FLAGS:
            // set flag based on which flag character
            switch (c)
            {
            case '-':
                // left justify
                field_flags |= FLAG_LEFT;
                break;
            case '+':
                // force sign indicator
                field_flags |= FLAG_SIGN;
                break;
            case ' ':
                // force sign or space
                field_flags |= FLAG_SIGN_SPACE;
                break;
            case '0':
                // pad with leading 0
                field_flags |= FLAG_LEAD_ZERO;
                break;
            default:
                PLF_ASSERT_ERR(0);
                break;
            }
            break;

        case S_WIDTH:
            if (c != '*') {
                // add digit to current field width
                field_width = field_width * 10 + (c - '0');
            } else {
                field_width = (int)va_arg(args, int);
            }
            break;

        case S_DOT:
            // Clear the field_precision variable
            field_precision = 0;
            break;

        case S_PRECIS:
            if (c != '*') {
                // Add digit to field_precision variable
                field_precision = field_precision * 10 + (c - '0');
            } else {
                field_precision = (int)va_arg(args, int);
            }
            break;

        case S_SIZE:
            // currently ignored
            switch (c)
            {
            case 'l':
                // 'l' => long int
                field_flags |= FLAG_LONG;
                break;

                #ifdef FLAG_SHORT_SUPPORTED
            case 'h':
                // 'h' => short int
                field_flags |= FLAG_SHORT;
                break;
                #endif // FLAG_SHORT_SUPPORTED

            default:
                PLF_ASSERT_ERR(0);
                break;
            }
            break;

        case S_TYPE:

            // Now the options have been decoded
            switch (c)
            {
            // c
            case 'c':
                // Store byte in Tx buffer
                buffer_tmp[0] = (char)va_arg(args, int);
                fmt_field_ptr = buffer_tmp;
                fmt_field_size = 1;
                break;


            // String
            case 's':
                // Read parameter (pointer on string)
                fmt_field_ptr =  va_arg(args, char *);
                fmt_field_size = 0;
                if (fmt_field_ptr != NULL) {
                    // Compute the length of the string
                    // field_precision is the maximum number of character to be
                    // display
                    tmp_ptr =  fmt_field_ptr;
                    while (*tmp_ptr != '\0') {
                        if (field_precision == 0)
                            break;
                        if (field_precision > 0)
                            field_precision--;
                        tmp_ptr++;
                        fmt_field_size++;
                    }
                }
                break;

            // MAC address
            case 'm':
            case 'M':
            {
                int i;
                fmt_field_ptr   = buffer_tmp;
                tmp_ptr         = va_arg(args, char *);
                fmt_field_size  = 17;
                for (i = 5;;) {
                    value = (unsigned char)*tmp_ptr++;
                    *fmt_field_ptr++ = hex_upper_table[value >> 4];
                    *fmt_field_ptr++ = hex_upper_table[value & 0xF];
                    if (i-- == 0)
                        break;
                    *fmt_field_ptr++ = ':';
                }
                fmt_field_ptr = buffer_tmp;
                break;
            }

            case 'a':
            case 'A':
                fmt_field_ptr = buffer_tmp;
                tmp_ptr = va_arg(args, char *);
                // prevent overflow
                field_width = min(field_width, sizeof(buffer_tmp) / 3);
                // if no width given
                if (!field_width)
                    field_width = 16;
                fmt_field_size = field_width * 3 - 1;
                for (;;) {
                    value = (unsigned char)*tmp_ptr++;
                    *fmt_field_ptr++ = hex_upper_table[value >> 4];
                    *fmt_field_ptr++ = hex_upper_table[value & 0xF];
                    if (--field_width == 0)
                        break;
                    // sep . (or : on align)
                    if (3 & (uint32_t)tmp_ptr)
                        *fmt_field_ptr++ = '.';
                    else
                        *fmt_field_ptr++ = ':';
                }
                fmt_field_ptr = buffer_tmp;
                break;

            case 'p':
            case 'P':
                fmt_field_ptr = buffer_tmp + sizeof(buffer_tmp);
                fmt_field_size = 0;
                tmp_ptr = va_arg(args, void *);
                value = (uint32_t) tmp_ptr;
                do {
                    // go backward
                    fmt_field_ptr--;
                    *fmt_field_ptr = hex_upper_table[value & 0XFUL];
                    value = ((uint32_t)value) >> 4;
                    fmt_field_size++;
                } while (value != 0);
                fmt_field_ptr--;
                *fmt_field_ptr = 'x';
                fmt_field_size++;
                fmt_field_ptr--;
                *fmt_field_ptr = '0';
                fmt_field_size++;
                break;

            case 'i': // signed decimal
                c = 'd';
            case 'd': // signed decimal
            case 'u': // unsigned
            case 'X': // hexa
            case 'x': // hexa
            case 'b': // binary

                // Point to the last byte of the buffer (go backward during
                // conversion)
                fmt_field_ptr = buffer_tmp + sizeof(buffer_tmp);
                fmt_field_size = 0;

                // Get the value
                if (field_flags & FLAG_LONG) {
                    // long
                    value = va_arg(args, uint32_t);
                }
                #ifdef FLAG_SHORT_SUPPORTED
                else if (field_flags & FLAG_SHORT) {
                    if (c == 'd') {
                        // extend the sign
                        value = (int16_t)va_arg(args, int);
                    } else {
                        value = (uint16_t)va_arg(args, int);
                    }
                }
                #endif // FLAG_SHORT_SUPPORTED
                else {
                    // int
                    // extend the sign
                    value = va_arg(args, int);
                }

                switch (c)
                {
                case 'd':
                    // Separate the sign to display it before the number
                    if (value < 0) {
                        value =  (uint32_t)(-value);
                        // remember negative sign
                        field_flags |= FLAG_NEGATIVE;
                    }

                case 'u':
                    do {
                        // go backward
                        fmt_field_ptr--;
                        *fmt_field_ptr = ('0' + ((char)(value % 10)));
                        value = value / 10;
                        fmt_field_size++;
                    } while (value != 0);

                    // Add the sign
                    if (field_flags & FLAG_NEGATIVE) { // prefix with a '-'
                        // go backward
                        fmt_field_ptr--;

                        *fmt_field_ptr = '-';
                        fmt_field_size++;
                    }
                    else if (field_flags & FLAG_SIGN) { // prefix with a '+' (sign is forced)
                        // go backward
                        fmt_field_ptr--;
                        *fmt_field_ptr = '+';
                        fmt_field_size++;
                    } else if (field_flags & FLAG_SIGN_SPACE) { // prefix with a ' ' (used instead of '+')
                        // go backward
                        fmt_field_ptr--;
                        *fmt_field_ptr = ' ';
                        fmt_field_size++;
                    }
                    break;

                case 'x':
                    #ifdef SUPPORT_LOWER_CASE_HEX
                    do {
                        // go backward
                        fmt_field_ptr--;
                        *fmt_field_ptr = hex_lower_table[value & 0XFUL];
                        value = ((uint32_t)value) >> 4;
                        fmt_field_size++;
                    } while (value != 0);
                    break;
                    #endif

                case 'X':
                    do {
                        // go backward
                        fmt_field_ptr--;
                        *fmt_field_ptr = hex_upper_table[value & 0XFUL];
                        value = ((uint32_t)value) >> 4;
                        fmt_field_size++;
                    } while (value != 0);
                    break;

                case 'b':
                    do {
                        // go backward
                        fmt_field_ptr--;
                        *fmt_field_ptr = '0' + ((char)(value & 0x01UL));
                        value = ((uint32_t)value) >> 1;
                        fmt_field_size++;
                    } while (value != 0);
                    break;
                default:
                    //ASSERT_ERR(0);
                    break;
                } // embedded switch type

            default:
                //ASSERT_ERR(0);
                break;
            } // switch type

            // Add padding
            field_padding = field_width - fmt_field_size;

            // put out the padding, prefix, and text, in the correct order
            if (field_flags & FLAG_LEAD_ZERO) {
                // Add leading zeros
                while (field_padding > 0) {
                    WRITE_CHAR('0');
                    field_padding--;
                }
            } else if (!(field_flags & FLAG_LEFT)) {
                while (field_padding > 0) {
                    WRITE_CHAR(' ');
                    field_padding--;
                }
            }

            // Copy the formated field
            while (fmt_field_size > 0) {
                WRITE_CHAR(*fmt_field_ptr);
                fmt_field_ptr++;
                fmt_field_size--;
            }

            // Add blanks at the rigth (means (field_flags & FLAG_LEFT))
            while ((field_padding > 0)) {
                WRITE_CHAR(' ');
                field_padding--;
            }
            break;

        default:
            PLF_ASSERT_ERR(0);
            break;
        }  // switch state
    }  // while

    #undef WRITE_CHAR

    if (remain) {
        *buffer = '\0';
    } else if (size) {
        buffer[-1] = '\0';
    }

    return res;
}

uint32_t dbg_snprintf(char *buffer, uint32_t size, const char *fmt, ...)
{
    uint32_t ret;
    const char *fmt_usr = (const char *)fmt;

    // print
    va_list args;
    va_start(args, fmt);
    ret = dbg_vsnprintf(buffer, size, fmt_usr, args);
    va_end(args);

    return ret;
}
