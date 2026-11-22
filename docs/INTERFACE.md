# 激光打靶系统 - 接口文档

**项目**: LaserTag  
**版本**: v1.0  
**负责人**: C (通信显示负责人)  
**日期**: 2026-11-22

---

## 一、系统架构

```
┌─────────────────┐          Wi-Fi          ┌─────────────────┐
│   枪端 (Gun A)   │ ◄──────────────────────► │  靶端 (Target B) │
│                 │                          │                 │
│  - 激光发射      │                          │  - 光敏检测      │
│  - 弹量管理      │                          │  - 计分管理      │
│  - 扳机输入      │                          │  - 命中音效      │
│  - 枪声/震动     │                          │  - 分数显示      │
│  - 弹量显示      │                          │                 │
└─────────────────┘                          └─────────────────┘
```

## 二、Wi-Fi 网络配置

### 2.1 网络拓扑
- **靶端**: AP模式(热点) - 192.168.4.1
- **枪端**: Station模式(客户端) - 自动分配IP

### 2.2 连接参数
```c
SSID:       LaserTag_AP
密码:       12345678
服务器IP:   192.168.4.1
服务器端口: 8080
```

### 2.3 初始化顺序
1. 靶端先启动,建立AP热点
2. 枪端启动,连接到靶端AP
3. 双端完成TCP连接建立
4. 开始消息通信

---

## 三、通信协议

### 3.1 消息格式

所有消息使用固定二进制格式:

```
┌──────────────┬──────────┬────────┬──────────┬───────────────┐
│ start_marker │ msg_type │ length │ checksum │   payload     │
│   (0xAA)     │  (1 B)   │ (1 B)  │  (1 B)   │  (变长)       │
└──────────────┴──────────┴────────┴──────────┴───────────────┘
```

**校验和算法**: 累加所有字节(除checksum字段外)后取反
```c
checksum = ~(start_marker + msg_type + length + payload_bytes)
```

### 3.2 消息类型

| 类型 | 值 | 发送方 | 描述 |
|-----|---|--------|------|
| MSG_ARM | 0 | 枪端 | 射击准备 |
| MSG_READY | 1 | 靶端 | 准备就绪 |
| MSG_RESULT | 2 | 靶端 | 射击结果 |
| MSG_RESET | 3 | 任一端 | 新局重置 |
| MSG_STATE | 4 | 任一端 | 状态同步 |
| MSG_HEARTBEAT | 5 | 任一端 | 心跳保活 |

### 3.3 消息定义

#### (1) ARM - 射击准备
```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;   // 局号
    uint16_t shot_id;      // 射击编号
} msg_arm_t;
```
**时机**: 枪端扣下扳机后,在发射激光前发送

#### (2) READY - 准备就绪
```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;
    uint16_t shot_id;
} msg_ready_t;
```
**时机**: 靶端收到ARM后,打开检测窗口并回复

#### (3) RESULT - 射击结果
```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;
    uint16_t shot_id;
    uint8_t hit;          // 1=命中, 0=未命中
    uint16_t score;       // 当前总分
} msg_result_t;
```
**时机**: 靶端检测窗口关闭后,发送结果

#### (4) RESET - 新局重置
```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;   // 新的局号
} msg_reset_t;
```
**时机**: 任一端按下重置按键

#### (5) STATE - 状态同步
```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;
    uint16_t score;
    uint8_t ammo;
    uint8_t conn_status;
} msg_state_t;
```
**时机**: 重连后同步状态

---

## 四、通信时序

### 4.1 正常射击流程

```
枪端                              靶端
  │                                │
  │  (1) 扣下扳机                  │
  │──────ARM(sid, shotid)────────►│
  │                                │  (2) 打开检测窗口
  │                                │
  │◄─────READY(sid, shotid)───────│
  │                                │
  │  (3) 发射激光                  │
  │  (4) 播放枪声                  │
  │  (5) 扣减弹量                  │
  │                                │  (6) 检测命中
  │                                │  (7) 更新分数
  │                                │  (8) 播放命中音
  │                                │
  │◄───RESULT(hit, score)─────────│
  │                                │
  │  (9) 显示结果                  │
  │                                │
```

### 4.2 超时处理

**READY超时** (2秒):
- 枪端取消本次射击
- 不扣减弹量
- 提示"网络超时"

**RESULT超时** (3秒):
- 枪端标记结果未知
- 已扣减的弹量不恢复
- 提示"结果未知"

### 4.3 去重机制

**关键**: 使用 `(session_id, shot_id)` 二元组标识唯一射击

