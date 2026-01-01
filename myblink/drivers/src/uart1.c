/**
 * @file uart1.c
 * @author your name (you@domain.com)
 * @brief uart空闲中断+dma半满、全满中断+dma双buffer+circlebuffer实现串口接收
 * @version 0.1
 * @date 2026-01-01
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "uart1.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "stdio.h"

// 2^11 = 2048，dma ringbuffer使用
#define UART_RX_BUFFER_LEN_BITS 11
#define UART_RX_BUFFER_LEN 2048
#define HALF_UART_RX_BUFFER_LEN (UART_RX_BUFFER_LEN >> 1)

#define UART uart1
#define UART_TX_PIN 6
#define UART_RX_PIN 7

typedef struct
{
    uint8_t uartRxBuffer[UART_RX_BUFFER_LEN] __aligned(4);
    int dmaChannel;
	int dmaSize;
} Uart1Dev;
static Uart1Dev uartDev;

static void DmaHalfIrqHandler(void)
{
	if (dma_channel_get_irq0_status(uartDev.dmaChannel)) {
		dma_channel_acknowledge_irq0(uartDev.dmaChannel);
		uint32_t curWriteAddr = dma_channel_hw_addr(uartDev.dmaChannel)->write_addr;
		uint32_t offset = curWriteAddr - (uintptr_t)uartDev.uartRxBuffer;
		if (offset < HALF_UART_RX_BUFFER_LEN) {
			printf("first complete!");
		} else {
			printf("second complete!");
		}
		dma_channel_set_transfer_count(uartDev.dmaChannel, HALF_UART_RX_BUFFER_LEN, true);
	}
}

static void Uart1initDma(void)
{
	// 获取空闲 DMA 通道
	int dmaChannel = dma_claim_unused_channel(true);
	dma_channel_config config = dma_channel_get_default_config(dmaChannel);
	channel_config_set_dreq(&config, uart_get_dreq(UART, false));
	channel_config_set_read_increment(&config, false);
    channel_config_set_write_increment(&config, true);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
	channel_config_set_ring(&config, uartDev.dmaChannel, UART_RX_BUFFER_LEN_BITS);
    dma_channel_configure(dmaChannel, 
        &config, 
        uartDev.uartRxBuffer,  // 读取到的数据写入到 uart buffer
        &uart_get_hw(UART)->dr,  // 从 uart 的 数据寄存器中读取
        HALF_UART_RX_BUFFER_LEN,  // dma 读取数据达到 uart buffer 的一半（1024）时候触发中断
        false);
	uartDev.dmaChannel = dmaChannel;

	// 设置dma结束的中断
	dma_channel_set_irq0_enabled(dmaChannel, true);
	irq_set_exclusive_handler(DMA_IRQ_0, DmaHalfIrqHandler);
	irq_set_enabled(DMA_IRQ_0, true);
}

void Uart1Init()
{
    uart_init(UART, PICO_DEFAULT_UART_BAUD_RATE);
	gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
	gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

	Uart1initDma();
}