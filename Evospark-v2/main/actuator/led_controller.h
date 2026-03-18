#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <cstdint>

namespace EvoSpark {

// LED 控制器 - 用于状态指示
class LEDController {
public:
    static LEDController& GetInstance() {
        static LEDController instance;
        return instance;
    }

    // 初始化
    bool Init(uint8_t gpio);

    // 设置颜色 (RGB)
    void SetColor(uint8_t r, uint8_t g, uint8_t b);

    // 设置亮度 (0-255)
    void SetBrightness(uint8_t brightness);

    // 开/关
    void On();
    void Off();

    // 闪烁效果
    void Blink(uint8_t r, uint8_t g, uint8_t b, uint32_t interval_ms = 500);

    // 呼吸效果
    void Breathe(uint8_t r, uint8_t g, uint8_t b, uint32_t period_ms = 2000);

    // 停止效果
    void StopEffect();

private:
    LEDController() = default;
    ~LEDController();

    static void BlinkTimerCallback(void* arg);
    static void BreatheTimerCallback(void* arg);

    uint8_t gpio_ = 0;
    uint8_t brightness_ = 255;
    bool initialized_ = false;
    bool is_on_ = false;

    // 效果控制
    esp_timer_handle_t effect_timer_ = nullptr;
    bool blink_state_ = false;
    uint32_t breathe_phase_ = 0;
    uint8_t target_r_ = 0, target_g_ = 0, target_b_ = 0;
};

} // namespace EvoSpark

#endif // LED_CONTROLLER_H
