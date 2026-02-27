#include "web_server.h"
#include "../memory/memory_manager.h"
#include "../config/config_manager.h"
#include <cstring>
#include <sys/socket.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include "esp_log.h"
#include "esp_netif.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace EvoSpark {

static const char* TAG = "WebServer";

// MonitorData 实现
std::string MonitorData::to_json() const {
    std::stringstream ss;
    ss << "{"
       << "\"conversation_count\":" << conversation_count << ","
       << "\"buffer_size\":" << buffer_size << ","
       << "\"free_space_kb\":" << free_space_kb << ","
       << "\"used_space_kb\":" << used_space_kb << ","
       << "\"is_idle\":" << (is_idle ? "true" : "false") << ","
       << "\"last_update\":\"" << last_update << "\""
       << "}";
    return ss.str();
}

// 内联 HTML（避免文件系统）
static const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Evo-spark 控制台</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Arial, sans-serif;
                 background: #1a1a2e; color: #eee; }
        .container { max-width: 1200px; margin: 0 auto; padding: 20px; display: grid;
                     grid-template-columns: 1fr 350px; gap: 20px; }
        @media (max-width: 900px) {
            .container { grid-template-columns: 1fr; }
        }
        .chat-panel { background: #16162a; border-radius: 12px; padding: 20px;
                      display: flex; flex-direction: column; height: calc(100vh - 40px); }
        .monitor-panel { background: #16162a; border-radius: 12px; padding: 20px; }
        h1 { color: #4cc9f0; margin-bottom: 20px; font-size: 24px; }
        h2 { color: #4cc9f0; margin: 15px 0 10px; font-size: 18px; }
        .chat-messages { flex: 1; overflow-y: auto; padding: 15px; background: #0f0f23;
                         border-radius: 8px; margin-bottom: 15px; }
        .message { margin-bottom: 15px; display: flex; }
        .message.user { justify-content: flex-end; }
        .message.assistant { justify-content: flex-start; }
        .message-content { max-width: 80%; padding: 12px 16px; border-radius: 12px;
                            line-height: 1.5; }
        .message.user .message-content { background: #4cc9f0; color: #000;
                                     border-bottom-right-radius: 2px; }
        .message.assistant .message-content { background: #2d2d44;
                                            border-bottom-left-radius: 2px; }
        .message-role { font-size: 12px; margin-bottom: 5px; opacity: 0.7; }
        .chat-input { display: flex; gap: 10px; }
        .chat-input input { flex: 1; padding: 12px 16px; border: 2px solid #4cc9f0;
                            border-radius: 8px; background: #0f0f23; color: #eee; font-size: 16px; }
        .chat-input input:focus { outline: none; border-color: #4cc9f0; }
        .chat-input button { padding: 12px 24px; background: #4cc9f0; color: #000;
                             border: none; border-radius: 8px; font-size: 16px; font-weight: bold;
                             cursor: pointer; transition: all 0.3s; }
        .chat-input button:hover { background: #3ab0d9; }
        .chat-input button:disabled { opacity: 0.5; cursor: not-allowed; }
        .status-card { background: #0f0f23; border-radius: 8px; padding: 15px;
                         margin-bottom: 15px; }
        .status-item { display: flex; justify-content: space-between;
                      margin-bottom: 10px; font-size: 14px; }
        .status-label { opacity: 0.7; }
        .status-value { color: #4cc9f0; font-weight: bold; }
        .memory-viewer { background: #0f0f23; border-radius: 8px; padding: 15px;
                           height: 300px; overflow-y: auto; font-family: monospace;
                           font-size: 12px; margin-bottom: 15px; }
        .memory-viewer pre { white-space: pre-wrap; word-wrap: break-word; }
        .backup-list { list-style: none; }
        .backup-item { background: #0f0f23; padding: 10px; border-radius: 8px;
                        margin-bottom: 8px; display: flex; justify-content: space-between;
                        align-items: center; }
        .backup-item button { padding: 6px 12px; background: #4cc9f0; color: #000;
                            border: none; border-radius: 4px; cursor: pointer; }
        .progress-bar { height: 8px; background: #0f0f23; border-radius: 4px;
                         overflow: hidden; margin-top: 5px; }
        .progress-fill { height: 100%; background: #4cc9f0; border-radius: 4px;
                        transition: width 0.3s; }
        .indicator { display: inline-block; width: 10px; height: 10px; border-radius: 50%;
                     margin-right: 8px; }
        .indicator.online { background: #4cc9f0; box-shadow: 0 0 10px #4cc9f0; }
        .indicator.offline { background: #666; }
        .typing-indicator { color: #666; font-size: 14px; margin: 10px 0;
                           display: none; }
    </style>
</head>
<body>
    <div class="container">
        <div class="chat-panel">
            <h1>💬 Evo-spark 对话</h1>
            <div class="chat-messages" id="chatMessages"></div>
            <div class="typing-indicator" id="typingIndicator">Evo-spark 正在输入...</div>
            <div class="chat-input">
                <input type="text" id="chatInput" placeholder="输入消息..." autocomplete="off">
                <button id="sendBtn">发送</button>
            </div>
        </div>
        <div class="monitor-panel">
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;">
                <h2 style="margin: 0;">📊 实时监控</h2>
                <a href="/config" style="color: #4cc9f0; text-decoration: none; font-size: 14px;">⚙️ 配置</a>
            </div>
            <div class="status-card">
                <div class="status-item">
                    <span class="status-label">系统状态</span>
                    <span><span class="indicator online" id="statusIndicator"></span>
                    <span class="status-value" id="systemStatus">在线</span></span>
                </div>
                <div class="progress-bar">
                    <div class="progress-fill" id="storageProgress" style="width: 10%"></div>
                </div>
                <div class="status-item">
                    <span class="status-label">存储使用</span>
                    <span class="status-value" id="storageUsage">0 KB / 0 KB</span>
                </div>
                <div class="status-item">
                    <span class="status-label">对话消息数</span>
                    <span class="status-value" id="messageCount">0</span>
                </div>
                <div class="status-item">
                    <span class="status-label">缓冲区大小</span>
                    <span class="status-value" id="bufferSize">0 KB</span>
                </div>
                <div class="status-item">
                    <span class="status-label">最后更新</span>
                    <span class="status-value" id="lastUpdate">-</span>
                </div>
            </div>
            <h2>💾 记忆包 <button onclick="refreshMemory()" style="font-size:12px;padding:4px 8px;margin-left:10px;">刷新</button></h2>
            <div class="memory-viewer" id="memoryViewer">
                <pre id="memoryContent">加载中...</pre>
            </div>
            <h2>🔄 历史版本</h2>
            <ul class="backup-list" id="backupList">
                <li class="backup-item">
                    <span>当前版本</span>
                </li>
            </ul>
        </div>
    </div>

    <script>
        const chatMessages = document.getElementById('chatMessages');
        const chatInput = document.getElementById('chatInput');
        const sendBtn = document.getElementById('sendBtn');

        // 发送消息
        function sendMessage() {
            const content = chatInput.value.trim();
            if (!content) return;

            addMessage('user', content);
            chatInput.value = '';
            sendBtn.disabled = true;

            fetch('/api/conversation', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ role: 'user', content: content })
            })
            .then(r => r.json())
            .then(data => {
                if (data.response) {
                    addMessage('assistant', data.response);
                }
            })
            .catch(err => {
                console.error('Error:', err);
            })
            .finally(() => {
                sendBtn.disabled = false;
            });
        }

        function addMessage(role, content) {
            const div = document.createElement('div');
            div.className = 'message ' + role;
            div.innerHTML = `
                <div class="message-role">${role === 'user' ? '你' : 'Evo-spark'}</div>
                <div class="message-content">${escapeHtml(content)}</div>
            `;
            chatMessages.appendChild(div);
            chatMessages.scrollTop = chatMessages.scrollHeight;
        }

        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }

        function updateStatus(status) {
            document.getElementById('systemStatus').textContent = status;
            const indicator = document.getElementById('statusIndicator');
            indicator.className = 'indicator ' + (status === '在线' ? 'online' : 'offline');
        }

        function updateMemory(memory) {
            document.getElementById('memoryContent').textContent = JSON.stringify(
                JSON.parse(memory), null, 2
            );
        }

        function updateMonitor(data) {
            document.getElementById('messageCount').textContent = data.conversation_count;
            document.getElementById('bufferSize').textContent =
                (data.buffer_size / 1024).toFixed(1) + ' KB';
            document.getElementById('storageUsage').textContent =
                data.used_space_kb + ' KB / ' + data.free_space_kb + ' KB';
            document.getElementById('lastUpdate').textContent = data.last_update;

            const progress = (data.used_space_kb /
                (data.used_space_kb + data.free_space_kb) * 100).toFixed(1);
            document.getElementById('storageProgress').style.width = progress + '%';
        }

        // 绑定事件
        sendBtn.addEventListener('click', sendMessage);
        chatInput.addEventListener('keypress', function(e) {
            if (e.key === 'Enter') sendMessage();
        });

        // 轮询更新监控数据（每秒）
        setInterval(() => {
            fetch('/api/status').then(r => r.json()).then(data => {
                updateMonitor(data.monitor);
            });
        }, 1000);

        // 记忆数据不再自动轮询，点击按钮手动刷新
        function refreshMemory() {
            fetch('/api/memory').then(r => r.json()).then(data => {
                updateMemory(data.memory);
            }).catch(err => {
                console.error('Failed to refresh memory:', err);
            });
        }
    </script>
</body>
</html>
)rawliteral";

WebServer::WebServer() : server_(NULL), is_running_(false) {
    // monitor_timer_ 已移除 - 前端使用 HTTP 轮询
}

WebServer::~WebServer() {
    Stop();
}

bool WebServer::Start() {
    ESP_LOGI(TAG, "Starting web server...");

    // 获取 IP 地址 - 从默认 STA 接口
    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        char ip_str[16];
        esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
        ip_address_ = std::string(ip_str);
        ESP_LOGI(TAG, "IP Address: %s", ip_str);
    } else {
        // 如果无法获取，使用默认值
        ip_address_ = "192.168.4.1";
        ESP_LOGW(TAG, "Could not get IP address, using default: 192.168.4.1");
    }

    // 创建 HTTP 服务器
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_open_sockets = 7;
    config.max_uri_handlers = 10;
    config.stack_size = 8192;  // 增加任务栈大小到 8KB
    config.task_priority = 5;   // 降低优先级，与压缩任务相同

    esp_err_t ret = httpd_start(&server_, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return false;
    }

    setup_http_handlers();

    // 监控定时器已移除 - 前端使用 HTTP 轮询
    // start_monitor_timer();

    is_running_ = true;
    ESP_LOGI(TAG, "Web server started at http://%s:80", ip_address_.c_str());
    return true;
}

void WebServer::Stop() {
    if (!is_running_) return;

    // 监控定时器已移除
    // stop_monitor_timer();

    if (server_ != NULL) {
        httpd_stop(server_);
        server_ = NULL;
    }

    is_running_ = false;
    ESP_LOGI(TAG, "Web server stopped");
}

void WebServer::setup_http_handlers() {
    httpd_uri_t uri_index = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_html_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server_, &uri_index);

    httpd_uri_t uri_config = {
        .uri       = "/config",
        .method    = HTTP_GET,
        .handler   = config_html_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server_, &uri_config);

    httpd_uri_t uri_api_config = {
        .uri       = "/api/config",
        .method    = HTTP_POST,
        .handler   = api_config_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server_, &uri_api_config);

    httpd_uri_t uri_api_restart = {
        .uri       = "/api/restart",
        .method    = HTTP_POST,
        .handler   = api_restart_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server_, &uri_api_restart);

    httpd_uri_t uri_conversation = {
        .uri       = "/api/conversation",
        .method    = HTTP_POST,
        .handler   = api_conversation_handler,
        .user_ctx  = this
    };
    httpd_register_uri_handler(server_, &uri_conversation);

    httpd_uri_t uri_memory = {
        .uri       = "/api/memory",
        .method    = HTTP_GET,
        .handler   = api_memory_handler,
        .user_ctx  = this
    };
    httpd_register_uri_handler(server_, &uri_memory);

    httpd_uri_t uri_status = {
        .uri       = "/api/status",
        .method    = HTTP_GET,
        .handler   = api_status_handler,
        .user_ctx  = this
    };
    httpd_register_uri_handler(server_, &uri_status);

    httpd_uri_t uri_rollback = {
        .uri       = "/api/rollback",
        .method    = HTTP_POST,
        .handler   = api_rollback_handler,
        .user_ctx  = this
    };
    httpd_register_uri_handler(server_, &uri_rollback);
}

// HTTP 处理器实现
esp_err_t WebServer::index_html_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Content-Type", "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t WebServer::api_conversation_handler(httpd_req_t *req) {
    (void)req;  // Unused parameter
    // 读取请求体（增加缓冲区大小以支持 UTF-8 中文）
    char buf[4096];
    int total_len = req->content_len;
    int received = 0;

    if (total_len >= sizeof(buf) || total_len <= 0) {
        ESP_LOGE(TAG, "Invalid content length: %d", total_len);
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Payload too large");
        return ESP_FAIL;
    }

    while (received < total_len) {
        int ret = httpd_req_recv(req, buf + received, sizeof(buf) - received - 1);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;  // 重试
        }
        if (ret <= 0) {
            ESP_LOGE(TAG, "Receive error: %d, received=%d/%d", ret, received, total_len);
            break;
        }
        received += ret;
    }

    if (received <= 0 || received != total_len) {
        ESP_LOGE(TAG, "Incomplete data: %d/%d bytes", received, total_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
        return ESP_FAIL;
    }

    buf[received] = '\0';

    // 解析 JSON
    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *role_item = cJSON_GetObjectItem(json, "role");
    cJSON *content_item = cJSON_GetObjectItem(json, "content");

    if (!role_item || !cJSON_IsString(role_item) || !content_item || !cJSON_IsString(content_item)) {
        ESP_LOGE(TAG, "Invalid JSON format");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    // 使用 std::string 拷贝内容，避免指针失效
    std::string role = role_item->valuestring ? role_item->valuestring : "";
    std::string content = content_item->valuestring ? content_item->valuestring : "";
    cJSON_Delete(json);

    ESP_LOGI(TAG, "Conversation: role=%s, content_len=%d", role.c_str(), content.length());

    // 添加到记忆管理器
    MemoryManager& mgr = MemoryManager::GetInstance();
    mgr.AddConversation(role, content);

    // 生成 AI 响应
    std::string ai_response;
    MemoryPackage memory = mgr.GetMemoryPackage();

    // 构造包含记忆的 prompt
    std::string prompt = R"(你是 Evo-spark 智能陪伴机器人，根据以下记忆和对话历史，自然地回应用户。

记忆包（JSON）：
)" + memory.raw_json + R"(

用户消息：)" + content + R"(

请用友好、自然的语气回应。只返回回应内容，不要其他说明。)";

    GLMClient& glm = GLMClient::GetInstance();
    if (glm.Chat(prompt, ai_response)) {
        ESP_LOGI(TAG, "AI response generated: %d bytes", ai_response.length());

        // 添加 AI 响应到记忆
        mgr.AddConversation("assistant", ai_response);

        // GLMClient 已经解析了响应，ai_response 就是内容文本
        // 使用 cJSON 生成正确的 JSON 响应（自动转义特殊字符）
        cJSON *json_root = cJSON_CreateObject();
        if (json_root) {
            cJSON_AddStringToObject(json_root, "response", ai_response.c_str());
            char *json_str = cJSON_PrintUnformatted(json_root);

            if (json_str) {
                httpd_resp_set_hdr(req, "Content-Type", "application/json");
                httpd_resp_send(req, json_str, strlen(json_str));
                free(json_str);
            } else {
                httpd_resp_set_hdr(req, "Content-Type", "application/json");
                httpd_resp_send(req, "{\"response\":\"[解析错误]\"}", HTTPD_RESP_USE_STRLEN);
            }

            cJSON_Delete(json_root);
            return ESP_OK;
        }
    }

    // 生成失败
    ESP_LOGE(TAG, "Failed to generate AI response");
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    httpd_resp_send(req, "{\"response\":\"抱歉，我暂时无法回应。\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t WebServer::api_memory_handler(httpd_req_t *req) {
    (void)req;  // Unused parameter
    MemoryPackage memory = MemoryManager::GetInstance().GetMemoryPackage();

    std::string response = "{\"memory\":" + memory.raw_json + "}";
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

esp_err_t WebServer::api_status_handler(httpd_req_t *req) {
    (void)req;  // Unused parameter
    MemoryManager& mgr = MemoryManager::GetInstance();

    MonitorData data;
    data.conversation_count = mgr.GetConversationCount();
    data.buffer_size = mgr.GetBufferSize();
    data.free_space_kb = mgr.GetFreeSpace() / 1024;
    data.used_space_kb = mgr.GetUsedSpace() / 1024;
    data.is_idle = mgr.IsBufferEmpty();
    data.last_update = "刚刚";

    std::string response = "{\"monitor\":" + data.to_json() + "}";
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

esp_err_t WebServer::api_rollback_handler(httpd_req_t *req) {
    (void)req;  // Unused parameter
    char buf[64];
    int total_len = req->content_len;
    int received = 0;

    if (total_len >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Payload too large");
        return ESP_FAIL;
    }

    while (received < total_len) {
        int ret = httpd_req_recv(req, buf + received, sizeof(buf) - received - 1);
        if (ret <= 0) {
            break;
        }
        received += ret;
    }

    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
        return ESP_FAIL;
    }

    buf[received] = '\0';

    cJSON *json = cJSON_Parse(buf);
    cJSON *version_item = cJSON_GetObjectItem(json, "version");
    if (!version_item || !cJSON_IsNumber(version_item)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    int version = version_item->valueint;
    cJSON_Delete(json);

    MemoryManager& mgr = MemoryManager::GetInstance();
    bool success = mgr.RollbackToBackup(version);

    std::string response = success ? "{\"status\":\"ok\"}" : "{\"status\":\"error\"}";
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

// 监控定时器 - 已移除，前端使用 HTTP 轮询
/*
void WebServer::start_monitor_timer() {
    esp_timer_create_args_t timer_args = {
        .callback = [](void* arg) {
            WebServer* server = static_cast<WebServer*>(arg);
            server->on_monitor_timer();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "monitor_timer",
        .skip_unhandled_events = true
    };

    if (esp_timer_create(&timer_args, &monitor_timer_) == ESP_OK) {
        esp_timer_start_periodic(monitor_timer_, 1000000);  // 1秒
        ESP_LOGI(TAG, "Monitor timer started");
    }
}

void WebServer::stop_monitor_timer() {
    if (monitor_timer_ != NULL) {
        esp_timer_stop(monitor_timer_);
        esp_timer_delete(monitor_timer_);
        monitor_timer_ = NULL;
    }
}

void WebServer::on_monitor_timer() {
    MemoryManager& mgr = MemoryManager::GetInstance();

    MonitorData data;
    data.conversation_count = mgr.GetConversationCount();
    data.buffer_size = mgr.GetBufferSize();
    data.free_space_kb = mgr.GetFreeSpace() / 1024;
    data.used_space_kb = mgr.GetUsedSpace() / 1024;
    data.is_idle = mgr.IsBufferEmpty();
    data.last_update = "刚刚";
}
*/

// 配置页面 HTML
static const char config_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Evo-spark 配置</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 16px;
            padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            width: 100%;
            max-width: 480px;
        }
        h1 {
            text-align: center;
            color: #333;
            margin-bottom: 30px;
            font-size: 28px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #555;
            font-weight: 600;
            font-size: 14px;
        }
        input {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 14px;
            transition: all 0.3s;
        }
        input:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        button {
            width: 100%;
            padding: 14px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
            margin-top: 10px;
        }
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }
        button:active {
            transform: translateY(0);
        }
        button:disabled {
            background: #ccc;
            cursor: not-allowed;
            transform: none;
        }
        .message {
            padding: 12px 16px;
            border-radius: 8px;
            margin-top: 20px;
            display: none;
            font-size: 14px;
        }
        .message.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .message.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .message.info {
            background: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
        }
        .info-box {
            background: #f8f9fa;
            border-left: 4px solid #667eea;
            padding: 12px 16px;
            margin-bottom: 20px;
            font-size: 13px;
            color: #666;
        }
        .spinner {
            display: inline-block;
            width: 14px;
            height: 14px;
            border: 2px solid rgba(255,255,255,0.3);
            border-radius: 50%;
            border-top-color: white;
            animation: spin 0.8s ease-in-out infinite;
            margin-right: 8px;
            vertical-align: middle;
        }
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🤖 Evo-spark 配置</h1>

        <div class="info-box">
            请配置您的 WiFi 网络和 GLM API Key。<br>
            配置完成后设备将自动重启并连接到您的网络。
        </div>

        <form id="configForm">
            <div class="form-group">
                <label for="ssid">WiFi 名称 (SSID)</label>
                <input type="text" id="ssid" name="ssid" required
                       placeholder="请输入您的 WiFi 名称">
            </div>

            <div class="form-group">
                <label for="password">WiFi 密码</label>
                <input type="password" id="password" name="password"
                       placeholder="请输入 WiFi 密码">
            </div>

            <div class="form-group">
                <label for="api_key">GLM API Key</label>
                <input type="password" id="api_key" name="api_key" required
                       placeholder="请输入 GLM API Key">
            </div>

            <button type="submit" id="submitBtn">保存配置并重启</button>
        </form>

        <div id="message" class="message"></div>
    </div>

    <script>
        const form = document.getElementById('configForm');
        const submitBtn = document.getElementById('submitBtn');
        const message = document.getElementById('message');

        function showMessage(text, type) {
            message.textContent = text;
            message.className = 'message ' + type;
            message.style.display = 'block';
        }

        form.addEventListener('submit', async (e) => {
            e.preventDefault();

            const ssid = document.getElementById('ssid').value.trim();
            const password = document.getElementById('password').value.trim();
            const api_key = document.getElementById('api_key').value.trim();

            if (!ssid || !api_key) {
                showMessage('请填写所有必填字段', 'error');
                return;
            }

            // 显示加载状态
            submitBtn.disabled = true;
            submitBtn.innerHTML = '<span class="spinner"></span>正在保存...';

            try {
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({
                        ssid: ssid,
                        password: password,
                        api_key: api_key
                    })
                });

                const data = await response.json();

                if (response.ok && data.success) {
                    showMessage('配置保存成功！设备正在重启...', 'success');
                    submitBtn.innerHTML = '保存成功！';

                    // 30秒后提示手动重启
                    setTimeout(() => {
                        message.innerHTML = '如果设备没有自动重启，请手动重启设备<br>' +
                                             '重启后请访问主页面：http://' + window.location.hostname;
                    }, 30000);
                } else {
                    showMessage('保存失败：' + (data.message || '未知错误'), 'error');
                    submitBtn.disabled = false;
                    submitBtn.innerHTML = '保存配置并重启';
                }
            } catch (error) {
                showMessage('网络错误：' + error.message, 'error');
                submitBtn.disabled = false;
                submitBtn.innerHTML = '保存配置并重启';
            }
        });
    </script>
</body>
</html>
)rawliteral";

// 配置页面处理器
esp_err_t WebServer::config_html_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Content-Type", "text/html");
    httpd_resp_send(req, config_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 配置保存处理器
esp_err_t WebServer::api_config_handler(httpd_req_t *req) {
    char buf[512];
    int total_len = req->content_len;
    int received = 0;

    if (total_len >= sizeof(buf)) {
        ESP_LOGE(TAG, "Request too large: %d bytes", total_len);
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Payload too large");
        return ESP_FAIL;
    }

    // 循环读取直到接收所有数据
    while (received < total_len) {
        int ret = httpd_req_recv(req, buf + received, sizeof(buf) - received - 1);
        if (ret <= 0) {
            break;
        }
        received += ret;
    }

    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
        return ESP_FAIL;
    }

    buf[received] = '\0';

    ESP_LOGI(TAG, "Received config request: %s", buf);

    // 简单的 JSON 解析（提取 ssid, password, api_key）
    std::string ssid = "", password = "", api_key = "";

    char* p = strstr(buf, "\"ssid\":\"");
    if (p) {
        p += 8;
        char* end = strstr(p, "\"");
        if (end) {
            ssid = std::string(p, end - p);
        }
    }

    p = strstr(buf, "\"password\":\"");
    if (p) {
        p += 12;
        char* end = strstr(p, "\"");
        if (end) {
            password = std::string(p, end - p);
        }
    }

    p = strstr(buf, "\"api_key\":\"");
    if (p) {
        p += 11;
        char* end = strstr(p, "\"");
        if (end) {
            api_key = std::string(p, end - p);
        }
    }

    ESP_LOGI(TAG, "Parsed config - SSID: %s, API Key: %s***",
             ssid.c_str(),
             api_key.substr(0, std::min((size_t)8, api_key.length())).c_str());

    // 保存配置
    ConfigManager& config = ConfigManager::GetInstance();
    esp_err_t err = config.SetConfig(ssid, password, api_key);

    std::string response;
    if (err == ESP_OK) {
        response = "{\"success\":true,\"message\":\"Configuration saved\"}";

        // 延迟重启，让响应先发送出去
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        response = "{\"success\":false,\"message\":\"Failed to save configuration\"}";
    }

    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

// 重启处理器
esp_err_t WebServer::api_restart_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Restart requested via Web API");

    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    httpd_resp_send(req, "{\"success\":true,\"message\":\"Restarting...\"}", HTTPD_RESP_USE_STRLEN);

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}

} // namespace EvoSpark
