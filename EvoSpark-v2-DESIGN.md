# EvoSpark v2.0 技术设计

> 更新时间: 2026-03-18
> 状态: 🚧 设计中

---

## 一、系统架构

```
┌──────────────────────────────────────────────────────────────────┐
│                        EvoSpark v2.0                             │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │                    交互层 (Interaction)                     │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │  │
│  │  │  按键    │  │  屏幕    │  │  麦克风  │  │  扬声器  │   │  │
│  │  │ (唤醒)   │  │ (LVGL)   │  │ (I2S)    │  │ (I2S)    │   │  │
│  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │  │
│  └────────────────────────────────────────────────────────────┘  │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │                    会话层 (Session)                         │  │
│  │                                                            │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │               SessionManager                         │  │  │
│  │  │  - 按键唤醒 → 创建新会话                              │  │  │
│  │  │  - 静默超时 / 按键结束 → 关闭会话                      │  │  │
│  │  │  - 会话生命周期管理                                   │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                          │                                │  │
│  │                          ▼                                │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │               ConversationBuffer                     │  │  │
│  │  │  - 当前会话的对话历史                                 │  │  │
│  │  │  - 支持 user/assistant/system 消息                   │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────────┘  │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │                    感知层 (Perception)                      │  │
│  │                                                            │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │  │
│  │  │   摄像头      │  │   YOLO       │  │   语音识别    │     │  │
│  │  │  (OV2640)    │→│  Detector    │  │   (ASR)      │     │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘     │  │
│  │         │                  │                  │           │  │
│  │         └──────────────────┴──────────────────┘           │  │
│  │                            │                              │  │
│  │                            ▼                              │  │
│  │                    多模态输入融合                          │  │
│  └────────────────────────────────────────────────────────────┘  │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │                    AI 层 (AI Engine)                        │  │
│  │                                                            │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │              PromptBuilder                           │  │  │
│  │  │  - 加载长期记忆作为 system prompt                     │  │  │
│  │  │  - 注入当前会话上下文                                 │  │  │
│  │  │  - 添加多模态输入（图像描述、语音转文字）              │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                          │                                │  │
│  │                          ▼                                │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │              LLMClient (GLM-4V / GPT-4V)             │  │  │
│  │  │  - 多模态对话 API 调用                                │  │  │
│  │  │  - 流式响应处理                                       │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────────┘  │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │                    记忆层 (Memory)                          │  │
│  │                                                            │  │
│  │  ┌──────────────────────────────────────────────────────┐ │  │
│  │  │                MemoryManager v2                       │ │  │
│  │  │                                                      │ │  │
│  │  │  ┌────────────────┐    ┌────────────────┐           │ │  │
│  │  │  │  长期记忆       │    │  会话记忆       │           │ │  │
│  │  │  │ compressed.json│    │ session_buffer │           │ │  │
│  │  │  │              │    │              │           │ │  │
│  │  │  │ - 用户画像     │    │ - 当前对话     │           │ │  │
│  │  │  │ - 重要事件     │    │ - 上下文窗口   │           │ │  │
│  │  │  │ - 偏好习惯     │    │              │           │ │  │
│  │  │  └────────────────┘    └────────────────┘           │ │  │
│  │  │         ▲                       │                   │ │  │
│  │  │         │      会话关闭时        │                   │ │  │
│  │  │         │    ┌──────────────┐   │                   │ │  │
│  │  │         └────│  LLM 压缩    │◄──┘                   │ │  │
│  │  │              │ old + new    │                       │ │  │
│  │  │              │    ↓ new     │                       │ │  │
│  │  │              └──────────────┘                       │ │  │
│  │  └──────────────────────────────────────────────────────┘ │  │
│  └────────────────────────────────────────────────────────────┘  │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │                    执行层 (Actuator)                        │  │
│  │                                                            │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │  │
│  │  │  扬声器  │  │  屏幕    │  │  LED     │  │  舵机    │   │  │
│  │  │ (TTS)    │  │ (显示)   │  │ (状态)   │  │ (预留)   │   │  │
│  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

---

## 二、会话生命周期

```
┌─────────────────────────────────────────────────────────────────┐
│                    Session Lifecycle                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────┐                                                    │
│  │  IDLE   │ ← 系统启动，等待唤醒                                │
│  └────┬────┘                                                    │
│       │                                                         │
│       │ 按键按下                                                 │
│       ▼                                                         │
│  ┌─────────┐                                                    │
│  │ WAKING  │ ← 初始化会话，加载记忆                              │
│  └────┬────┘                                                    │
│       │                                                         │
│       │ 会话创建完成                                             │
│       ▼                                                         │
│  ┌─────────┐     用户输入      ┌─────────┐                      │
│  │ LISTEN- │ ───────────────→ │PROCESS- │                      │
│  │  ING    │                   │  ING    │                      │
│  └────┬────┘ ←─────────────── └─────────┘                      │
│       │          AI 响应完成                                    │
│       │                                                         │
│       │ 超时 N 分钟 / 按键结束                                   │
│       ▼                                                         │
│  ┌─────────┐                                                    │
│  │CLOSING  │ ← 压缩记忆，保存，清理                              │
│  └────┬────┘                                                    │
│       │                                                         │
│       │ 会话关闭完成                                             │
│       ▼                                                         │
│  ┌─────────┐                                                    │
│  │  IDLE   │ ← 返回待机状态                                      │
│  └─────────┘                                                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 三、目录结构

