#include "logic.h"
#include "pico/stdlib.h"

#define LOGIC1_PIN 14
#define LOGIC2_PIN 15

void LogicInit()
{
    gpio_init(LOGIC1_PIN);
    gpio_init(LOGIC2_PIN);
    gpio_set_dir(LOGIC1_PIN, GPIO_OUT);
    gpio_set_dir(LOGIC2_PIN, GPIO_OUT);
}

void Logic1Up()
{
    gpio_put(LOGIC1_PIN, true);
}

void Logic1Down()
{
    gpio_put(LOGIC1_PIN, false);
}

void Logic1Xor()
{
    gpio_xor_mask(1 << LOGIC1_PIN);
}

void Logic2Up()
{
    gpio_put(LOGIC2_PIN, true);
}

void Logic2Down()
{
    gpio_put(LOGIC2_PIN, false);
}

void Logic2Xor()
{
    gpio_xor_mask(1 << LOGIC2_PIN);
}