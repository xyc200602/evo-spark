#include "glm_client.h"
#include "../memory/memory_types.h"
#include <cstring>
#include "esp_log.h"

namespace EvoSpark {

static const char* TAG = "GLMClient";

GLMClient::GLMClient()
    : api_url_("http://open.bigmodel.cn/api/paas/v4/chat/completions"),  // 使用 HTTP 绕过 TLS 问题（测试用）
      is_initialized_(false) {
}

GLMClient::~GLMClient() {
}

bool GLMClient::Init(const std::string& api_key) {
    if (api_key.empty()) {
        ESP_LOGE(TAG, "API key is empty");
        return false;
    }

    api_key_ = api_key;
    is_initialized_ = true;
    ESP_LOGI(TAG, "GLM client initialized");
    return true;
}

struct ResponseData {
    std::string body;
    size_t received = 0;
};

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    ResponseData* data = static_cast<ResponseData*>(evt->user_data);

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            data->body.append((char*)evt->data, evt->data_len);
            data->received += evt->data_len;
            break;

        default:
            break;
    }

    return ESP_OK;
}

bool GLMClient::Chat(const std::string& message, std::string& response) {
    if (!is_initialized_) {
        ESP_LOGE(TAG, "GLM client not initialized");
        return false;
    }

    ESP_LOGI(TAG, "Sending message to GLM: %d bytes", message.length());

    // 构造 JSON 请求体
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return false;
    }

    if (!cJSON_AddStringToObject(root, "model", "glm-4.7-flash")) {
        ESP_LOGE(TAG, "Failed to add model to JSON");
        cJSON_Delete(root);
        return false;
    }

    cJSON *messages = cJSON_CreateArray();
    if (!messages) {
        ESP_LOGE(TAG, "Failed to create messages array");
        cJSON_Delete(root);
        return false;
    }

    cJSON *msg = cJSON_CreateObject();
    if (!msg) {
        ESP_LOGE(TAG, "Failed to create message object");
        cJSON_Delete(messages);
        cJSON_Delete(root);
        return false;
    }

    if (!cJSON_AddStringToObject(msg, "role", "user") ||
        !cJSON_AddStringToObject(msg, "content", message.c_str())) {
        ESP_LOGE(TAG, "Failed to add message fields");
        cJSON_Delete(msg);
        cJSON_Delete(messages);
        cJSON_Delete(root);
        return false;
    }

    cJSON_AddItemToArray(messages, msg);
    cJSON_AddItemToObject(root, "messages", messages);

    // 直接添加参数到根级别（根据 GLM API 文档）
    cJSON_AddNumberToObject(root, "max_tokens", max_tokens_);
    cJSON_AddNumberToObject(root, "temperature", temperature_);

    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(root);
        return false;
    }

    std::string request_body(json_str);
    cJSON_Delete(root);
    free(json_str);

    // 配置 HTTP 客户端
    ResponseData response_data{};

    esp_http_client_config_t config = {};
    config.url = api_url_.c_str();
    config.method = HTTP_METHOD_POST;
    config.event_handler = http_event_handler;
    config.user_data = &response_data;
    config.timeout_ms = 60000;  // 60秒超时（处理慢速网络）
    config.buffer_size = 32768;  // 增大缓冲区以处理大响应
    config.use_global_ca_store = true;  // 使用全局 CA 证书存储
    config.skip_cert_common_name_check = true;  // 跳过证书 CN 检查
    config.disable_auto_redirect = true;  // 禁用自动重定向
    config.keep_alive_enable = false;  // 禁用 keep-alive，确保连接正确关闭

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        return false;
    }

    // 设置请求头
    std::string auth_header = "Bearer " + api_key_;
    esp_http_client_set_header(client, "Authorization", auth_header.c_str());
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // 发送请求
    esp_http_client_set_post_field(client, request_body.c_str(), request_body.length());

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "HTTP status: %d, response size: %d bytes",
             status_code, response_data.body.length());

    if (status_code != 200) {
        ESP_LOGE(TAG, "API returned non-200 status: %d", status_code);
        ESP_LOGE(TAG, "Response: %s", response_data.body.c_str());
        return false;
    }

    // 解析响应
    cJSON *response_json = cJSON_Parse(response_data.body.c_str());
    if (!response_json) {
        ESP_LOGE(TAG, "Failed to parse response JSON");
        return false;
    }

    cJSON *choices = cJSON_GetObjectItem(response_json, "choices");
    if (!choices || !cJSON_IsArray(choices)) {
        ESP_LOGE(TAG, "Invalid response format: no choices");
        cJSON_Delete(response_json);
        return false;
    }

    cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
    if (!first_choice) {
        ESP_LOGE(TAG, "Invalid response: choices array is empty");
        cJSON_Delete(response_json);
        return false;
    }

    cJSON *message_obj = cJSON_GetObjectItem(first_choice, "message");
    if (!message_obj) {
        ESP_LOGE(TAG, "Invalid response: no message in choice");
        cJSON_Delete(response_json);
        return false;
    }

    cJSON *content = cJSON_GetObjectItem(message_obj, "content");

    if (!content || !cJSON_IsString(content)) {
        ESP_LOGE(TAG, "Invalid message content");
        cJSON_Delete(response_json);
        return false;
    }

    response = content->valuestring;
    cJSON_Delete(response_json);

    ESP_LOGI(TAG, "GLM response received: %d bytes", response.length());
    return true;
}

} // namespace EvoSpark
