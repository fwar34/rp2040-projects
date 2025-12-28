#include "st7735s_config.h"
#include "st7735s.h"
#include "font.h"
#include "spinlock.h"
#include "qpc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"

Q_DEFINE_THIS_MODULE("st7735s")

#define SPI_DMA_BUFFER_SIZE 1024

#define SPI_PORT spi0
// rp2040 和 st7735s 配置最大的 spi 速度，实际为 312500000，再大达到 62500000 时，rp2040 没法和 st7735s 通信成功
#define SPI_BAUDRATE (32 * 1000 * 1000)
#define SPI_SCK_PIN_2 2
#define SPI_MOSI_PIN_3 3
#define SPI_CS_PIN 4
#define SPI_DC_PIN_8 8
#define SPI_RST_PIN_9 9
// #define SPI_BLK_PIN 
// #define SPI_MISO_PIN

// 管理LCD重要参数
// 默认为竖屏
LcdDev lcddev;

static inline void TftCmd()
{
	// asm volatile("nop \n nop \n nop");
	gpio_put(SPI_DC_PIN_8, false);
	// sleep_us(20);
	// asm volatile("nop \n nop \n nop");
}

static inline void TftData()
{
    gpio_put(SPI_DC_PIN_8, true);
}

// #define LCD_BEGIN() gpio_put(SPI_CS_PIN, false)
// #define LCD_END() gpio_put(SPI_CS_PIN, true)
#define LCD_BEGIN() 
#define LCD_END() 

#define LCD_CMD() gpio_put(SPI_DC_PIN_8, false)
#define LCD_DATA() gpio_put(SPI_DC_PIN_8, true)
// #define LCD_CMD() TftCmd()
// #define LCD_DATA() TftData()

#define CLR_LCD_RES gpio_put(SPI_RST_PIN_9, false)
#define SET_LCD_RES gpio_put(SPI_RST_PIN_9, true)

#define SPI_LOCK() while (spiDmaTxComplete == 0); spiDmaTxComplete = 0;
#define SPI_UNLOCK() (spiDmaTxComplete = 1)

static volatile uint8_t spiDmaTxComplete = 1; // SPI是否正在传输，即判断送图是否完成

//////////////////////////////////////
enum LineBufferState {
	LINE_BUFFER_STATE_INVALID = 0,
	LINE_BUFFER_STATE_FLUSHING,
	LINE_BUFFER_STATE_RENDERING
};

#define SELECT_FONT_SIZE 16

// 全局像素缓冲区（假设屏幕分辨率为 160x80）
typedef struct {
    // stm32f103c8t6 内存不够缓存整个 160 * 80 分辨率的 rgb565 屏幕，这里只缓存 LCD_WIDTH * SELECT_FONT_SIZE 的，
	// SELECT_FONT_SIZE 为字体的像素高度，即每次最多只渲染满一行文字
    uint16_t pixelBuffer[LCD_WIDTH * SELECT_FONT_SIZE] __attribute__((aligned(4)));
	uint16_t bufLen;
	uint8_t curState;
} LineBuffer;

static LineBuffer fb[2]; // 双缓冲
static uint8_t g_curBufIndex = 0;  // 0 or 1
static volatile SpinLock g_spinLock = LOCK_INIT_VALUE; // 操作buffer状态，双buffer索引的锁

void SetFbRendering()
{
    accquire_spinlock(&g_spinLock, 0);
    if (fb[g_curBufIndex].curState == LINE_BUFFER_STATE_FLUSHING) {
        fb[g_curBufIndex].curState = LINE_BUFFER_STATE_RENDERING;
        // UartPrintf("setfb %d", g_curBufIndex);
    } else if (fb[g_curBufIndex ^ 1].curState == LINE_BUFFER_STATE_FLUSHING) {
        fb[g_curBufIndex ^ 1].curState = LINE_BUFFER_STATE_RENDERING;
        // UartPrintf("setfb %d", g_curBufIndex ^ 1);
    } else {
        printf("fb error!!!");
    }
    release_spinlock(&g_spinLock);
}

