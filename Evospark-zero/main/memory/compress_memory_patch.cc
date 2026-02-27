// 压缩记忆函数补丁
// 替换 CompressMemory 函数，添加详细日志和验证

bool MemoryManager::CompressMemory(const MemoryPackage& old_memory,
                                  const std::string& new_conversations,
                                  MemoryPackage& new_memory) {
    // 构造 prompt
    std::string prompt = R"(你是 Evo-spark 智能陪伴机器人的记忆管理系统。

你的任务：将旧记忆和新对话合并，生成压缩后的新记忆包。

旧记忆（JSON格式）：
)" + old_memory.raw_json + R"(
   新对话：

请生成新的记忆包，要求：
1. 保留所有重要用户信息和偏好
2. 合并相似记忆，删除冗余
3. 新记忆包格式必须是 JSON
4. 总大小不超过 5KB
5. 使用简洁的语言和短描述
6. 包含完整的 version, metadata, user_profile, memories, recent_context 字段
7. metadata.total_memories 表示记忆项总数
8. 记忆重要性评分（0.0-1.0）
9. 只返回 JSON，不要其他说明文字。";

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
