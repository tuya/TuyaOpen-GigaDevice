/*!
    \file    gd32vw55x.ld
    \brief   Load file for GD32VW55x SDK.

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

#include "config_gdm32.h"
#include "mbl_region.h"
#include "wlan_config.h"
#include "util_config.h"
#include "app_cfg.h"

OUTPUT_ARCH( "riscv" )

ENTRY( _start )

MEMORY
{
    /*Run in FLASH */
    flash (rxai!w) : ORIGIN = RE_FLASH_BASE + RE_IMG_0_OFFSET + IMG_OVERHEAD_LEN, LENGTH = RE_END_OFFSET - (RE_IMG_0_OFFSET + IMG_OVERHEAD_LEN)
    /* LA or wifi rx psd need use 32K memory (320K - 32K = 288K) */
    ram   (wxa!ri) : ORIGIN = RE_SRAM_BASE + RE_IMG_DATA_START, LENGTH = (288K - RE_IMG_DATA_START)
    mib   (ra!wxi) : ORIGIN = 0x40030800, LENGTH = 2K
#ifdef CFG_GD_TRACE_EXT
    /*0x30000000 is a virtual address, just for log section. It will be filtered after build image*/
    log   (wxa!ri) : ORIGIN = 0x30000000, LENGTH = 40K
#endif
}

