##
# @file gd32_sdk_toolchain.cmake
# @brief Toolchain for the GigaDevice GD32VW55x SDK build.
#
# gd32_os/ is GigaDevice's own CMake project (140-odd CMakeLists, one of the
# three project flavours the SDK ships alongside eclipse/ and segger/), and
# this is its toolchain: assembly and C++ flags, link flags, linker-script
# preprocessing, and the binutils its post-build scripts reach for.
#
# Deliberately self-contained. None of this belongs in toolchain_file.cmake -
# that file implements TuyaOpen's porting contract and serves TuyaOpen's own
# library build only. Both files pick the same -march/-mabi because the SDK
# links the libraries TuyaOpen produces; should they ever drift, ld refuses
# the link outright ("can't link soft-float modules with single-float
# modules"), so the duplication cannot turn into a silent ABI mismatch.
#
# Included by:
#   platform/GD32/CMakeLists.txt   the wrapper project that builds the SDK
#   gd32_os/CMakeLists.txt         the SDK itself (its own project() scope)
#
# Public surface used elsewhere in the tree - grep before renaming:
#   TOOLCHAIN_DIR, CROSS_COMPILE   gd32_os/MBL/mainboot, gd32_os/MSDK post-build scripts
#   toolchain_reload_compiler()    both includers above
#   target_add_scatter_file()      gd32_os/MBL/mainboot, gd32_os/MSDK
#/

########################################
# Target system
########################################
set(SYSTEM_PROCESSOR    riscv)
set(SYSTEM_ARCHITECTURE rv32imafcbp)
set(SYSTEM_ABI          ilp32f)
set(SYSTEM_FP           ON)

set(CMAKE_SYSTEM_NAME         Generic)
set(CMAKE_SYSTEM_PROCESSOR    ${SYSTEM_PROCESSOR})
set(CMAKE_SYSTEM_ARCHITECTURE ${SYSTEM_ARCHITECTURE})

# The prebuilt Nuclei GCC ships bfd-plugins that make CMake's own try-compile
# probe fail on Windows. Declare the compilers working and keep the probe to a
# static library so it never reaches the linker.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_COMPILER_WORKS   1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)

########################################
# Toolchain location
#
# This file is included before project(), i.e. before CMake has processed
# CMAKE_TOOLCHAIN_FILE, so it resolves the toolchain itself rather than
# inheriting anything. PLATFORM_PATH falls back to this file's own directory.
# TOOLCHAIN_DIR and CROSS_COMPILE are read by the vendor post-build scripts.
########################################
if(NOT DEFINED PLATFORM_PATH OR "${PLATFORM_PATH}" STREQUAL "")
    set(PLATFORM_PATH "${CMAKE_CURRENT_LIST_DIR}")
endif()

# platform_prepare.py unpacks the toolchain into platform/tools/<folder>/gcc,
# and the package - hence the folder - differs per host; keep the names in step
# with TOOLCHAIN_PACKAGES in tools/download_toolchain.py.
if(CMAKE_HOST_WIN32)
    set(_TC_FOLDER "nuclei_riscv_newlibc_prebuilt_win32_2022.04")
else()
    set(_TC_FOLDER "nuclei_riscv_newlibc_prebuilt_linux64_2022.04")
endif()
set(TOOLCHAIN_DIR "${PLATFORM_PATH}/../tools/${_TC_FOLDER}/gcc"
    CACHE PATH "RISC-V Nuclei GCC toolchain root")
set(CROSS_COMPILE riscv-nuclei-elf)
set(TOOLCHAIN_PRE "${CROSS_COMPILE}-")

if(CMAKE_HOST_WIN32)
    set(_TC_EXE ".exe")
else()
    set(_TC_EXE "")
endif()

set(CMAKE_C_COMPILER   "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}gcc${_TC_EXE}")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}g++${_TC_EXE}")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}gcc${_TC_EXE}")
set(CMAKE_AR           "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}ar${_TC_EXE}")
set(CMAKE_RANLIB       "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}ranlib${_TC_EXE}")
set(CMAKE_STRIP        "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}strip${_TC_EXE}")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}objcopy${_TC_EXE}")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}objdump${_TC_EXE}")
set(CMAKE_SIZE         "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}size${_TC_EXE}")

# A missing toolchain otherwise surfaces as a wall of "command not found" from
# ninja; say it once, up front, and point at the script that installs it.
if(NOT EXISTS "${CMAKE_C_COMPILER}")
    message(FATAL_ERROR
        "GD32 toolchain not found: ${CMAKE_C_COMPILER}\n"
        "Run 'python platform_prepare.py' in platform/GD32 to download it, "
        "or pass -DTOOLCHAIN_DIR=<gcc root>.")
