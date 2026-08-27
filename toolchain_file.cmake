##
# @file toolchain_file.cmake
# @brief RISC-V cross toolchain (Nuclei GCC) for TuyaOpen's GD32 build.
#
# This is the file TuyaOpen's top-level CMakeLists.txt includes for every
# platform (see tools/porting/template/toolchain_file.cmake for the contract),
# and it serves that build only: compile TuyaOpen's sources into static
# libraries. That stage assembles nothing, compiles no C++, and links no
# executable.
#
# The GigaDevice SDK under gd32_os/ is a separate CMake project with its own
# toolchain needs; it is served by gd32_sdk_toolchain.cmake, which stands on
# its own and does not include this file. The two agree on -march/-mabi
# because the SDK links the libraries this stage produces - and disagreement
# is a hard link error ("can't link soft-float modules with single-float
# modules"), never a silent miscompile.
#/

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

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
# TuyaOpen includes this file directly rather than passing it as
# CMAKE_TOOLCHAIN_FILE, and sets PLATFORM_PATH beforehand; fall back to this
# file's own directory so a standalone `cmake -DCMAKE_TOOLCHAIN_FILE=...` works
# too. TOOLCHAIN_DIR stays overridable for a toolchain installed elsewhere.
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
set(COMPILE_PREX "${TOOLCHAIN_DIR}/bin/riscv-nuclei-elf-")

if(CMAKE_HOST_WIN32)
    set(_TC_EXE ".exe")
else()
    set(_TC_EXE "")
endif()

set(CMAKE_C_COMPILER   "${COMPILE_PREX}gcc${_TC_EXE}")
set(CMAKE_CXX_COMPILER "${COMPILE_PREX}g++${_TC_EXE}")
set(CMAKE_ASM_COMPILER "${COMPILE_PREX}gcc${_TC_EXE}")
set(CMAKE_AR           "${COMPILE_PREX}ar${_TC_EXE}")
set(CMAKE_RANLIB       "${COMPILE_PREX}ranlib${_TC_EXE}")
set(CMAKE_STRIP        "${COMPILE_PREX}strip${_TC_EXE}")

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

# .obj -> .o, matching what the SDK stage expects of the libraries it links
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_DIR}/set_extensions.cmake)

########################################
# Compiler flags
#
# -march/-mabi must match gd32_sdk_toolchain.cmake; see the header note.
########################################
set(CMAKE_C_FLAGS "-march=rv32imafcbp -mabi=ilp32f -mcmodel=medlow -msmall-data-limit=8 -msave-restore \
-Os -std=c99 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common \
-Wuninitialized -Wno-implicit-function-declaration")