static void DMACallback()
{
	if (dma_channel_get_irq0_status(lcddev.dmaReadChannel)) {
		dma_channel_acknowledge_irq0(lcddev.dmaReadChannel);
		SetFbRendering();
		LCD_END();
		SPI_UNLOCK();
	}
}

static void LcdinitDma()
{
	// 获取空闲 DMA 通道
	int dmaReadChannel = dma_claim_unused_channel(true);
	dma_channel_config config = dma_channel_get_default_config(dmaReadChannel);
	channel_config_set_dreq(&config, spi_get_dreq(SPI_PORT, true));
	// channel_config_set_read_increment(&config, false);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
	dma_channel_configure(dmaReadChannel, &config, &spi_get_hw(SPI_PORT)->dr, NULL, 0, false);

	// 设置dma结束的中断
	dma_channel_set_irq0_enabled(dmaReadChannel, true);
	irq_set_exclusive_handler(DMA_IRQ_0, DMACallback);
	irq_set_enabled(DMA_IRQ_0, true);

	lcddev.dmaReadChannel = dmaReadChannel;
}

#if false
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi == &hspi1) {
		SetFbRendering();
		LCD_END();
		SPI_UNLOCK();
	}
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi == &hspi1) {
		SetFbRendering();
		LCD_END();
		SPI_UNLOCK();
		printf("spi2 dma error");
	}
}
#endif

// 写命令 dcx = 0
void LcdWriteCmd(uint8_t data)
{
	LCD_CMD();
	spi_write_blocking(SPI_PORT, &data, sizeof(uint8_t));
}

// 写数据 dcx = 1
void LcdWriteData(uint8_t data)
{
	LCD_DATA();
	spi_write_blocking(SPI_PORT, &data, sizeof(uint8_t));
}

void LcdWriteDataSPI(uint8_t *data, uint16_t dataSize)
{
	LCD_DATA();
	spi_write_blocking(SPI_PORT, data, dataSize);
}

void LcdWriteDataU16(uint16_t data)
{
	uint8_t tmp[2];
	tmp[0] = (data >> 8) & 0xFF;
	tmp[1] = data & 0xFF;
	LCD_DATA();
	spi_write_blocking(SPI_PORT, tmp, sizeof(tmp));
}

void LcdWriteDataArray(const uint8_t *array, uint16_t arrayLen)
{
	LCD_DATA();
	dma_channel_transfer_from_buffer_now(lcddev.dmaReadChannel, array, arrayLen);
    // DMA 完成后会在回调函数中释放锁和片选
}

#if false
HAL_StatusTypeDef LcdWriteDataArrayWithBuffer(LineBuffer *fbBuf)
{
	SPI_LOCK();
	LCD_DATA();
	HAL_StatusTypeDef ret = HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)fbBuf->pixelBuffer, fbBuf->bufLen);
    if (ret != HAL_OK) {
		LCD_END();
		SPI_UNLOCK();
		fbBuf->curState = LINE_BUFFER_STATE_RENDERING;
    }

    // DMA 完成后会在回调函数中释放锁和片选
	return ret;
}
#endif

// res低电平>5us,这里使用1ms，最长复位时间120ms
void LcdReset()
{
	SET_LCD_RES;
	CLR_LCD_RES;
	sleep_ms(100);
	SET_LCD_RES;
	sleep_ms(120);
}

