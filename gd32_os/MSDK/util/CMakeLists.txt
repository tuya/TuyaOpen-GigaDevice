
cmake_minimum_required(VERSION 3.15)

add_library(util_api INTERFACE)

add_library(util)

target_sources(util
    PRIVATE
        src/aes_ecb.c
        # src/cJSON.c
        src/crc.c
        src/cyclic_buffer.c
        src/debug_print.c
        src/dlist.c
        src/slist.c
        src/trace_ext.c
        src/user_setting.c
        src/util.c
)


target_include_directories(util_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(util
    rtos_api
    util_api
    riscv_api
    mbedtls_api
    bsp_api
)

add_lib_api(util_api)
add_lib(util)