#ifndef CONVERSATION_BUFFER_H
#define CONVERSATION_BUFFER_H

#include <vector>
#include <string>
#include <mutex>
#include "memory_types.h"

namespace EvoSpark {

// 对话缓冲区 - 存储当前会话的对话历史
class ConversationBuffer {
public:
    ConversationBuffer(size_t max_size = 10 * 1024);  // 默认 10KB
    ~ConversationBuffer() = default;

    // 添加消息
    void AddMessage(Role role, const std::string& content);

    // 获取所有消息
    std::vector<Message> GetMessages() const;

    // 获取最近 N 条消息
    std::vector<Message> GetRecentMessages(size_t n) const;

    // 清空缓冲区
    void Clear();

    // 是否为空
    bool IsEmpty() const;

    // 获取消息数量
    size_t GetCount() const;

    // 获取缓冲区大小（字节）
    size_t GetSize() const;

    // 转换为 JSON 字符串
    std::string ToJson() const;

    // 从 JSON 字符串加载
    bool FromJson(const std::string& json);

private:
    std::vector<Message> messages_;
    size_t max_size_;
    size_t current_size_;
    mutable std::mutex mutex_;

    // 计算消息大小
    size_t CalculateSize(const Message& msg) const;
};

} // namespace EvoSpark

#endif // CONVERSATION_BUFFER_H
