#ifndef ST7735_H
#define ST7735_H

#include <stdint.h>

// ST7735 1.8寸 128x160 显示屏驱动（SPI）
// 只实现了「数字显示」所需的最小功能，可继续扩展

// RGB565 颜色定义
#define ST7735_BLACK  0x0000
#define ST7735_WHITE  0xFFFF
#define ST7735_RED    0xF800
#define ST7735_GREEN  0x07E0
#define ST7735_BLUE   0x001F
#define ST7735_YELLOW 0xFFE0

void ST7735_Init(void);
void ST7735_FillScreen(uint16_t color);
void ST7735_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
// 画一个数字字符（仅支持 '0'~'9'），size 为放大倍数
void ST7735_DrawChar(uint16_t x, uint16_t y, uint8_t ch, uint16_t color, uint16_t bg, uint8_t size);
// 在 (x,y) 处打印整数，size 为放大倍数
void ST7735_PrintInt(uint16_t x, uint16_t y, int32_t num, uint16_t color, uint16_t bg, uint8_t size);

#endif
