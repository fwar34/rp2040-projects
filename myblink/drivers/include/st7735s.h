/*
 * st7735s.h 0.96寸lcd屏幕
 *
 *  Created on: Mar 12, 2025
 *      Author: fwar3
 */

#ifndef INC_ST7735S_H_
#define INC_ST7735S_H_

// #include "rgb_color.h"
#include <inttypes.h>
#include <stdbool.h>

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

#define LCD_WIDTH 160
#define LCD_HEIGHT 80

// LCD重要参数集
typedef struct {
	uint16_t width;	 // LCD 宽度
	uint16_t height;	 // LCD 高度
	uint16_t id;		 // LCD ID
	uint8_t dir;		 // 横屏还是竖屏控制：0，竖屏；1，横屏。
	uint16_t wramcmd; // 开始写gram指令
	uint16_t setxcmd; // 设置x坐标指令
	uint16_t setycmd; // 设置y坐标指令
	uint8_t xoffset;
	uint8_t yoffset;
	int dmaReadChannel; // lcd 用于读取 buffer 发送到 lcd 的 DMA 通道
} LcdDev;

// LCD参数
extern LcdDev lcddev; // 管理LCD重要参数
/////////////////////////////////////用户配置区///////////////////////////////////
#define USE_HORIZONTAL 3 // 定义液晶屏顺时针旋转方向 	0-0度旋转，1-90度旋转，2-180度旋转，3-270度旋转

typedef enum {
	LINE_DIR_HORIZONTAL, LINE_DIR_VERTICAL, LINE_DIR_OBLIQUE,
} LineDir;

typedef struct {
	uint16_t x;
	uint16_t y;
} Point;

typedef struct {
	Point leftUp;
//	Point rightDown;
	uint16_t width;
	uint16_t height;
} Rect;

void LcdInit();
void LcdReset();
void LcdClear(uint16_t color);
void LcdDrawBlock(uint8_t xStart, uint8_t yStart, uint8_t width, uint8_t height,
		uint16_t color);
void LcdDrawPoint(uint16_t x, uint16_t y, uint16_t color);
void LcdDrawLine(uint16_t startX, uint16_t startY, uint16_t len, uint16_t color,
		uint8_t direction);
void LcdDrawRect(const Rect *rect, uint16_t color);
void LcdDrawCircle(const Point *center, uint16_t radius, uint16_t color);
//void LcdShowChar(uint16_t x, uint16_t y, char c, uint16_t frontColor, uint16_t backColor, uint8_t fontSize);
void TFT_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t back_color,
		uint16_t font_color, uint8_t font_size, bool setLcdCS);
void TFT_DrawCharToBuffer(uint16_t x_offset, char ch, uint16_t back_col,
                          uint16_t font_col, uint8_t font_size,
                          uint16_t *buffer, uint16_t buffer_width);
void LCD_ShowCharStr(uint16_t x, uint16_t y, uint32_t max_width, char *str,
		uint16_t back_color, uint16_t font_color, uint8_t font_size);
void LCD_ShowCharStr_DMA_Optimized(uint16_t x, uint16_t y, uint32_t maxWidth,
                                  const char *str, uint16_t backColor, 
                                  uint16_t fontColor, uint8_t fontSize);
void LCD_ShowCharNumber(uint16_t x, uint16_t y, uint32_t max_width,
		char* number_ascii, uint16_t back_color, uint16_t font_color,
		uint8_t font_size);
void LcdShowBmp(const uint8_t *pbmp, uint8_t x0, uint8_t y0, uint8_t x_Len,
		uint8_t y_Len);
void LcdDrawRgb565(const uint16_t *rgb565, uint16_t xStart, uint16_t yStart, uint16_t width, uint16_t height);
void LcdDrawData(const uint8_t *rgb565, uint16_t xStart, uint16_t yStart, uint16_t width, uint16_t height);

#endif /* INC_ST7735S_H_ */
