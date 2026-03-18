#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <cstdint>

namespace EvoSpark {

// LCD 驱动器 - ST7789 SPI
class LCDDriver {
public:
    static LCDDriver& GetInstance() {
        static LCDDriver instance;
        return instance;
    }

    // 初始化 LCD
    bool Init();

    // 设置背光 (0-255)
    void SetBacklight(uint8_t brightness);

    // 清屏
    void Clear(uint16_t color = 0x0000);

    // 绘制像素
    void DrawPixel(int x, int y, uint16_t color);

    // 绘制矩形区域
    void FillRect(int x, int y, int width, int height, uint16_t color);

    // 绘制图像（RGB565）
    void DrawImage(int x, int y, int width, int height, const uint16_t* data);

    // 获取屏幕尺寸
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

    // 是否已初始化
    bool IsInitialized() const { return initialized_; }

private:
    LCDDriver() = default;
    ~LCDDriver();

    // 发送命令
    void SendCommand(uint8_t cmd);

    // 发送数据
    void SendData(const uint8_t* data, size_t len);

    // 设置窗口
    void SetWindow(int x, int y, int width, int height);

    bool initialized_ = false;
    int width_ = 240;
    int height_ = 240;
};

} // namespace EvoSpark

#endif // LCD_DRIVER_H
