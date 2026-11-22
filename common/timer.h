/**
 * timer.h
 * 定时与超时管理
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "game_config.h"

/**
 * 初始化系统定时器 (1ms tick)
 */
void timer_init(void);

/**
 * 获取系统运行时间 (ms)
 */
uint32_t timer_get_ms(void);

/**
 * 延时 (ms)
 */
void timer_delay_ms(uint32_t ms);

/**
 * 检查是否超时
 * @param start_time 起始时间
 * @param timeout_ms 超时时间(ms)
 * @return true=已超时
 */
bool timer_is_timeout(uint32_t start_time, uint32_t timeout_ms);

#endif // TIMER_H
