#include "logic.h"
#include "pico/stdlib.h"

#define LOGIC_PIN 23

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
