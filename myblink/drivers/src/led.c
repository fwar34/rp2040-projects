#include "led.h"
#include "pico/stdlib.h"

#define LED0 25
#define LED1 6
// #define LED2 21
// #define LED3 22
// #define LED123_MASK (1 << LED1 | 1 << LED2 | 1 << LED3)
// #define ALL_LED_MASK (1 << LED | LED123_MASK)

void LedInit()
{
    // gpio_init_mask(ALL_LED_MASK);
    // gpio_set_dir_masked(ALL_LED_MASK, ALL_LED_MASK);
    // gpio_put_masked(ALL_LED_MASK, ALL_LED_MASK);
    gpio_init(LED0);
    gpio_set_dir(LED0, GPIO_OUT);
    gpio_put(LED0, false);
    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);
    gpio_put(LED1, true);
}

void Led0Xor()
{
    gpio_xor_mask(1 << LED0);
}

void LightLed0()
{
    gpio_put(LED0, true);
}

void OffLed0()
{
    gpio_put(LED1, false);
}

void LightLed1()
{
    gpio_put(LED1, false);
}

// void LightLed2()
// {
//     gpio_put(LED2, false);
// }

// void LightLed3()
// {
//     gpio_put(LED3, false);
// }

void OffLed1()
{
    gpio_put(LED1, true);
}

void Led1Xor()
{
    gpio_xor_mask(1 << LED1);
}

void LedForError()
{
    static uint32_t lastTick = 0;
    // Led0Xor();
    Led1Xor();
    while (time_us_32() - lastTick < 200 * 1000);
    lastTick = time_us_32();
    // Led0Xor();
    Led1Xor();
    while (time_us_32() - lastTick < 200 * 1000);
    lastTick = time_us_32();
}

// void OffLed2()
// {
//     gpio_put(LED2, true);
// }

// void OffLed3()
// {
//     gpio_put(LED3, true);
// }

// void ToggleLed2()
// {
//     gpio_xor_mask(1 << LED2);
// }
