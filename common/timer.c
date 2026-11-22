/**
 * timer.c
 * 定时与超时管理
 */

#include "timer.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

static volatile uint32_t system_tick = 0;

/**
 * 定时器中断处理
 */
void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    system_tick++;
}

/**
 * 初始化定时器 (1ms tick)
 */
void timer_init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    // 设置1ms周期
    TimerLoadSet(TIMER0_BASE, TIMER_A, SYSTEM_CLOCK_HZ / 1000 - 1);

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE, TIMER_A);

    system_tick = 0;
}

/**
 * 获取当前时间戳 (ms)
 */
uint32_t timer_get_ms(void) {
    return system_tick;
}

/**
 * 延时 (ms)
 */
void timer_delay_ms(uint32_t ms) {
    uint32_t start = system_tick;
    while ((system_tick - start) < ms);
}

/**
 * 检查是否超时
 */
bool timer_is_timeout(uint32_t start_time, uint32_t timeout_ms) {
    return (system_tick - start_time) >= timeout_ms;
}
