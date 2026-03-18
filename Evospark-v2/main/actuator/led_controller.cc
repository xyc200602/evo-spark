#include "led_controller.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include <cmath>

namespace EvoSpark {

static const char* TAG = "LED";

// WS2812B RMT 配置
#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_CLK_DIV 2

LEDController::~LEDController() {
    if (effect_timer_) {
        esp_timer_delete(effect_timer_);
    }
}

bool LEDController::Init(uint8_t gpio) {
    if (initialized_) {
        return true;
    }

    gpio_ = gpio;

    // 配置 RMT
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX((gpio_num_t)gpio, RMT_TX_CHANNEL);
    config.clk_div = RMT_CLK_DIV;

    esp_err_t ret = rmt_config(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure RMT: %s", esp_err_to_name(ret));
        return false;
    }

    ret = rmt_driver_install(RMT_TX_CHANNEL, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install RMT driver: %s", esp_err_to_name(ret));
        return false;
    }

    // 创建效果定时器
    esp_timer_create_args_t timer_args = {
        .callback = nullptr,  // 稍后设置
        .arg = this,
        .name = "led_effect"
    };

    initialized_ = true;
    ESP_LOGI(TAG, "LED controller initialized on GPIO %d", gpio);
    return true;
}

void LEDController::SetColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!initialized_) return;

    // 应用亮度
    r = (r * brightness_) / 255;
    g = (g * brightness_) / 255;
    b = (b * brightness_) / 255;

    // WS2812B 格式: GRB
    uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;

    // 转换为 RMT 项目
    rmt_item32_t items[24];
    for (int i = 23; i >= 0; i--) {
        if (color & (1 << i)) {
            // 1 bit: 高电平 0.85us, 低电平 0.4us
            items[23 - i] = { {
                .duration0 = 17,  // 0.85us @ 20MHz
                .level0 = 1,
                .duration1 = 8,   // 0.4us
                .level1 = 0
            }};
        } else {
            // 0 bit: 高电平 0.4us, 低电平 0.85us
            items[23 - i] = { {
                .duration0 = 8,
                .level0 = 1,
                .duration1 = 17,
                .level1 = 0
            }};
        }
    }

    rmt_write_items(RMT_TX_CHANNEL, items, 24, true);
    is_on_ = true;
}

void LEDController::SetBrightness(uint8_t brightness) {
    brightness_ = brightness;
}

void LEDController::On() {
    SetColor(255, 255, 255);
}

void LEDController::Off() {
    SetColor(0, 0, 0);
    is_on_ = false;
}

void LEDController::Blink(uint8_t r, uint8_t g, uint8_t b, uint32_t interval_ms) {
    StopEffect();

    target_r_ = r;
    target_g_ = g;
    target_b_ = b;
    blink_state_ = true;

    // TODO: 实现定时器闪烁
    SetColor(r, g, b);
}

void LEDController::Breathe(uint8_t r, uint8_t g, uint8_t b, uint32_t period_ms) {
    StopEffect();

    target_r_ = r;
    target_g_ = g;
    target_b_ = b;

    // TODO: 实现呼吸效果
}

void LEDController::StopEffect() {
    if (effect_timer_) {
        esp_timer_stop(effect_timer_);
    }
}

void LEDController::BlinkTimerCallback(void* arg) {
    LEDController* self = static_cast<LEDController*>(arg);
    self->blink_state_ = !self->blink_state_;

    if (self->blink_state_) {
        self->SetColor(self->target_r_, self->target_g_, self->target_b_);
    } else {
        self->SetColor(0, 0, 0);
    }
}

void LEDController::BreatheTimerCallback(void* arg) {
    // TODO: 实现呼吸计算
}

} // namespace EvoSpark
