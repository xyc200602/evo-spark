#include "prompt_builder.h"
#include "esp_log.h"
#include "core/session_manager.h"
#include <sstream>

namespace EvoSpark {

static const char* TAG = "PromptBuilder";

std::string PromptBuilder::BuildBasePersona() {
    return R"(你是 EvoSpark，一个友好的 AI 陪伴机器人。
你的特点：
- 温暖、耐心、善于倾听
- 回答简洁明了，避免冗长
- 偶尔会表达自己的情感和想法
- 记得和用户之前的对话内容

你由 ESP32-S3 驱动，具备视觉、听觉和表达能力。)";
}

std::string PromptBuilder::FormatMemory(const CompressedMemory& memory) {
    if (memory.IsEmpty()) {
        return "\n[这是我们的第一次对话，还没有关于你的记忆。]";
    }

    std::ostringstream oss;

    // 用户画像
    if (!memory.user_profile.empty()) {
        oss << "\n【用户画像】\n" << memory.user_profile << "\n";
    }

    // 关键事件
    if (!memory.key_events.empty()) {
        oss << "\n【重要事件】\n";
        for (const auto& event : memory.key_events) {
            oss << "- " << event << "\n";
        }
    }

    // 偏好
    if (!memory.preferences.empty()) {
        oss << "\n【用户偏好】\n";
        for (const auto& pref : memory.preferences) {
            oss << "- " << pref << "\n";
        }
    }

    // 上次会话摘要
    if (!memory.last_session_summary.empty()) {
        oss << "\n【上次对话】\n" << memory.last_session_summary << "\n";
    }

    return oss.str();
}

std::string PromptBuilder::FormatHistory(const std::vector<Message>& messages) {
    if (messages.empty()) {
        return "";
    }

    std::ostringstream oss;

    for (const auto& msg : messages) {
        // 跳过系统消息
        if (msg.role == Role::SYSTEM) {
            continue;
        }

        const char* role_name = (msg.role == Role::USER) ? "用户" : "EvoSpark";
        oss << role_name << ": " << msg.content << "\n";
    }

    return oss.str();
}

std::string PromptBuilder::BuildSystemPrompt(const CompressedMemory& memory) {
    std::ostringstream oss;

    // 基础人设
    oss << BuildBasePersona();

    // 长期记忆
    oss << "\n\n【关于用户的记忆】";
    oss << FormatMemory(memory);

    // 指导
    oss << "\n【互动指南】\n";
    oss << "- 根据记忆中的信息调整你的回答\n";
    oss << "- 如果记忆中有相关事件，可以适当提及\n";
    oss << "- 尊重用户的偏好\n";
    oss << "- 保持自然、亲切的对话风格\n";

    return oss.str();
}

std::vector<Message> PromptBuilder::BuildRequest(
    const CompressedMemory& memory,
    const std::vector<Message>& history,
    const std::string& user_input
) {
    std::vector<Message> request;

    // 系统消息
    request.push_back(Message(Role::SYSTEM, BuildSystemPrompt(memory)));

    // 对话历史（排除第一条系统消息）
    for (size_t i = 1; i < history.size(); i++) {
        if (history[i].role != Role::SYSTEM) {
            request.push_back(history[i]);
        }
    }

    // 当前用户输入
    if (!user_input.empty()) {
        request.push_back(Message(Role::USER, user_input));
    }

    return request;
}

std::string PromptBuilder::BuildCompressionPrompt(
    const CompressedMemory& old_memory,
    const std::vector<Message>& session_messages
) {
    std::ostringstream oss;

    oss << "你是一个记忆压缩助手。请将用户的旧记忆和本次对话整合，生成新的压缩记忆。\n\n";

    oss << "【要求】\n";
    oss << "1. 保留用户画像的关键信息\n";
    oss << "2. 更新重要事件列表（保留最近 20 条）\n";
    oss << "3. 更新用户偏好（发现新偏好时添加，最多 10 条）\n";
    oss << "4. 移除过时或重复的信息\n";
    oss << "5. 生成简短的上次会话摘要（1-2 句话）\n\n";

    oss << "【输出格式】\n";
    oss << "请严格按照以下 JSON 格式输出，不要包含其他内容：\n";
    oss << "{\n";
    oss << "  \"user_profile\": \"用户画像描述\",\n";
    oss << "  \"key_events\": [\"事件1\", \"事件2\", ...],\n";
    oss << "  \"preferences\": [\"偏好1\", \"偏好2\", ...],\n";
    oss << "  \"last_session_summary\": \"上次会话摘要\"\n";
    oss << "}\n\n";

    oss << "【旧记忆】\n";
    oss << FormatMemory(old_memory);
    oss << "\n";

    oss << "【本次对话】\n";
    oss << FormatHistory(session_messages);

    return oss.str();
}

} // namespace EvoSpark
