cmake_minimum_required(VERSION 3.15)

add_library(fatfs_api INTERFACE)
add_library(fatfs)

set(FATFS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# FatFS source files
set(fatfs_SRCS
    ${FATFS_DIR}/src/ff.c
    ${FATFS_DIR}/src/ffunicode.c
    ${FATFS_DIR}/src/ffsystem.c
    ${FATFS_DIR}/src/diskio.c
)

# FatFS port files
set(fatfs_port_SRCS
    ${FATFS_DIR}/port/fatfs.c
)

target_sources(fatfs
    PRIVATE
        ${fatfs_SRCS}
        ${fatfs_port_SRCS}
)

target_include_directories(fatfs_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/port>
)

target_link_libraries(fatfs
    fatfs_api
    rtos_api
    bsp_api
)

add_lib_api(fatfs_api)
add_lib(fatfs)