SECTIONS
{
  __stack_size = DEFINED(__stack_size) ? __stack_size : 2K;
#ifdef CFG_MATTER
  __heap_size = DEFINED(__heap_size) ? __heap_size : 16K;
#else
  __heap_size = DEFINED(__heap_size) ? __heap_size : 2K;
#endif
#if defined( CFG_GD_TRACE_EXT )
  __trace_size = 16K;
#elif defined( CFG_TRACE )
  __trace_size = 4K;
#else
  __trace_size = DEFINED(__trace_size) ? __trace_size : 4;
#endif

  .init           :
  {
    _vetor_base = .;
    KEEP (*(SORT_NONE(.init)))
  } >flash AT>flash

  .ilalign         :
  {
    . = ALIGN(4);
    PROVIDE( _ilm_lma = . );
  } >flash AT>flash

  .ialign         :
  {
    PROVIDE( _ilm = . );
  } >flash AT>flash

  _sicode = LOADADDR(.code_to_sram);

  .code_to_sram :
  {
    . = ALIGN(4);
    _scode = .;

    *entry.o* (.text.irq)
    *tx_thread_entry.o* (.text.irq)
    *tx_thread_schedule.o* (.text.irq)
    *tx_thread_context_save.o* (.text.irq)
    *tx_thread_context_restore.o* (.text.irq)
    *port.o* (.text.eclic_mtip_handler)
    *lib_a-memcpy.o* (.text*)
    *wrapper_os.o* (.text.sys_memcpy*)
    *uart.o* (.text.log_uart_irq_hdl)
    *atcmd.o* (.text.at_spi_rx_irq_hdl)
    *inet_chksum.o* (.text.lwip_standard_chksum)
    /**macsw/modules/ke/ke_event.o* (.rodata.ke_evt_hdlr*)
    *rxl_cntrl.o* (.text.rxl_upload_evt)*/
    *txl_agg.o* (.text.txl_agg_push_mpdu  .text.txl_agg_finish)
    *txl_he.o* (.text.txl_he_tb_prot_trigger .text.txl_he_trigger_push)
    *rxl_hwdesc.o* (.text.rxl_immediate_frame_get  .text.rxl_rxcntrl_frame)

    /* rtthread os */
    *board.o* (.text.eclic_mtip_handler)
    *cpuport.o* (.text*)
    *context_gcc.o* (.text*)
    *gd32vw55x_fmc.o* (.text.ob_unlock .text.ob_lock .text.fmc_unlock .text.fmc_lock .text.fmc_offset_value_config .text.fmc_offset_region_config)
    *raw_flash_api.o* (.text.raw_flash_read)

#if defined( CFG_WFA_HE ) || defined( CFG_WIFI_HIGH_PERFORMANCE )
    *txl_agg.o* (.text.* .rodata.*)
    *txl_he.o* (.text.* .rodata.*)
    *rxl_hwdesc.o* (.text.* .rodata.*)
#endif

#if defined( CONFIG_FLASH_NOT_BLOCK_UART_RX )
    *gd32vw55x_it.o* (.text.UART1_IRQHandler) /* choose UART on condition */
    *atcmd.o* (.text.at_uart_rx_irq_hdl)
    *uart.o* (.text.uart_irq_hdl)
    *gd32vw55x_usart.o* (.text.usart_flag_get .text.usart_flag_clear)
    *gd32vw55x_usart.o* (.text.usart_data_receive .text.usart_data_transmit)
    *gd32vw55x_usart.o* (.text.usart_interrupt_enable .text.usart_interrupt_disable)
    *raw_flash_api.o* (.text.raw_flash_write_fast .text.raw_flash_erase)
    *save-restore.o* (.text*)
#endif

#ifdef TUYAOS_SUPPORT
    /* SPI slave driver — run from SRAM for DMA/ISR latency */
    *mm_spi.o* (.text* .rodata*)
    *wifi_bridge.o* (.text* .rodata*)
    *spi_dma_slave.o* (.text* .rodata*)
    *spi.o* (.text* .rodata*)
#endif

    . = ALIGN(4);
    _ecode = .;
  } >ram AT>flash

  .mesh_settings          :
  {
    . = ALIGN(4);
    PROVIDE( _settings_cbs = . );
    KEEP (*(.MESH_SETTINGS_CBS .MESH_SETTINGS_CBS*))
    PROVIDE( _esettings_cbs = . );

    . = ALIGN(4);
    PROVIDE( _appkeys_cbs = . );
    KEEP (*(.MESH_APPKEYS_CBS .MESH_APPKEYS_CBS*))
    PROVIDE( _eappkeys_cbs = . );

    . = ALIGN(4);
    PROVIDE( _proxy_cbs = . );
    KEEP (*(.MESH_PROXY_CBS .MESH_PROXY_CBS*))
    PROVIDE( _eproxy_cbs = . );

    . = ALIGN(4);
    PROVIDE( _lpn_cbs = . );
    KEEP (*(.MESH_LPN_CBS .MESH_LPN_CBS*))
    PROVIDE( _elpn_cbs = . );

    . = ALIGN(4);
    PROVIDE( _friend_cbs = . );
    KEEP (*(.MESH_FRIEND_CBS .MESH_FRIEND_CBS*))
    PROVIDE( _efriend_cbs = . );

    . = ALIGN(4);
    PROVIDE( _beacon_cbs = . );
    KEEP (*(.MESH_BEACON_CBS .MESH_BEACON_CBS*))
    PROVIDE( _ebeacon_cbs = . );

    . = ALIGN(4);
    PROVIDE( _hb_cbs = . );
    KEEP (*(.MESH_HB_CBS .MESH_HB_CBS*))
    PROVIDE( _ehb_cbs = . );

    . = ALIGN(4);
    PROVIDE( _subnet_cbs = . );
    KEEP (*(.MESH_SUBNET_CBS .MESH_SUBNET_CBS*))
    PROVIDE( _esubnet_cbs = . );
  } >flash AT>flash


  .text           :
  {
    . = ALIGN(4);
    *(.rodata .rodata.*)
    *(.srodata .srodata.*)
    *(.text.unlikely .text.unlikely.*)
    *(.text.startup .text.startup.*)
    *(.text .text.*)
    *(.gnu.linkonce.t.*)
    *(.sdata2 .sdata2.*)
  } >flash AT>flash

  .fini           :
  {
    KEEP (*(SORT_NONE(.fini)))
  } >flash AT>flash

  . = ALIGN(4);

  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  PROVIDE( _eilm = . );

  .preinit_array  :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >flash AT>flash

  .init_array     :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >flash AT>flash

  .fini_array     :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >flash AT>flash

  .ctors          :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    /* We don't want to include the .ctor section from
       the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } >flash AT>flash

  .dtors          :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } >flash AT>flash

    . = ALIGN(4);
    PROVIDE( _eilm = . );

  .lalign         :
  {
    . = ALIGN(4);
    PROVIDE( _data_lma = . );
  } >flash AT>flash

  .dalign         :
  {
    . = ALIGN(4);
  } >ram AT>flash

  .data          :
  {
    PROVIDE( _data = . );
/*  *(.rdata)
    *(.gnu.linkonce.r.*)
*/
    *(.data .data.*)
    *(.gnu.linkonce.d.*)
    *(.sdata .sdata.*)
    *(.gnu.linkonce.s.*)
    . = ALIGN(8);
  } >ram AT>flash

  . = ALIGN(4);
  PROVIDE( _edata = . );
  PROVIDE( edata = . );

  PROVIDE( _fbss = . );
  .bss            :
  {
    PROVIDE( __bss_start = . );
    *(.sbss*)
    *(.gnu.linkonce.sb.*)
    . = ALIGN(8);
    PROVIDE( __global_pointer$ = .);
    *(.bss .bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(4);
  } >ram AT>ram

  . = ALIGN(8);
  PROVIDE( _rtos = . );

  .trace ORIGIN(ram) + LENGTH(ram) - __stack_size - __heap_size - __trace_size :
  {
    PROVIDE( _ertos = . - 4);
    . = ALIGN(4);
    PROVIDE( _trace = . );
    . = __trace_size - 4;
    PROVIDE( _etrace = . );
  } >ram AT>ram

  . = ALIGN(8);
  PROVIDE( _end = . );
  PROVIDE( end = . );

  .heap ORIGIN(ram) + LENGTH(ram) - __stack_size - __heap_size :
  {
    . = __heap_size - 4;
    PROVIDE( _heap_end = . );
  } >ram AT>ram

  .stack ORIGIN(ram) + LENGTH(ram) - __stack_size :
  {
    . = __stack_size;
    PROVIDE( _sp = . );
  } >ram AT>ram

  .mib :
  {
    *(MACHWMIB)
  } >mib

#if defined(CFG_GD_TRACE_EXT)
  .log :
  {
    *(.GDTRACE .GDTRACE*)
  } >log
#endif
}
