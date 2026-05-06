/*!
    \file    mbedtls_patch.c
    \brief   mbedtls patch for GD32VW55x SDK

    \version 2023-8-8, V1.0.0, firmware for GD32VW55x
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

#include "rom_export_mbedtls.h"
#include "mbedtls/platform_util.h"

#define DHM_VALIDATE( cond )        \
    MBEDTLS_INTERNAL_VALIDATE( cond )

void mbedtls_dhm_init( mbedtls_dhm_context *ctx )
{
    DHM_VALIDATE( ctx != NULL );
    memset( ctx, 0, sizeof( mbedtls_dhm_context ) );

    mbedtls_mpi_init( &ctx->pX );
    mbedtls_mpi_init( &ctx->Vf );
    mbedtls_mpi_init( &ctx->Vi );
    mbedtls_mpi_init( &ctx->RP );
    mbedtls_mpi_init( &ctx->K  );
    mbedtls_mpi_init( &ctx->GY );
    mbedtls_mpi_init( &ctx->GX );
    mbedtls_mpi_init( &ctx->X  );
    mbedtls_mpi_init( &ctx->G  );
    mbedtls_mpi_init( &ctx->P  );
}

