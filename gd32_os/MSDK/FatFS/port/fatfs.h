/*!
    \file    fatfs.h
    \brief   fatfs init and config file

    \version 2024-05-30, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

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
#ifndef _FATFS_H_
#define _FATFS_H_

#include "app_cfg.h"
#include "ff.h"
#include "qspi_flash_api.h"

#ifdef CONFIG_FATFS_SUPPORT

FATFS* fatfs_get_fs(void);
bool fatfs_mk_mount(const MKFS_PARM* opt);
bool fatfs_unmount(void);
uint32_t file_size(char *path);
uint32_t cmd_fatfs_exec(int srgc, char** argv);
int fatfs_write_at_offset(char *path, uint32_t offset, uint8_t *data, uint32_t len);
int fatfs_append(char *path, uint8_t *data, int len);
int fatfs_delete(const char* path);
int fatfs_create(char *path);
uint32_t fatfs_read(char *path, uint8_t *buff, uint32_t len, uint32_t offset);
int fatfs_show(char *path, uint8_t root_path, bool at_stdout);
int fatfs_rename(char *path, char *new_path);
int fs_flash_write(LBA_t sector,const BYTE *buff, UINT count);
int fs_flash_read(LBA_t sector, BYTE *buff, UINT count);
int fs_flash_erase(LBA_t sector, UINT count);
uint32_t fs_flash_size(void);
#endif /* CONFIG_FATFS_SUPPORT */

#endif /* _FATFS_H_ */