```
Evospark-v2/
├── main/
│   ├── main.cc                      # 主程序入口
│   │
│   ├── core/                        # 核心模块
│   │   ├── session_manager.h/cc     # 🆕 会话管理器
│   │   ├── event_bus.h/cc           # 🆕 事件总线（模块间通信）
│   │   └── state_machine.h/cc       # 🆕 状态机
│   │
│   ├── memory/                      # 记忆系统 (升级)
│   │   ├── memory_types.h           # 数据结构
│   │   ├── conversation_buffer.h/cc # 对话缓冲区
│   │   ├── memory_manager.h/cc      # ✨ 升级 v2
│   │   └── prompt_builder.h/cc      # 🆕 Prompt 构建器
│   │
│   ├── perception/                  # 🆕 感知模块
│   │   ├── camera/
│   │   │   ├── ov2640_driver.h/cc   # 摄像头驱动
│   │   │   └── camera_manager.h/cc  # 摄像头管理
│   │   ├── vision/
│   │   │   ├── yolo_detector.h/cc   # YOLO 物体检测
│   │   │   └── image_utils.h/cc     # 图像处理工具
│   │   └── audio/
│   │       ├── i2s_audio.h/cc       # I2S 音频驱动
│   │       ├── microphone.h/cc      # 麦克风输入
│   │       └── speaker.h/cc         # 扬声器输出
│   │
│   ├── ai/                          # 🆕 AI 引擎
│   │   ├── llm_client.h/cc          # LLM API 客户端（多模态）
│   │   ├── asr_client.h/cc          # 语音识别客户端
│   │   └── tts_client.h/cc          # 语音合成客户端
│   │
│   ├── display/                     # 🆕 显示模块
│   │   ├── lcd_driver.h/cc          # LCD 驱动
│   │   ├── ui_manager.h/cc          # UI 管理器 (LVGL)
│   │   └── expressions/             # 表情/动画资源
│   │
│   ├── input/                       # 🆕 输入模块
│   │   ├── button.h/cc              # 按键处理
│   │   └── touch.h/cc               # 触摸输入（可选）
│   │
│   ├── actuator/                    # 🆕 执行器模块
│   │   ├── led_controller.h/cc      # LED 控制
│   │   ├── servo_controller.h/cc    # 舵机控制（预留）
│   │   └── motor_controller.h/cc    # 电机控制（预留）
│   │
│   ├── storage/                     # 存储模块
│   │   ├── flash_storage.h/cc       # Flash 存储
│   │   └── spiffs_manager.h/cc      # SPIFFS 管理
│   │
│   ├── config/                      # 配置模块
│   │   └── config_manager.h/cc      # 配置管理
│   │
│   ├── web/                         # Web 模块
│   │   └── web_server.h/cc          # Web 服务器
│   │
│   └── utils/                       # 工具模块
│       ├── timer.h/cc               # 定时器工具
│       ├── logger.h/cc              # 日志工具
│       └── json_utils.h/cc          # JSON 工具
│
├── components/                      # ESP-IDF 组件
│   ├── esp32-camera/                # ESP32 摄像头组件
│   ├── esp-sr/                      # ESP 语音识别组件
│   └── lvgl/                        # LVGL 图形库
│
├── resources/                       # 资源文件
│   ├── sounds/                      # 音效文件
│   ├── images/                      # 图像资源
│   └── fonts/                       # 字体文件
│
├── partitions.csv                   # 分区表
├── sdkconfig.defaults               # ESP-IDF 默认配置
├── CMakeLists.txt
└── README.md
```

---

## 四、核心模块设计

