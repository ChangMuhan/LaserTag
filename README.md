# LaserTag - 激光打靶系统

**嵌入式课程项目** | TM4C123GH6PM LaunchPad | 双端通信与显示系统

---

## 📋 项目概述

这是一个基于TI TM4C123GH6PM的双人激光打靶游戏系统,包含:
- **枪端(Gun A)**: 激光发射、弹量管理、扳机检测、音效反馈
- **靶端(Target B)**: 光敏检测、计分管理、命中提示
- **双端通信**: ESP-01S Wi-Fi无线通信,实时状态同步
- **显示系统**: ST7735彩屏显示弹量和分数

## 🎯 功能特性

### 枪端
- ✅ 扳机检测与消抖
- ✅ 激光发射控制
- ✅ 弹量管理 (初始10发)
- ✅ 枪声音效播放
- ✅ 弹量实时显示
- ⏳ 震动反馈(可选)

### 靶端
- ✅ 光敏传感器检测
- ✅ 命中去重机制
- ✅ 分数累计
- ✅ 命中音效提示
- ✅ 分数实时显示

### 通信系统
- ✅ Wi-Fi双端通信 (靶端AP, 枪端STA)
- ✅ 自定义二进制协议
- ✅ 校验和验证
- ✅ 超时重试机制
- ✅ 连接状态监控

### 显示系统
- ✅ ST7735 128x160彩屏
- ✅ 枪端弹量条显示
- ✅ 靶端分数大字显示
- ✅ 连接状态指示
- ✅ 射击结果反馈

## 🏗️ 项目结构

```
LaserTag/
│
├── common/                    # 双端共用模块
│   ├── board_pins.h           # 引脚分配
│   ├── game_config.h          # 游戏参数配置
│   ├── protocol.h/c           # 通信协议
│   ├── esp_at.h/c             # ESP-01S Wi-Fi驱动
│   ├── lcd.h/c                # ST7735显示驱动
│   ├── button.h/c             # 按键与消抖
│   ├── timer.h/c              # 定时器管理
│   └── dfplayer.h/c           # DFPlayer音频播放
│
├── gun_A/                     # 枪端业务代码 (负责人: A)
│   ├── main.c                 # 枪端主程序
│   ├── gun_app.h/c            # 枪端状态机
│   ├── ammo.h/c               # 弹量管理
│   ├── vibration.h/c          # 震动马达(可选)
│   └── gun_ui.h/c             # 枪端界面逻辑
│
├── target_B/                  # 靶端业务代码 (负责人: B)
│   ├── main.c                 # 靶端主程序
│   ├── target_app.h/c         # 靶端状态机
│   ├── hit_detect.h/c         # ADC光敏检测
│   ├── score.h/c              # 计分管理
│   └── target_ui.h/c          # 靶端界面逻辑
│
├── docs/                      # 文档
│   ├── INTERFACE.md           # 接口文档
│   ├── PROTOCOL.md            # 通信协议详解
│   ├── wiring_gun.png         # 枪端接线图
│   └── wiring_target.png      # 靶端接线图
│
└── README.md                  # 本文件
```

## 🔌 硬件清单

### 开发板与核心模块
- [x] TM4C123GH6PM LaunchPad × 2
- [x] ESP-01S Wi-Fi模块 × 2
- [x] ST7735 LCD (128x160) × 2

### 枪端器件
- [x] 激光二极管模块 (红光650nm)
- [x] 按键开关 (扳机)
- [x] DFPlayer Mini音频模块
- [x] 扬声器 8Ω 0.5W
- [ ] 震动马达 (可选)
- [x] 4节AA电池盒

### 靶端器件
- [x] 光敏电阻/光敏二极管
- [x] DFPlayer Mini音频模块
- [x] 扬声器 8Ω 0.5W
- [x] 4节AA电池盒

### 辅助器件
- [x] 降压模块 (5V/3.3V)
- [x] 杜邦线若干
- [x] 面包板/洞洞板

## 📌 引脚分配

