# LaserTag 项目快速上手

## 📋 你需要做什么

作为 **C (通信显示负责人)**,你的工作已经完成! ✅

现在需要进行以下步骤:

---

## 1️⃣ 推送到GitHub (网络恢复后)

当前网络连接有问题,等网络恢复后运行:

```bash
cd /Users/panjiaying/Documents/Codex/2026-11-07/2/work/LaserTag
./push_to_github.sh
```

或手动推送:

```bash
git remote add origin https://github.com/ChangMuhan/LaserTag.git
git branch -M main
git push -u origin main
```

---

## 2️⃣ 在Windows上同步代码

### 方法1: 从GitHub克隆
```bash
# 在Windows的Git Bash中
cd C:/Users/你的用户名/Documents
git clone https://github.com/ChangMuhan/LaserTag.git
```

### 方法2: U盘拷贝
```bash
# 在Mac上
cp -r LaserTag /Volumes/你的U盘/

# 在Windows上
# 从U盘复制到本地文件夹
```

---

## 3️⃣ 提供给队友的文件

### 📦 交付清单

**必需文件** (已完成):
- ✅ `common/` - 所有公共模块源码
- ✅ `docs/INTERFACE.md` - 完整接口文档
- ✅ `docs/PROTOCOL.md` - 通信协议详解
- ✅ `docs/WINDOWS_GUIDE.md` - Windows开发指南
- ✅ `README.md` - 项目说明

**发给A (枪端负责人)**:
```
给A看这些文档:
1. docs/INTERFACE.md → 第6.1节 "枪端集成"
2. common/esp_at.h → ESP通信接口
3. common/lcd.h → 显示接口 (lcd_gun_*函数)
4. common/protocol.h → 消息定义

A需要实现:
- gun_A/gun_app.c - 枪端状态机
- gun_A/ammo.c - 弹量管理
- 调用esp_send_data()发送ARM消息
- 调用lcd_gun_update_ammo()更新显示
```

**发给B (靶端负责人)**:
```
给B看这些文档:
1. docs/INTERFACE.md → 第6.2节 "靶端集成"
2. common/esp_at.h → ESP通信接口
3. common/lcd.h → 显示接口 (lcd_target_*函数)
4. common/protocol.h → 消息定义

B需要实现:
- target_B/hit_detect.c - ADC光敏检测
- target_B/score.c - 计分管理
- 处理on_data_received()回调中的ARM消息
- 调用esp_send_data()发送READY和RESULT
- 调用lcd_target_update_score()更新显示
```

---

## 4️⃣ 测试计划

### 阶段1: 单端测试 (各自独立)
- [ ] A测试: 按键→扣弹→显示
- [ ] B测试: 光敏→计分→显示
- [ ] C测试: 双屏显示 (已完成)

### 阶段2: 通信测试 (AB与C联调)
- [ ] 枪端发送ARM,靶端收到
- [ ] 靶端发送READY,枪端收到
- [ ] 靶端发送RESULT,枪端显示

### 阶段3: 完整流程 (三人联调)
- [ ] 扣扳机→命中→加分
- [ ] 连续10次射击
- [ ] 掉线重连
- [ ] 新局重置

---

## 5️⃣ 在Windows上开发的准备

**需要安装的工具**:
1. **Code Composer Studio** (CCS)
   - 下载: https://www.ti.com/tool/CCSTUDIO
   - 选择 "Tiva C Series" 支持包

2. **Git for Windows**
   - 下载: https://git-scm.com/download/win

3. **串口工具** (任选其一)
   - PuTTY: https://www.putty.org/
   - TeraTerm: https://ttssh2.osdn.jp/

**详细步骤**: 见 `docs/WINDOWS_GUIDE.md`

---

## 6️⃣ 目前的GitHub状态

### 已完成的提交:
```bash
git log --oneline
bd27af8 Initial commit: 通信显示模块完成
```

