# Evo-spark - 基于 xiaozhi-esp32 的桌面陪伴机器人

## 项目简介

Evo-spark 是基于开源项目 [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) 的桌面陪伴机器人开发项目。

**核心理念**：
> "智能不是训练出来的。是学出来的。"

## 项目特点

- 🎯 基于 ESP32-S3（8MB PSRAM + 16MB Flash）
- 📱 支持 WiFi 网络连接
- 🗣️ 离线语音唤醒和识别
- 🤖 支持 MCP（Model Context Protocol）工具调用
- 🎨 表情显示支持
- 📡 OPUS 音频编解码
- 🔄 OTA 固件升级

## 快速开始

### 环境准备

详见：[ESP32-S3 xiaozhi-esp32 开发教程](./ESP32-S3-xiaozhi-esp32-开发教程.md)

### 硬件要求

- ESP32-S3-N16R8 开发板或兼容板
- USB 数据线

### 开发环境

- ESP-IDF v5.4
- Windows 10/11
- VS Code 或 ESP-IDF 插件

## 项目结构

```
Evo-spark/
├── ESP32-S3-xiaozhi-esp32-开发教程.md    # 完整开发教程
├── README.md                                  # 本文件
└── xiaozhi-esp32/                           # 原版项目（参考）
    ├── main/                                  # 主程序
    │   ├── protocols/                           # 通信协议
    │   │   ├── protocol.h
    │   │   ├── websocket_protocol.h
    │   │   └── mqtt_protocol.h
    │   ├── audio/                               # 音频处理
    │   ├── display/                             # 显示驱动
    │   ├── boards/                              # 板级配置
    │   ├── mcp_server.h/.cc                  # MCP 服务端
    │   └── application.h/.cc                 # 主应用程序
    └── ...
```

## 开发方向

### 规划的修改

1. **ClaudeCode 接入**
   - WebSocket 客户端连接本地 ClaudeCode
   - 语音输入通过 ClaudeCode 处理
   - 替代原 Telegram 接口

2. **直接 LLM API 调用**
   - 支持 Claude API (Anthropic)
   - 支持 GLM-4.7-flash (智谱)
   - 替代原官方服务器方案

3. **本地记忆系统**
   - SOUL.md - 机器人"灵魂"
   - USER.md - 用户信息
   - MEMORY.md - 长期对话记忆
   - SPIFFS/LittleFS 文件存储

4. **增强 Agent 能力**
   - 对话上下文管理
   - 智能记忆检索
   - 多轮对话优化

## 当前状态

- ✅ ESP-IDF v5.4 环境配置完成
- ✅ xiaozhi-esp32 v2.0.3 编译成功
- ✅ 固件烧录到 ESP32-S3 设备
- ✅ 设备正常运行
- 📝 文档已上传

## 参考资料

- [xiaozhi-esp32 官方仓库](https://github.com/78/xiaozhi-esp32)
- [xiaozhi.me 官网](https://xiaozhi.me)
- [ESP-IDF 官方文档](https://docs.espressif.com/projects/esp-idf/)
- [MimiClaw 项目](https://github.com/memovai/mimiclaw)
- [OpenClaw 参考](https://github.com/anthropics/openclaw)

## 贡献

欢迎提交 Issue 和 Pull Request！

## License

MIT License

## 作者

**xyc200602**
**项目**: Evo-spark

---

> "智能不是训练出来的。是学出来的。"
