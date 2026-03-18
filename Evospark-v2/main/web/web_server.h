#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <string>
#include <functional>
#include "esp_http_server.h"

namespace EvoSpark {

class WebServer {
public:
    static WebServer& GetInstance() {
        static WebServer instance;
        return instance;
    }

    // 启动服务器
    bool Start();

    // 停止服务器
    void Stop();

    // 是否运行中
    bool IsRunning() const { return server_ != nullptr; }

private:
    WebServer() = default;
    ~WebServer();

    // HTTP 处理函数
    static esp_err_t HandleRoot(httpd_req_t *req);
    static esp_err_t HandleConfig(httpd_req_t *req);
    static esp_err_t HandleApiStatus(httpd_req_t *req);
    static esp_err_t HandleApiConfig(httpd_req_t *req);
    static esp_err_t HandleApiMemory(httpd_req_t *req);

    httpd_handle_t server_ = nullptr;
};

} // namespace EvoSpark

#endif // WEB_SERVER_H