void LCDDirection(uint8_t direction)
{
	lcddev.setxcmd = 0x2A;
	lcddev.setycmd = 0x2B;
	lcddev.wramcmd = 0x2C;
	lcddev.dir = direction % 4;
	switch (direction)
	{
	case 0:
		lcddev.width = LCD_WIDTH;
		lcddev.height = LCD_HEIGHT;
		lcddev.xoffset = 26;
		lcddev.yoffset = 1;
		LcdWriteCmd(0x36);
		LcdWriteData((1 << 3) | (1 << 6) | (1 << 7)); // BGR==1,MY==0,MX==0,MV==0
		break;
	case 1:
		lcddev.width = LCD_HEIGHT;
		lcddev.height = LCD_WIDTH;
		lcddev.xoffset = 1;
		lcddev.yoffset = 26;
		LcdWriteCmd(0x36);
		LcdWriteData((1 << 3) | (1 << 7) | (1 << 5)); // BGR==1,MY==1,MX==0,MV==1
		break;
	case 2:
		lcddev.width = LCD_WIDTH;
		lcddev.height = LCD_HEIGHT;
		lcddev.xoffset = 26;
		lcddev.yoffset = 1;
		LcdWriteCmd(0x36);
		LcdWriteData((1 << 3) | (0 << 6) | (0 << 7)); // BGR==1,MY==0,MX==0,MV==0
		break;
	case 3:
		lcddev.width = LCD_HEIGHT;
		lcddev.height = LCD_WIDTH;
		lcddev.xoffset = 1;
		lcddev.yoffset = 26;
		LcdWriteCmd(0x36);
		LcdWriteData((1 << 3) | (1 << 6) | (1 << 5)); // BGR==1,MY==1,MX==0,MV==1
		break;
	default:
		break;
	}
}

void LcdSetParam()
{
	LCD_BEGIN();
	LcdWriteCmd(0x11); // 退出睡眠
	sleep_ms(120);

//	LcdWriteCmd(0x21);
	LcdWriteCmd(0x21);

	LcdWriteCmd(0xB1);
	LcdWriteData(0x05);
	LcdWriteData(0x3A);
	LcdWriteData(0x3A);

	LcdWriteCmd(0xB2);
	LcdWriteData(0x05);
	LcdWriteData(0x3A);
	LcdWriteData(0x3A);

	LcdWriteCmd(0xB3);
	LcdWriteData(0x05);
	LcdWriteData(0x3A);
	LcdWriteData(0x3A);
	LcdWriteData(0x05);
	LcdWriteData(0x3A);
	LcdWriteData(0x3A);

	LcdWriteCmd(0xB4);
	LcdWriteData(0x03);

	LcdWriteCmd(0xC0);
	LcdWriteData(0x62);
	LcdWriteData(0x02);
	LcdWriteData(0x04);

	LcdWriteCmd(0xC1);
	LcdWriteData(0xC0);

	LcdWriteCmd(0xC2);
	LcdWriteData(0x0D);
	LcdWriteData(0x00);

	LcdWriteCmd(0xC3);
	LcdWriteData(0x8D);
	LcdWriteData(0x6A);

	LcdWriteCmd(0xC4);
	LcdWriteData(0x8D);
	LcdWriteData(0xEE);

	LcdWriteCmd(0xC5); /*VCOM*/
	LcdWriteData(0x0E);

	LcdWriteCmd(0xE0);
	LcdWriteData(0x10);
	LcdWriteData(0x0E);
	LcdWriteData(0x02);
	LcdWriteData(0x03);
	LcdWriteData(0x0E);
	LcdWriteData(0x07);
	LcdWriteData(0x02);
	LcdWriteData(0x07);
	LcdWriteData(0x0A);
	LcdWriteData(0x12);
	LcdWriteData(0x27);
	LcdWriteData(0x37);
	LcdWriteData(0x00);
	LcdWriteData(0x0D);
	LcdWriteData(0x0E);
	LcdWriteData(0x10);

	LcdWriteCmd(0xE1);
	LcdWriteData(0x10);
	LcdWriteData(0x0E);
	LcdWriteData(0x03);
	LcdWriteData(0x03);
	LcdWriteData(0x0F);
	LcdWriteData(0x06);
	LcdWriteData(0x02);
	LcdWriteData(0x08);
	LcdWriteData(0x0A);
	LcdWriteData(0x13);
	LcdWriteData(0x26);
	LcdWriteData(0x36);
	LcdWriteData(0x00);
	LcdWriteData(0x0D);
	LcdWriteData(0x0E);
	LcdWriteData(0x10);

	LcdWriteCmd(0x3A);
	LcdWriteData(0x05);
	// LcdWriteData(0x48);

	// LcdWriteCmd(0x36);
	// LcdWriteData(0x00); //
	// LcdWriteData(0xA8); //
	// LcdWriteData(0xC8); //
	// LcdWriteData(0x60 | 0x08);
	// LcdWriteData(0xF0);
	LCDDirection(USE_HORIZONTAL);

	LcdWriteCmd(0x29);
	LCD_END();
}

