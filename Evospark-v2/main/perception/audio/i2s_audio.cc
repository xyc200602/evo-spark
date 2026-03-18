#include "i2s_audio.h"
#include "esp_log.h"
#include <cstring>

namespace EvoSpark {

static const char* TAG = "I2SAudio";

I2SAudio::~I2SAudio() {
    if (initialized_) {
        i2s_driver_uninstall(port_);
    }
}

bool I2SAudio::Init(i2s_port_t port, const AudioConfig& config) {
    if (initialized_) {
        return true;
    }

    ESP_LOGI(TAG, "Initializing I2S on port %d...", port);

    port_ = port;
    config_ = config;

    // I2S 配置
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = config.sample_rate,
        .bits_per_sample = (i2s_bits_per_sample_t)config.bits_per_sample,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = config.dma_buf_count,
        .dma_buf_len = config.dma_buf_len,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    // I2S 引脚配置
    // ES8311 (扬声器) + ES7210 (麦克风)
    i2s_pin_config_t pin_config = {
        .bck_io_num = 36,      // I2S_BCLK
        .ws_io_num = 37,       // I2S_WS
        .data_out_num = 40,    // I2S_DOUT -> ES8311 DIN
        .data_in_num = 41      // I2S_DIN <- ES7210 DOUT
    };

    esp_err_t ret = i2s_driver_install(port, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(ret));
        return false;
    }

    ret = i2s_set_pin(port, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(port);
        return false;
    }

    ret = i2s_set_clk(port, config.sample_rate, 
                      (i2s_bits_per_sample_t)config.bits_per_sample,
                      I2S_CHANNEL_MONO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S clock: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(port);
        return false;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "I2S initialized: %d Hz, %d bits", config.sample_rate, config.bits_per_sample);
    return true;
}

size_t I2SAudio::Read(uint8_t* data, size_t len, uint32_t timeout_ms) {
    if (!initialized_) return 0;

    size_t bytes_read = 0;
    esp_err_t ret = i2s_read(port_, data, len, &bytes_read, pdMS_TO_TICKS(timeout_ms));
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2S read failed: %s", esp_err_to_name(ret));
        return 0;
    }

    return bytes_read;
}

size_t I2SAudio::Write(const uint8_t* data, size_t len, uint32_t timeout_ms) {
    if (!initialized_) return 0;

    size_t bytes_written = 0;
    esp_err_t ret = i2s_write(port_, data, len, &bytes_written, pdMS_TO_TICKS(timeout_ms));
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2S write failed: %s", esp_err_to_name(ret));
        return 0;
    }

    return bytes_written;
}

bool I2SAudio::StartRecording() {
    if (!initialized_) return false;
    i2s_zero_dma_buffer(port_);
    return true;
}

bool I2SAudio::StopRecording() {
    return true;
}

bool I2SAudio::StartPlayback() {
    if (!initialized_) return false;
    i2s_zero_dma_buffer(port_);
    return true;
}

bool I2SAudio::StopPlayback() {
    if (!initialized_) return false;
    i2s_zero_dma_buffer(port_);
    return true;
}

} // namespace EvoSpark
