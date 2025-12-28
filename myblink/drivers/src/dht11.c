#include "dht11.h"
#include "qpc.h"
#include "bsp.h"
#include "logic.h"
#include "signals.h"
#include "pico/stdlib.h"
#include "util_print.h"
#include "st7735s.h"
// #include "lcd.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define DHT11_PIN 10

#define DHT11_MASTER_TICK_MS 20
#define DHT11_TICK_MS 1500
#define DHT11_DATA_BIT_THRESHOLD_50US 50
#define DHT11_DATA_BIT_NUM_OF_BYTE_8 8
#define DHT11_DATA_BYTE_NUM_5 5
#define DHT11_MASTER_HIGH_US 25
#define DHT11_DATA_ERROR_THRESHOLD_US 255U

#define DHT11_WRITE_HIGH() (gpio_put(DHT11_PIN, true))
#define DHT11_WRITE_LOW() (gpio_put(DHT11_PIN, false))
#define DHT11_READ_HIGH() (gpio_get(DHT11_PIN) == true)
#define DHT11_READ_LOW() (gpio_get(DHT11_PIN) == false)

typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    Dht11Result result;
} Dht11;

static Dht11 g_instance = {0};
QActive *g_Dht11 = &g_instance.super;

static QState Dht11Init(Dht11 *me, const void *arg);
static QState Dht11MeasureProcess(Dht11 *me, const QEvt *e);
static QState Dht11Complete(Dht11 *me, const QEvt *e);

void Dht11Ctor(void)
{
    Dht11 *dht11 = &g_instance;
    QActive_ctor(&dht11->super, Q_STATE_CAST(&Dht11Init));
    QTimeEvt_ctorX(&dht11->timeEvt, &dht11->super, SIGNAL_TIMEOUT, 0U);
}

QState Dht11Init(Dht11 *me, const void *arg)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(arg);
    gpio_init(DHT11_PIN);
    return Q_TRAN(&Dht11MeasureProcess);
}

static bool CheckSum(char *data)
{
	return (data[0] + data[1] + data[2] + data[3]) == data[4];
}

static void Dht11Output()
{
    gpio_set_dir(DHT11_PIN, GPIO_OUT);
}

static void Dht11Input()
{
    gpio_set_dir(DHT11_PIN, GPIO_IN);
}


QState Dht11MeasureProcess(Dht11 *me, const QEvt *e)
{
    QState status;
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        Dht11Output();
        DHT11_WRITE_HIGH();
        DHT11_WRITE_LOW();
        QTimeEvt_armX(&me->timeEvt, DHT11_MASTER_TICK_MS, 0);
        status = Q_HANDLED();
        break;
    case SIGNAL_TIMEOUT:
        QF_INT_DISABLE();
        DHT11_WRITE_HIGH();
        uint32_t timer3Tick = time_us_32();
        uint32_t timeNow = 0;
        for (;;) {
            timeNow = time_us_32();
            if (timeNow - timer3Tick > DHT11_MASTER_HIGH_US) { // 主机开始信号发送完成后拉高延时等待从机
                break;
            }
        }
        // sleep_us(DHT11_MASTER_HIGH_US); // 关中的情况下无法使用此函数
        Dht11Input();

        timer3Tick = time_us_32();
        while (DHT11_READ_LOW()) {
            if (time_us_32() - timer3Tick > DHT11_DATA_ERROR_THRESHOLD_US) {
                QF_INT_ENABLE();
                printf("dth11 return 1\n");
                return Q_TRAN(&Dht11Complete);
            }
        }

        timer3Tick = time_us_32();
        while (DHT11_READ_HIGH()) {
            if (time_us_32() - timer3Tick > DHT11_DATA_ERROR_THRESHOLD_US) {
                QF_INT_ENABLE();
                printf("dth11 return 2\n");
                return Q_TRAN(&Dht11Complete);
            }
        }

        char data[DHT11_DATA_BYTE_NUM_5] = {0};
        for (uint8_t i = 0; i < DHT11_DATA_BYTE_NUM_5; i++) {
            for (uint8_t j = 0; j < DHT11_DATA_BIT_NUM_OF_BYTE_8; j++) {
                uint32_t timer3Tick = time_us_32();
                while (DHT11_READ_LOW()) {
                    if (time_us_32() - timer3Tick > DHT11_DATA_ERROR_THRESHOLD_US) {
                        QF_INT_ENABLE();
                        printf("dth11 return 3\n");
                        return Q_TRAN(&Dht11Complete);
                    }
                }

                timer3Tick = time_us_32();
                while (DHT11_READ_HIGH()) {
                    if (time_us_32() - timer3Tick > DHT11_DATA_ERROR_THRESHOLD_US) {
                        QF_INT_ENABLE();
                        printf("dth11 return 4\n");
                        return Q_TRAN(&Dht11Complete);
                    }
                }
                
                if (time_us_32() - timer3Tick > DHT11_DATA_BIT_THRESHOLD_50US) {
                    data[i] |= 1 << (DHT11_DATA_BIT_NUM_OF_BYTE_8 - j - 1); // 先读取出来的是 MSB
                }
            }
        }
        QF_INT_ENABLE();

        // LogicUp();
        uint32_t time1 = time_us_32();
        if (CheckSum(data)) {
            me->result.humidity = (float)(data[0]) + (float)(data[1]) / 100;
            me->result.temperature = (float)(data[2]) + (float)(data[3]) / 100;

            printf("humidity: %.2f, dht11: %.2f\n", me->result.humidity, me->result.temperature);
            
            char number_buffer[128] = {0};
            sprintf(number_buffer, "humidity:%.2f", me->result.humidity);
            LCD_ShowCharStr_DMA_Optimized(0, 32, LCD_WIDTH, number_buffer, RGB565_BLUE, RGB565_RED, 16);
            
            memset(number_buffer, 0 , sizeof(number_buffer));
            sprintf(number_buffer, "temperature:%.2f", me->result.temperature);
            LCD_ShowCharStr_DMA_Optimized(0, 48, LCD_WIDTH, number_buffer, RGB565_YELLOW, RGB565_BLUE, 16);
        }
        uint32_t duration = time_us_32() - time1;
        printf("dht11 display duration:%d\n", duration);
        // LogicDown();

        // LcdDrawBlock(20, 10, 60, 20, RGB565_BLUE);
        // sleep_ms(2000);
        // LcdDrawBlock(20, 10, 60, 20, RGB565_YELLOW);
        // LcdClear(RGB565_RED);
        // sleep_ms(2000);
        // LcdClear(RGB565_GREEN);
        // TftClear(RGB565_GREEN);
        // sleep_ms(1500);
        // TftClear(RGB565_YELLOW);

        status = Q_TRAN(Dht11Complete);
        break;
    case Q_EXIT_SIG:
        // StopTimer3();
        status = Q_HANDLED();
        break;
    default:
        status = Q_SUPER(&QHsm_top);
        break;
    }

    return status;
}

QState Dht11Complete(Dht11 *me, const QEvt *e)
{
    QState status;
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        Dht11Output();
        QTimeEvt_armX(&me->timeEvt, DHT11_TICK_MS, 0);
        status = Q_HANDLED();
        break;
    case SIGNAL_TIMEOUT:
        status = Q_TRAN(Dht11MeasureProcess);
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->timeEvt);
        status = Q_HANDLED();
        break;
    default:
        status = Q_SUPER(&QHsm_top);
        break;
    }

    return status;
}

Dht11Result *GetDht11Result()
{
    return &g_instance.result;
}