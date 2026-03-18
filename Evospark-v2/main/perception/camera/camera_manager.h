#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <vector>
#include <cstdint>
#include "esp_camera.h"

namespace EvoSpark {

// 图像帧
struct ImageFrame {
    uint8_t* data;
    size_t size;
    int width;
    int height;
    pixformat_t format;

    ImageFrame() : data(nullptr), size(0), width(0), height(0), format(PIXFORMAT_JPEG) {}
    
    bool IsValid() const { return data != nullptr && size > 0; }
    
    void Free() {
        if (data) {
            free(data);
            data = nullptr;
        }
    }
};

// 摄像头管理器
class CameraManager {
public:
    static CameraManager& GetInstance() {
        static CameraManager instance;
        return instance;
    }

    // 初始化摄像头
    bool Init();

    // 捕获一帧图像
    ImageFrame Capture();

    // 捕获 JPEG 图像
    std::vector<uint8_t> CaptureJPEG(int quality = 12);

    // 开始连续捕获
    bool StartStreaming();

    // 停止连续捕获
    void StopStreaming();

    // 是否已初始化
    bool IsInitialized() const { return initialized_; }

    // 设置参数
    void SetBrightness(int brightness);  // -2 to 2
    void SetContrast(int contrast);      // -2 to 2
    void SetSaturation(int saturation);  // -2 to 2

private:
    CameraManager() = default;
    ~CameraManager();

    bool initialized_ = false;
    bool streaming_ = false;
};

} // namespace EvoSpark

#endif // CAMERA_MANAGER_H
