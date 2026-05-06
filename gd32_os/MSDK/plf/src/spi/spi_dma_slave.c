/*!
    \file    spi_dma_slave.c
    \brief   SPI DMA slave interface for GD32VW55x.

             Provides the spi_dma_slave_init / _deinit / _read / _write
             abstraction consumed by the upper-layer mm_spi driver.  Wraps
             the existing spi_dma_config() / spi_slave_init() BSP helpers
             in spi.c and owns the DMA channel ISR handlers for the SPI
             RX (DMA_CH2) and TX (DMA_CH3) channels.

    Design notes
    ------------
    SPI is full-duplex, so every transfer clocks both MISO and MOSI
    simultaneously.  The upper layer drives the pattern:

      spi_dma_slave_write(tx_buf, N);   // stage TX buffer (no DMA start)
      spi_dma_slave_read (rx_buf, N);   // arm RX + commit both channels

    or, for receive-only (dummy TX):

      spi_dma_slave_read(rx_buf, N);    // no prior write → dummy TX byte

    When the transfer completes:
      DMA_CH2 FTF → DMA_Channel2_IRQHandler → read_done_cb(rx_buf, N)
      DMA_CH3 FTF → DMA_Channel3_IRQHandler → write_done_cb(tx_buf, N)

    DMA_Channel2_IRQHandler overrides the __attribute__((weak)) stub in
    gd32vw55x_it.c.  DMA_Channel3_IRQHandler is only defined under
    CONFIG_ATCMD_SPI / CONFIG_SPI_I2S guards in that file, so the
    definition here is the sole strong symbol when those macros are absent.

    \version 2024-09-29, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
    ...
    (Standard GigaDevice BSD-3 licence — see spi.c for full text.)
*/

#include "gd32vw55x.h"
#include "spi.h"
#include "wrapper_os.h"
#include <stdbool.h>
#include <stddef.h>

/* ------------------------------------------------------------------
 * Registered completion callbacks
 * ------------------------------------------------------------------ */
static void (*s_read_done_cb)(char *, int) = NULL;
static void (*s_write_done_cb)(char *, int) = NULL;

/* ------------------------------------------------------------------
 * Per-transaction state
 * set by spi_dma_slave_write / spi_dma_slave_read; read by ISRs
 * ------------------------------------------------------------------ */
static char *volatile s_rx_buf = NULL;
static int s_rx_size = 0;
static char *volatile s_tx_buf = NULL;
static int s_tx_size = 0;

/*
 * s_tx_armed: set by spi_dma_slave_write(), cleared by the subsequent
 * spi_dma_slave_read() call.  When true the real TX buffer is used;
 * otherwise spi_dma_dummy_mode_config() clocks out a constant byte.
 */
static volatile bool s_tx_armed = false;

/* ------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------ */

/**
 * @brief  Initialise SPI DMA slave mode and register completion callbacks.
 *
 * Calls spi_slave_init() for full hardware setup (GPIO, ECLIC IRQ enable,
 * SPI parameter config).  Must be called before any _read / _write.
 *
 * @param read_done_cb   Invoked from DMA_Channel2_IRQHandler when the RX
 *                       DMA full-transfer-flag fires.
 * @param write_done_cb  Invoked from DMA_Channel3_IRQHandler when the TX
 *                       DMA full-transfer-flag fires.
 */
void spi_dma_slave_init(void (*read_done_cb)(char *, int), void (*write_done_cb)(char *, int))
{
    s_read_done_cb = read_done_cb;
    s_write_done_cb = write_done_cb;
    s_rx_buf = NULL;
    s_tx_buf = NULL;
    s_tx_armed = false;

    spi_slave_init();
}

/**
 * @brief  Deinitialise SPI DMA slave.
 *
 * Disables the DMA channel IRQs, DMA channels, SPI DMA requests, and the
 * SPI peripheral itself.  Clears all stored state.
 */
void spi_dma_slave_deinit(void)
{
    eclic_irq_disable(SPI_RX_DMA_CH_IRQn);
    eclic_irq_disable(SPI_TX_DMA_CH_IRQn);

    spi_dma_disable(SPI_DMA_RECEIVE);
    spi_dma_disable(SPI_DMA_TRANSMIT);
    dma_channel_disable(SPI_RX_DMA_CH);
    dma_channel_disable(SPI_TX_DMA_CH);
    spi_disable();

    s_read_done_cb = NULL;
    s_write_done_cb = NULL;
    s_rx_buf = NULL;
    s_tx_buf = NULL;
    s_tx_armed = false;
}

