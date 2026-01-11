#if 0
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"


#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define UART0_TX_PIN 0
#define UART0_RX_PIN 1

#define UART1_TX_PIN 4
#define UART1_RX_PIN 5

#define TRANSFER_SIZE 256

static volatile int uart_irq_count = 0;
static volatile int dma_irq_count = 0;

static int rx_dma_chan = -1;

// RX interrupt handler
void on_uart_rx() {
    uart_irq_count++;
    // Clear the IRQ by emptying the data
    while (uart_is_readable(uart0)) {
        uint8_t ch = uart_getc(uart0);
    }
}

// DMA interrupt handler
void on_dma_complete() {
    dma_irq_count++;
    // Clear the IRQ
    dma_channel_acknowledge_irq0(rx_dma_chan);
}

int main() {
    uint8_t tx_data[TRANSFER_SIZE];
    uint8_t rx_data[TRANSFER_SIZE];

    // Initialize stdio over USB
    stdio_usb_init();

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART0_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX_PIN, GPIO_FUNC_UART);

    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);

    // Initialize our UARTs.
    uart_init(uart0, BAUD_RATE);
    uart_init(uart1, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(uart0, false, false);
    uart_set_hw_flow(uart1, false, false);

    // Set our data format
    uart_set_format(uart0, DATA_BITS, STOP_BITS, PARITY);
    uart_set_format(uart1, DATA_BITS, STOP_BITS, PARITY);

    // Turn on the FIFOs
    uart_set_fifo_enabled(uart0, true);
    uart_set_fifo_enabled(uart1, true);

    // Set up the RX interrupt handler
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);

    // Set RX FIFO watermark to 16 bytes (half full)
    hw_write_masked(&uart_get_hw(uart0)->ifls, 2 << UART_UARTIFLS_RXIFLSEL_LSB, UART_UARTIFLS_RXIFLSEL_BITS);

    // Enable the UART to send RX timeout and FIFO watermark interrupts
    uart_get_hw(uart0)->imsc = UART_UARTIMSC_RXIM_BITS | UART_UARTIMSC_RTIM_BITS;

    // Enable the RX interrupt handler
    irq_set_enabled(UART0_IRQ, true);

    // Get the DMA channel for RX, panic() if there are none
    rx_dma_chan = dma_claim_unused_channel(true);

    dma_channel_config rx_dma_config = dma_channel_get_default_config(rx_dma_chan);
    channel_config_set_read_increment(&rx_dma_config, false);
    channel_config_set_write_increment(&rx_dma_config, true);
    channel_config_set_dreq(&rx_dma_config, uart_get_dreq(uart0, false));
    channel_config_set_transfer_data_size(&rx_dma_config, DMA_SIZE_8);
    // channel_config_set_ring(&rx_dma_config, true, 8);

    dma_channel_set_config(rx_dma_chan, &rx_dma_config, false);
    dma_channel_set_read_addr(rx_dma_chan, &uart_get_hw(uart0)->dr, false);
    // dma_channel_set_write_addr(rx_dma_chan, rx_data, false);

    // Set up the DMA interrupt handler
    irq_set_exclusive_handler(DMA_IRQ_0, on_dma_complete);
    dma_channel_set_irq0_enabled(rx_dma_chan, true);

    // Enable the DMA interrupt handler
    irq_set_enabled(DMA_IRQ_0, true);

    // OK, all set up.
    // Lets wait for USB stdio to be active, and then run the test loop and report results
    for (int i = 0; i < TRANSFER_SIZE; i++) {
        tx_data[i] = i & 0xFF;
        rx_data[i] = 0;
    }

    while (!stdio_usb_connected())
        tight_loop_contents();

    // Turn on the DMA transfer
    dma_channel_transfer_to_buffer_now(rx_dma_chan, rx_data, TRANSFER_SIZE);

    // Send the data
    uart_write_blocking(uart1, tx_data, TRANSFER_SIZE);

    // Send some more incomplete data
    uart_write_blocking(uart1, tx_data, 23);

    // Wait for everything to be finished
    sleep_ms(100);

    // Report the activity
    printf("%d calls to UART IRQ\n", uart_irq_count);
    printf("%d calls to DMA IRQ\n", dma_irq_count);

    printf("\nRX buffer contains:\n");
    for (int i = 0; i < TRANSFER_SIZE; i += 16) {
        for (int j = 0; j < 16; j++) {
            printf("%02X ", rx_data[i + j]);
        }
        printf("\n");
    }

    while (1)
        tight_loop_contents();
}

#else

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"

#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define UART0_TX_PIN 0
#define UART0_RX_PIN 1

