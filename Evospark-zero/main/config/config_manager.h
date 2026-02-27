#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <esp_err.h>

namespace EvoSpark {

class ConfigManager {
public:
    static ConfigManager& GetInstance() {
        static ConfigManager instance;
        return instance;
    }

    // 初始化（从 NVS 读取配置）
    esp_err_t Init();

    // 检查是否已配置
    bool IsConfigured() const { return is_configured_; }

    // 获取配置
    std::string GetWifiSSID() const { return wifi_ssid_; }
    std::string GetWifiPassword() const { return wifi_password_; }
    std::string GetApiKey() const { return api_key_; }

    // 保存配置
    esp_err_t SetConfig(const std::string& ssid, const std::string& password,
                       const std::string& api_key);

    // 清除配置（恢复出厂设置）
    esp_err_t ClearConfig();

private:
    ConfigManager();
    ~ConfigManager();

    std::string wifi_ssid_;
    std::string wifi_password_;
    std::string api_key_;
    bool is_configured_;
};

} // namespace EvoSpark

#endif // CONFIG_MANAGER_H
