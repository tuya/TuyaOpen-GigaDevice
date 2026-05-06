/*!
    \file    init_rom_symbol.c
    \brief   Rom symbol init function for GD32VW55x SDK.

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
#define EXTERN

#include "mbedtls/version.h"

#include "gd32vw55x_rcu.h"
#ifndef EXEC_USING_STD_PRINTF
#include "dbg_print.h"
#endif
#include "app_cfg.h"

#if (MBEDTLS_VERSION_NUMBER == 0x02110000)
#include "rom_export_mbedtls.h"
#endif

static uint8_t ana_version_get(void)
{
    return REG32(RCU + 0x00000138U);
}

void rom_symbol_init(void)
{
    int acut = (ana_version_get() == 0) ? 1 : 0;

    if (acut) {
        mbedtls_hwpka_flag_set(0
#ifndef CONFIG_NEED_RSA_4096
                            | MBEDTLS_HW_EXP_MOD
                            | MBEDTLS_HW_RSA_PRIVATE
                            | MBEDTLS_HW_MPI_MUL
 #endif /* CONFIG_NEED_RSA_4096 */
                            | MBEDTLS_HW_ECP_MUL
                            | MBEDTLS_HW_ECP_CHECK
                            );
    } else {
        mbedtls_hwpka_flag_set(0
#ifndef CONFIG_NEED_RSA_4096
                            | MBEDTLS_HW_EXP_MOD
                            | MBEDTLS_HW_RSA_PRIVATE
                            | MBEDTLS_HW_MPI_MUL
#endif /* CONFIG_NEED_RSA_4096 */
                            | MBEDTLS_HW_ECDSA_SIGN
                            | MBEDTLS_HW_ECDSA_VERIFY
                            | MBEDTLS_HW_ECP_MUL
                            | MBEDTLS_HW_ECP_CHECK
                            );
    }

