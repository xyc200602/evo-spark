#include "speaker.h"
#include "esp_log.h"
#include <algorithm>

namespace EvoSpark {

static const char* TAG = "Speaker";

Speaker::~Speaker() {
    Stop();
}

bool Speaker::Init() {
    ESP_LOGI(TAG, "Initializing speaker...");

    // I2S 已由 Microphone 初始化，这里只检查
    // 实际项目中可能需要单独的 I2S 端口

    ESP_LOGI(TAG, "Speaker initialized");
    return true;
}

bool Speaker::Play(const std::vector<int16_t>& audio_data) {
    if (audio_data.empty()) {
        return false;
    }

    return Play((const uint8_t*)audio_data.data(), audio_data.size() * sizeof(int16_t));
}

bool Speaker::Play(const uint8_t* data, size_t len) {
    if (is_playing_) {
        ESP_LOGW(TAG, "Already playing, stop first");
        Stop();
    }

    ESP_LOGI(TAG, "Playing %zu bytes...", len);

    // 复制音频数据
    audio_buffer_.assign(data, data + len);
    playback_position_ = 0;

    // 应用音量
    // TODO: 实现音量调节

    is_playing_ = true;

    // 创建播放任务
    xTaskCreate(
        PlaybackTask,
        "speaker_playback",
        4096,
        this,
        5,
        &playback_task_
    );

    return true;
}

void Speaker::Stop() {
    if (!is_playing_) {
        return;
    }

    ESP_LOGI(TAG, "Stopping playback...");

    is_playing_ = false;

    if (playback_task_) {
        vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelete(playback_task_);
        playback_task_ = nullptr;
    }

    i2s_.StopPlayback();
    audio_buffer_.clear();
    playback_position_ = 0;

    ESP_LOGI(TAG, "Playback stopped");
}

void Speaker::SetVolume(int volume) {
    volume_ = std::max(0, std::min(100, volume));
    ESP_LOGI(TAG, "Volume set to %d", volume_);
}

void Speaker::PlaybackTask(void* arg) {
    Speaker* self = static_cast<Speaker*>(arg);
    self->ProcessPlayback();
}

void Speaker::ProcessPlayback() {
    const size_t CHUNK_SIZE = 1024;

    i2s_.StartPlayback();

    while (is_playing_ && playback_position_ < audio_buffer_.size()) {
        size_t remaining = audio_buffer_.size() - playback_position_;
        size_t chunk_size = std::min(CHUNK_SIZE, remaining);

        size_t written = i2s_.Write(
            &audio_buffer_[playback_position_],
            chunk_size,
            100
        );

        if (written == 0) {
            ESP_LOGW(TAG, "Failed to write audio data");
            break;
        }

        playback_position_ += written;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    is_playing_ = false;

    if (playback_callback_) {
        playback_callback_();
    }

    vTaskDelete(nullptr);
}

} // namespace EvoSpark
