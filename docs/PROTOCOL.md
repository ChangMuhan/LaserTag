# 通信协议详解

**版本**: v1.0  
**最后更新**: 2026-11-22

---

## 一、协议设计原则

1. **简洁高效** - 二进制格式,减少传输开销
2. **可靠传输** - 校验和验证,检测传输错误
3. **状态关联** - (session_id, shot_id) 唯一标识射击
4. **去重机制** - 防止重复计分
5. **超时保护** - 避免无限等待

---

## 二、消息格式

### 2.1 通用消息头

```c
typedef struct {
    uint8_t start_marker;   // 固定0xAA
    uint8_t msg_type;       // 消息类型
    uint8_t length;         // 负载长度(不含header)
    uint8_t checksum;       // 校验和
} msg_header_t;
```

**字段说明**:
- `start_marker`: 固定为`0xAA`,用于帧同步
- `msg_type`: 见消息类型表
- `length`: 负载字节数(不包含header的4字节)
- `checksum`: 累加校验和的反码

### 2.2 校验和算法

```c
// 计算校验和
uint8_t calc_checksum(uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    // 累加除checksum字段外的所有字节
    sum += data[0];  // start_marker
    sum += data[1];  // msg_type
    sum += data[2];  // length
    for (int i = 4; i < len; i++) {
        sum += data[i];  // payload
    }
    return ~sum;  // 取反
}

// 验证校验和
bool verify_checksum(uint8_t *data, uint8_t len) {
    uint8_t expected = calc_checksum(data, len);
    return (data[3] == expected);
}
```

---

## 三、消息类型定义

### 3.1 ARM - 射击准备

**方向**: 枪端 → 靶端  
**触发**: 扳机按下后,发射激光前

```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;  // 局号
    uint16_t shot_id;     // 射击编号
} msg_arm_t;  // 总长度: 8字节
```

**示例数据包**:
```
AA 00 04 F7 01 00 01 00
│  │  │  │  └─────┴──── shot_id = 1
│  │  │  │  └────────── session_id = 1
│  │  │  └──────────── checksum
│  │  └──────────────── length = 4
│  └──────────────────── msg_type = ARM (0)
└──────────────────────── start_marker
```

**语义**:
- `session_id`: 每次新局递增,用于区分不同游戏
- `shot_id`: 每次射击递增,局内唯一

**超时**: 2秒未收到READY,取消本次射击

---

### 3.2 READY - 准备就绪

**方向**: 靶端 → 枪端  
**触发**: 收到ARM后,检测窗口打开

```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;
    uint16_t shot_id;
} msg_ready_t;  // 总长度: 8字节
```

**示例数据包**:
```
AA 01 04 F6 01 00 01 00
```

**语义**:
- 告知枪端可以发射
- session_id和shot_id必须与ARM匹配

**靶端行为**:
1. 收到ARM
2. 打开ADC检测窗口(200ms)
3. 发送READY
4. 等待激光照射

---

### 3.3 RESULT - 射击结果

**方向**: 靶端 → 枪端  
**触发**: 检测窗口关闭后

```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;
    uint16_t shot_id;
    uint8_t hit;          // 1=命中, 0=未命中
    uint16_t score;       // 当前总分
} msg_result_t;  // 总长度: 11字节
```

**示例数据包** (命中,分数=5):
```
AA 02 07 F1 01 00 01 00 01 05 00
│  │  │  │  │     │     │  └──── score = 5
│  │  │  │  │     │     └──────── hit = 1
│  │  │  │  │     └──────────── shot_id = 1
│  │  │  │  └──────────────── session_id = 1
│  │  │  └──────────────────── checksum
│  │  └──────────────────────── length = 7
│  └──────────────────────────── msg_type = RESULT (2)
└──────────────────────────────── start_marker
```

**hit字段**:
- `0`: 未命中,分数不变
- `1`: 命中,分数+1

**超时**: 3秒未收到RESULT,标记为"结果未知"

---

### 3.4 RESET - 新局重置

**方向**: 双向  
**触发**: 任一端按下重置按键

```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;  // 新的局号
} msg_reset_t;  // 总长度: 6字节
```

**示例数据包** (新局号=2):
```
AA 03 02 F9 02 00
```