void LcdSetWindow(uint16_t xStar, uint16_t yStar, uint16_t xEnd, uint16_t yEnd)
{
	LcdWriteCmd(lcddev.setxcmd);
	LcdWriteData(0x00);
	LcdWriteData(xStar + lcddev.xoffset);
	LcdWriteData(0x00);
	LcdWriteData(xEnd + lcddev.xoffset);

	LcdWriteCmd(lcddev.setycmd);
	LcdWriteData(0x00);
	LcdWriteData(yStar + lcddev.yoffset);
	LcdWriteData(0x00);
	LcdWriteData(yEnd + lcddev.yoffset);

	// LCD_WriteRAM_Prepare(); // 开始写入GRAM
	LcdWriteCmd(lcddev.wramcmd);
}

void LcdInit()
{
	uint baudrate = spi_init(SPI_PORT, SPI_BAUDRATE);
    printf("spi baudrate:%d\n", baudrate);
	
    gpio_init_mask(1 << SPI_DC_PIN_8 | 1 << SPI_RST_PIN_9);
    gpio_set_dir_out_masked(1 << SPI_DC_PIN_8 | 1 << SPI_RST_PIN_9);
    // 设置 spi 的功能管脚
    gpio_set_function_masked(1 << SPI_SCK_PIN_2 | 1 << SPI_MOSI_PIN_3, GPIO_FUNC_SPI);

    // 复位屏幕
	LcdReset();
	LcdSetParam();

	LcdinitDma();
	spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

#if false
// 设置显示区域
void LcdSetWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
	// 列地址设置
	LcdWriteCmd(0x2A);
//	LcdWriteData(xStart >> 8);
//	LcdWriteData(xStart + 1);
//	LcdWriteData(xEnd >> 8);
//	LcdWriteData(xEnd + 1);
	uint8_t xRegion[4] = {xStart >> 8, (xStart & 0xFF) + 1, xEnd >> 8, (xEnd & 0xFF) + 1};
    LcdWriteDataSPI(xRegion, 4);

    // 行地址设置
	LcdWriteCmd(0x2B);
//	LcdWriteData(yStart >> 8);
//	LcdWriteData(yStart + 0x1A);
//	LcdWriteData(yEnd >> 8);
//	LcdWriteData(yEnd + 0x1A);
	uint8_t yRegion[4] = {yStart >> 8, (yStart & 0xFF) + 0x1A, yEnd >> 8, (yEnd & 0xFF) + 0x1A};
    LcdWriteDataSPI(yRegion, 4);


	LcdWriteCmd(0x2C); // 写入显存
}
#endif

/**
 * startX: 矩形左上角起点X坐标
 * startY: 矩形左上角起点Y坐标
 * width: 矩形宽度
 * height: 矩形高度
 */
void LcdDrawBlock(uint8_t xStart, uint8_t yStart, uint8_t width, uint8_t height, uint16_t color)
{
	uint8_t xEnd = xStart + width - 1;
	uint8_t yEnd = yStart + height - 1;

	// uint16_t pxColor[width * height];
	// for (uint16_t i = 0; i < width * height; ++i) {
	// 	pxColor[i] = color;
	// }
	uint8_t pxColor[width * height * 2];
	for (uint16_t i = 0; i < width * height * 2; ++i) {
		pxColor[i++] = color >> 8;
		pxColor[i] = color & 0xFF;
	}

	LCD_BEGIN();
	LcdSetWindow(xStart, yStart, xEnd, yEnd);
	LcdWriteDataSPI(pxColor, width * height * 2);
	LCD_END();
}

