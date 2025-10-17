#ifndef TRACKER_WRAPPER_H
#define TRACKER_WRAPPER_H

#include <vector>
#include <opencv2/opencv.hpp>
#include "tracking.h"  // 引用BoTSORT头文件
#include "yolov8.h"  

// 追踪结果结构体（对外统一格式，与底层解耦）
struct TrackResult {
    int track_id;          // 唯一追踪ID
    cv::Rect bbox;         // 目标检测框（OpenCV格式，便于绘制）
    int cls_id;            // 目标类别ID（与YOLOv8一致）
    float confidence;      // 目标置信度（继承自检测结果）
};

// 追踪器封装类（基于BoTSORT）
class TrackerWrapper {
public:
    TrackerWrapper() = default;
    ~TrackerWrapper() = default;

    /**
     * @brief 初始化追踪器
     * @param max_age 目标消失后最大保留帧数（默认30）
     * @param min_hits 目标连续检测到的最小帧数（默认3）
     * @param iou_threshold IOU匹配阈值（默认0.3）
     */
    bool Init(int max_age = 30, int min_hits = 3, float iou_threshold = 0.3);

    /**
     * @brief 更新追踪结果（核心接口）
     * @param det_results YOLOv8检测结果列表（直接使用原始结果，无需box_t）
     * @param frame_size 当前帧尺寸（宽、高），过滤超出范围的检测框
     */
    void Update(const object_detect_result_list& det_results, const cv::Size& frame_size);

    /**
     * @brief 获取当前所有有效追踪结果
     * @param results 输出参数，存储追踪结果
     */
    void GetTrackResults(std::vector<TrackResult>& results) const;

private:
    BoTSORTTracker tracker_;          // 底层BoTSORT追踪器
    bool is_initialized_ = false;     // 初始化状态标记
    int max_age_ = 30;                // 目标最大消失帧数
    int min_hits_ = 3;                // 目标最小连续检测帧数
    float iou_threshold_ = 0.3;       // IOU匹配阈值

    /**
     * @brief 辅助函数：将YOLOv8检测结果转换为追踪器输入格式（Eigen矩阵）
     * @param det_results YOLOv8原始检测结果
     * @param frame_size 帧尺寸（过滤无效框）
     * @param detections 输出：追踪器需要的检测框（中心x,y + 宽w + 高h）
     * @param cls_ids 输出：每个检测框的类别ID
     * @param confidences 输出：每个检测框的置信度
     */
    void ConvertDetToTrackerInput(
        const object_detect_result_list& det_results,
        const cv::Size& frame_size,
        std::vector<Eigen::VectorXd>& detections,
        std::vector<int>& cls_ids,
        std::vector<float>& confidences
    ) const;

    /**
     * @brief 辅助函数：过滤无效检测框（直接使用object_detect_result中的box字段）
     * @param det 单个检测结果（包含left, top, right, bottom）
     * @param frame_size 帧尺寸
     * @return true：有效；false：无效
     */
    bool IsValidDetection(const object_detect_result& det, const cv::Size& frame_size) const;
};

#endif // TRACKER_WRAPPER_H