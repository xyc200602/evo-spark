#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <esp_mac.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <string.h>
#include "memory/memory_manager.h"
#include "web/web_server.h"
#include "config/config_manager.h"

using namespace EvoSpark;

extern "C" void app_main(void);

static const char* TAG = "EvoSpark";

// WiFi 事件标志
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// WiFi AP 配置（配置模式）
#define AP_SSID     "EvoSpark-Setup"
#define AP_PASSWORD "evo-spark-123"
#define AP_CHANNEL  1

// WiFi 全局变量
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;
static bool is_ap_mode = false;

// WiFi 事件处理函数
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            ESP_LOGI(TAG, "WiFi disconnected, retrying (%d/10)...", s_retry_num + 1);
            s_retry_num++;
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "Failed to connect to WiFi after 10 attempts");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station " MACSTR " joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

// 初始化 WiFi（STA 模式）
static bool init_wifi_sta(const std::string& ssid, const std::string& password) {
    ESP_LOGI(TAG, "Initializing WiFi in STA mode...");

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return false;
    }

    esp_netif_init();
    esp_event_loop_create_default();
    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return false;
    }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ret = esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return false;
    }

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", ssid.c_str());

    // 等待连接
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(30000));  // 30秒超时

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected successfully!");
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
        return false;
    } else {
        ESP_LOGE(TAG, "WiFi connection timeout");
        return false;
    }
}

// 初始化 WiFi（AP 模式，用于配置）
static bool init_wifi_ap() {
    ESP_LOGI(TAG, "Initializing WiFi in AP mode...");

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return false;
    }

    esp_netif_init();
    esp_event_loop_create_default();
    ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return false;
    }

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    strncpy((char*)wifi_config.ap.ssid, AP_SSID, sizeof(wifi_config.ap.ssid) - 1);
    strncpy((char*)wifi_config.ap.password, AP_PASSWORD, sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.channel = AP_CHANNEL;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    if (strlen(AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(TAG, "WiFi AP started");
    ESP_LOGI(TAG, "SSID: %s", AP_SSID);
    ESP_LOGI(TAG, "Password: %s", AP_PASSWORD);
    ESP_LOGI(TAG, "Please connect to this WiFi and open: http://192.168.4.1/config");

    return true;
}

// 获取本地 IP 地址
static std::string get_local_ip() {
    esp_netif_ip_info_t ip_info;
    if (is_ap_mode) {
        esp_netif_get_ip_info(ap_netif, &ip_info);
    } else {
        esp_netif_get_ip_info(sta_netif, &ip_info);
    }

    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
    return std::string(ip_str);
}

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   Evo-spark-zero Memory System v1.0.0   ");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Hardware: ESP32-S3 N16R8");

    // 1. 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 初始化配置管理器
    ConfigManager& config = ConfigManager::GetInstance();
    config.Init();

    // 3. 检查配置并初始化 WiFi
    if (config.IsConfigured()) {
        ESP_LOGI(TAG, "Configuration found, connecting to WiFi...");
        is_ap_mode = false;
        if (!init_wifi_sta(config.GetWifiSSID(), config.GetWifiPassword())) {
            ESP_LOGE(TAG, "Failed to connect to WiFi, starting AP mode for reconfiguration...");
            esp_wifi_stop();
            esp_netif_destroy(sta_netif);
            is_ap_mode = true;
            init_wifi_ap();
        }
    } else {
        ESP_LOGI(TAG, "No configuration found, starting AP mode...");
        is_ap_mode = true;
        init_wifi_ap();
    }

    // 4. 初始化 Flash 存储
    ESP_LOGI(TAG, "Initializing Flash storage...");
    // FlashStorage 会自动初始化

    // 5. 初始化 Web 服务器
    WebServer& web_server = WebServer::GetInstance();
    if (!web_server.Start()) {
        ESP_LOGE(TAG, "Failed to start web server");
    } else {
        ESP_LOGI(TAG, "Web server started: http://%s", get_local_ip().c_str());
        if (is_ap_mode) {
            ESP_LOGI(TAG, "Please visit: http://%s/config", get_local_ip().c_str());
        } else {
            ESP_LOGI(TAG, "Please visit: http://%s", get_local_ip().c_str());
        }
    }

    // 6. 初始化记忆管理器
    if (config.IsConfigured()) {
        std::string api_key = config.GetApiKey();
        if (!api_key.empty()) {
            MemoryManager& memory_mgr = MemoryManager::GetInstance();
            if (!memory_mgr.Init(api_key)) {
                ESP_LOGE(TAG, "Failed to initialize memory manager");
            } else {
                ESP_LOGI(TAG, "Memory manager initialized successfully");
                ESP_LOGI(TAG, "Free space: %d KB", memory_mgr.GetFreeSpace() / 1024);
            }
        } else {
            ESP_LOGW(TAG, "API Key not configured, memory features disabled");
        }
    } else {
        ESP_LOGW(TAG, "Device not configured, waiting for configuration via Web UI");
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "========================================");

    // 主循环
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
