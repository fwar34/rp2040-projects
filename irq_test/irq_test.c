#if 0
/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define GPIO_WATCH_PIN 19
#define GPIO_WATCH_PIN28 28
#define GPIO_WATCH_PIN29 29

static char event_str[128];

void gpio_event_string(char *buf, uint32_t events);

void gpio_callback28(uint gpio, uint32_t events) {
    if (gpio == GPIO_WATCH_PIN28) {
        printf("irq for 28\n");
        return;
    }
}

void gpio_callback29(uint gpio, uint32_t events) {
    if (gpio == GPIO_WATCH_PIN29) {
        printf("irq for 29\n");
        return;
    } else if (gpio == GPIO_WATCH_PIN28) {
        printf("irq for 28\n");
        return;
    } else if (gpio ==  GPIO_WATCH_PIN) {
        printf("irq for 19\n");
        return;
    }
}

void gpio_callback(uint gpio, uint32_t events) {
    // Put the GPIO event(s) that just happened into event_str
    // so we can print it
    gpio_event_string(event_str, events);
    printf("GPIO %d %s\n", gpio, event_str);
}

int main() {
    stdio_init_all();

    printf("Hello GPIO IRQ\n");
    gpio_init(GPIO_WATCH_PIN);
    gpio_init(GPIO_WATCH_PIN28);
    gpio_init(GPIO_WATCH_PIN29);
    gpio_set_irq_enabled_with_callback(GPIO_WATCH_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(GPIO_WATCH_PIN28, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback28);
    gpio_set_irq_enabled_with_callback(GPIO_WATCH_PIN29, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback29);

    // Wait forever
    while (1);
}


static const char *gpio_irq_str[] = {
        "LEVEL_LOW",  // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL",  // 0x4
        "EDGE_RISE"   // 0x8
};

void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            // Copy this event string into the user string
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
            events &= ~mask;

            // If more events add ", "
            if (events) {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf++ = '\0';
}
#else
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "stdio.h"

// 定义中断引脚（可根据需要修改）
#define INTERRUPT_PIN_1 19
#define INTERRUPT_PIN_2 29

/**
 * @brief GPIO中断处理函数（所有GPIO中断都会进入这个函数）
 * @param gpio 触发中断的GPIO引脚号
 * @param events 触发的事件类型（上升沿/下降沿等）
 */
void gpio_interrupt_handler1(uint gpio, uint32_t events) {
    // 关中断（可选，防止同一引脚短时间多次触发）
    // gpio_set_irq_enabled(gpio, events, false);

    // 判断是哪个引脚触发的中断
    if (gpio == INTERRUPT_PIN_1) {
        printf("irq:GPIO%d, event:%d\n", INTERRUPT_PIN_1, events);
    } else if (gpio == INTERRUPT_PIN_2) {
        printf("irq:GPIO%d, event:%d\n", INTERRUPT_PIN_2, events);
    }

    // 简单消抖（20ms）
    // sleep_ms(20);
    
    // 重新启用中断
    // gpio_set_irq_enabled(gpio, events, true);
}

void gpio_interrupt_handler2(uint gpio, uint32_t events) {
    // 关中断（可选，防止同一引脚短时间多次触发）
    // gpio_set_irq_enabled(gpio, events, false);

    // 判断是哪个引脚触发的中断
    if (gpio == INTERRUPT_PIN_1) {
        printf("irq:GPIO%d, event:%d\n", INTERRUPT_PIN_1, events);
    } else if (gpio == INTERRUPT_PIN_2) {
        printf("irq:GPIO%d, event:%d\n", INTERRUPT_PIN_2, events);
    }

    // 简单消抖（20ms）
    // sleep_ms(20);
    
    // 重新启用中断
    // gpio_set_irq_enabled(gpio, events, true);
}

/**
 * @brief 初始化GPIO中断
 * @param pin GPIO引脚号
 * @param trigger 中断触发方式
 */
void init_gpio_interrupt(uint pin, uint32_t trigger) {
    // 配置GPIO为输入模式，启用上拉电阻（按键接GND时使用）
    gpio_init(pin);
    // gpio_set_dir(pin, GPIO_IN);
    // gpio_pull_up(pin);

    // 注册中断处理函数
    if (pin == INTERRUPT_PIN_1) {
        gpio_set_irq_enabled_with_callback(pin, trigger, true, &gpio_interrupt_handler1);
    } else {
        gpio_set_irq_enabled_with_callback(pin, trigger, true, &gpio_interrupt_handler2);
    }
    
    printf("GPIO%d irq init, trigger method:%d\n", pin, trigger);
}

int main() {
    // 初始化串口（用于打印信息）
    stdio_init_all();
    sleep_ms(1000); // 等待串口就绪
    printf("Pico SDK 多GPIO中断示例\n");

    // 初始化多个GPIO中断
    // GPIO_IRQ_EDGE_FALL：下降沿触发
    // GPIO_IRQ_EDGE_RISE：上升沿触发
    // GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE：双边沿触发
    init_gpio_interrupt(INTERRUPT_PIN_1, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);
    init_gpio_interrupt(INTERRUPT_PIN_2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);

    // 主循环
    while (true) {
        sleep_ms(1000); // 主循环可执行其他任务
        // printf("main loop running...\n");
    }

    return 0;
}
#endif