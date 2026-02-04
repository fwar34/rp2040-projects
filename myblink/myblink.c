#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "qpc.h"
#include "led.h"
#include "bsp.h"
#if (defined LOGIC_TEST)
#include "logic.h"
#endif
#include "lcd.h"
#include "st7735s.h"

int main()
{
    stdio_init_all();
    // while (!stdio_usb_connected());
    printf("system clock:%d\n", clock_get_hz(clk_sys));
    LedInit();
    // TftInit();

    LcdInit();

#if (defined LOGIC_TEST)
    LogicInit();
#endif // LOGIC_TEST

    // while (true) {
    //     LogicUp();
    //     sleep_us(25);
    //     LogicDown();
    //     sleep_us(25);
    // }

    QF_init();
    BSP_init();
    BSP_start();
    QF_run();

    return 0;
}
