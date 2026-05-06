
cmake_minimum_required(VERSION 3.15)

add_library(freertos_api INTERFACE)

add_library(freertos)

target_sources(freertos
    PRIVATE
        Source/croutine.c
        Source/event_groups.c
        Source/list.c
        Source/queue.c
        Source/stream_buffer.c
        Source/tasks.c
        Source/timers.c
)

target_sources(freertos
    PRIVATE
        Source/portable/riscv32/port.c
        Source/portable/riscv32/portasm.S
        Source/portable/riscv32/memcpy.S
        Source/portable/Common/tickless_sleep.c
        Source/portable/MemMang/heap_4.c
)



target_include_directories(freertos_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/Common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/riscv32>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
)

target_link_libraries(freertos
    freertos_api
    wifi_mgmt_api
    rtos_api
    riscv_api
    util_api
    bsp_api
)
add_lib_api(freertos_api)
add_lib(freertos)
