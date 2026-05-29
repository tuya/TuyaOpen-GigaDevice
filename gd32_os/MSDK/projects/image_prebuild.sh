#!/bin/bash
TOOLKIT=$1
MSDK_DIR=$2
TOOLCHAIN_DIR=$3

# When invoked from CMake, resolve full path prefix for gcc so it works without PATH.
if [ -n "${TOOLCHAIN_DIR}" ] && [ -x "${TOOLCHAIN_DIR}/bin/${TOOLKIT}gcc" ]; then
    TOOLKIT="${TOOLCHAIN_DIR}/bin/${TOOLKIT}"
fi

APP_CONFIG=${MSDK_DIR}/app/app_cfg.h

# Copy ble lib and ble_config.h based on CONFIG_BLE_LIB setting
cfg_ble_lib_max=0
grep -qP "#define\s+CONFIG_BLE_LIB\s+BLE_LIB_MAX" "${APP_CONFIG}" && cfg_ble_lib_max=1
if [ "${cfg_ble_lib_max}" == "0" ]; then
    cp "${MSDK_DIR}/lib/libble_min.a" "${MSDK_DIR}/lib/libble.a"
    if ! diff -q "${MSDK_DIR}/blesw/src/export/config/ble_config.h" \
                 "${MSDK_DIR}/blesw/src/export/ble_config.h" > /dev/null 2>&1; then
        cp "${MSDK_DIR}/blesw/src/export/config/ble_config.h" \
           "${MSDK_DIR}/blesw/src/export/ble_config.h"
        touch "${MSDK_DIR}/blesw/src/export/ble_config.h"
    fi
else
    cp "${MSDK_DIR}/lib/libble_max.a" "${MSDK_DIR}/lib/libble.a"
    if ! diff -q "${MSDK_DIR}/blesw/src/export/config_max/ble_config.h" \
                 "${MSDK_DIR}/blesw/src/export/ble_config.h" > /dev/null 2>&1; then
        cp "${MSDK_DIR}/blesw/src/export/config_max/ble_config.h" \
           "${MSDK_DIR}/blesw/src/export/ble_config.h"
        touch "${MSDK_DIR}/blesw/src/export/ble_config.h"
    fi
fi

# Generate gd32vw55x.lds
if [ "${TOOLKIT}" != "IAR" ]; then
    "${TOOLKIT}gcc" -E -P \
        -o "${MSDK_DIR}/plf/riscv/env/gd32vw55x.lds" \
        -x c-header "${MSDK_DIR}/plf/riscv/env/gd32vw55x.ld" \
        -I "${MSDK_DIR}/../config" \
        -I "${MSDK_DIR}/../MBL/mainboot" \
        -I "${MSDK_DIR}/macsw/export" \
        -I "${MSDK_DIR}/util/include" \
        -I "${MSDK_DIR}/app"
fi

echo "/* Do not change the content here, it's auto generated */" > ${MSDK_DIR}/app/_build_date.h

DATE=$(date +"%Y/%m/%d-%T")
echo \#define SDK_BUILD_DATE  \"$DATE\" >> ${MSDK_DIR}/app/_build_date.h