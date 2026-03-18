#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <functional>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include "memory_types.h"

namespace EvoSpark {

// 事件类型
enum class EventType {
    // 会话事件
    SESSION_START,
    SESSION_END,
    SESSION_TIMEOUT,

    // 输入事件
    BUTTON_PRESS,
    USER_INPUT,
    SILENCE_DETECTED,

    // 状态变更
    STATE_CHANGE,

    // AI 事件
    AI_RESPONSE_START,
    AI_RESPONSE_CHUNK,
    AI_RESPONSE_END,
    AI_ERROR,

    // 记忆事件
    MEMORY_UPDATE,
    MEMORY_COMPRESS,

    // 感知事件
    OBJECT_DETECTED,
    FACE_DETECTED,

    // 错误事件
    ERROR_OCCURRED
};

// 事件数据
struct Event {
    EventType type;
    std::string source;
    std::time_t timestamp;

    // 可选数据
    std::string str_data;
    int int_data = 0;
    SessionState old_state = SessionState::IDLE;
    SessionState new_state = SessionState::IDLE;
    Message message;
    MultimodalInput input;

    Event(EventType t, const std::string& src = "")
        : type(t), source(src), timestamp(std::time(nullptr)) {}
};

// 事件回调类型
using EventCallback = std::function<void(const Event&)>;

// 事件总线（发布-订阅模式）
class EventBus {
public:
    static EventBus& GetInstance() {
        static EventBus instance;
        return instance;
    }

    // 订阅事件
    void Subscribe(EventType type, EventCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribers_[type].push_back(callback);
    }

    // 发布事件
    void Publish(const Event& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscribers_.find(event.type);
        if (it != subscribers_.end()) {
            for (auto& callback : it->second) {
                callback(event);
            }
        }
    }

    // 发布简单事件
    void Publish(EventType type, const std::string& source = "") {
        Event event(type, source);
        Publish(event);
    }

private:
    EventBus() = default;
    ~EventBus() = default;

    std::map<EventType, std::vector<EventCallback>> subscribers_;
    std::mutex mutex_;
};

} // namespace EvoSpark

#endif // EVENT_BUS_H
