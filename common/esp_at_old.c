/**
 * esp_at.c
 * ESP-01S Wi-Fi模块AT指令驱动实现
 */

#include "esp_at.h"
#include "board_pins.h"
#include <string.h>
#include <stdio.h>

// TivaWare库
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

/* ===== 内部状态 ===== */
static struct {
    esp_config_t config;
    conn_state_t conn_state;
    char rx_buffer[256];
    uint16_t rx_index;
    uint32_t last_heartbeat;
    bool initialized;

    // AT指令响应验证
    char at_response[128];
    bool at_response_ready;
    bool at_response_ok;

    // +IPD数据缓存
    uint8_t ipd_buffer[256];
    uint16_t ipd_length;
    bool ipd_ready;
    uint8_t client_id;  // 多连接时的客户端ID
} esp_ctx;

/* ===== 内部函数声明 ===== */
static bool esp_send_cmd(const char *cmd, const char *expected, uint32_t timeout_ms);
static void esp_uart_init(void);
static void esp_parse_response(const char *response);

/**
 * UART中断处理
 */
void ESP_UART_IntHandler(void) {
    uint32_t status = UARTIntStatus(ESP_UART_BASE, true);
    UARTIntClear(ESP_UART_BASE, status);

    while (UARTCharsAvail(ESP_UART_BASE)) {
        char c = UARTCharGetNonBlocking(ESP_UART_BASE);

        if (esp_ctx.rx_index < sizeof(esp_ctx.rx_buffer) - 1) {
            esp_ctx.rx_buffer[esp_ctx.rx_index++] = c;

            // 检测到换行,处理完整响应
            if (c == '\n') {
                esp_ctx.rx_buffer[esp_ctx.rx_index] = '\0';
                esp_parse_response(esp_ctx.rx_buffer);
                esp_ctx.rx_index = 0;
            }
        } else {
            // 缓冲区满,重置
            esp_ctx.rx_index = 0;
        }
    }
}

/**
 * 初始化UART
 */
static void esp_uart_init(void) {
    // 使能外设
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    // 配置引脚复用
    GPIOPinConfigure(GPIO_PD7_U2TX);
    GPIOPinConfigure(GPIO_PD6_U2RX);
    GPIOPinTypeUART(ESP_GPIO_BASE, ESP_TX_PIN | ESP_RX_PIN);

    // 配置UART参数
    UARTConfigSetExpClk(ESP_UART_BASE, SYSTEM_CLOCK_HZ, ESP_UART_BAUD,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE);

    // 使能FIFO
    UARTFIFOEnable(ESP_UART_BASE);

    // 使能接收中断
    UARTIntEnable(ESP_UART_BASE, UART_INT_RX | UART_INT_RT);

    // 使能UART
    UARTEnable(ESP_UART_BASE);
}

/**
 * 发送AT指令
 */
static bool esp_send_cmd(const char *cmd, const char *expected, uint32_t timeout_ms) {
    // 发送指令
    const char *p = cmd;
    while (*p) {
        UARTCharPut(ESP_UART_BASE, *p++);
    }
    UARTCharPut(ESP_UART_BASE, '\r');
    UARTCharPut(ESP_UART_BASE, '\n');

    // 简单延时等待响应(实际应用中应使用定时器)
    SysCtlDelay(SYSTEM_CLOCK_HZ / 3 / 1000 * timeout_ms);

    // 检查响应(简化版,实际应解析rx_buffer)
    return true;
}

/**
 * 解析ESP响应
 */
static void esp_parse_response(const char *response) {
    // 检查连接状态
    if (strstr(response, "WIFI CONNECTED")) {
        esp_ctx.conn_state = CONN_CONNECTED;
        if (esp_ctx.config.conn_cb) {
            esp_ctx.config.conn_cb(true);
        }
    } else if (strstr(response, "WIFI DISCONNECT")) {
        esp_ctx.conn_state = CONN_DISCONNECTED;
        if (esp_ctx.config.conn_cb) {
            esp_ctx.config.conn_cb(false);
        }
    }

    // 检查数据接收 "+IPD,<len>:<data>"
    if (strstr(response, "+IPD,")) {
        // 简化处理:提取数据并回调
        // 实际实现需要完整解析IPD格式
        if (esp_ctx.config.data_cb) {
            // TODO: 完整解析数据
        }
    }
}

/**
 * 初始化ESP模块
 */
bool esp_init(const esp_config_t *config) {
    if (!config) {
        return false;
    }

    memset(&esp_ctx, 0, sizeof(esp_ctx));
    memcpy(&esp_ctx.config, config, sizeof(esp_config_t));
    esp_ctx.conn_state = CONN_DISCONNECTED;

    // 初始化UART
    esp_uart_init();

    // 等待模块启动
    SysCtlDelay(SYSTEM_CLOCK_HZ / 3);  // 1秒

    // 测试AT指令
    if (!esp_send_cmd("AT", "OK", 1000)) {
        return false;
    }

    // 设置模式
    if (config->device_type == DEVICE_TARGET) {
        // 靶端作为AP(热点)
        esp_send_cmd("AT+CWMODE=2", "OK", 2000);

        // 配置AP
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "AT+CWSAP=\"%s\",\"%s\",5,3",
                config->ssid, config->password);
        esp_send_cmd(cmd, "OK", 2000);

        // 启动TCP服务器
        esp_send_cmd("AT+CIPMUX=1", "OK", 1000);  // 多连接模式
        snprintf(cmd, sizeof(cmd), "AT+CIPSERVER=1,%d", config->server_port);
        esp_send_cmd(cmd, "OK", 1000);

    } else {
        // 枪端作为Station(客户端)
        esp_send_cmd("AT+CWMODE=1", "OK", 2000);

        // 连接到AP
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"",
                config->ssid, config->password);
        esp_send_cmd(cmd, "OK", 10000);

        // 连接到服务器
        snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d",
                config->server_ip, config->server_port);
        esp_send_cmd(cmd, "OK", 5000);
    }

    esp_ctx.initialized = true;
    esp_ctx.conn_state = CONN_CONNECTED;

    return true;
}

/**
 * 主循环处理
 */
void esp_process(void) {
    if (!esp_ctx.initialized) {
        return;
    }

    // 定期发送心跳(简化版)
    // 实际应使用定时器
}

/**
 * 发送数据
 */
bool esp_send_data(const uint8_t *data, uint16_t len) {
    if (!esp_ctx.initialized || esp_ctx.conn_state != CONN_CONNECTED) {
        return false;
    }

    // 发送数据长度
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d", len);

    const char *p = cmd;
    while (*p) {
        UARTCharPut(ESP_UART_BASE, *p++);
    }
    UARTCharPut(ESP_UART_BASE, '\r');
    UARTCharPut(ESP_UART_BASE, '\n');

    // 等待">"提示符
    SysCtlDelay(SYSTEM_CLOCK_HZ / 3 / 100);  // 10ms

    // 发送实际数据
    for (uint16_t i = 0; i < len; i++) {
        UARTCharPut(ESP_UART_BASE, data[i]);
    }

    return true;
}

/**
 * 获取连接状态
 */
conn_state_t esp_get_connection_state(void) {
    return esp_ctx.conn_state;
}

/**
 * 重置ESP模块
 */
void esp_reset(void) {
    esp_send_cmd("AT+RST", "OK", 3000);
    SysCtlDelay(SYSTEM_CLOCK_HZ / 3 * 2);  // 等待2秒
}

/**
 * 检查是否有数据可接收
 */
bool esp_data_available(void) {
    return UARTCharsAvail(ESP_UART_BASE);
}