/**
 * @brief  Stage a TX buffer for the next full-duplex DMA transfer.
 *
 * Does NOT start DMA.  The DMA start is deferred to the subsequent
 * spi_dma_slave_read() call so that both directions are committed
 * atomically through a single spi_dma_config() invocation.
 *
 * @param buf   TX data (must remain valid until write_done_cb fires).
 * @param size  Byte count; must match the RX size passed to slave_read().
 */
void spi_dma_slave_write(char *buf, int size)
{
    s_tx_buf = buf;
    s_tx_size = size;
    s_tx_armed = true;
}

/**
 * @brief  Arm the RX buffer and start the full-duplex DMA transfer.
 *
 * If spi_dma_slave_write() was called before this function, the real TX
 * buffer is used for MISO; otherwise a dummy constant byte is clocked out.
 * After this call the SPI peripheral is enabled and waiting for the master
 * to drive the clock.
 *
 * Completion is signalled asynchronously:
 *   - DMA_CH2 FTF → DMA_Channel2_IRQHandler → read_done_cb(buf, size)
 *   - DMA_CH3 FTF → DMA_Channel3_IRQHandler → write_done_cb(tx_buf, tx_size)
 *
 * @param buf   RX destination buffer.
 * @param size  Number of bytes to receive.
 */
void spi_dma_slave_read(char *buf, int size)
{
    bool use_real_tx;

    s_rx_buf = buf;
    s_rx_size = size;
    use_real_tx = s_tx_armed;
    s_tx_armed = false; /* clear before DMA start to avoid re-arm race */

    spi_dma_config(/*dma_rx=*/true, (uint32_t)buf,
                   /*dma_tx=*/use_real_tx, use_real_tx ? (uint32_t)s_tx_buf : 0U, (uint32_t)size,
                   /*from_isr=*/false);
}

/* ------------------------------------------------------------------
 * DMA ISR handlers
 * ------------------------------------------------------------------ */

/**
 * @brief  DMA Channel 2 ISR — SPI RX full-transfer complete.
 *
 * SPI_RX_DMA_CH == DMA_CH2 (spi.h).
 * Overrides the __attribute__((weak)) definition in gd32vw55x_it.c.
 */
void DMA_Channel2_IRQHandler(void)
{
    sys_int_enter();

    if (RESET != dma_interrupt_flag_get(SPI_RX_DMA_CH, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(SPI_RX_DMA_CH, DMA_INT_FLAG_FTF);
        if (s_read_done_cb) {
            s_read_done_cb(s_rx_buf, s_rx_size);
        }
    }

    sys_int_exit();
}

/**
 * @brief  DMA Channel 3 ISR — SPI TX full-transfer complete.
 *
 * SPI_TX_DMA_CH == DMA_CH3 (spi.h).
 * Provided as a strong symbol; CONFIG_ATCMD_SPI / CONFIG_SPI_I2S guards in
 * gd32vw55x_it.c ensure no duplicate definition when those features are off.
 */
void DMA_Channel3_IRQHandler(void)
{
    sys_int_enter();

    if (RESET != dma_interrupt_flag_get(SPI_TX_DMA_CH, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(SPI_TX_DMA_CH, DMA_INT_FLAG_FTF);
        if (s_write_done_cb) {
            s_write_done_cb(s_tx_buf, s_tx_size);
        }
    }

    sys_int_exit();
}

/**
 * @brief  Query how many bytes the RX DMA channel has actually transferred so far.
 *
 * Reads the DMA CHCNT register while DMA is still running (or after an
 * unexpected stop) to determine how many bytes arrived:
 *   received = s_rx_size - remaining
 *
 * @param[out] out_received  If non-NULL, filled with the number of bytes received.
 * @return  Remaining bytes DMA has NOT yet transferred (0 means fully done).
 *          Returns s_rx_size (received=0) if no transfer was armed.
 */
int spi_dma_slave_rx_query(int *out_received)
{
    int remaining = (int)dma_transfer_number_get(SPI_RX_DMA_CH);
    int received = s_rx_size - remaining;
    if (out_received) {
        *out_received = received;
    }
    return remaining;
}
