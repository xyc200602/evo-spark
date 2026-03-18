#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <string>
#include <vector>
#include "memory_types.h"
#include "conversation_buffer.h"
#include "../storage/flash_storage.h"

namespace EvoSpark {

// 记忆管理器 v2 - 管理长期记忆和压缩
class MemoryManager {
public:
    static MemoryManager& GetInstance() {
        static MemoryManager instance;
        return instance;
    }

    // 初始化
    bool Init(const std::string& api_key = "");

    // 加载长期记忆
    CompressedMemory LoadMemory();

    // 保存长期记忆
    bool SaveMemory(const CompressedMemory& memory);

    // 压缩记忆（LLM 调用）
    CompressedMemory CompressMemory(
        const CompressedMemory& old_memory,
        const std::vector<Message>& session_messages
    );

    // 回滚到历史版本
    bool RollbackToBackup(int version);

    // 获取备份列表
    std::vector<std::string> GetBackupList();

    // 获取存储信息
    size_t GetFreeSpace();
    size_t GetUsedSpace();

    // 清空记忆（重置）
    bool ClearMemory();

private:
    MemoryManager();
    ~MemoryManager() = default;

    // 禁止拷贝
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // 解析 JSON 到 CompressedMemory
    bool ParseMemory(const std::string& json, CompressedMemory& memory);

    // 序列化 CompressedMemory 到 JSON
    std::string SerializeMemory(const CompressedMemory& memory);

    // 滚动备份
    void RotateBackups();

    // 调用 LLM API 压缩记忆
    bool CallLLMForCompression(
        const std::string& prompt,
        std::string& response
    );

    FlashStorage flash_storage_;
    std::string api_key_;
    bool initialized_ = false;

    // 缓存的当前记忆
    CompressedMemory cached_memory_;
};

} // namespace EvoSpark

#endif // MEMORY_MANAGER_H
