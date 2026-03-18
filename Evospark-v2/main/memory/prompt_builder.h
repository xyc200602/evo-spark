#ifndef PROMPT_BUILDER_H
#define PROMPT_BUILDER_H

#include <string>
#include <vector>
#include "memory_types.h"

namespace EvoSpark {

// Prompt 构建器 - 构建发送给 LLM 的完整 Prompt
class PromptBuilder {
public:
    // 构建系统 Prompt（包含长期记忆）
    static std::string BuildSystemPrompt(const CompressedMemory& memory);

    // 构建完整请求（系统 Prompt + 对话历史 + 当前输入）
    static std::vector<Message> BuildRequest(
        const CompressedMemory& memory,
        const std::vector<Message>& history,
        const std::string& user_input
    );

    // 构建记忆压缩 Prompt
    static std::string BuildCompressionPrompt(
        const CompressedMemory& old_memory,
        const std::vector<Message>& session_messages
    );

    // 构建基础人设
    static std::string BuildBasePersona();

private:
    // 格式化记忆为可读文本
    static std::string FormatMemory(const CompressedMemory& memory);

    // 格式化对话历史为可读文本
    static std::string FormatHistory(const std::vector<Message>& messages);
};

} // namespace EvoSpark

#endif // PROMPT_BUILDER_H
