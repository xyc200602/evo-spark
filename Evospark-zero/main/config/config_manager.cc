#include "config_manager.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>

namespace EvoSpark {

static const char* TAG = "ConfigManager";
static const char* NVS_NAMESPACE = "evo_config";
static const char* KEY_WIFI_SSID = "wifi_ssid";
static const char* KEY_WIFI_PASS = "wifi_pass";
static const char* KEY_API_KEY = "api_key";
static const char* KEY_CONFIGURED = "configured";

ConfigManager::ConfigManager() : is_configured_(false) {
}

ConfigManager::~ConfigManager() {
}

esp_err_t ConfigManager::Init() {
    ESP_LOGI(TAG, "Initializing ConfigManager...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);

    if (err == ESP_OK) {
        // 检查是否已配置
        uint8_t configured = 0;
        if (nvs_get_u8(nvs_handle, KEY_CONFIGURED, &configured) == ESP_OK && configured) {
            is_configured_ = true;

            // 读取 WiFi SSID
            size_t required_size = 0;
            nvs_get_str(nvs_handle, KEY_WIFI_SSID, NULL, &required_size);
            if (required_size > 0) {
                char* buffer = new char[required_size];
                if (nvs_get_str(nvs_handle, KEY_WIFI_SSID, buffer, &required_size) == ESP_OK) {
                    wifi_ssid_ = std::string(buffer);
                }
                delete[] buffer;
            }

            // 读取 WiFi 密码
            required_size = 0;
            nvs_get_str(nvs_handle, KEY_WIFI_PASS, NULL, &required_size);
            if (required_size > 0) {
                char* buffer = new char[required_size];
                if (nvs_get_str(nvs_handle, KEY_WIFI_PASS, buffer, &required_size) == ESP_OK) {
                    wifi_password_ = std::string(buffer);
                }
                delete[] buffer;
            }

            // 读取 API Key
            required_size = 0;
            nvs_get_str(nvs_handle, KEY_API_KEY, NULL, &required_size);
            if (required_size > 0) {
                char* buffer = new char[required_size];
                if (nvs_get_str(nvs_handle, KEY_API_KEY, buffer, &required_size) == ESP_OK) {
                    api_key_ = std::string(buffer);
                }
                delete[] buffer;
            }

            ESP_LOGI(TAG, "Configuration loaded:");
            ESP_LOGI(TAG, "  WiFi SSID: %s", wifi_ssid_.c_str());
            ESP_LOGI(TAG, "  WiFi Password: %s", wifi_password_.empty() ? "(empty)" : "***");
            ESP_LOGI(TAG, "  API Key: %s***", api_key_.substr(0, std::min((size_t)8, api_key_.length())).c_str());
        } else {
            ESP_LOGI(TAG, "No configuration found");
        }

        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t ConfigManager::SetConfig(const std::string& ssid,
                                   const std::string& password,
                                   const std::string& api_key) {
    ESP_LOGI(TAG, "Saving configuration...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    // 保存 WiFi SSID
    err = nvs_set_str(nvs_handle, KEY_WIFI_SSID, ssid.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save WiFi SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // 保存 WiFi 密码
    err = nvs_set_str(nvs_handle, KEY_WIFI_PASS, password.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save WiFi password: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // 保存 API Key
    err = nvs_set_str(nvs_handle, KEY_API_KEY, api_key.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save API key: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // 标记为已配置
    err = nvs_set_u8(nvs_handle, KEY_CONFIGURED, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set configured flag: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // 提交到 NVS
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);

    // 更新内存中的配置
    wifi_ssid_ = ssid;
    wifi_password_ = password;
    api_key_ = api_key;
    is_configured_ = true;

    ESP_LOGI(TAG, "Configuration saved successfully!");
    ESP_LOGI(TAG, "  WiFi SSID: %s", ssid.c_str());
    ESP_LOGI(TAG, "  WiFi Password: %s", password.empty() ? "(empty)" : "***");
    ESP_LOGI(TAG, "  API Key: %s***", api_key.substr(0, std::min((size_t)8, api_key.length())).c_str());

    return ESP_OK;
}

esp_err_t ConfigManager::ClearConfig() {
    ESP_LOGI(TAG, "Clearing configuration...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    // 清除所有配置
    nvs_erase_key(nvs_handle, KEY_WIFI_SSID);
    nvs_erase_key(nvs_handle, KEY_WIFI_PASS);
    nvs_erase_key(nvs_handle, KEY_API_KEY);
    nvs_erase_key(nvs_handle, KEY_CONFIGURED);

    // 提交
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);

    // 清除内存中的配置
    wifi_ssid_.clear();
    wifi_password_.clear();
    api_key_.clear();
    is_configured_ = false;

    ESP_LOGI(TAG, "Configuration cleared!");

    return ESP_OK;
}

} // namespace EvoSpark