#if true
void LcdClear(uint16_t color)
{
	// uint16_t data[LCD_WIDTH] = {0};
	// for (int i = 0; i < LCD_WIDTH; ++i) {
	// 	data[i] = color;
	// }
	// LCD_BEGIN();
    // for (int i = 0; i < LCD_HEIGHT; i++) {
    //     LcdWriteDataSPI((uint8_t*)data, LCD_WIDTH * 2);
    // }
    // LCD_END();
	printf("LcdClear color:0x%04X\n", color);
	LcdSetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
	for (size_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
		LcdWriteDataU16(color);
	}
}
#else
void LcdClear(uint16_t Color)
{
	unsigned int i, m;
	LcdSetWindow(0, 0, lcddev.width - 1, lcddev.height - 1);
	for (i = 0; i < lcddev.height; i++)
	{
		for (m = 0; m < lcddev.width; m++)
		{
			LcdWriteData(Color >> 8);
			LcdWriteData(Color);
		}
	}
}
#endif

void LcdDrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
	LcdSetWindow(x, y, x, y);
	LcdWriteDataU16(color);
}

/**
 * startX: 线条起点X坐标
 * endX: 线条起点Y坐标
 * len: 线条像素长度
 */
void LcdDrawLine(uint16_t startX, uint16_t startY, uint16_t length, uint16_t color, uint8_t direction)
{
	if (direction == LINE_DIR_HORIZONTAL) {
		LcdDrawBlock(startX, startY, length, 1, color);
	} else if (direction == LINE_DIR_VERTICAL) {
		LcdDrawBlock(startX, startY, 1, length, color);
	}
}

void LcdDrawRect(const Rect *rect, uint16_t color)
{
	LCD_BEGIN();
	LcdDrawLine(rect->leftUp.x, rect->leftUp.y, rect->width, color, LINE_DIR_HORIZONTAL);
	LcdDrawLine(rect->leftUp.x, rect->leftUp.y + rect->height - 1, rect->width, color, LINE_DIR_HORIZONTAL);

	LcdDrawLine(rect->leftUp.x, rect->leftUp.y, rect->height, color, LINE_DIR_VERTICAL);
	LcdDrawLine(rect->leftUp.x + rect->width - 1, rect->leftUp.y, rect->height, color, LINE_DIR_VERTICAL);
	LCD_END();
}

void LcdDrawCircle(const Point *center, uint16_t radius, uint16_t color)
{
	Q_UNUSED_PAR(center);
	Q_UNUSED_PAR(radius);
	Q_UNUSED_PAR(color);
}

// 字体参数结构体（固化参数减少运行时计算）
typedef struct {
    const uint8_t* data;    // 字体点阵数据起始地址
    uint8_t width;          // 字符宽度（像素）
    uint8_t height;         // 字符高度（像素）
    uint16_t buf_size;      // 字符像素缓冲区大小（字节：width*height*2）
    uint8_t bytes_per_char; // 单个字符点阵字节数
} FontDef;

// 预定义字体参数（编译期计算常量）
#define FONT_12_WIDTH    6
#define FONT_12_HEIGHT   12
#define FONT_12_BUF_SIZE (FONT_12_WIDTH * FONT_12_HEIGHT * 2)
#define FONT_12_BYTES    12

#define FONT_16_WIDTH    8
#define FONT_16_HEIGHT   16
#define FONT_16_BUF_SIZE (FONT_16_WIDTH * FONT_16_HEIGHT * 2)
#define FONT_16_BYTES    16
// 透明背景标记（16位颜色值）
#define TFT_TRANSPARENT 0xFFFF

/**
 * @brief 高性能字符显示（整缓冲区批量传输）
 * @param x/y        字符左上角坐标（ST7735S：0~159/0~79）
 * @param ch         显示字符（ASCII可打印）
 * @param back_col   背景色（TFT_TRANSPARENT透明）
 * @param font_col   前景色
 * @param fontSize  字体大小（12/16）
 * @param setLcdCS   是否控制片选
 */
