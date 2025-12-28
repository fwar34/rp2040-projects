#ifndef __ST7735S_COMMAND_H_
#define __ST7735S_COMMAND_H_

/* ST7735S 命令宏定义 */
#define ST7735S_CMD_NOP_0X00 0x00                              // 空操作
#define ST7735S_CMD_SOFTWARE_RESET_0X01 0x01                   // 软件复位
#define ST7735S_CMD_READ_DISPLAY_ID_0X04 0x04                  // 读取显示ID
#define ST7735S_CMD_READ_DISPLAY_STATUS_0X09 0x09              // 读取显示状态
#define ST7735S_CMD_READ_DISPLAY_POWER_MODE_0X0A 0x0A          // 读取显示电源模式
#define ST7735S_CMD_READ_DISPLAY_MADCTL_0X0B 0x0B              // 读取MADCTL
#define ST7735S_CMD_READ_DISPLAY_PIXEL_FORMAT_0X0C 0x0C        // 读取像素格式
#define ST7735S_CMD_READ_DISPLAY_IMAGE_MODE_0X0D 0x0D          // 读取图像模式
#define ST7735S_CMD_READ_DISPLAY_SIGNAL_MODE_0X0E 0x0E         // 读取信号模式
#define ST7735S_CMD_READ_DISPLAY_SELF_DIAGNOSTIC_0X0F 0x0F     // 读取自诊断结果
#define ST7735S_CMD_SLEEP_IN_0X10 0x10                         // 进入睡眠模式
#define ST7735S_CMD_SLEEP_OUT_0X11 0x11                        // 退出睡眠模式
#define ST7735S_CMD_PARTIAL_MODE_ON_0X12 0x12                  // 部分显示模式开
#define ST7735S_CMD_NORMAL_DISPLAY_MODE_ON_0X13 0x13           // 普通显示模式开
#define ST7735S_CMD_DISPLAY_INVERSION_OFF_0X20 0x20            // 显示反转关闭
#define ST7735S_CMD_DISPLAY_INVERSION_ON_0X21 0x21             // 显示反转开启
#define ST7735S_CMD_GAMMA_SET_0X26 0x26                        // 伽马设置
#define ST7735S_CMD_DISPLAY_OFF_0X28 0x28                      // 关闭显示
#define ST7735S_CMD_DISPLAY_ON_0X29 0x29                       // 开启显示
#define ST7735S_CMD_COLUMN_ADDRESS_SET_0X2A 0x2A               // 列地址设置
#define ST7735S_CMD_PAGE_ADDRESS_SET_0X2B 0x2B                 // 页地址设置
#define ST7735S_CMD_MEMORY_WRITE_0X2C 0x2C                     // 写入显存
#define ST7735S_CMD_COLOR_SET_0X2D 0x2D                        // 颜色设置
#define ST7735S_CMD_MEMORY_READ_0X2E 0x2E                      // 读取显存
#define ST7735S_CMD_PARTIAL_AREA_0X30 0x30                     // 部分区域设置
#define ST7735S_CMD_VERTICAL_SCROLLING_DEFINITION_0X33 0x33    // 垂直滚动定义
#define ST7735S_CMD_TEARING_EFFECT_LINE_OFF_0X34 0x34          // 关闭撕裂效应线输出
#define ST7735S_CMD_TEARING_EFFECT_LINE_ON_0X35 0x35           // 开启撕裂效应线输出
#define ST7735S_CMD_MEMORY_ACCESS_CONTROL_0X36 0x36            // 存储访问控制（方向）
#define ST7735S_CMD_VERTICAL_SCROLLING_START_ADDRESS_0X37 0x37 // 垂直滚动起始地址
#define ST7735S_CMD_IDLE_MODE_OFF_0X38 0x38                    // 空闲模式关闭
#define ST7735S_CMD_IDLE_MODE_ON_0X39 0x39                     // 空闲模式开启
#define ST7735S_CMD_PIXEL_FORMAT_SET_0X3A 0x3A                 // 像素格式设置
#define ST7735S_CMD_WRITE_MEMORY_CONTINUE_0X3C 0x3C            // 写入显存继续
#define ST7735S_CMD_READ_MEMORY_CONTINUE_0X3E 0x3E             // 读取显存继续
#define ST7735S_CMD_SET_TEAR_SCANLINE_0X44 0x44                // 设置撕裂扫描线
#define ST7735S_CMD_GET_SCANLINE_0X45 0x45                     // 获取扫描线
#define ST7735S_CMD_READ_ID1_0XDA 0xDA                         // 读取ID1
#define ST7735S_CMD_READ_ID2_0XDB 0xDB                         // 读取ID2
#define ST7735S_CMD_READ_ID3_0XDC 0xDC                         // 读取ID3

