/**
 * @file uart1.c
 * @author your name (you@domain.com)
 * @brief uart空闲中断+dma半满、全满中断+dma+ring+双buffer+circlebuffer实现串口接收
 * 在主逻辑中启动定时器来模拟uart空闲中断
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
#include <stdio.h>
#include <string.h>
#include <math.h>

// 2^10 = 1024，dma ringbuffer使用
#define UART_RX_BUFFER_POW_BITS (5UL)
#define UART_RX_BUFFER_LEN (1 << UART_RX_BUFFER_POW_BITS)
#define HALF_UART_RX_BUFFER_LEN (UART_RX_BUFFER_LEN >> 1)

#define UART uart1
#define UART_TX_PIN (4UL)
#define UART_RX_PIN (5UL)

#define UART_RX_IDLE_JUDGE_INTERVAL 20

static uint8_t circle_buf[UART_RX_BUFFER_LEN << 1];
static uint32_t total_count = 0;
static uint32_t idleCount = 0;
static uint32_t irqCount = 0;
static uint32_t rxDataTimes = 0;

typedef struct
{
    uint8_t uartRxBuffer[UART_RX_BUFFER_LEN] __aligned(UART_RX_BUFFER_LEN);
    int dmaChannel;
} Uart1Dev;

typedef struct {
    QActive super;
    QTimeEvt rxIdleJudgeEvt;
	Uart1Dev uartDev;
    uint32_t totalCount;
} UartProcess;

static UartProcess g_Instance;
QActive *g_UartProcess = &g_Instance.super;

static QState UartProcessInit(UartProcess *me, const void *arg);
void UartProcessCtor(void)
{
    UartProcess *uartProcess = &g_Instance;
    QActive_ctor(&uartProcess->super, Q_STATE_CAST(UartProcessInit));
}

static void StopDMA()
{
    dma_hw->abort = 1u << g_Instance.uartDev.dmaChannel; // 触发 DMA 中断
}

static void PrintResult(const char *msg, uint32_t tCount)
{
    uint32_t writeAddr = dma_channel_hw_addr(g_Instance.uartDev.dmaChannel)->write_addr;
    printf("%s idleCount:%u, base:%d, waddr:%d, tCount:%d\n",
           msg, idleCount, (uintptr_t)g_Instance.uartDev.uartRxBuffer, writeAddr, tCount);
}

static uint32_t GetRecvSize()
{
    uint32_t recvSize = HALF_UART_RX_BUFFER_LEN;
    uint32_t rxAddr = (uintptr_t)g_Instance.uartDev.uartRxBuffer;
    uint32_t writeAddr = dma_channel_hw_addr(g_Instance.uartDev.dmaChannel)->write_addr;
    if (writeAddr == rxAddr) { // 后半部分接收满
        recvSize = HALF_UART_RX_BUFFER_LEN;
    } else if (writeAddr == rxAddr + HALF_UART_RX_BUFFER_LEN) { // 前半部分接收满
        recvSize = HALF_UART_RX_BUFFER_LEN;
    } else if (writeAddr > rxAddr + HALF_UART_RX_BUFFER_LEN) { // 后半部分接收未满
        recvSize = writeAddr - (rxAddr + HALF_UART_RX_BUFFER_LEN);
    } else if (writeAddr > rxAddr) { // 前半部分接收未满
        recvSize = writeAddr - rxAddr;
    }
    return recvSize;
}
static void UartRxIdle()
{
    static uint32_t rxLastWriteIndex = 0;
    static uint32_t lastTime = 0;
    uintptr_t rxAddr = (uintptr_t)g_Instance.uartDev.uartRxBuffer;
    uintptr_t writeAddr = dma_channel_hw_addr(g_Instance.uartDev.dmaChannel)->write_addr;
    if (writeAddr == rxLastWriteIndex && writeAddr != rxAddr && (writeAddr != rxAddr + HALF_UART_RX_BUFFER_LEN)) {
        idleCount++;
        total_count += GetRecvSize();
        PrintResult("abort", total_count);
        printf("abort time use:%d\n", time_us_32() - lastTime);
        StopDMA();
        sleep_us(100);
        total_count = 0;
        uint32_t bit = log2(UART_RX_BUFFER_LEN);
        printf("log2(%d)=%d\n", UART_RX_BUFFER_LEN, bit);
    }

    if (writeAddr == rxLastWriteIndex && (writeAddr == rxAddr || writeAddr == rxAddr + HALF_UART_RX_BUFFER_LEN) && total_count > 0) {
        PrintResult("normal", total_count);
        printf("normal time use:%d\n", time_us_32() - lastTime);
        total_count = 0;
    }
    rxLastWriteIndex = writeAddr;
    lastTime = time_us_32();
}

static void UartRxEventProcess(UartRxEvent *e)
{
    g_Instance.totalCount += e->rxDataLen;
    // printf("add %d, rxTotal:%d\n", e->rxDataLen, g_Instance.totalCount);
}

static QState UartProcessRun(UartProcess *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case SIGNAL_RX_DATA:
        rxDataTimes++;
        UartRxEvent *uartRxEvt = (UartRxEvent *)e;
        UartRxEventProcess(uartRxEvt);
        status = Q_HANDLED();
        break;
    case SIGNAL_RX_IDLE_TIMEOUT:
        UartRxIdle();
        static uint32_t ix = 0;
        static uint32_t lastTCount = 0;
        if (ix++ % 300 == 0) {
            printf("rxTotal:%d\n", g_Instance.totalCount);
            if (lastTCount == g_Instance.totalCount) {
                g_Instance.totalCount = 0;
                printf("set rxTotal to 0\n");
            }
        }
        lastTCount = g_Instance.totalCount;
        status = Q_HANDLED();
        break;
    default:
        status = Q_SUPER(QHsm_top);
        break;
    }

    return status;
}

static void DmaHalfIrqHandler(void)
{
	if (!dma_channel_get_irq1_status(g_Instance.uartDev.dmaChannel)) {
        return;
    }
    dma_channel_acknowledge_irq1(g_Instance.uartDev.dmaChannel);
    irqCount++;

    uint32_t recvSize = 0;
    uint8_t *copySrcAddr = NULL;
    uint32_t writeAddr = dma_channel_hw_addr(g_Instance.uartDev.dmaChannel)->write_addr;
    uint32_t rxAddr = (uintptr_t)g_Instance.uartDev.uartRxBuffer;
    if (writeAddr == rxAddr) { // 后半部分接收满
        copySrcAddr = g_Instance.uartDev.uartRxBuffer + HALF_UART_RX_BUFFER_LEN; 
        recvSize = HALF_UART_RX_BUFFER_LEN;
    } else if (writeAddr == rxAddr + HALF_UART_RX_BUFFER_LEN) { // 前半部分接收满
        copySrcAddr = g_Instance.uartDev.uartRxBuffer;
        recvSize = HALF_UART_RX_BUFFER_LEN;
    } else if (writeAddr > rxAddr + HALF_UART_RX_BUFFER_LEN) { // 后半部分接收未满
        copySrcAddr = g_Instance.uartDev.uartRxBuffer + HALF_UART_RX_BUFFER_LEN;
        recvSize = writeAddr - (rxAddr + HALF_UART_RX_BUFFER_LEN);
        dma_channel_set_write_addr(g_Instance.uartDev.dmaChannel, g_Instance.uartDev.uartRxBuffer, false);
    } else if (writeAddr > rxAddr) { // 前半部分接收未满
        copySrcAddr = g_Instance.uartDev.uartRxBuffer;
        recvSize = writeAddr - rxAddr;
        dma_channel_set_write_addr(g_Instance.uartDev.dmaChannel, g_Instance.uartDev.uartRxBuffer + HALF_UART_RX_BUFFER_LEN, false);
    }

    // **关键：重新启动环形 DMA 传输**
    // 这样下一个 RX_BUF_SIZE 数据到达时会再次触发中断
    // dma_channel_set_write_addr(g_Instance.uartDev.dmaChannel, g_Instance.uartDev.uartRxBuffer, false);
    dma_channel_set_trans_count(g_Instance.uartDev.dmaChannel, HALF_UART_RX_BUFFER_LEN, true);

    // memcpy(circle_buf, copySrcAddr, recvSize);
    total_count += recvSize;

    UartRxEvent *e = Q_NEW(UartRxEvent, SIGNAL_RX_DATA);
    e->rxDataLen = recvSize;
    QACTIVE_POST(g_UartProcess, &e->super, &g_Instance.super);
}

static void Uart1initDma(void)
{
	// 获取空闲 DMA 通道
	int dmaChannel = dma_claim_unused_channel(true);
	dma_channel_config config = dma_channel_get_default_config(dmaChannel);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
	channel_config_set_read_increment(&config, false);
    channel_config_set_write_increment(&config, true);
    // 将写地址设置为环形缓冲区，大小为 2^UART_RX_BUFFER_POW_BITS 字节
    channel_config_set_ring(&config, true, UART_RX_BUFFER_POW_BITS);
	channel_config_set_dreq(&config, uart_get_dreq(UART, false));
    channel_config_set_enable(&config, true);
    dma_channel_configure(dmaChannel, 
        &config, 
        g_Instance.uartDev.uartRxBuffer,  // 读取到的数据写入到 uart buffer
        &uart_get_hw(UART)->dr,  // 从 uart 的 数据寄存器中读取
        HALF_UART_RX_BUFFER_LEN,  // dma 读取数据达到 uart buffer 的（1024）时候触发中断
        true);
	g_Instance.uartDev.dmaChannel = dmaChannel;

	// 设置dma结束的中断
	dma_channel_set_irq1_enabled(dmaChannel, true);
	irq_set_exclusive_handler(DMA_IRQ_1, DmaHalfIrqHandler);
	irq_set_enabled(DMA_IRQ_1, true);
    
    uint32_t curWriteAddr = dma_channel_hw_addr(g_Instance.uartDev.dmaChannel)->write_addr;
    printf("Uart1InitDma curWriteAddr:%d, rxBufferAddress:%d\n", curWriteAddr, (uintptr_t)g_Instance.uartDev.uartRxBuffer);
}

QState UartProcessInit(UartProcess *me, const void *arg)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(arg);
	gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
	gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
	uart_init(UART, PICO_DEFAULT_UART_BAUD_RATE);
    uart_set_hw_flow(UART, false, false);
    // 禁用UART FIFO，让数据直接进入DR寄存器（适合DMA）
    uart_set_fifo_enabled(UART, false);

	Uart1initDma();

    me->totalCount = 0U;
    QTimeEvt_ctorX(&g_Instance.rxIdleJudgeEvt, &g_Instance.super, SIGNAL_RX_IDLE_TIMEOUT, 0U);
    QTimeEvt_armX(&g_Instance.rxIdleJudgeEvt, UART_RX_IDLE_JUDGE_INTERVAL, UART_RX_IDLE_JUDGE_INTERVAL);
    return Q_TRAN(UartProcessRun);
}