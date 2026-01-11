#ifndef __UART1_H_
#define __UART1_H_

#include "qpc.h"
#include "signals.h"

typedef struct {
    QEvt super;
    uint32_t rxDataLen;
} UartRxEvent;

void UartProcessCtor(void);
extern QActive *g_UartProcess;

#endif