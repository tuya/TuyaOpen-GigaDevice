cmake_minimum_required(VERSION 3.15)

add_library(rtos_api INTERFACE)
add_library(rtos)

target_compile_options(rtos
    PRIVATE
        -w
)

if (PLATFORM_OS_FREERTOS)
    add_subdirectory(FreeRTOS)
    target_include_directories(rtos_api
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Source/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Source/portable/riscv32>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/config>
    )
endif()



target_include_directories(rtos_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rtos_wrapper>
)

target_sources(rtos
    PRIVATE
        rtos_wrapper/wrapper_os.c
)

target_link_libraries(rtos
    $<$<BOOL:${PLATFORM_OS_FREERTOS}>:freertos>
    rtos_api
)

add_lib_api(rtos_api)
add_lib(rtos)