- 靶端缓存最近10次射击结果
- 收到重复ARM时,直接返回缓存的RESULT
- 不重复开窗,不重复计分

---

## 五、公共模块接口

### 5.1 通信模块 (esp_at.c)

#### 初始化
```c
esp_config_t config = {
    .device_type = DEVICE_GUN,  // 或 DEVICE_TARGET
    .ssid = WIFI_SSID,
    .password = WIFI_PASSWORD,
    .server_ip = WIFI_SERVER_IP,
    .server_port = WIFI_SERVER_PORT,
    .conn_cb = on_connection_changed,
    .data_cb = on_data_received
};
esp_init(&config);
```

#### 发送消息
```c
msg_arm_t msg;
protocol_build_arm(&msg, session_id, shot_id);
esp_send_data((uint8_t*)&msg, sizeof(msg));
```

#### 主循环
```c
while (1) {
    esp_process();  // 处理Wi-Fi事件
    // ...
}
```

### 5.2 显示模块 (lcd.c)

#### 枪端接口
```c
// 初始化
lcd_init(DEVICE_GUN);

// 更新弹量
lcd_gun_update_ammo(current_ammo, INITIAL_AMMO);

// 更新连接状态
lcd_gun_update_connection(is_connected);

// 显示射击结果
lcd_gun_show_result("HIT!", COLOR_GREEN);
lcd_gun_show_result("MISS", COLOR_GRAY);
lcd_gun_show_result("TIMEOUT", COLOR_RED);
```

#### 靶端接口
```c
// 初始化
lcd_init(DEVICE_TARGET);

// 更新分数
lcd_target_update_score(current_score);

// 更新连接状态
lcd_target_update_connection(is_connected);

// 命中动画
lcd_target_show_hit_animation();
```

### 5.3 协议模块 (protocol.c)

#### 构建消息
```c
msg_arm_t arm_msg;
protocol_build_arm(&arm_msg, session_id, shot_id);

msg_result_t result_msg;
protocol_build_result(&result_msg, session_id, shot_id, true, 15);
```

#### 解析消息
```c
uint8_t rx_buffer[64];
msg_arm_t received_msg;

int msg_type = protocol_parse_message(rx_buffer, len, &received_msg);
if (msg_type == MSG_ARM) {
    // 处理ARM消息
}
```

---

## 六、集成步骤 (A和B参考)

### 6.1 枪端集成 (A的工作)

```c
// main.c
#include "common/lcd.h"
#include "common/esp_at.h"
#include "common/protocol.h"
#include "common/timer.h"
#include "common/button.h"

void on_connection_changed(bool connected) {
    lcd_gun_update_connection(connected);
}

void on_data_received(const uint8_t *data, uint16_t len) {
    msg_result_t result;
    if (protocol_parse_message(data, len, &result) == MSG_RESULT) {
        // A的业务逻辑: 处理射击结果
        gun_handle_result(&result);
    }
}

int main(void) {
    // 1. 初始化公共模块
    timer_init();
    lcd_init(DEVICE_GUN);
    button_init(GUN_TRIGGER_PORT, GUN_TRIGGER_PIN, TRIGGER_DEBOUNCE_MS);
    
    esp_config_t esp_cfg = {
        .device_type = DEVICE_GUN,
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
        .server_ip = WIFI_SERVER_IP,
        .server_port = WIFI_SERVER_PORT,
        .conn_cb = on_connection_changed,
        .data_cb = on_data_received
    };
    esp_init(&esp_cfg);
    
    // 2. 初始化A的业务模块
    gun_app_init();
    ammo_init(INITIAL_AMMO);
    
    // 3. 主循环
    while (1) {
        button_scan();
        esp_process();
        gun_app_process();  // A的状态机
    }
}
```

**A需要调用的显示接口**:
- `lcd_gun_update_ammo()` - 每次弹量变化
- `lcd_gun_show_result()` - 收到RESULT后

**A需要调用的通信接口**:
- `esp_send_data()` - 发送ARM消息
- `esp_get_connection_state()` - 检查连接

### 6.2 靶端集成 (B的工作)

