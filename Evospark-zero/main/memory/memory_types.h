#ifndef MEMORY_TYPES_H
#define MEMORY_TYPES_H

#include <string>
#include <vector>
#include <chrono>

namespace EvoSpark {

// 记忆类型
enum class MemoryType {
    CONVERSATION,    // 对话记忆
    FACT,           // 事实记忆
    PREFERENCE,      // 偏好记忆
    EVENT           // 事件记忆
};

// 用户偏好项
struct Preference {
    std::string type;
    std::string value;
    double weight;  // 0.0 - 1.0

    std::string to_json() const;
};

// 单个记忆项
struct MemoryItem {
    std::string id;
    MemoryType type;
    std::string summary;
    double importance;  // 0.0 - 1.0
    std::string timestamp;
    std::string context;

    std::string to_json() const;
};

// 最近上下文
struct RecentContext {
    std::string last_topic;
    std::string emotional_state;
    std::string interaction_style;

    std::string to_json() const;
};

// 用户画像
struct UserProfile {
    std::string name;
    std::string age;
    std::string gender;
    std::vector<Preference> preferences;
    std::vector<std::string> traits;

    std::string to_json() const;
};

// 元数据
struct Metadata {
    std::string created_at;
    std::string last_updated;
    int compression_level;
    int total_memories;

    std::string to_json() const;
};

// 完整记忆包
struct MemoryPackage {
    std::string version;
    Metadata metadata;
    UserProfile user_profile;
    std::vector<MemoryItem> memories;
    RecentContext recent_context;
    std::string raw_json;  // 原始 JSON 字符串

    std::string to_json() const;
    bool from_json(const std::string& json);
};

} // namespace EvoSpark

#endif // MEMORY_TYPES_H
