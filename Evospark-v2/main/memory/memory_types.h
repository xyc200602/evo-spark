#ifndef MEMORY_TYPES_H
#define MEMORY_TYPES_H

#include <string>
#include <vector>
#include <ctime>

namespace EvoSpark {

// 消息角色
enum class Role {
    SYSTEM,
    USER,
    ASSISTANT
};

// 单条消息
struct Message {
    Role role;
    std::string content;
    std::time_t timestamp;

    Message() : role(Role::USER), timestamp(0) {}
    Message(Role r, const std::string& c)
        : role(r), content(c), timestamp(std::time(nullptr)) {}
};

// 对话角色转字符串
inline const char* RoleToString(Role role) {
    switch (role) {
        case Role::SYSTEM: return "system";
        case Role::USER: return "user";
        case Role::ASSISTANT: return "assistant";
        default: return "user";
    }
}

// 压缩后的记忆结构
struct CompressedMemory {
    std::string user_profile;              // 用户画像
    std::vector<std::string> key_events;   // 关键事件
    std::vector<std::string> preferences;  // 偏好
    std::string last_session_summary;      // 上次会话摘要
    std::time_t last_updated;              // 最后更新时间

    // 元数据
    int version = 1;
    int total_sessions = 0;

    // 原始 JSON（用于存储）
    std::string raw_json;

    CompressedMemory() : last_updated(0), version(1), total_sessions(0) {}

    // 是否为空
    bool IsEmpty() const {
        return user_profile.empty() &&
               key_events.empty() &&
               preferences.empty();
    }
};

// 会话状态
enum class SessionState {
    IDLE,           // 待机状态
    WAKING,         // 唤醒中
    LISTENING,      // 等待输入
    PROCESSING,     // 处理中
    SPEAKING,       // 说话中
    CLOSING         // 关闭中
};

// 会话状态转字符串
inline const char* StateToString(SessionState state) {
    switch (state) {
        case SessionState::IDLE: return "IDLE";
        case SessionState::WAKING: return "WAKING";
        case SessionState::LISTENING: return "LISTENING";
        case SessionState::PROCESSING: return "PROCESSING";
        case SessionState::SPEAKING: return "SPEAKING";
        case SessionState::CLOSING: return "CLOSING";
        default: return "UNKNOWN";
    }
}

// 会话统计
struct SessionStats {
    int message_count = 0;
    int user_messages = 0;
    int assistant_messages = 0;
    std::time_t start_time = 0;
    std::time_t end_time = 0;
    int duration_seconds = 0;
};

// 多模态输入类型
enum class InputType {
    TEXT,
    AUDIO,
    IMAGE
};

// 多模态输入
struct MultimodalInput {
    InputType type;
    std::string text;                       // 文本内容
    std::vector<uint8_t> audio_data;        // 音频数据
    std::vector<uint8_t> image_data;        // 图像数据
    std::string image_description;          // 图像描述（YOLO 检测结果）
};

} // namespace EvoSpark

#endif // MEMORY_TYPES_H
