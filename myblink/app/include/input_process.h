#ifndef __INPUT_PROCESS_H_
#define __INPUT_PROCESS_H_

#include "qpc.h"
#include "signals.h"

typedef struct {
    QEvt super;
    uint32_t key;
    union {
        uint8_t kickCount;
        int16_t rotatePosition;
        uint8_t qeueueMin;
    } data;
} InputEvent;

void InputProcessCtor(void);
extern QActive *g_InputProcess;

#endif // !__INPUT_PROCESS_H_