/**
 * esp_at.h
 * ESP-01S Wi-Fi模块AT指令驱动
 * 负责人: C (通信显示负责人)
 */

#ifndef ESP_AT_H
#define ESP_AT_H

#include <stdint.h>
#include <stdbool.h>
#include "game_config.h"

/* ===== Wi-Fi模式 ===== */
typedef enum {
    WIFI_MODE_AP = 2,       // 热点模式
    WIFI_MODE_STA = 1,      // 客户端模式
    WIFI_MODE_AP_STA = 3    // 混合模式
} wifi_mode_t;

/* ===== 连接状态回调 ===== */
typedef void (*esp_conn_callback_t)(bool connected);

/* ===== 数据接收回调 ===== */
typedef void (*esp_data_callback_t)(const uint8_t *data, uint16_t len);

/* ===== 初始化配置 ===== */
typedef struct {
    device_type_t device_type;      // 设备类型(枪/靶)
    const char *ssid;               // SSID
    const char *password;           // 密码
    const char *server_ip;          // 服务器IP(客户端模式)
    uint16_t server_port;           // 服务器端口
    esp_conn_callback_t conn_cb;    // 连接状态回调
    esp_data_callback_t data_cb;    // 数据接收回调
} esp_config_t;

/**
 * 初始化ESP模块
 * @param config 配置参数
 * @return true=成功, false=失败
 */
bool esp_init(const esp_config_t *config);

/**
 * 主循环处理(需定期调用)
 */
void esp_process(void);

/**
 * 发送数据
 * @param data 数据缓冲区
 * @param len 数据长度
 * @return true=成功发送, false=失败
 */
bool esp_send_data(const uint8_t *data, uint16_t len);

/**
 * 获取连接状态
 */
conn_state_t esp_get_connection_state(void);

/**
 * 重置ESP模块
 */
void esp_reset(void);

/**
 * 检查是否有数据可接收
 */
bool esp_data_available(void);

#endif // ESP_AT_H
