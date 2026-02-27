#ifndef GLM_CLIENT_H
#define GLM_CLIENT_H

#include <string>
#include <esp_http_client.h>
#include <cJSON.h>

namespace EvoSpark {

class GLMClient {
public:
    static GLMClient& GetInstance() {
        static GLMClient instance;
        return instance;
    }

    bool Init(const std::string& api_key);
    bool Chat(const std::string& message, std::string& response);

    // 设置模型参数
    void SetMaxTokens(int max_tokens) { max_tokens_ = max_tokens; }
    void SetTemperature(float temperature) { temperature_ = temperature; }

private:
    GLMClient();
    ~GLMClient();

    std::string api_key_;
    std::string api_url_;
    int max_tokens_ = 4096;
    float temperature_ = 0.7f;
    bool is_initialized_ = false;
};

} // namespace EvoSpark

#endif // GLM_CLIENT_H
