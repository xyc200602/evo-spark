#ifndef SPEAKER_H
#define SPEAKER_H

#include <vector>
#include <functional>
#include "i2s_audio.h"
#include "freertos/task.h"

namespace EvoSpark {

// 扬声器管理器
class Speaker {
public:
    using PlaybackCallback = std::function<void()>;

    static Speaker& GetInstance() {
        static Speaker instance;
        return instance;
    }

    // 初始化
    bool Init();

    // 播放音频数据 (PCM 16-bit)
    bool Play(const std::vector<int16_t>& audio_data);

    // 播放音频数据 (原始字节)
    bool Play(const uint8_t* data, size_t len);

    // 停止播放
    void Stop();

    // 设置音量 (0-100)
    void SetVolume(int volume);

    // 获取音量
    int GetVolume() const { return volume_; }

    // 是否正在播放
    bool IsPlaying() const { return is_playing_; }

    // 设置播放完成回调
    void SetPlaybackCallback(PlaybackCallback cb) { playback_callback_ = cb; }

private:
    Speaker() = default;
    ~Speaker();

    static void PlaybackTask(void* arg);
    void ProcessPlayback();

    I2SAudio& i2s_ = I2SAudio::GetInstance();

    TaskHandle_t playback_task_ = nullptr;
    bool is_playing_ = false;
    int volume_ = 80;
    PlaybackCallback playback_callback_;

    // 播放缓冲区
    std::vector<uint8_t> audio_buffer_;
    size_t playback_position_ = 0;
};

} // namespace EvoSpark

#endif // SPEAKER_H