#ifdef CONFIG_ASIC_CUT_AUTO
#if (MBEDTLS_VERSION_NUMBER == 0x02110000)
    if (acut) {
#ifndef EXEC_USING_STD_PRINTF
        dbg_print(NOTICE, "Chip: GD32VW55x ACut\r\n");
#endif
        ecp_supported_curves = (mbedtls_ecp_curve_info *)0x0bf73eb8;
        mbedtls_ecdsa_free_fn = (void *)0x0bf715dc;
        mbedtls_ecdsa_from_keypair_fn = (void *)0x0bf7158a;
        mbedtls_ecdsa_genkey_fn = (void *)0x0bf7154c;
        mbedtls_ecdsa_init_fn = (void *)0x0bf715d8;
        mbedtls_ecdsa_read_signature_fn = (void *)0x0bf71546;
        mbedtls_ecdsa_read_signature_restartable_fn = (void *)0x0bf714a0;
        mbedtls_ecdsa_sign_fn = (void *)0x0bf70bca;
        mbedtls_ecdsa_sign_det_fn = (void *)0x0bf7108c;
        mbedtls_ecdsa_verify_fn = (void *)0x0bf71090;
        mbedtls_ecdsa_write_signature_fn = (void *)0x0bf71470;
        mbedtls_ecdsa_write_signature_det_fn = (void *)0x0bf71480;
        mbedtls_ecdsa_write_signature_restartable_fn = (void *)0x0bf713aa;
        mbedtls_ecp_check_privkey_fn = (void *)0x0bf72c3e;
        mbedtls_ecp_check_pub_priv_fn = (void *)0x0bf73c8e;
        mbedtls_ecp_check_pubkey_fn = (void *)0x0bf72f6a;
        mbedtls_ecp_copy_fn = (void *)0x0bf72172;
        mbedtls_ecp_curve_info_from_grp_id_fn = (void *)0x0bf71f7e;
        mbedtls_ecp_curve_info_from_name_fn = (void *)0x0bf71fda;
        mbedtls_ecp_curve_info_from_tls_id_fn = (void *)0x0bf71faa;
        mbedtls_ecp_curve_list_fn = (void *)0x0bf71f0a;
        mbedtls_ecp_gen_key_fn = (void *)0x0bf73c48;
        mbedtls_ecp_gen_keypair_fn = (void *)0x0bf73c38;
        mbedtls_ecp_gen_keypair_base_fn = (void *)0x0bf73bdc;
        mbedtls_ecp_gen_privkey_fn = (void *)0x0bf72cbc;
        mbedtls_ecp_group_copy_fn = (void *)0x0bf721b0;
        mbedtls_ecp_group_free_fn = (void *)0x0bf720cc;
        mbedtls_ecp_group_init_fn = (void *)0x0bf72044;
        mbedtls_ecp_grp_id_list_fn = (void *)0x0bf71f32;
        mbedtls_ecp_is_zero_fn = (void *)0x0bf728e4;
        mbedtls_ecp_keypair_free_fn = (void *)0x0bf7214c;
        mbedtls_ecp_keypair_init_fn = (void *)0x0bf720a2;
        mbedtls_ecp_mul_fn = (void *)0x0bf73bd6;
        mbedtls_ecp_mul_restartable_fn = (void *)0x0bf73220;
        mbedtls_ecp_muladd_fn = (void *)0x0bf73e98;
        mbedtls_ecp_muladd_restartable_fn = (void *)0x0bf73e7c;
        mbedtls_ecp_point_cmp_fn = (void *)0x0bf728fa;
        mbedtls_ecp_point_free_fn = (void *)0x0bf720c4;
        mbedtls_ecp_point_init_fn = (void *)0x0bf72022;
        mbedtls_ecp_point_read_binary_fn = (void *)0x0bf72a54;
        mbedtls_ecp_point_read_string_fn = (void *)0x0bf72942;
        mbedtls_ecp_point_write_binary_fn = (void *)0x0bf72986;
        mbedtls_ecp_set_zero_fn = (void *)0x0bf721b6;
        mbedtls_ecp_tls_read_group_fn = (void *)0x0bf72bc4;
        mbedtls_ecp_tls_read_group_id_fn = (void *)0x0bf72b5c;
        mbedtls_ecp_tls_read_point_fn = (void *)0x0bf72af2;
        mbedtls_ecp_tls_write_group_fn = (void *)0x0bf72be4;
        mbedtls_ecp_tls_write_point_fn = (void *)0x0bf72b26;
        mbedtls_internal_md5_process_fn = (void *)0x0bf76912;
        mbedtls_internal_sha256_process_fn = (void *)0x0bf7674e;
        mbedtls_md5_fn = (void *)0x0bf7778e;
        mbedtls_md5_clone_fn = (void *)0x0bf768a4;
        mbedtls_md5_finish_fn = (void *)0x0bf77758;
        mbedtls_md5_finish_ret_fn = (void *)0x0bf7765a;
        mbedtls_md5_free_fn = (void *)0x0bf76898;
        mbedtls_md5_init_fn = (void *)0x0bf7688e;
        mbedtls_md5_process_fn = (void *)0x0bf77644;
        mbedtls_md5_ret_fn = (void *)0x0bf7775c;
        mbedtls_md5_starts_fn = (void *)0x0bf768e0;
        mbedtls_md5_starts_ret_fn = (void *)0x0bf768ac;
        mbedtls_md5_update_fn = (void *)0x0bf77652;
        mbedtls_md5_update_ret_fn = (void *)0x0bf77648;
        mbedtls_mpi_div_int_fn = (void *)0x0bf6f20a;
        mbedtls_mpi_div_mpi_fn = (void *)0x0bf6ed1e;
        mbedtls_mpi_exp_mod_fn = (void *)0x0bf6fb5c;
        mbedtls_mpi_exp_mod_sw_fn = (void *)0x0bf6f54c;
        mbedtls_mpi_fill_random_fn = (void *)0x0bf6fee4;
        mbedtls_mpi_gcd_fn = (void *)0x0bf6fcec;
        mbedtls_mpi_gen_prime_fn = (void *)0x0bf705e2;
        mbedtls_mpi_inv_mod_fn = (void *)0x0bf70212;
        mbedtls_mpi_is_prime_fn = (void *)0x0bf705d6;
        mbedtls_mpi_is_prime_ext_fn = (void *)0x0bf70534;
        mbedtls_mpi_mod_int_fn = (void *)0x0bf6f2d0;
        mbedtls_mpi_mod_mpi_fn = (void *)0x0bf6f234;
        mbedtls_mpi_mul_int_fn = (void *)0x0bf6ed04;
        mbedtls_mpi_mul_mpi_fn = (void *)0x0bf6ead6;
        mbedtls_mpi_read_string_fn = (void *)0x0bf6eaec;
        mbedtls_mpi_write_string_fn = (void *)0x0bf6f304;
        mbedtls_rsa_check_privkey_fn = (void *)0x0bf74972;
        mbedtls_rsa_check_pub_priv_fn = (void *)0x0bf74c08;
        mbedtls_rsa_check_pubkey_fn = (void *)0x0bf74910;
        mbedtls_rsa_complete_fn = (void *)0x0bf74490;
        mbedtls_rsa_copy_fn = (void *)0x0bf76132;
        mbedtls_rsa_export_fn = (void *)0x0bf74728;
        mbedtls_rsa_export_crt_fn = (void *)0x0bf7482a;
        mbedtls_rsa_export_raw_fn = (void *)0x0bf745f0;
        mbedtls_rsa_free_fn = (void *)0x0bf7623c;
        mbedtls_rsa_gen_key_fn = (void *)0x0bf749e2;
        mbedtls_rsa_get_len_fn = (void *)0x0bf7490c;
        mbedtls_rsa_import_fn = (void *)0x0bf74364;
        mbedtls_rsa_import_raw_fn = (void *)0x0bf743f6;
        mbedtls_rsa_init_fn = (void *)0x0bf748d4;
        mbedtls_rsa_pkcs1_decrypt_fn = (void *)0x0bf75a32;
        mbedtls_rsa_pkcs1_encrypt_fn = (void *)0x0bf75574;
        mbedtls_rsa_pkcs1_sign_fn = (void *)0x0bf75d64;
        mbedtls_rsa_pkcs1_verify_fn = (void *)0x0bf760d4;
        mbedtls_rsa_private_fn = (void *)0x0bf752ce;
        mbedtls_rsa_private_sw_fn = (void *)0x0bf74da8;
        mbedtls_rsa_public_fn = (void *)0x0bf74c50;
        mbedtls_rsa_rsaes_oaep_decrypt_fn = (void *)0x0bf755a8;
        mbedtls_rsa_rsaes_oaep_encrypt_fn = (void *)0x0bf752e2;
        mbedtls_rsa_rsaes_pkcs1_v15_decrypt_fn = (void *)0x0bf757da;
        mbedtls_rsa_rsaes_pkcs1_v15_encrypt_fn = (void *)0x0bf75448;
        mbedtls_rsa_rsassa_pkcs1_v15_sign_fn = (void *)0x0bf75c56;
        mbedtls_rsa_rsassa_pkcs1_v15_verify_fn = (void *)0x0bf75fca;
        mbedtls_rsa_rsassa_pss_sign_fn = (void *)0x0bf75a6c;
        mbedtls_rsa_rsassa_pss_verify_fn = (void *)0x0bf75faa;
        mbedtls_rsa_rsassa_pss_verify_ext_fn = (void *)0x0bf75d82;
        mbedtls_rsa_set_padding_fn = (void *)0x0bf74902;
        mbedtls_sha256_clone_fn = (void *)0x0bf767b0;
        mbedtls_sha256_finish_fn = (void *)0x0bf7686a;
        mbedtls_sha256_finish_ret_fn = (void *)0x0bf76842;
        mbedtls_sha256_free_fn = (void *)0x0bf7679e;
        mbedtls_sha256_init_fn = (void *)0x0bf7679a;
        mbedtls_sha256_starts_fn = (void *)0x0bf767e2;
        mbedtls_sha256_starts_ret_fn = (void *)0x0bf767b8;
        mbedtls_sha256_update_fn = (void *)0x0bf76816;
        mbedtls_sha256_update_ret_fn = (void *)0x0bf767e6;
    } else {
#ifndef EXEC_USING_STD_PRINTF
        dbg_print(NOTICE, "Chip: GD32VW55x\r\n");
#endif
        ecp_supported_curves = (mbedtls_ecp_curve_info *)0x0bf73a7c;
        mbedtls_ecdsa_free_fn = (void *)0x0bf71390;
        mbedtls_ecdsa_from_keypair_fn = (void *)0x0bf7133e;
        mbedtls_ecdsa_genkey_fn = (void *)0x0bf71300;
        mbedtls_ecdsa_init_fn = (void *)0x0bf7138c;
        mbedtls_ecdsa_read_signature_fn = (void *)0x0bf712fa;
        mbedtls_ecdsa_read_signature_restartable_fn = (void *)0x0bf71254;
        mbedtls_ecdsa_sign_fn = (void *)0x0bf70b6e;
        mbedtls_ecdsa_sign_det_fn = (void *)0x0bf70f2e;
        mbedtls_ecdsa_verify_fn = (void *)0x0bf70f32;
        mbedtls_ecdsa_write_signature_fn = (void *)0x0bf71224;
        mbedtls_ecdsa_write_signature_det_fn = (void *)0x0bf71234;
        mbedtls_ecdsa_write_signature_restartable_fn = (void *)0x0bf7115e;
        mbedtls_ecp_check_privkey_fn = (void *)0x0bf72a0e;
        mbedtls_ecp_check_pub_priv_fn = (void *)0x0bf73848;
        mbedtls_ecp_check_pubkey_fn = (void *)0x0bf72c78;
        mbedtls_ecp_copy_fn = (void *)0x0bf71f42;
        mbedtls_ecp_curve_info_from_grp_id_fn = (void *)0x0bf71d4e;
        mbedtls_ecp_curve_info_from_name_fn = (void *)0x0bf71daa;
        mbedtls_ecp_curve_info_from_tls_id_fn = (void *)0x0bf71d7a;
        mbedtls_ecp_curve_list_fn = (void *)0x0bf71cda;
        mbedtls_ecp_gen_key_fn = (void *)0x0bf73802;
        mbedtls_ecp_gen_keypair_fn = (void *)0x0bf737f2;
        mbedtls_ecp_gen_keypair_base_fn = (void *)0x0bf73796;
        mbedtls_ecp_gen_privkey_fn = (void *)0x0bf72a8c;
        mbedtls_ecp_group_copy_fn = (void *)0x0bf71f80;
        mbedtls_ecp_group_free_fn = (void *)0x0bf71e9c;
        mbedtls_ecp_group_init_fn = (void *)0x0bf71e14;
        mbedtls_ecp_grp_id_list_fn = (void *)0x0bf71d02;
        mbedtls_ecp_is_zero_fn = (void *)0x0bf726b4;
        mbedtls_ecp_keypair_free_fn = (void *)0x0bf71f1c;
        mbedtls_ecp_keypair_init_fn = (void *)0x0bf71e72;
        mbedtls_ecp_mul_fn = (void *)0x0bf73790;
        mbedtls_ecp_mul_restartable_fn = (void *)0x0bf72ebc;
        mbedtls_ecp_muladd_fn = (void *)0x0bf73a52;
        mbedtls_ecp_muladd_restartable_fn = (void *)0x0bf73a36;
        mbedtls_ecp_point_cmp_fn = (void *)0x0bf726ca;
        mbedtls_ecp_point_free_fn = (void *)0x0bf71e94;
        mbedtls_ecp_point_init_fn = (void *)0x0bf71df2;
        mbedtls_ecp_point_read_binary_fn = (void *)0x0bf72824;
        mbedtls_ecp_point_read_string_fn = (void *)0x0bf72712;
        mbedtls_ecp_point_write_binary_fn = (void *)0x0bf72756;
        mbedtls_ecp_set_zero_fn = (void *)0x0bf71f86;
        mbedtls_ecp_tls_read_group_fn = (void *)0x0bf72994;
        mbedtls_ecp_tls_read_group_id_fn = (void *)0x0bf7292c;
        mbedtls_ecp_tls_read_point_fn = (void *)0x0bf728c2;
        mbedtls_ecp_tls_write_group_fn = (void *)0x0bf729b4;
        mbedtls_ecp_tls_write_point_fn = (void *)0x0bf728f6;
        mbedtls_internal_md5_process_fn = (void *)0x0bf764d6;
        mbedtls_internal_sha256_process_fn = (void *)0x0bf76312;
        mbedtls_md5_fn = (void *)0x0bf77352;
        mbedtls_md5_clone_fn = (void *)0x0bf76468;
        mbedtls_md5_finish_fn = (void *)0x0bf7731c;
        mbedtls_md5_finish_ret_fn = (void *)0x0bf7721e;
        mbedtls_md5_free_fn = (void *)0x0bf7645c;
        mbedtls_md5_init_fn = (void *)0x0bf76452;
        mbedtls_md5_process_fn = (void *)0x0bf77208;
        mbedtls_md5_ret_fn = (void *)0x0bf77320;
        mbedtls_md5_starts_fn = (void *)0x0bf764a4;
        mbedtls_md5_starts_ret_fn = (void *)0x0bf76470;
        mbedtls_md5_update_fn = (void *)0x0bf77216;
        mbedtls_md5_update_ret_fn = (void *)0x0bf7720c;
        mbedtls_mpi_div_int_fn = (void *)0x0bf6f1c6;
        mbedtls_mpi_div_mpi_fn = (void *)0x0bf6ecda;
        mbedtls_mpi_exp_mod_fn = (void *)0x0bf6fb18;
        mbedtls_mpi_exp_mod_sw_fn = (void *)0x0bf6f508;
        mbedtls_mpi_fill_random_fn = (void *)0x0bf6fe86;
        mbedtls_mpi_gcd_fn = (void *)0x0bf6fc8e;
        mbedtls_mpi_gen_prime_fn = (void *)0x0bf70584;
        mbedtls_mpi_inv_mod_fn = (void *)0x0bf701b4;
        mbedtls_mpi_is_prime_fn = (void *)0x0bf70578   ;
        mbedtls_mpi_is_prime_ext_fn = (void *)0x0bf704d6;
        mbedtls_mpi_mod_int_fn = (void *)0x0bf6f28c;
        mbedtls_mpi_mod_mpi_fn = (void *)0x0bf6f1f0;
        mbedtls_mpi_mul_int_fn = (void *)0x0bf6ecc0;
        mbedtls_mpi_mul_mpi_fn = (void *)0x0bf6ea92;
        mbedtls_mpi_read_string_fn = (void *)0x0bf6eaa8;
        mbedtls_mpi_write_string_fn = (void *)0x0bf6f2c0;
        mbedtls_rsa_check_privkey_fn = (void *)0x0bf74536;
        mbedtls_rsa_check_pub_priv_fn = (void *)0x0bf747cc;
        mbedtls_rsa_check_pubkey_fn = (void *)0x0bf744d4;
        mbedtls_rsa_complete_fn = (void *)0x0bf74054;
        mbedtls_rsa_copy_fn = (void *)0x0bf75cf6;
        mbedtls_rsa_export_fn = (void *)0x0bf742ec;
        mbedtls_rsa_export_crt_fn = (void *)0x0bf743ee;
        mbedtls_rsa_export_raw_fn = (void *)0x0bf741b4;
        mbedtls_rsa_free_fn = (void *)0x0bf75e00;
        mbedtls_rsa_gen_key_fn = (void *)0x0bf745a6;
        mbedtls_rsa_get_len_fn = (void *)0x0bf744d0;
        mbedtls_rsa_import_fn = (void *)0x0bf73f28;
        mbedtls_rsa_import_raw_fn = (void *)0x0bf73fba;
        mbedtls_rsa_init_fn = (void *)0x0bf74498;
        mbedtls_rsa_pkcs1_decrypt_fn = (void *)0x0bf755f6;
        mbedtls_rsa_pkcs1_encrypt_fn = (void *)0x0bf75138;
        mbedtls_rsa_pkcs1_sign_fn = (void *)0x0bf75928;
        mbedtls_rsa_pkcs1_verify_fn = (void *)0x0bf75c98;
        mbedtls_rsa_private_fn = (void *)0x0bf74e92;
        mbedtls_rsa_private_sw_fn = (void *)0x0bf7496c;
        mbedtls_rsa_public_fn = (void *)0x0bf74814;
        mbedtls_rsa_rsaes_oaep_decrypt_fn = (void *)0x0bf7516c;
        mbedtls_rsa_rsaes_oaep_encrypt_fn = (void *)0x0bf74ea6;
        mbedtls_rsa_rsaes_pkcs1_v15_decrypt_fn = (void *)0x0bf7539e;
        mbedtls_rsa_rsaes_pkcs1_v15_encrypt_fn = (void *)0x0bf7500c;
        mbedtls_rsa_rsassa_pkcs1_v15_sign_fn = (void *)0x0bf7581a;
        mbedtls_rsa_rsassa_pkcs1_v15_verify_fn = (void *)0x0bf75b8e;
        mbedtls_rsa_rsassa_pss_sign_fn = (void *)0x0bf75630;
        mbedtls_rsa_rsassa_pss_verify_fn = (void *)0x0bf75b6e;
        mbedtls_rsa_rsassa_pss_verify_ext_fn = (void *)0x0bf75946;
        mbedtls_rsa_set_padding_fn = (void *)0x0bf744c6;
        mbedtls_sha256_clone_fn = (void *)0x0bf76374;
        mbedtls_sha256_finish_fn = (void *)0x0bf7642e;
        mbedtls_sha256_finish_ret_fn = (void *)0x0bf76406;
        mbedtls_sha256_free_fn = (void *)0x0bf76362;
        mbedtls_sha256_init_fn = (void *)0x0bf7635e;
        mbedtls_sha256_starts_fn = (void *)0x0bf763a6;
        mbedtls_sha256_starts_ret_fn = (void *)0x0bf7637c;
        mbedtls_sha256_update_fn = (void *)0x0bf763da;
        mbedtls_sha256_update_ret_fn = (void *)0x0bf763aa;
    }
#endif /* MBEDTLS_VERSION_NUMBER == 0x02110000 */
#endif /* CONFIG_ASIC_CUT_AUTO */
}
