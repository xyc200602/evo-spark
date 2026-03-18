#include "llm_client.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include <sstream>
#include <cstring>

namespace EvoSpark {

static const char* TAG = "LLMClient";

bool LLMClient::Init(const std::string& api_key, const std::string& base_url) {
    if (api_key.empty()) {
        ESP_LOGE(TAG, "API key is empty");
        return false;
    }

    api_key_ = api_key;
    if (!base_url.empty()) {
        base_url_ = base_url;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "LLM client initialized (model: %s)", model_.c_str());
    return true;
}

LLMResponse LLMClient::Chat(const std::vector<Message>& messages) {
    LLMResponse response;

    if (!initialized_) {
        response.error_message = "LLM client not initialized";
        return response;
    }

    // 构建请求
    std::string body = BuildRequestJson(messages);
    std::string url = base_url_ + "/chat/completions";

    // 发送请求
    std::string resp_str;
    if (!PostRequest(url, body, resp_str)) {
        response.error_message = "HTTP request failed";
        return response;
    }

    // 解析响应
    if (ParseResponseJson(resp_str, response)) {
        response.success = true;
    }

    return response;
}

LLMResponse LLMClient::ChatWithImage(const std::vector<Message>& messages,
                                     const std::vector<uint8_t>& image_data) {
    // TODO: 实现多模态请求
    // 1. 将图像编码为 base64
    // 2. 构建包含图像的消息
    // 3. 调用多模态模型

    LLMResponse response;
    response.error_message = "Multimodal not implemented";
    return response;
}

LLMResponse LLMClient::ChatStream(const std::vector<Message>& messages,
                                  StreamCallback callback) {
    // TODO: 实现流式响应
    // 使用 Server-Sent Events (SSE)

    LLMResponse response;
    response.error_message = "Streaming not implemented";
    return response;
}

LLMResponse LLMClient::CompressMemory(const std::string& prompt) {
    LLMResponse response;

    if (!initialized_) {
        response.error_message = "LLM client not initialized";
        return response;
    }

    // 构建压缩请求
    std::vector<Message> messages;
    messages.push_back(Message(Role::USER, prompt));

    response = Chat(messages);

    // 如果成功，提取 JSON 内容
    if (response.success) {
        // 尝试提取 JSON 块
        size_t start = response.content.find('{');
        size_t end = response.content.rfind('}');
        if (start != std::string::npos && end != std::string::npos) {
            response.content = response.content.substr(start, end - start + 1);
        }
    }

    return response;
}

std::string LLMClient::BuildRequestJson(const std::vector<Message>& messages) {
    std::ostringstream oss;

    oss << "{";
    oss << "\"model\":\"" << model_ << "\",";
    oss << "\"messages\":[";

    for (size_t i = 0; i < messages.size(); i++) {
        const Message& msg = messages[i];
        if (i > 0) oss << ",";

        oss << "{\"role\":\"" << RoleToString(msg.role) << "\"";
        oss << ",\"content\":\"";

        // 转义 JSON
        for (char c : msg.content) {
            switch (c) {
                case '"': oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default: oss << c; break;
            }
        }

        oss << "\"}";
    }

    oss << "],";
    oss << "\"temperature\":0.7,";
    oss << "\"max_tokens\":2000";
    oss << "}";

    return oss.str();
}

bool LLMClient::ParseResponseJson(const std::string& json, LLMResponse& response) {
    // 简单 JSON 解析（生产环境应使用 cJSON 或 ArduinoJson）

    // 查找 content 字段
    std::string content_key = "\"content\":\"";
    size_t start = json.find(content_key);
    if (start == std::string::npos) {
        ESP_LOGE(TAG, "Failed to find content in response");
        return false;
    }

    start += content_key.length();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) {
        return false;
    }

    response.content = json.substr(start, end - start);

    // 反转义
    // TODO: 实现 JSON 反转义

    // 查找 usage 字段
    std::string usage_key = "\"total_tokens\":";
    size_t usage_pos = json.find(usage_key);
    if (usage_pos != std::string::npos) {
        size_t num_start = usage_pos + usage_key.length();
        response.tokens_used = std::stoi(json.substr(num_start));
    }

    return true;
}

bool LLMClient::PostRequest(const std::string& url, const std::string& body,
                            std::string& response) {
    ESP_LOGI(TAG, "POST %s", url.c_str());
    ESP_LOGD(TAG, "Body: %s", body.c_str());

    esp_http_client_config_t config = {
        .url = url.c_str(),
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        return false;
    }

    // 设置请求头
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", 
                               ("Bearer " + api_key_).c_str());

    // 设置请求体
    esp_http_client_set_post_field(client, body.c_str(), body.length());

    // 发送请求
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    // 获取状态码
    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        ESP_LOGE(TAG, "HTTP status: %d", status);
        esp_http_client_cleanup(client);
        return false;
    }

    // 读取响应
    int content_length = esp_http_client_get_content_length(client);
    if (content_length > 0) {
        response.resize(content_length);
        int read = esp_http_client_read(client, &response[0], content_length);
        if (read != content_length) {
            ESP_LOGW(TAG, "Read %d bytes, expected %d", read, content_length);
        }
    }

    esp_http_client_cleanup(client);

    ESP_LOGD(TAG, "Response: %s", response.c_str());
    return true;
}

} // namespace EvoSpark
