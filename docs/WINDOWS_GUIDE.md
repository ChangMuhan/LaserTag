# Windows 开发指南

由于你提到需要在 Windows 上进行测试和开发,这里提供完整的跨平台开发说明。

---

## 一、Windows 开发环境配置

### 1.1 必需工具

#### Code Composer Studio (推荐)
```
下载地址: https://www.ti.com/tool/CCSTUDIO
版本: 最新版 (支持TM4C123)
大小: ~1.5GB

安装步骤:
1. 下载安装包
2. 选择 "Tiva C Series" 支持包
3. 安装驱动 (自动)
4. 重启电脑
```

#### Keil MDK (备选)
```
下载地址: https://www.keil.com/download/
版本: MDK-ARM
许可: 学生免费许可

安装步骤:
1. 下载MDK-ARM
2. 安装 Pack Installer
3. 搜索并安装 "Tiva TM4C123" Pack
```

#### Git for Windows
```
下载地址: https://git-scm.com/download/win
勾选: Git Bash, Git GUI

配置:
git config --global user.name "你的名字"
git config --global user.email "your_email@example.com"
```

### 1.2 驱动安装

#### TM4C123 USB驱动
```
位置: CCS安装目录\ccs\drivers\
文件: stellaris_icdi_drivers.exe

安装后设备管理器应显示:
- Stellaris ICDI (COM端口)
- Stellaris Virtual Serial Port
```

### 1.3 串口调试工具

#### PuTTY (推荐)
```
下载: https://www.putty.org/
配置:
- Serial line: COM3 (根据实际调整)
- Speed: 115200
- Connection type: Serial
```

#### TeraTerm
```
下载: https://ttssh2.osdn.jp/
配置类似PuTTY
```

---

## 二、从 Mac 到 Windows 迁移步骤

### 2.1 克隆仓库

**方法1: HTTPS** (推荐新手)
```bash
# 在Windows上打开Git Bash
cd C:/Users/你的用户名/Documents
git clone https://github.com/ChangMuhan/LaserTag.git
cd LaserTag
```

**方法2: SSH** (需配置密钥)
```bash
# 生成SSH密钥
ssh-keygen -t ed25519 -C "your_email@example.com"

# 复制公钥到GitHub
cat ~/.ssh/id_ed25519.pub

# 克隆
git clone git@github.com:ChangMuhan/LaserTag.git
```

### 2.2 导入到CCS

```
File → Import...
→ Code Composer Studio → CCS Projects
→ Select search-directory: C:\Users\...\LaserTag
→ 勾选 gun_A 和 target_B
→ Finish
```

### 2.3 配置TivaWare路径

```
项目右键 → Properties
→ Build → Variables
→ 添加变量:
   TIVAWARE_ROOT = C:\ti\TivaWare_C_Series-2.2.0.295

→ Build → Arm Compiler → Include Options
→ 添加路径:
   ${TIVAWARE_ROOT}
```

---

## 三、文件路径差异处理

### 3.1 头文件包含

**Mac/Linux**:
```c
#include "driverlib/gpio.h"    // 使用正斜杠
```

**Windows**: 同样支持正斜杠,无需修改

### 3.2 换行符问题

**配置Git自动转换**:
```bash
# Windows上设置
git config --global core.autocrlf true

# Mac上设置
git config --global core.autocrlf input
```

### 3.3 Makefile (可选)

如果使用Makefile,需要安装MinGW:
```bash
# 使用Chocolatey安装
choco install mingw

# 或下载: http://www.mingw.org/
```

---

## 四、CCS项目配置

### 4.1 创建枪端工程

```
File → New → CCS Project
项目名: gun_A
Target: TM4C123GH6PM
编译器: TI v20.2.x (最新版)
模板: Empty Project (with main.c)

项目结构:
gun_A/
├── main.c
├── gun_app.c/h
├── ammo.c/h
└── ...
```

### 4.2 添加common模块

**方法1: 链接方式** (推荐)
```
项目右键 → Properties → Build → Arm Compiler → Include Options
添加: "${workspace_loc}/common"

源文件:
项目右键 → Add Files → Link to files
选择 common/*.c
```

**方法2: 复制方式**
```
直接复制common文件夹到gun_A下
```

### 4.3 编译配置

**优化级别**:
```
Debug:   -O0 (无优化, 便于调试)
Release: -O2 (标准优化)
```

**链接脚本**:
```
使用 TM4C123GH6PM.cmd (自动生成)
或自定义 linker.ld
```

---

## 五、调试技巧

### 5.1 CCS 调试器

**启动调试**:
```
Run → Debug (F11)
```

**常用操作**:
- F8: 继续运行
- F5: 单步进入
- F6: 单步跳过
- F7: 单步退出
- Ctrl+Shift+B: 设置断点

