#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <vector>
#include <functional>
#include "i2s_audio.h"
#include "freertos/task.h"
#include "freertos/queue.h"

namespace EvoSpark {

// 麦克风管理器
class Microphone {
public:
    using AudioCallback = std::function<void(const std::vector<int16_t>& audio_data)>;

    static Microphone& GetInstance() {
        static Microphone instance;
        return instance;
    }

    // 初始化
    bool Init();

    // 开始录音
    bool StartRecording();

    // 停止录音
    void StopRecording();

    // 设置音频回调
    void SetAudioCallback(AudioCallback cb) { audio_callback_ = cb; }

    // 是否正在录音
    bool IsRecording() const { return is_recording_; }

    // 获取当前音量级别 (0-100)
    int GetVolumeLevel();

private:
    Microphone() = default;
    ~Microphone();

    static void RecordingTask(void* arg);
    void ProcessRecording();

    I2SAudio& i2s_ = I2SAudio::GetInstance();
    
    TaskHandle_t recording_task_ = nullptr;
    bool is_recording_ = false;
    AudioCallback audio_callback_;

    // 录音缓冲区
    static constexpr size_t BUFFER_SIZE = 4096;
    int16_t* buffer_ = nullptr;
};

} // namespace EvoSpark

#endif // MICROPHONE_H