**行为**:
- 枪端: 弹量恢复初始值
- 靶端: 分数清零
- 双端: session_id更新,shot_id重置为0

---

### 3.5 STATE - 状态同步

**方向**: 双向  
**触发**: 重连后同步状态

```c
typedef struct {
    msg_header_t header;
    uint16_t session_id;
    uint16_t score;
    uint8_t ammo;
    uint8_t conn_status;  // 连接状态
} msg_state_t;  // 总长度: 10字节
```

**示例数据包**:
```
AA 04 06 F3 01 00 05 00 08 01
│  │  │  │  │     │     │  └─── conn_status = 1 (已连接)
│  │  │  │  │     │     └─────── ammo = 8
│  │  │  │  │     └─────────── score = 5
│  │  │  │  └─────────────── session_id = 1
│  │  │  └─────────────────── checksum
│  │  └─────────────────────── length = 6
│  └─────────────────────────── msg_type = STATE (4)
└─────────────────────────────── start_marker
```

**用途**:
- 断线重连后恢复游戏状态
- 避免分数和弹量不一致

---

## 四、完整通信时序

### 4.1 正常流程

```
时间    枪端                                靶端
0ms     扣下扳机
5ms     构建ARM消息
10ms    ──────ARM(sid=1,shot=1)──────►    收到ARM
15ms                                        打开检测窗口
20ms                                        发送READY
25ms    ◄─────READY(sid=1,shot=1)─────
30ms    发射激光(100ms脉冲)
35ms    播放枪声
40ms    扣减弹量(10→9)
45ms    更新屏幕显示
        ...                                 等待检测窗口关闭
130ms                                       激光命中检测
135ms                                       分数+1 (0→1)
140ms                                       播放命中音
145ms                                       发送RESULT
150ms   ◄───RESULT(hit=1,score=1)─────
155ms   显示"HIT!"
160ms   [流程结束]
```

**关键时间点**:
- ARM→READY: ~15ms (Wi-Fi延迟)
- 激光脉冲: 100ms (可配置)
- 检测窗口: 200ms (覆盖网络延迟+脉冲)
- READY→RESULT: ~130ms

### 4.2 超时流程

#### READY超时
```
枪端                                靶端
 │
 ├────ARM(sid=1,shot=1)──────►    [靶端掉线]
 │
 ├──等待READY...
 │   (2000ms超时)
 │
 ├──超时!
 ├──不发射激光
 ├──不扣减弹量
 └──显示"TIMEOUT"
```

#### RESULT超时
```
枪端                                靶端
 │
 ├────ARM(sid=1,shot=1)──────►
 │
 │◄────READY(sid=1,shot=1)────
 │
 ├──发射激光
 ├──扣减弹量(10→9)              [靶端网络中断]
 │
 ├──等待RESULT...
 │   (3000ms超时)
 │
 ├──超时!
 ├──弹量不恢复(已扣减)
 └──显示"UNKNOWN"
```

### 4.3 重复消息处理

**场景**: 枪端未收到READY,重发ARM

```
枪端                                靶端
 │
 ├────ARM(sid=1,shot=1)──────►
 │                                  发送READY(丢包)
 ├──超时,重发
 │
 ├────ARM(sid=1,shot=1)──────►    检测到重复shot_id
 │                                  从缓存取出结果
 │◄────RESULT(cached)─────────
 │
 └──收到结果,不重复扣弹
```

**去重规则**:
1. 靶端缓存最近10次 `(session_id, shot_id)` → RESULT
2. 收到重复ARM,直接返回缓存结果
3. 不重新开窗,不重新计分

---

## 五、状态机设计

### 5.1 枪端状态机

```
        [IDLE]
           │
           │ 扣扳机
           ↓
        [ARMING] ──────超时(2s)────→ [TIMEOUT] ─→ [IDLE]
           │
           │ 收到READY
           ↓
        [FIRING]
           │
           │ 发射完成
           ↓
      [WAITING_RESULT] ──超时(3s)──→ [UNKNOWN] ─→ [IDLE]
           │
           │ 收到RESULT
           ↓
        [HIT/MISS]
           │
           │ 显示完成
           ↓
        [IDLE]
```

### 5.2 靶端状态机