**查看变量**:
```
Expressions 窗口: 输入变量名
Memory Browser: 查看内存
Registers: 查看寄存器
```

### 5.2 串口输出调试

**添加printf支持**:

```c
// uart_printf.c
#include "driverlib/uart.h"
#include <stdarg.h>
#include <stdio.h>

void uart_printf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    for (int i = 0; buf[i]; i++) {
        UARTCharPut(UART0_BASE, buf[i]);
    }
}

// main.c
uart_printf("Debug: ammo=%d, score=%d\n", ammo, score);
```

**查看输出**:
```
PuTTY连接 COM3, 115200-8-N-1
```

### 5.3 逻辑分析仪

**使用CCS内置分析器**:
```
Tools → Graph → Single Time
选择变量: ammo, score
采样率: 10Hz
```

---

## 六、常见问题 (Windows专属)

### 6.1 COM端口找不到

**解决**:
```
1. 设备管理器 → 端口 → 查看COM编号
2. 如果显示黄色感叹号:
   - 右键 → 更新驱动程序
   - 浏览计算机 → 从列表选择
   - 选择 "Stellaris ICDI"
```

### 6.2 下载失败 "Error connecting to target"

**解决**:
```
1. 检查USB线连接
2. 按板子上的Reset按钮
3. CCS: Run → Debug Configurations
   → Target Configuration → 选择正确的.ccxml
4. 重启CCS
```

### 6.3 编译错误 "cannot find driverlib"

**解决**:
```
1. 确认TivaWare已安装
2. 项目属性 → Include Paths → 添加:
   C:\ti\TivaWare_C_Series-2.2.0.295
   C:\ti\TivaWare_C_Series-2.2.0.295\driverlib
```

### 6.4 中文乱码

**解决**:
```
Window → Preferences → General → Workspace
→ Text file encoding: UTF-8
```

---

## 七、Mac ↔ Windows 协作流程

### 7.1 Mac 上开发

```bash
# 1. 编辑代码
vim common/lcd.c

# 2. 提交到本地
git add common/lcd.c
git commit -m "更新LCD驱动"

# 3. 推送到GitHub
git push origin main
```

### 7.2 Windows 上同步

```bash
# 1. 拉取最新代码
git pull origin main

# 2. 在CCS中刷新项目
项目右键 → Refresh

# 3. 重新编译
Project → Build Project
```

### 7.3 冲突解决

```bash
# 如果出现冲突
git status  # 查看冲突文件

# 编辑冲突文件,手动合并
<<<<<<< HEAD
// 你的改动
=======
// 别人的改动
>>>>>>> branch

# 标记为已解决
git add <file>
git commit -m "解决冲突"
git push
```

---

## 八、性能对比

| 操作 | Mac (CCS) | Windows (CCS) | Windows (Keil) |
|-----|-----------|---------------|----------------|
| 编译时间 | ~5秒 | ~5秒 | ~3秒 |
| 下载速度 | 正常 | 正常 | 稍快 |
| 调试稳定性 | 高 | 高 | 中 |
| 串口工具 | screen | PuTTY | TeraTerm |

**推荐**: Windows + CCS (与Mac环境一致)

---

## 九、自动化脚本 (Windows)

### build_all.bat
```batch
@echo off
echo 编译枪端...
cd gun_A
make clean
make
if %errorlevel% neq 0 goto error

echo 编译靶端...
cd ..\target_B
make clean
make
if %errorlevel% neq 0 goto error

echo ✅ 编译成功!
goto end

:error
echo ❌ 编译失败!
exit /b 1

:end
pause
```

### flash.bat
```batch
@echo off
set FLASH_TOOL="C:\ti\ccs\ccs_base\DebugServer\bin\DSLite.exe"
set CCXML="C:\ti\ccs\ccs_base\common\targetdb\connections\TIVA_TM4C123GH6PM.ccxml"

echo 烧录枪端...
%FLASH_TOOL% load -c %CCXML% -f gun_A\Debug\gun_A.out

echo 烧录靶端...
%FLASH_TOOL% load -c %CCXML% -f target_B\Debug\target_B.out

echo ✅ 烧录完成!
pause
```

---

## 十、推荐工作流程

### 日常开发
1. **Mac**: 编写公共模块 (common/)
2. **Windows**: 测试烧录到硬件
3. **Mac**: 编写文档和Git管理
4. **Windows**: 最终验证和演示

### 协作开发
1. A在Windows上开发gun_A
2. B在Windows上开发target_B
3. C在Mac上维护common + 文档
4. 每天同步: `git pull` → 测试 → `git push`

---

**提示**: Windows上的路径使用反斜杠 `\`,但在代码中仍使用正斜杠 `/`

**最后更新**: 2026-11-22
