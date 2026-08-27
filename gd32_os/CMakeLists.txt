cmake_minimum_required(VERSION 3.15)
set (CMAKE_C_COMPILER_FORCED true)
set (CMAKE_CXX_COMPILER_FORCED true)
project(GD32W55X LANGUAGES C ASM CXX)

set(CONFIG_OS                      "FREERTOS"    CACHE STRING    "Select which OS")
set(CONFIG_BLE_FEATURE             "MAX"         CACHE STRING    "Select which BLE CONFIG")
set(CONFIG_MBEDTLS_VERSION         "3.6.2"       CACHE STRING    "Select which mbedtls version")
# set(CONFIG_OS                      "RTTHREAD"    CACHE STRING    "Select which OS")
set(CONFIG_STATIC_LIB_BUILDED         ON            CACHE BOOL      "Whether static lib builded, like wlan, iperf3 lib")

# Own project() means own directory scope: re-apply the SDK toolchain flags.
include(${PLATFORM_PATH}/gd32_sdk_toolchain.cmake)
toolchain_reload_compiler()

# CONFIG_OS is only known here (set as a cache entry a few lines up), which is
# why this lives in the SDK subtree rather than in the toolchain file.
if(CONFIG_OS STREQUAL "FREERTOS")
    set(PLATFORM_OS_FREERTOS ON)
    add_compile_definitions(PLATFORM_OS_FREERTOS)
elseif(CONFIG_OS STREQUAL "RTTHREAD")
    set(PLATFORM_OS_RTTHREAD ON)
    add_compile_definitions(PLATFORM_OS_RTTHREAD)
endif()

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
    MSDK/util/include
)

include_directories(MBL/mainboot)
add_subdirectory(MBL/mainboot)

add_subdirectory(MSDK)