#define UART1_TX_PIN 4
#define UART1_RX_PIN 5

#define TRANSFER_SIZE 256

static volatile int uart_irq_count = 0;
static volatile int dma_irq_count = 0;

static int rx_dma_chan = -1;

// RX中断处理函数（仅响应超时中断）
void on_uart_rx() {
    uart_irq_count++;
    // 清空FIFO中所有剩余数据（处理超时的零散数据）
    printf("on_uart_rx:\n");
    while (uart_is_readable(uart0)) {
        uint8_t ch = uart_getc(uart0);
        // 可选：将超时接收的数据存入缓冲区（示例）
        // rx_data[rx_idx++] = ch;
        printf("%x ", ch);
    }
    // 清除UART0的中断标志（显式清除，避免残留）
    uart_get_hw(uart0)->icr = UART_UARTICR_RTIC_BITS;
}

// DMA中断处理函数
void on_dma_complete() {
    dma_irq_count++;
    // 清除DMA中断标志
    dma_channel_acknowledge_irq0(rx_dma_chan);
}

int main() {
    uint8_t tx_data[TRANSFER_SIZE];
    uint8_t rx_data[TRANSFER_SIZE];

    // 初始化USB串口
    stdio_usb_init();

    // 配置GPIO为UART功能
    gpio_set_function(UART0_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);

    // 初始化UART0/1
    uart_init(uart0, BAUD_RATE);
    uart_init(uart1, BAUD_RATE);

    // 关闭硬件流控
    uart_set_hw_flow(uart0, false, false);
    uart_set_hw_flow(uart1, false, false);

    // 设置UART格式
    uart_set_format(uart0, DATA_BITS, STOP_BITS, PARITY);
    uart_set_format(uart1, DATA_BITS, STOP_BITS, PARITY);

    // 启用FIFO（超时中断依赖FIFO）
    uart_set_fifo_enabled(uart0, true);
    uart_set_fifo_enabled(uart1, true);

    // ====================== 关键修改1：仅配置超时中断 ======================
    // 设置UART0_RX中断处理函数
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);

    // 【核心】仅启用接收超时中断（RTIM），关闭FIFO水位中断（RXIM）
    // 注：FIFO水位可保持默认（1/8，即2字节），不影响超时中断
    uart_get_hw(uart0)->imsc = UART_UARTIMSC_RTIM_BITS; // 只保留RTIM

    // 启用UART0_IRQ
    irq_set_enabled(UART0_IRQ, true);

    // ====================== DMA配置（保持不变） ======================
    rx_dma_chan = dma_claim_unused_channel(true);

    dma_channel_config rx_dma_config = dma_channel_get_default_config(rx_dma_chan);
    channel_config_set_read_increment(&rx_dma_config, false);
    channel_config_set_write_increment(&rx_dma_config, true);
    channel_config_set_dreq(&rx_dma_config, uart_get_dreq(uart0, false));
    channel_config_set_transfer_data_size(&rx_dma_config, DMA_SIZE_8);

    dma_channel_set_config(rx_dma_chan, &rx_dma_config, false);
    dma_channel_set_read_addr(rx_dma_chan, &uart_get_hw(uart0)->dr, false);

    // 配置DMA中断
    irq_set_exclusive_handler(DMA_IRQ_0, on_dma_complete);
    dma_channel_set_irq0_enabled(rx_dma_chan, true);
    irq_set_enabled(DMA_IRQ_0, true);

    // 初始化发送/接收缓冲区
    for (int i = 0; i < TRANSFER_SIZE; i++) {
        tx_data[i] = i & 0xFF;
        rx_data[i] = 0;
    }

    // 等待USB连接
    while (!stdio_usb_connected())
        tight_loop_contents();

    // 启动DMA接收
    dma_channel_transfer_to_buffer_now(rx_dma_chan, rx_data, TRANSFER_SIZE);

    // 发送测试数据
    uart_write_blocking(uart1, tx_data, TRANSFER_SIZE);
    uart_write_blocking(uart1, tx_data, 23);

    // 等待数据接收和中断处理完成
    sleep_ms(100);

    // 打印结果
    printf("%d calls to UART IRQ\n", uart_irq_count);
    printf("%d calls to DMA IRQ\n", dma_irq_count);

    printf("\nRX buffer contains:\n");
    for (int i = 0; i < TRANSFER_SIZE; i += 16) {
        for (int j = 0; j < 16; j++) {
            printf("%02X ", rx_data[i + j]);
        }
        printf("\n");
    }

    while (1)
        tight_loop_contents();
}
#endif