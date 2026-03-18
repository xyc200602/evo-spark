# EvoSpark v2.0 - 完整版

基于 ESP32-S3 的智能对话机器人

## ✨ 完整功能列表

### 核心系统
- ✅ **会话管理** - 按键唤醒、任务式对话、静默/按键结束
- ✅ **记忆系统** - 长期记忆、会话记忆、LLM 压缩
- ✅ **事件总线** - 模块间解耦通信
- ✅ **配置管理** - NVS 存储、AP/STA 模式
- ✅ **Web 服务器** - 配置界面、状态监控

### 感知系统
- ✅ **摄像头** - OV2640 驱动、JPEG 捕获
- ✅ **YOLO** - 物体检测框架（待加载模型）
- ✅ **麦克风** - I2S 音频输入
- ✅ **扬声器** - I2S 音频输出

### 显示系统
- ✅ **LCD 驱动** - ST7789 SPI 驱动
- ✅ **UI 管理** - 状态显示、对话界面

### AI 系统
- ✅ **LLM 客户端** - GLM API 集成
- ✅ **Prompt 构建** - 记忆注入、多模态支持

### 执行器
- ✅ **LED 控制** - WS2812 RGB、状态指示
- ✅ **按键输入** - GPIO 中断、防抖
- ⏳ **舵机控制** - PWM 接口预留

## 📊 项目规模

| 类别 | 文件数 | 代码量 |
|------|--------|--------|
| 头文件 | 19 | ~25KB |
| 源文件 | 19 | ~92KB |
| 配置文件 | 3 | ~2KB |
| 文档 | 3 | ~10KB |
| **总计** | **44** | **~130KB** |

## 🏗️ 编译指南

### 环境要求
- ESP-IDF v5.4+
- Python 3.8+
- CMake 3.16+

### 步骤

```bash
# 1. 设置环境
. $IDF_PATH/export.sh

# 2. 进入项目目录
cd Evospark-v2

# 3. 配置（可选）
idf.py menuconfig

# 4. 编译
idf.py build

# 5. 烧录
idf.py -p COM5 flash

# 6. 监控
idf.py -p COM5 monitor
```

## 🔧 硬件连接

| 功能 | GPIO | 备注 |
|------|------|------|
| Boot 按键 | GPIO0 | 唤醒/结束 |
| WS2812 LED | GPIO45 | 状态指示 |
| 摄像头 | GPIO4-18, 21 | OV2640 |
| 音频 I2C | GPIO38, 39 | ES8311/ES7210 |
| 音频 I2S | GPIO35-41 | 数据线 |
| LCD SPI | GPIO1-3, 42, 47, 48 | ST7789 |
| 扩展 PWM | GPIO24, 25 | 舵机预留 |

## 📁 目录结构

```
Evospark-v2/
├── main/
│   ├── main.cc                    # 主程序入口
│   ├── core/                      # 核心模块
│   │   ├── session_manager.*      # 会话管理
│   │   └── event_bus.*            # 事件总线
│   ├── memory/                    # 记忆系统
│   │   ├── memory_types.h         # 数据类型
│   │   ├── conversation_buffer.*  # 对话缓冲
│   │   ├── prompt_builder.*       # Prompt 构建
│   │   └── memory_manager.*       # 记忆管理
│   ├── perception/                # 感知模块
│   │   ├── audio/                 # 音频
│   │   ├── camera/                # 摄像头
│   │   └── vision/                # 视觉
│   ├── display/                   # 显示模块
│   │   ├── lcd_driver.*           # LCD 驱动
│   │   └── ui_manager.*           # UI 管理
│   ├── ai/                        # AI 模块
│   │   └── llm_client.*           # LLM 客户端
│   ├── input/                     # 输入模块
│   │   └── button.*               # 按键
│   ├── actuator/                  # 执行器
│   │   └── led_controller.*       # LED
│   ├── storage/                   # 存储
│   │   └── flash_storage.*        # SPIFFS
│   ├── config/                    # 配置
│   │   └── config_manager.*       # NVS
│   └── web/                       # Web 服务器
│       └── web_server.*           # HTTP
├── components/                    # ESP-IDF 组件
├── resources/                     # 资源文件
├── CMakeLists.txt
├── partitions.csv
├── sdkconfig.defaults
├── README.md
└── BUG_REPORT.md
```

## 🚀 使用流程

### 首次使用
1. 烧录固件
2. 连接 WiFi `EvoSpark-v2-Setup`
3. 访问 http://192.168.4.1/config
4. 配置 WiFi 和 API Key
5. 保存，设备自动重启

### 日常使用
1. **唤醒** - 按下 Boot 按键，LED 变绿
2. **对话** - 通过 Web 或串口输入
3. **结束** - 再按按键或等待 5 分钟
4. **记忆** - 自动压缩保存

## ⚠️ 已知限制

1. **YOLO 检测** - 框架已完成，需要加载实际模型
2. **ASR/TTS** - 框架已完成，需要集成云端 API
3. **LVGL** - 未集成，显示功能简化
4. **JSON 解析** - 使用简单字符串匹配，建议升级

## 📝 开发状态

| Phase | 模块 | 状态 | 完成度 |
|-------|------|------|--------|
| Phase 1 | 会话系统 | ✅ | 100% |
| Phase 2 | 音频系统 | ✅ | 90% |
| Phase 3 | 视觉系统 | ✅ | 80% |
| Phase 4 | 显示系统 | ✅ | 70% |
| Phase 5 | 整合测试 | ⏳ | 0% |

## 🔗 相关文档

- [技术设计](../EvoSpark-v2-DESIGN.md)
- [Bug 检查报告](BUG_REPORT.md)
- [硬件设计](../evospark-hardware/HARDWARE_DESIGN.md)

## 📄 许可证

MIT License

---

**"让具身智能的未来发生"**

---

*项目完成时间: 2026-03-18*
*代码规模: 44 文件, ~130KB*
*开发时长: ~2 小时*
