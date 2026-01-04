#include "input_process.h"
#include "ec11.h"
#include "dht11.h"
#include "st7735s.h"
#include <stdio.h>
#include <string.h>

#define DHT11_TICK_MS 1500
typedef struct {
    QActive super;
    QTimeEvt dht11ResultDisplay;
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
    static char display_buffer[48] = {0};
    switch (inputEvent->key)
    {
    case EC11_KEY_CONTINUE_CLICK:
        sprintf(display_buffer, "click,count:%d", inputEvent->data.kickCount);
        LCD_ShowCharStr_DMA_Optimized(0, 0, LCD_WIDTH, display_buffer, RGB565_ORANGE, RGB565_BLUE, 16);
        break;
    case EC11_KEY_LONG_CLICK:
        sprintf(display_buffer, "long click");
        LCD_ShowCharStr_DMA_Optimized(0, 0, LCD_WIDTH, display_buffer, RGB565_ORANGE, RGB565_BLUE, 16);
        break;
    case EC11_KEY_LEFT_ROTATE:
        sprintf(display_buffer, "left,cur:%d", inputEvent->data.rotatePosition);
        LCD_ShowCharStr_DMA_Optimized(0, 0, LCD_WIDTH, display_buffer, RGB565_ORANGE, RGB565_BLUE, 16);
        break;
    case EC11_KEY_RIGHT_ROTATE:
        sprintf(display_buffer, "right,cur:%d", inputEvent->data.rotatePosition);
        LCD_ShowCharStr_DMA_Optimized(0, 0, LCD_WIDTH, display_buffer, RGB565_ORANGE, RGB565_BLUE, 16);
        break;
    case EC11_KEY_PRESS_RIGHT_ROTATE:
        break;
    case EC11_DEBUG:
    {
        // sprintf(display_buffer, "ec11 min:%d", inputEvent->data.qeueueMin);
        // LCD_ShowCharStr_DMA_Optimized(0, 16, LCD_WIDTH, display_buffer, RGB565_GRAY_0, RGB565_YELLOW, 16);
        
        break;
    }
    default:
        printf("unknow ec11 keyState:%d\n", inputEvent->key);
        break;
    }
}

void Dht11ResultDisplay()
{
    Dht11Result result = *GetDht11Result();
    printf("humidity: %.2f, dht11: %.2f\n", result.humidity, result.temperature);

    char number_buffer[128] = {0};
    sprintf(number_buffer, "humidity:%.2f", result.humidity);
    LCD_ShowCharStr_DMA_Optimized(0, 32, LCD_WIDTH, number_buffer, RGB565_BLUE, RGB565_RED, 16);

    // memset(number_buffer, 0, sizeof(number_buffer));
    sprintf(number_buffer, "temperature:%.2f", result.temperature);
    LCD_ShowCharStr_DMA_Optimized(0, 48, LCD_WIDTH, number_buffer, RGB565_YELLOW, RGB565_BLUE, 16);
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
    case SIGNAL_DHT11_RESULT_DISPLAY:
        Dht11ResultDisplay();
        status = Q_HANDLED();
        break;
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
    QTimeEvt_ctorX(&g_Instance.dht11ResultDisplay, &g_Instance.super, SIGNAL_DHT11_RESULT_DISPLAY, 0U);
    QTimeEvt_armX(&g_Instance.dht11ResultDisplay, DHT11_TICK_MS, DHT11_TICK_MS);
    return Q_TRAN(InputProcessRun);
}
