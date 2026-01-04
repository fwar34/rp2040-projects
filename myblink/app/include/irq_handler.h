#ifndef __IRQ_HANDLER_H_
#define __IRQ_HANDLER_H_

typedef void (*IrqHandler)(uint32_t events);

bool RegisterIrq(uint32_t gpio, uint32_t events, bool enabled, IrqHandler handler);
void UnRegisterIrq(uint32_t gpio, uint32_t events);

#endif