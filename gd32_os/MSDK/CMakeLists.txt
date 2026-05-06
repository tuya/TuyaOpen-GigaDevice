cmake_minimum_required(VERSION 3.15)

set(TARGET_EXE MSDK  CACHE STRING    "main target")

set(TARGET_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/projects/)

set_property(GLOBAL PROPERTY LIBSAPI)

macro (add_lib_api libapi)
    set_property(GLOBAL APPEND PROPERTY LIBSAPI "${libapi}")
endmacro()

macro (add_lib libname)
    set_property(GLOBAL APPEND PROPERTY LIBS "${libname}")
endmacro()

add_executable(${TARGET_EXE})

add_definitions(
    -DCFG_RTOS
)

if (CONFIG_BLE_FEATURE STREQUAL "MIN")
    set(ble_lib ble)
else ()
    set(ble_lib ble_max)
endif ()

add_compile_options (
    -fno-unroll-loops
    # -Werror
    -Wunused
    -Wall
    -Wno-format
    -Wno-unused-function
    -g
    -Os
    -Wno-pointer-sign
)

add_link_options(
    -fno-unroll-loops
    # -Werror
    -Wunused
    -Wuninitialized
    -Wall -Wno-format
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -Wno-pointer-sign
)

if (CONFIG_MBEDTLS_VERSION STREQUAL "2.17.0")
    include_directories(
        ${PROJECT_SOURCE_DIR}/ROM-EXPORT/mbedtls-2.17.0-rom/include
    )

    target_link_options(${TARGET_EXE}
        PUBLIC
        -Wl,--just-symbols=${PROJECT_SOURCE_DIR}/ROM-EXPORT/symbol/rom_symbol_m.gcc
    )
endif ()

add_subdirectory(blesw)
add_subdirectory(ble)
add_subdirectory(FatFS)
add_subdirectory(lwip)
add_subdirectory(macsw)
add_subdirectory(mbedtls)
add_subdirectory(plf)
add_subdirectory(rtos)
add_subdirectory(util)
add_subdirectory(wifi_manager)

if (APP)
    add_subdirectory(${APP})
else()
    add_subdirectory(app)
endif()

set_target_properties(${TARGET_EXE}
    PROPERTIES
        SUFFIX ".elf"
        ARCHIVE_OUTPUT_DIRECTORY "${TARGET_PROJECT_DIR}/cmake/output/lib"
        LIBRARY_OUTPUT_DIRECTORY "${TARGET_PROJECT_DIR}/cmake/output/lib"
        RUNTIME_OUTPUT_DIRECTORY "${TARGET_PROJECT_DIR}/cmake/output/bin"
    )

target_add_scatter_file(${TARGET_EXE}
    ${PROJECT_SOURCE_DIR}/MSDK/plf/riscv/env/gd32vw55x.ld
    )

target_link_directories(${TARGET_EXE}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/plf/riscv/NMSIS/Library/DSP/GCC
    )

get_property(list_lib GLOBAL PROPERTY LIBS)

message(STATUS "libs ${list_lib}")

get_property(list_lib_inc GLOBAL PROPERTY LIBSAPI)

# Detect CONFIG_MP3_PLAY_ENABLE from app_cfg.h.
# Matches only uncommented "#define CONFIG_MP3_PLAY_ENABLE" lines.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/app/app_cfg.h _mp3_cfg_lines
    REGEX "^[ \t]*#define[ \t]+CONFIG_MP3_PLAY_ENABLE")
if(_mp3_cfg_lines)
    set(CONFIG_MP3_PLAY_ENABLE ON)
    message(STATUS "CONFIG_MP3_PLAY_ENABLE detected in app_cfg.h — linking play_music")
endif()

target_link_libraries(${TARGET_EXE}
    PRIVATE
        ${list_lib_inc}
        -Wl,--start-group
        wifi
        rf
        rftest
        wpas
        $<$<BOOL:${CONFIG_MP3_PLAY_ENABLE}>:play_music>
        ${ble_lib}
        nmsis_dsp_rv32imafcbp
        m
        iperf3
        ${list_lib}
        -Wl,--end-group
    )

target_link_options(${TARGET_EXE}
    PUBLIC
    -Wl,-Map=${TARGET_PROJECT_DIR}/cmake/output/bin/${TARGET_EXE}.map
)

target_sources(${TARGET_EXE}
    PRIVATE
    _build_date.h
)

if(${CMAKE_HOST_WIN32})
    add_custom_command(OUTPUT _build_date.h
        COMMAND ${TARGET_PROJECT_DIR}/image_prebuild.bat ${CROSS_COMPILE}- ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_custom_command(TARGET ${TARGET_EXE} POST_BUILD
        COMMAND  cd ${TARGET_PROJECT_DIR}/cmake/output/bin && ${CMAKE_OBJDUMP} --source --all-headers --demangle --line-numbers --wide ${TARGET_EXE}.elf > ${TARGET_EXE}.lst
        COMMAND  cd ${TARGET_PROJECT_DIR}/cmake/output/bin && ${CMAKE_OBJCOPY} -O ihex  ${TARGET_EXE}.elf  ${TARGET_EXE}.hex
        COMMAND  cd ${TARGET_PROJECT_DIR}/cmake/output/bin && ${CMAKE_OBJCOPY} -O binary  ${TARGET_EXE}.elf  ${TARGET_EXE}.bin
        COMMAND  cd ${TARGET_PROJECT_DIR}/cmake/output/bin && ${TARGET_PROJECT_DIR}/image_afterbuild.bat  ${CROSS_COMPILE}- ECDSA256 CERT ${TOOLCHAIN_DIR} ${PROJECT_SOURCE_DIR} \"\" ${CONFIG_PROJECT_NAME}
        COMMENT "Building ${TARGET_EXE}"
    )
elseif(${CMAKE_HOST_UNIX})
    add_custom_command(OUTPUT _build_date.h
        COMMAND ${TARGET_PROJECT_DIR}/image_prebuild.sh ${CROSS_COMPILE}- ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_custom_command(TARGET ${TARGET_EXE} POST_BUILD
        COMMAND  cd ${TARGET_PROJECT_DIR}/cmake/output/bin && ${CMAKE_OBJDUMP} --source --all-headers --demangle --line-numbers --wide ${TARGET_EXE}.elf > ${TARGET_EXE}.lst
        COMMAND  cd ${TARGET_PROJECT_DIR}/cmake/output/bin && ${CMAKE_OBJCOPY} -O ihex  ${TARGET_EXE}.elf  ${TARGET_EXE}.hex
        COMMAND  cd ${TARGET_PROJECT_DIR}/cmake/output/bin && ${CMAKE_OBJCOPY} -O binary  ${TARGET_EXE}.elf  ${TARGET_EXE}.bin
        COMMAND  cd ${TARGET_PROJECT_DIR}/cmake/output/bin && ${TARGET_PROJECT_DIR}/image_afterbuild.sh  ${CROSS_COMPILE}- ECDSA256 CERT ${PROJECT_SOURCE_DIR}/tools/xpack-openocd-0.11.0-3_linux/bin ${PROJECT_SOURCE_DIR} \"\"
        COMMENT "Building ${TARGET_EXE}"
    )
else()
    message( FATAL_ERROR "only support Windows platform or Linux platform")
endif()
