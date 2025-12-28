#ifndef BSP_H_
#define BSP_H_

#include <stdint.h>
#include "qpc.h"

void BSP_init(void);
void BSP_start(void);
void BSP_terminate(int16_t result);

void BSP_ledOn(void);
void BSP_ledOff(void);

#endif // BSP_H_