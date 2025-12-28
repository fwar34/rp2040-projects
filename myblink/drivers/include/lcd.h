#ifndef __LCD_H_
#define __LCD_H_

#include <stdint.h>
#include <stddef.h>

// 屏幕尺寸
#define LCD_WIDTH 160
#define LCD_HEIGHT 80
#define LCD_SIZE (LCD_WIDTH * LCD_HEIGHT)

// 颜色
#define RGB565(r, g, b) ((((r & 0xF8) | g >> 5) << 8) | (((g & 0x1C) << 3) | b >> 3))
#define RGB5652(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

#define RGB565_BLACK 0x0000
#define RGB565_BLUE 0x001F
#define RGB565_GREEN 0X07E0
#define RGB565_RED 0XF800
#define RGB565_YELLOW 0XFFE0
#define RGB565_WHITE 0xFFFF
// #define RGB565_ORANGE 0xFD20
#define RGB565_ORANGE 0xFC00

// 屏幕状态
typedef struct {
    uint16_t width;
    uint16_t height;
} TftDevice;

extern TftDevice tftDevice;

// 屏幕方向
typedef enum {
    ST7735S_DIRECTION_0,
    ST7735S_DIRECTION_90,
    ST7735S_DIRECTION_180,
    ST7735S_DIRECTION_270,
} TftDirection;

void TftInit();
void TftSetWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void TftSetDirection(TftDirection direction);
void TftClear(uint16_t color);
void TftPlot(uint16_t x, uint16_t y, uint16_t color);
void TftDrawArray(uint16_t *src, size_t len);

#endif