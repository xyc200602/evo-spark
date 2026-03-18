#include "session_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <chrono>

namespace EvoSpark {

static const char* TAG = "SessionManager";

SessionManager::SessionManager()
    : event_bus_(EventBus::GetInstance()) {
}

SessionManager::~SessionManager() {
    if (silence_timer_) {
        esp_timer_delete(silence_timer_);
    }
    if (session_buffer_) {
        delete session_buffer_;
    }
}

bool SessionManager::Init() {
    if (initialized_) {
        return true;
    }

    ESP_LOGI(TAG, "Initializing SessionManager...");

    // 创建静默定时器
    esp_timer_create_args_t timer_args = {
        .callback = &SilenceTimerCallback,
        .arg = this,
        .name = "silence_timer"
    };

    esp_err_t ret = esp_timer_create(&timer_args, &silence_timer_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create silence timer: %s", esp_err_to_name(ret));
        return false;
    }

    // 订阅事件
    event_bus_.Subscribe(EventType::BUTTON_PRESS, [this](const Event& e) {
        OnButtonPress();
    });

    event_bus_.Subscribe(EventType::USER_INPUT, [this](const Event& e) {
        if (e.input.type == InputType::TEXT && !e.input.text.empty()) {
            OnUserInput(e.input.text);
        } else {
            OnUserInput(e.input);
        }
    });

    initialized_ = true;
    ESP_LOGI(TAG, "SessionManager initialized");
    return true;
}

void SessionManager::OnButtonPress() {
    ESP_LOGI(TAG, "Button pressed, current state: %s", StateToString(state_));

    if (state_ == SessionState::IDLE) {
        // 唤醒
        StartSession();
    } else {
        // 结束会话
        EndSession();
    }
}

void SessionManager::StartSession() {
    ESP_LOGI(TAG, "Starting session...");

    SetState(SessionState::WAKING);

    // 初始化会话缓冲区
    if (session_buffer_) {
        delete session_buffer_;
    }
    session_buffer_ = new ConversationBuffer();

    // 初始化统计
    stats_ = SessionStats();
    stats_.start_time = std::time(nullptr);

    // 加载长期记忆
    MemoryManager& memory_mgr = MemoryManager::GetInstance();
    CompressedMemory memory = memory_mgr.LoadMemory();

    // 添加系统消息（包含长期记忆）
    std::string system_prompt = PromptBuilder::BuildSystemPrompt(memory);
    session_buffer_->AddMessage(Role::SYSTEM, system_prompt);

    // 启动静默定时器
    StartSilenceTimer();

    // 转入监听状态
    SetState(SessionState::LISTENING);

    // 发布事件
    Event event(EventType::SESSION_START, "SessionManager");
    event.old_state = SessionState::IDLE;
    event.new_state = SessionState::LISTENING;
    event_bus_.Publish(event);

    ESP_LOGI(TAG, "Session started");
}

void SessionManager::EndSession() {
    ESP_LOGI(TAG, "Ending session...");

    SetState(SessionState::CLOSING);

    // 停止静默定时器
    StopSilenceTimer();

    // 更新统计
    stats_.end_time = std::time(nullptr);
    stats_.duration_seconds = stats_.end_time - stats_.start_time;

    // 压缩并保存记忆
    CompressAndSaveMemory();

    // 清理会话缓冲区
    if (session_buffer_) {
        delete session_buffer_;
        session_buffer_ = nullptr;
    }

    // 返回待机状态
    SetState(SessionState::IDLE);

    // 发布事件
    Event event(EventType::SESSION_END, "SessionManager");
    event.old_state = SessionState::CLOSING;
    event.new_state = SessionState::IDLE;
    event_bus_.Publish(event);

    ESP_LOGI(TAG, "Session ended. Duration: %d seconds, Messages: %d",
             stats_.duration_seconds, stats_.message_count);
}

void SessionManager::OnUserInput(const std::string& text) {
    if (state_ != SessionState::LISTENING) {
        ESP_LOGW(TAG, "Ignoring input, not in LISTENING state");
        return;
    }

    if (text.empty()) {
        return;
    }

    ESP_LOGI(TAG, "User input: %s", text.c_str());

    // 添加用户消息到缓冲区
    session_buffer_->AddMessage(Role::USER, text);
    stats_.message_count++;
    stats_.user_messages++;

    // 处理输入
    ProcessInput();

    // 重置静默定时器
    ResetSilenceTimer();
}

void SessionManager::OnUserInput(const MultimodalInput& input) {
    if (state_ != SessionState::LISTENING) {
        ESP_LOGW(TAG, "Ignoring input, not in LISTENING state");
        return;
    }

    // 处理不同类型的输入
    std::string content = input.text;

    if (input.type == InputType::IMAGE && !input.image_description.empty()) {
        content = "[图像: " + input.image_description + "] " + input.text;
    }

    if (!content.empty()) {
        OnUserInput(content);
    }
}

void SessionManager::ProcessInput() {
    SetState(SessionState::PROCESSING);

    ESP_LOGI(TAG, "Processing input...");

    // TODO: 调用 LLM API
    // 1. 构建完整请求（系统 Prompt + 对话历史 + 当前输入）
    // 2. 调用 LLM API
    // 3. 处理响应
    // 4. 调用 TTS（可选）
    // 5. 显示回复

    // 模拟响应
    std::string response = "我收到了你的消息。";

    // 添加助手消息到缓冲区
    session_buffer_->AddMessage(Role::ASSISTANT, response);
    stats_.assistant_messages++;

    // 返回监听状态
    SetState(SessionState::LISTENING);
}

void SessionManager::CompressAndSaveMemory() {
    ESP_LOGI(TAG, "Compressing and saving memory...");

    if (!session_buffer_ || session_buffer_->IsEmpty()) {
        ESP_LOGI(TAG, "Session buffer is empty, skip compression");
        return;
    }

    MemoryManager& memory_mgr = MemoryManager::GetInstance();

    // 获取本次会话的消息
    std::vector<Message> session_messages = session_buffer_->GetMessages();

    // 压缩记忆
    CompressedMemory new_memory = memory_mgr.CompressMemory(
        memory_mgr.LoadMemory(),
        session_messages
    );

    // 保存记忆
    if (!memory_mgr.SaveMemory(new_memory)) {
        ESP_LOGE(TAG, "Failed to save memory");
    } else {
        ESP_LOGI(TAG, "Memory saved successfully");
    }
}

void SessionManager::OnSilenceTimeout() {
    ESP_LOGI(TAG, "Silence timeout detected");

    if (state_ == SessionState::IDLE || state_ == SessionState::CLOSING) {
        return;
    }

    // 发布事件
    event_bus_.Publish(EventType::SILENCE_DETECTED, "SessionManager");

    // 结束会话
    EndSession();
}

void SessionManager::SetState(SessionState new_state) {
    if (state_ == new_state) {
        return;
    }

    SessionState old_state = state_;
    state_ = new_state;

    ESP_LOGI(TAG, "State changed: %s -> %s",
             StateToString(old_state), StateToString(new_state));

    // 发布状态变更事件
    Event event(EventType::STATE_CHANGE, "SessionManager");
    event.old_state = old_state;
    event.new_state = new_state;
    event_bus_.Publish(event);

    // 调用回调
    if (state_callback_) {
        state_callback_(old_state, new_state);
    }
}

void SessionManager::StartSilenceTimer() {
    if (silence_timer_) {
        esp_timer_start_once(silence_timer_, Config::SILENCE_TIMEOUT_MS * 1000);
    }
}

void SessionManager::StopSilenceTimer() {
    if (silence_timer_) {
        esp_timer_stop(silence_timer_);
    }
}

void SessionManager::ResetSilenceTimer() {
    StopSilenceTimer();
    StartSilenceTimer();
}

void SessionManager::SilenceTimerCallback(void* arg) {
    SessionManager* self = static_cast<SessionManager*>(arg);
    self->OnSilenceTimeout();
}

} // namespace EvoSpark
