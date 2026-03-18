#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <string>
#include <vector>

namespace EvoSpark {

// Flash 存储管理器 - 封装 SPIFFS 操作
class FlashStorage {
public:
    static FlashStorage& GetInstance() {
        static FlashStorage instance;
        return instance;
    }

    // 初始化 SPIFFS
    bool Init();

    // 读取文件
    bool ReadFile(const std::string& path, std::string& content);

    // 写入文件
    bool WriteFile(const std::string& path, const std::string& content);

    // 删除文件
    bool DeleteFile(const std::string& path);

    // 检查文件是否存在
    bool FileExists(const std::string& path);

    // 获取剩余空间
    size_t GetFreeSpace();

    // 获取已用空间
    size_t GetUsedSpace();

    // 列出目录下的文件
    std::vector<std::string> ListFiles(const std::string& directory = "/spiffs");

private:
    FlashStorage() = default;
    ~FlashStorage() = default;

    bool initialized_ = false;
};

} // namespace EvoSpark

#endif // FLASH_STORAGE_H
