##
# @file toolchain_file.cmake
#/

####################################################
# Modify the content of this file
# according to the actual situation
# and configure the actual path of the compilation tool
####################################################
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

# 跳过编译器检查，避免 Windows 上 bfd-plugins 错误
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER_WORKS 1)
SET(CMAKE_ASM_COMPILER_WORKS 1)

# Toolchain file runs before platform CMakeLists sets PLATFORM_PATH; derive root from this file's directory.
if(NOT DEFINED PLATFORM_PATH OR "${PLATFORM_PATH}" STREQUAL "")
    set(PLATFORM_PATH "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "GD32 platform root (directory containing toolchain_file.cmake)")
endif()

set(TOOLCHAIN_DIR "${PLATFORM_PATH}/prebuilt/nuclei_riscv_newlibc_prebuilt_win32_2022.04/gcc" CACHE PATH "RISC-V Nuclei GCC toolchain root")
set(CROSS_COMPILE riscv-nuclei-elf)
set(TOOLCHAIN_PRE "${CROSS_COMPILE}-")

if(WIN32)
    set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}gcc.exe")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}g++.exe")
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}gcc.exe")
    set(CMAKE_AR "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}ar.exe")
    set(CMAKE_RANLIB "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}ranlib.exe")
    set(CMAKE_STRIP "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}strip.exe")
    set(CMAKE_OBJCOPY "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}objcopy.exe")
    set(CMAKE_OBJDUMP "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}objdump.exe")
else()
    set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}gcc")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}g++")
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}gcc")
    set(CMAKE_AR "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}ar")
    set(CMAKE_RANLIB "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}ranlib")
    set(CMAKE_STRIP "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}strip")
    set(CMAKE_OBJCOPY "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}objcopy")
    set(CMAKE_OBJDUMP "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}objdump")
endif()

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_DIR}/set_extensions.cmake)

set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR}/bin)

set(SYSTEM_PROCESSOR riscv)
set(SYSTEM_ARCHITECTURE rv32imafcbp)
set(SYSTEM_FP ON)
set(CMAKE_SYSTEM_PROCESSOR ${SYSTEM_PROCESSOR})
set(CMAKE_SYSTEM_ARCHITECTURE ${SYSTEM_ARCHITECTURE})

if (CONFIG_OS STREQUAL "FREERTOS")
    set(PLATFORM_OS_FREERTOS ON)
    add_definitions(
        -DPLATFORM_OS_FREERTOS
    )
elseif (CONFIG_OS STREQUAL "RTTHREAD")
    set(PLATFORM_OS_RTTHREAD ON)
    add_definitions(
        -DPLATFORM_OS_RTTHREAD
    )
endif()

    add_definitions(-DCONFIG_MBEDTLS_VERSION="3.6.2")
    add_definitions(-DTUYAOS_SUPPORT)

## 使用字符串而非列表，避免生成表达式时出现分号拼接成单一参数
set(TOOLCHAIN_C_FLAG_LIST
    -mcmodel=medlow
    -msmall-data-limit=8
    -msave-restore
    -mabi=ilp32f
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wuninitialized
    -std=c99
    -Os
    -Wno-implicit-function-declaration
)

set(TOOLCHAIN_CXX_FLAG_LIST
    -mcmodel=medlow
    -msmall-data-limit=8
    -msave-restore
    -mabi=ilp32f
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wuninitialized
    -std=gnu++17
    -fno-exceptions
    -fno-rtti
    -fno-use-cxa-atexit
    -fpermissive
    -fno-jump-tables
    -fno-tree-switch-conversion
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -Wno-register
)
set(TOOLCHAIN_ASM_FLAG_LIST
    -mcmodel=medlow
    -msmall-data-limit=8
    -msave-restore
    -mabi=ilp32f
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wuninitialized
    -x assembler-with-cpp
    -Os
)

macro(toolchain_reset_compiler_flags)
    set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "")
    foreach(f ${TOOLCHAIN_C_FLAG_LIST})
        add_compile_options($<$<COMPILE_LANGUAGE:C>:${f}>)
    endforeach()
    foreach(f ${TOOLCHAIN_ASM_FLAG_LIST})
        add_compile_options($<$<COMPILE_LANGUAGE:ASM>:${f}>)
    endforeach()
    foreach(f ${TOOLCHAIN_CXX_FLAG_LIST})
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${f}>)
    endforeach()
endmacro()

######################################

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

macro(toolchain_reset_linker_flags)
    set_property(DIRECTORY PROPERTY LINK_OPTIONS "")
    add_link_options(
        -mcmodel=medlow
        -msmall-data-limit=8
        -msave-restore
        -mabi=ilp32f
        -Os
        -fmessage-length=0
        -fsigned-char
        -ffunction-sections
        -fdata-sections
        -fno-common
        -nostartfiles
        -fno-exceptions
        -fno-rtti
        -fno-lto
        -Xlinker --gc-sections
        --specs=nano.specs
        --specs=nosys.specs
        -Wuninitialized
        LINKER:--print-memory-usage
        LINKER:-check-sections
    )
endmacro()

macro(toolchain_set_processor_arch)
    set(CMAKE_SYSTEM_PROCESSOR ${SYSTEM_PROCESSOR})
    set(CMAKE_SYSTEM_ARCHITECTURE ${SYSTEM_ARCHITECTURE})
endmacro()

macro(toolchain_reload_compiler)
    toolchain_set_processor_arch()
    toolchain_reset_compiler_flags()
    toolchain_reset_linker_flags()
    # 追加 -march，不覆盖已有 flags
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:-march=${SYSTEM_ARCHITECTURE}>"
                        "$<$<COMPILE_LANGUAGE:CXX>:-march=${SYSTEM_ARCHITECTURE}>"
                        "$<$<COMPILE_LANGUAGE:ASM>:-march=${SYSTEM_ARCHITECTURE}>")
    # 链接阶段同样追加一次
    add_link_options(-march=${SYSTEM_ARCHITECTURE})
    # ABI 防御（列表无法直接替换，判断后追加替换逻辑）
    foreach(listvar TOOLCHAIN_C_FLAG_LIST TOOLCHAIN_CXX_FLAG_LIST TOOLCHAIN_ASM_FLAG_LIST)
        set(_newList "")
        foreach(item ${${listvar}})
            if(item MATCHES "-mabi=lp64f")
                set(item "-mabi=ilp32f")
            elseif(item MATCHES "-mabi=lp64")
                set(item "-mabi=ilp32f")
            endif()
            list(APPEND _newList ${item})
        endforeach()
        set(${listvar} ${_newList})
    endforeach()
endmacro()

toolchain_reload_compiler()

macro(target_add_scatter_file target)
    # message(STATUS "[scatter] target_add_scatter_file: target='${target}' files='${ARGN}'")
    target_link_options(${target} PRIVATE -T $<TARGET_OBJECTS:${target}_scatter>)
    add_dependencies(${target} ${target}_scatter)
    add_library(${target}_scatter OBJECT)
    foreach(scatter_file ${ARGN})
        target_sources(${target}_scatter PRIVATE ${scatter_file})
        # 解析原始文件路径，设置为 C 语言以便预处理
        string(REGEX REPLACE ".*>:(.*)>$" "\\1" SCATTER_FILE_PATH "${scatter_file}")
        set_source_files_properties(${SCATTER_FILE_PATH} PROPERTIES LANGUAGE C)
    endforeach()
    target_compile_options(${target}_scatter PRIVATE -E -P -xc)
endmacro()
