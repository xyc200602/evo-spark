#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <string>
#include <vector>
#include <functional>
#include "memory_types.h"

namespace EvoSpark {

// LLM 响应
struct LLMResponse {
    std::string content;
    bool success = false;
    std::string error_message;
    int tokens_used = 0;
};

// LLM 客户端（支持多模态）
class LLMClient {
public:
    static LLMClient& GetInstance() {
        static LLMClient instance;
        return instance;
    }

    // 初始化
    bool Init(const std::string& api_key, const std::string& base_url = "");

    // 发送对话请求
    LLMResponse Chat(const std::vector<Message>& messages);

    // 发送多模态请求（带图像）
    LLMResponse ChatWithImage(const std::vector<Message>& messages,
                              const std::vector<uint8_t>& image_data);

    // 流式响应
    using StreamCallback = std::function<void(const std::string& chunk, bool is_done)>;
    LLMResponse ChatStream(const std::vector<Message>& messages,
                           StreamCallback callback);

    // 压缩记忆
    LLMResponse CompressMemory(const std::string& prompt);

    // 是否已初始化
    bool IsInitialized() const { return initialized_; }

    // 设置模型
    void SetModel(const std::string& model) { model_ = model; }

private:
    LLMClient() = default;
    ~LLMClient() = default;

    // HTTP POST 请求
    bool PostRequest(const std::string& url, const std::string& body,
                     std::string& response);

    // 构建请求 JSON
    std::string BuildRequestJson(const std::vector<Message>& messages);

    // 解析响应 JSON
    bool ParseResponseJson(const std::string& json, LLMResponse& response);

    std::string api_key_;
    std::string base_url_ = "https://open.bigmodel.cn/api/paas/v4";
    std::string model_ = "glm-4-flash";
    bool initialized_ = false;
};

} // namespace EvoSpark

#endif // LLM_CLIENT_H