void TFT_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t back_col,
                  uint16_t font_col, uint8_t fontSize, bool setLcdCS)
{
    // 1. 快速合法性校验（提前返回减少分支）
    if (ch < ' ' || ch > '~')
        return;
    const uint8_t ch_idx = ch - ' ';

    // 2. 字体参数快速映射（无分支查表思想）
    FontDef font;
    if (fontSize == 12) {
        font.data = (const uint8_t *)&asc2_1206[ch_idx];
        font.width = FONT_12_WIDTH;
        font.height = FONT_12_HEIGHT;
        font.buf_size = FONT_12_BUF_SIZE;
        font.bytes_per_char = FONT_12_BYTES;
    } else if (fontSize == 16) {
        font.data = (const uint8_t *)&asc2_1608[ch_idx];
        font.width = FONT_16_WIDTH;
        font.height = FONT_16_HEIGHT;
        font.buf_size = FONT_16_BUF_SIZE;
        font.bytes_per_char = FONT_16_BYTES;
    } else
        return;

    // 3. 边界检查（确保字符完全显示）
    if (x + font.width > LCD_WIDTH || y + font.height > LCD_HEIGHT)
        return;

    // 4. 分配连续像素缓冲区（栈上分配最快，STM32栈足够承载）
    uint8_t pixel_buf[FONT_16_BUF_SIZE]
        __attribute__((aligned(4)));          // 4字节对齐加速DMA
    uint16_t *buf_16 = (uint16_t *)pixel_buf; // 16位颜色指针
    uint16_t buf_idx = 0;

    // 5. 点阵→像素缓冲区快速填充（流水线解析）
    const uint8_t *dot_data = font.data;
    const bool is_transparent = (back_col == TFT_TRANSPARENT);
    const uint8_t width = font.width;

    for (uint8_t row = 0; row < font.height; row++) {
        const uint8_t byte = dot_data[row]; // 当前行点阵数据
        uint8_t mask = 0x80;                // 位掩码（从高位开始）

        for (uint8_t col = 0; col < width; col++) {
            // 快速颜色赋值（无分支三元运算）
            buf_16[buf_idx++] =
                (byte & mask) ? font_col : (is_transparent ? 0x0000 : back_col);
            mask >>= 1; // 移位替代重复计算掩码
        }
    }

    // 6. 批量传输（仅一次命令+一次数据传输）
    if (setLcdCS)
        LCD_BEGIN();

    // 设置显示窗口（仅一次）
    LcdSetWindow(x, y, x + width - 1, y + font.height - 1);
    // 整缓冲区批量发送（支持DMA的话此处用DMA更优）
#if false
    HAL_StatusTypeDef ret = LcdWriteDataArray(pixel_buf, font.buf_size);
    if (setLcdCS && ret != HAL_OK)
        LCD_END();
#endif
}

//Display str (string type) in (x, y) with backColor, front_color and front_size under maxWidth. font.h is essential.
void LCD_ShowCharStr(uint16_t x, uint16_t y, uint32_t maxWidth, char *str,
		uint16_t backColor, uint16_t fontColor, uint8_t fontSize) {

	maxWidth += x;

	LCD_BEGIN();
	while ((*str <= '~') && (*str >= ' ')) {
		if (x >= maxWidth) {
			break;
		}

		TFT_ShowChar(x, y, *str, backColor, fontColor, fontSize, false);
		x += fontSize / 2;
		str++;
	}
	LCD_END();
}

//Display number (byte type) in (x, y) with backColor, front_color and front_size under maxWidth. font.h is essential.
void LCD_ShowCharNumber(uint16_t x, uint16_t y, uint32_t maxWidth,
		char* number_ascii, uint16_t backColor, uint16_t fontColor,
		uint8_t fontSize) {
	// char number_ascii[10];

	// sprintf(number_ascii, "%d", number);

	LCD_ShowCharStr(x, y, maxWidth, number_ascii, backColor, fontColor,
			fontSize);
}