### 已推送的文件:
```
LaserTag/
├── README.md             ✅ 项目说明
├── .gitignore            ✅ Git忽略规则
├── push_to_github.sh     ✅ 推送脚本
├── common/               ✅ 公共模块 (13个文件)
│   ├── board_pins.h
│   ├── game_config.h
│   ├── protocol.h/c
│   ├── esp_at.h/c
│   ├── lcd.h/c
│   ├── button.h/c
│   └── timer.h/c
└── docs/                 ✅ 文档 (3个文件)
    ├── INTERFACE.md      - 接口文档
    ├── PROTOCOL.md       - 协议详解
    └── WINDOWS_GUIDE.md  - Windows指南
```

---

## 7️⃣ 当前待办事项

### 立即执行:
- [ ] 等网络恢复后推送到GitHub
- [ ] 通知A和B: 公共模块已完成
- [ ] 发送 `docs/INTERFACE.md` 给队友

### 后续工作:
- [ ] 协助A集成通信模块 (解答接口问题)
- [ ] 协助B集成显示模块 (解答接口问题)
- [ ] 提供模拟测试程序 (让AB无网络也能测试)
- [ ] 三人联调时调试通信问题

### 可选增强:
- [ ] 实现DFPlayer音频驱动 (dfplayer.h/c)
- [ ] 完善LCD字库 (8x16点阵)
- [ ] 增加心跳保活机制
- [ ] 实现掉线自动重连

---

## 8️⃣ 如何联系队友

### 发送消息模板:

**给A和B**:
```
嗨! 通信显示模块已经完成,代码已提交到本地Git。

📦 你们需要的文件:
- common/ 文件夹 (公共模块)
- docs/INTERFACE.md (接口文档)

🔗 GitHub仓库 (待推送):
https://github.com/ChangMuhan/LaserTag

📚 如何使用:
1. 看 docs/INTERFACE.md 第6节 (集成步骤)
2. 参考示例代码调用公共模块
3. 有问题随时联系我!

⏰ 下一步:
大家各自完成单端测试后,我们约时间联调通信。
```

---

## 9️⃣ 预期时间线

| 日期 | 任务 | 负责人 | 状态 |
|-----|------|--------|------|
| Day 1-2 | 公共模块开发 | C | ✅ 完成 |
| Day 3-5 | 枪端业务开发 | A | 进行中 |
| Day 3-5 | 靶端业务开发 | B | 进行中 |
| Day 6 | 单端测试 | A, B, C | 待开始 |
| Day 7 | 通信联调 | A, B, C | 待开始 |
| Day 8 | 硬件装配 | A, B | 待开始 |
| Day 9 | 完整测试 | A, B, C | 待开始 |
| Day 10 | 文档整理 | A, B, C | 待开始 |

---

## 🔟 常见问题

### Q1: 如果队友不会用Git怎么办?
**A**: 
1. 直接发送 `common/` 文件夹压缩包
2. 或者用U盘拷贝
3. 教他们只需要会 `git clone` 和 `git pull` 就行

### Q2: 如果队友看不懂接口文档?
**A**: 
1. 给他们看 `docs/INTERFACE.md` 第6节的示例代码
2. 提供完整的 `main.c` 模板 (见文档)
3. 远程协助,共享屏幕讲解

### Q3: 如何测试通信模块?
**A**: 
我可以写一个测试程序:
- 枪端模拟: 定时发送ARM
- 靶端模拟: 收到ARM后回复READY和RESULT
- 用串口打印日志验证

### Q4: 如果ESP模块不工作?
**A**: 
1. 检查接线: TX↔RX交叉连接
2. 检查波特率: 115200 (可能需要AT指令调整)
3. 用串口助手发AT指令测试模块

---

## 📧 联系方式

**C (通信显示负责人)**  
- GitHub: ChangMuhan (待队友创建仓库)
- 邮箱: (填写你的邮箱)

**项目地址**: https://github.com/ChangMuhan/LaserTag

---

**最后更新**: 2026-11-22  
**当前状态**: ✅ C的工作已完成,等待队友集成  
**下一步**: 推送到GitHub并通知队友
