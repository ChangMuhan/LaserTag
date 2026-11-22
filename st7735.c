#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h"
#include "st7735.h"

// 引脚分配（按之前定的接线）：
//   SCK  = PA2 (SSI0Clk)
//   MOSI = PA5 (SSI0Tx)
//   CS   = PB1
//   DC   = PB2
//   RST  = PB3
#define TFT_CS_PORT  GPIO_PORTB_BASE
#define TFT_CS_PIN   GPIO_PIN_1
#define TFT_DC_PORT  GPIO_PORTB_BASE
#define TFT_DC_PIN   GPIO_PIN_2
#define TFT_RST_PORT GPIO_PORTB_BASE
#define TFT_RST_PIN  GPIO_PIN_3

#define TFT_WIDTH  128
#define TFT_HEIGHT 160

#define CS_LOW()   GPIOPinWrite(TFT_CS_PORT,  TFT_CS_PIN,  0)
#define CS_HIGH()  GPIOPinWrite(TFT_CS_PORT,  TFT_CS_PIN,  TFT_CS_PIN)
#define DC_LOW()   GPIOPinWrite(TFT_DC_PORT,  TFT_DC_PIN,  0)
#define DC_HIGH()  GPIOPinWrite(TFT_DC_PORT,  TFT_DC_PIN,  TFT_DC_PIN)
#define RST_LOW()  GPIOPinWrite(TFT_RST_PORT, TFT_RST_PIN, 0)
#define RST_HIGH() GPIOPinWrite(TFT_RST_PORT, TFT_RST_PIN, TFT_RST_PIN)

// Gamma 校正表（文件作用域）
static const uint8_t gamma_pos[16] =
    {0x02,0x1C,0x07,0x12,0x37,0x32,0x29,0x2D,
     0x29,0x25,0x2B,0x39,0x00,0x01,0x03,0x10};
static const uint8_t gamma_neg[16] =
    {0x03,0x1D,0x07,0x06,0x2E,0x2C,0x29,0x2D,
     0x2E,0x2E,0x37,0x3F,0x00,0x00,0x02,0x10};

// 5x7 数字字体，每列一个字节，bit0 = 顶行
static const uint8_t font_digits[10][5] =
{
    {0x3E,0x51,0x49,0x45,0x3E},  // 0
    {0x00,0x42,0x7F,0x40,0x00},  // 1
    {0x42,0x61,0x51,0x49,0x46},  // 2
    {0x21,0x41,0x45,0x4B,0x31},  // 3
    {0x18,0x14,0x12,0x7F,0x10},  // 4
    {0x27,0x45,0x45,0x45,0x39},  // 5
    {0x3C,0x4A,0x49,0x49,0x30},  // 6
    {0x01,0x71,0x09,0x05,0x03},  // 7
    {0x36,0x49,0x49,0x49,0x36},  // 8
    {0x06,0x49,0x49,0x29,0x1E},  // 9
};

static void delay_ms(uint32_t ms)
{
    SysCtlDelay(SysCtlClockGet() / 3000 * ms);
}

// 阻塞式发送一个字节
static void ssi_send(uint8_t data)
{
    while (SSIBusy(SSI0_BASE)) {}
    SSIDataPut(SSI0_BASE, data);
    while (SSIBusy(SSI0_BASE)) {}
}

static void write_cmd(uint8_t cmd)
{
    DC_LOW();
    CS_LOW();
    ssi_send(cmd);
    CS_HIGH();
}

static void write_data(uint8_t data)
{
    DC_HIGH();
    CS_LOW();
    ssi_send(data);
    CS_HIGH();
}

// 设置像素写入窗口，并以 RAMWR 结束（后续都是像素数据）
static void set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    write_cmd(0x2A);  // CASET
    write_data(0x00); write_data(x0); write_data(0x00); write_data(x1);
    write_cmd(0x2B);  // RASET
    write_data(0x00); write_data(y0); write_data(0x00); write_data(y1);
    write_cmd(0x2C);  // RAMWR
}

