#include "camera_manager.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace EvoSpark {

static const char* TAG = "Camera";

CameraManager::~CameraManager() {
    if (initialized_) {
        esp_camera_deinit();
    }
}

bool CameraManager::Init() {
    if (initialized_) {
        return true;
    }

    ESP_LOGI(TAG, "Initializing camera...");

    // OV2640 配置
    camera_config_t config = {
        .pin_pwdn = 6,
        .pin_reset = 21,
        .pin_xclk = 7,
        .pin_sscb_sda = 4,
        .pin_sscb_scl = 5,
        .pin_d7 = 18,
        .pin_d6 = 17,
        .pin_d5 = 16,
        .pin_d4 = 15,
        .pin_d3 = 14,
        .pin_d2 = 13,
        .pin_d1 = 12,
        .pin_d0 = 11,
        .pin_vsync = 8,
        .pin_href = 9,
        .pin_pclk = 10,

        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_QVGA,  // 320x240
        .jpeg_quality = 12,
        .fb_count = 2,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST
    };

    esp_err_t ret = esp_camera_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(ret));
        return false;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "Camera initialized (320x240 JPEG)");
    return true;
}

ImageFrame CameraManager::Capture() {
    ImageFrame frame;

    if (!initialized_) {
        ESP_LOGE(TAG, "Camera not initialized");
        return frame;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Failed to capture frame");
        return frame;
    }

    frame.data = (uint8_t*)malloc(fb->len);
    if (frame.data) {
        memcpy(frame.data, fb->buf, fb->len);
        frame.size = fb->len;
        frame.width = fb->width;
        frame.height = fb->height;
        frame.format = fb->format;
    }

    esp_camera_fb_return(fb);
    return frame;
}

std::vector<uint8_t> CameraManager::CaptureJPEG(int quality) {
    std::vector<uint8_t> jpeg;

    if (!initialized_) {
        return jpeg;
    }

    // 如果需要调整质量
    if (quality != 12) {
        sensor_t* s = esp_camera_sensor_get();
        if (s) {
            s->set_quality(s, quality);
        }
    }

    ImageFrame frame = Capture();
    if (frame.IsValid()) {
        jpeg.assign(frame.data, frame.data + frame.size);
        frame.Free();
    }

    return jpeg;
}

bool CameraManager::StartStreaming() {
    if (!initialized_) {
        return false;
    }

    streaming_ = true;
    ESP_LOGI(TAG, "Streaming started");
    return true;
}

void CameraManager::StopStreaming() {
    streaming_ = false;
    ESP_LOGI(TAG, "Streaming stopped");
}

void CameraManager::SetBrightness(int brightness) {
    if (!initialized_) return;

    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, brightness);
    }
}

void CameraManager::SetContrast(int contrast) {
    if (!initialized_) return;

    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_contrast(s, contrast);
    }
}

void CameraManager::SetSaturation(int saturation) {
    if (!initialized_) return;

    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_saturation(s, saturation);
    }
}

} // namespace EvoSpark
