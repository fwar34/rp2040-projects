#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/dma.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/async_context_poll.h"

#define UART1_TX 4
#define UART1_RX 5
#define LED1 20
#define LED2 21
#define LED3 22
#define ALL_LED_MASK (1 << LED1 | 1 << LED2 | 1 << LED3)

// Data will be copied from src to dst
const char src[] = "Hello, world! (from DMA)";
char dst[count_of(src)];
static queue_t queue;

static void ReceiveMessageFromCore1(async_context_t *context, __unused struct async_when_pending_worker *worker);
static void PrintCore1MessageToUart0(async_context_t *context, __unused struct async_when_pending_worker *worker);
// static async_when_pending_worker_t workerRecv = {.do_work = ReceiveMessageFromCore1, .work_pending = true};
static async_when_pending_worker_t workerRecv = {.do_work = ReceiveMessageFromCore1};
static async_when_pending_worker_t workerPrint = {.do_work = PrintCore1MessageToUart0};

void LedInit()
{
    gpio_init_mask(ALL_LED_MASK);
    gpio_set_dir_masked(ALL_LED_MASK, GPIO_OUT);
    gpio_put_masked(ALL_LED_MASK, ALL_LED_MASK);
}

void Core1Main()
{
    static uint32_t count = 0;
    static char msg[64] = {0};
    while (true) {
        sprintf(msg, "I'm core1, count:%d\n", count++);
        uart_puts(uart1, msg);
        // gpio_xor_mask(1 << LED1);
        sleep_ms(1000);
    }
}

static async_context_poll_t context;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}

static char message[8];
static void ReceiveMessageFromCore1(async_context_t *context, __unused struct async_when_pending_worker *worker)
{
    uart_read_blocking(uart1, message, 8 - 1);
    message[7] = '\0';
    // printf("try add:%p\n", &message);
    // queue_try_add(&queue, &message);
    // gpio_put(LED3, false);
    // async_context_wait_for_work_ms(context, 500);
    // sleep_ms(500);
    async_context_set_work_pending(context, &workerPrint);
}

static void PrintCore1MessageToUart0(async_context_t *context, __unused struct async_when_pending_worker *worker)
{
    char (*msg)[8] = NULL;
    // printf("queue count:%d\n", queue_get_level(&queue));
    printf("PrintCore1MessageToUart0: %s\n", message);
    // while (!queue_is_empty(&queue)) {
    //     if (queue_try_remove(&queue, &msg)) {
    //         printf("try remove %p\n", msg);
    //     }
    // }
    // gpio_put(LED3, true);
    // async_context_wait_for_work_ms(context, 500);
    // sleep_ms(500);
    async_context_set_work_pending(context, &workerRecv);
}

int main()
{
    stdio_init_all();

    LedInit();
    uart_init(uart1, PICO_DEFAULT_UART_BAUD_RATE);
    gpio_set_function_masked(1 << UART1_TX | 1 << UART1_RX, GPIO_FUNC_UART);

    // Get a free channel, panic() if there are none
    int chan = dma_claim_unused_channel(true);
    
    // 8 bit transfers. Both read and write address increment after each
    // transfer (each pointing to a location in src or dst respectively).
    // No DREQ is selected, so the DMA transfers as fast as it can.
    
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);
    
    dma_channel_configure(
        chan,          // Channel to be configured
        &c,            // The configuration we just created
        dst,           // The initial write address
        src,           // The initial read address
        count_of(src), // Number of transfers; in this case each is 1 byte.
        true           // Start immediately.
    );
    
    // We could choose to go and do something else whilst the DMA is doing its
    // thing. In this case the processor has nothing else to do, so we just
    // wait for the DMA to finish.
    dma_channel_wait_for_finish_blocking(chan);
    
    // The DMA has now copied our text from the transmit buffer (src) to the
    // receive buffer (dst), so we can print it out from there.
    puts(dst);

    // Timer example code - This example fires off the callback after 2000ms
    // add_alarm_in_ms(2000, alarm_callback, NULL, false);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    if (!async_context_poll_init_with_defaults(&context)) {
        printf("create async_context_poll_t failed!");
        return -1;
    }

    queue_init(&queue, sizeof(char **), 16);
    async_context_add_when_pending_worker(&context.core, &workerRecv); 
    async_context_add_when_pending_worker(&context.core, &workerPrint); 
    async_context_set_work_pending(&context.core, &workerRecv);

    multicore_launch_core1(Core1Main);

    while (true) {
        printf("main running!\n");
        async_context_poll(&context.core);
        async_context_wait_for_work_ms(&context.core, 1000);
    }
}
