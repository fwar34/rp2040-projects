#include "dht11.h"
#include "ec11.h"
#include "irq_handler.h"
#include "hardware/gpio.h"

#define IRQ_MAX_COUNT (30UL)

typedef struct {
    IrqHandler handler;
} IrqInfo;
static IrqInfo g_IrqInfos[30] = {0};

static void GpioIrqHandler(uint gpio, uint32_t event_mask)
{
    IrqInfo *irqInfo = &g_IrqInfos[gpio];
    if (irqInfo->handler) {
        irqInfo->handler(event_mask);
    }
}
bool RegisterIrq(uint32_t gpio, uint32_t events, bool enabled, IrqHandler handler)
{
    if (gpio >= IRQ_MAX_COUNT) {
        return false;
    }

    gpio_set_irq_enabled_with_callback(gpio, events, enabled, GpioIrqHandler);
    g_IrqInfos[gpio].handler = handler;

    return true;
}

void UnRegisterIrq(uint32_t gpio, uint32_t events)
{
    if (gpio >= IRQ_MAX_COUNT) {
        return;
    }

    g_IrqInfos[gpio].handler = NULL;
    gpio_set_irq_enabled(gpio, events, false);
}
