OUTPUT_ARCH( "riscv" )
ENTRY( _start )
MEMORY
{
    flash (rxai!w) : ORIGIN = 0x08000000 + 0, LENGTH = 0x400000 - (0)
    ram (wxa!ri) : ORIGIN = 0x20000000 + 0x200, LENGTH = (288K - 0x200)
    mib (ra!wxi) : ORIGIN = 0x40030800, LENGTH = 2K
}
SECTIONS
{
  __stack_size = DEFINED(__stack_size) ? __stack_size : 2K;
  __heap_size = DEFINED(__heap_size) ? __heap_size : 2K;
  __trace_size = DEFINED(__trace_size) ? __trace_size : 4;
  .init :
  {
    KEEP (*(SORT_NONE(.init)))
  } >flash AT>flash
  .ilalign :
  {
    . = ALIGN(4);
    PROVIDE( _ilm_lma = . );
  } >flash AT>flash
  .ialign :
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
    *inet_chksum.o* (.text.lwip_standard_chksum)
    *txl_agg.o* (.text.txl_agg_push_mpdu .text.txl_agg_finish)
    *txl_he.o* (.text.txl_he_tb_prot_trigger .text.txl_he_trigger_push)
    *rxl_hwdesc.o* (.text.rxl_immediate_frame_get .text.rxl_rxcntrl_frame)
    . = ALIGN(4);
    _ecode = .;
  } >ram AT>flash
  .text :
  {
    *(.rodata .rodata.*)
    *(.srodata .srodata.*)
    *(.text.unlikely .text.unlikely.*)
    *(.text.startup .text.startup.*)
    *(.text .text.*)
    *(.gnu.linkonce.t.*)
    *(.sdata2 .sdata2.*)
  } >flash AT>flash
  .fini :
  {
    KEEP (*(SORT_NONE(.fini)))
  } >flash AT>flash
  . = ALIGN(4);
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  PROVIDE( _eilm = . );
  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >flash AT>flash
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >flash AT>flash
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >flash AT>flash
  .ctors :
  {
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } >flash AT>flash
  .dtors :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } >flash AT>flash
    . = ALIGN(4);
    PROVIDE( _eilm = . );
  .lalign :
  {
    . = ALIGN(4);
    PROVIDE( _data_lma = . );
  } >flash AT>flash
  .dalign :
  {
    . = ALIGN(4);
  } >ram AT>flash
  .data :
  {
    PROVIDE( _data = . );
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
  .bss :
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
}
