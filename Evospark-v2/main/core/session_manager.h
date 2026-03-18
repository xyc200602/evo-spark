#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <functional>
#include "memory_types.h"
#include "event_bus.h"
#include "../memory/conversation_buffer.h"
#include "../memory/memory_manager.h"
#include "../memory/prompt_builder.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

namespace EvoSpark {

namespace Config {
    constexpr int SILENCE_TIMEOUT_MS = 5 * 60 * 1000;  // 5 分钟
    constexpr int MAX_SESSION_MESSAGES = 100;
    constexpr int MAX_MEMORY_EVENTS = 20;
}

// 会话管理器 - 核心控制器
class SessionManager {
public:
    static SessionManager& GetInstance() {
        static SessionManager instance;
        return instance;
    }

    // 初始化
    bool Init();

    // 按键回调（唤醒/结束）
    void OnButtonPress();

    // 用户输入
    void OnUserInput(const std::string& text);
    void OnUserInput(const MultimodalInput& input);

    // 静默超时回调
    void OnSilenceTimeout();

    // 获取当前状态
    SessionState GetState() const { return state_; }

    // 状态变更回调
    using StateCallback = std::function<void(SessionState, SessionState)>;
    void SetStateCallback(StateCallback cb) { state_callback_ = cb; }

    // 获取当前会话统计
    const SessionStats& GetStats() const { return stats_; }

    // 是否在会话中
    bool InSession() const {
        return state_ != SessionState::IDLE;
    }

private:
    SessionManager();
    ~SessionManager();

    // 禁止拷贝
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    // 状态转换
    void SetState(SessionState new_state);

    // 会话生命周期
    void StartSession();
    void EndSession();
    void ProcessInput();

    // 记忆压缩和保存
    void CompressAndSaveMemory();

    // 静默定时器
    void StartSilenceTimer();
    void StopSilenceTimer();
    void ResetSilenceTimer();
    static void SilenceTimerCallback(void* arg);

    // 状态
    SessionState state_ = SessionState::IDLE;

    // 定时器
    esp_timer_handle_t silence_timer_ = nullptr;

    // 会话数据
    ConversationBuffer* session_buffer_ = nullptr;
    SessionStats stats_;

    // 回调
    StateCallback state_callback_;

    // 事件总线
    EventBus& event_bus_;

    // 标志
    bool initialized_ = false;
    bool button_press_pending_ = false;
};

} // namespace EvoSpark

#endif // SESSION_MANAGER_H