### 4.1 SessionManager (会话管理器)

```cpp
// main/core/session_manager.h

namespace EvoSpark {

enum class SessionState {
    IDLE,           // 待机状态
    WAKING,         // 唤醒中
    LISTENING,      // 等待输入
    PROCESSING,     // 处理中
    CLOSING         // 关闭中
};

class SessionManager {
public:
    static SessionManager& GetInstance();

    // 初始化
    bool Init();

    // 按键回调（唤醒/结束）
    void OnButtonPress();

    // 用户输入
    void OnUserInput(const std::string& text);
    void OnUserInput(const std::vector<uint8_t>& audio);
    void OnUserInput(const ImageFrame& image);

    // 静默超时回调
    void OnSilenceTimeout();

    // 获取当前状态
    SessionState GetState() const { return state_; }

    // 状态变更回调
    using StateCallback = std::function<void(SessionState, SessionState)>;
    void SetStateCallback(StateCallback cb) { state_callback_ = cb; }

private:
    SessionManager();
    ~SessionManager();

    void StartSession();
    void EndSession();
    void ProcessInput();
    void CompressAndSaveMemory();

    SessionState state_ = SessionState::IDLE;
    TimerHandle_t silence_timer_;
    ConversationBuffer* current_buffer_;
    StateCallback state_callback_;
};

} // namespace EvoSpark
```

### 4.2 MemoryManager v2 (记忆管理器)

```cpp
// main/memory/memory_manager.h (v2)

namespace EvoSpark {

struct CompressedMemory {
    std::string user_profile;           // 用户画像
    std::vector<std::string> key_events; // 关键事件
    std::vector<std::string> preferences; // 偏好
    std::string last_session_summary;   // 上次会话摘要
    std::string raw_json;               // 原始 JSON
};

class MemoryManager {
public:
    static MemoryManager& GetInstance();

    // 初始化
    bool Init(const std::string& api_key);

    // 加载长期记忆
    CompressedMemory LoadMemory();

    // 保存长期记忆
    bool SaveMemory(const CompressedMemory& memory);

    // 压缩记忆（LLM 调用）
    // 输入：旧记忆 + 本次会话对话
    // 输出：新记忆
    CompressedMemory CompressMemory(
        const CompressedMemory& old_memory,
        const std::vector<Message>& session_messages
    );

    // 回滚
    bool RollbackToBackup(int version);

private:
    MemoryManager();

    FlashStorage flash_storage_;
    LLMClient* llm_client_;
    CompressedMemory current_memory_;
};

} // namespace EvoSpark
```

### 4.3 PromptBuilder (Prompt 构建器)

```cpp
// main/memory/prompt_builder.h

namespace EvoSpark {

class PromptBuilder {
public:
    // 构建系统 Prompt（包含长期记忆）
    static std::string BuildSystemPrompt(const CompressedMemory& memory);

    // 构建完整请求（系统 Prompt + 对话历史 + 当前输入）
    static std::vector<Message> BuildRequest(
        const CompressedMemory& memory,
        const std::vector<Message>& history,
        const std::string& user_input
    );

    // 构建多模态请求（带图像）
    static std::vector<Message> BuildMultimodalRequest(
        const CompressedMemory& memory,
        const std::vector<Message>& history,
        const std::string& user_input,
        const std::vector<uint8_t>& image_data
    );
};

} // namespace EvoSpark
```

### 4.4 YOLODetector (物体检测)

```cpp
// main/perception/vision/yolo_detector.h

namespace EvoSpark {

struct DetectedObject {
    std::string label;      // 物体名称
    float confidence;       // 置信度
    int x, y, width, height; // 边界框
};

class YOLODetector {
public:
    static YOLODetector& GetInstance();

    // 初始化（加载模型）
    bool Init(const std::string& model_path);

    // 检测物体
    std::vector<DetectedObject> Detect(const ImageFrame& image);

    // 检测并生成描述（用于 LLM 输入）
    std::string DetectAndDescribe(const ImageFrame& image);

private:
    YOLODetector();

    // ESP-DL 或 TensorFlow Lite Micro 模型
    void* model_;
    bool initialized_ = false;
};

} // namespace EvoSpark
```

---

## 五、数据流

### 5.1 对话流程

