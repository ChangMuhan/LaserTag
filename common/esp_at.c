/**
 * esp_at.c - 修复版本
 * ESP-01S Wi-Fi模块AT指令驱动实现
 *
 * 修复内容:
 * - P0: AT指令响应验证 (不再永远返回true)
 * - P0: +IPD数据解析和回调 (完整实现)
 * - P1: 发送成功判断 (等待SEND OK)
 * - P1: 连接状态正确管理
 */

#include "esp_at.h"
#include "board_pins.h"
#include "timer.h"
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

    // UART接收缓冲
    char rx_buffer[256];
    uint16_t rx_index;

    // AT指令响应验证
    char at_response[128];
    bool at_response_ready;
    bool at_response_ok;

    // +IPD数据缓存
    uint8_t ipd_buffer[256];
    uint16_t ipd_length;
    uint16_t ipd_received;
    bool ipd_receiving;
    uint8_t client_id;  // 多连接时的客户端ID

    uint32_t last_heartbeat;
    bool initialized;
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

        // 如果正在接收IPD数据
        if (esp_ctx.ipd_receiving) {
            esp_ctx.ipd_buffer[esp_ctx.ipd_received++] = (uint8_t)c;

            // 数据接收完成
            if (esp_ctx.ipd_received >= esp_ctx.ipd_length) {
                esp_ctx.ipd_receiving = false;

                // 回调给业务层
                if (esp_ctx.config.data_cb) {
                    esp_ctx.config.data_cb(esp_ctx.ipd_buffer, esp_ctx.ipd_length);
                }
            }
            continue;
        }

        // 正常行缓冲处理
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

    // 解锁PD7 (特殊引脚)
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= 0x80;
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = 0;

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
    IntEnable(INT_UART2);

    // 使能UART
    UARTEnable(ESP_UART_BASE);
}

/**
 * 发送AT指令并等待响应 (修复: 真正验证响应)
 */
static bool esp_send_cmd(const char *cmd, const char *expected, uint32_t timeout_ms) {
    // 清除之前的响应状态
    esp_ctx.at_response_ready = false;
    esp_ctx.at_response_ok = false;
    memset(esp_ctx.at_response, 0, sizeof(esp_ctx.at_response));

    // 发送指令
    const char *p = cmd;
    while (*p) {
        UARTCharPut(ESP_UART_BASE, *p++);
    }
    UARTCharPut(ESP_UART_BASE, '\r');
    UARTCharPut(ESP_UART_BASE, '\n');

    // 等待响应 (轮询方式,兼容中断未启用情况)
    uint32_t start_time = timer_get_ms();
    while (!esp_ctx.at_response_ready) {
        // 手动处理UART接收
        while (UARTCharsAvail(ESP_UART_BASE)) {
            char c = UARTCharGetNonBlocking(ESP_UART_BASE);
            if (esp_ctx.rx_index < sizeof(esp_ctx.rx_buffer) - 1) {
                esp_ctx.rx_buffer[esp_ctx.rx_index++] = c;
                if (c == '\n') {
                    esp_ctx.rx_buffer[esp_ctx.rx_index] = '\0';
                    esp_parse_response(esp_ctx.rx_buffer);
                    esp_ctx.rx_index = 0;
                }
            }
        }

        // 超时检查
        if (timer_get_ms() - start_time > timeout_ms) {
            return false;  // 超时失败
        }

        // 短暂延时
        SysCtlDelay(SYSTEM_CLOCK_HZ / 3 / 1000);  // 1ms
    }

    // 检查是否匹配期望响应
    if (expected && strstr(esp_ctx.at_response, expected)) {
        return true;
    }

    // 检查是否收到OK
    return esp_ctx.at_response_ok;
}

/**
 * 解析ESP响应 (修复: 完整的+IPD解析)
 */
