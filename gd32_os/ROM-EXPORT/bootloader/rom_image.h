/*!
    \file    rom_image.h
    \brief   Rom image file for GD32VW55x SDK

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

#ifndef __ROM_IMAGE_H__
#define __ROM_IMAGE_H__

/* !!!!!Image signed with ED25519 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IN
#define IN
#define OUT
#endif

/*
* BOOT_RECORD is a feature macro that applies to the ROM and MBL code.
* This feature is an alternative implementation of mbl_status_add_sw_info fucntion
* which can read the already CBOR encoded measured boot record from the image manifest
* and writes it to the shared data area.
* In the future, it will only keep this feature, deprecate the original implementation
* and this macro will be removed.
*/
//#define BOOT_RECORD

/*
 * Image Magic
 */
#define IMG_MAGIC_NONE              0xffffffff
#define IMG_MAGIC_H                 0x96f3b83d
#define IMG_MAGIC_PTLV              0x6907
#define IMG_MAGIC_T                 {0xf395c277, 0x7fefd260, 0x0f505235, 0x8079b62c}

/*
 * Image Manifest Format Version
 */
#define IMG_MANI_FORMAT_VER         0

/*
 * Image TYPE.
 */
#define IMG_TYPE_MBL                0x1
#define IMG_TYPE_IMG                0x2

/*
 * Image Digest Hash Algorithm.
 */
#define IMG_HASH_SHA256             0x1
#define IMG_HASH_SHA512             0x2

/*
 * Image Signature Algorithm.
 */
#define IMG_SIG_ED25519             0x1
#define IMG_SIG_ECDSA256            0x2
//#define IMG_SIG_RSA2048             0x3
//#define IMG_SIG_RSA3072             0x4

/*
 * Image Protected TLV types.
 */
#define IMG_TLV_DIGEST              0x01   /* digest of image hdr + body */
#define IMG_TLV_CERT_PK             0x02   /* Public Key used to verify cert signature */
#define IMG_TLV_CERT                0x03   /* Certificate */
#define IMG_TLV_PK                  0x04   /* Public Key used to verify image signature */
#define IMG_TLV_SIG                 0x05   /* Image Signature */

/*
 * Image Related Length.
 */
#define IMG_HEADER_SIZE             32
#define IMG_VER_MAJOR_LEN           8
#define IMG_VER_MINOR_LEN           8
#define IMG_VER_REVISION_LEN        16

#define ED25519_PK_LEN              32
#define ED25519_SIG_LEN             64
//#define ECDSA256_PK_LEN             91
//#define ECDSA256_SIG_LEN            72
#define SIG_MAX_LEN                 128
#define PK_MAX_LEN                  128
#define PK_HASH_LEN                 32

#define IMG_DIGEST_SHA256_LEN       32
#define IMG_DIGEST_SHA512_LEN       64
#define IMG_DIGEST_MAX_LEN          IMG_DIGEST_SHA512_LEN

#define HAU_DMA_BLOCK_SIZE          32768  /* MAX: 65535 bytes, preferably 64-byte aligned */

/*!
 * \def MAX_BOOT_RECORD_SZ
 *
 * \brief Maximum size of the measured boot record.
 *
 * Its size can be calculated based on the following aspects:
 *   - There are 5 allowed software component claims,
 *   - SHA256 is used as the measurement method for the other claims.
 * Considering these aspects, the only claim which size can vary is the
 * type of the software component. In case of single image boot it is
 * "NSPE_SPE" which results the maximum boot record size of 96.
 */
//#define MAX_BOOT_RECORD_LEN  (96u)
/*
 * Image Status.
 */
#if 0
#define IMG_STATUS_NEW            0x1
#define IMG_STATUS_READY          0x2  /* The Image is verified by MBL, but not verified by the remote cloud server. */
#define IMG_STATUS_OK             0x3  /* The Image is verified by MBL and the remote cloud server. */
#define IMG_STATUS_BAD            0x4
#define IMG_STATUS_UNKNOWN        0x5
#endif

#define IMG_FLAG_NEWER_MASK       0x01
#define IMG_FLAG_VERIFY_MASK      0x06
#define IMG_FLAG_IA_MASK          0x18

#define IMG_FLAG_OLDER            0x0
#define IMG_FLAG_NEWER            0x1         /* The image with higher version will be set to NEWER.
                                                Default Newer after ISP. Set or Cleared when New image downloaded though OTA.
                                                Checked when MBL finding the boot image.
                                                Only one image is set to be NEWER at the same time. */

