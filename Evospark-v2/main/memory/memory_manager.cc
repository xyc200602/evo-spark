#include "memory_manager.h"
#include "prompt_builder.h"
#include "../ai/llm_client.h"
#include "esp_log.h"
#include <sstream>
#include <algorithm>

namespace EvoSpark {

static const char* TAG = "MemoryManager";

constexpr const char* MEMORY_FILE = "/spiffs/memory.json";
constexpr const char* BACKUP_FILE_PREFIX = "/spiffs/memory_backup_";
constexpr int BACKUP_COUNT = 3;
constexpr int MAX_MEMORY_EVENTS = 20;

MemoryManager::MemoryManager() {
}

bool MemoryManager::Init(const std::string& api_key) {
    if (initialized_) {
        return true;
    }

    ESP_LOGI(TAG, "Initializing MemoryManager...");

    api_key_ = api_key;

    // 初始化 Flash 存储
    if (!flash_storage_.Init()) {
        ESP_LOGE(TAG, "Failed to initialize flash storage");
        return false;
    }

    // 加载缓存
    cached_memory_ = LoadMemory();

    initialized_ = true;
    ESP_LOGI(TAG, "MemoryManager initialized");
    return true;
}

CompressedMemory MemoryManager::LoadMemory() {
    CompressedMemory memory;

    std::string json;
    if (!flash_storage_.ReadFile(MEMORY_FILE, json)) {
        ESP_LOGI(TAG, "No existing memory file, creating new one");
        return memory;
    }

    if (ParseMemory(json, memory)) {
        memory.raw_json = json;
        ESP_LOGI(TAG, "Memory loaded: %zu events, %zu preferences",
                 memory.key_events.size(), memory.preferences.size());
    } else {
        ESP_LOGE(TAG, "Failed to parse memory file");
    }

    return memory;
}

bool MemoryManager::SaveMemory(const CompressedMemory& memory) {
    // 滚动备份
    RotateBackups();

    // 序列化
    std::string json = SerializeMemory(memory);

    // 保存
    if (!flash_storage_.WriteFile(MEMORY_FILE, json)) {
        ESP_LOGE(TAG, "Failed to save memory");
        return false;
    }

    // 更新缓存
    cached_memory_ = memory;
    cached_memory_.raw_json = json;

    ESP_LOGI(TAG, "Memory saved: %zu bytes", json.size());
    return true;
}

CompressedMemory MemoryManager::CompressMemory(
    const CompressedMemory& old_memory,
    const std::vector<Message>& session_messages
) {
    ESP_LOGI(TAG, "Compressing memory...");

    // 构建压缩 Prompt
    std::string prompt = PromptBuilder::BuildCompressionPrompt(
        old_memory,
        session_messages
    );

    // 调用 LLM
    std::string response;
    if (!CallLLMForCompression(prompt, response)) {
        ESP_LOGE(TAG, "Failed to call LLM for compression");
        return old_memory;  // 返回旧记忆
    }

    // 解析响应
    CompressedMemory new_memory;
    if (ParseMemory(response, new_memory)) {
        new_memory.last_updated = std::time(nullptr);
        new_memory.version = old_memory.version + 1;
        new_memory.total_sessions = old_memory.total_sessions + 1;
        new_memory.raw_json = response;

        ESP_LOGI(TAG, "Memory compressed: v%d, %zu events, %zu preferences",
                 new_memory.version, new_memory.key_events.size(),
                 new_memory.preferences.size());

        return new_memory;
    } else {
        ESP_LOGE(TAG, "Failed to parse LLM response, keeping old memory");
        return old_memory;
    }
}

bool MemoryManager::RollbackToBackup(int version) {
    if (version < 1 || version > BACKUP_COUNT) {
        ESP_LOGE(TAG, "Invalid backup version: %d", version);
        return false;
    }

    std::string backup_file = BACKUP_FILE_PREFIX + std::to_string(version) + ".json";
    std::string json;

    if (!flash_storage_.ReadFile(backup_file, json)) {
        ESP_LOGE(TAG, "Failed to read backup file: %s", backup_file.c_str());
        return false;
    }

    // 恢复备份
    if (!flash_storage_.WriteFile(MEMORY_FILE, json)) {
        ESP_LOGE(TAG, "Failed to restore backup");
        return false;
    }

    // 重新加载缓存
    cached_memory_ = LoadMemory();

    ESP_LOGI(TAG, "Rolled back to backup %d", version);
    return true;
}

std::vector<std::string> MemoryManager::GetBackupList() {
    std::vector<std::string> backups;

    for (int i = 1; i <= BACKUP_COUNT; i++) {
        std::string file = BACKUP_FILE_PREFIX + std::to_string(i) + ".json";
        if (flash_storage_.FileExists(file)) {
            backups.push_back("Backup " + std::to_string(i));
        }
    }

    return backups;
}

size_t MemoryManager::GetFreeSpace() {
    return flash_storage_.GetFreeSpace();
}

size_t MemoryManager::GetUsedSpace() {
    return flash_storage_.GetUsedSpace();
}

bool MemoryManager::ClearMemory() {
    ESP_LOGW(TAG, "Clearing all memory...");

    // 删除主文件
    flash_storage_.DeleteFile(MEMORY_FILE);

    // 删除备份
    for (int i = 1; i <= BACKUP_COUNT; i++) {
        std::string file = BACKUP_FILE_PREFIX + std::to_string(i) + ".json";
        flash_storage_.DeleteFile(file);
    }

    // 清空缓存
    cached_memory_ = CompressedMemory();

    ESP_LOGI(TAG, "Memory cleared");
    return true;
}

void MemoryManager::RotateBackups() {
    // 备份 3 -> 删除
    std::string file3 = BACKUP_FILE_PREFIX + "3.json";
    flash_storage_.DeleteFile(file3);

    // 备份 2 -> 备份 3
    std::string file2 = BACKUP_FILE_PREFIX + "2.json";
    if (flash_storage_.FileExists(file2)) {
        std::string content;
        flash_storage_.ReadFile(file2, content);
        flash_storage_.WriteFile(file3, content);
    }

    // 备份 1 -> 备份 2
    std::string file1 = BACKUP_FILE_PREFIX + "1.json";
    if (flash_storage_.FileExists(file1)) {
        std::string content;
        flash_storage_.ReadFile(file1, content);
        flash_storage_.WriteFile(file2, content);
    }

    // 当前 -> 备份 1
    if (flash_storage_.FileExists(MEMORY_FILE)) {
        std::string content;
        flash_storage_.ReadFile(MEMORY_FILE, content);
        flash_storage_.WriteFile(file1, content);
    }

    ESP_LOGI(TAG, "Backups rotated");
}

bool MemoryManager::ParseMemory(const std::string& json, CompressedMemory& memory) {
    // 简单 JSON 解析（生产环境应使用 cJSON 或 ArduinoJson）
    // 这里仅作演示

    if (json.empty()) {
        return false;
    }

    // TODO: 使用 JSON 库正确解析
    // 临时方案：假设是格式化的 JSON

    memory.raw_json = json;

    // 查找 user_profile
    size_t pos = json.find("\"user_profile\":");
    if (pos != std::string::npos) {
        size_t start = json.find("\"", pos + 16);
        size_t end = json.find("\"", start + 1);
        if (start != std::string::npos && end != std::string::npos) {
            memory.user_profile = json.substr(start + 1, end - start - 1);
        }
    }

    // TODO: 解析其他字段

    return true;
}

std::string MemoryManager::SerializeMemory(const CompressedMemory& memory) {
    std::ostringstream oss;

    oss << "{\n";
    oss << "  \"version\": " << memory.version << ",\n";
    oss << "  \"total_sessions\": " << memory.total_sessions << ",\n";
    oss << "  \"last_updated\": " << memory.last_updated << ",\n";

    // user_profile
    oss << "  \"user_profile\": \"" << memory.user_profile << "\",\n";

    // key_events
    oss << "  \"key_events\": [\n";
    for (size_t i = 0; i < memory.key_events.size(); i++) {
        oss << "    \"" << memory.key_events[i] << "\"";
        if (i < memory.key_events.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";

    // preferences
    oss << "  \"preferences\": [\n";
    for (size_t i = 0; i < memory.preferences.size(); i++) {
        oss << "    \"" << memory.preferences[i] << "\"";
        if (i < memory.preferences.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";

    // last_session_summary
    oss << "  \"last_session_summary\": \"" << memory.last_session_summary << "\"\n";

    oss << "}\n";

    return oss.str();
}

bool MemoryManager::CallLLMForCompression(
    const std::string& prompt,
    std::string& response
) {
    ESP_LOGI(TAG, "Calling LLM for compression...");

    LLMClient& llm = LLMClient::GetInstance();
    if (!llm.IsInitialized()) {
        ESP_LOGE(TAG, "LLM client not initialized");
        return false;
    }

    LLMResponse resp = llm.CompressMemory(prompt);
    if (!resp.success) {
        ESP_LOGE(TAG, "LLM compression failed: %s", resp.error_message.c_str());
        return false;
    }

    response = resp.content;
    ESP_LOGI(TAG, "Compression complete, %d tokens used", resp.tokens_used);
    return true;
}

} // namespace EvoSpark
