#include "conversation_buffer.h"
#include "esp_log.h"
#include <algorithm>
#include <cstring>

namespace EvoSpark {

static const char* TAG = "ConvBuffer";

ConversationBuffer::ConversationBuffer(size_t max_size)
    : max_size_(max_size), current_size_(0) {
}

void ConversationBuffer::AddMessage(Role role, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);

    Message msg(role, content);
    size_t msg_size = CalculateSize(msg);

    // 如果超过最大大小，移除最早的消息
    while (current_size_ + msg_size > max_size_ && !messages_.empty()) {
        size_t removed_size = CalculateSize(messages_.front());
        messages_.erase(messages_.begin());
        current_size_ -= removed_size;
        ESP_LOGW(TAG, "Removed oldest message to make space");
    }

    messages_.push_back(msg);
    current_size_ += msg_size;

    ESP_LOGI(TAG, "Added %s message (%zu bytes), total: %zu/%zu bytes",
             RoleToString(role), msg_size, current_size_, max_size_);
}

std::vector<Message> ConversationBuffer::GetMessages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_;
}

std::vector<Message> ConversationBuffer::GetRecentMessages(size_t n) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (n >= messages_.size()) {
        return messages_;
    }

    return std::vector<Message>(
        messages_.end() - n,
        messages_.end()
    );
}

void ConversationBuffer::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
    current_size_ = 0;
}

bool ConversationBuffer::IsEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_.empty();
}

size_t ConversationBuffer::GetCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_.size();
}

size_t ConversationBuffer::GetSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_size_;
}

std::string ConversationBuffer::ToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string json = "[";

    for (size_t i = 0; i < messages_.size(); i++) {
        const Message& msg = messages_[i];

        if (i > 0) json += ",";

        json += "{\"role\":\"" + std::string(RoleToString(msg.role)) + "\"";
        json += ",\"content\":\"";

        // 简单转义（生产环境应使用 JSON 库）
        for (char c : msg.content) {
            switch (c) {
                case '"': json += "\\\""; break;
                case '\\': json += "\\\\"; break;
                case '\n': json += "\\n"; break;
                case '\r': json += "\\r"; break;
                case '\t': json += "\\t"; break;
                default: json += c; break;
            }
        }

        json += "\"}";
    }

    json += "]";
    return json;
}

size_t ConversationBuffer::CalculateSize(const Message& msg) const {
    return msg.content.size() + 32;  // 内容 + 元数据开销
}

} // namespace EvoSpark