```
┌─────────────────────────────────────────────────────────────────┐
│                     Conversation Flow                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. 用户按下按键                                                 │
│     │                                                           │
│     ▼                                                           │
│  2. SessionManager: IDLE → WAKING                               │
│     │                                                           │
│     ├─→ 加载 CompressedMemory                                   │
│     ├─→ 创建 ConversationBuffer                                 │
│     ├─→ 启动静默定时器 (5分钟)                                   │
│     │                                                           │
│     ▼                                                           │
│  3. SessionManager: WAKING → LISTENING                          │
│     │                                                           │
│     │ 用户输入（语音/文字/图像）                                  │
│     ▼                                                           │
│  4. SessionManager: LISTENING → PROCESSING                      │
│     │                                                           │
│     ├─→ [可选] ASR: 音频 → 文字                                  │
│     ├─→ [可选] YOLO: 图像 → 物体描述                             │
│     ├─→ PromptBuilder: 构建 Prompt                              │
│     ├─→ LLMClient: 调用 LLM API                                 │
│     ├─→ [可选] TTS: 文字 → 音频                                  │
│     ├─→ Speaker: 播放音频                                       │
│     ├─→ Display: 显示回复                                       │
│     │                                                           │
│     ▼                                                           │
│  5. SessionManager: PROCESSING → LISTENING                      │
│     │                                                           │
│     │ 重置静默定时器                                             │
│     │                                                           │
│     │ ┌───────────────────────────────────────┐                 │
│     │ │ 循环：LISTENING ↔ PROCESSING           │                 │
│     │ └───────────────────────────────────────┘                 │
│     │                                                           │
│     │ 静默超时 / 按键结束                                        │
│     ▼                                                           │
│  6. SessionManager: LISTENING → CLOSING                         │
│     │                                                           │
│     ├─→ MemoryManager.CompressMemory()                         │
│     │   (旧记忆 + 本次对话 → 新记忆)                             │
│     ├─→ FlashStorage: 保存新记忆                                │
│     ├─→ 清理 ConversationBuffer                                 │
│     │                                                           │
│     ▼                                                           │
│  7. SessionManager: CLOSING → IDLE                              │
│     │                                                           │
│     └─→ 等待下次唤醒                                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 记忆压缩流程

```
┌─────────────────────────────────────────────────────────────────┐
│                    Memory Compression                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  输入:                                                          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ 旧记忆 (compressed_memory.json):                           │ │
│  │ {                                                          │ │
│  │   "user_profile": "用户叫小明，喜欢编程和音乐...",          │ │
│  │   "key_events": [                                          │ │
│  │     "2026-03-15: 讨论了 Python 项目",                      │ │
│  │     "2026-03-17: 帮助解决了 Git 冲突"                       │ │
│  │   ],                                                       │ │
│  │   "preferences": ["喜欢简洁的回答", "偏好中文交流"]         │ │
│  │ }                                                          │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              +                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ 本次对话 (session_buffer):                                 │ │
│  │ [                                                          │ │
│  │   {"role": "user", "content": "帮我写一首关于春天的诗"},    │ │
│  │   {"role": "assistant", "content": "..."},                 │ │
│  │   {"role": "user", "content": "很好，再来一首关于秋天的"},  │ │
│  │   {"role": "assistant", "content": "..."}                  │ │
│  │ ]                                                          │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                  │
│                              ▼                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ LLM 压缩 Prompt:                                           │ │
│  │                                                            │ │
│  │ 你是一个记忆压缩助手。请将用户的旧记忆和本次对话整合，       │ │
│  │ 生成新的压缩记忆。                                          │ │
│  │                                                            │ │
│  │ 要求：                                                      │ │
│  │ 1. 保留用户画像的关键信息                                   │ │
│  │ 2. 更新重要事件列表（保留最近 10 条）                        │ │
│  │ 3. 更新用户偏好（发现新偏好时添加）                          │ │
│  │ 4. 移除过时或重复的信息                                     │ │
│  │                                                            │ │
│  │ 输出格式：JSON                                              │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                  │
│                              ▼                                  │
│  输出:                                                          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ 新记忆 (compressed_memory.json):                           │ │
│  │ {                                                          │ │
│  │   "user_profile": "用户叫小明，喜欢编程、音乐和诗歌...",    │ │
│  │   "key_events": [                                          │ │
│  │     "2026-03-17: 帮助解决了 Git 冲突",                      │ │
│  │     "2026-03-18: 创作了春天和秋天的诗"  ← 新增              │ │
│  │   ],                                                       │ │
│  │   "preferences": [                                         │ │
│  │     "喜欢简洁的回答",                                       │ │
│  │     "偏好中文交流",                                         │ │
│  │     "对诗歌创作感兴趣"  ← 新增                              │ │
│  │   ]                                                        │ │
│  │ }                                                          │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 六、配置参数