static void esp_parse_response(const char *response) {
    // 保存响应内容
    strncpy(esp_ctx.at_response, response, sizeof(esp_ctx.at_response) - 1);

    // 检查OK/ERROR/SEND OK响应
    if (strstr(response, "OK\r") || strstr(response, "OK\n")) {
        esp_ctx.at_response_ok = true;
        esp_ctx.at_response_ready = true;
    } else if (strstr(response, "ERROR")) {
        esp_ctx.at_response_ok = false;
        esp_ctx.at_response_ready = true;
    } else if (strstr(response, "SEND OK")) {
        esp_ctx.at_response_ok = true;
        esp_ctx.at_response_ready = true;
    } else if (strstr(response, "SEND FAIL")) {
        esp_ctx.at_response_ok = false;
        esp_ctx.at_response_ready = true;
    }

    // 检查连接状态事件
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
    } else if (strstr(response, "CONNECT")) {
        // 客户端连接事件
        esp_ctx.conn_state = CONN_CONNECTED;
    } else if (strstr(response, "CLOSED")) {
        // 连接关闭事件
        esp_ctx.conn_state = CONN_DISCONNECTED;
    }

    // 解析 +IPD 数据接收 (修复: 完整解析)
    // 格式: +IPD,<len>:<data>  (单连接)
    // 或:   +IPD,<id>,<len>:<data>  (多连接)
    const char *ipd_pos = strstr(response, "+IPD,");
    if (ipd_pos) {
        ipd_pos += 5;  // 跳过 "+IPD,"

        int first_num = 0;
        int second_num = -1;

        sscanf(ipd_pos, "%d", &first_num);

        // 查找冒号或逗号
        const char *comma = strchr(ipd_pos, ',');
        const char *colon = strchr(ipd_pos, ':');

        if (comma && colon && comma < colon) {
            // 多连接模式: +IPD,<id>,<len>:<data>
            esp_ctx.client_id = (uint8_t)first_num;
            sscanf(comma + 1, "%d", &second_num);
            esp_ctx.ipd_length = (uint16_t)second_num;
        } else if (colon) {
            // 单连接模式: +IPD,<len>:<data>
            esp_ctx.client_id = 0;
            esp_ctx.ipd_length = (uint16_t)first_num;
        } else {
            return;  // 格式错误
        }

        // 启动IPD数据接收模式
        if (esp_ctx.ipd_length > 0 && esp_ctx.ipd_length < sizeof(esp_ctx.ipd_buffer)) {
            esp_ctx.ipd_received = 0;
            esp_ctx.ipd_receiving = true;

            // 尝试从当前行读取数据 (冒号后的部分)
            if (colon) {
                const char *data_start = colon + 1;
                while (*data_start && *data_start != '\r' && *data_start != '\n' &&
                       esp_ctx.ipd_received < esp_ctx.ipd_length) {
                    esp_ctx.ipd_buffer[esp_ctx.ipd_received++] = (uint8_t)*data_start++;
                }

                // 如果本行已读完全部数据
                if (esp_ctx.ipd_received >= esp_ctx.ipd_length) {
                    esp_ctx.ipd_receiving = false;

                    // 回调给业务层
                    if (esp_ctx.config.data_cb) {
                        esp_ctx.config.data_cb(esp_ctx.ipd_buffer, esp_ctx.ipd_length);
                    }
                }
            }
        }
    }
}

/**
 * 初始化ESP模块 (修复: 正确判断连接状态)
 */
