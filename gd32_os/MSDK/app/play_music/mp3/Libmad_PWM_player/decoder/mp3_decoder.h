/*!
    \file    mp3_decoder.h
    \brief   the header file of mp3_decoder.c

    \version 2024-07-29, V1.3.0, firmware for GD32VW55x
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

#ifndef __MP3_DECODER_H__
#define __MP3_DECODER_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <string.h>


#define INPUT_BUFFER_SIZE	(1024 * 26)
#define OUTPUT_BUFFER_SIZE	 70 * 1024


typedef enum {
    decode_error = 0u,
    decode_ok = 1u,

} decode_error_state;


/* init decoder, and initialize libmad */
decode_error_state mp3_decoder_init(const uint8_t* mp3_data_address, uint32_t byte_number);

/* start decoder, store data into decode_data_address */
decode_error_state mp3_decoder_start(void);

uint16_t mp3_decoder_calc_maxfactor(void);

decode_error_state pm_mp3_decoder_init(const uint8_t* mp3_data_address, uint32_t *tag_size);

decode_error_state mp3_decoder_get_info(const uint8_t* mp3_data_address, uint32_t byte_number, int *ptr_frame_bytes);

uint32_t mp3_decode_play(const uint8_t* mp3_data_address, uint32_t byte_number);

void mp3_decode_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __MP3_DECODER_H__ */