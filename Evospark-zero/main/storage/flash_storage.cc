#include "flash_storage.h"
#include <cstring>

namespace EvoSpark {

static const char* TAG = "FlashStorage";
const char* FlashStorage::MEMORY_FILE = "/spiffs/memory.json";
const char* FlashStorage::BACKUP_FILE_PREFIX = "/spiffs/memory_backup";

FlashStorage::FlashStorage() : mounted_(false) {
}

FlashStorage::~FlashStorage() {
    if (mounted_) {
        esp_vfs_spiffs_unregister(NULL);
        mounted_ = false;
    }
}

bool FlashStorage::Init() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,  // 支持主文件 + 3个备份
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS: %s", esp_err_to_name(ret));
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS info");
        return false;
    }

    ESP_LOGI(TAG, "SPIFFS mounted: total=%zu KB, used=%zu KB, free=%zu KB",
             total / 1024, used / 1024, (total - used) / 1024);

    mounted_ = true;
    return true;
}

MemoryPackage FlashStorage::ReadMemory() {
    MemoryPackage memory;
    memory.raw_json = "{}";  // 默认空记忆

    FILE* f = fopen(MEMORY_FILE, "r");
    if (f == NULL) {
        ESP_LOGW(TAG, "Memory file not found, returning empty package");
        return memory;
    }

    // 获取文件大小
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    ESP_LOGI(TAG, "Reading memory file: %ld bytes", size);

    // 读取内容
    char* buffer = new char[size + 1];
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    memory.raw_json = std::string(buffer);
    delete[] buffer;

    // 解析 JSON（简化版，实际需要完整的 JSON 解析）
    ESP_LOGI(TAG, "Memory loaded: %zu bytes", memory.raw_json.length());

    return memory;
}

bool FlashStorage::WriteMemory(const MemoryPackage& memory) {
    // 先创建备份（滚动备份）
    BackupMemory(memory);

    // 写入主文件
    FILE* f = fopen(MEMORY_FILE, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open memory file for writing");
        return false;
    }

    size_t written = fwrite(memory.raw_json.c_str(), 1,
                        memory.raw_json.length(), f);
    fclose(f);

    if (written != memory.raw_json.length()) {
        ESP_LOGE(TAG, "Write incomplete: %zu/%zu bytes",
                 written, memory.raw_json.length());
        return false;
    }

    ESP_LOGI(TAG, "Memory written: %zu bytes", written);

    // 清理旧备份
    CleanOldBackups();

    return true;
}

std::string FlashStorage::GetBackupFileName(int version) {
    std::stringstream ss;
    ss << BACKUP_FILE_PREFIX << "_" << version << ".json";
    return ss.str();
}

bool FlashStorage::BackupMemory(const MemoryPackage& memory) {
    ESP_LOGI(TAG, "Creating backup...");

    // 滚动备份：3->2, 2->1, 新版本为1
    for (int v = MAX_BACKUP_VERSIONS; v > 1; v--) {
        std::string old_backup = GetBackupFileName(v);
        std::string new_backup = GetBackupFileName(v - 1);

        // SPIFFS 不支持 rename，使用复制-删除策略
        //1. 读取旧备份
        FILE* src = fopen(old_backup.c_str(), "r");
        if (src != NULL) {
            fseek(src, 0, SEEK_END);
            long size = ftell(src);
            fseek(src, 0, SEEK_SET);

            char* buffer = new char[size + 1];
            fread(buffer, 1, size, src);
            buffer[size] = '\0';
            fclose(src);

            // 2. 写入新备份
            FILE* dst = fopen(new_backup.c_str(), "w");
            if (dst == NULL) {
                ESP_LOGE(TAG, "Failed to create backup file: %s", new_backup.c_str());
                delete[] buffer;
                return false;
            }
            fwrite(buffer, 1, size, dst);
            fclose(dst);

            // 3. 删除旧备份
            if (remove(old_backup.c_str()) == 0) {
                ESP_LOGI(TAG, "Deleted old backup: %s", old_backup.c_str());
            } else {
                ESP_LOGW(TAG, "Failed to delete old backup: %s", old_backup.c_str());
            }

            delete[] buffer;
        } else {
            ESP_LOGW(TAG, "Backup file not found: %s", old_backup.c_str());
        }
    }

    // 写入新备份（版本 1）
    std::string backup_file = GetBackupFileName(1);
    FILE* f = fopen(backup_file.c_str(), "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to create backup file: %s", backup_file.c_str());
        return false;
    }

    fwrite(memory.raw_json.c_str(), 1, memory.raw_json.length(), f);
    fclose(f);

    ESP_LOGI(TAG, "Backup created: %s (%zu bytes)", backup_file.c_str(),
             memory.raw_json.length());
    return true;
}

bool FlashStorage::RollbackToBackup(int version) {
    if (version < 1 || version > MAX_BACKUP_VERSIONS) {
        ESP_LOGE(TAG, "Invalid backup version: %d (valid: 1-%d)",
                  version, MAX_BACKUP_VERSIONS);
        return false;
    }

    std::string backup_file = GetBackupFileName(version);
    FILE* f = fopen(backup_file.c_str(), "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Backup file not found: %s", backup_file.c_str());
        return false;
    }

    // 读取备份
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = new char[size + 1];
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    std::string backup_content(buffer);
    delete[] buffer;

    // 写入主文件
    FILE* main_f = fopen(MEMORY_FILE, "w");
    if (main_f == NULL) {
        ESP_LOGE(TAG, "Failed to open main file for rollback");
        return false;
    }

    fwrite(backup_content.c_str(), 1, backup_content.length(), main_f);
    fclose(main_f);

    ESP_LOGI(TAG, "Rolled back to backup version %d: %zu bytes",
             version, backup_content.length());
    return true;
}

bool FlashStorage::DeleteBackup(int version) {
    std::string backup_file = GetBackupFileName(version);
    if (remove(backup_file.c_str()) == 0) {
        ESP_LOGI(TAG, "Deleted backup: %s", backup_file.c_str());
        return true;
    }
    return false;
}

bool FlashStorage::CleanOldBackups() {
    // 检查备份数量，删除超过 MAX_BACKUP_VERSIONS 的
    int count = 0;
    std::vector<int> versions_to_delete;

    for (int v = 1; v <= MAX_BACKUP_VERSIONS + 2; v++) {
        std::string backup_file = GetBackupFileName(v);
        FILE* f = fopen(backup_file.c_str(), "r");
        if (f != NULL) {
            fclose(f);
            count++;
            if (count > MAX_BACKUP_VERSIONS) {
                // 超过最大数量，标记为待删除
                versions_to_delete.push_back(v);
                count--;
            }
        }
    }

    // 删除标记的文件（从旧到新，避免覆盖）
    std::sort(versions_to_delete.begin(), versions_to_delete.end(), std::greater<int>());
    for (int v : versions_to_delete) {
        if (v >= 1 && v <= MAX_BACKUP_VERSIONS) {
            DeleteBackup(v);
        } else {
            ESP_LOGW(TAG, "Invalid version to delete: %d", v);
        }
    }

    return true;
}

size_t FlashStorage::GetFreeSpace() {
    size_t total = 0, used = 0;
    if (esp_spiffs_info(NULL, &total, &used) == ESP_OK) {
        return total - used;
    }
    return 0;
}

size_t FlashStorage::GetUsedSpace() {
    size_t total = 0, used = 0;
    if (esp_spiffs_info(NULL, &total, &used) == ESP_OK) {
        return used;
    }
    return 0;
}

} // namespace EvoSpark
