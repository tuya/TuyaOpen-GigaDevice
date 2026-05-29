#!/bin/bash
TOOLKIT=$1
ALGO_SIGN=$2
WITH_CERT=$3
OPENOCD_PATH=$4
ROOT=$5
AESK=$6

if [ -n "${OPENOCD_PATH}" ] && [ -x "${OPENOCD_PATH}/bin/${TOOLKIT}objcopy" ]; then
    TOOLKIT="${OPENOCD_PATH}/bin/${TOOLKIT}"
fi

ALGO_HASH=SHA256

TARGET=MSDK

if [[ ${ALGO_SIGN} != 'ECDSA256' && ${ALGO_SIGN}  != 'ED25519' ]]; then
    echo ALGO_SIGN must be 'ECDSA256' or 'ED25519'!
    exit 1
fi

if [[ ${ALGO_SIGN}  = 'ED25519' ]]; then
    KEY_PASSPHRASE=-P "12345678"
else
    KEY_PASSPHRASE=""
fi

if [[ ${AESK} != "" ]]; then
    AES_SUFFIX=-aes
else
    AES_SUFFIX=
fi
# echo ${AESK}

MBL_KEY=${ROOT}/scripts/certs/${ALGO_SIGN}/mbl-key.pem
ROTPK=${ROOT}/scripts/certs/${ALGO_SIGN}/rot-key.pem
MBL_CERT=${ROOT}/scripts/certs/${ALGO_SIGN}/mbl-cert.pem
CONFIG_FILE=${ROOT}/config/config_gdm32.h
IMGTOOL=${ROOT}/scripts/imgtool/imgtool.py
HEXTOOL=${ROOT}/scripts/imgtool/hextool.py
GENTOOL=${ROOT}/scripts/imgtool/gentool.py
AESTOOL=${ROOT}/scripts/imgtool/aestool.py
SREC_CAT=srec_cat
OUTPUT_PATH=${ROOT}/scripts/images
DOWNLOAD_BIN=${OUTPUT_PATH}/image-ota-sign${AES_SUFFIX}.bin

echo "${TARGET}.elf"
${TOOLKIT}objcopy -O binary -j ".log" "${TARGET}.elf" "trace.bin"
# ${TOOLKIT}objcopy -R ".log" "${TARGET}.elf" "${TARGET}.elf"


if [[ ${TOOLKIT} != "IAR" ]];then
    ${TOOLKIT}objdump -S -l -d ${TARGET}.elf > ${TARGET}.dump
    ${TOOLKIT}objcopy -O binary --remove-section ".log" ${TARGET}.elf ${TARGET}.bin
fi

if [[ -e ${OUTPUT_PATH}/image-ota.bin ]];then
    rm ${OUTPUT_PATH}/image-ota.bin
fi

if [[ -e ${DOWNLOAD_BIN} ]];then
    rm ${DOWNLOAD_BIN}
fi

# find RE_MBL_OFFSET defined in CONFIG_FILE

mbl_offset=$(awk '$2=="RE_MBL_OFFSET" {print $3}' ${CONFIG_FILE} )
image0_offset=$(awk '$2=="RE_IMG_0_OFFSET" {print $3}' ${CONFIG_FILE} )
image1_offset=$(awk '$2=="RE_IMG_1_OFFSET" {print $3}' ${CONFIG_FILE} )
echo mbl_offset=${mbl_offset} image0_offset=${image0_offset} image1_offset=${image1_offset}

rftest_on=$(cat ${ROOT}/MSDK/app/rftest_cfg.h | grep -c -E "^\#define CONFIG_RF_TEST_SUPPORT" )
# echo rftest_on=${rftest_on}

cur_dir=$PWD
cd ${OUTPUT_PATH}

mbl_len=0
if [[ -e MBL.bin ]]; then
    mbl_len=$(ls -l MBL.bin | awk '{print $5}')
fi
echo mbl_len = ${mbl_len}

rftest_len=0
if [[ -e rftest.bin ]]; then
    rftest_len=$(ls -l rftest.bin | awk '{print $5}')
fi

rftest_end=$(printf '%#X' $((0xA000 + ${rftest_len})))
# echo rftest_end = ${rftest_end}
cd ${cur_dir}

