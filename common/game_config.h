/**
 * game_config.h
 * 游戏参数配置
 */

#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* ===== 游戏规则 ===== */
#define INITIAL_AMMO            10      // 初始弹药数
#define SCORE_PER_HIT           1       // 每次命中得分
#define MAX_SCORE               99      // 最大分数显示

/* ===== 时序参数 ===== */
#define LASER_PULSE_MS          100     // 激光脉冲持续时间(ms)
#define TRIGGER_DEBOUNCE_MS     50      // 扳机消抖时间(ms)
#define HIT_DETECT_WINDOW_MS    200     // 命中检测窗口(ms)
#define HIT_LOCKOUT_MS          500     // 命中锁定时间,防止重复计分(ms)
#define VIBRATION_DURATION_MS   100     // 震动持续时间(ms)

/* ===== 通信超时 ===== */
#define COMM_READY_TIMEOUT_MS   2000    // 等待READY超时(ms)
#define COMM_RESULT_TIMEOUT_MS  3000    // 等待RESULT超时(ms)
#define COMM_RETRY_COUNT        3       // 重试次数
#define WIFI_CONNECT_TIMEOUT_MS 10000   // Wi-Fi连接超时(ms)

/* ===== ADC阈值(需标定) ===== */
#define ADC_BASELINE            500     // 环境光基线值(待标定)
#define ADC_HIT_THRESHOLD       800     // 命中阈值(待标定)
#define ADC_STABLE_SAMPLES      3       // 稳定检测所需采样次数

/* ===== Wi-Fi配置 ===== */
#define WIFI_SSID               "LaserTag_AP"
#define WIFI_PASSWORD           "12345678"
#define WIFI_SERVER_IP          "192.168.4.1"
#define WIFI_SERVER_PORT        8080

/* ===== 设备类型 ===== */
typedef enum {
    DEVICE_GUN = 0,
    DEVICE_TARGET = 1
} device_type_t;

/* ===== 游戏状态 ===== */
typedef enum {
    GAME_IDLE = 0,          // 空闲
    GAME_READY,             // 准备就绪
    GAME_ARMED,             // 已装填,等待发射
    GAME_FIRING,            // 发射中
    GAME_WAITING_RESULT,    // 等待结果
    GAME_HIT,               // 命中
    GAME_MISS,              // 未命中
    GAME_TIMEOUT,           // 超时
    GAME_RELOADING,         // 换弹中
    GAME_GAME_OVER          // 游戏结束
} game_state_t;

/* ===== 连接状态 ===== */
typedef enum {
    CONN_DISCONNECTED = 0,
    CONN_CONNECTING,
    CONN_CONNECTED,
    CONN_ERROR
} conn_state_t;

#endif // GAME_CONFIG_H
