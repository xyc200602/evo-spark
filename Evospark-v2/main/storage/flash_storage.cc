#include "flash_storage.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <sys/stat.h>
#include <fstream>
#include <sstream>

namespace EvoSpark {

static const char* TAG = "FlashStorage";

bool FlashStorage::Init() {
    if (initialized_) {
        return true;
    }

    ESP_LOGI(TAG, "Initializing SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS: total=%d KB, used=%d KB", total / 1024, used / 1024);
    }

    initialized_ = true;
    return true;
}

bool FlashStorage::ReadFile(const std::string& path, std::string& content) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        ESP_LOGW(TAG, "Failed to open file for reading: %s", path.c_str());
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    content = ss.str();

    file.close();
    return true;
}

bool FlashStorage::WriteFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path.c_str());
        return false;
    }

    file << content;
    file.close();

    ESP_LOGI(TAG, "Wrote %zu bytes to %s", content.size(), path.c_str());
    return true;
}

bool FlashStorage::DeleteFile(const std::string& path) {
    if (remove(path.c_str()) == 0) {
        ESP_LOGI(TAG, "Deleted file: %s", path.c_str());
        return true;
    } else {
        ESP_LOGW(TAG, "Failed to delete file: %s", path.c_str());
        return false;
    }
}

bool FlashStorage::FileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
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

std::vector<std::string> FlashStorage::ListFiles(const std::string& directory) {
    std::vector<std::string> files;
    // TODO: 实现目录遍历
    return files;
}

} // namespace EvoSpark
