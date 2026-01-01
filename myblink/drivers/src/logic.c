#include "logic.h"
#include "pico/stdlib.h"

#define LOGIC_PIN 14

void LogicInit()
{
    gpio_init(LOGIC_PIN);
    gpio_set_dir(LOGIC_PIN, GPIO_OUT);
}

void LogicUp()
{
    gpio_put(LOGIC_PIN, true);
}

void LogicDown()
{
    gpio_put(LOGIC_PIN, false);
}

void LogicXor()
{
    gpio_xor_mask(1 << LOGIC_PIN);
}