bool esp_init(const esp_config_t *config) {
    if (!config) {
        return false;
    }

    memset(&esp_ctx, 0, sizeof(esp_ctx));
    memcpy(&esp_ctx.config, config, sizeof(esp_config_t));
    esp_ctx.conn_state = CONN_DISCONNECTED;

    // 初始化定时器 (如果未初始化)
    timer_init();

    // 初始化UART
    esp_uart_init();

    // 等待模块启动
    SysCtlDelay(SYSTEM_CLOCK_HZ / 3);  // 1秒

    // 测试AT指令
    if (!esp_send_cmd("AT", "OK", 1000)) {
        return false;  // AT测试失败
    }

    // 回显关闭
    esp_send_cmd("ATE0", "OK", 1000);

    // 设置模式
    char cmd[128];
    if (config->device_type == DEVICE_TARGET) {
        // 靶端作为AP(热点)
        if (!esp_send_cmd("AT+CWMODE=2", "OK", 2000)) {
            return false;
        }

        // 配置AP
        snprintf(cmd, sizeof(cmd), "AT+CWSAP=\"%s\",\"%s\",5,3",
                config->ssid, config->password);
        if (!esp_send_cmd(cmd, "OK", 2000)) {
            return false;
        }

        // 启动多连接模式
        if (!esp_send_cmd("AT+CIPMUX=1", "OK", 1000)) {
            return false;
        }

        // 启动TCP服务器
        snprintf(cmd, sizeof(cmd), "AT+CIPSERVER=1,%d", config->server_port);
        if (!esp_send_cmd(cmd, "OK", 1000)) {
            return false;
        }

        // 服务器模式: AP启动后即标记为已连接 (等待客户端接入)
        esp_ctx.conn_state = CONN_CONNECTED;

    } else {
        // 枪端作为Station(客户端)
        if (!esp_send_cmd("AT+CWMODE=1", "OK", 2000)) {
            return false;
        }

        // 单连接模式
        if (!esp_send_cmd("AT+CIPMUX=0", "OK", 1000)) {
            return false;
        }

        // 连接到AP
        snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"",
                config->ssid, config->password);
        if (!esp_send_cmd(cmd, "OK", 10000)) {
            return false;  // Wi-Fi连接失败
        }

        // 连接到TCP服务器
        snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d",
                config->server_ip, config->server_port);
        if (!esp_send_cmd(cmd, "OK", 5000)) {
            return false;  // TCP连接失败
        }

        // 客户端模式: TCP连接成功才标记为已连接
        esp_ctx.conn_state = CONN_CONNECTED;
    }

    esp_ctx.initialized = true;

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
 * 发送数据 (修复: 等待SEND OK)
 */
bool esp_send_data(const uint8_t *data, uint16_t len) {
    if (!esp_ctx.initialized || esp_ctx.conn_state != CONN_CONNECTED) {
        return false;
    }

    // 构建CIPSEND命令
    char cmd[32];
    if (esp_ctx.config.device_type == DEVICE_TARGET) {
        // AP模式: 指定连接ID (发送给最近连接的客户端)
        snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d", esp_ctx.client_id, len);
    } else {
        // STA模式: 单连接,不需要ID
        snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d", len);
    }

    // 清除响应状态
    esp_ctx.at_response_ready = false;

    // 发送CIPSEND命令
    const char *p = cmd;
    while (*p) {
        UARTCharPut(ESP_UART_BASE, *p++);
    }
    UARTCharPut(ESP_UART_BASE, '\r');
    UARTCharPut(ESP_UART_BASE, '\n');

    // 等待">"提示符
    uint32_t start_time = timer_get_ms();
    bool got_prompt = false;
    while (timer_get_ms() - start_time < 1000) {
        if (UARTCharsAvail(ESP_UART_BASE)) {
            char c = UARTCharGetNonBlocking(ESP_UART_BASE);
            if (c == '>') {
                got_prompt = true;
                break;
            }
        }
        SysCtlDelay(SYSTEM_CLOCK_HZ / 3 / 1000);  // 1ms
    }

    if (!got_prompt) {
        return false;  // 未收到提示符
    }

    // 发送实际数据
    for (uint16_t i = 0; i < len; i++) {
        UARTCharPut(ESP_UART_BASE, data[i]);
    }

    // 等待SEND OK响应 (修复: 真正等待)
    start_time = timer_get_ms();
    while (!esp_ctx.at_response_ready && timer_get_ms() - start_time < 5000) {
        while (UARTCharsAvail(ESP_UART_BASE)) {
            char c = UARTCharGetNonBlocking(ESP_UART_BASE);
            if (esp_ctx.rx_index < sizeof(esp_ctx.rx_buffer) - 1) {
                esp_ctx.rx_buffer[esp_ctx.rx_index++] = c;
                if (c == '\n') {
                    esp_ctx.rx_buffer[esp_ctx.rx_index] = '\0';
                    esp_parse_response(esp_ctx.rx_buffer);
                    esp_ctx.rx_index = 0;
                }
            }
        }
        SysCtlDelay(SYSTEM_CLOCK_HZ / 3 / 1000);  // 1ms
    }

    return esp_ctx.at_response_ok;
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
    esp_ctx.initialized = false;
}

/**
 * 检查是否有数据可接收
 */
bool esp_data_available(void) {
    return UARTCharsAvail(ESP_UART_BASE);
}