### 枪端 (Gun A)
| 功能 | 引脚 | 备注 |
|-----|------|------|
| 激光发射 | PF1 | GPIO输出 |
| 扳机按键 | PF4 | 内部上拉 |
| 震动马达 | PF3 | PWM(可选) |
| LCD_SCK | PA2 | SSI0 |
| LCD_MOSI | PA5 | SSI0 |
| LCD_DC | PA6 | GPIO |
| LCD_CS | PA3 | GPIO |
| LCD_RST | PA7 | GPIO |
| ESP_TX | PD7 | UART2 |
| ESP_RX | PD6 | UART2 |
| AUDIO_TX | PB1 | UART1 |
| AUDIO_RX | PB0 | UART1 |

### 靶端 (Target B)
| 功能 | 引脚 | 备注 |
|-----|------|------|
| 光敏传感器 | PE3 | ADC0/AIN0 |
| LCD_SCK | PA2 | SSI0 |
| LCD_MOSI | PA5 | SSI0 |
| LCD_DC | PA6 | GPIO |
| LCD_CS | PA3 | GPIO |
| LCD_RST | PA7 | GPIO |
| ESP_TX | PD7 | UART2 |
| ESP_RX | PD6 | UART2 |
| AUDIO_TX | PB1 | UART1 |
| AUDIO_RX | PB0 | UART1 |

详细接线图见 `docs/wiring_*.png`

## 🚀 快速开始

### 1. 环境准备

**所需工具**:
- Code Composer Studio (CCS) 或 Keil MDK
- TivaWare库 (官方SDK)
- LM Flash Programmer (烧录工具)

**克隆仓库**:
```bash
git clone https://github.com/ChangMuhan/LaserTag.git
cd LaserTag
```

### 2. 编译与烧录

#### 使用Code Composer Studio
```bash
# 1. 导入项目
File → Import → CCS Projects → Select LaserTag/gun_A

# 2. 编译
Project → Build Project

# 3. 烧录
Run → Debug (自动下载)
```

#### 使用Makefile (推荐)
```bash
# 编译枪端
cd gun_A
make clean && make

# 编译靶端
cd ../target_B
make clean && make

# 烧录
make flash
```

### 3. 启动顺序

1. **靶端先上电** - 建立Wi-Fi热点
2. **等待5秒** - 确保AP稳定
3. **枪端上电** - 自动连接靶端
4. **观察屏幕** - 两端显示"CONN"即连接成功

### 4. 游戏流程

```
1. 初始状态: 枪端10发弹药, 靶端0分
2. 瞄准靶子,扣下扳机
3. 激光发射,枪声响起,弹量-1
4. 命中: 靶端+1分,播放命中音,枪端显示"HIT!"
5. 未命中: 靶端无反应,枪端显示"MISS"
6. 弹药耗尽: 枪端提示"NO AMMO",需按换弹键
7. 新局: 按RESET键重置,弹药和分数归零
```

## 📡 通信协议

### 网络配置
```
SSID:       LaserTag_AP
密码:       12345678
靶端IP:     192.168.4.1:8080 (AP模式)
枪端IP:     自动分配 (STA模式)
```

### 消息类型
| 消息 | 方向 | 描述 |
|-----|------|------|
| ARM | 枪→靶 | 射击准备,包含局号和射击编号 |
| READY | 靶→枪 | 靶端准备就绪,可以发射 |
| RESULT | 靶→枪 | 射击结果(命中/未命中/分数) |
| RESET | 双向 | 新局重置 |
| STATE | 双向 | 状态同步(重连后) |

### 时序示例
```
枪端                    靶端
 │                       │
 ├──ARM(sid=1,shot=1)──►│ 打开检测窗口
 │                       │
 │◄─READY(sid=1,shot=1)─┤
 │                       │
 ├──发射激光──┐         │
 ├──播枪声────┤         │ 检测命中
 ├──扣弹量────┘         │ 更新分数
 │                       │
 │◄─RESULT(hit=1,sc=1)──┤
 │                       │
 └──显示"HIT!"          │
```

完整协议说明见 `docs/PROTOCOL.md`

## 🧪 测试与调试

### 单端测试 (无通信)

#### 枪端
```c
// 测试激光和弹量
void test_gun_standalone() {
    while (1) {
        if (button_pressed()) {
            laser_on();
            delay_ms(100);
            laser_off();
            ammo_decrement();
            lcd_update_ammo();
        }
    }
}
```

#### 靶端
```c
// 测试检测和计分
void test_target_standalone() {
    while (1) {
        if (light_detected()) {
            score_increment();
            lcd_update_score();
            play_hit_sound();
        }
    }
}
```

