#ifndef __RGB_COLOR_H_
#define __RGB_COLOR_H_

// ===================== 基础标准色 =====================
#define RGB565_BLACK       0x0000    // 纯黑 (0, 0, 0)
#define RGB565_WHITE       0xFFFF    // 纯白 (255, 255, 255)
#define RGB565_RED         0xF800    // 纯红 (255, 0, 0)
#define RGB565_GREEN       0x07E0    // 纯绿 (0, 255, 0)
#define RGB565_BLUE        0x001F    // 纯蓝 (0, 0, 255)
#define RGB565_YELLOW      0xFFE0    // 黄色 (255, 255, 0)
#define RGB565_CYAN        0x07FF    // 青色 (0, 255, 255)
#define RGB565_MAGENTA     0xF81F    // 品红/洋红 (255, 0, 255)
#define RGB565_ORANGE      0xFC60    // 橙色 (255, 165, 0)
#define RGB565_PURPLE      0x801F    // 紫色 (128, 0, 128)
#define RGB565_PINK        0xF89F    // 粉色 (255, 192, 203)
#define RGB565_BROWN       0x9820    // 棕色 (165, 42, 42)

// ===================== 灰度色系（渐变） =====================
#define RGB565_GRAY_0      0x0000    // 黑（同纯黑）
#define RGB565_GRAY_1      0x1818    // 极暗灰 (32, 32, 32)
#define RGB565_GRAY_2      0x3030    // 暗灰 (64, 64, 64)
#define RGB565_GRAY_3      0x4848    // 中暗灰 (96, 96, 96)
#define RGB565_GRAY_4      0x6060    // 中灰 (128, 128, 128)
#define RGB565_GRAY_5      0x7878    // 中亮灰 (160, 160, 160)
#define RGB565_GRAY_6      0x9090    // 亮灰 (192, 192, 192)
#define RGB565_GRAY_7      0xA8A8    // 极亮灰 (224, 224, 224)
#define RGB565_GRAY_8      0xFFFF    // 白（同纯白）

// ===================== 嵌入式UI常用交互色 =====================
#define RGB565_BTN_NORMAL  0x528B    // 按钮常态（浅蓝灰）
#define RGB565_BTN_PRESS   0x2A52    // 按钮按下（深蓝灰）
#define RGB565_BTN_HIGHLIGHT 0x041F  // 按钮高亮（浅蓝）
#define RGB565_TEXT_NORMAL 0xFFFF    // 文字常态（白）
#define RGB565_TEXT_WARN   0xFC60    // 警告文字（橙）
#define RGB565_TEXT_ERROR  0xF800    // 错误文字（红）
#define RGB565_BG_MAIN     0x0000    // 主背景（黑）
#define RGB565_BG_SUB      0x1818    // 副背景（极暗灰）


// https://blog.csdn.net/qq_20222919/article/details/116168044
// #696969
#define DIMGRAY 0X6B4D
// #808080
#define GRAY    0X8410
// #C0C0C0
#define SILVER  0XC618
// #A52A2A
#define BROWN   0XA145
// #800000
#define MAROON  0X8000
// #FA8072
#define SALMON  0XFC0E

// https://github.com/adafruit/Adafruit-ST7735-Library/blob/master/Adafruit_ST7735.h
// Some ready-made 16-bit ('565') color settings:
#define ST77XX_BLACK 0x0000
#define ST77XX_WHITE 0xFFFF
#define ST77XX_RED 0xF800
#define ST77XX_GREEN 0x07E0
#define ST77XX_BLUE 0x001F
#define ST77XX_CYAN 0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW 0xFFE0
#define ST77XX_ORANGE 0xFC00

// Some ready-made 16-bit ('565') color settings:
#define ST7735_BLACK ST77XX_BLACK
#define ST7735_WHITE ST77XX_WHITE
#define ST7735_RED ST77XX_RED
#define ST7735_GREEN ST77XX_GREEN
#define ST7735_BLUE ST77XX_BLUE
#define ST7735_CYAN ST77XX_CYAN
#define ST7735_MAGENTA ST77XX_MAGENTA
#define ST7735_YELLOW ST77XX_YELLOW
#define ST7735_ORANGE ST77XX_ORANGE

