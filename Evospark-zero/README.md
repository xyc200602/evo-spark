# Evo-spark-zero

基于 ESP32-S3 的智能陪伴机器人 - 记忆系统

## 硬件要求

- ESP32-S3-WROOM-1-N16R8 (16MB PSRAM, 16MB Flash)
- WiFi 连接（用于调用 GLM-4.7-flash API）

## 🎉 新功能 - Web 配置系统

**现在无需修改代码即可配置设备！**

### 首次使用

1. **烧录固件**到设备
2. 设备自动启动 **AP 模式**（WiFi 热点）
3. 连接 WiFi：`EvoSpark-Setup`
4. 密码：`evo-spark-123`
5. 打开浏览器访问：**http://192.168.4.1/config**
6. 填写配置信息：
   - WiFi SSID（你的 WiFi 名称）
   - WiFi 密码
   - GLM API Key：`your-api-key-here` (请在 GLM 平台获取)
7. 点击"保存配置并重启"
8. 设备重启后自动连接到你的 WiFi 网络
9. 访问主界面：**http://设备IP:80**

### 重新配置

- 在主界面点击右上角的"⚙️ 配置"链接
- 或者直接访问：**http://设备IP:80/config**
- 修改配置后设备将自动重启

## 功能特性

### 🌐 Web 界面
- 对话界面：通过浏览器与 Evo-spark 实时对话
- 实时监控：查看系统状态、存储使用、记忆包
- 历史管理：回滚到任意历史版本
- **Web 配置**：无需修改代码即可配置 WiFi 和 API Key
- WebSocket 实时通信

### 记忆系统
- 对话缓冲：暂存最近 20 条消息（最大 10KB）
- 智能压缩：调用 GLM-4.7-flash API 压缩记忆
- 自动触发：5 分钟静默后自动更新
- 历史版本：保留 3 个历史备份
- 闪存存储：使用 SPIFFS 持久化

### 存储管理
- 主记忆文件：`/spiffs/memory.json`
- 备份文件：`/spiffs/memory_backup_1.json` ~ `_3.json`
- 滚动备份：每次更新时滚动备份版本
- 回滚支持：可回滚到任意历史版本

### 配置管理
- NVS 存储：配置信息保存在 Flash 的 NVS 分区
- AP/STA 模式：自动切换，未配置时进入配置模式
- WiFi 自动重连：连接失败后自动重试 10 次

## 记忆包结构
```json
{
  "version": "1.0",
  "metadata": {
    "created_at": "2026-02-26T15:30:00Z",
    "last_updated": "2026-02-26T20:45:00Z",
    "compression_level": 2,
    "total_memories": 12
  },
  "user_profile": {
    "name": "用户",
    "preferences": [...],
    "traits": [...]
  },
  "memories": [
    {
      "id": "mem_001",
      "type": "conversation",
      "summary": "...",
      "importance": 0.7,
      "timestamp": "...",
      "context": "..."
    }
  ],
  "recent_context": {
    "last_topic": "...",
    "emotional_state": "...",
    "interaction_style": "..."
  }
}
```

## 编译和烧录

### 1. 安装 ESP-IDF

参考：https://docs.espressif.com/projects/esp-idf/

### 2. 配置 SPIFFS 分区

确保 `partitions.csv` 包含 SPIFFS 分区：

```
# Name,   Type, SubType, Offset,  Size,   Flags
nvs,      data, nvs,     0x9000,  0x5000,
phy_init, data, phy,     0xe000,  0x1000,
factory,  app,  factory, 0x10000,  1M,
storage,  data, spiffs,  0x110000, 0x100000,
```

### 3. 设置编译环境

```bash
# Windows (Git Bash)
. $IDF_PATH/export.sh

# 或使用 ESP-IDF CMD 提示符
```

### 4. 编译

```bash
idf.py build
```

### 5. 烧录

```bash
idf.py -p COM5 flash
```

### 6. 监控

```bash
idf.py -p COM5 monitor
```

### 7. 编译烧录一条命令

```bash
idf.py -p COM5 build flash monitor
```

## 🌐 Web 界面使用

### 访问配置页面

首次启动或设备未配置时：

1. 连接 WiFi：`EvoSpark-Setup`
2. 密码：`evo-spark-123`
3. 访问：**http://192.168.4.1/config**

配置完成后访问主界面：

```
http://设备IP:80
```

例如：`http://192.168.31.66:80`

### Web 功能

#### 配置界面
- WiFi SSID 配置
- WiFi 密码配置
- GLM API Key 配置
- 保存后自动重启

#### 对话界面
- 输入消息并发送
- 查看对话历史
- 实时显示 AI 回复

#### 实时监控
- **系统状态**：在线/离线指示
- **存储使用**：Flash 空间使用情况（进度条）
- **对话消息数**：当前缓冲区的消息数量
- **缓冲区大小**：对话缓冲区占用 KB
- **最后更新**：数据更新时间

#### 记忆包查看
- 查看当前记忆包的 JSON 内容
- 格式化显示，易于阅读
- 包含用户画像、记忆项、上下文

#### 历史版本管理
- 显示当前版本和历史备份列表
- 点击回滚到指定版本

### API 端点

```
GET  /                  # Web 主界面
GET  /config           # Web 配置界面
GET  /ws               # WebSocket 实时通信
POST /api/config       # 保存配置
POST /api/restart      # 重启设备
POST /api/conversation # 发送对话消息
GET  /api/memory      # 获取当前记忆包
GET  /api/status       # 获取系统状态
POST /api/rollback     # 回滚到历史版本
```

