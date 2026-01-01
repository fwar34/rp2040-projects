#include "input_process.h"
#include "ec11.h"
#include "st7735s.h"
#include <stdio.h>

typedef struct {
    QActive super;
    QTimeEvt timeEvt;
} InputProcess;

static InputProcess g_Instance;
QActive *g_InputProcess = &g_Instance.super;

static QState InputProcessInit(InputProcess *me, const void *arg);
void InputProcessCtor(void)
{
    InputProcess *inputProcess = &g_Instance;
    QActive_ctor(&inputProcess->super, Q_STATE_CAST(InputProcessInit));
}

static void Ec11EventProcess(InputEvent *inputEvent)
{
    // HAL_GPIO_WritePin(testChannel2_GPIO_Port, testChannel2_Pin, GPIO_PIN_SET);
    char display_buffer[48] = {0};
    sprintf(display_buffer, "recv:%s", GetKeyEventName(inputEvent->key));
    LCD_ShowCharStr_DMA_Optimized(0, 0, LCD_WIDTH, display_buffer, RGB565_ORANGE, RGB565_BLUE, 16);

    switch (inputEvent->key)
    {
    case EC11_KEY_CONTINUE_CLICK:
        if (inputEvent->data.kickCount == 1) {
            // HAL_GPIO_TogglePin(led_red_GPIO_Port, led_red_Pin);
        } else {
            // HAL_GPIO_TogglePin(led_yellow_GPIO_Port, led_yellow_Pin);
        }
        break;
    case EC11_KEY_LEFT_ROTATE:
        break;
    case EC11_KEY_RIGHT_ROTATE:
        break;
    case EC11_KEY_PRESS_LEFT_ROTATE:
        break;
    case EC11_KEY_PRESS_RIGHT_ROTATE:
        break;
    case EC11_DEBUG: {
        // sprintf(display_buffer, "ec11 min:%d", inputEvent->data.qeueueMin);
        // LCD_ShowCharStr_DMA_Optimized(0, 16, LCD_WIDTH, display_buffer, RGB565_GRAY_0, RGB565_YELLOW, 16);
        
        break;
    }
    default:
        printf("unknow ec11 keyState:%d\n", inputEvent->key);
        break;
    }
    // HAL_GPIO_WritePin(testChannel2_GPIO_Port, testChannel2_Pin, GPIO_PIN_RESET);
}

static QState InputProcessRun(InputProcess *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case SIGNAL_INPUT:
    {
        InputEvent *inputEvent = (InputEvent *)e;
        Ec11EventProcess(inputEvent);
        status = Q_HANDLED();
        break;
    }
    default:
        status = Q_SUPER(QHsm_top);
        break;
    }

    return status;
}

QState InputProcessInit(InputProcess *me, const void *arg)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(arg);
    return Q_TRAN(InputProcessRun);
}
