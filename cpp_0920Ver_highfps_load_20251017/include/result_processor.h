#ifndef RESULT_PROCESSOR_H
#define RESULT_PROCESSOR_H

#include <opencv2/opencv.hpp>
#include <unordered_map>
#include "yolov8.h"  // 依赖检测结果结构体
#include "tracker_wrapper.h"  // 依赖追踪器
#include "common_utils.h"  // 依赖FPS统计器
#include "thread_pool.h"  

// 结果处理器：封装推理结果后处理+可视化
class ResultProcessor {
public:
    ResultProcessor() = default;
    ~ResultProcessor() = default;

    // 初始化：设置图像尺寸
    bool init(ThreadPool* thread_pool);

    // 核心接口：处理推理结果并绘制到帧
    // 输入：推理结果、原始帧、追踪器、FPS统计器
    // 输出：绘制后的帧（通过引用修改）
    void process_and_draw(const object_detect_result_list& od_results,
                           cv::Mat& frame,
                           const TrackerWrapper& tracker,
                           const FPSCounter& fps_counter);

private:
    ThreadPool* draw_thread_pool_ = nullptr;
    bool is_inited_ = false;                  // 初始化状态标记

    // 人体骨架连接索引（私有化，隐藏细节）
    const int skeleton_[38] = {16, 14, 14, 12, 17, 15, 15, 13, 12, 13, 6, 12, 7, 13, 6, 7, 6, 8,
                                7, 9, 8, 10, 9, 11, 2, 3, 1, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7};
    // COCO类别名称（私有化，隐藏细节）
    const char* coco_cls_names_[80] = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
        "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
        "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
        "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
        "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
        "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
        "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
        "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator",
        "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
    };

    // 内部辅助：建立「检测框-追踪ID」映射（私有化，隐藏匹配逻辑）
    void build_detection_track_map(const object_detect_result_list& od_results,
                                    const TrackerWrapper& tracker,
                                    std::unordered_map<int, int>& detection_to_track_id);

    // 内部辅助：单检测结果绘制
    void draw_single_detection(const object_detect_result& det,
                                int track_id,
                                cv::Mat& frame);
};

#endif // RESULT_PROCESSOR_H