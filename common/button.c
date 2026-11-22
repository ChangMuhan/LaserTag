/**
 * button.c
 * 按键输入与消抖
 */

#include "button.h"
#include "timer.h"
#include "driverlib/gpio.h"

/* ===== 按键状态 ===== */
typedef struct {
    uint32_t gpio_base;
    uint8_t gpio_pin;
    bool last_state;
    bool current_state;
    uint32_t last_change_time;
    uint32_t debounce_ms;
    bool initialized;
} button_state_t;

static button_state_t button_ctx;

/**
 * 初始化按键
 */
void button_init(uint32_t gpio_base, uint8_t gpio_pin, uint32_t debounce_ms) {
    button_ctx.gpio_base = gpio_base;
    button_ctx.gpio_pin = gpio_pin;
    button_ctx.debounce_ms = debounce_ms;
    button_ctx.last_state = true;  // 假设上拉
    button_ctx.current_state = true;
    button_ctx.last_change_time = 0;
    button_ctx.initialized = true;
}

/**
 * 按键扫描 (需定期调用)
 */
void button_scan(void) {
    if (!button_ctx.initialized) {
        return;
    }

    // 读取当前电平
    bool raw_state = (GPIOPinRead(button_ctx.gpio_base, button_ctx.gpio_pin) != 0);

    // 消抖处理
    if (raw_state != button_ctx.last_state) {
        button_ctx.last_change_time = timer_get_ms();
        button_ctx.last_state = raw_state;
    } else if (timer_is_timeout(button_ctx.last_change_time, button_ctx.debounce_ms)) {
        button_ctx.current_state = raw_state;
    }
}

/**
 * 是否按下
 */
bool button_is_pressed(void) {
    return !button_ctx.current_state;  // 低电平有效
}

/**
 * 检测按下边沿 (返回后需手动清除状态)
 */
bool button_is_falling_edge(void) {
    static bool last_read_state = false;
    bool pressed = button_is_pressed();
    bool edge = (!last_read_state && pressed);
    last_read_state = pressed;
    return edge;
}
