#include "microphone.h"
#include "esp_log.h"
#include <cmath>

namespace EvoSpark {

static const char* TAG = "Microphone";

Microphone::~Microphone() {
    StopRecording();
    if (buffer_) {
        delete[] buffer_;
    }
}

bool Microphone::Init() {
    ESP_LOGI(TAG, "Initializing microphone...");

    AudioConfig config = {
        .sample_rate = 16000,
        .bits_per_sample = 16,
        .channels = 1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024
    };

    if (!i2s_.Init(I2S_NUM_0, config)) {
        ESP_LOGE(TAG, "Failed to initialize I2S for microphone");
        return false;
    }

    buffer_ = new int16_t[BUFFER_SIZE / 2];

    ESP_LOGI(TAG, "Microphone initialized");
    return true;
}

bool Microphone::StartRecording() {
    if (is_recording_) {
        return true;
    }

    ESP_LOGI(TAG, "Starting recording...");

    if (!i2s_.StartRecording()) {
        ESP_LOGE(TAG, "Failed to start I2S recording");
        return false;
    }

    is_recording_ = true;

    // 创建录音任务
    xTaskCreate(
        RecordingTask,
        "mic_recording",
        4096,
        this,
        5,
        &recording_task_
    );

    ESP_LOGI(TAG, "Recording started");
    return true;
}

void Microphone::StopRecording() {
    if (!is_recording_) {
        return;
    }

    ESP_LOGI(TAG, "Stopping recording...");

    is_recording_ = false;

    if (recording_task_) {
        vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelete(recording_task_);
        recording_task_ = nullptr;
    }

    i2s_.StopRecording();
    ESP_LOGI(TAG, "Recording stopped");
}

void Microphone::RecordingTask(void* arg) {
    Microphone* self = static_cast<Microphone*>(arg);
    self->ProcessRecording();
}

void Microphone::ProcessRecording() {
    while (is_recording_) {
        size_t bytes_read = i2s_.Read((uint8_t*)buffer_, BUFFER_SIZE, 100);
        
        if (bytes_read > 0 && audio_callback_) {
            size_t samples = bytes_read / sizeof(int16_t);
            std::vector<int16_t> audio_data(buffer_, buffer_ + samples);
            audio_callback_(audio_data);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int Microphone::GetVolumeLevel() {
    if (!buffer_ || !is_recording_) {
        return 0;
    }

    // 读取一小段音频
    size_t bytes_read = i2s_.Read((uint8_t*)buffer_, 1024, 100);
    if (bytes_read == 0) {
        return 0;
    }

    // 计算 RMS
    size_t samples = bytes_read / sizeof(int16_t);
    double sum = 0;
    for (size_t i = 0; i < samples; i++) {
        sum += buffer_[i] * buffer_[i];
    }
    double rms = std::sqrt(sum / samples);

    // 转换为 0-100
    int level = std::min(100, (int)(rms / 32767.0 * 100));
    return level;
}

} // namespace EvoSpark
