cmake_minimum_required(VERSION 3.15)

project(tuyaos_adapter LANGUAGES C CXX)

# Collect adapter sources (core system + drivers + utilities)
file(GLOB ADAPTER_CORE_SRC
    src/tkl_bluetooth.c
    src/tuya_ble_hal.c
    src/tkl_memory.c
    src/tkl_mutex.c
    src/tkl_network.c
    src/tkl_output.c
    src/tkl_queue.c
    src/tkl_semaphore.c
    src/tkl_sleep.c
    src/tkl_system.c
    src/tkl_thread.c
    src/tkl_ota.c
    src/tkl_fs.c
    src/tkl_wifi.c
    src/tkl_wifi_hostap.c
    src/tkl_gpio.c
    src/tkl_flash.c
    src/tkl_uart.c
    src/tkl_pwm.c
    src/tkl_timer.c
    src/tkl_watchdog.c
    src/tkl_wakeup.c
    src/tkl_pinmux.c
    src/tkl_adc.c
    src/tkl_i2c.c
    src/tkl_spi.c
    src/tkl_qspi.c
    src/tkl_rtc.c
    src/tkl_lwip.c
    src/tkl_hci.c
    src/tkl_hash.c
    src/tkl_asymmetrical.c
    src/tkl_symmetry.c
    src/tkl_display.c
)

file(GLOB INIT_SRC
    include/init/src/tkl_init_common.c
    include/init/src/tkl_init_system.c
)

if(OPEN_HEADER_DIR)
    string(REPLACE ";" " " HEADER_DIR_LIST "${OPEN_HEADER_DIR}")
    separate_arguments(HEADER_DIR_LIST)
    foreach(header_dir ${HEADER_DIR_LIST})
        include_directories(${header_dir})
    endforeach()
endif()

if(CONFIG_ENABLE_WIFI STREQUAL "y")
    list(APPEND INIT_SRC include/init/src/tkl_init_wifi.c)
endif()

if(CONFIG_ENABLE_WIRED STREQUAL "y")
    list(APPEND INIT_SRC include/init/src/tkl_init_wired.c)
endif()

set(ADAPTER_SOURCES
    ${ADAPTER_CORE_SRC}
    ${INIT_SRC}
    include/utilities/src/tuya_tools.c
    include/utilities/src/tuya_ringbuf.c
    include/utilities/src/tuya_queue.c
    include/utilities/src/tuya_list.c
    include/utilities/src/tuya_hashmap.c
    include/utilities/src/tuya_mem_heap.c
    include/utilities/src/tuya_smartpointer.c
)

add_library(tuyaos_adapter STATIC ${ADAPTER_SOURCES})

target_include_directories(tuyaos_adapter PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/adc
    ${CMAKE_CURRENT_LIST_DIR}/include/bluetooth
    ${CMAKE_CURRENT_LIST_DIR}/include/display
    ${CMAKE_CURRENT_LIST_DIR}/include/flash
    ${CMAKE_CURRENT_LIST_DIR}/include/gpio
    ${CMAKE_CURRENT_LIST_DIR}/include/hci
    ${CMAKE_CURRENT_LIST_DIR}/include/i2c
    ${CMAKE_CURRENT_LIST_DIR}/include/init/include
    ${CMAKE_CURRENT_LIST_DIR}/include/network
    ${CMAKE_CURRENT_LIST_DIR}/include/pinmux
    ${CMAKE_CURRENT_LIST_DIR}/include/pwm
    ${CMAKE_CURRENT_LIST_DIR}/include/qspi
    ${CMAKE_CURRENT_LIST_DIR}/include/rtc
    ${CMAKE_CURRENT_LIST_DIR}/include/security
    ${CMAKE_CURRENT_LIST_DIR}/include/spi
    ${CMAKE_CURRENT_LIST_DIR}/include/system
    ${CMAKE_CURRENT_LIST_DIR}/include/timer
    ${CMAKE_CURRENT_LIST_DIR}/include/uart
    ${CMAKE_CURRENT_LIST_DIR}/include/utilities/include
    ${CMAKE_CURRENT_LIST_DIR}/include/wakeup
    ${CMAKE_CURRENT_LIST_DIR}/include/watchdog
    ${CMAKE_CURRENT_LIST_DIR}/include/wifi
    ${CMAKE_CURRENT_LIST_DIR}/include/wired
    ${PLATFORM_PATH}/gd32_os/MSDK/rtos/rtos_wrapper
    ${PLATFORM_PATH}/gd32_os/ROM-EXPORT/bootloader
    ${PLATFORM_PATH}/gd32_os/ROM-EXPORT/halcomm
    ${PLATFORM_PATH}/gd32_os/MSDK/macsw/export
    ${PLATFORM_PATH}/gd32_os/MSDK/macsw/import
    ${PLATFORM_PATH}/gd32_os/MSDK/blesw/src/export
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/src
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/src/time
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/riscv/arch
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/riscv/arch/boot
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/riscv/arch/compiler
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/riscv/arch/ll
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/riscv/gd32vw55x
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/riscv/NMSIS/Core/Include
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/src/raw_flash
    ${PLATFORM_PATH}/gd32_os/MSDK/app
    ${PLATFORM_PATH}/gd32_os/MSDK/util/include
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/src/uart
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/GD32VW55x_standard_peripheral
    ${PLATFORM_PATH}/gd32_os/MSDK/plf/GD32VW55x_standard_peripheral/Include
    ${PLATFORM_PATH}/gd32_os/config
    ${PLATFORM_PATH}/gd32_os/MSDK/wifi_manager
    ${PLATFORM_PATH}/gd32_os/MSDK/lwip/lwip-2.2.0/src/include
    ${PLATFORM_PATH}/gd32_os/MSDK/lwip/lwip-2.2.0/port
    ${PLATFORM_PATH}/gd32_os/MSDK/ble/app
    ${PLATFORM_PATH}/gd32_os/MSDK/wifi_manager/wpas
)

target_compile_features(tuyaos_adapter PUBLIC c_std_99)

# Allow upper level to add extra defines via INTERFACE
target_compile_definitions(tuyaos_adapter PUBLIC TUYAOS_ADAPTER_BUILD)

# message(STATUS "tuyaos_adapter sources: ${ADAPTER_SOURCES}")
