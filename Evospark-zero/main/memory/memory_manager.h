#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <string>
#include <functional>
#include "memory_types.h"
#include "conversation_buffer.h"
#include "../storage/flash_storage.h"
#include "../api/glm_client.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace EvoSpark {

// 压缩任务数据结构（使用固定长度字符数组）
#define MAX_ROLE_LEN 32
#define MAX_CONTENT_LEN 2048

struct CompressionTask {
    char role[MAX_ROLE_LEN];
    char content[MAX_CONTENT_LEN];
};

class MemoryManager {
public:
    static MemoryManager& GetInstance() {
        static MemoryManager instance;
        return instance;
    }

    // 初始化
    bool Init(const std::string& api_key);

    // 添加对话到缓冲区（异步）
    void AddConversation(const std::string& role, const std::string& content);

    // 手动触发记忆更新
    bool UpdateMemory();

    // 获取当前记忆包
    MemoryPackage GetMemoryPackage();

    // 获取对话缓冲区状态
    size_t GetConversationCount() const { return conversation_buffer_.GetCount(); }
    bool IsBufferEmpty() const { return conversation_buffer_.IsEmpty(); }
    size_t GetBufferSize() const { return conversation_buffer_.GetSize(); }

    // 回滚到历史版本
    bool RollbackToBackup(int version);

    // 获取存储信息
    size_t GetFreeSpace() { return flash_storage_.GetFreeSpace(); }
    size_t GetUsedSpace() { return flash_storage_.GetUsedSpace(); }

private:
    MemoryManager();
    ~MemoryManager();
    void OnTimerCallback();

    // 异步压缩任务
    static void CompressionTaskWrapper(void* arg);
    void ProcessCompressionTask();

    // 调用 GLM API 压缩记忆
    bool CompressMemory(const MemoryPackage& old_memory,
                      const std::string& new_conversations,
                      MemoryPackage& new_memory);

    // 验证记忆包大小
    bool ValidateMemorySize(const std::string& json);

    ConversationBuffer conversation_buffer_;
    FlashStorage flash_storage_;
    GLMClient* glm_client_;

    // FreeRTOS 队列和任务
    QueueHandle_t compression_queue_;
    TaskHandle_t compression_task_;

    bool is_initialized_ = false;
};

} // namespace EvoSpark

#endif // MEMORY_MANAGER_H