```c
// main.c
#include "common/lcd.h"
#include "common/esp_at.h"
#include "common/protocol.h"
#include "common/timer.h"

void on_data_received(const uint8_t *data, uint16_t len) {
    msg_arm_t arm;
    if (protocol_parse_message(data, len, &arm) == MSG_ARM) {
        // B的业务逻辑: 打开检测窗口
        target_handle_arm(&arm);
    }
}

int main(void) {
    // 1. 初始化公共模块
    timer_init();
    lcd_init(DEVICE_TARGET);
    
    esp_config_t esp_cfg = {
        .device_type = DEVICE_TARGET,
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
        .server_port = WIFI_SERVER_PORT,
        .data_cb = on_data_received
    };
    esp_init(&esp_cfg);
    
    // 2. 初始化B的业务模块
    target_app_init();
    hit_detect_init();
    score_init();
    
    // 3. 主循环
    while (1) {
        esp_process();
        hit_detect_scan();   // B的ADC采样
        target_app_process(); // B的状态机
    }
}
```

**B需要调用的显示接口**:
- `lcd_target_update_score()` - 每次分数变化
- `lcd_target_show_hit_animation()` - 检测到命中

**B需要调用的通信接口**:
- `esp_send_data()` - 发送READY和RESULT

---

## 七、测试与调试

### 7.1 单端测试 (无网络)

#### 枪端测试
```c
// 模拟READY消息
void test_gun_without_network(void) {
    // 1. 测试显示
    lcd_gun_update_ammo(10, 10);
    lcd_gun_update_connection(false);
    
    // 2. 模拟扳机
    if (button_is_falling_edge()) {
        ammo_decrement();
        lcd_gun_update_ammo(ammo_get(), INITIAL_AMMO);
        lcd_gun_show_result("TEST OK", COLOR_GREEN);
    }
}
```

#### 靶端测试
```c
// 模拟ARM消息
void test_target_without_network(void) {
    // 1. 测试显示
    lcd_target_update_score(0);
    lcd_target_update_connection(false);
    
    // 2. 模拟命中
    if (hit_detected()) {
        score_increment();
        lcd_target_update_score(score_get());
        lcd_target_show_hit_animation();
    }
}
```

### 7.2 通信测试

#### 串口日志格式
```
[枪端]
[INIT] ESP module initialized
[CONN] Connecting to LaserTag_AP...
[CONN] Connected, IP: 192.168.4.2
[SEND] ARM sid=1 shotid=1
[RECV] READY sid=1 shotid=1
[FIRE] Laser fired
[RECV] RESULT sid=1 shotid=1 hit=1 score=1

[靶端]
[INIT] ESP module initialized
[AP] AP started: LaserTag_AP
[SERVER] TCP server listening on :8080
[RECV] ARM sid=1 shotid=1
[SEND] READY sid=1 shotid=1
[HIT] Detected hit, score: 0 -> 1
[SEND] RESULT sid=1 shotid=1 hit=1 score=1
```

### 7.3 已知问题与解决方案

| 问题 | 可能原因 | 解决方法 |
|-----|---------|---------|
| 枪端连不上AP | 靶端未先启动 | 确保靶端先上电 |
| 发送失败 | Wi-Fi断连 | 检查连接状态后再发送 |
| 重复计分 | 未缓存结果 | 实现去重逻辑 |
| 屏幕乱码 | SPI时钟过快 | 降低SSI频率 |
| 消息校验失败 | 字节序问题 | 检查uint16_t拆分顺序 |

---

## 八、交付清单

### C (通信显示负责人) 交付内容

**代码文件**:
- ✅ `common/game_config.h` - 游戏参数配置
- ✅ `common/board_pins.h` - 引脚分配
- ✅ `common/protocol.h/c` - 通信协议
- ✅ `common/esp_at.h/c` - Wi-Fi驱动
- ✅ `common/lcd.h/c` - 显示驱动
- ✅ `common/timer.h/c` - 定时器
- ✅ `common/button.h/c` - 按键输入
- ✅ `common/dfplayer.h/c` - 音频驱动(TODO)

**文档**:
- ✅ 本接口文档
- ✅ 通信协议说明
- ✅ 集成指南

**测试程序**:
- ✅ 双屏测试 (`test/lcd_test.c`)
- ✅ 通信测试 (`test/comm_test.c`)
- ⏳ 完整演示 (待AB集成)

---

## 九、下一步工作

### 9.1 待完成项
- [ ] DFPlayer音频驱动实现
- [ ] 完善LCD字库
- [ ] 增加心跳保活机制
- [ ] 实现掉线自动重连

### 9.2 协作事项
- [ ] 与A对接: ARM/RESULT消息触发时机
- [ ] 与B对接: READY消息和检测窗口时序
- [ ] 三人联调: 完整射击流程测试

---

**最后更新**: 2026-11-22  
**联系人**: C (通信显示负责人)
