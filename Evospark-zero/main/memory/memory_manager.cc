#include "memory_manager.h"
#include <cstring>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "esp_log.h"
#include "esp_timer.h"

namespace EvoSpark {

static const char* TAG = "MemoryManager";
static const size_t MAX_MEMORY_SIZE = 10240;  // 10 KB 上限
static const int COMPRESSION_QUEUE_SIZE = 5;  // 队列大小

MemoryManager::MemoryManager() : glm_client_(nullptr),
                              compression_queue_(nullptr),
                              compression_task_(nullptr) {
}

MemoryManager::~MemoryManager() {
    if (compression_task_ != nullptr) {
        vTaskDelete(compression_task_);
    }
    if (compression_queue_ != nullptr) {
        vQueueDelete(compression_queue_);
    }
}

bool MemoryManager::Init(const std::string& api_key) {
    // 初始化 Flash 存储
    if (!flash_storage_.Init()) {
        ESP_LOGE(TAG, "Failed to initialize flash storage");
        return false;
    }

    // 初始化对话缓冲区
    conversation_buffer_.Init();

    // 初始化 GLM 客户端
    glm_client_ = &GLMClient::GetInstance();
    if (!glm_client_->Init(api_key)) {
        ESP_LOGE(TAG, "Failed to initialize GLM client");
        return false;
    }

    // 创建压缩队列
    compression_queue_ = xQueueCreate(COMPRESSION_QUEUE_SIZE, sizeof(CompressionTask));
    if (compression_queue_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create compression queue");
        return false;
    }

    // 创建压缩任务
    BaseType_t ret = xTaskCreate(
        CompressionTaskWrapper,
        "compression_task",
        16384,  // 16KB 堆栈
        this,
        5,      // 优先级
        &compression_task_
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create compression task");
        vQueueDelete(compression_queue_);
        compression_queue_ = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "Memory manager initialized");
    is_initialized_ = true;
    return true;
}

void MemoryManager::AddConversation(const std::string& role,
                                  const std::string& content) {
    if (!is_initialized_ || compression_queue_ == nullptr) {
        return;
    }

    // 将字符串复制到固定长度数组
    CompressionTask task;
    memset(&task, 0, sizeof(task));

    snprintf(task.role, MAX_ROLE_LEN, "%s", role.c_str());
    snprintf(task.content, MAX_CONTENT_LEN, "%s", content.c_str());

    // 将任务放入队列
    if (xQueueSend(compression_queue_, &task, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Compression queue full, task dropped");
    }
}

// 静态任务函数
void MemoryManager::CompressionTaskWrapper(void* arg) {
    MemoryManager* manager = static_cast<MemoryManager*>(arg);
    manager->ProcessCompressionTask();
}

void MemoryManager::ProcessCompressionTask() {
    ESP_LOGI(TAG, "Compression task started");

    CompressionTask task;
    while (true) {
        // 从队列获取任务
        if (xQueueReceive(compression_queue_, &task, portMAX_DELAY) == pdTRUE) {
            // 添加到buffer
            std::string role_str(task.role);
            std::string content_str(task.content);
            conversation_buffer_.Add(role_str, content_str);

            // 只在缓冲区达到 10 条对话时才压缩记忆
            // 这样可以避免每次消息都触发 GLM API 调用
            if (conversation_buffer_.GetCount() >= 10) {
                UpdateMemory();
            }

            // 给其他任务让出 CPU
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void MemoryManager::OnTimerCallback() {
    // 静默定时器已移除
}

bool MemoryManager::UpdateMemory() {
    // 1. 读取旧记忆
    MemoryPackage old_memory = flash_storage_.ReadMemory();

    // 2. 获取新对话
    std::string new_conversations = conversation_buffer_.GetAsString();

    ESP_LOGI(TAG, "Compressing memory: old_size=%d, new_size=%d",
             old_memory.raw_json.length(), new_conversations.length());

    // 3. 调用 GLM API 压缩记忆
    MemoryPackage new_memory;
    if (!CompressMemory(old_memory, new_conversations, new_memory)) {
        ESP_LOGE(TAG, "Failed to compress memory");
        return false;
    }

    // 4. 验证大小
    if (!ValidateMemorySize(new_memory.raw_json)) {
        ESP_LOGE(TAG, "Compressed memory too large: %d bytes (max: %d)",
                 new_memory.raw_json.length(), MAX_MEMORY_SIZE);
        return false;
    }

    // 5. 写入 Flash
    if (!flash_storage_.WriteMemory(new_memory)) {
        ESP_LOGE(TAG, "Failed to write memory to flash");
        return false;
    }

    ESP_LOGI(TAG, "Memory updated successfully, new_size=%d bytes",
             new_memory.raw_json.length());

    // 6. 清空缓冲区
    conversation_buffer_.Clear();

    return true;
}

bool MemoryManager::CompressMemory(const MemoryPackage& old_memory,
                                  const std::string& new_conversations,
                                  MemoryPackage& new_memory) {
    // 构造 prompt
    std::string prompt = R"(你是 Evo-spark 智能陪伴机器人的记忆管理系统。

你的任务：将旧记忆和新对话合并，生成压缩后的新记忆包。

旧记忆（JSON格式）：)" + old_memory.raw_json + R"(

新对话：)" + new_conversations + R"(

请生成新的记忆包，要求：
1. 保留所有重要用户信息和偏好
2. 合并相似记忆，删除冗余
3. 新记忆包格式必须是 JSON
4. 总大小不超过 5KB
5. 使用简洁的语言和短描述
6. 包含完整的 version, metadata, user_profile, memories, recent_context 字段
7. metadata.total_memories 表示记忆项总数
8. 记忆重要性评分（0.0-1.0）
9. 只返回 JSON，不要其他说明文字。)";

    ESP_LOGD(TAG, "Sending to GLM: %d bytes", prompt.length());

    // 调用 GLM API
    std::string response;
    if (!glm_client_->Chat(prompt, response)) {
        ESP_LOGE(TAG, "GLM API call failed");
        return false;
    }

    // 记录响应内容（用于调试）
    ESP_LOGD(TAG, "GLM response: %s", response.c_str());

    // 验证响应非空
    if (response.empty()) {
        ESP_LOGE(TAG, "GLM API returned empty response");
        return false;
    }

    // 清理响应中的 Markdown 代码块标记
    if (response.find("```json") == 0) {
        size_t end = response.rfind("```");
        if (end != std::string::npos) {
            response = response.substr(7, end - 7);
        }
    }

    // 验证 JSON 格式
    cJSON *json = cJSON_Parse(response.c_str());
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse GLM response as JSON: %s", response.c_str());
        return false;
    }

    cJSON *choices = cJSON_GetObjectItem(json, "choices");
    if (!choices || !cJSON_IsArray(choices)) {
        ESP_LOGE(TAG, "GLM response has no choices array");
        cJSON_Delete(json);
        return false;
    }

    cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
    if (!first_choice) {
        ESP_LOGE(TAG, "GLM response choices array is empty");
        cJSON_Delete(json);
        return false;
    }

    cJSON *message_obj = cJSON_GetObjectItem(first_choice, "message");
    if (!message_obj) {
        ESP_LOGE(TAG, "GLM response has no message object");
        cJSON_Delete(json);
        return false;
    }

    cJSON *content = cJSON_GetObjectItem(message_obj, "content");
    if (!content || !cJSON_IsString(content)) {
        ESP_LOGE(TAG, "GLM response content is not valid string");
        cJSON_Delete(json);
        return false;
    }

    new_memory.raw_json = content->valuestring;
    cJSON_Delete(json);

    ESP_LOGI(TAG, "Received compressed memory: %d bytes", new_memory.raw_json.length());
    return true;
}

bool MemoryManager::ValidateMemorySize(const std::string& json) {
    return json.length() <= MAX_MEMORY_SIZE;
}

MemoryPackage MemoryManager::GetMemoryPackage() {
    return flash_storage_.ReadMemory();
}

bool MemoryManager::RollbackToBackup(int version) {
    return flash_storage_.RollbackToBackup(version);
}

} // namespace EvoSpark
