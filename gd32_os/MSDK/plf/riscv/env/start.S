/*!
    \file    start.S
    \brief   startup functions for GD32VW55x SDK.

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "riscv_encoding.h"

    .section .init
    .weak     eclic_msip_handler                # Software interrupt
    .weak     eclic_mtip_handler                # CPU Timer interrupt
    .weak     WWDGT_IRQHandler                  # Window Watchdog Timer interrupt
    .weak     LVD_IRQHandler                    # LVD through EXTI Line detect
    .weak     TAMPER_STAMP_IRQHandler           # Tamper and TimeStamp through EXTI Line detect
    .weak     RTC_WKUP_IRQHandler               # RTC Wakeup through EXTI Line
    .weak     FMC_IRQHandler                    # FMC
    .weak     RCU_IRQHandler                    # RCU
    .weak     EXTI0_IRQHandler                  # EXTI Line 0
    .weak     EXTI1_IRQHandler                  # EXTI Line 1
    .weak     EXTI2_IRQHandler                  # EXTI Line 2
    .weak     EXTI3_IRQHandler                  # EXTI Line 3
    .weak     EXTI4_IRQHandler                  # EXTI Line 4
    .weak     DMA_Channel0_IRQHandler           # DMA Channel0
    .weak     DMA_Channel1_IRQHandler           # DMA Channel1
    .weak     DMA_Channel2_IRQHandler           # DMA Channel2
    .weak     DMA_Channel3_IRQHandler           # DMA Channel3
    .weak     DMA_Channel4_IRQHandler           # DMA Channel4
    .weak     DMA_Channel5_IRQHandler           # DMA Channel5
    .weak     DMA_Channel6_IRQHandler           # DMA Channel6
    .weak     DMA_Channel7_IRQHandler           # DMA Channel7
    .weak     ADC_IRQHandler                    # ADC
    .weak     EXTI5_9_IRQHandler                # EXTI5 to EXTI9
    .weak     TIMER0_BRK_IRQHandler             # TIMER0 Break
    .weak     TIMER0_UP_IRQHandler              # TIMER0 Update
    .weak     TIMER0_CMT_IRQHandler             # TIMER0 Commutation
    .weak     TIMER0_Channel_IRQHandler         # TIMER0 Channel Capture Compare
    .weak     TIMER1_IRQHandler                 # TIMER1
    .weak     TIMER2_IRQHandler                 # TIMER2
    .weak     I2C0_EV_IRQHandler                # I2C0 Event
    .weak     I2C0_ER_IRQHandler                # I2C0 Error
    .weak     I2C1_EV_IRQHandler                # I2C1 Event
    .weak     I2C1_ER_IRQHandler                # I2C1 Error
    .weak     SPI_IRQHandler                    # SPI
    .weak     USART0_IRQHandler                 # USART0
    .weak     UART1_IRQHandler                  # UART1
    .weak     UART2_IRQHandler                  # UART2
    .weak     EXTI10_15_IRQHandler              # EXTI10 to EXTI15
    .weak     RTC_Alarm_IRQHandler              # RTC Alarm
    .weak     TIMER15_IRQHandler                # TIMER15
    .weak     TIMER16_IRQHandler                # TIMER16
    .weak     I2C0_WKUP_IRQHandler              # I2C0 Wakeup
    .weak     USART0_WKUP_IRQHandler            # USART0 Wakeup
    .weak     TIMER5_IRQHandler                 # TIMER5
    .weak     WIFI_PROT_IRQHandler              # WIFI Protocol trigger interrupt
    .weak     WIFI_INTGEN_IRQHandler            # WIFI Intgen interrupt
    .weak     WIFI_TX_IRQHandler                # WIFI Tx interrupt
    .weak     WIFI_RX_IRQHandler                # WIFI Rx interrupt
    .weak     LA_IRQHandler                     # LA interrupt
    .weak     WIFI_WKUP_IRQHandler              # WIFI wakeup interrupt
    .weak     BLE_WKUP_IRQHandler               # BLE wakeup interrupt
    .weak     PLATFORM_WAKE_IRQHandler          # Platform wake interrupt
    .weak     ISO_BT_STAMP0_IRQHandler          # ISO Bluetooth TimeStamp interrupt0
    .weak     ISO_BT_STAMP1_IRQHandler          # ISO Bluetooth TimeStamp interrupt1
    .weak     ISO_BT_STAMP2_IRQHandler          # ISO Bluetooth TimeStamp interrupt2
    .weak     ISO_BT_STAMP3_IRQHandler          # ISO Bluetooth TimeStamp interrupt3
    .weak     ISO_BT_STAMP4_IRQHandler          # ISO Bluetooth TimeStamp interrupt4
    .weak     ISO_BT_STAMP5_IRQHandler          # ISO Bluetooth TimeStamp interrupt5
    .weak     ISO_BT_STAMP6_IRQHandler          # ISO Bluetooth TimeStamp interrupt6
    .weak     ISO_BT_STAMP7_IRQHandler          # ISO Bluetooth TimeStamp interrupt7
    .weak     BLE_POWER_STATUS_IRQHandler       # BLE power status interrupt
    .weak     CAU_IRQHandler                    # CAU
    .weak     HAU_TRNG_IRQHandler               # HAU and TRNG
    .weak     WIFI_INT_IRQHandler               # WIFI interrupt
    .weak     BLE_SW_TRIG_IRQHandler            # SW triggered interrupt
    .weak     BLE_FINE_TIMER_TARGET_IRQHandler  # Fine Timer Target interrupt
    .weak     BLE_STAMP_TARGET1_IRQHandler      # Timestamp Target 1 interrupt
    .weak     BLE_STAMP_TARGET2_IRQHandler      # Timestamp Target 2 interrupt
    .weak     BLE_STAMP_TARGET3_IRQHandler      # Timestamp Target 3 interrupt
    .weak     BLE_ENCRYPTION_ENGINE_IRQHandler  # Encryption engine Interrupt
    .weak     BLE_SLEEP_MODE_IRQHandler         # Sleep Mode Interrupt
    .weak     BLE_HALF_SLOT_IRQHandler          # Half Slot interrupt
    .weak     BLE_FIFO_ACTIVITY_IRQHandler      # FIFO Activity interrupt
    .weak     BLE_ERROR_IRQHandler              # Error Interrupt
    .weak     BLE_FREQ_SELECT_IRQHandler        # Frequency Selection Interrupt
    .weak     EFUSE_IRQHandler                  # EFUSE
    .weak     QSPI_IRQHandler                   # QUADSPI
    .weak     PKCAU_IRQHandler                  # PKCAU

vector_base:
    j _start                                    /* 0: Reserved, Jump to _start when reset for vector table not remapped cases.*/
    .align LOG_REGBYTES                         /*    Need to align 4 byte for RV32, 8 Byte for RV64 */
    .word     0
    .word     0
    .word     eclic_msip_handler                # Software interrupt
    .word     0                                 # Reserved
    .word     0
    .word     0
    .word     eclic_mtip_handler                # CPU Timer interrupt
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     WWDGT_IRQHandler                  # Window Watchdog Timer
    .word     LVD_IRQHandler                    # LVD through EXTI Line detect
    .word     TAMPER_STAMP_IRQHandler           # Tamper and TimeStamp through EXTI Line detect
    .word     RTC_WKUP_IRQHandler               # RTC Wakeup through EXTI Line
    .word     FMC_IRQHandler                    # FMC
    .word     RCU_IRQHandler                    # RCU
    .word     EXTI0_IRQHandler                  # EXTI Line 0
    .word     EXTI1_IRQHandler                  # EXTI Line 1
    .word     EXTI2_IRQHandler                  # EXTI Line 2
    .word     EXTI3_IRQHandler                  # EXTI Line 3
    .word     EXTI4_IRQHandler                  # EXTI Line 4
    .word     DMA_Channel0_IRQHandler           # DMA Channel0
    .word     DMA_Channel1_IRQHandler           # DMA Channel1
    .word     DMA_Channel2_IRQHandler           # DMA Channel2
    .word     DMA_Channel3_IRQHandler           # DMA Channel3
    .word     DMA_Channel4_IRQHandler           # DMA Channel4
    .word     DMA_Channel5_IRQHandler           # DMA Channel5
    .word     DMA_Channel6_IRQHandler           # DMA Channel6
    .word     DMA_Channel7_IRQHandler           # DMA Channel7
    .word     ADC_IRQHandler                    # ADC
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     EXTI5_9_IRQHandler                # EXTI5 to EXTI9
    .word     TIMER0_BRK_IRQHandler             # TIMER0 Break
    .word     TIMER0_UP_IRQHandler              # TIMER0 Update
    .word     TIMER0_CMT_IRQHandler             # TIMER0 Commutation
    .word     TIMER0_Channel_IRQHandler         # TIMER0 Channel Capture Compare
    .word     TIMER1_IRQHandler                 # TIMER1
    .word     TIMER2_IRQHandler                 # TIMER2
    .word     0                                 # Reserved
    .word     I2C0_EV_IRQHandler                # I2C0 Event
    .word     I2C0_ER_IRQHandler                # I2C0 Error
    .word     I2C1_EV_IRQHandler                # I2C1 Event
    .word     I2C1_ER_IRQHandler                # I2C1 Error
    .word     SPI_IRQHandler                    # SPI
    .word     0                                 # Reserved
    .word     USART0_IRQHandler                 # USART0
    .word     UART1_IRQHandler                  # UART1
    .word     UART2_IRQHandler                  # UART2
    .word     EXTI10_15_IRQHandler              # EXTI10 to EXTI15
    .word     RTC_Alarm_IRQHandler              # RTC Alarm
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     TIMER15_IRQHandler                # TIMER15
    .word     TIMER16_IRQHandler                # TIMER16
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     I2C0_WKUP_IRQHandler              # I2C0 Wakeup
    .word     USART0_WKUP_IRQHandler            # USART0 Wakeup
    .word     0                                 # Reserved
    .word     TIMER5_IRQHandler                 # TIMER5
    .word     WIFI_PROT_IRQHandler              # WIFI Protocol trigger
    .word     WIFI_INTGEN_IRQHandler            # WIFI Intgen
    .word     WIFI_TX_IRQHandler                # WIFI Tx
    .word     WIFI_RX_IRQHandler                # WIFI Rx
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     LA_IRQHandler                     # LA interrupt
    .word     WIFI_WKUP_IRQHandler              # WIFI wakeup interrupt
    .word     BLE_WKUP_IRQHandler               # BLE wakeup interrupt
    .word     PLATFORM_WAKE_IRQHandler          # Platform wake
    .word     ISO_BT_STAMP0_IRQHandler          # ISO Bluetooth TimeStamp interrupt0
    .word     ISO_BT_STAMP1_IRQHandler          # ISO Bluetooth TimeStamp interrupt1
    .word     ISO_BT_STAMP2_IRQHandler          # ISO Bluetooth TimeStamp interrupt2
    .word     ISO_BT_STAMP3_IRQHandler          # ISO Bluetooth TimeStamp interrupt3
    .word     ISO_BT_STAMP4_IRQHandler          # ISO Bluetooth TimeStamp interrupt4
    .word     ISO_BT_STAMP5_IRQHandler          # ISO Bluetooth TimeStamp interrupt5
    .word     ISO_BT_STAMP6_IRQHandler          # ISO Bluetooth TimeStamp interrupt6
    .word     ISO_BT_STAMP7_IRQHandler          # ISO Bluetooth TimeStamp interrupt7
    .word     BLE_POWER_STATUS_IRQHandler       # BLE power status interrupt
    .word     0                                 # Reserved
    .word     0                                 # Reserved
    .word     CAU_IRQHandler                    # CAU
    .word     HAU_TRNG_IRQHandler               # HAU and TRNG
    .word     0                                 # Reserved
    .word     WIFI_INT_IRQHandler               # WIFI interrupt
    .word     BLE_SW_TRIG_IRQHandler            # SW triggered interrupt
    .word     BLE_FINE_TIMER_TARGET_IRQHandler  # Fine Timer Target interrupt
    .word     BLE_STAMP_TARGET1_IRQHandler      # Timestamp Target 1 interrupt
    .word     BLE_STAMP_TARGET2_IRQHandler      # Timestamp Target 2 interrupt
    .word     BLE_STAMP_TARGET3_IRQHandler      # Timestamp Target 3 interrupt
    .word     BLE_ENCRYPTION_ENGINE_IRQHandler  # Encryption engine Interrupt
    .word     BLE_SLEEP_MODE_IRQHandler         # Sleep Mode interrupt
    .word     BLE_HALF_SLOT_IRQHandler          # Half Slot interrupt
    .word     BLE_FIFO_ACTIVITY_IRQHandler      # FIFO Activity interrupt
    .word     BLE_ERROR_IRQHandler              # Error interrupt
    .word     BLE_FREQ_SELECT_IRQHandler        # Frequency Selection iterrupt
    .word     EFUSE_IRQHandler                  # EFUSE
    .word     QSPI_IRQHandler                   # QUADSPI
    .word     PKCAU_IRQHandler                  # PKCAU
    .globl _start
    .type _start,@function