endif()

set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR}/bin)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# .obj -> .o, so the post-build scripts and the scatter rules find their inputs
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_DIR}/set_extensions.cmake)

########################################
# Compiler flags
#
# Kept as lists rather than strings: toolchain_reset_compiler_flags() feeds
# them to add_compile_options() one at a time behind a COMPILE_LANGUAGE
# generator expression, and a semicolon-joined string would reach the compiler
# as a single argument.
########################################
set(TOOLCHAIN_MACHINE_FLAG_LIST
    -mcmodel=medlow
    -msmall-data-limit=8
    -msave-restore
    -mabi=${SYSTEM_ABI}
)

set(TOOLCHAIN_COMMON_FLAG_LIST
    ${TOOLCHAIN_MACHINE_FLAG_LIST}
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections    # with --gc-sections at link time, drops unreferenced code
    -fdata-sections
    -fno-common
    -Wuninitialized
)

set(TOOLCHAIN_C_FLAG_LIST
    ${TOOLCHAIN_COMMON_FLAG_LIST}
    -std=c99
    -Os
    # the vendor SDK calls a handful of functions ahead of their declaration
    -Wno-implicit-function-declaration
    -march=${SYSTEM_ARCHITECTURE}
)

set(TOOLCHAIN_CXX_FLAG_LIST
    ${TOOLCHAIN_COMMON_FLAG_LIST}
    -std=gnu++17
    # no C++ runtime on this target: no exceptions, no RTTI, no static
    # destructor registration, and no unwind tables to carry them
    -fno-exceptions
    -fno-rtti
    -fno-use-cxa-atexit
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -fpermissive
    -fno-jump-tables
    -fno-tree-switch-conversion
    -Wno-register
    -march=${SYSTEM_ARCHITECTURE}
)

set(TOOLCHAIN_ASM_FLAG_LIST
    ${TOOLCHAIN_COMMON_FLAG_LIST}
    -x assembler-with-cpp
    -Os
    -march=${SYSTEM_ARCHITECTURE}
)

set(TOOLCHAIN_LINK_FLAG_LIST
    ${TOOLCHAIN_MACHINE_FLAG_LIST}
    -Os
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections
    -fno-common
    -nostartfiles          # start.S in the SDK provides the reset entry
    -fno-exceptions
    -fno-rtti
    -fno-lto
    -Xlinker --gc-sections
    --specs=nano.specs
    --specs=nosys.specs
    -Wuninitialized
    LINKER:--print-memory-usage
    LINKER:-check-sections
    -march=${SYSTEM_ARCHITECTURE}
)

########################################
# Macros
#
# gd32_os/CMakeLists.txt starts its own project() and therefore its own
# directory scope, which inherits none of the parent's compile options; it
# calls toolchain_reload_compiler() to re-apply them. The reset_* macros clear
# the directory property first so a second call does not double every flag.
########################################
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

macro(toolchain_reset_linker_flags)
    set_property(DIRECTORY PROPERTY LINK_OPTIONS "")
    add_link_options(${TOOLCHAIN_LINK_FLAG_LIST})
endmacro()

macro(toolchain_set_processor_arch)
    set(CMAKE_SYSTEM_PROCESSOR ${SYSTEM_PROCESSOR})
    set(CMAKE_SYSTEM_ARCHITECTURE ${SYSTEM_ARCHITECTURE})
endmacro()

macro(toolchain_reload_compiler)
    toolchain_set_processor_arch()
    toolchain_reset_compiler_flags()
    toolchain_reset_linker_flags()
endmacro()

toolchain_reload_compiler()

########################################
# Scatter (linker script) support
#
# The SDK's .ld files are C preprocessor input: they pull in the memory layout
# headers. Build each one as an OBJECT library compiled with -E -P so the
# preprocessed script becomes the target's -T argument.
########################################
macro(target_add_scatter_file target)
    add_library(${target}_scatter OBJECT)
    target_compile_options(${target}_scatter PRIVATE -E -P -xc)
    foreach(scatter_file ${ARGN})
        target_sources(${target}_scatter PRIVATE ${scatter_file})
        # strip any generator expression wrapper to reach the real path, which
        # has to be marked as C for the -xc preprocess run above to apply
        string(REGEX REPLACE ".*>:(.*)>$" "\\1" SCATTER_FILE_PATH "${scatter_file}")
        set_source_files_properties(${SCATTER_FILE_PATH} PROPERTIES LANGUAGE C)
    endforeach()
    target_link_options(${target} PRIVATE -T $<TARGET_OBJECTS:${target}_scatter>)
    add_dependencies(${target} ${target}_scatter)
endmacro()
