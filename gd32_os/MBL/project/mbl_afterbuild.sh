#!/bin/bash
TOOLKIT=$1
ROOT=$2
ALGO_SIGN=$3
WITH_CERT=$4
OPENOCD_PATH=$5
AESK=$6

ALGO_HASH=SHA256
TARGET=MBL

if [ -n "${OPENOCD_PATH}" ] && [ -x "${OPENOCD_PATH}/bin/${TOOLKIT}objcopy" ]; then
    TOOLKIT="${OPENOCD_PATH}/bin/${TOOLKIT}"
fi

if [[ ${ALGO_SIGN} != "ECDSA256" &&  ${ALGO_SIGN} != "ED25519" ]]; then
    echo ALGO_SIGN must be "ECDSA256" or "ED25519"!
    exit 1
fi

if [[ ${ALGO_SIGN} = "ED25519" ]]; then
    KEY_PASSPHRASE=-P "12345678"
else
    KEY_PASSPHRASE=""
fi

if [[ ${AESK} != "" ]]; then
    AES_SUFFIX=-aes
else
    AES_SUFFIX=""
fi

MBL_KEY=${ROOT}/scripts/certs/${ALGO_SIGN}/mbl-key.pem
ROTPK=${ROOT}/scripts/certs/${ALGO_SIGN}/rot-key.pem
MBL_CERT=${ROOT}/scripts/certs/${ALGO_SIGN}/mbl-cert.pem
CONFIG_FILE=${ROOT}/config/config_gdm32.h
SYSTOOL=${ROOT}/scripts/imgtool/sysset.py
IMGTOOL=${ROOT}/scripts/imgtool/imgtool.py
HEXTOOL=${ROOT}/scripts/imgtool/hextool.py
GENTOOL=${ROOT}/scripts/imgtool/gentool.py
AESTOOL=${ROOT}/scripts/imgtool/aestool.py
SREC_CAT=${ROOT}/scripts/imgtool/srec_cat.exe
OUTPUT_PATH=${ROOT}/scripts/images
DOWNLOAD_BIN=${OUTPUT_PATH}/mbl-sys${AES_SUFFIX}.bin
if [ ! -d "${OUTPUT_PATH}" ]; then
    mkdir -p "${OUTPUT_PATH}"
fi

# Generate dump and bin file
if [[ ${TOOLKIT} != "IAR" ]];then
    ${TOOLKIT}objdump -d ${TARGET}.elf >  ${TARGET}.dump
    ${TOOLKIT}objcopy -O binary ${TARGET}.elf  ${TARGET}.bin
fi

if [[ -e ${OUTPUT_PATH}/${TARGET}.bin ]]; then
    rm ${OUTPUT_PATH}/${TARGET}.bin
fi

if [[ -e ${DOWNLOAD_BIN} ]]; then
    rm ${DOWNLOAD_BIN}
fi


# find RE_MBL_OFFdefined in CONFIG_FILE
mbl_offset=0x0
mbl_offset=$(awk '$2=="RE_MBL_OFFSET" {print $3}' ${ROOT}/config/config_gdm32.h )

# Check if need python to add sysset/mbl header/mbl tailer
if [[ ${mbl_offset} = "0x0" ]]; then
    echo "Not add image header and tailer, goto download!"
    cp ${TARGET}.bin ${OUTPUT_PATH}/${TARGET}.bin
    DOWNLOAD_BIN=${TARGET}.bin

else
    # Print ROTPK HASH
    #python ${IMGTOOL} getpub -k ${ROTPK}  ${KEY_PASSPHRASE}  --sha256 1

    # Generate system setting hex
    python ${SYSTOOL} -t "SYS_SET" -c ${CONFIG_FILE} ${OUTPUT_PATH}/sysset.bin

    # Generate system status hex (padding with 0xFF)
    # python ${SYSTOOL} -t "SYS_STATUS" -c ${CONFIG_FILE}  ${OUTPUT_PATH}/sysstatus.bin

    if [[ -e ${OUTPUT_PATH}/mbl-sign.bin ]]; then
        rm ${OUTPUT_PATH}/mbl-sign.bin
    fi

    # Add image header, ptlvs and concatenate the cert
    cp ${TARGET}.bin ${OUTPUT_PATH}/${TARGET}.bin
    if [[ ${WITH_CERT} = "CERT" ]];then
        python ${IMGTOOL} sign --config ${CONFIG_FILE} \
                        -k ${MBL_KEY} \
                        ${KEY_PASSPHRASE} \
                        -t "MBL" \
                        --algo_hash "${ALGO_HASH}" \
                        --algo_sig "${ALGO_SIGN}" \
                        --cert ${MBL_CERT} \
                        --cert_key ${ROTPK} \
                        ${TARGET}.bin ${OUTPUT_PATH}/mbl-sign.bin
    else
        python ${IMGTOOL} sign --config ${CONFIG_FILE} \
                        -k ${ROTPK} \
                        ${KEY_PASSPHRASE} \
                        -t "MBL" \
                        --algo_hash "${ALGO_HASH}" \
                        --algo_sig "${ALGO_SIGN}" \
                        ${TARGET}.bin ${OUTPUT_PATH}/mbl-sign.bin
    fi

    python ${GENTOOL} --config ${CONFIG_FILE} \
                    --sys_set ${OUTPUT_PATH}/sysset.bin \
                    --mbl ${OUTPUT_PATH}/mbl-sign.bin \
                    -o ${OUTPUT_PATH}/mbl-sys.bin
    if [[ -e ${OUTPUT_PATH}/sysset.bin ]]; then
        rm ${OUTPUT_PATH}/sysset.bin
    fi

    if [[ -e ${OUTPUT_PATH}/mbl-sign.bin ]]; then
        rm ${OUTPUT_PATH}/mbl-sign.bin
    fi

    if [[ ${AESK} = "" ]]; then
        python ${HEXTOOL} -c ${CONFIG_FILE} \
                -t "SYS_SET" \
                -e ${SREC_CAT} \
                ${OUTPUT_PATH}/mbl-sys.bin \
                ${OUTPUT_PATH}/mbl-sys.hex
    else
        python ${IMGTOOL} pad -s 0x8000 \
                            ${OUTPUT_PATH}/mbl-sys.bin ${OUTPUT_PATH}/mbl-sys-pad.bin
        python ${AESTOOL} --c ${CONFIG_FILE}   \
                -t "SYS_SET" \
                -i ${OUTPUT_PATH}/mbl-sys-pad.bin \
                -o ${OUTPUT_PATH}/mbl-sys${AES_SUFFIX}.bin \
                -k ${AESK}
#        python ${HEXTOOL} -c ${CONFIG_FILE} \
#                -t "SYS_SET" \
#                -e ${SREC_CAT} \
#                ${OUTPUT_PATH}/mbl-sys${AES_SUFFIX}.bin \
#                ${OUTPUT_PATH}/mbl-sys.hex
        rm ${OUTPUT_PATH}/mbl-sys-pad.bin
        echo Encrypted!
    fi
fi

OPENOCD="${OPENOCD_PATH}/openocd"
LINKCFG="$PWD/../openocd_gdlink.cfg"
#LINKCFG="$PWD/../openocd_jlink.cfg"

#${OPENOCD} -f ${LINKCFG} -c "program ${DOWNLOAD_BIN} 0x08000000 verify exit;"
echo "Download image use the follow command: "
echo "${OPENOCD} -f ${LINKCFG} -c \"program ${OUTPUT_PATH}/${DOWNLOAD_BIN} 0x08000000 verify exit;\""