```cpp
// main/config/config_params.h

namespace EvoSpark {
namespace Config {

// 会话参数
constexpr int SILENCE_TIMEOUT_MS = 5 * 60 * 1000;  // 5 分钟静默超时
constexpr int MAX_SESSION_MESSAGES = 100;          // 单次会话最大消息数
constexpr int MAX_MEMORY_EVENTS = 20;              // 最大保留事件数

// 音频参数
constexpr int AUDIO_SAMPLE_RATE = 16000;           // 16kHz
constexpr int AUDIO_BITS_PER_SAMPLE = 16;
constexpr int AUDIO_BUFFER_SIZE = 4096;

// 摄像头参数
constexpr int CAMERA_FRAME_SIZE = FRAMESIZE_QVGA;  // 320x240
constexpr int CAMERA_JPEG_QUALITY = 12;

// YOLO 参数
constexpr float YOLO_CONFIDENCE_THRESHOLD = 0.5f;
constexpr int YOLO_NMS_THRESHOLD = 0.45;

// 记忆参数
constexpr int MEMORY_BACKUP_COUNT = 3;             // 保留 3 个备份
constexpr int MAX_MEMORY_SIZE_KB = 50;             // 最大记忆大小 50KB

// API 参数
constexpr const char* GLM_API_URL = "https://open.bigmodel.cn/api/paas/v4/chat/completions";
constexpr const char* GLM_MODEL = "glm-4v-flash";  // 多模态模型

} // namespace Config
} // namespace EvoSpark
```

---

## 七、开发路线

### Phase 1: 会话系统（Week 1-2）
- [ ] 实现 SessionManager
- [ ] 升级 MemoryManager v2
- [ ] 实现 PromptBuilder
- [ ] 按键唤醒/结束逻辑
- [ ] 静默超时检测

### Phase 2: 音频系统（Week 3-4）
- [ ] I2S 驱动（ES8311 + ES7210）
- [ ] 麦克风输入
- [ ] 扬声器输出
- [ ] ASR 集成（可选云端）
- [ ] TTS 集成（可选云端）

### Phase 3: 视觉系统（Week 5-6）
- [ ] OV2640 摄像头驱动
- [ ] YOLO 模型集成
- [ ] 物体检测功能
- [ ] 图像描述生成

### Phase 4: 显示系统（Week 7-8）
- [ ] ST7789 LCD 驱动
- [ ] LVGL 集成
- [ ] UI 界面设计
- [ ] 表情/动画显示

### Phase 5: 整合测试（Week 9-10）
- [ ] 完整流程测试
- [ ] 性能优化
- [ ] Bug 修复
- [ ] 文档完善

---

## 八、技术选型

| 模块 | 技术方案 | 备选 |
|------|----------|------|
| 摄像头 | ESP32-Camera 组件 | - |
| 物体检测 | ESP-DL + YOLO-Tiny | TensorFlow Lite Micro |
| 音频 I/O | ESP-ADF / I2S 直接驱动 | - |
| 语音识别 | 云端 API (GLM-4V) | 本地 ESP-SR |
| 语音合成 | 云端 API | 本地 TTS |
| LCD 显示 | LVGL + ESP-LCD | - |
| LLM | GLM-4V-Flash | GPT-4V / Claude |
| 存储 | SPIFFS | LittleFS |

---

## 九、硬件确认

基于 HARDWARE_DESIGN.md v1.3：

| 组件 | 型号 | GPIO | 状态 |
|------|------|------|------|
| 主控 | ESP32-S3-N16R8 | - | ✅ |
| 摄像头 | OV2640 | 4-18, 21 | ✅ |
| 音频 CODEC | ES8311 | 38, 39 (I2C) + 35-41 (I2S) | ✅ |
| 麦克风 ADC | ES7210 | 38, 39 (I2C) + 35-41 (I2S) | ✅ |
| LCD | ST7789 240x240 | 1-3, 42, 47, 48 (SPI) | ✅ |
| 按键 | Boot 按键 | GPIO0 | ✅ |
| LED | WS2812B | GPIO45 | ✅ |
| 扩展 PWM | - | GPIO24, 25 | 预留 |

---

## 十、下一步

1. ✅ 确认需求（已完成）
2. ✅ 技术设计（本文档）
3. ⏳ 开始 Phase 1 开发
4. ⏳ 创建项目目录结构
5. ⏳ 实现 SessionManager

---

**文档版本**: v1.0
**创建时间**: 2026-03-18
**状态**: 🚧 待开始开发
