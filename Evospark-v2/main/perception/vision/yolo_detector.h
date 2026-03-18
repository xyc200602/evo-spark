#ifndef YOLO_DETECTOR_H
#define YOLO_DETECTOR_H

#include <vector>
#include <string>
#include <cstdint>

namespace EvoSpark {

// 检测到的物体
struct DetectedObject {
    std::string label;       // 物体名称
    float confidence;        // 置信度 (0-1)
    int x, y;                // 边界框左上角
    int width, height;       // 边界框宽高

    DetectedObject() : confidence(0), x(0), y(0), width(0), height(0) {}
};

// YOLO 物体检测器
class YOLODetector {
public:
    static YOLODetector& GetInstance() {
        static YOLODetector instance;
        return instance;
    }

    // 初始化（加载模型）
    bool Init(const std::string& model_path = "/spiffs/yolo.tflite");

    // 检测物体（从图像数据）
    std::vector<DetectedObject> Detect(const uint8_t* image_data, 
                                       int width, int height, 
                                       int channels = 3);

    // 检测物体（从 JPEG）
    std::vector<DetectedObject> DetectFromJPEG(const std::vector<uint8_t>& jpeg_data);

    // 检测并生成描述（用于 LLM 输入）
    std::string DetectAndDescribe(const uint8_t* image_data,
                                  int width, int height);

    // 是否已初始化
    bool IsInitialized() const { return initialized_; }

    // 设置置信度阈值
    void SetConfidenceThreshold(float threshold) {
        confidence_threshold_ = threshold;
    }

private:
    YOLODetector() = default;
    ~YOLODetector();

    // 预处理图像
    void PreprocessImage(const uint8_t* input, int w, int h, int c,
                         uint8_t* output, int target_w, int target_h);

    // 后处理输出
    std::vector<DetectedObject> Postprocess(const float* output,
                                            int num_boxes, int num_classes);

    // 非极大值抑制
    std::vector<DetectedObject> NMS(std::vector<DetectedObject>& objects, float iou_threshold);

    bool initialized_ = false;
    float confidence_threshold_ = 0.5f;
    float nms_threshold_ = 0.45f;

    // 模型相关（预留）
    void* model_ = nullptr;
    void* interpreter_ = nullptr;
};

} // namespace EvoSpark

#endif // YOLO_DETECTOR_H