// https://blog.csdn.net/Lennon8_8/article/details/120383321
//---------------------颜色表--------------------
//		\\浅粉红
#define GUI_LightPink		        0xFDB8
//		\\粉红
#define GUI_Pink		            0xFE19
//		\\猩红
#define GUI_Crimson		            0xD8A7
//		\\脸红的淡紫色
#define GUI_LavenderBlush		    0xFF9E
//		\\苍白的紫罗兰红色
#define GUI_PaleVioletRed		    0xDB92
//		\\热情的粉红
#define GUI_HotPink		            0xFB56
//		\\深粉色
#define GUI_DeepPink		        0xF8B2
//		\\适中的紫罗兰红色
#define GUI_MediumVioletRed		    0xC0B0
//		\\兰花的紫色
#define GUI_Orchid		            0xDB9A
//		\\蓟		          
#define GUI_Thistle  0xDDFB
//		\\李子
#define GUI_plum		            0xDD1B
//		\\紫罗兰
#define GUI_Violet		            0xEC1D
//		\\洋红
#define GUI_Magenta		            0xF81F
//		\\灯笼海棠(紫红色)
#define GUI_Fuchsia		            0xF81F
//		\\深洋红色
#define GUI_DarkMagenta		        0x8811
//		\\紫色
#define GUI_Purple		            0x8010
//		\\适中的兰花紫
#define GUI_MediumOrchid		    0xBABA
//		\\深紫罗兰色
#define GUI_DarkVoilet		        0x901A
//		\\深兰花紫
#define GUI_DarkOrchid		        0x9999
//		\\靛青
#define GUI_Indigo		            0x4810
//		\\深紫罗兰的蓝色
#define GUI_BlueViolet		        0x895C
//		\\适中的紫色
#define GUI_MediumPurple		    0x939B
//		\\适中的板岩暗蓝灰色
#define GUI_MediumSlateBlue		    0x7B5D
//		\\板岩暗蓝灰色
#define GUI_SlateBlue		        0x6AD9
//		\\深岩暗蓝灰色
#define GUI_DarkSlateBlue		    0x49F1
//		\\熏衣草花的淡紫色
#define GUI_Lavender		        0xE73F
//		\\幽灵的白色
#define GUI_GhostWhite		        0xFFDF
//		\\纯蓝
#define GUI_Blue		            0x001F
//		\\适中的蓝色
#define GUI_MediumBlue		        0x0019
//		\\午夜的蓝色
#define GUI_MidnightBlue		    0x18CE
//		\\深蓝色
#define GUI_DarkBlue		        0x0011
//		\\海军蓝
#define GUI_Navy		            0x0010
//		\\皇家蓝
#define GUI_RoyalBlue		        0x435C
//		\\矢车菊的蓝色
#define GUI_CornflowerBlue		    0x64BD
//		\\淡钢蓝
#define GUI_LightSteelBlue		    0xB63B
//		\\浅石板灰
#define GUI_LightSlateGray		    0x7453
//		\\石板灰
#define GUI_SlateGray		        0x7412
//		\\道奇蓝
#define GUI_DoderBlue		        0x1C9F
//		\\爱丽丝蓝
#define GUI_AliceBlue		        0xF7DF
//		\\钢蓝
#define GUI_SteelBlue		        0x4416
//		\\淡蓝色
#define GUI_LightSkyBlue		    0x867F
//		\\天蓝色
#define GUI_SkyBlue		            0x867D
//		\\深天蓝
#define GUI_DeepSkyBlue		        0x05FF
//		\\淡蓝
#define GUI_LightBLue		        0xAEDC
//		\\火药蓝
#define GUI_PowDerBlue		        0xB71C
//		\\军校蓝
#define GUI_CadetBlue		        0x5CF4
//		\\蔚蓝色
#define GUI_Azure		            0xF7FF
//		\\淡青色
#define GUI_LightCyan		        0xE7FF
//		\\苍白的绿宝石
#define GUI_PaleTurquoise		    0xAF7D
//		\\青色
#define GUI_Cyan		            0x07FF
//		\\水绿色
#define GUI_Aqua		            0xD79C
//		\\深绿宝石
#define GUI_DarkTurquoise		    0x067A
//		\\深石板灰
#define GUI_DarkSlateGray		    0x2A69
//		\\深青色
#define GUI_DarkCyan		        0x0451
//		\\水鸭色
#define GUI_Teal		            0x0410
//		\\适中的绿宝石
#define GUI_MediumTurquoise		    0x4E99
//		\\浅海洋绿
#define GUI_LightSeaGreen		    0x2595
//		\\绿宝石
#define GUI_Turquoise		        0x471A
//		\\绿玉\碧绿色
#define GUI_Auqamarin		        0x7FF5
//		\\适中的碧绿色
#define GUI_MediumAquamarine		0x07D3
//		\\适中的春天的绿色
#define GUI_MediumSpringGreen		0x07EF
//		\\薄荷奶油
#define GUI_MintCream		0xF7FF
//		\\春天的绿色
#define GUI_SpringGreen		0x3D8E
//		\\海洋绿
#define GUI_SeaGreen		0x2C4A
//		\\蜂蜜
#define GUI_Honeydew		0xF7FE
//		\\淡绿色
#define GUI_LightGreen		0x9772
//		\\苍白的绿色
#define GUI_PaleGreen		0x9FD3
//		\\深海洋绿
#define GUI_DarkSeaGreen	0x8DF1
//		\\酸橙绿
#define GUI_LimeGreen		0x3666
//		\\酸橙色
#define GUI_Lime		    0x07E0
//		\\森林绿
#define GUI_ForestGreen		0x2444
//		\\纯绿
#define GUI_Green		    0x0400
//		\\深绿色
#define GUI_DarkGreen		0x0320
//		\\查特酒绿
#define GUI_Chartreuse		0x7FE0
//		\\草坪绿
#define GUI_LawnGreen		0x7FE0
//		\\绿黄色
#define GUI_GreenYellow		0xAFE5
//		\\橄榄土褐色
#define GUI_OliveDrab		0x5345
//		\\米色(浅褐色)
#define GUI_Beige		    0xF7BB
//		\\浅秋麒麟黄
#define GUI_LightGoldenrodYellow		0xFFDA
//		\\象牙
#define GUI_Ivory		    0xFFFE
//		\\浅黄色
#define GUI_LightYellow		0xFFFC
//		\\纯黄
#define GUI_Yellow		    0xFFE0
//		\\橄榄
#define GUI_Olive		    0x8400
//		\\深卡其布
#define GUI_DarkKhaki		0xBDAD
//		\\柠檬薄纱
#define GUI_LemonChiffon	0xFFD9
//		\\灰秋麒麟
#define GUI_PaleGodenrod	0xEF55
//		\\卡其布
#define GUI_Khaki		    0xF731
//		\\金
#define GUI_Gold            0xFEA0
//		\\玉米色
#define GUI_Cornislk		0xFFDB
//		\\秋麒麟
#define GUI_GoldEnrod		0xDD24
//		\\花的白色
#define GUI_FloralWhite		0xFFDE
//		\\老饰带
#define GUI_OldLace		    0xFFBC
//		\\小麦色
#define GUI_Wheat		    0xF6F6
//		\\鹿皮鞋
#define GUI_Moccasin		0xFF36
//		\\橙色
#define GUI_Orange		    0xFD20
//		\\番木瓜
#define GUI_PapayaWhip		0xFF7A
//		\\漂白的杏仁
#define GUI_BlanchedAlmond	0xFF59
//		\\纳瓦霍白
#define GUI_NavajoWhite		0xFEF5
//		\\古代的白色
#define GUI_AntiqueWhite	0xFF5A
//		\\晒黑
#define GUI_Tan		        0xD5B1
//		\\结实的树
#define GUI_BrulyWood		0xDDD0
//		\\(浓汤)乳脂,番茄等
#define GUI_Bisque		    0xFF38
//		\\深橙色
#define GUI_DarkOrange		0xFC60
//		\\亚麻布
#define GUI_Linen		    0xFF9C
//		\\秘鲁
#define GUI_Peru		    0xCC27
//		\\桃色
#define GUI_PeachPuff		0xFED7
//		\\沙棕色
#define GUI_SandyBrown		0xF52C
//		\\巧克力
#define GUI_Chocolate		0xD343
//		\\马鞍棕色
#define GUI_SaddleBrown		0x8A22
//		\\海贝壳
#define GUI_SeaShell		0xFFBD
//		\\黄土赭色
#define GUI_Sienna		    0xA285
//)色
//		\\浅鲜肉(鲑鱼
#define GUI_LightSalmon		0xFD0F
//		\\珊瑚
#define GUI_Coral		    0xFBEA
//		\\橙红色
#define GUI_OrangeRed		0xFA20
//		\\深鲜肉(鲑鱼)色
#define GUI_DarkSalmon		0xECAF
//		\\番茄
#define GUI_Tomato		    0xFB08
//		\\薄雾玫瑰
#define GUI_MistyRose		0xFF3C
//		\\鲜肉(鲑鱼)色
#define GUI_Salmon		    0xFC0E
//		\\雪
#define GUI_Snow		    0xFFDF
//		\\淡珊瑚色
#define GUI_LightCoral		0xF410
//		\\玫瑰棕色
#define GUI_RosyBrown		0xBC71
//		\\印度红
#define GUI_IndianRed		0xCAEB
//		\\纯红
#define GUI_Red			    0xF800
//		\\棕色
#define GUI_Brown		    0xA145
//		\\耐火砖
#define GUI_FireBrick		0xB104
//		\\深红色
#define GUI_DarkRed		    0x8800
//		\\栗色
#define GUI_Maroon		    0x8000
//		\\纯白
#define GUI_White		    0xFFFF
//		\\白烟
#define GUI_WhiteSmoke		0xF7BE
//		\\亮灰色
#define GUI_Gainsboro		0xDEFB
//		\\浅灰色
#define GUI_LightGrey		0xD69A
//		\\银白色
#define GUI_Silver		    0xC618
//		\\深灰色
#define GUI_DarkGray	    0xAD55
//		\\灰色
#define GUI_Gray		    0x8410
//		\\暗淡的灰色
#define GUI_DimGray		    0x6B4D
//		\\纯黑
#define GUI_Black		    0x0000


#endif // !__RGB_COLOR_H_