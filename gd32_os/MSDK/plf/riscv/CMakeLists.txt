
cmake_minimum_required(VERSION 3.15)

add_library(riscv_api INTERFACE)


target_sources(${TARGET_EXE}
    PRIVATE
        env/entry.S
        env/start.S
        env/env_init.c
        env/handlers.c
        arch/lib/lib_hook.c
        gd32vw55x/gd32vw55x_it.c
        gd32vw55x/system_gd32vw55x.c
)


target_include_directories(riscv_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gd32vw55x>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/NMSIS/Core/Include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/NMSIS/DSP/Include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/NMSIS/DSP/Include/dsp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/arch>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/arch/boot>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/arch/lib>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/arch/ll>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/arch/compiler>
)


add_lib_api(riscv_api)

