
cmake_minimum_required(VERSION 3.15)

add_library(ble_app_profile_api INTERFACE)

add_library(ble_app_profile)

# 编译隔离：创建 vendor_ble_core 接口库，仅暴露 blesw 导出的 GAP 相关头，避免其他随机包含目录抢占同名 ble_gap.h。
add_library(vendor_ble_core INTERFACE)
if (CONFIG_BLE_FEATURE STREQUAL "MIN")
    set(_ble_cfg_dir config)
else()
    set(_ble_cfg_dir config_max)
endif()
target_include_directories(vendor_ble_core
    INTERFACE
        $<BUILD_INTERFACE:${PLATFORM_PATH}/gd32_os/MSDK/blesw/src/export>
        $<BUILD_INTERFACE:${PLATFORM_PATH}/gd32_os/MSDK/blesw/src/export/${_ble_cfg_dir}>
)

target_compile_options(ble_app_profile
    PRIVATE
        -w
)

file(GLOB_RECURSE SRC_BLE_APP  ${CMAKE_CURRENT_SOURCE_DIR}/app/*.c)
file(GLOB_RECURSE SRC_BLE_PROFILE  ${CMAKE_CURRENT_SOURCE_DIR}/profile/*.c)

MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

HEADER_DIRECTORIES(header_list)

list(REMOVE_ITEM SRC_BLE_APP "${CMAKE_CURRENT_SOURCE_DIR}/app/app_test_case.c")
list(REMOVE_ITEM SRC_BLE_APP "${CMAKE_CURRENT_SOURCE_DIR}/app/app_cmd_int.c")
list(REMOVE_ITEM SRC_BLE_APP "${CMAKE_CURRENT_SOURCE_DIR}/app/atcmd_ble.c")
list(REMOVE_ITEM SRC_BLE_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/profile/hogp/ble_hogpd.c")

target_sources(ble_app_profile
    PRIVATE
        ${SRC_BLE_APP}
        ${SRC_BLE_PROFILE}
)

target_include_directories(ble_app_profile
    PRIVATE
    ${header_list}               # 本模块自身头，仅 PRIVATE，不对外扩散
    ${CMAKE_CURRENT_SOURCE_DIR}/profile/sample   # 显式确保 ble_sample_srv.h 可见
    ${CMAKE_CURRENT_SOURCE_DIR}/profile/throughput  # 显式确保 ble_throughput_srv.h 可见
)

# 强化优先级：显式 BEFORE 添加 blesw 导出目录，使其在搜索顺序中早于 header_list 中其它目录（避免潜在同名覆盖）。
target_include_directories(ble_app_profile
    BEFORE PRIVATE
        ${PLATFORM_PATH}/gd32_os/MSDK/blesw/src/export
        ${PLATFORM_PATH}/gd32_os/MSDK/blesw/src/export/${_ble_cfg_dir}
)

# 确保 ble_app_profile 首先搜索 blesw 导出的 GAP 头文件（避免与 nimble 的同名 ble_gap.h 冲突）
# 不再直接把 blesw 导出目录作为 ble_app_profile PRIVATE include（改由 vendor_ble_core 接口库提供并可重复利用）。

target_include_directories(ble_app_profile_api
    BEFORE INTERFACE
        # 仅传播 app + profile API 头，不再扩散内部 header_list 全量目录；blesw 导出通过 vendor_ble_core 选择性传播。
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/app/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/>
    # 需要对外可见的各 profile 子目录（包含示例/服务头），防止 "ble_sample_srv.h" 等嵌套头缺失
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/sample>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/prox>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/dis>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/ota>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/bas>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/datatrans>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/cscs>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/throughput>
)

target_link_libraries(ble_app_profile
    PRIVATE
    ble_app_profile_api
    vendor_ble_core          # 通过该接口库获取 blesw 导出头 (ble_gap.h)
        bsp_api
        riscv_api
        util_api
        rtos_api
        wifi_mgmt_api
        lwIP_api
        wifi_api
        mbedtls_api
        gd32vw55x_ble_api
)

add_lib_api(ble_app_profile_api)
add_lib(ble_app_profile)
