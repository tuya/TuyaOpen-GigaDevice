/*!
    \file    rom_xmodem.h
    \brief   UART xmodem protocol defines

    \version 2022-08-30, V1.0.0
*/

/*
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
#ifndef __XMODE_H__
#define __XMODE_H__

#include "stdint.h"

#define    _TRUE            1U
#define    _FALSE           0U
/*
* command byte
*/
#define    SOH            0x01        /* Start of header */
#define    STX            0x02        /* Start of header XModem-1K */
#define    EOT            0x04        /* End of transmission */
#define    ACK            0x06        /* Acknowledge */
#define    NAK            0x15        /* Not acknowledge */
#define    CAN            0x18        /* Cancel */
#define    ESC            0x1b        /* User Break */
#define    BAUDSET        0x05        /* Set Baud Rate */
#define    FRMSIZE        0x07        /* Set transfered frame size and check Baud Rate set OK */
#define    FLERASE        0x17        /* Erase Flash */
#define    CPIDGET        0x20        /* Get chip ID */

#define    IMGCHCK        0x21        /* Check the integrity of image */
#define    IMGDONE        0x22        /* Image checked OK */

#define    SYNCREQ        0x75        /* UART detect sync request */
#define    SYNCACK        0x79        /* UART detect sync ack */

/*
 * xmodem status
 */
#define    XMODEM_OK                        1
#define    XMODEM_CANCEL                    2
#define    XMODEM_ACK                       3
#define    XMODEM_NAK                       4
#define    XMODEM_COMPLETE                  5
#define    XMODEM_NO_SESSION                6
#define    XMODEM_ABORT                     7
#define    XMODEM_TIMEOUT                   8

/*
 * Other defs
 */
#define    FRAME_SIZE                       (128 + 4)
#define    FRAME_SIZE_MAX                   (3072 + 4)
#define    WAIT_FRAME_TIME                  (2500 * 1000)
#define    WAIT_CHAR_TIME                   (1250 * 1000)
#define    WAIT_HANDSHAKE_TIME              (5000 * 1000)
#define    XMODEM_READ_MAXRETRANS           25

#define    XMODEM_WRITE_FLASH               0x08
#define    XMODEM_WRITE_RAM                 0x20

/*
 * xmodem control
 */
typedef struct _xmodem_ctrl {
    uint16_t cur_frm_seq;
    uint16_t expected_sz;
    uint16_t frame_size;                        /* Specified by UI APP different by BaudRate */
    int32_t b_finished;
    uint8_t frm_buf[FRAME_SIZE_MAX + 20];
} xmodem_ctrl_t;

enum {
    LED_GREEN = 0,
    LED_RED,
    LED_BLUE,
};

extern void xmodem_start(uint32_t timeout);
extern void xmodem_led_init(void);
extern void xmodem_led_on(int led);
extern void xmodem_led_off(int led);

#endif /* __XMODE_H__ */

