#ifndef CONVERSATION_BUFFER_H
#define CONVERSATION_BUFFER_H

#include <string>
#include <vector>
#include <deque>
#include <esp_log.h>

namespace EvoSpark {

// 单条对话消息
struct Message {
    std::string role;      // "user" 或 "assistant"
    std::string content;
    std::string timestamp;

    std::string to_json() const {
        return "{\"role\":\"" + role + "\",\"content\":\"" + content + "\"}";
    }
};

// 对话缓冲区
class ConversationBuffer {
public:
    static const size_t MAX_MESSAGES = 20;      // 最多保留 20 条消息
    static const size_t MAX_SIZE_BYTES = 10240;   // 最大 10 KB

    ConversationBuffer();
    ~ConversationBuffer();

    void Init();

    // 添加消息
    void Add(const std::string& role, const std::string& content);

    // 获取所有消息的 JSON 格式
    std::string GetAsString() const;

    // 获取最近 N 条消息
    std::string GetLastN(size_t n) const;

    // 清空缓冲区
    void Clear();

    // 判断是否为空
    bool IsEmpty() const;

    // 获取消息数量
    size_t GetCount() const;

    // 获取当前大小（字节）
    size_t GetSize() const;

private:
    std::deque<Message> messages_;
    size_t current_size_;

    void TrimIfNeeded();  // 超出限制时自动裁剪
};

} // namespace EvoSpark

#endif // CONVERSATION_BUFFER_H