### 通信测试

使用串口日志监控:
```bash
# 枪端
[INIT] System initialized
[WIFI] Connecting to LaserTag_AP...
[WIFI] Connected, IP: 192.168.4.2
[FIRE] Trigger pressed, sending ARM
[RECV] Got READY, firing laser
[RECV] Got RESULT: HIT, score=1

# 靶端
[INIT] System initialized
[WIFI] AP started: LaserTag_AP
[RECV] Got ARM sid=1 shot=1
[SEND] Sending READY
[HIT] Detected, score: 0 → 1
[SEND] Sending RESULT
```

## 🐛 常见问题

### 枪端连不上Wi-Fi
- ✅ 确保靶端先启动
- ✅ 检查SSID/密码配置
- ✅ 用手机搜索"LaserTag_AP"验证

### 发射无反应
- ✅ 检查弹量是否耗尽
- ✅ 检查是否已连接靶端
- ✅ 查看串口日志确认消息收发

### 靶端不计分
- ✅ 检查光敏传感器阈值
- ✅ 用手电筒测试检测功能
- ✅ 检查ARM消息是否收到

### 屏幕显示异常
- ✅ 检查SPI接线
- ✅ 降低SSI时钟频率
- ✅ 检查电源电压是否稳定

## 📚 开发指南

### 模块分工

| 模块 | 负责人 | 文件 | 状态 |
|-----|--------|------|------|
| 枪端业务 | A | `gun_A/*` | 进行中 |
| 靶端业务 | B | `target_B/*` | 进行中 |
| 通信显示 | C | `common/*` | ✅ 已完成 |

### 集成步骤

#### A (枪端负责人)
1. 实现 `gun_app.c` 状态机
2. 实现 `ammo.c` 弹量管理
3. 调用 `esp_send_data()` 发送ARM
4. 处理 `on_data_received()` 回调
5. 调用 `lcd_gun_*()` 更新显示

#### B (靶端负责人)
1. 实现 `hit_detect.c` ADC采样
2. 实现 `score.c` 计分逻辑
3. 处理 `on_data_received()` 回调
4. 调用 `esp_send_data()` 发送READY/RESULT
5. 调用 `lcd_target_*()` 更新显示

#### C (通信显示负责人)
- ✅ 提供 `common/` 下所有模块
- ✅ 编写接口文档
- ✅ 提供测试程序
- ⏳ 协助A/B集成调试

详细接口说明见 `docs/INTERFACE.md`

## 📊 性能指标

| 指标 | 数值 | 备注 |
|-----|------|------|
| 通信延迟 | <100ms | ARM→READY往返 |
| 检测响应 | <50ms | 激光脉冲→命中判定 |
| 屏幕刷新 | 10Hz | 足够流畅 |
| 最大射程 | ~5m | 取决于激光功率 |
| 电池续航 | 2-3小时 | 4节AA电池 |
| 准确率 | >95% | 固定距离和光照 |

## 🛠️ 进阶功能 (可选)

- [ ] 多靶点检测 (多个光敏传感器)
- [ ] 环数识别 (不同区域不同分值)
- [ ] 震动反馈
- [ ] 语音提示
- [ ] 低电量警告
- [ ] 历史记录保存
- [ ] 蓝牙通信替代Wi-Fi
- [ ] OLED屏幕替代TFT

## 🤝 贡献者

- **A** - 枪端负责人
- **B** - 靶端负责人
- **C** - 通信显示负责人

## 📄 许可证

本项目仅用于课程学习,未经授权请勿用于商业用途。

## 🔗 参考资料

- [TM4C123GH6PM数据手册](https://www.ti.com/product/TM4C123GH6PM)
- [TivaWare SDK](https://www.ti.com/tool/SW-TM4C)
- [ST7735驱动库](https://github.com/adafruit/Adafruit-ST7735-Library)
- [ESP-01S AT指令集](https://www.espressif.com/sites/default/files/documentation/4a-esp8266_at_instruction_set_cn.pdf)
- [DFPlayer Mini手册](https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299)

---

**最后更新**: 2026-11-22  
**项目状态**: 开发中 (通信显示模块已完成)  
**GitHub**: https://github.com/ChangMuhan/LaserTag
