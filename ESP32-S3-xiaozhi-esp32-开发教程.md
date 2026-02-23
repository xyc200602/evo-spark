# ESP32-S3 xiaozhi-esp32 Development Tutorial
# ESP32-S3 xiaozhi-esp32 开发教程

---

## Table of Contents / 目录

- [Environment Setup / 环境准备](#environment-setup-环境准备)
- [ESP-IDF Installation / ESP-IDF 安装](#esp-idf-installation-esp-idf-安装)
- [Project Cloning / 项目克隆](#project-cloning-项目克隆)
- [Compilation Process / 编译流程](#compilation-process-编译流程)
- [Flashing to Device / 烧录到设备](#flashing-to-device-烧录到设备)
- [Common Issues / 常见问题解决](#common-issues-常见问题解决)
- [Next Development Directions / 下一步开发方向](#next-development-directions-下一步开发方向)

---

## Environment Setup / 环境准备

### Hardware / 硬件

- **ESP32-S3-N16R8** or compatible dev board / 兼容的开发板
  - CPU: Xtensa LX7 dual-core 240MHz / 双核 240MHz
  - PSRAM: 8MB (Octal) / 8MB（八位）
  - Flash: 16MB / 16MB
- **USB data cable** (supports data transfer) / USB 数据线（支持数据传输）
- **Optional**: Display, microphone, speaker (based on project requirements) / **可选**：显示屏、麦克风、扬声器（根据项目需求）

### Software / 软件

- **Windows 10/11**
- **VS Code** or **ESP-IDF Extension** (recommended) / **VS Code** 或 **ESP-IDF 插件**（推荐）
- **ESP-IDF v5.4** (DO NOT use v5.5.3, compatibility issues exist) / **ESP-IDF v5.4**（不要使用 v5.5.3，有兼容问题）

---

## ESP-IDF Installation / ESP-IDF 安装

### Step 1: Download Installer / 下载安装器

Visit official download page / 访问官方下载页面：
```
https://dl.espressif.com/dl/esp-idf/
```

Download / 下载：

```
ESP-IDF v5.4.3 - Offline Installer
Size: 1.53 GB / 大小：1.53 GB
System: Windows 10/11 / 系统：Windows 10/11
```

### Step 2: Run Installer / 运行安装器

1. Run downloaded `esp-idf-tools-setup-5.4.3.exe` / 运行下载的安装程序
2. Select installation components / 选择安装组件：
   - ✅ ESP32-S3 support / ESP32-S3 支持
   - ✅ Git (if not installed) / Git（如果没有）
   - ✅ Python (if not installed) / Python（如果没有）
   - ✅ VS Code Extension (recommended) / VS Code 扩展（推荐）
3. Wait for installation to complete (5-10 minutes) / 等待安装完成（5-10分钟）

### Step 3: Verify Installation / 验证安装

Open **ESP-IDF 5.4 CMD** / 打开 **ESP-IDF 5.4 CMD**：
- Press `Win` key, search for "ESP-IDF 5.4 CMD" and open / 按 `Win` 键，搜索 "ESP-IDF 5.4 CMD" 并打开
- Run / 运行：
  ```cmd
  idf.py --version
  ```
- Should see / 应该看到：
  ```
  ESP-IDF v5.4.3
  ```

---

## Project Cloning / 项目克隆

### Step 1: Create Working Directory / 创建工作目录

```bash
mkdir -p ~/Desktop/Evo-spark
cd ~/Desktop/Evo-spark
```

### Step 2: Clone Project / 克隆项目

```bash
git clone --branch v2.0.3 --depth 1 https://github.com/78/xiaozhi-esp32.git
```

**Note / 说明**：
- `--branch v2.0.3`: Clone specific stable version / 克隆特定稳定版本
- `--depth 1`: Clone only latest commit, reduce download time / 只克隆最新提交，减少下载时间

---

## Compilation Process / 编译流程

### Step 1: Open ESP-IDF 5.4 CMD / 打开 ESP-IDF 5.4 CMD

Press `Win` key, search for "ESP-IDF 5.4 CMD" and open / 按 `Win` 键，搜索 "ESP-IDF 5.4 CMD" 并打开

### Step 2: Navigate to Project Directory / 切换到项目目录

```cmd
cd C:\Users\xyc\Desktop\Evo-spark\xiaozhi-esp32
```

### Step 3: Set Target Chip / 设置目标芯片

```cmd
idf.py set-target esp32s3
```

**Expected Output / 预期输出**：
```
Set Target to: esp32s3, new sdkconfig will be created.
```

### Step 4: Compile Project / 编译项目

```cmd
idf.py build
```

**Expected Output / 预期输出**：
```
Executing "ninja build"...
[Progress bar]
Project build complete. To flash, run:
    idf.py flash
or
    idf.py -p (PORT) flash
```

**Compilation Time / 编译时间**：
- First build: 5-15 minutes / 首次编译：5-15 分钟
- Incremental build: 1-3 minutes / 增量编译：1-3 分钟

---

## Flashing to Device / 烧录到设备

### Step 1: Check Serial Port / 检查串口号

```cmd
mode
```

Look for similar output / 查找类似输出：
```
设备状态 COM4:
----------
    波特率:     9600 / Baud rate: 9600
```

**Note / 注意**:
- COM port number may change after unplugging/replugging USB (COM3, COM4, COM5...) / 端口号可能因插拔 USB 而变化（COM3、COM4、COM5...）

### Step 2: Flash Firmware / 烧录固件

```cmd
idf.py -p COM4 flash monitor
```

**Parameter Description / 参数说明**：
- `-p COM4`: Specify serial port (adjust according to actual situation) / 指定串口（根据实际情况调整）
- `flash`: Flash firmware / 烧录固件
- `monitor`: Automatically open serial monitor after flashing / 烧录后自动打开串口监视

**Expected Output / 预期输出**：
```
Writing at 0x00010000... (100%)
Wrote 2457856 bytes in 19.2 seconds
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
```

### Step 3: Enter Download Mode (if needed) / 进入下载模式（如果需要）

If flashing gets stuck, enter download mode / 如果烧录卡住，进入下载模式：
1. Hold **BOOT** button / 按住 **BOOT** 按钮
2. Press **RESET** button once / 按一下 **RESET** 按钮
3. Release both buttons / 松开两个按钮
4. Immediately execute flash command / 立即执行烧录命令

---

## Common Issues / 常见问题解决

### Issue 1: Component Dependency Error / 组件依赖错误

**Error Message / 错误信息**：
```
ERROR: espressif2022/image_player (==1.1.0~1) doesn't match any versions
```

**Cause / 原因**：
ESP-IDF version is incompatible with component library / ESP-IDF 版本与组件库不兼容。

**Solution / 解决方案**：

Comment out problematic dependency, edit file / 注释掉问题依赖，编辑文件：
```bash
main/idf_component.yml
```

Add `#` before line 38 / 在第 38 行前加 `#`：
```yaml
# espressif2022/image_player: ==1.1.0~1
```

Then rebuild / 然后重新编译。

---

### Issue 2: Chip Model Mismatch / 芯片型号不匹配

**Error Message / 错误信息**：
```
This chip is ESP32-S3, not ESP32. Wrong --chip argument?
```

**Cause / 原因**：
`CONFIG_IDF_TARGET` in `sdkconfig` is set incorrectly / `sdkconfig` 中的 `CONFIG_IDF_TARGET` 设置错误。

**Solution / 解决方案**：

Re-set target chip / 重新设置目标芯片：
```cmd
rd /s /q build
idf.py set-target esp32s3
idf.py build
```

Verify configuration / 验证配置：
```bash
grep "CONFIG_IDF_TARGET" sdkconfig
```

Should see / 应该看到：
```
CONFIG_IDF_TARGET="esp32s3"
CONFIG_IDF_TARGET_ESP32S3=y
```

---

### Issue 3: COM Port Busy / COM 端口被占用

**Error Message / 错误信息**：
```
Could not open COM4, port is busy or doesn't exist.
```

**Cause / 原因**：
Another program is using the serial port / 其他程序正在使用串口。

**Solution / 解决方案**：

1. **Close programs occupying the port** / **关闭占用端口的程序**：
   - VS Code Serial Monitor / VS Code 串口监视器
   - Arduino IDE
   - PuTTY/Tera Term

2. **Re-plug USB** / **重新插拔 USB**：
   - Unplug USB cable / 拔掉 USB 线
   - Wait 2 seconds / 等待 2 秒
   - Plug back in / 重新插上
   - Check for port changes / 检查端口变化

3. **Check available ports** / **检查可用端口**：
   ```cmd
   mode
   ```

---

### Issue 4: Assets File Missing / Assets 文件缺失

**Error Message / 错误信息**：
```
WARNING: File generated_assets.bin is empty
No such file or directory: 'generated_assets.bin'
```

**Cause / 原因**：
Asset files were not generated correctly / 资源文件没有正确生成。

**Solution A: Create Empty File (Temporary) / 创建空文件（临时方案）**:

```bash
cd build
python -c "open('generated_assets.bin', 'wb').write(b'')"
```

**Solution B: Modify Partition Table (Recommended) / 修改分区表（推荐）**：

```cmd
idf.py menuconfig
```

Navigate to / 导航到：
```
Partition Table → Partition table
→ Custom partition table → partitions/v1/8m.csv
```

Save and rebuild / 保存后重新编译。

---

### Issue 5: Device Stuck in Download Mode / 设备处于下载模式

**Log Message / 日志信息**：
```
boot:0x0 (DOWNLOAD(USB/UART0))
waiting for download
```

**Cause / 原因**：
BOOT button is held down, or flashing mode异常 / BOOT 按钮被按住，或烧录模式异常。

**Solution / 解决方案**：

1. **Press RESET button** to let device boot normally / **按一下 RESET 按钮**，让设备正常启动
2. **Check BOOT button** if stuck, press to release / 检查 BOOT 按钮是否卡住，按一下弹起
3. **Re-plug USB** if problem persists / 如果问题持续，重新插拔 USB

Normal boot should show / 正常启动应该看到：
```
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
```

---

## Device Boot and Configuration / 设备启动与配置

### Normal Boot Log / 正常启动日志

After successful flashing, serial monitor will show / 成功烧录后，串口监视器会显示：

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)

I (xxx) main: ESP32-S3 chip model: ESP32-S3
I (xxx) main: CPU frequency: 240 MHz
I (xxx) main: PSRAM: 8MB
I (xxx) main: Flash size: 16MB
I (xxx) main: Initializing WiFi...
```

### Configure WiFi / 配置 WiFi

Device will create WiFi hotspot after boot / 设备启动后会创建 WiFi 热点：

```
热点名称 / Hotspot Name: Xiaozhi-9EE1
密码 / Password: (None or other)
```

**Configuration Steps / 配置步骤**：

1. Connect phone to hotspot `Xiaozhi-9EE1` / 用手机连接热点 `Xiaozhi-9EE1`
2. Visit in browser / 用浏览器访问：
   ```
   http://192.168.4.1
   ```
3. On configuration page, set up / 在配置页面设置：
   - Your WiFi network name and password / 你的 WiFi 网络名称和密码
   - Server address (official or self-hosted) / 服务器地址（官方或自建）

### WiFi Connection Success Log / WiFi 连接成功日志

```
I (xxx) WifiStation: Got IP: 192.168.31.66
I (xxx) Display: SetStatus: 已连接 / Connected
I (xxx) Application: 激活设备 / Activating device
I (xxx) Display:      xiaozhi.me:847442
```

---

## Next Development Directions / 下一步开发方向

### Get Familiar with Original Version / 熟悉原版功能

Test existing features of xiaozhi-esp32 / 测试 xiaozhi-esp32 的现有功能：
- ✅ Voice wake-up (default: "你好小智") / 语音唤醒（默认："你好小智"）
- ✅ Conversation via xiaozhi.me platform / 通过 xiaozhi.me 平台对话
- ✅ Display expressions / 表情显示
- ✅ MCP tool calling (control GPIO, sensors, etc.) / MCP 工具调用（控制 GPIO、传感器等）
- ✅ OTA firmware updates / OTA 固件升级

Visit configuration page / 访问配置页面：
```
http://192.168.31.66
```

---

### Custom Modification Plans / 自定义修改计划

#### 1. ClaudeCode Integration / ClaudeCode 接入

Create new protocol class `ClaudeCodeProtocol` inheriting `Protocol` / 创建新协议类 `ClaudeCodeProtocol`，继承 `Protocol`：

```cpp
// main/protocols/claudecode_protocol.h
class ClaudeCodeProtocol : public Protocol {
public:
    ClaudeCodeProtocol();
    ~ClaudeCodeProtocol();

    bool Start() override;
    bool SendAudio(std::unique_ptr<AudioStreamPacket> packet) override;
    // ...
};
```

Implement WebSocket client to connect to local ClaudeCode service / 实现 WebSocket 客户端连接本地 ClaudeCode 服务。
Process voice input through ClaudeCode / 通过 ClaudeCode 处理语音输入.
Replace original Telegram interface / 替代原 Telegram 接口.

#### 2. Direct LLM API Calling / 直接 LLM API 调用

Create `DirectApiProtocol` / 新增 `DirectApiProtocol`：

```cpp
// main/protocols/direct_api_protocol.h
class DirectApiProtocol : public Protocol {
private:
    std::string api_key_;
    std::string api_url_;

public:
    void SetApiKey(const std::string& key) { api_key_ = key; }
    void SetModel(const std::string& model) { /* ... */ }
};
```

Supported APIs / 支持的 API：
- Claude API (Anthropic) / Claude API (Anthropic)
- GLM-4.7-flash (Zhipu AI) / GLM-4.7-flash (智谱 AI)
- OpenAI API / OpenAI API

Replace original server-based solution / 替代原基于服务器的方案.

#### 3. Local Memory System / 本地记忆系统

Create `memory/` directory / 创建 `memory/` 目录：

```
main/memory/
├── memory_manager.h       # Memory Manager / 记忆管理器
├── memory_manager.cc
├── file_storage.h          # File Storage Interface / 文件存储接口
└── file_storage.cc
```

Implement files / 实现文件：
- `SOUL.md` - Robot's "personality" (personality, settings) / 机器人"灵魂"（性格、设定）
- `USER.md` - User information (name, preferences) / 用户信息（姓名、偏好）
- `MEMORY.md` - Long-term conversation memory / 长期对话记忆

Use SPIFFS/LittleFS for storage / 使用 SPIFFS/LittleFS 存储：

```cpp
// Load memory
void load_memory(char *buffer, size_t size);

// Save memory
void save_memory(const char *data);

// Smart update
void update_memory(const char *conversation);
```

#### 4. Enhanced Agent Logic / 增强 Agent 逻辑

Create `ai/` directory / 新增 `ai/` 目录：

```
main/ai/
├── agent.h               # Agent Core Logic / Agent 核心逻辑
├── agent.cc
├── context.h             # Context Management / 对话上下文管理
├── context.cc
└── llm_api.h             # Unified LLM API Interface / LLM API 统一接口
```

Implement features / 实现功能：
- Context window management / 上下文窗口管理
- Conversation history tracking / 对话历史跟踪
- Smart decision making / 智能决策
- Memory retrieval and update / 记忆检索和更新

---

## Project Structure / 项目结构

```
Evo-spark/
├── xiaozhi-esp32/           # Original Version / 原版（已测试）
├── evospark-mod/             # Modified Version / 修改版（新分支）
│   ├── main/
│   │   ├── protocols/
│   │   │   ├── claudecode_protocol.h   # ClaudeCode Client / ClaudeCode 客户端
│   │   │   ├── direct_api_protocol.h     # Direct API Calling / 直接 API 调用
│   │   │   └── protocol.h              # Abstract Protocol Layer / 抽象协议层
│   ├── memory/                 # Memory System / 记忆系统
│   │   ├── memory_manager.h
│   │   ├── memory_manager.cc
│   │   ├── file_storage.h
│   │   └── file_storage.cc
│   ├── ai/                     # Agent System / Agent 系统
│   │   ├── agent.h
│   │   ├── agent.cc
│   │   ├── context.h
│   │   ├── context.cc
│   │   └── llm_api.h
│   └── config/
│       └── memory_files/
│           ├── SOUL.md
│           ├── USER.md
│           └── MEMORY.md
└── ESP32-S3-xiaozhi-esp32-开发教程.md   # This Tutorial / 本教程
```

---

## References / 参考资料

### Open Source Projects / 开源项目

- [xiaozhi-esp32 Official Repository](https://github.com/78/xiaozhi-esp32)
- [xiaozhi.me Official Platform](https://xiaozhi.me)
- [MimiClaw](https://github.com/memovai/mimiclaw) - AI Agent on ESP32 / ESP32 上的 AI Agent
- [OpenClaw](https://github.com/anthropics/openclaw) - Multi-channel AI Gateway / 多通道 AI 网关

### Documentation / 文档

- [ESP-IDF Official Documentation](https://docs.espressif.com/projects/esp-idf/)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html)

---

## Update Log / 更新日志

### v2.0 (2026-02-23) / v2.0 (2026-02-23)

- Initial bilingual version / 初始双语版本
- Complete environment setup, compilation, and flashing process / 完整的环境准备、编译、烧录流程
- Common issues and solutions documented / 常见问题和解决方案记录
- Next development directions outlined / 下一步开发方向说明

---

## Author / 作者

**xyc200602**
**Project**: Evo-spark - Desktop Companion Robot Based on xiaozhi-esp32

---

## License / 许可证

MIT License

---

> **"Intelligence is not trained. It is learned."**
> **"智能不是训练出来的。是学出来的。"**
