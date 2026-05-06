##
# @file platform_config.cmake
#/

####################################################
# By configuring the variable PLATFORM_PUBINC,
# the header file in the platform is provided to TuyaOpen for use.
####################################################

list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuyaos/tuyaos_adapter)

set(PLATFORM_PUBINC_2
    ${PLATFORM_PATH}/gd32_os/MSDK/mbedtls/mbedtls/include
    ${PLATFORM_PATH}/gd32_os/MSDK/mbedtls/mbedtls/tests/include/spe
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/riscv/gd32vw55x
    ${PLATFORM_PATH}/gd32_os/config
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/riscv/NMSIS/Core/Include
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/GD32VW55x_standard_peripheral
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/GD32VW55x_standard_peripheral/Include
    ${PLATFORM_PATH}/gd32_os/MSDK/rtos/rtos_wrapper
    ${PLATFORM_PATH}/gd32_os/MSDK/app/play_music/mp3
    ${PLATFORM_PATH}/gd32_os/MSDK/app/play_music/codec_driver
    ${PLATFORM_PATH}/gd32_os/MSDK/app
)

set(PLATFORM_PUBINC
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})
