/*!
    \file    iap_command.h
    \brief   the header file of systick

    \version 2017-02-10, V1.0.0, firmware for GD32F30x
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

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

#ifndef IAP_COMMAND_H
#define IAP_COMMAND_H

#include <stdint.h>
#include "gd32vw55x.h"

#define F505_FW_START_ADDR 0x08010000
#define F505_FW_IN_W553_FLASH_ADDR 0x083CA000

/*USART bootloader commands	*/
#define USART_CMD_GET                 0x00  // ???????????
#define USART_CMD_GR                  0x01  // ???????????
#define USART_CMD_GET_ID              0x02  // ????ID
#define USART_CMD_READ                0x11  // ? RAM & Flash ????????256???
#define USART_CMD_GO                  0x21  // ????????(FLASH OR SRAM)
#define USART_CMD_PROGRAM             0x31  // ? 256????? RAM & Flash ?????
#define USART_CMD_ERASE               0x43  // ???????
#define USART_CMD_Ex_ERASE            0x44  // ????????????????(V3.0????)
#define USART_CMD_WRP_EN              0x63  // ?????????
#define USART_CMD_WRP_DIS             0x73  // ??flash???????
#define USART_CMD_RDP_EN              0x82  // ?????
#define USART_CMD_RDP_DIS             0x92  // ?????
#define USART_CMD_GET_PID             0x06
#define USART_CMD_FW_UPDATE           0x07
#define USART_CMD_HANDSHAKE           0x7F
#define USART_CMD_RUN_STOP            0x09
#define USART_CMD_FW_LENGTH           0x08

#define USART_CMD_MOTORS_CONTROL        0x51
#define USART_CMD_MOTORS_STATUS_GET     0x52
#define USART_CMD_MOTORS_CURRENT_GET    0x53
#define USART_CMD_SERVO_ANGLE_GET       0x54
#define USART_CMD_WORK_MODE_SET         0x55
#define USART_CMD_FAN_SPEED_GET         0x56

typedef enum {
    FAN_ID     = 0,
    SERVO_ID   = 1,
    PUMP_ID    = 2,
    MOTOR_ID_1 = 3,
    MOTOR_ID_2 = 4,
    MOTOR_ID_3 = 5,
    MAX_ID,
} motor_id_t;

typedef enum {
    MODE_STRONG     = 0,
    MODE_POWERSAVE  = 1,
    MODE_MAX,
} work_mode_t;


// ErrStatus usart_handshake(void);
ErrStatus usart_send_fw_update_command(uint32_t erase_size);
ErrStatus usart_send_fw_length_command(uint32_t erase_size);
ErrStatus usart_send_erase_command(uint32_t bin_size);
ErrStatus usart_program_bin(uint32_t pro_addr, uint32_t bin_addr, uint32_t bin_size);
ErrStatus usart_send_reset_command(void);
ErrStatus usart_send_handshake_command(void);
ErrStatus usart_send_run_stop_command(uint8_t flag);

ErrStatus usart_send_motors_control(motor_id_t motor_id, bool control);
ErrStatus usart_motor_status_get(motor_id_t motor_id, uint8_t *status);
ErrStatus usart_motor_current_get(motor_id_t motor_id, uint32_t *current);
ErrStatus usart_servo_angle_get(uint32_t *angle);
ErrStatus usart_fan_speed_get(uint32_t *speed);
ErrStatus usart_work_mode_set(uint8_t mode);
void usart_iap_init(void);

#endif /* IAP_COMMAND_H */
