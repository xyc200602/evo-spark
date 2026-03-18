#ifndef BUTTON_H
#define BUTTON_H

#include <functional>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "event_bus.h"

namespace EvoSpark {

// 按键配置
struct ButtonConfig {
    gpio_num_t gpio;              // GPIO 引脚
    bool active_low = true;       // 低电平触发
    uint32_t debounce_ms = 50;    // 防抖时间
    uint32_t long_press_ms = 1000; // 长按时间
};

// 按键类
class Button {
public:
    Button(gpio_num_t gpio, bool active_low = true);
    ~Button();

    // 设置短按回调
    using PressCallback = std::function<void()>;
    void SetPressCallback(PressCallback cb) { press_callback_ = cb; }

    // 设置长按回调
    void SetLongPressCallback(PressCallback cb) { long_press_callback_ = cb; }

private:
    static void IRAM_ATTR GPIOISRHandler(void* arg);
    void HandleInterrupt();
    static void DebounceTimerCallback(void* arg);
    void ProcessButton();

    gpio_num_t gpio_;
    bool active_low_;
    bool last_state_;
    esp_timer_handle_t debounce_timer_;
    uint32_t press_start_time_;

    PressCallback press_callback_;
    PressCallback long_press_callback_;
};

} // namespace EvoSpark

#endif // BUTTON_H