_start:

    /* Disable Global Interrupt */
    csrc CSR_MSTATUS, MSTATUS_MIE
    /* Initialize GP and Stack Pointer SP */
.option push
.option norelax
    la gp, __global_pointer$
.option pop
    la sp, _sp

    /* Set the the NMI base to share with mtvec by setting CSR_MMISC_CTL */
    li t0, MMISC_CTL_NMI_CAUSE_FFF
    csrs CSR_MMISC_CTL, t0
    /* Intial the mtvt*/
    la t0, vector_base
    csrw CSR_MTVT, t0

    /* Intial the mtvt2 and enable it*/
    la t0, irq_entry
    csrw CSR_MTVT2, t0
    csrs CSR_MTVT2, 0x1

    /* Intial the CSR MTVEC for the Trap and NMI base addr*/
    la t0, exc_entry
    csrw CSR_MTVEC, t0

    /* Set the interrupt processing mode to ECLIC mode */
    li t0, 0x3f
    csrc CSR_MTVEC, t0
    csrs CSR_MTVEC, 0x3
    /* ===== Startup Stage 2 ===== */
#ifdef __riscv_flen
    /* Enable FPU */
    li t0, MSTATUS_FS
    csrs mstatus, t0
    csrw fcsr, x0
#endif

    /* Enable mcycle and minstret counter */
    csrci CSR_MCOUNTINHIBIT, 0x5

    /* ===== Startup Stage 3 ===== */
    /* Load code to sram */
    la a0, _sicode
    la a1, _scode
    la a2, _ecode
    bgeu a1, a2, 2f
1:
    lw t0, (a0)
    sw t0, (a1)
    addi a0, a0, 4
    addi a1, a1, 4
    bltu a1, a2, 1b

2:
    /* Load data section */
    la a0, _data_lma
    la a1, _data
    la a2, _edata
    bgeu a1, a2, 2f
1:
    lw t0, (a0)
    sw t0, (a1)
    addi a0, a0, 4
    addi a1, a1, 4
    bltu a1, a2, 1b
2:
    /* Clear bss section */
    la a0, __bss_start
    la a1, _end
    bgeu a0, a1, 2f
1:
    sw zero, (a0)
    addi a0, a0, 4
    bltu a0, a1, 1b
2:

    /*
     * Call vendor defined SystemInit to
     * initialize the micro-controller system
     */
    call SystemInit

    /* Call global constructors */
    la a0, __libc_fini_array
    call atexit
    /* Call C/C++ constructor start up code */
    call __libc_init_array

    /* do pre-init steps before main */
    call _premain_init
    /* ===== Call Main Function  ===== */
    /* argc = argv = 0 */
    li a0, 0
    li a1, 0
    call main
    /* do post-main steps after main */
    call _postmain_fini

1:
    j 1b
