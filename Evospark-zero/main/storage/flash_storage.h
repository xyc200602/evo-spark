#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <string>
#include "esp_spiffs.h"
#include "esp_log.h"
#include "../memory/memory_types.h"

namespace EvoSpark {

// 历史版本管理
static const int MAX_BACKUP_VERSIONS = 3;  // 保留 3 个历史版本

class FlashStorage {
public:
    static const char* MEMORY_FILE;
    static const char* BACKUP_FILE_PREFIX;

    FlashStorage();
    ~FlashStorage();

    bool Init();
    MemoryPackage ReadMemory();
    bool WriteMemory(const MemoryPackage& memory);

    // 备份管理
    bool BackupMemory(const MemoryPackage& memory);
    bool RollbackToBackup(int version);  // version: 1=最新备份, 2=次新, 3=最旧
    bool CleanOldBackups();

    // 存储信息
    size_t GetFreeSpace();
    size_t GetUsedSpace();

private:
    bool mounted_;
    std::string GetBackupFileName(int version);
    bool DeleteBackup(int version);
};

} // namespace EvoSpark

#endif // FLASH_STORAGE_H
