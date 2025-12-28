#include "lcd.h"
#include "st7735s_command.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>


#define SPI_PORT spi0
// rp2040 和 st7735s 配置最大的 spi 速度，实际为 312500000，再大达到 62500000 时，rp2040 没法和 st7735s 通信成功
#define SPI_BAUDRATE (10 * 1000 * 1000)
// #define SPI_BAUDRATE (32 * 1000 * 1000)
#define SPI_SCK_PIN 2
#define SPI_MOSI_PIN 3
#define SPI_CS_PIN 4
#define SPI_DC_PIN 8
#define SPI_RST_PIN 9
// #define SPI_BLK_PIN 
// #define SPI_MISO_PIN

TftDevice tftDevice = {
    .width = LCD_WIDTH,
    .height = LCD_HEIGHT
};

static inline void TftCmd()
{
    gpio_put(SPI_DC_PIN, false);
}

static inline void TftData()
{
    gpio_put(SPI_DC_PIN, true);
}

static inline void TftWriteCmd(const uint8_t *cmd, size_t len)
{
    // 写命令
    TftCmd();
    spi_write_blocking(SPI_PORT, cmd, 1);
    
    // 如果带参数
    if (len > 0) {
        TftData();
        spi_write_blocking(SPI_PORT, &cmd[1], len - 1);
    }
}

static inline void TftWriteData(const uint16_t data)
{
    TftData();
    uint8_t tmp[2];
    tmp[0] = (data >> 8) & 0xFF;
    tmp[1] = data & 0xFF;

    spi_write_blocking(SPI_PORT, tmp, sizeof(tmp));
}

void TftInit()
{
    uint baudrate = spi_init(SPI_PORT, SPI_BAUDRATE);
    printf("spi baudrate:%d\n", baudrate);

    // 初始化 gpio
    gpio_init_mask(1 << SPI_DC_PIN | 1 << SPI_RST_PIN);
    gpio_set_dir_out_masked(1 << SPI_DC_PIN | 1 << SPI_RST_PIN);

    // 设置 spi 的功能管脚
    gpio_set_function_masked(1 << SPI_SCK_PIN | 1 << SPI_MOSI_PIN, GPIO_FUNC_SPI);

    // 复位屏幕
    gpio_put(SPI_RST_PIN, false);
    sleep_ms(100);
    gpio_put(SPI_RST_PIN, true);
    sleep_ms(100);

    /*
    屏幕方向
    MCU_write_TFT_Byte(0x36,TFT_COMMAND);//MX,MY,RGB mode 
	switch (DISPLAY_DIRECTION)//显示的方向(竖屏:0,横屏:1,竖屏旋转180度:2,横屏旋转180度:3)
	{
		case	0: MCU_write_TFT_Byte(0xC8,TFT_DATA);break;//竖屏
		case	1: MCU_write_TFT_Byte(0xA8,TFT_DATA);break;//横屏
		case    2: MCU_write_TFT_Byte(0x08,TFT_DATA);break;//竖屏翻转180度
		default	 : MCU_write_TFT_Byte(0x68,TFT_DATA);break;//横屏翻转180度
	}
    */

    // 命令和参数的总字节数量、命令、参数，最后一条是 0x00
    // 数量的最高比特位如果是 1 则代表此命令需要延时
    const uint8_t initCmd[] = {
        0x81, ST7735S_CMD_SLEEP_OUT_0X11,
        1, ST7735S_CMD_DISPLAY_INVERSION_ON_0X21,
        4, ST7735S_CMD_FRAME_RATE_CONTROL1_0XB1, 0x05, 0x3A, 0x3A,
        4, ST7735S_CMD_FRAME_RATE_CONTROL2_0XB2, 0x05, 0x3A, 0x3A,
        7, ST7735S_CMD_FRAME_RATE_CONTROL3_0XB3, 0x05, 0x3A, 0x3A, 0x05, 0x3A, 0x3A,
        2, ST7735S_CMD_INVERSION_CONTROL_0XB4, 0x03,
        4, ST7735S_CMD_POWER_CONTROL1_0XC0, 0x62, 0x02, 0x04,
        2, ST7735S_CMD_POWER_CONTROL2_0XC1, 0xC0,
        3, ST7735S_CMD_POWER_CONTROL3_0XC2, 0x0D, 0x00,
        3, ST7735S_CMD_POWER_CONTROL4_0XC3, 0x8D, 0x6A,
        3, ST7735S_CMD_POWER_CONTROL5_0XC4, 0x8D, 0xEE,
        2, ST7735S_CMD_VCOM_CONTROL1_0XC5, 0x0E,
        17, ST7735S_CMD_POSITIVE_GAMMA_CORRECTION_0XE0, 0x10, 0x0E, 0x02, 0x03, 0x0E, 0x07, 0x02,
        0x07, 0x0A, 0x12, 0x27, 0x37, 0x00, 0x0D, 0x0E, 0x10,
        17, ST7735S_CMD_NEGATIVE_GAMMA_CORRECTION_0XE1, 0x10, 0x0E, 0x03, 0x03, 0x0F, 0x06, 0x02,
        0x08, 0x0A, 0x13, 0x26, 0x36, 0x00, 0x0D, 0x0E, 0x10,
        2, ST7735S_CMD_PIXEL_FORMAT_SET_0X3A, 0x05,
        2, ST7735S_CMD_MEMORY_ACCESS_CONTROL_0X36, 0x68,
        1, ST7735S_CMD_DISPLAY_ON_0X29,
        0x00
    };

    const uint8_t *curIndex = initCmd;
    uint8_t count = 0;
    uint8_t tmp = 0;
    while (*curIndex) {
        tmp = *curIndex++;
        count = tmp & 0x7F;
        TftWriteCmd(curIndex, count);
        curIndex += count;
        if (tmp & 0x80) {
            sleep_ms(120);
        }
    }
    
    // 打开背光

    // 清屏
    TftClear(RGB565_RED);
}

