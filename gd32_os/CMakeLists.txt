cmake_minimum_required(VERSION 3.15)
set (CMAKE_C_COMPILER_FORCED true)
set (CMAKE_CXX_COMPILER_FORCED true)
project(GD32W55X LANGUAGES C ASM CXX)

set(CONFIG_OS                      "FREERTOS"    CACHE STRING    "Select which OS")
set(CONFIG_BLE_FEATURE             "MAX"         CACHE STRING    "Select which BLE CONFIG")
set(CONFIG_MBEDTLS_VERSION         "3.6.2"       CACHE STRING    "Select which mbedtls version")
# set(CONFIG_OS                      "RTTHREAD"    CACHE STRING    "Select which OS")
set(CONFIG_STATIC_LIB_BUILDED         ON            CACHE BOOL      "Whether static lib builded, like wlan, iperf3 lib")

include(${CMAKE_TOOLCHAIN_FILE})
toolchain_reload_compiler()

include_directories(
    config
    ROM-EXPORT/bootloader
    ROM-EXPORT/halcomm
    MSDK/plf/riscv/gd32vw55x
    MSDK/plf/riscv/NMSIS/Core/Include
    MSDK/plf/GD32VW55x_standard_peripheral/Include
    MSDK/plf/GD32VW55x_standard_peripheral
    MSDK/plf/src
    MSDK/macsw/export
    MSDK/app
    MSDK/app/play_music/mp3
    MSDK/app/play_music/codec_driver
    MSDK/util/include
)

include_directories(MBL/mainboot)
add_subdirectory(MBL/mainboot)

add_subdirectory(MSDK)