### WebSocket 消息

```json
// 对话消息
{
  "type": "conversation",
  "role": "user",
  "content": "你好"
}

// 状态更新
{
  "type": "status",
  "status": "在线"
}

// 记忆更新
{
  "type": "memory",
  "memory": "{...}"
}

// 监控数据
{
  "type": "monitor",
  "data": {
    "conversation_count": 5,
    "buffer_size": 1024,
    "free_space_kb": 8192,
    "used_space_kb": 1024
  }
}
```

## 使用示例

### 添加对话

```cpp
MemoryManager& mgr = MemoryManager::GetInstance();
mgr.AddConversation("user", "你好，我叫小明");
mgr.AddConversation("assistant", "你好小明，很高兴认识你！");
```

### 手动更新记忆

```cpp
mgr.UpdateMemory();  // 立即触发记忆压缩和存储
```

### 等待自动更新

默认 5 分钟无新对话后自动更新：
```cpp
mgr.SetSilenceTimeout(300000);  // 5 分钟（毫秒）
```

### 回滚到历史版本

```cpp
// 回滚到最新的备份
mgr.RollbackToBackup(1);

// 回滚到次新的备份
mgr.RollbackToBackup(2);

// 回滚到最旧的备份
mgr.RollbackToBackup(3);
```

### 获取记忆包

```cpp
MemoryPackage memory = mgr.GetMemoryPackage();
ESP_LOGI(TAG, "Memory: %s", memory.raw_json.c_str());
```

### 获取配置

```cpp
ConfigManager& config = ConfigManager::GetInstance();

if (config.IsConfigured()) {
    std::string ssid = config.GetWifiSSID();
    std::string password = config.GetWifiPassword();
    std::string api_key = config.GetApiKey();

    ESP_LOGI(TAG, "WiFi SSID: %s", ssid.c_str());
    ESP_LOGI(TAG, "API Key: %s***", api_key.substr(0, 8).c_str());
}
```

### 清除配置（恢复出厂）

```cpp
ConfigManager& config = ConfigManager::GetInstance();
config.ClearConfig();
esp_restart();  // 重启设备
```

## 目录结构

```
Evospark-zero/
├── main/
│   ├── memory/
│   │   ├── memory_types.h/cc      # 数据结构定义
│   │   ├── conversation_buffer.h/cc  # 对话缓冲
│   │   └── memory_manager.h/cc      # 核心管理器
│   ├── storage/
│   │   └── flash_storage.h/cc    # Flash 存储
│   ├── api/
│   │   └── glm_client.h/cc       # GLM API 客户端
│   ├── config/
│   │   ├── config_manager.h/cc   # 配置管理器
│   ├── web/
│   │   └── web_server.h/cc      # Web 服务器
│   ├── main.cc                   # 主程序
│   └── CMakeLists.txt
├── partitions.csv                 # Flash 分区表
├── sdkconfig                     # ESP-IDF 配置
├── CMakeLists.txt               # 项目配置
├── README.md                    # 本文件
├── TODO.md                     # 待办事项
└── BUG_FIXES_SUMMARY.md         # Bug 修复总结
```

## 技术栈

- **固件框架**: ESP-IDF v5.4+
- **编程语言**: C++17
- **LLM API**: GLM-4.7-flash (智谱AI)
- **存储**: SPIFFS (Flash 文件系统) + NVS (非易失性存储)
- **HTTP**: esp_http_client
- **定时器**: ESP Timer
- **WiFi**: AP/STA 模式自动切换

## 故障排查

### 设备无法连接到 WiFi

1. 检查 WiFi SSID 和密码是否正确
2. 检查 WiFi 信号强度
3. 尝试重新配置：访问 `http://192.168.4.1/config`
4. 或者通过主界面点击"⚙️ 配置"

### 配置后设备没有重启

1. 手动重启设备（断电再通电）
2. 检查串口日志查看错误信息

### 记忆功能不工作

1. 检查 GLM API Key 是否正确
2. 检查设备是否已连接到 WiFi
3. 查看串口日志中的错误信息

### Web 界面无法访问

1. 确认设备已连接到 WiFi
2. 查看串口日志获取设备 IP 地址
3. 尝试直接访问：`http://192.168.4.1` (AP 模式)

### 恢复出厂设置

清除所有配置：

```cpp
// 在串口 monitor 中输入（需要添加调试命令）
// 或通过代码：
config.ClearConfig();
esp_restart();
```

未来版本将添加 Web 界面按钮恢复出厂设置。

## 未来扩展

- [x] Web 配置界面（WiFi 和 API Key）
- [x] AP/STA 模式自动切换
- [x] 配置持久化（NVS）
- [ ] 恢复出厂设置按钮
- [ ] 麦克风输入
- [ ] 扬声器输出
- [ ] 摄像头视觉
- [ ] 机械臂控制
- [ ] 局域网控制 API

## GLM-4.7-flash API

- **文档**: https://docs.bigmodel.cn/cn/guide/models/text/glm-4.7
- **API Endpoint**: https://open.bigmodel.cn/api/paas/v4/chat/completions
- **格式**: OpenAI 兼容
- **费用**: 免费使用

## 许可证

MIT License

## 作者

CKLC Evo-spark 项目组

---

**让具身智能的未来发生 - 从简单开始！**
