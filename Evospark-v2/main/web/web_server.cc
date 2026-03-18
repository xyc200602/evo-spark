#include "web_server.h"
#include "esp_log.h"
#include "core/session_manager.h"
#include "memory/memory_manager.h"
#include "config/config_manager.h"
#include <sstream>

namespace EvoSpark {

static const char* TAG = "WebServer";

WebServer::~WebServer() {
    Stop();
}

bool WebServer::Start() {
    if (server_) {
        ESP_LOGW(TAG, "Web server already running");
        return true;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 10;
    config.stack_size = 8192;

    esp_err_t ret = httpd_start(&server_, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        return false;
    }

    // 注册 URI 处理函数
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = HandleRoot,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &root_uri);

    httpd_uri_t config_uri = {
        .uri = "/config",
        .method = HTTP_GET,
        .handler = HandleConfig,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &config_uri);

    httpd_uri_t api_status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = HandleApiStatus,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &api_status_uri);

    httpd_uri_t api_config_uri = {
        .uri = "/api/config",
        .method = HTTP_POST,
        .handler = HandleApiConfig,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &api_config_uri);

    httpd_uri_t api_memory_uri = {
        .uri = "/api/memory",
        .method = HTTP_GET,
        .handler = HandleApiMemory,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &api_memory_uri);

    ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
    return true;
}

void WebServer::Stop() {
    if (server_) {
        httpd_stop(server_);
        server_ = nullptr;
        ESP_LOGI(TAG, "Web server stopped");
    }
}

esp_err_t WebServer::HandleRoot(httpd_req_t *req) {
    const char* html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>EvoSpark v2</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }
        h1 { color: #333; }
        .status { padding: 10px; margin: 10px 0; background: #e8f5e9; border-radius: 4px; }
        .config-link { margin: 20px 0; }
        a { color: #2196F3; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🤖 EvoSpark v2</h1>
        <div class="status">
            <strong>状态:</strong> <span id="state">加载中...</span>
        </div>
        <div class="config-link">
            <a href="/config">⚙️ 配置设置</a>
        </div>
        <div>
            <h3>快速开始</h3>
            <p>1. 按下设备上的按键唤醒</p>
            <p>2. 开始对话</p>
            <p>3. 再次按键或等待超时结束会话</p>
        </div>
    </div>
    <script>
        setInterval(() => {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('state').textContent = data.state;
                });
        }, 2000);
    </script>
</body>
</html>
)";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

esp_err_t WebServer::HandleConfig(httpd_req_t *req) {
    const char* html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>EvoSpark 配置</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }
        h1 { color: #333; }
        .form-group { margin: 15px 0; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="text"], input[type="password"] { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
        button { background: #2196F3; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background: #1976D2; }
    </style>
</head>
<body>
    <div class="container">
        <h1>⚙️ EvoSpark 配置</h1>
        <form id="configForm">
            <div class="form-group">
                <label>WiFi SSID</label>
                <input type="text" id="ssid" required>
            </div>
            <div class="form-group">
                <label>WiFi 密码</label>
                <input type="password" id="password">
            </div>
            <div class="form-group">
                <label>API Key (GLM)</label>
                <input type="text" id="apiKey" required>
            </div>
            <button type="submit">保存配置</button>
        </form>
        <p><a href="/">← 返回首页</a></p>
    </div>
    <script>
        document.getElementById('configForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const data = {
                ssid: document.getElementById('ssid').value,
                password: document.getElementById('password').value,
                api_key: document.getElementById('apiKey').value
            };
            const response = await fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(data)
            });
            if (response.ok) {
                alert('配置已保存，设备将重启');
            } else {
                alert('保存失败');
            }
        });
    </script>
</body>
</html>
)";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

esp_err_t WebServer::HandleApiStatus(httpd_req_t *req) {
    SessionManager& session = SessionManager::GetInstance();

    std::ostringstream json;
    json << "{";
    json << "\"state\":\"" << StateToString(session.GetState()) << "\",";
    json << "\"in_session\":" << (session.InSession() ? "true" : "false") << ",";
    json << "\"message_count\":" << session.GetStats().message_count;
    json << "}";

    std::string response = json.str();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

esp_err_t WebServer::HandleApiConfig(httpd_req_t *req) {
    char buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // TODO: 使用 JSON 库解析
    // 临时简单解析
    std::string body(buf);

    auto extract = [&body](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t start = body.find(search);
        if (start == std::string::npos) return "";
        start += search.length();
        size_t end = body.find("\"", start);
        return body.substr(start, end - start);
    };

    std::string ssid = extract("ssid");
    std::string password = extract("password");
    std::string api_key = extract("api_key");

    ConfigManager& config = ConfigManager::GetInstance();
    esp_err_t err = config.SetConfig(ssid, password, api_key);

    if (err == ESP_OK) {
        httpd_resp_send(req, "{\"success\":true}", 16);

        // 延迟重启
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save");
    }

    return ESP_OK;
}

esp_err_t WebServer::HandleApiMemory(httpd_req_t *req) {
    MemoryManager& memory = MemoryManager::GetInstance();
    CompressedMemory mem = memory.LoadMemory();

    std::string json = mem.raw_json.empty() ? "{}" : mem.raw_json;

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
}

} // namespace EvoSpark