```
        [IDLE]
           │
           │ 收到ARM
           ↓
      [OPENING_WINDOW]
           │
           │ 发送READY
           ↓
       [DETECTING]
           │
           ├──200ms内检测到──→ [HIT] ───→ 发送RESULT(hit=1)
           │                      │
           └──200ms超时───────→ [MISS] ──→ 发送RESULT(hit=0)
                                  │
                                  ↓
                                [IDLE]
```

---

## 六、错误处理

### 6.1 校验和错误

```c
void handle_checksum_error(uint8_t *data, uint8_t len) {
    // 丢弃该消息
    // 不回复,等待发送方超时重传
    log("Checksum error, dropping packet");
}
```

### 6.2 消息类型未知

```c
void handle_unknown_message(uint8_t msg_type) {
    // 忽略未知消息
    log("Unknown message type: %d", msg_type);
}
```

### 6.3 会话ID不匹配

```c
void handle_session_mismatch(uint16_t recv_sid, uint16_t local_sid) {
    // 可能对方已重置,同步状态
    send_state_message();
}
```

---

## 七、性能优化

### 7.1 减少往返次数

**当前**: ARM → READY → 发射 → RESULT (2次往返)

**优化**: 
- 方案A: 省略READY,枪端直接发射后等结果 (不推荐,靶端可能未准备好)
- 方案B: ARM携带发射时间戳,靶端提前开窗 (需时钟同步)

**结论**: 当前方案最稳妥

### 7.2 批量消息

连续射击时,可合并多个RESULT:

```c
typedef struct {
    msg_header_t header;
    uint8_t count;  // 结果数量
    struct {
        uint16_t shot_id;
        uint8_t hit;
    } results[5];
    uint16_t score;  // 最终分数
} msg_batch_result_t;
```

### 7.3 心跳保活

防止长时间空闲时TCP连接断开:

```c
// 每30秒发送一次心跳
if (time_since_last_tx() > 30000) {
    send_heartbeat();
}
```

---

## 八、安全考虑

### 8.1 消息注入

**风险**: 恶意设备伪造RESULT消息

**防护**:
1. Wi-Fi使用WPA2加密
2. 可选: 消息签名(HMAC)

### 8.2 重放攻击

**风险**: 重放旧的RESULT消息

**防护**:
1. session_id每局递增
2. shot_id严格递增
3. 忽略旧的session_id

### 8.3 拒绝服务

**风险**: 大量无效消息占用带宽

**防护**:
1. 限制消息速率(>100msg/s丢弃)
2. 检测重复消息并忽略

---

## 九、测试用例

### 9.1 正常场景

| 用例 | 步骤 | 预期结果 |
|-----|------|---------|
| 正常命中 | ARM→READY→发射→照射→RESULT | hit=1, 分数+1 |
| 正常未命中 | ARM→READY→发射→未照射→RESULT | hit=0, 分数不变 |
| 连续射击 | 10次射击,全部命中 | 分数=10 |

### 9.2 异常场景

| 用例 | 步骤 | 预期结果 |
|-----|------|---------|
| READY超时 | ARM→无READY | 2秒后取消,不扣弹 |
| RESULT超时 | ARM→READY→发射→无RESULT | 3秒后显示未知,已扣弹 |
| 重复ARM | ARM→READY丢包→重发ARM | 返回缓存结果 |
| 校验和错误 | 发送错误checksum | 靶端丢弃,枪端超时 |
| 会话ID错误 | 枪sid=1, 靶sid=2 | 发送STATE同步 |

### 9.3 压力测试

| 用例 | 描述 | 通过标准 |
|-----|------|---------|
| 快速射击 | 最快速度连续扣扳机 | 不丢消息,不重复计分 |
| 长时间运行 | 连续运行1小时 | 无内存泄漏,无死锁 |
| 断线重连 | 靶端重启后枪端重连 | 状态正确恢复 |

---

## 十、协议版本演进

### v1.0 (当前)
- 基础消息类型
- 校验和验证
- 超时重试

### v1.1 (计划)
- [ ] 心跳保活
- [ ] 批量结果
- [ ] 消息签名

### v2.0 (未来)
- [ ] 多枪多靶
- [ ] 观众模式
- [ ] 统计数据上报

---

**作者**: C (通信显示负责人)  
**审阅**: A, B  
**版本**: v1.0  
**日期**: 2026-11-22
