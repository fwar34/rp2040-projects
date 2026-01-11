#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/dma.h"
#include "pico/time.h"
#include "hardware/regs/uart.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// 配置项
#define UART_ID uart1
// #define BAUD_RATE 38400
#define BAUD_RATE 115200
#define UART_RX_PIN 5   // GP5 RX for uart1 (请根据你的硬件改)
#define RX_BUF_SIZE 256 // 必须为 2^N，用于 channel_config_set_ring 的 N

// DMA / buffer 状态
static uint8_t rx_buf[RX_BUF_SIZE];
static int rx_chan;
static uint8_t circle_buf[RX_BUF_SIZE << 1];
static uint32_t total_count = 0;
static uint32_t idleCount = 0;

static void PrintResult(const char *msg)
{
    uint32_t writeAddr = dma_channel_hw_addr(rx_chan)->write_addr;
    printf("%s idleCount:%u, base:%d, waddr:%d, idx:%d, tCount:%d\n",
           msg, idleCount, (uintptr_t)rx_buf, writeAddr, 
           writeAddr - (uintptr_t)rx_buf, total_count);
}

static void UartRxIdle()
{
    static uint32_t rxLastWriteIndex = 0;
    static uint32_t lastTime = 0;
    uintptr_t base = (uintptr_t)rx_buf;
    uintptr_t curWriteAddr = dma_channel_hw_addr(rx_chan)->write_addr;
    if (curWriteAddr == rxLastWriteIndex && curWriteAddr > base) {
        idleCount++;
        dma_hw->abort = 1u << rx_chan;
        PrintResult("abort");
        printf("time use:%d\n", time_us_32() - lastTime);
        total_count = 0;
    }

    if (curWriteAddr == rxLastWriteIndex && curWriteAddr == base && total_count > 0) {
        PrintResult("normal");
        printf("time use:%d\n", time_us_32() - lastTime);
        total_count = 0;
    }
    rxLastWriteIndex = curWriteAddr;
    lastTime = time_us_32();
}

void dma_uart_rx_irq_handler(void) {
    // 用 SDK 提供的函数清中断（这是正确的方式）
    if (rx_chan < 8) {
        dma_channel_acknowledge_irq0(rx_chan);
    } else {
        dma_channel_acknowledge_irq1(rx_chan);
    }
    uint32_t writeAddr = dma_channel_hw_addr(rx_chan)->write_addr;
    // printf("irq wAddr:%d\n", writeAddr);
    // 此处缓冲区已被填满 RX_BUF_SIZE 字节一次
    // 处理缓冲中的数据（示例：把数据写回 USB 串口）
    // 注意：中断中不要做耗时操作，必要时将数据拷贝到另一个队列/缓冲并在主循环处理
    uint32_t recvSize = (writeAddr == (uintptr_t)rx_buf ? RX_BUF_SIZE : (writeAddr - (uintptr_t)rx_buf));
    memcpy(circle_buf, rx_buf, recvSize);
    total_count += recvSize;

    // **关键：重新启动环形 DMA 传输**
    // 这样下一个 RX_BUF_SIZE 数据到达时会再次触发中断
    dma_channel_set_write_addr(rx_chan, rx_buf, false);
    dma_channel_set_trans_count(rx_chan, RX_BUF_SIZE, true);
}

void uart_dma_rx_init() {
    // 初始化 UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // 如果需要 TX，也设置 TX 引脚：
    // gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    // uart_set_fifo_enabled(UART_ID, false);
    uart_set_hw_flow(UART_ID, false, false);

    printf("rx addr:%d\n", rx_buf);
    // 获取一个可用的 DMA 通道
    rx_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(rx_chan);

    // 数据宽度为 8 位
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);

    // 从周边读取（UART RX 寄存器），外设地址不递增
    channel_config_set_read_increment(&cfg, false);

    // 写入内存，写地址需要递增
    channel_config_set_write_increment(&cfg, true);

    // 绑定 DREQ 到 UART0 RX。这样 DMA 会根据 UART RX FIFO 的 DREQ 触发传输
    channel_config_set_dreq(&cfg, uart_get_dreq(UART_ID, false)); // false 表示 RX

    // 配置为环形（ring）缓冲：启用 + 指数 N，使写指针在 2^N 字节边界回绕
    // 这里传入的 7 表示 2^7 = 128 bytes 环形大小（与 RX_BUF_SIZE 对应）
    // channel_config_set_ring(&cfg, true, log2(RX_BUF_SIZE));

    // 不在此处直接启用 IRQ0；后面会根据分配的 channel 选择 IRQ0 或 IRQ1 并启用

    // 配置DMA传输：外设地址 = UART DR 寄存器地址，内存地址 = rx_buf，传输计数 = RX_BUF_SIZE
    // 启动（start=true）后 DMA 会在环形模式下持续工作，每次填满 RX_BUF_SIZE 时产生一次中断
    dma_channel_configure(
        rx_chan,
        &cfg,
        rx_buf,                             // dst (memory)
        &uart_get_hw(UART_ID)->dr,          // src (peripheral data register)
        RX_BUF_SIZE,                        // transfer count
        true                                 // start immediately
    );

    // 根据分配的 channel 选择 IRQ0 或 IRQ1，并启用相应中断
    if (rx_chan < 8) {
        dma_channel_set_irq0_enabled(rx_chan, true);
        irq_set_exclusive_handler(DMA_IRQ_0, dma_uart_rx_irq_handler);
        irq_set_enabled(DMA_IRQ_0, true);
    } else {
        dma_channel_set_irq1_enabled(rx_chan, true);
        irq_set_exclusive_handler(DMA_IRQ_1, dma_uart_rx_irq_handler);
        irq_set_enabled(DMA_IRQ_1, true);
    }
}

int main() {
    stdio_init_all(); // 为 putchar_raw() 等提供USB串口输出（如果需要）
    uart_dma_rx_init();

    // 主循环可以做其他工作
    while (1) {
        // 推荐：在主循环内处理非关键路径数据，或 sleep
        UartRxIdle();
        sleep_ms(2);
    }
}