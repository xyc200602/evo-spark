#include "lcd_driver.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <cstring>

namespace EvoSpark {

static const char* TAG = "LCD";

// SPI 引脚定义
#define LCD_SPI_HOST    SPI3_HOST
#define LCD_PIN_MOSI    47
#define LCD_PIN_CLK     48
#define LCD_PIN_CS      1
#define LCD_PIN_DC      2
#define LCD_PIN_RST     3
#define LCD_PIN_BL      42

// SPI 设备句柄
static spi_device_handle_t spi_handle = nullptr;

LCDDriver::~LCDDriver() {
    if (spi_handle) {
        spi_bus_remove_device(spi_handle);
    }
}

bool LCDDriver::Init() {
    if (initialized_) {
        return true;
    }

    ESP_LOGI(TAG, "Initializing LCD (ST7789)...");

    // 配置 SPI 总线
    spi_bus_config_t bus_config = {
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = width_ * height_ * 2
    };

    esp_err_t ret = spi_bus_initialize(LCD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    }

    // 配置 SPI 设备
    spi_device_interface_config_t dev_config = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = 40 * 1000 * 1000,  // 40MHz
        .input_delay_ns = 0,
        .spics_io_num = LCD_PIN_CS,
        .flags = 0,
        .queue_size = 7,
        .pre_cb = nullptr,
        .post_cb = nullptr
    };

    ret = spi_bus_add_device(LCD_SPI_HOST, &dev_config, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(LCD_SPI_HOST);
        return false;
    }

    // 配置 GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_PIN_DC) | (1ULL << LCD_PIN_RST) | (1ULL << LCD_PIN_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 硬件复位
    gpio_set_level((gpio_num_t)LCD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)LCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // 初始化序列
    SendCommand(0x11);  // Sleep out
    vTaskDelay(pdMS_TO_TICKS(120));

    SendCommand(0x36);  // MADCTL
    uint8_t madctl = 0x00;
    SendData(&madctl, 1);

    SendCommand(0x3A);  // COLMOD
    uint8_t colmod = 0x55;  // 16-bit
    SendData(&colmod, 1);

    SendCommand(0x21);  // Inversion on
    SendCommand(0x13);  // Normal display on
    SendCommand(0x29);  // Display on

    // 设置背光
    SetBacklight(128);

    initialized_ = true;
    ESP_LOGI(TAG, "LCD initialized (%dx%d)", width_, height_);
    return true;
}

void LCDDriver::SendCommand(uint8_t cmd) {
    gpio_set_level((gpio_num_t)LCD_PIN_DC, 0);  // Command mode

    spi_transaction_t trans = {};
    trans.length = 8;
    trans.tx_buffer = &cmd;
    spi_device_transmit(spi_handle, &trans);
}

void LCDDriver::SendData(const uint8_t* data, size_t len) {
    if (len == 0) return;

    gpio_set_level((gpio_num_t)LCD_PIN_DC, 1);  // Data mode

    spi_transaction_t trans = {};
    trans.length = len * 8;
    trans.tx_buffer = data;
    spi_device_transmit(spi_handle, &trans);
}

void LCDDriver::SetWindow(int x, int y, int width, int height) {
    uint8_t data[4];

    // Column address
    SendCommand(0x2A);
    data[0] = (x >> 8) & 0xFF;
    data[1] = x & 0xFF;
    data[2] = ((x + width - 1) >> 8) & 0xFF;
    data[3] = (x + width - 1) & 0xFF;
    SendData(data, 4);

    // Row address
    SendCommand(0x2B);
    data[0] = (y >> 8) & 0xFF;
    data[1] = y & 0xFF;
    data[2] = ((y + height - 1) >> 8) & 0xFF;
    data[3] = (y + height - 1) & 0xFF;
    SendData(data, 4);

    // Write to RAM
    SendCommand(0x2C);
}

void LCDDriver::SetBacklight(uint8_t brightness) {
    // PWM 控制
    // TODO: 使用 LEDC PWM
    gpio_set_level((gpio_num_t)LCD_PIN_BL, brightness > 0 ? 1 : 0);
}

void LCDDriver::Clear(uint16_t color) {
    FillRect(0, 0, width_, height_, color);
}

void LCDDriver::DrawPixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;

    SetWindow(x, y, 1, 1);
    SendData((uint8_t*)&color, 2);
}

void LCDDriver::FillRect(int x, int y, int width, int height, uint16_t color) {
    if (x < 0 || y < 0 || width <= 0 || height <= 0) return;

    SetWindow(x, y, width, height);

    // 分块发送
    size_t chunk_size = 1024;
    std::vector<uint16_t> buffer(chunk_size, color);

    size_t total_pixels = width * height;
    for (size_t i = 0; i < total_pixels; i += chunk_size) {
        size_t pixels = std::min(chunk_size, total_pixels - i);
        SendData((uint8_t*)buffer.data(), pixels * 2);
    }
}

void LCDDriver::DrawImage(int x, int y, int width, int height, const uint16_t* data) {
    if (!data || width <= 0 || height <= 0) return;

    SetWindow(x, y, width, height);
    SendData((const uint8_t*)data, width * height * 2);
}

} // namespace EvoSpark