#define IMG_FLAG_VERIFY_NONE      (0x0 << 1)  /* Default None. Set after MBL verification finished. Checked when MBL finding the boot image. */
#define IMG_FLAG_VERIFY_OK        (0x1 << 1)
#define IMG_FLAG_VERIFY_FAIL      (0x2 << 1)
#define IMG_FLAG_IA_NONE          (0x0 << 3)  /* Default None. Set after IA finished. Checked when MBL finding the boot image. */
#define IMG_FLAG_IA_OK            (0x1 << 3)
#define IMG_FLAG_IA_FAIL          (0x2 << 3)

/*
* Image check error.
*/
enum img_validate_err_t {
    IMG_OK = 0,
    IMG_ERR_BAD_HEADER = -1,
    IMG_ERR_BAD_PTLV = -2,
    IMG_ERR_BAD_DIGEST = -3,
    IMG_ERR_BAD_PK = -4,
    IMG_ERR_BAD_SIG = -5,
    IMG_ERR_BAD_CERT_PK = -6,
    IMG_ERR_BAD_CERT = -7,
    IMG_ERR_MISSING_TLV = -8,
    IMG_ERR_READ_FLASH = -9,
    IMG_ERR_BAD_RECORD = -10,
};

/*
* Cert check error.
*/
enum cert_validate_err_t {
    CERT_OK = 0,
    CERT_ERR_BAD_ADDR = -1,
    CERT_ERR_BAD_SZ = -2,
    CERT_ERR_BAD_FORMAT = -3,
    CERT_ERR_BAD_SIG = -4,
    CERT_ERR_READ_FLASH = -5,
};

/** Image header.  All fields are in little endian byte order. Total 32 bytes. */
struct image_header {
    uint32_t magic_h;                /* Head Magic for boundary check. */
    uint32_t tot_sz;                 /* The total size of Image, including header, TLVs and the cert if existed. */
    uint8_t mani_ver;                /* The version of Image Manifest Format. */
    uint8_t img_type;                /* The type of Image (Firmware). */
    uint8_t algo_hash;               /* The Hash algorithm for Image digest */
    uint8_t algo_sign;               /* The algorithm used to sign Image manifest. */
    uint16_t hdr_sz;                 /* Size of Image Header (bytes). */
    uint16_t ptlv_sz;                /* Size of PTLVs (bytes). */
    uint32_t img_sz;                 /* Size of Image itself (bytes). */
    uint8_t ver_major;               /* Major Version. */
    uint8_t ver_minor;               /* Minor Version. */
    uint16_t ver_rev;                /* Revision. */
    uint32_t rsvd;                   /* Reserved. */
    uint32_t chksum;                 /* Header check sum. */
};

/** Image TLV header.  All fields in little endian. */
struct image_tlv_info {
    uint16_t magic_tlv;
    uint16_t tlv_sz;  /* size of TLV area (including tlv_info header) */
};

/** Image trailer TLV format. All fields in little endian. */
struct image_tlv {
    uint16_t type;   /* IMAGE_TLV_[...]. */
    uint16_t len;    /* Data length (not including TLV header). */
};

_Static_assert(sizeof(struct image_header) == IMG_HEADER_SIZE,
               "struct image_header not required size");

int digest_haudma_enable(uint32_t enable);
uint32_t cal_checksum(uint8_t *ptr, uint32_t sz);
int img_find(IN uint32_t start_faddr,
                IN uint8_t img_type,
                OUT uint32_t *img_faddr);
int img_verify_sign(IN uint8_t algo_sign,
                        IN uint8_t *pk,
                        IN size_t klen,
                        IN uint8_t *hash,
                        IN uint32_t hlen,
                        IN uint8_t *sig,
                        IN uint32_t slen);
int img_verify_digest(IN uint8_t algo_hash,
                        IN uint32_t faddr,  /* Flash Adress */
                        IN uint32_t len,
                        IN uint8_t *digest,
                        IN uint32_t diglen);
int img_verify_hdr(IN struct image_header *hdr,
                    IN uint8_t img_type);

int img_verify_pkhash(IN uint8_t *pk,
                IN uint32_t klen,
                IN uint8_t *pkhash,
                IN uint32_t hlen);

int img_validate(IN uint32_t img_faddr,
                   IN uint8_t img_type,
                   IN uint8_t *pkhash,
                   OUT void *img_info);

int cert_img_validate(IN uint32_t img_offset,
                   IN uint8_t img_type,
                   IN uint8_t *pkhash,
                   OUT void *img_info);

extern int ED25519_verify(const uint8_t *message, size_t message_len,
                   const uint8_t signature[64], const uint8_t public_key[32]);

#ifdef __cplusplus
}
#endif

#endif // __ROM_IMAGE_H__