/*************************************************
函数名：f_Lcd_ShowBmp_ColorValue
功能：指定位置显示图片
入口参数：
	pbmp 	: 图片数组指针(像素点字节颜色值，高字节在前)
	x0	 	: 图片显示起点x坐标
	y0	 	: 图片显示起点y坐标
	x_Len	: 图片宽度
	y_Len	: 图片高度
返回值：无
*************************************************/
void LcdShowBmp(const uint8_t *pbmp, uint8_t x0, uint8_t y0, uint8_t x_Len,
		uint8_t y_Len) {
	uint8_t x1 = x0 + x_Len - 1;
	uint8_t y1 = y0 + y_Len - 1;
	uint16_t xy = (x_Len * y_Len) << 1;
	uint16_t i = 0;

	LCD_BEGIN();
	LcdSetWindow(x0, y0, x1, y1);
	LcdWriteCmd(0x2C);

	for (i = 0; (i << 1) < xy; i++)
		LcdWriteDataU16((pbmp[i << 1] << 8) + pbmp[(i << 1) + 1]);
	LCD_END();
}

void LcdDrawRgb565(const uint16_t *rgb565, uint16_t xStart, uint16_t yStart, uint16_t width, uint16_t height)
{
	uint16_t xEnd = xStart + width - 1;
	uint16_t yEnd = yStart + height - 1;
	LCD_BEGIN();
	LcdSetWindow(xStart, yStart, xEnd, yEnd);
	for (uint16_t i = 0; i < width * height; i++) {
		LcdWriteDataU16(rgb565[i]);
	}
	LCD_END();
}

void LcdDrawData(const uint8_t *rgb565, uint16_t xStart, uint16_t yStart, uint16_t width, uint16_t height)
{
	uint16_t xEnd = xStart + width - 1;
	uint16_t yEnd = yStart + height - 1;
	LCD_BEGIN();
	LcdSetWindow(xStart, yStart, xEnd, yEnd);
#if 0
	LcdWriteDataArray(rgb565, width * height * 2);
#endif
	LCD_END();
}

LineBuffer *GetCurBuffer()
{
	uint8_t curIndex = 0;
	accquire_spinlock(&g_spinLock, 0);
	curIndex = g_curBufIndex;
	release_spinlock(&g_spinLock);

	return &fb[curIndex];
}

bool SwitchBuffer()
{
	accquire_spinlock(&g_spinLock, 0);
	uint8_t destIndex = g_curBufIndex ^ 1;
	Q_ASSERT(destIndex == 0 || destIndex == 1);
	if (fb[destIndex].curState != LINE_BUFFER_STATE_FLUSHING) {
        g_curBufIndex = destIndex;
    } else {
		printf("switch err, cur:%d", g_curBufIndex);
	}
    release_spinlock(&g_spinLock);

	return true;
}

/**
 * @brief 绘制单个字符到指定缓冲区
 */
void TFT_DrawCharToBuffer(uint16_t x_offset, char ch, uint16_t back_col,
                          uint16_t font_col, uint8_t fontSize,
                          uint16_t *buffer, uint16_t bufferWidth)
{
    if (ch < ' ' || ch > '~' || !buffer) return;


    const uint8_t ch_idx = ch - ' ';
    FontDef font = {0};

    if (fontSize == 12) {
        // font.data = asc2_1206 + ch_idx * FONT_12_BYTES;
        font.data = (const uint8_t *)&asc2_1206[ch_idx][0];
        font.width = FONT_12_WIDTH;
        font.height = FONT_12_HEIGHT;
    } else if (fontSize == 16) {
        // font.data = asc2_1608 + ch_idx * FONT_16_BYTES;
        font.data = &asc2_1608[ch_idx][0];
        font.width = FONT_16_WIDTH;
        font.height = FONT_16_HEIGHT;
    } else return;

	if (x_offset + font.width > LCD_WIDTH) {
		return;
	}

    const uint8_t *dot_data = font.data;
    const bool is_transparent = (back_col == TFT_TRANSPARENT);

    for (uint8_t row = 0; row < font.height; row++) {
        const uint8_t byte = dot_data[row];
        uint8_t mask = 0x80;
        uint16_t *buf_ptr = buffer + row * bufferWidth + x_offset;

        for (uint8_t col = 0; col < font.width; col++) {
            // 修复：透明时保留缓冲区原有值，不覆盖
            if (byte & mask) {
                *buf_ptr = font_col << 8 | font_col >> 8;
            } else if (!is_transparent) {
                *buf_ptr = back_col << 8 | back_col >> 8;
            }
            mask >>= 1;
			buf_ptr++;
        }
    }
}

