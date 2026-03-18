#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "core/session_manager.h"
#include "core/event_bus.h"
#include "memory/memory_manager.h"
#include "config/config_manager.h"
#include "web/web_server.h"
#include "input/button.h"
#include "actuator/led_controller.h"
#include "storage/flash_storage.h"
#include "perception/audio/microphone.h"
#include "perception/audio/speaker.h"
#include "perception/camera/camera_manager.h"
#include "perception/vision/yolo_detector.h"
#include "display/ui_manager.h"

using namespace EvoSpark;

extern "C" void app_main(void);

static const char* TAG = "EvoSpark-v2";

// GPIO 定义
#define BUTTON_GPIO    GPIO_NUM_0   // Boot 按键
#define LED_GPIO       45           // WS2812 LED

// WiFi 配置
#define AP_SSID     "EvoSpark-v2-Setup"
#define AP_PASSWORD "evo-spark-123"

// WiFi 事件
static EventGroupHandle_t s_wifi_event_group = nullptr;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;
static bool is_ap_mode = false;

// WiFi 事件处理
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry WiFi connection (%d/10)", s_retry_num);
        } else {
            if (s_wifi_event_group) {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        if (s_wifi_event_group) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

// 初始化 WiFi (STA 模式)
static bool init_wifi_sta(const std::string& ssid, const std::string& password) {
    ESP_LOGI(TAG, "Initializing WiFi STA...");

    s_wifi_event_group = xEventGroupCreate();
    if (!s_wifi_event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        return false;
    }
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, nullptr));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(30000));

    return (bits & WIFI_CONNECTED_BIT) != 0;
}

// 初始化 WiFi (AP 模式)
static bool init_wifi_ap() {
    ESP_LOGI(TAG, "Initializing WiFi AP...");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.ap.ssid, AP_SSID, sizeof(wifi_config.ap.ssid) - 1);
    strncpy((char*)wifi_config.ap.password, AP_PASSWORD, sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP started: %s (password: %s)", AP_SSID, AP_PASSWORD);
    ESP_LOGI(TAG, "Connect to WiFi and open: http://192.168.4.1/config");

    return true;
}

// 获取本地 IP
static std::string get_local_ip() {
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey(
        is_ap_mode ? "WIFI_AP_DEF" : "WIFI_STA_DEF");
    if (!netif) return "0.0.0.0";

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(netif, &ip_info);

    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
    return std::string(ip_str);
}

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   EvoSpark v2.0 - Session Memory System   ");
    ESP_LOGI(TAG, "========================================");

    // 1. 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 初始化配置管理器
    ConfigManager& config = ConfigManager::GetInstance();
    ESP_ERROR_CHECK(config.Init());

    // 3. 初始化 WiFi
    if (config.IsConfigured()) {
        ESP_LOGI(TAG, "Connecting to WiFi...");
        is_ap_mode = false;
        if (!init_wifi_sta(config.GetWifiSSID(), config.GetWifiPassword())) {
            ESP_LOGE(TAG, "WiFi connection failed, starting AP mode");
            esp_wifi_stop();
            is_ap_mode = true;
            init_wifi_ap();
        }
    } else {
        ESP_LOGI(TAG, "No configuration, starting AP mode");
        is_ap_mode = true;
        init_wifi_ap();
    }

    // 4. 初始化 Flash 存储
    FlashStorage& flash = FlashStorage::GetInstance();
    if (!flash.Init()) {
        ESP_LOGE(TAG, "Failed to initialize flash storage");
    }

    // 5. 初始化记忆管理器
    MemoryManager& memory = MemoryManager::GetInstance();
    if (config.IsConfigured() && !config.GetApiKey().empty()) {
        if (!memory.Init(config.GetApiKey())) {
            ESP_LOGE(TAG, "Failed to initialize memory manager");
        } else {
            ESP_LOGI(TAG, "Memory manager initialized");
        }
    }

    // 6. 初始化 LED
    LEDController& led = LEDController::GetInstance();
    if (!led.Init(LED_GPIO)) {
        ESP_LOGE(TAG, "Failed to initialize LED");
    } else {
        led.SetColor(0, 100, 255);  // 蓝色：系统启动中
    }

    // 7. 初始化显示
    UIManager& ui = UIManager::GetInstance();
    if (!ui.Init()) {
        ESP_LOGE(TAG, "Failed to initialize display");
    }

    // 8. 初始化摄像头
    CameraManager& camera = CameraManager::GetInstance();
    if (!camera.Init()) {
        ESP_LOGW(TAG, "Camera not available");
    }

    // 9. 初始化 YOLO
    YOLODetector& yolo = YOLODetector::GetInstance();
    if (!yolo.Init("/spiffs/yolo.tflite")) {
        ESP_LOGW(TAG, "YOLO model not loaded");
    }

    // 10. 初始化麦克风
    Microphone& mic = Microphone::GetInstance();
    if (!mic.Init()) {
        ESP_LOGW(TAG, "Microphone not available");
    }

    // 11. 初始化扬声器
    Speaker& speaker = Speaker::GetInstance();
    if (!speaker.Init()) {
        ESP_LOGW(TAG, "Speaker not available");
    }

    // 12. 初始化按键
    Button button(BUTTON_GPIO, true);  // 低电平触发
    button.SetPressCallback([]() {
        EventBus::GetInstance().Publish(EventType::BUTTON_PRESS, "Button");
    });

    // 13. 初始化会话管理器
    SessionManager& session = SessionManager::GetInstance();
    if (!session.Init()) {
        ESP_LOGE(TAG, "Failed to initialize session manager");
    }

    // 14. 设置状态回调（LED + UI）
    session.SetStateCallback([](SessionState old_state, SessionState new_state) {
        LEDController& led = LEDController::GetInstance();
        UIManager& ui = UIManager::GetInstance();
        
        ui.ShowStatus(new_state);

        switch (new_state) {
            case SessionState::IDLE:
                led.SetColor(0, 100, 255);  // 蓝色：待机
                break;
            case SessionState::LISTENING:
                led.SetColor(0, 255, 100);  // 绿色：监听
                break;
            case SessionState::PROCESSING:
                led.Blink(255, 255, 0, 300);  // 黄色闪烁：处理中
                break;
            case SessionState::CLOSING:
                led.SetColor(255, 100, 0);  // 橙色：关闭中
                break;
            default:
                break;
        }
    });

    // 15. 启动 Web 服务器
    WebServer& web = WebServer::GetInstance();
    if (!web.Start()) {
        ESP_LOGE(TAG, "Failed to start web server");
    } else {
        ESP_LOGI(TAG, "Web server: http://%s", get_local_ip().c_str());
        if (is_ap_mode) {
            ESP_LOGI(TAG, "Configure: http://192.168.4.1/config");
        }
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "System ready! Press button to start session.");
    ESP_LOGI(TAG, "========================================");

    // 主循环
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