# Check if need python to add sysset/mbl_header/mbl_tailer
# if mbl_offset is equal to 0, which means boot from MBL directly (not from ROM)
if [[ ${mbl_offset} = "0x0" ]];then
    # echo "Not add image header and tailer!"
    if [[ ${rftest_on} = 1 ]];then
        cp ${TARGET}.bin "${OUTPUT_PATH}/rftest.bin"
        DOWNLOAD_BIN="${OUTPUT_PATH}/rftest.bin"
    else
        cp ${TARGET}.bin "${OUTPUT_PATH}/image-ota.bin"
        DOWNLOAD_BIN="${OUTPUT_PATH}/image-ota.bin"
    fi

    if [[ -e "${OUTPUT_PATH}/MBL.bin" ]]; then
        ${SREC_CAT} "${OUTPUT_PATH}/MBL.bin" -Binary -offset "0" \
                 ${TARGET}.bin -Binary -offset "${image0_offset}" \
                 -fill 0xFF ${mbl_len} "${image0_offset}" \
                 -o "${OUTPUT_PATH}/image-all.bin" -Binary
    fi

    if [[ -e "${OUTPUT_PATH}/MBL.bin" && -e "${OUTPUT_PATH}/rftest.bin" && -e "${OUTPUT_PATH}/image-ota.bin" ]]; then
        if [[ -e "${OUTPUT_PATH}/image-all-mp.bin" ]]; then
            rm "${OUTPUT_PATH}/image-all-mp.bin"
        fi
        ${SREC_CAT} "${OUTPUT_PATH}/MBL.bin" -Binary -offset "0" \
            "${OUTPUT_PATH}/rftest.bin" -Binary -offset "${image0_offset}" \
            "${OUTPUT_PATH}/image-ota.bin" -Binary -offset "${image1_offset}" \
            -fill 0xFF ${mbl_len} "${image0_offset}" \
            -fill 0xFF ${rftest_end} "${image1_offset}" \
            -o "${OUTPUT_PATH}/image-all-mp.bin" -Binary
        echo image-all-mp.bin generated!
    fi

else
    if [[ ${rftest_on} = 1 ]];then
        cp ${TARGET}.bin "${OUTPUT_PATH}/rftest.bin"
    else
        cp ${TARGET}.bin "${OUTPUT_PATH}/${TARGET}.bin"
    fi

    if [[ -e ${OUTPUT_PATH}/image-ota-sign.bin ]]; then
        rm ${OUTPUT_PATH}/image-ota-sign.bin
    fi

    if [[ ${WITH_CERT} = "CERT" ]]; then
        python ${IMGTOOL} sign --config ${CONFIG_FILE} \
                        -k ${MBL_KEY} \
                        ${KEY_PASSPHRASE} \
                        -t "IMG" \
                        --algo_hash "${ALGO_HASH}" \
                        --algo_sig "${ALGO_SIGN}" \
                        --cert ${MBL_CERT} \
                        --cert_key ${ROTPK} \
                        ${TARGET}.bin ${OUTPUT_PATH}/image-ota-sign.bin
    else
        python ${IMGTOOL} sign --config ${CONFIG_FILE} \
                        -k ${ROTPK} \
                        ${KEY_PASSPHRASE} \
                        -t "IMG" \
                        --algo_hash "${ALGO_HASH}" \
                        --algo_sig "${ALGO_SIGN}" \
                        ${TARGET}.bin ${OUTPUT_PATH}/image-ota-sign.bin
    fi

    if [[ "${AESK}" = "" ]]; then
        python ${HEXTOOL} -c ${CONFIG_FILE} \
                -t "IMG_0" \
                -e ${SREC_CAT} \
                ${OUTPUT_PATH}/image-ota-sign.bin \
                ${OUTPUT_PATH}/image-ota-sign.hex
    else
        python ${AESTOOL} --c ${CONFIG_FILE} \
                -t "IMG_0" \
                -i ${OUTPUT_PATH}/image-ota-sign.bin \
                -o ${OUTPUT_PATH}/image-ota-sign${AES_SUFFIX}.bin \
                -k ${AESK}
        echo Encrypted!
    fi

    python ${GENTOOL} --config ${CONFIG_FILE} \
                    --sys_set ${OUTPUT_PATH}/mbl-sys${AES_SUFFIX}.bin \
                    --img_0 ${OUTPUT_PATH}/image-ota-sign${AES_SUFFIX}.bin \
                    -o "${OUTPUT_PATH}/image-all-sign.bin"
fi

OPENOCD="${OPENOCD_PATH}/openocd"
LINKCFG="${cur_dir}/../openocd_gdlink.cfg"
#LINKCFG="${cur_dir}/../openocd_jlink.cfg"

#${OPENOCD} -f ${LINKCFG} -c "program ${DOWNLOAD_BIN} 0x0800A000 verify reset exit;"

# echo "Download OTA image use the follow command: "
# echo "${OPENOCD} -f ${LINKCFG} -c \"program ${DOWNLOAD_BIN} 0x0800A000 verify exit;\""
# echo "Or download ALL image:"
# echo "${OPENOCD} -f ${LINKCFG} -c \"program ${OUTPUT_PATH}/image-all.bin 0x08000000 verify exit;\""
