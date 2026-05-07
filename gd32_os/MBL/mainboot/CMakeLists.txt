
cmake_minimum_required(VERSION 3.15)
set(TARGET_EXE MBL)
set(TARGET_PROJECT_DIR ${PROJECT_SOURCE_DIR}/MBL/project)

add_definitions(
    -DEXEC_USING_STD_PRINTF
)

add_compile_options (
    -O2
    -g
)

add_executable(${TARGET_EXE})

target_include_directories(${TARGET_EXE}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/ROM-EXPORT/mbedtls-2.17.0-rom/include
)

set_target_properties(${TARGET_EXE}
    PROPERTIES
        SUFFIX ".elf"
        ARCHIVE_OUTPUT_DIRECTORY "${TARGET_PROJECT_DIR}/cmake/lib"
        LIBRARY_OUTPUT_DIRECTORY "${TARGET_PROJECT_DIR}/cmake/lib"
        RUNTIME_OUTPUT_DIRECTORY "${TARGET_PROJECT_DIR}/cmake/bin"
)

target_sources(${TARGET_EXE}
    PRIVATE
        mbl.c
        mbl_image_validate.c
    )

target_sources(${TARGET_EXE}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/MSDK/plf/GD32VW55x_standard_peripheral/Source/gd32vw55x_eclic.c
        ${PROJECT_SOURCE_DIR}/MSDK/plf/GD32VW55x_standard_peripheral/Source/gd32vw55x_fmc.c
        ${PROJECT_SOURCE_DIR}/MSDK/plf/GD32VW55x_standard_peripheral/Source/gd32vw55x_gpio.c
        ${PROJECT_SOURCE_DIR}/MSDK/plf/GD32VW55x_standard_peripheral/Source/gd32vw55x_rcu.c
        ${PROJECT_SOURCE_DIR}/MSDK/plf/GD32VW55x_standard_peripheral/Source/gd32vw55x_usart.c
    )

target_sources(${TARGET_EXE}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/MSDK/plf/riscv/env/handlers.c
        ${PROJECT_SOURCE_DIR}/MSDK/plf/riscv/env/env_init.c
        ${PROJECT_SOURCE_DIR}/MSDK/plf/riscv/env/entry.S
        ${PROJECT_SOURCE_DIR}/MSDK/plf/src/init_rom_symbol.c
        ${PROJECT_SOURCE_DIR}/MSDK/plf/riscv/env/start.S
        ${PROJECT_SOURCE_DIR}/MSDK/plf/riscv/arch/lib/lib_hook_mbl.c
        ${PROJECT_SOURCE_DIR}/MSDK/plf/riscv/gd32vw55x/system_gd32vw55x.c
    )

target_add_scatter_file(${TARGET_EXE}
    ${TARGET_PROJECT_DIR}/eclipse/mbl.ld
)

target_link_directories(${TARGET_EXE}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)

target_link_libraries(${TARGET_EXE}
    PRIVATE
        mbl
)

target_link_options(${TARGET_EXE}
    PUBLIC
    -Wl,--just-symbols=${PROJECT_SOURCE_DIR}/ROM-EXPORT/symbol/rom_symbol_m.gcc
    -Wl,-Map=${TARGET_PROJECT_DIR}/cmake/bin/${TARGET_EXE}.map
)

# Full path prefix (e.g. .../gcc/bin/riscv-nuclei-elf-) so mbl_afterbuild.{bat,sh} finds objcopy/objdump without PATH (matches MSDK image_* scripts).
set(_GD32_MBL_TOOL_PREFIX "${TOOLCHAIN_DIR}/bin/${CROSS_COMPILE}-")

if(${CMAKE_HOST_WIN32})
    add_custom_command(TARGET ${TARGET_EXE} POST_BUILD
        COMMAND ${TARGET_PROJECT_DIR}/mbl_afterbuild.bat ${_GD32_MBL_TOOL_PREFIX} ${PROJECT_SOURCE_DIR} ECDSA256 CERT ${OPENOCD_DIR} 00ffeeddccbbaa998877665544332211
        WORKING_DIRECTORY "${TARGET_PROJECT_DIR}/cmake/bin"
        COMMENT "Building ${TARGET_EXE}"
    )
elseif(${CMAKE_HOST_UNIX})
    add_custom_command(TARGET ${TARGET_EXE} POST_BUILD
        COMMAND cd ${TARGET_PROJECT_DIR}/cmake/bin && ../../mbl_afterbuild.sh ${_GD32_MBL_TOOL_PREFIX} ${PROJECT_SOURCE_DIR} ECDSA256 CERT ${OPENOCD_DIR} 00ffeeddccbbaa998877665544332211
        COMMENT "Building ${TARGET_EXE}"
    )
else()
    message( FATAL_ERROR "only support Windows platform or Linux platform")
endif()
