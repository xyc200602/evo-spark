#include "memory_types.h"
#include <sstream>
#include <iomanip>
#include <random>

namespace EvoSpark {

// 辅助函数：生成唯一 ID
static std::string generate_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 999999);

    std::stringstream ss;
    ss << "mem_" << std::setfill('0') << std::setw(6) << dis(gen);
    return ss.str();
}

// 辅助函数：获取当前时间戳
static std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// 辅助函数：转义 JSON 字符串
static std::string escape_json(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b";  break;
            case '\f': result += "\\f";  break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;         break;
        }
    }
    return result;
}

// Preference 实现
std::string Preference::to_json() const {
    std::stringstream ss;
    ss << "{"
       << "\"type\":\"" << escape_json(type) << "\","
       << "\"value\":\"" << escape_json(value) << "\","
       << "\"weight\":" << weight
       << "}";
    return ss.str();
}

// MemoryItem 实现
std::string MemoryItem::to_json() const {
    std::string type_str;
    switch (type) {
        case MemoryType::CONVERSATION:
            type_str = "conversation";
            break;
        case MemoryType::FACT:
            type_str = "fact";
            break;
        case MemoryType::PREFERENCE:
            type_str = "preference";
            break;
        case MemoryType::EVENT:
            type_str = "event";
            break;
    }

    std::stringstream ss;
    ss << "{"
       << "\"id\":\"" << escape_json(id) << "\","
       << "\"type\":\"" << type_str << "\","
       << "\"summary\":\"" << escape_json(summary) << "\","
       << "\"importance\":" << importance << ","
       << "\"timestamp\":\"" << escape_json(timestamp) << "\","
       << "\"context\":\"" << escape_json(context) << "\""
       << "}";
    return ss.str();
}

// RecentContext 实现
std::string RecentContext::to_json() const {
    std::stringstream ss;
    ss << "{"
       << "\"last_topic\":\"" << escape_json(last_topic) << "\","
       << "\"emotional_state\":\"" << escape_json(emotional_state) << "\","
       << "\"interaction_style\":\"" << escape_json(interaction_style) << "\""
       << "}";
    return ss.str();
}

// UserProfile 实现
std::string UserProfile::to_json() const {
    std::stringstream ss;
    ss << "{"
       << "\"name\":\"" << escape_json(name) << "\","
       << "\"age\":" << (age.empty() ? "null" : "\"" + escape_json(age) + "\"") << ","
       << "\"gender\":" << (gender.empty() ? "null" : "\"" + escape_json(gender) + "\"") << ","
       << "\"preferences\":[";

    for (size_t i = 0; i < preferences.size(); i++) {
        if (i > 0) ss << ",";
        ss << preferences[i].to_json();
    }

    ss << "],"
       << "\"traits\":[";

    for (size_t i = 0; i < traits.size(); i++) {
        if (i > 0) ss << ",";
        ss << "\"" << escape_json(traits[i]) << "\"";
    }

    ss << "]}";
    return ss.str();
}

// Metadata 实现
std::string Metadata::to_json() const {
    std::stringstream ss;
    ss << "{"
       << "\"created_at\":\"" << escape_json(created_at) << "\","
       << "\"last_updated\":\"" << escape_json(last_updated) << "\","
       << "\"compression_level\":" << compression_level << ","
       << "\"total_memories\":" << total_memories
       << "}";
    return ss.str();
}

// MemoryPackage 实现
std::string MemoryPackage::to_json() const {
    std::stringstream ss;
    ss << "{"
       << "\"version\":\"" << escape_json(version) << "\","
       << "\"metadata\":" << metadata.to_json() << ","
       << "\"user_profile\":" << user_profile.to_json() << ","
       << "\"memories\":[";

    for (size_t i = 0; i < memories.size(); i++) {
        if (i > 0) ss << ",";
        ss << memories[i].to_json();
    }

    ss << "],"
       << "\"recent_context\":" << recent_context.to_json()
       << "}";
    return ss.str();
}

} // namespace EvoSpark