// 设置显示区域
void TftSetWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    uint8_t xCmd[] = {ST7735S_CMD_COLUMN_ADDRESS_SET_0X2A, (xStart >> 8) & 0xFF,
        (xStart & 0xFF) + 1, (xEnd >> 8) & 0xFF, (xEnd & 0xFF) + 1};
    TftWriteCmd(xCmd, sizeof(xCmd) + 1);
    uint8_t yCmd[] = {ST7735S_CMD_PAGE_ADDRESS_SET_0X2B, (yStart >> 8) & 0xFF,
        (yStart & 0xFF) + 26, (yEnd >> 8) & 0xFF, (yEnd & 0xFF) + 27};
    TftWriteCmd(yCmd, sizeof(yCmd) + 1);
    uint8_t mWriteCmd = ST7735S_CMD_MEMORY_WRITE_0X2C;
    TftWriteCmd(&mWriteCmd, 1);
}

void TftSetDirection(TftDirection direction)
{
}

void TftClear(uint16_t color)
{
    TftSetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    // uint16_t data[LCD_WIDTH] = {0};
    // for (size_t i = 0; i < LCD_WIDTH; i++) {
    //     data[i] = (color >> 8 | color & 0xFF); // msb 在低字节，lsb 在高字节
    // }
    uint8_t data[LCD_WIDTH * 2];
    for (size_t i = 0; i < LCD_WIDTH * 2; i++) {
        data[i++] = color >> 8;
        data[i] = color & 0xFF;
    }

    for (size_t i = 0; i < LCD_HEIGHT; i++) {
        spi_write_blocking(SPI_PORT, (uint8_t *)data, LCD_WIDTH * 2);
    }
}

void TftPlot(uint16_t x, uint16_t y, uint16_t color)
{
}

void TftDrawArray(uint16_t *src, size_t len)
{
    // TftWriteData()
}
