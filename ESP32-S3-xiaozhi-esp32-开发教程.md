# ESP32-S3 xiaozhi-esp32 开发教程

## 目录

- [环境准备](#环境准备)
- [ESP-IDF 安装](#esp-idf-安装)
- [项目克隆](#项目克隆)
- [编译流程](#编译流程)
- [烧录到设备](#烧录到设备)
- [常见问题解决](#常见问题解决)
- [下一步开发方向](#下一步开发方向)

---

## 环境准备

### 硬件

- **ESP32-S3-N16R8** 或兼容的开发板
  - CPU: Xtensa LX7 双核 240MHz
  - PSRAM: 8MB (Octal)
  - Flash: 16MB
- **USB 数据线**（支持数据传输）
- **可选**: 显示屏、麦克风、扬声器（根据项目需求）

### 软件

- **Windows 10/11**
- **VS Code** 或 **ESP-IDF 插件**（推荐）
- **ESP-IDF v5.4**（不要使用 v5.5.3，有兼容问题）

---

## ESP-IDF 安装

### 步骤 1: 下载安装器

访问官方下载页面：
```
https://dl.espressif.com/dl/esp-idf/
```

下载：
```
ESP-IDF v5.4.3 - Offline Installer
大小：1.53 GB
系统：Windows 10/11
```

### 步骤 2: 运行安装器

1. 运行下载的 `esp-idf-tools-setup-5.4.3.exe`
2. 选择安装组件：
   - ✅ ESP32-S3 支持
   - ✅ Git（如果没有）
   - ✅ Python（如果没有）
   - ✅ VS Code 扩展（推荐）
3. 等待安装完成（5-10分钟）

### 步骤 3: 验证安装

打开 **ESP-IDF 5.4 CMD**：
- 按 `Win` 键，搜索 "ESP-IDF 5.4 CMD" 并打开
- 运行：
  ```cmd
  idf.py --version
  ```

应该看到：
```
ESP-IDF v5.4.3
```

---

## 项目克隆

### 步骤 1: 创建工作目录

```bash
mkdir -p ~/Desktop/Evo-spark
cd ~/Desktop/Evo-spark
```

### 步骤 2: 克隆项目

```bash
git clone --branch v2.0.3 --depth 1 https://github.com/78/xiaozhi-esp32.git
```

**说明：**
- `--branch v2.0.3`：克隆特定稳定版本
- `--depth 1`：只克隆最新提交，减少下载时间

---

## 编译流程

### 步骤 1: 打开 ESP-IDF 5.4 CMD

按 `Win` 键，搜索 "ESP-IDF 5.4 CMD" 并打开。

### 步骤 2: 切换到项目目录

```cmd
cd C:\Users\xyc\Desktop\Evo-spark\xiaozhi-esp32
```

### 步骤 3: 设置目标芯片

```cmd
idf.py set-target esp32s3
```

**预期输出：**
```
Set Target to: esp32s3, new sdkconfig will be created.
```

### 步骤 4: 编译项目

```cmd
idf.py build
```

**预期输出：**
```
Executing "ninja build"...
[进度条]
Project build complete. To flash, run:
    idf.py flash
or
    idf.py -p (PORT) flash
```

**编译时间：**
- 首次编译：5-15 分钟
- 增量编译：1-3 分钟

---

## 烧录到设备

### 步骤 1: 检查串口号

```cmd
mode
```

找到类似：
```
设备状态 COM4:
----------
    波特率:     9600
```

**注意：** 端口号可能因插拔 USB 而变化（COM3、COM4、COM5...）

### 步骤 2: 烧录固件

```cmd
idf.py -p COM4 flash monitor
```

**参数说明：**
- `-p COM4`：指定串口（根据实际情况）
- `flash`：烧录固件
- `monitor`：烧录后自动打开串口监视

**预期输出：**
```
Writing at 0x00010000... (100%)
Wrote 2457856 bytes in 19.2 seconds
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
```

### 步骤 3: 进入下载模式（如果需要）

如果烧录卡住，进入下载模式：
1. 按住 **BOOT** 按钮
2. 按一下 **RESET** 按钮
3. 松开两个按钮
4. 立即执行烧录命令

---

## 常见问题解决

### 问题 1: 组件依赖错误

**错误信息：**
```
ERROR: espressif2022/image_player (==1.1.0~1) doesn't match any versions
```

**原因：** ESP-IDF 版本与组件库不兼容。

**解决方案：**

注释掉问题依赖，编辑文件：
```bash
main/idf_component.yml
```

在第 38 行前加 `#`：
```yaml
# espressif2022/image_player: ==1.1.0~1
```

然后重新编译。

---

### 问题 2: 芯片型号不匹配

**错误信息：**
```
This chip is ESP32-S3, not ESP32. Wrong --chip argument?
```

**原因：** `sdkconfig` 中的 `CONFIG_IDF_TARGET` 设置错误。

**解决方案：**

重新设置目标芯片：
```cmd
rd /s /q build
idf.py set-target esp32s3
idf.py build
```

验证配置：
```bash
grep "CONFIG_IDF_TARGET" sdkconfig
```

应该看到：
```
CONFIG_IDF_TARGET="esp32s3"
CONFIG_IDF_TARGET_ESP32S3=y
```

---

### 问题 3: COM 端口被占用

**错误信息：**
```
Could not open COM4, port is busy or doesn't exist.
```

**解决方案：**

1. **关闭占用端口的程序**
   - VS Code 串口监视器
   - Arduino IDE
   - PuTTY/Tera Term

2. **重新插拔 USB**
   - 拔掉 USB 线
   - 等待 2 秒
   - 重新插上
   - 检查端口变化

3. **检查可用端口**
   ```cmd
   mode
   ```

---

### 问题 4: Assets 文件缺失

**错误信息：**
```
WARNING: File generated_assets.bin is empty
No such file or directory: 'generated_assets.bin'
```

**原因：** 资源文件没有正确生成。

**解决方案 A：创建空文件（临时）**

```bash
cd build
python -c "open('generated_assets.bin', 'wb').write(b'')"
```

**解决方案 B：修改分区表（推荐）**

```cmd
idf.py menuconfig
```

导航到：
```
Partition Table → Partition table
→ Custom partition table → partitions/v1/8m.csv
```

保存后重新编译。

---

### 问题 5: 设备处于下载模式

**日志信息：**
```
boot:0x0 (DOWNLOAD(USB/UART0))
waiting for download
```

**原因：** BOOT 按钮被按住，或烧录模式异常。

**解决方案：**

1. **按一下 RESET 按钮**，让设备正常启动
2. **检查 BOOT 按钮**是否卡住，按一下弹起
3. **重新插拔 USB**

正常启动应该看到：
```
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
```

---

## 设备启动与配置

### 正常启动日志

成功烧录后，串口监视器会显示：

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

### 配置 WiFi

设备启动后，会创建 WiFi 热点：

```
热点名称: Xiaozhi-9EE1
密码: 无（或其他）
```

**配置步骤：**

1. 用手机连接热点 `Xiaozhi-9EE1`
2. 用浏览器访问：
   ```
   http://192.168.4.1
   ```
3. 在配置页面设置：
   - 你的 WiFi 网络名称和密码
   - 服务器地址（官方或自建）

### WiFi 连接成功日志

```
I (xxx) WifiStation: Got IP: 192.168.31.66
I (xxx) Display: SetStatus: 已连接
I (xxx) Application: 激活设备
I (xxx) Display:      xiaozhi.me:847442
```

---

## 下一步开发方向

### 熟悉原版功能

测试 xiaozhi-esp32 的现有功能：
- ✅ 语音唤醒（默认："你好小智"）
- ✅ 对话功能（通过 xiaozhi.me 平台）
- ✅ 表情显示
- ✅ MCP 工具调用（控制 GPIO、传感器等）
- ✅ OTA 升级

访问配置页面：
```
http://192.168.31.66
```

---

### 自定义修改方向

#### 1. 添加 ClaudeCode 接入

创建新协议类 `ClaudeCodeProtocol`，继承 `Protocol`：

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

实现 WebSocket 客户端连接本地 ClaudeCode 服务。

#### 2. 直接调用 LLM API

新增 `DirectApiProtocol`：

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

支持的 API：
- Claude API (Anthropic)
- GLM-4.7-flash (智谱)
- OpenAI API

#### 3. 实现本地记忆系统

创建 `memory/` 目录：

```
main/memory/
├── memory_manager.h       # 记忆管理器
├── memory_manager.cc
├── file_storage.h          # 文件存储接口
└── file_storage.cc
```

实现文件：
- `SOUL.md` - 机器人"灵魂"（性格、设定）
- `USER.md` - 用户信息（姓名、偏好）
- `MEMORY.md` - 长期对话记忆

使用 SPIFFS/LittleFS 存储：
```cpp
// 读取记忆
void load_memory(char *buffer, size_t size);

// 保存记忆
void save_memory(const char *data);

// 智能更新
void update_memory(const char *conversation);
```

#### 4. 增强 Agent 逻辑

新增 `ai/` 目录：

```
main/ai/
├── agent.h               # Agent 核心逻辑
├── agent.cc
├── context.h             # 对话上下文管理
├── context.cc
└── llm_api.h             # LLM API 统一接口
```

实现功能：
- 上下文窗口管理
- 对话历史跟踪
- 智能决策
- 记忆检索和更新

---

## 项目结构建议

```
Evo-spark/
├── xiaozhi-esp32/           # 原版（已测试）
├── evospark-mod/             # 修改版（新分支）
│   ├── main/
│   │   ├── protocols/
│   │   │   ├── claudecode_protocol.h   # ClaudeCode 客户端
│   │   │   ├── direct_api_protocol.h     # 直接 API 调用
│   │   │   └── protocol.h              # 抽象协议层
│   ├── memory/                 # 记忆系统
│   │   ├── memory_manager.h
│   │   ├── memory_manager.cc
│   │   ├── file_storage.h
│   │   └── file_storage.cc
│   ├── ai/                     # Agent 系统
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
└── README.md                  # 本教程
```

---

## 参考资料

- [xiaozhi-esp32 GitHub](https://github.com/78/xiaozhi-esp32)
- [xiaozhi.me 官网](https://xiaozhi.me)
- [ESP-IDF 官方文档](https://docs.espressif.com/projects/esp-idf/)
- [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html)

---

## 更新日志

### v1.0 (2026-02-23)
- 初始版本
- 完整的环境准备、编译、烧录流程
- 常见问题解决方案
- 下一步开发方向说明

---

**作者**: xyc200602
**日期**: 2026年2月23日
**项目**: Evo-spark - 基于 xiaozhi-esp32 的桌面陪伴机器人

---

## License

MIT License

---

> **智能不是训练出来的。是学出来的。**
