#ifndef I2S_AUDIO_H
#define I2S_AUDIO_H

#include <cstdint>
#include <vector>
#include <functional>
#include "driver/i2s.h"

namespace EvoSpark {

// 音频配置
struct AudioConfig {
    int sample_rate = 16000;
    int bits_per_sample = 16;
    int channels = 1;
    int dma_buf_count = 8;
    int dma_buf_len = 1024;
};

// I2S 音频驱动
class I2SAudio {
public:
    static I2SAudio& GetInstance() {
        static I2SAudio instance;
        return instance;
    }

    // 初始化 I2S
    bool Init(i2s_port_t port, const AudioConfig& config);

    // 读取音频数据（麦克风）
    size_t Read(uint8_t* data, size_t len, uint32_t timeout_ms = 1000);

    // 写入音频数据（扬声器）
    size_t Write(const uint8_t* data, size_t len, uint32_t timeout_ms = 1000);

    // 开始录音
    bool StartRecording();

    // 停止录音
    bool StopRecording();

    // 开始播放
    bool StartPlayback();

    // 停止播放
    bool StopPlayback();

    // 获取配置
    const AudioConfig& GetConfig() const { return config_; }

private:
    I2SAudio() = default;
    ~I2SAudio();

    i2s_port_t port_ = I2S_NUM_0;
    AudioConfig config_;
    bool initialized_ = false;
};

} // namespace EvoSpark

#endif // I2S_AUDIO_H
