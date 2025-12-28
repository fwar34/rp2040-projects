#include "led.h"
#include "pico/stdlib.h"

#define LED 25
#define LED1 20
#define LED2 21
#define LED3 22
#define LED123_MASK (1 << LED1 | 1 << LED2 | 1 << LED3)
#define ALL_LED_MASK (1 << LED | LED123_MASK)

void LedInit()
{
    gpio_init_mask(ALL_LED_MASK);
    gpio_set_dir_masked(ALL_LED_MASK, ALL_LED_MASK);
    gpio_put_masked(ALL_LED_MASK, ALL_LED_MASK);
}

void LightLedForError()
{
    gpio_put_masked(LED123_MASK, 0);
}

void LightLed1()
{
    gpio_put(LED1, false);
}

void LightLed2()
{
    gpio_put(LED2, false);
}

void LightLed3()
{
    gpio_put(LED3, false);
}

void OffLed1()
{
    gpio_put(LED1, true);
}

void OffLed2()
{
    gpio_put(LED2, true);
}

void OffLed3()
{
    gpio_put(LED3, true);
}

void ToggleLed2()
{
    gpio_xor_mask(1 << LED2);
}
