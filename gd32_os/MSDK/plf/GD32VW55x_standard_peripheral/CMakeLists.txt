
cmake_minimum_required(VERSION 3.15)

add_library(gd32vw55x_peripheral_api INTERFACE)

add_library(gd32vw55x_peripheral)

target_sources(gd32vw55x_peripheral
    PRIVATE
        Source/gd32vw55x_adc.c
        Source/gd32vw55x_cau_aes.c
        Source/gd32vw55x_cau_des.c
        Source/gd32vw55x_cau_tdes.c
        Source/gd32vw55x_cau.c
        Source/gd32vw55x_crc.c
        Source/gd32vw55x_dbg.c
        Source/gd32vw55x_dma.c
        Source/gd32vw55x_eclic.c
        Source/gd32vw55x_efuse.c
        Source/gd32vw55x_exti.c
        Source/gd32vw55x_fmc.c
        Source/gd32vw55x_fwdgt.c
        Source/gd32vw55x_gpio.c
        Source/gd32vw55x_hau_sha_md5.c
        Source/gd32vw55x_hau.c
        Source/gd32vw55x_i2c.c
        Source/gd32vw55x_pkcau.c
        Source/gd32vw55x_pmu.c
        Source/gd32vw55x_qspi.c
        Source/gd32vw55x_rcu.c
        Source/gd32vw55x_rtc.c
        Source/gd32vw55x_spi.c
        Source/gd32vw55x_syscfg.c
        Source/gd32vw55x_timer.c
        Source/gd32vw55x_trng.c
        Source/gd32vw55x_usart.c
        Source/gd32vw55x_wwdgt.c

)

target_include_directories(gd32vw55x_peripheral_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
)

target_link_libraries(gd32vw55x_peripheral
    gd32vw55x_peripheral_api
)

add_lib_api(gd32vw55x_peripheral_api)
add_lib(gd32vw55x_peripheral)