#if true
/**
 * @brief 高效显示字符串（DMA + 双缓冲优化版）
 */
void LCD_ShowCharStr_DMA_Optimized(uint16_t x, uint16_t y, uint32_t maxWidth,
                                  const char *str, uint16_t backColor, 
                                  uint16_t fontColor, uint8_t fontSize)
{
    // HAL_GPIO_WritePin(testChannel2_GPIO_Port, testChannel2_Pin, GPIO_PIN_SET);
	if (x > LCD_WIDTH - FONT_16_WIDTH) {
		return;
	}

    if (!str || *str == '\0' || maxWidth == 0) {
		printf("str error!");
		return;
	}

	// uint8_t strLen = strlen(str);
	LineBuffer *curBuf = GetCurBuffer();

	// 1. 如果当前 buffer 正在送图则等待送图完成后才能开始渲染
	accquire_spinlock(&g_spinLock, 0);
	if (curBuf->curState == LINE_BUFFER_STATE_FLUSHING) {
		release_spinlock(&g_spinLock);
		printf("error: curBf is flusing!!! str[%s]", str);
		return;
	}
	release_spinlock(&g_spinLock);

	// 2. 开始渲染
    uint16_t charWidth = (fontSize == 12) ? FONT_12_WIDTH : FONT_16_WIDTH;
    uint16_t charHeight = (fontSize == 12) ? FONT_12_HEIGHT : FONT_16_HEIGHT;
    
    // 计算最大可显示字符数
    uint16_t maxChars = maxWidth / charWidth;
    uint16_t bufferWidth = maxChars * charWidth;
    // uint16_t bufferHeight = charHeight;

    // 批量填充背景并渲染字符到缓冲区A
	for (uint8_t i = 0; i < charHeight; i++) {
		uint16_t *tmp = curBuf->pixelBuffer + i * LCD_WIDTH;
		for (uint16_t j = 0; j < LCD_WIDTH; j++) {
			tmp[j] = backColor << 8 | backColor >> 8;
		}
	}

    uint16_t currentX = x;
    uint16_t charsRendered = 0;
    const char *tempStr = str;
    
    while (*tempStr && charsRendered < maxChars) {
        TFT_DrawCharToBuffer(currentX, *tempStr, backColor, fontColor, fontSize,
                            curBuf->pixelBuffer, bufferWidth);
        currentX += charWidth;
        charsRendered++;
        tempStr++;
    }

    if (charsRendered > 0) {
        SPI_LOCK();
		// 3. 渲染完成后准备将当前 buffer 送图，设置当前 buffer 状态为送图状态
		accquire_spinlock(&g_spinLock, 0);
		curBuf->curState = LINE_BUFFER_STATE_FLUSHING;
		release_spinlock(&g_spinLock);

		// 4. 交换双 buffer
		SwitchBuffer();

        LCD_BEGIN();
        LcdSetWindow(0, y, LCD_WIDTH - 1, y + charHeight - 1);
        // 5. 送图：发送刚渲染好的缓冲区给LCD
        LcdWriteDataArray((uint8_t *)curBuf->pixelBuffer, LCD_WIDTH * charHeight * 2);
    }
    // HAL_GPIO_WritePin(testChannel2_GPIO_Port, testChannel2_Pin, GPIO_PIN_RESET);
}
#endif