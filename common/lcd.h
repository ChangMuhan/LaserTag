/**
 * lcd.h
 * ST7735 LCD显示接口
 * 负责人: C (通信显示负责人)
 */

#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include <stdbool.h>
#include "game_config.h"

/* ===== LCD尺寸 ===== */
#define LCD_WIDTH           128
#define LCD_HEIGHT          160

/* ===== 颜色定义 (RGB565) ===== */
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_GRAY          0x8410
#define COLOR_ORANGE        0xFD20
#define COLOR_DARKGREEN     0x03E0

/**
 * 初始化LCD
 * @param device_type 设备类型(枪/靶)
 */
void lcd_init(device_type_t device_type);

/**
 * 清屏
 * @param color 填充颜色
 */
void lcd_clear(uint16_t color);

/**
 * 画点
 */
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * 画线
 */
void lcd_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * 画矩形
 */
void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * 填充矩形
 */
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * 画圆
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * 显示字符 (8x16点阵)
 */
void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg);

/**
 * 显示字符串
 */
void lcd_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg);

/**
 * 显示大数字 (用于弹量和分数, 16x24点阵)
 */
void lcd_draw_big_number(uint16_t x, uint16_t y, uint8_t num, uint16_t fg, uint16_t bg);

/**
 * === 枪端专用界面 ===
 */

/**
 * 更新枪端弹量显示
 * @param ammo 当前弹量
 * @param max_ammo 最大弹量
 */
void lcd_gun_update_ammo(uint8_t ammo, uint8_t max_ammo);

/**
 * 更新枪端连接状态
 * @param connected 是否已连接
 */
void lcd_gun_update_connection(bool connected);

/**
 * 显示枪端射击反馈 (命中/未命中/超时)
 */
void lcd_gun_show_result(const char *result, uint16_t color);

/**
 * === 靶端专用界面 ===
 */

/**
 * 更新靶端分数显示
 * @param score 当前分数
 */
void lcd_target_update_score(uint16_t score);

/**
 * 更新靶端连接状态
 */
void lcd_target_update_connection(bool connected);

/**
 * 显示靶端命中动画
 */
void lcd_target_show_hit_animation(void);

/**
 * === 通用功能 ===
 */

/**
 * 显示启动画面
 */
void lcd_show_splash(device_type_t device_type);

/**
 * 显示Wi-Fi连接中
 */
void lcd_show_connecting(void);

/**
 * 显示错误信息
 */
void lcd_show_error(const char *msg);

#endif // LCD_H
