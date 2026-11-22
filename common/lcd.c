/**
 * lcd.c
 * ST7735 LCD显示实现 (简化版)
 * 完整实现需要参考ST7735数据手册
 */

#include "lcd.h"
#include "board_pins.h"
#include <string.h>
#include <stdio.h>

// TivaWare库
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"

/* ===== 内部状态 ===== */
static struct {
    device_type_t device_type;
    bool initialized;
} lcd_ctx;

/* ===== ST7735命令 ===== */
#define ST7735_NOP      0x00
#define ST7735_SWRESET  0x01
#define ST7735_SLPOUT   0x11
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A

/* ===== 内部函数 ===== */
static void lcd_write_cmd(uint8_t cmd);
static void lcd_write_data(uint8_t data);
static void lcd_write_data16(uint16_t data);
static void lcd_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
static void lcd_delay_ms(uint32_t ms);

// 简单字体 (8x16)
extern const uint8_t font_8x16[][16];

/**
 * 延时
 */
static void lcd_delay_ms(uint32_t ms) {
    SysCtlDelay(SYSTEM_CLOCK_HZ / 3 / 1000 * ms);
}

/**
 * 发送命令
 */
static void lcd_write_cmd(uint8_t cmd) {
    GPIOPinWrite(GUN_LCD_GPIO_BASE, GUN_LCD_DC, 0);  // DC=0 命令
    GPIOPinWrite(GUN_LCD_GPIO_BASE, GUN_LCD_CS, 0);  // CS=0 选中
    SSIDataPut(GUN_LCD_SSI_BASE, cmd);
    while(SSIBusy(GUN_LCD_SSI_BASE));
    GPIOPinWrite(GUN_LCD_GPIO_BASE, GUN_LCD_CS, GUN_LCD_CS);  // CS=1
}

/**
 * 发送数据
 */
static void lcd_write_data(uint8_t data) {
    GPIOPinWrite(GUN_LCD_GPIO_BASE, GUN_LCD_DC, GUN_LCD_DC);  // DC=1 数据
    GPIOPinWrite(GUN_LCD_GPIO_BASE, GUN_LCD_CS, 0);
    SSIDataPut(GUN_LCD_SSI_BASE, data);
    while(SSIBusy(GUN_LCD_SSI_BASE));
    GPIOPinWrite(GUN_LCD_GPIO_BASE, GUN_LCD_CS, GUN_LCD_CS);
}

/**
 * 发送16位数据
 */
static void lcd_write_data16(uint16_t data) {
    lcd_write_data(data >> 8);
    lcd_write_data(data & 0xFF);
}

/**
 * 设置绘图窗口
 */
static void lcd_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    lcd_write_cmd(ST7735_CASET);
    lcd_write_data16(x);
    lcd_write_data16(x + w - 1);

    lcd_write_cmd(ST7735_RASET);
    lcd_write_data16(y);
    lcd_write_data16(y + h - 1);

    lcd_write_cmd(ST7735_RAMWR);
}

/**
 * 初始化LCD
 */
void lcd_init(device_type_t device_type) {
    lcd_ctx.device_type = device_type;

    // 使能外设
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // 配置SSI引脚
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);
    GPIOPinTypeSSI(GUN_LCD_GPIO_BASE, GUN_LCD_SCK | GUN_LCD_MOSI);

    // 配置控制引脚
    GPIOPinTypeGPIOOutput(GUN_LCD_GPIO_BASE, GUN_LCD_DC | GUN_LCD_CS | GUN_LCD_RST);

    // 配置SSI
    SSIConfigSetExpClk(GUN_LCD_SSI_BASE, SYSTEM_CLOCK_HZ, SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 10000000, 8);  // 10MHz
    SSIEnable(GUN_LCD_SSI_BASE);

    // 硬件复位
    GPIOPinWrite(GUN_LCD_GPIO_BASE, GUN_LCD_RST, 0);
    lcd_delay_ms(10);
    GPIOPinWrite(GUN_LCD_GPIO_BASE, GUN_LCD_RST, GUN_LCD_RST);
    lcd_delay_ms(120);

    // 初始化序列
    lcd_write_cmd(ST7735_SWRESET);
    lcd_delay_ms(150);

    lcd_write_cmd(ST7735_SLPOUT);
    lcd_delay_ms(500);

    lcd_write_cmd(ST7735_COLMOD);
    lcd_write_data(0x05);  // 16-bit color

    lcd_write_cmd(ST7735_MADCTL);
    lcd_write_data(0xC0);  // Row/Column exchange

    lcd_write_cmd(ST7735_NORON);
    lcd_delay_ms(10);

    lcd_write_cmd(ST7735_DISPON);
    lcd_delay_ms(100);

    lcd_ctx.initialized = true;

    // 显示启动画面
    lcd_show_splash(device_type);
}

/**
 * 清屏
 */
