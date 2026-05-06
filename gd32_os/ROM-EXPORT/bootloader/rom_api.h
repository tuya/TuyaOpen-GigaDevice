/*!
    \file    rom_api.h
    \brief   Rom API for GD32VW55x SDK

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

#ifndef __ROM_API_H__
#define __ROM_API_H__

#define MAX_API_NUM     256

struct rom_api_t {
    int (*printf)(const char *format, ...);
    int (*trace_ex)(uint32_t level, const char *fmt, ...);
    int (*rand)(unsigned char *output, unsigned int len);
    int (*hardware_poll)(void *data, unsigned char *output, size_t len, size_t *olen);

    uint32_t (*cal_checksum)(IN uint8_t *ptr, IN uint32_t sz);
    int (*img_verify_sign)(IN uint8_t algo_sign,
                        IN uint8_t *pk,
                        IN size_t klen,
                        IN uint8_t *hash,
                        IN uint32_t hlen,
                        IN uint8_t *sig,
                        IN uint32_t slen);
    int (*img_verify_digest)(IN uint8_t algo_hash,
                        IN uint32_t faddr,  /* Flash Adress */
                        IN uint32_t len,
                        IN uint8_t *digest,
                        IN uint32_t diglen);
    int (*img_verify_hdr)(IN void *hdr,
                    IN uint8_t img_type);

    int (*img_verify_pkhash)(IN uint8_t *pk,
                IN uint32_t klen,
                IN uint8_t *pkhash,
                IN uint32_t hlen);

    int (*img_validate)(IN uint32_t img_faddr,
                   IN uint8_t img_type,
                   IN uint8_t *pkhash,
                   OUT void *img_info);

    int (*cert_img_validate)(IN uint32_t img_offset,
                   IN uint8_t img_type,
                   IN uint8_t *pkhash,
                   OUT void *img_info);

    int (*sys_setting_get)(void *settings);
    int (*sys_status_set)(uint8_t type, uint8_t len, uint8_t *pval);
    int (*sys_status_get)(uint8_t type, uint8_t len, uint8_t *pval);
    int (*sys_set_trace_level)(uint8_t trace_level);
    int (*sys_set_err_process)(uint8_t method);
    int (*sys_set_img_flag)(uint8_t idx, uint8_t mask, uint8_t flag);
    int (*sys_reset_img_flag)(uint8_t idx);
    int (*sys_set_running_img)(uint8_t idx);
    int (*sys_set_fw_version)(uint32_t type, uint32_t version);
    int (*sys_set_pk_version)(uint32_t type, uint8_t key_ver);

    int (*flash_read)(uint32_t addr, void *data, int len);
    int (*flash_write)(uint32_t addr, const void *data, int len);
    int (*flash_erase)(uint32_t addr, int len);

    int (*efuse_get_rotpkh)(uint8_t *rotpkh);

    int (*do_symm_key_derive)(uint8_t *label, size_t label_sz,
                        uint8_t *key, size_t key_len);
    /* Add for MBL boot directly */
    int (*sys_status_check)(void);
    int (*log_uart_set)(uint32_t uart_peripheral);

    /* Add HAU DMA option for image digest calculatiion */
    int (*digest_haudma_en)(uint32_t enable);
};

#ifdef ROM_PROJECT
int sys_setting_get_api(void *settings);
int sys_status_set_api(uint8_t type, uint8_t len, uint8_t *pval);
int sys_status_get_api(uint8_t type, uint8_t len, uint8_t* pval);
int sys_set_err_process_api(uint8_t method);
int sys_set_trace_level_api(uint8_t trace_level);
int sys_set_img_flag_api(uint8_t idx, uint8_t mask, uint8_t flag);
int sys_reset_img_flag_api(uint8_t idx);
int sys_set_running_img_api(uint8_t idx);
int sys_set_fw_version_api(uint32_t type, uint32_t version);
int sys_set_pk_version_api(uint32_t type, uint8_t key_ver);
#endif /* ROM_SELF_TEST */

#endif  //__ROM_API_H__