/* 扩展命令 */
#define ST7735S_CMD_FRAME_RATE_CONTROL1_0XB1 0xB1      // 帧率控制1
#define ST7735S_CMD_FRAME_RATE_CONTROL2_0XB2 0xB2      // 帧率控制2
#define ST7735S_CMD_FRAME_RATE_CONTROL3_0XB3 0xB3      // 帧率控制3
#define ST7735S_CMD_INVERSION_CONTROL_0XB4 0xB4        // 反转控制
#define ST7735S_CMD_DISPLAY_FUNCTION_CONTROL_0XB6 0xB6 // 显示功能控制
#define ST7735S_CMD_ENTRY_MODE_SET_0XB7 0xB7           // 输入模式设置

#define ST7735S_CMD_POWER_CONTROL1_0XC0 0xC0 // 电源控制1
#define ST7735S_CMD_POWER_CONTROL2_0XC1 0xC1 // 电源控制2
#define ST7735S_CMD_POWER_CONTROL3_0XC2 0xC2 // 电源控制3
#define ST7735S_CMD_POWER_CONTROL4_0XC3 0xC3 // 电源控制4
#define ST7735S_CMD_POWER_CONTROL5_0XC4 0xC4 // 电源控制5
#define ST7735S_CMD_VCOM_CONTROL1_0XC5 0xC5  // VCOM 控制1
#define ST7735S_CMD_VCOM_CONTROL2_0XC7 0xC7  // VCOM 控制2

#define ST7735S_CMD_POWER_CONTROL_A_0XCB 0xCB // 电源控制A
#define ST7735S_CMD_POWER_CONTROL_B_0XCF 0xCF // 电源控制B

#define ST7735S_CMD_NVMEMORY_WRITE_0XD0 0xD0          // NV存储写入
#define ST7735S_CMD_NVMEMORY_PROTECTION_KEY_0XD1 0xD1 // NV存储保护键
#define ST7735S_CMD_NVMEMORY_STATUS_READ_0XD2 0xD2    // NV存储状态读取

#define ST7735S_CMD_READ_ID4_0XD3 0xD3                  // 读取ID4
#define ST7735S_CMD_POSITIVE_GAMMA_CORRECTION_0XE0 0xE0 // 正伽马校正
#define ST7735S_CMD_NEGATIVE_GAMMA_CORRECTION_0XE1 0xE1 // 负伽马校正

#define ST7735S_CMD_DIGITAL_GAMMA_CONTROL1_0XE2 0xE2 // 数字伽马控制1
#define ST7735S_CMD_DIGITAL_GAMMA_CONTROL2_0XE3 0xE3 // 数字伽马控制2

#define ST7735S_CMD_DRIVER_TIMING_CONTROL_A_0XE8 0xE8 // 驱动时序控制A
#define ST7735S_CMD_DRIVER_TIMING_CONTROL_B_0XEA 0xEA // 驱动时序控制B

#define ST7735S_CMD_POWER_ON_SEQUENCE_CONTROL_0XED 0xED // 上电时序控制

#define ST7735S_CMD_ENABLE_3G_0XF2 0xF2          // 启用3Gamma
#define ST7735S_CMD_INTERFACE_CONTROL_0XF6 0xF6  // 接口控制
#define ST7735S_CMD_PUMP_RATIO_CONTROL_0XF7 0xF7 // 泵比率控制


#endif // !__ST7735S_COMMAND_H_