void lcd_clear(uint16_t color) {
    lcd_set_window(0, 0, LCD_WIDTH, LCD_HEIGHT);
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        lcd_write_data16(color);
    }
}

/**
 * 填充矩形
 */
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    lcd_set_window(x, y, w, h);
    for (uint32_t i = 0; i < w * h; i++) {
        lcd_write_data16(color);
    }
}

/**
 * 显示字符串
 */
void lcd_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg) {
    while (*str) {
        lcd_draw_char(x, y, *str, fg, bg);
        x += 8;
        if (x + 8 > LCD_WIDTH) {
            x = 0;
            y += 16;
        }
        str++;
    }
}

/**
 * 显示字符 (简化版, 需要完整字库)
 */
void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg) {
    // 简化实现: 显示方块代表字符
    lcd_fill_rect(x, y, 8, 16, fg);
}

/**
 * === 枪端界面 ===
 */

void lcd_gun_update_ammo(uint8_t ammo, uint8_t max_ammo) {
    char buf[32];

    // 清除弹量区域
    lcd_fill_rect(0, 40, LCD_WIDTH, 60, COLOR_BLACK);

    // 显示"AMMO"标题
    lcd_draw_string(40, 45, "AMMO", COLOR_CYAN, COLOR_BLACK);

    // 显示大数字
    snprintf(buf, sizeof(buf), "%d/%d", ammo, max_ammo);
    lcd_draw_string(30, 70, buf, COLOR_WHITE, COLOR_BLACK);

    // 弹量条
    uint16_t bar_width = (ammo * 100) / max_ammo;
    lcd_fill_rect(14, 100, bar_width, 10,
                  ammo > 3 ? COLOR_GREEN : COLOR_RED);
    lcd_draw_rect(14, 100, 100, 10, COLOR_WHITE);
}

void lcd_gun_update_connection(bool connected) {
    uint16_t color = connected ? COLOR_GREEN : COLOR_RED;
    const char *text = connected ? "CONN" : "DISC";

    lcd_fill_rect(84, 5, 40, 12, COLOR_BLACK);
    lcd_draw_string(88, 7, text, color, COLOR_BLACK);
}

void lcd_gun_show_result(const char *result, uint16_t color) {
    lcd_fill_rect(20, 120, 88, 20, COLOR_BLACK);
    lcd_draw_string(30, 125, result, color, COLOR_BLACK);
}

/**
 * === 靶端界面 ===
 */

void lcd_target_update_score(uint16_t score) {
    char buf[16];

    // 清除分数区域
    lcd_fill_rect(0, 40, LCD_WIDTH, 60, COLOR_BLACK);

    // 显示"SCORE"标题
    lcd_draw_string(38, 45, "SCORE", COLOR_YELLOW, COLOR_BLACK);

    // 显示大数字
    snprintf(buf, sizeof(buf), "%03d", score);
    lcd_draw_string(42, 75, buf, COLOR_WHITE, COLOR_BLACK);
}

void lcd_target_update_connection(bool connected) {
    uint16_t color = connected ? COLOR_GREEN : COLOR_RED;
    const char *text = connected ? "READY" : "WAIT";

    lcd_fill_rect(80, 5, 45, 12, COLOR_BLACK);
    lcd_draw_string(82, 7, text, color, COLOR_BLACK);
}

void lcd_target_show_hit_animation(void) {
    // 简单闪烁效果
    lcd_fill_rect(10, 110, 108, 30, COLOR_RED);
    lcd_draw_string(38, 120, "HIT!", COLOR_WHITE, COLOR_RED);
    lcd_delay_ms(200);
    lcd_fill_rect(10, 110, 108, 30, COLOR_BLACK);
}

/**
 * === 通用界面 ===
 */

void lcd_show_splash(device_type_t device_type) {
    lcd_clear(COLOR_BLACK);

    // 标题
    lcd_draw_string(20, 40, "LaserTag", COLOR_CYAN, COLOR_BLACK);
    lcd_draw_string(28, 60, "System", COLOR_CYAN, COLOR_BLACK);

    // 设备类型
    const char *type_str = (device_type == DEVICE_GUN) ? "GUN" : "TARGET";
    lcd_draw_string(40, 85, type_str, COLOR_YELLOW, COLOR_BLACK);

    // 版本
    lcd_draw_string(32, 110, "v1.0", COLOR_GRAY, COLOR_BLACK);

    lcd_delay_ms(2000);
    lcd_clear(COLOR_BLACK);
}

void lcd_show_connecting(void) {
    lcd_fill_rect(10, 70, 108, 20, COLOR_BLACK);
    lcd_draw_string(20, 75, "Connecting...", COLOR_YELLOW, COLOR_BLACK);
}

void lcd_show_error(const char *msg) {
    lcd_clear(COLOR_BLACK);
    lcd_draw_string(30, 70, "ERROR", COLOR_RED, COLOR_BLACK);
    lcd_draw_string(10, 90, msg, COLOR_WHITE, COLOR_BLACK);
}