void ST7735_Init(void)
{
    int i;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // SSI0：PA2=时钟，PA5=数据发送
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_2);
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(),
                       SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 4000000, 8);
    SSIEnable(SSI0_BASE);

    // CS / DC / RST 为普通输出
    GPIOPinTypeGPIOOutput(TFT_CS_PORT,  TFT_CS_PIN);
    GPIOPinTypeGPIOOutput(TFT_DC_PORT,  TFT_DC_PIN);
    GPIOPinTypeGPIOOutput(TFT_RST_PORT, TFT_RST_PIN);
    CS_HIGH();
    DC_HIGH();

    // 复位脉冲
    RST_HIGH();
    delay_ms(10);
    RST_LOW();
    delay_ms(10);
    RST_HIGH();
    delay_ms(120);

    // ---- 初始化序列（ST7735R，128x160）----
    write_cmd(0x01);              // SWRESET
    delay_ms(150);
    write_cmd(0x11);              // SLPOUT
    delay_ms(500);

    write_cmd(0xB1);              // FRMCTR1
    write_data(0x01); write_data(0x2C); write_data(0x2D);
    write_cmd(0xB2);              // FRMCTR2
    write_data(0x01); write_data(0x2C); write_data(0x2D);
    write_cmd(0xB3);              // FRMCTR3
    write_data(0x01); write_data(0x2C); write_data(0x2D);
    write_data(0x01); write_data(0x2C); write_data(0x2D);
    write_cmd(0xB4);              // INVCTR
    write_data(0x07);

    write_cmd(0xC0);              // PWCTR1
    write_data(0xA2); write_data(0x02); write_data(0x84);
    write_cmd(0xC1);              // PWCTR2
    write_data(0xC5);
    write_cmd(0xC2);              // PWCTR3
    write_data(0x0A); write_data(0x00);
    write_cmd(0xC3);              // PWCTR4
    write_data(0x8A); write_data(0x2A);
    write_cmd(0xC4);              // PWCTR5
    write_data(0x8A); write_data(0xEE);
    write_cmd(0xC5);              // VMCTR1
    write_data(0x0E);

    write_cmd(0xE0);              // GMCTRP1
    for (i = 0; i < 16; i++) write_data(gamma_pos[i]);
    write_cmd(0xE1);              // GMCTRN1
    for (i = 0; i < 16; i++) write_data(gamma_neg[i]);

    write_cmd(0x20);              // INVOFF（若颜色反了改成 0x21 INVON）
    write_cmd(0x36);              // MADCTL（旋转方向）
    write_data(0xC0);             // 清 BGR 位，纠正红蓝反色
    write_cmd(0x3A);              // COLMOD 16bit
    write_data(0x05);

    write_cmd(0x2A);              // CASET 0..127
    write_data(0x00); write_data(0x00); write_data(0x00); write_data(0x7F);
    write_cmd(0x2B);              // RASET 0..159
    write_data(0x00); write_data(0x00); write_data(0x00); write_data(0x9F);

    write_cmd(0x13);              // NORON
    delay_ms(10);
    write_cmd(0x29);              // DISPON
    delay_ms(100);

    ST7735_FillScreen(ST7735_BLACK);
}

void ST7735_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint32_t i;
    uint32_t n = (uint32_t)w * h;

    set_addr_window(x, y, x + w - 1, y + h - 1);
    DC_HIGH();
    CS_LOW();
    for (i = 0; i < n; i++)
    {
        ssi_send(hi);
        ssi_send(lo);
    }
    CS_HIGH();
}

void ST7735_FillScreen(uint16_t color)
{
    ST7735_FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    ST7735_FillRect(x, y, 1, 1, color);
}

void ST7735_DrawChar(uint16_t x, uint16_t y, uint8_t ch,
                     uint16_t color, uint16_t bg, uint8_t size)
{
    int col, row;
    const uint8_t *glyph;

    if (ch < '0' || ch > '9') return;
    glyph = font_digits[ch - '0'];
    for (col = 0; col < 5; col++)
    {
        uint8_t line = glyph[col];
        for (row = 0; row < 7; row++)
        {
            uint16_t c = (line & (1 << row)) ? color : bg;
            ST7735_FillRect(x + col * size, y + row * size, size, size, c);
        }
    }
}

void ST7735_PrintInt(uint16_t x, uint16_t y, int32_t num,
                     uint16_t color, uint16_t bg, uint8_t size)
{
    char buf[12];
    int i = 0;
    uint16_t cx = x;

    if (num < 0) num = -num;   // 当前只显示数字，不画负号
    if (num == 0)
    {
        ST7735_DrawChar(x, y, '0', color, bg, size);
        return;
    }
    while (num)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i)
    {
        i--;
        ST7735_DrawChar(cx, y, buf[i], color, bg, size);
        cx += 6 * size;   // 每字符占 5 列 + 1 列间距
    }
}
