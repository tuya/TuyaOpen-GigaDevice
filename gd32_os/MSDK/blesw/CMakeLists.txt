cmake_minimum_required(VERSION 3.15)

add_library(gd32vw55x_ble_api INTERFACE)
if (CONFIG_BLE_FEATURE STREQUAL "MIN")
    set(ble_config_dir config)
else ()
    set(ble_config_dir config_max)
endif ()
target_include_directories(gd32vw55x_ble_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/export>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/export/${ble_config_dir}>
)

add_lib_api(gd32vw55x_ble_api)