#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <string>
#include <functional>
#include "lcd_driver.h"
#include "memory_types.h"

namespace EvoSpark {

// UI 管理器
class UIManager {
public:
    static UIManager& GetInstance() {
        static UIManager instance;
        return instance;
    }

    // 初始化
    bool Init();

    // 显示文本
    void ShowText(const std::string& text);

    // 显示对话（用户 + 助手）
    void ShowConversation(const std::string& user, const std::string& assistant);

    // 显示状态
    void ShowStatus(SessionState state);

    // 显示表情
    void ShowExpression(const std::string& expression);

    // 显示检测到的物体
    void ShowDetectedObjects(const std::vector<DetectedObject>& objects);

    // 清屏
    void Clear();

    // 刷新显示
    void Refresh();

private:
    UIManager() = default;
    ~UIManager() = default;

    // 绘制状态栏
    void DrawStatusBar();

    // 绘制内容区
    void DrawContentArea();

    LCDDriver& lcd_ = LCDDriver::GetInstance();
    bool initialized_ = false;

    // 当前显示内容
    std::string current_text_;
    SessionState current_state_ = SessionState::IDLE;
};

} // namespace EvoSpark

#endif // UI_MANAGER_H
