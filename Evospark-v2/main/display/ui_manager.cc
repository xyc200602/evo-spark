#include "ui_manager.h"
#include "esp_log.h"
#include <sstream>

namespace EvoSpark {

static const char* TAG = "UI";

// 颜色定义 (RGB565)
constexpr uint16_t COLOR_BG         = 0x0000;  // 黑色
constexpr uint16_t COLOR_TEXT       = 0xFFFF;  // 白色
constexpr uint16_t COLOR_USER       = 0x07E0;  // 绿色
constexpr uint16_t COLOR_ASSISTANT  = 0x07FF;  // 青色
constexpr uint16_t COLOR_STATUS     = 0xF800;  // 红色

bool UIManager::Init() {
    if (initialized_) {
        return true;
    }

    ESP_LOGI(TAG, "Initializing UI manager...");

    if (!lcd_.Init()) {
        ESP_LOGE(TAG, "Failed to initialize LCD");
        return false;
    }

    // 清屏
    Clear();

    // 显示欢迎界面
    ShowText("EvoSpark v2.0\n\nPress button to start");

    initialized_ = true;
    ESP_LOGI(TAG, "UI manager initialized");
    return true;
}

void UIManager::ShowText(const std::string& text) {
    current_text_ = text;

    // 清除内容区
    lcd_.FillRect(0, 20, lcd_.GetWidth(), lcd_.GetHeight() - 20, COLOR_BG);

    // TODO: 使用 LVGL 或字库渲染文本
    // 临时方案：显示简单提示

    ESP_LOGI(TAG, "Text: %s", text.c_str());
}

void UIManager::ShowConversation(const std::string& user, const std::string& assistant) {
    std::ostringstream oss;
    oss << "You: " << user << "\n\nEvoSpark: " << assistant;
    ShowText(oss.str());
}

void UIManager::ShowStatus(SessionState state) {
    current_state_ = state;

    std::string status_text;
    uint16_t color = COLOR_TEXT;

    switch (state) {
        case SessionState::IDLE:
            status_text = "IDLE";
            color = 0x7BEF;  // 灰色
            break;
        case SessionState::LISTENING:
            status_text = "LISTENING...";
            color = COLOR_USER;
            break;
        case SessionState::PROCESSING:
            status_text = "THINKING...";
            color = 0xFFE0;  // 黄色
            break;
        case SessionState::SPEAKING:
            status_text = "SPEAKING...";
            color = COLOR_ASSISTANT;
            break;
        case SessionState::CLOSING:
            status_text = "CLOSING...";
            color = COLOR_STATUS;
            break;
        default:
            status_text = "UNKNOWN";
            break;
    }

    // 绘制状态栏
    lcd_.FillRect(0, 0, lcd_.GetWidth(), 18, color);

    ESP_LOGI(TAG, "Status: %s", status_text.c_str());
}

void UIManager::ShowExpression(const std::string& expression) {
    // TODO: 显示表情动画
    ESP_LOGI(TAG, "Expression: %s", expression.c_str());

    if (expression == "happy") {
        // 显示开心表情
    } else if (expression == "thinking") {
        // 显示思考表情
    } else if (expression == "confused") {
        // 显示困惑表情
    }
}

void UIManager::ShowDetectedObjects(const std::vector<DetectedObject>& objects) {
    if (objects.empty()) {
        ShowText("No objects detected");
        return;
    }

    std::ostringstream oss;
    oss << "Detected:\n";
    for (const auto& obj : objects) {
        oss << "- " << obj.label << " (" << (int)(obj.confidence * 100) << "%)\n";
    }

    ShowText(oss.str());
}

void UIManager::Clear() {
    lcd_.Clear(COLOR_BG);
    current_text_.clear();
}

void UIManager::Refresh() {
    if (!current_text_.empty()) {
        ShowText(current_text_);
    }
    ShowStatus(current_state_);
}

void UIManager::DrawStatusBar() {
    lcd_.FillRect(0, 0, lcd_.GetWidth(), 18, 0x1082);  // 深蓝色
}

void UIManager::DrawContentArea() {
    lcd_.FillRect(0, 20, lcd_.GetWidth(), lcd_.GetHeight() - 20, COLOR_BG);
}

} // namespace EvoSpark
