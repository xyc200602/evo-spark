#include "button.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <chrono>

namespace EvoSpark {

static const char* TAG = "Button";

Button::Button(gpio_num_t gpio, bool active_low)
    : gpio_(gpio), active_low_(active_low), last_state_(false), press_start_time_(0) {

    // 配置 GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    gpio_config(&io_conf);

    // 创建防抖定时器
    esp_timer_create_args_t timer_args = {
        .callback = &DebounceTimerCallback,
        .arg = this,
        .name = "button_debounce"
    };

    esp_timer_create(&timer_args, &debounce_timer_);

    // 安装中断服务
    gpio_install_isr_service(0);
    gpio_isr_handler_add(gpio_, GPIOISRHandler, this);

    ESP_LOGI(TAG, "Button initialized on GPIO %d", gpio_);
}

Button::~Button() {
    gpio_isr_handler_remove(gpio_);
    esp_timer_delete(debounce_timer_);
}

void IRAM_ATTR Button::GPIOISRHandler(void* arg) {
    Button* self = static_cast<Button*>(arg);
    self->HandleInterrupt();
}

void Button::HandleInterrupt() {
    // 启动防抖定时器
    esp_timer_start_once(debounce_timer_, 50000);  // 50ms
}

void Button::DebounceTimerCallback(void* arg) {
    Button* self = static_cast<Button*>(arg);
    self->ProcessButton();
}

void Button::ProcessButton() {
    int level = gpio_get_level(gpio_);
    bool current_state = active_low_ ? (level == 0) : (level == 1);

    // 检测按下（上升沿）
    if (current_state && !last_state_) {
        press_start_time_ = esp_timer_get_time() / 1000;  // 转换为毫秒
        ESP_LOGD(TAG, "Button pressed");
    }

    // 检测释放（下降沿）
    if (!current_state && last_state_) {
        uint32_t press_duration = (esp_timer_get_time() / 1000) - press_start_time_;

        if (press_duration >= 1000 && long_press_callback_) {
            // 长按
            ESP_LOGI(TAG, "Long press detected (%lu ms)", press_duration);
            long_press_callback_();
        } else if (press_callback_) {
            // 短按
            ESP_LOGI(TAG, "Short press detected (%lu ms)", press_duration);
            press_callback_();
        }
    }

    last_state_ = current_state;
}

} // namespace EvoSpark
