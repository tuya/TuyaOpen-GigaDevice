
cmake_minimum_required(VERSION 3.15)

add_library(wifi_mgmt_api INTERFACE)

add_library(wifi_mgmt)

target_sources(wifi_mgmt
    PRIVATE
        wifi_eloop.c
        wifi_init.c
        wifi_management.c
        wifi_net_ip.c
        wifi_netlink.c
        wifi_vif.c
        wifi_wpa.c
)


target_include_directories(wifi_mgmt_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wpas>
)

target_link_libraries(wifi_mgmt
    wifi_mgmt_api
    lwIP_api
    rtos_api
    bsp_api
    mbedtls_api
    gd32vw55x_peripheral
    wifi_api
    lwIP
)

add_lib_api(wifi_mgmt_api)
add_lib(wifi_mgmt)