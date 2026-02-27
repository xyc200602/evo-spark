#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <string>
#include <esp_http_server.h>
#include <esp_timer.h>
#include "esp_log.h"
#include "../memory/memory_types.h"

namespace EvoSpark {

// 监控数据
struct MonitorData {
    size_t conversation_count;
    size_t buffer_size;
    size_t free_space_kb;
    size_t used_space_kb;
    bool is_idle;
    std::string last_update;

    std::string to_json() const;
};

class WebServer {
public:
    static WebServer& GetInstance() {
        static WebServer instance;
        return instance;
    }

    bool Start();
    void Stop();

    // 获取服务器地址
    std::string GetURL() const { return "http://" + ip_address_ + ":80"; }

private:
    WebServer();
    ~WebServer();

    // HTTP 处理器
    static esp_err_t index_html_handler(httpd_req_t *req);
    static esp_err_t config_html_handler(httpd_req_t *req);
    static esp_err_t api_config_handler(httpd_req_t *req);
    static esp_err_t api_restart_handler(httpd_req_t *req);
    static esp_err_t api_conversation_handler(httpd_req_t *req);
    static esp_err_t api_memory_handler(httpd_req_t *req);
    static esp_err_t api_status_handler(httpd_req_t *req);
    static esp_err_t api_rollback_handler(httpd_req_t *req);

    // 辅助方法
    void setup_http_handlers();

    httpd_handle_t server_;
    bool is_running_;
    std::string ip_address_;

    // 监控定时器 - 已移除，前端使用 HTTP 轮询
    // esp_timer_handle_t monitor_timer_;
    // void start_monitor_timer();
    // void stop_monitor_timer();
    // void on_monitor_timer();
};

} // namespace EvoSpark

#endif // WEB_SERVER_H
