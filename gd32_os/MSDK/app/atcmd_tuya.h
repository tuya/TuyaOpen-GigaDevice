/*!
    \file    atcmd_tuya.h
    \brief   AT command for Tuya cloud

    \version 2025-02-10, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

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

#ifndef _ATCMD_TUYA_H_
#define _ATCMD_TUYA_H_

#include "app_cfg.h"

enum {
    AT_ERR = -1,
    AT_OK = 0,
    AT_RSP_ERR = 1,
    AT_RSP_TIMEOUT = 2,
};

/*=============================================================================*/
/*                  DPID Mapping Configuration                                */
/*=============================================================================*/

/**
 * @brief DPID mapping callback function type
 * @param dp_name DP name string (e.g., "power_percentage", "led_status")
 * @return DPID value for this project
 */
typedef uint8_t (*tuya_dpid_map_cb_t)(const char *dp_name);

/**
 * @brief Register DPID mapping callback
 * @param cb Callback function to map DP names to project-specific DPIDs
 *
 * This allows different projects to use different DPID values while sharing
 * the same atcmd_tuya.c implementation.
 *
 * Usage example in tuya_main.c:
 *   uint8_t my_dpid_map(const char *name) {
 *       if (strcmp(name, "power_percentage") == 0) return 102;
 *       if (strcmp(name, "led_status") == 0) return 103;
 *       return 0;
 *   }
 *   tuya_at_register_dpid_map(my_dpid_map);
 */
void tuya_at_register_dpid_map(tuya_dpid_map_cb_t cb);

/*=============================================================================*/
/*                  F527 -> VW553: AT Command Handlers                        */
/*  These functions handle AT commands received from F527                     */
/*=============================================================================*/

/**
 * @brief AT+TYTELS command handler
 * @brief Receive telemetry data or work mode from F527
 * @param argc Number of arguments
 * @param argv Argument array
 * @return none
 *
 * Command format 1: AT+TYTELS=<mode>
 *   Report work mode
 *   mode: 0=charge mode, 1=discharge mode
 *
 * Command format 2: AT+TYTELS=<mode>,<idx>,<value>
 *   Report telemetry data (mode can be 0 or 1, not used in data reporting)
 *   idx (uint8_t): Data type index
 *     - 0: Power percentage (uint8_t, 0-100)
 *     - 1: Voltage (float, 0-30V)
 *     - 2: Current (float, 0-30A)
 *     - 3: LED status (uint8_t, 0=off, 1=on)
 *
 * Examples:
 *   AT+TYTELS=0          // Report charge mode
 *   AT+TYTELS=1          // Report discharge mode
 *   AT+TYTELS=0,0,85     // Report power 85%
 *   AT+TYTELS=0,1,20.1   // Report voltage 20.1V
 *   AT+TYTELS=1,2,3.5    // Report current 3.5A
 *   AT+TYTELS=0,3,1      // Report LED on
 */
void at_tytels(int argc, char **argv);

/**
 * @brief AT+TYPROPS command handler
 * @brief Receive property status from F527
 * @param argc Number of arguments
 * @param argv Argument array
 * @return none
 *
 * Command format: AT+TYPROPS=<idx>,<status>
 * Parameters:
 *   idx (uint8_t): Property type index
 *     - 0: LED status
 *     - 1: Energy storage working status
 *   status (uint8_t):
 *     - For idx=0 (LED): 0=off, 1=on
 *     - For idx=1 (Work status): status value
 *
 * Examples:
 *   AT+TYPROPS=0,1   // LED on
 *   AT+TYPROPS=0,0   // LED off
 *   AT+TYPROPS=1,5   // Energy storage work status 5
 */
void at_typrops(int argc, char **argv);

/**
 * @brief AT+TYERRCODE command handler
 * @brief Receive error code from F527
 * @param argc Number of arguments
 * @param argv Argument array
 * @return none
 *
 * Command format: AT+TYERRCODE=<err_code>
 * Parameters:
 *   err_code (uint8_t): Error code (0-255)
 *
 * Examples:
 *   AT+TYERRCODE=0    // No error
 *   AT+TYERRCODE=1    // Error code 1
 *   AT+TYERRCODE=255  // Error code 255
 */
void at_tuya_error_code(int argc, char **argv);

/*=============================================================================*/
/*                  VW553 -> F527: API Functions                              */
/*  These functions send AT commands to F527                                  */
/*=============================================================================*/

/**
 * @brief Send connection response to F527
 * @param status Connection status (0:success, 1:fail)
 * @return AT_OK(0) on success, error code on failure
 *
 * This function sends: AT+TYCONNRSP=<status>
 *
 * Usage example:
 *   tuya_send_conn_response(0);  // Notify F527 connection success
 *   tuya_send_conn_response(1);  // Notify F527 connection failed
 */
int tuya_send_conn_response(uint8_t status);

/**
 * @brief Send property control request to F527
 * @param idx Control type index
 *   - 0: LED control
 *   - 1: Energy storage main switch control
 *   - 2: Energy storage charge control
 *   - 3: Energy storage discharge control
 * @param cmd Command value
 *   - For idx=0 (LED): 0=off, 1=on
 *   - For idx=1 (Main switch): 0=off, 1=on
 *   - For idx=2 (Charge): 0=on, 1=off
 *   - For idx=3 (Discharge): 0=on, 1=off
 * @return AT_OK(0) on success, error code on failure
 *
 * This function sends: AT+TYPROPREQ=<idx>,<cmd>
 *
 * Usage examples:
 *   tuya_send_property_request(0, 1);  // Turn on LED
 *   tuya_send_property_request(0, 0);  // Turn off LED
 *   tuya_send_property_request(1, 1);  // Main switch on
 *   tuya_send_property_request(1, 0);  // Main switch off
 *   tuya_send_property_request(2, 0);  // Charge on
 *   tuya_send_property_request(2, 1);  // Charge off
 *   tuya_send_property_request(3, 0);  // Discharge on
 *   tuya_send_property_request(3, 1);  // Discharge off
 */
int tuya_send_property_request(uint8_t idx, uint8_t cmd);

/**
 * @brief Send cloud-to-device message to F527
 * @param message Message string to send
 * @return AT_OK(0) on success, error code on failure
 *
 * This function sends: AT+TYC2DMSGS=<message>
 *
 * Usage example:
 *   tuya_send_c2d_message("Hello GD32");
 *   tuya_send_c2d_message("{\"cmd\":\"update\",\"value\":100}");
 */
int tuya_send_c2d_message(const char *message);

#endif /* _ATCMD_TUYA_H_ */
