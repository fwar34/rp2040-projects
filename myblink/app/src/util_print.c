#include "util_print.h"
#include "hardware/uart.h"
#include <stdarg.h>
#include <stdio.h>

void Print(const char *fmt, ...)
{
    static char message[128] = {0};
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    uart_puts(uart0, message);
}
