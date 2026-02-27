#include "conversation_buffer.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace EvoSpark {

static const char* TAG = "ConvBuffer";

ConversationBuffer::ConversationBuffer() : current_size_(0) {
}

ConversationBuffer::~ConversationBuffer() {
}

void ConversationBuffer::Init() {
    messages_.clear();
    current_size_ = 0;
    ESP_LOGI(TAG, "Conversation buffer initialized");
}

void ConversationBuffer::Add(const std::string& role, const std::string& content) {
    // 获取当前时间戳
    auto now = std::chrono::system_clock::now();
    auto time_info = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_info), "%Y-%m-%dT%H:%M:%SZ");

    // 创建消息
    Message msg;
    msg.role = role;
    msg.content = content;
    msg.timestamp = ss.str();

    // 添加到缓冲区
    messages_.push_back(msg);
    current_size_ += role.length() + content.length() + 20;  // 估算大小

    ESP_LOGD(TAG, "Added message: role=%s, size=%d, total=%d/%d",
             role.c_str(), content.length(), current_size_, MAX_SIZE_BYTES);

    // 检查是否需要裁剪
    TrimIfNeeded();
}

std::string ConversationBuffer::GetAsString() const {
    if (messages_.empty()) {
        return "";
    }

    std::stringstream ss;
    ss << "对话记录：\n\n";

    for (size_t i = 0; i < messages_.size(); i++) {
        const Message& msg = messages_[i];
        std::string role_name = (msg.role == "user") ? "用户" : "助手";
        ss << role_name << ": " << msg.content << "\n";
    }

    return ss.str();
}

std::string ConversationBuffer::GetLastN(size_t n) const {
    std::stringstream ss;
    ss << "最近 " << n << " 条对话：\n\n";

    size_t start = (n > messages_.size()) ? 0 : messages_.size() - n;

    for (size_t i = start; i < messages_.size(); i++) {
        const Message& msg = messages_[i];
        std::string role_name = (msg.role == "user") ? "用户" : "助手";
        ss << role_name << ": " << msg.content << "\n";
    }

    return ss.str();
}

void ConversationBuffer::Clear() {
    messages_.clear();
    current_size_ = 0;
    ESP_LOGI(TAG, "Conversation buffer cleared");
}

bool ConversationBuffer::IsEmpty() const {
    return messages_.empty();
}

size_t ConversationBuffer::GetCount() const {
    return messages_.size();
}

size_t ConversationBuffer::GetSize() const {
    return current_size_;
}

void ConversationBuffer::TrimIfNeeded() {
    bool trimmed = false;

    // 1. 检查消息数量
    while (messages_.size() > MAX_MESSAGES) {
        current_size_ -= messages_.front().role.length() +
                       messages_.front().content.length() + 20;
        messages_.pop_front();
        trimmed = true;
    }

    // 2. 检查字节大小
    while (current_size_ > MAX_SIZE_BYTES && !messages_.empty()) {
        current_size_ -= messages_.front().role.length() +
                       messages_.front().content.length() + 20;
        messages_.pop_front();
        trimmed = true;
    }

    if (trimmed) {
        ESP_LOGW(TAG, "Buffer trimmed: %d messages, %d bytes",
                  messages_.size(), current_size_);
    }
}

} // namespace EvoSpark
