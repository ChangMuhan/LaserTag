/**
 * button.h
 * 按键输入与消抖
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>

/**
 * 初始化按键
 * @param gpio_base GPIO基址
 * @param gpio_pin GPIO引脚
 * @param debounce_ms 消抖时间(ms)
 */
void button_init(uint32_t gpio_base, uint8_t gpio_pin, uint32_t debounce_ms);

/**
 * 按键扫描 (需在主循环中定期调用)
 */
void button_scan(void);

/**
 * 当前是否按下
 */
bool button_is_pressed(void);

/**
 * 检测按下边沿 (下降沿)
 */
bool button_is_falling_edge(void);

#endif // BUTTON_H
