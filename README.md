# Evo-spark - Desktop Companion Robot Based on xiaozhi-esp32
# Evo-spark - 基于 xiaozhi-esp32 的桌面陪伴机器人

---

## Project Introduction / 项目简介

Evo-spark is a desktop companion robot development project based on the open-source project [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32).

Evo-spark 是基于开源项目 [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) 的桌面陪伴机器人开发项目。

### Core Concept / 核心理念

> **"Intelligence is not trained. It is learned."**
> **"智能不是训练出来的。是学出来的。"**

---

## Project Features / 项目特点

- 🎯 ESP32-S3 Support (8MB PSRAM + 16MB Flash)
- 📱 WiFi Network Connection
- 🗣️ Offline Voice Wake-up and Recognition
- 🤖 MCP (Model Context Protocol) Support for Tool Calling
- 🎨 Display Support with Expressions
- 📡 OPUS Audio Encoding/Decoding
- 🔄 OTA Firmware Updates

---

## Quick Start / 快速开始

### Environment Setup / 环境准备

See: [ESP32-S3 xiaozhi-esp32 Development Tutorial](./ESP32-S3-xiaozhi-esp32-开发教程.md) (Chinese)

详见：[ESP32-S3 xiaozhi-esp32 Development Tutorial](./ESP32-S3-xiaozhi-esp32-开发教程.md)

### Hardware Requirements / 硬件要求

- ESP32-S3-N16R8 Dev Board or Compatible Board
- USB Data Cable (Supports Data Transfer)

### Development Environment / 开发环境

- ESP-IDF v5.4
- Windows 10/11
- VS Code or ESP-IDF Extension

---

## Project Structure / 项目结构

```
Evo-spark/
├── ESP32-S3-xiaozhi-esp32-开发教程.md    # Complete Development Tutorial / 完整开发教程
├── README.md                                  # This File / 本文件
└── xiaozhi-esp32/                           # Reference to Original / 原版参考
    ├── main/                                  # Main Program / 主程序
    │   ├── protocols/                           # Communication Protocols / 通信协议
    │   │   ├── protocol.h
    │   │   ├── websocket_protocol.h
    │   │   └── mqtt_protocol.h
    │   ├── audio/                               # Audio Processing / 音频处理
    │   ├── display/                             # Display Driver / 显示驱动
    │   ├── boards/                              # Board Config / 板级配置
    │   ├── mcp_server.h/.cc                  # MCP Server / MCP 服务端
    │   └── application.h/.cc                 # Main Application / 主应用程序
    └── ...
```

---

## Development Roadmap / 开发路线图

### Planned Modifications / 计划修改

1. **ClaudeCode Integration / ClaudeCode 接入**
   - WebSocket Client connecting to local ClaudeCode service
   - Voice input processing through ClaudeCode
   - Replace original Telegram interface

2. **Direct LLM API Calling / 直接 LLM API 调用**
   - Claude API Support (Anthropic)
   - GLM-4.7-flash Support (Zhipu AI)
   - Replace original server-based solution

3. **Local Memory System / 本地记忆系统**
   - SOUL.md - Robot's "personality"
   - USER.md - User information (name, preferences)
   - MEMORY.md - Long-term conversation memory
   - SPIFFS/LittleFS file storage

4. **Enhanced Agent Capabilities / 增强 Agent 能力**
   - Context window management
   - Smart memory retrieval
   - Multi-turn conversation optimization

---

## Current Status / 当前状态

- ✅ ESP-IDF v5.4 Environment Setup Completed / 环境配置完成
- ✅ xiaozhi-esp32 v2.0.3 Compiled Successfully / 编译成功
- ✅ Firmware Flashed to ESP32-S3 / 固件烧录到 ESP32-S3
- ✅ Device Running Normally / 设备正常运行
- 📝 Documentation Uploaded / 文档已上传

---

## References / 参考资料

### Open Source Projects / 开源项目

- [xiaozhi-esp32 Official Repository](https://github.com/78/xiaozhi-esp32)
- [xiaozhi.me Official Platform](https://xiaozhi.me)
- [MimiClaw](https://github.com/memovai/mimiclaw) - AI Agent on ESP32
- [OpenClaw](https://github.com/anthropics/openclaw) - Multi-channel AI Gateway

### Documentation / 文档

- [ESP-IDF Official Documentation](https://docs.espressif.com/projects/esp-idf/)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html)

---

## License / 许可证

MIT License

---

## Author / 作者

**xyc200602**
**Project**: Evo-spark - Desktop Companion Robot

---

> **"Intelligence is not trained. It is learned."**
> **"智能不是训练出来的。是学出来的。"**
