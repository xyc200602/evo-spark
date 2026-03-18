#include "yolo_detector.h"
#include "esp_log.h"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace EvoSpark {

static const char* TAG = "YOLO";

// COCO 类别名称（80类）
static const char* COCO_CLASSES[] = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck",
    "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench",
    "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra",
    "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
    "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
    "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
    "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
    "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
    "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
    "toothbrush"
};
constexpr int NUM_CLASSES = 80;

YOLODetector::~YOLODetector() {
    // TODO: 释放模型资源
}

bool YOLODetector::Init(const std::string& model_path) {
    if (initialized_) {
        return true;
    }

    ESP_LOGI(TAG, "Initializing YOLO detector...");
    ESP_LOGI(TAG, "Model path: %s", model_path.c_str());

    // TODO: 加载 TensorFlow Lite 模型
    // 1. 读取模型文件
    // 2. 创建 interpreter
    // 3. 分配 tensor

    // 临时方案：模拟初始化
    ESP_LOGW(TAG, "YOLO model loading not implemented, using mock detection");
    initialized_ = true;

    ESP_LOGI(TAG, "YOLO detector initialized");
    return true;
}

std::vector<DetectedObject> YOLODetector::Detect(const uint8_t* image_data,
                                                  int width, int height,
                                                  int channels) {
    std::vector<DetectedObject> objects;

    if (!initialized_) {
        ESP_LOGE(TAG, "YOLO not initialized");
        return objects;
    }

    // TODO: 实现 YOLO 推理
    // 1. 预处理图像（缩放到模型输入尺寸）
    // 2. 运行推理
    // 3. 后处理输出（NMS）

    // 临时方案：返回模拟检测结果
    // 模拟检测到一个 "person"
    DetectedObject mock_obj;
    mock_obj.label = "person";
    mock_obj.confidence = 0.85f;
    mock_obj.x = width / 4;
    mock_obj.y = height / 4;
    mock_obj.width = width / 2;
    mock_obj.height = height / 2;
    objects.push_back(mock_obj);

    ESP_LOGI(TAG, "Detected %zu objects", objects.size());
    return objects;
}

std::vector<DetectedObject> YOLODetector::DetectFromJPEG(const std::vector<uint8_t>& jpeg_data) {
    // TODO: 解码 JPEG 并检测
    // 1. 使用 esp_jpeg 解码
    // 2. 调用 Detect()

    // 临时方案：返回空
    return Detect(nullptr, 320, 240, 3);
}

std::string YOLODetector::DetectAndDescribe(const uint8_t* image_data,
                                            int width, int height) {
    auto objects = Detect(image_data, width, height, 3);

    if (objects.empty()) {
        return "画面中没有检测到明显的物体";
    }

    std::ostringstream oss;
    oss << "画面中检测到：";

    for (size_t i = 0; i < objects.size() && i < 5; i++) {
        const auto& obj = objects[i];
        if (i > 0) oss << "、";
        oss << obj.label << "（置信度" << (int)(obj.confidence * 100) << "%）";
    }

    return oss.str();
}

void YOLODetector::PreprocessImage(const uint8_t* input, int w, int h, int c,
                                   uint8_t* output, int target_w, int target_h) {
    // TODO: 实现图像预处理
    // 1. 缩放
    // 2. 归一化
    // 3. 转换格式（如需要）
}

std::vector<DetectedObject> YOLODetector::Postprocess(const float* output,
                                                      int num_boxes, int num_classes) {
    std::vector<DetectedObject> objects;

    // TODO: 实现后处理
    // 1. 解析边界框
    // 2. 应用置信度阈值
    // 3. 获取类别标签

    return objects;
}

std::vector<DetectedObject> YOLODetector::NMS(std::vector<DetectedObject>& objects,
                                              float iou_threshold) {
    std::vector<DetectedObject> result;

    if (objects.empty()) {
        return result;
    }

    // 按置信度排序
    std::sort(objects.begin(), objects.end(),
              [](const DetectedObject& a, const DetectedObject& b) {
                  return a.confidence > b.confidence;
              });

    // NMS 循环
    while (!objects.empty()) {
        result.push_back(objects[0]);
        
        auto last = objects.begin();
        objects.erase(objects.begin());

        for (auto it = objects.begin(); it != objects.end(); ) {
            float iou = CalculateIOU(result.back(), *it);
            if (iou > iou_threshold) {
                it = objects.erase(it);
            } else {
                ++it;
            }
        }
    }

    return result;
}

// 计算 IOU（交集/并集）- 前向声明
static float CalculateIOU(const DetectedObject& a, const DetectedObject& b);

std::vector<DetectedObject> YOLODetector::NMS(std::vector<DetectedObject>& objects,
                                              float iou_threshold) {
    std::vector<DetectedObject> result;

    if (objects.empty()) {
        return result;
    }

    // 按置信度排序
    std::sort(objects.begin(), objects.end(),
              [](const DetectedObject& a, const DetectedObject& b) {
                  return a.confidence > b.confidence;
              });

    // NMS 循环
    while (!objects.empty()) {
        result.push_back(objects[0]);
        
        auto last = objects.begin();
        objects.erase(objects.begin());

        for (auto it = objects.begin(); it != objects.end(); ) {
            float iou = CalculateIOU(result.back(), *it);
            if (iou > iou_threshold) {
                it = objects.erase(it);
            } else {
                ++it;
            }
        }
    }

    return result;
}

// 计算 IOU 实现
static float CalculateIOU(const DetectedObject& a, const DetectedObject& b) {
    int x1 = std::max(a.x, b.x);
    int y1 = std::max(a.y, b.y);
    int x2 = std::min(a.x + a.width, b.x + b.width);
    int y2 = std::min(a.y + a.height, b.y + b.height);

    if (x2 < x1 || y2 < y1) return 0.0f;

    int intersection = (x2 - x1) * (y2 - y1);
    int union_area = a.width * a.height + b.width * b.height - intersection;

    return union_area > 0 ? (float)intersection / union_area : 0.0f;
}

} // namespace EvoSpark
