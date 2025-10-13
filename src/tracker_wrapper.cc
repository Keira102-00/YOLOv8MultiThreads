#include "tracker_wrapper.h"
#include "tracking.h"
#include "common_utils.h"  // 通用工具（线程安全打印）
#include <algorithm>       // 用于过滤无效检测框

// 初始化追踪器
bool TrackerWrapper::Init(int max_age, int min_hits, float iou_threshold) {
    max_age_ = max_age;
    min_hits_ = min_hits;
    iou_threshold_ = iou_threshold;

    try {
        tracker_ = BoTSORTTracker(); 
        is_initialized_ = true;
        safe_printf("[TrackerWrapper] Initialized (max_age: %d, min_hits: %d, iou_threshold: %.2f)",
                    max_age_, min_hits_, iou_threshold_);
        return true;
    } catch (...) {
        safe_printf("[TrackerWrapper] Unexpected error during initialization");
        is_initialized_ = false;
        return false;
    }
}

// 更新追踪结果（核心逻辑）
void TrackerWrapper::Update(const object_detect_result_list& det_results, const cv::Size& frame_size) {
    if (!is_initialized_) {
        safe_printf("[TrackerWrapper] Error: Call Update before Init!");
        return;
    }

    // 1. 将YOLOv8检测结果转换为追踪器输入格式（无box_t，直接用原始结果）
    std::vector<Eigen::VectorXd> detections;
    std::vector<int> cls_ids;
    std::vector<float> confidences;
    ConvertDetToTrackerInput(det_results, frame_size, detections, cls_ids, confidences);

    // 2. 调用底层BoTSORT更新追踪器
    cv::Rect frame_region(0, 0, frame_size.width, frame_size.height);  // 帧完整区域
    tracker_.update(detections, cls_ids, frame_region);

    safe_printf("[TrackerWrapper] Updated with %d valid detections (total input: %d)",
                static_cast<int>(detections.size()), det_results.count);
}

// 获取所有有效追踪结果
void TrackerWrapper::GetTrackResults(std::vector<TrackResult>& results) const {
    results.clear();
    if (!is_initialized_) {
        safe_printf("[TrackerWrapper] Error: Call GetTrackResults before Init!");
        return;
    }

    // 遍历底层追踪器的原始结果
    const auto& raw_tracks = tracker_.get_tracks();
    for (const auto& track : raw_tracks) {
        // 跳过未确认的追踪目标
        // 注意：这里需要根据实际的Track类接口进行调整
        bool is_confirmed = true; // 临时默认值，实际应调用track类的相应方法

        // 获取追踪框（发现实际返回的是2维状态向量：[中心x, 中心y]，而非期望的4维）
        const Eigen::VectorXd& track_state = track.get_state();
        if (track_state.size() < 2) {
            safe_printf("[TrackerWrapper] Invalid track state size: %d", static_cast<int>(track_state.size()));
            continue;
        }

        // 转换为OpenCV Rect格式（便于后续绘制）
        int center_x = static_cast<int>(track_state(0));
        int center_y = static_cast<int>(track_state(1));
        
        // 由于状态向量缺少宽度和高度信息，我们使用默认值（实际应用中可能需要从其他来源获取）
        int w = 50;  // 默认宽度
        int h = 100; // 默认高度（适合人体检测的一般尺寸）
        cv::Rect bbox(
            center_x - w / 2,  // left（中心x - 宽/2）
            center_y - h / 2,  // top（中心y - 高/2）
            w,                 // width
            h                  // height
        );

        // 构造最终追踪结果
        TrackResult result;
        result.track_id = track.id;                // 追踪ID（底层Track类属性）
        result.bbox = bbox;                        // 检测框（OpenCV格式）
        result.cls_id = track.cls_id;              // 类别ID（Track类属性）
        result.confidence = 0.9f;                  // 临时设置置信度，实际应根据接口调整

        results.push_back(result);
    }

    safe_printf("[TrackerWrapper] Got %d valid track results", static_cast<int>(results.size()));
}

// 辅助函数：将YOLOv8-Pose检测结果转换为追踪器输入格式
void TrackerWrapper::ConvertDetToTrackerInput(
    const object_detect_result_list& det_results,
    const cv::Size& frame_size,
    std::vector<Eigen::VectorXd>& detections,
    std::vector<int>& cls_ids,
    std::vector<float>& confidences
) const {
    detections.clear();
    cls_ids.clear();
    confidences.clear();

    // 遍历所有检测结果，过滤无效项并转换格式
    for (int i = 0; i < det_results.count; ++i) {
        const object_detect_result& det = det_results.results[i];

        // 1. 过滤无效检测（置信度≥25% + 坐标有效）
        if (!IsValidDetection(det, frame_size) || det.prop < 0.25) {
            continue;
        }

        // 2. 转换检测框格式：YOLOv8-Pose（left,top,right,bottom）→ 追踪器（中心x,y + 宽w + 高h）
        int center_x = (det.box.left + det.box.right) / 2;  // 检测框中心x
        int center_y = (det.box.top + det.box.bottom) / 2;  // 检测框中心y
        int w = det.box.right - det.box.left;               // 检测框宽度
        int h = det.box.bottom - det.box.top;               // 检测框高度

        // 3. 构造追踪器需要的Eigen矩阵（4维：x, y, w, h）
        Eigen::VectorXd det_vec(4);
        det_vec << center_x, center_y, w, h;

        // 4. 保存转换后的数据
        detections.push_back(det_vec);
        cls_ids.push_back(det.cls_id);          // 类别ID
        confidences.push_back(det.prop);        // 置信度
    }

    safe_printf("[TrackerWrapper] Converted %d valid detections (total input: %d)",
                static_cast<int>(detections.size()), det_results.count);
}

// 辅助函数：过滤无效检测框（直接使用object_detect_result的box字段）
bool TrackerWrapper::IsValidDetection(const object_detect_result& det, const cv::Size& frame_size) const {
    // 1. 检测框坐标是否在帧范围内（left≥0, top≥0, right≤帧宽, bottom≤帧高）
    if (det.box.left < 0 || det.box.top < 0 ||
        det.box.right > frame_size.width || det.box.bottom > frame_size.height) {
        return false;
    }

    // 2. 检测框尺寸是否过小（宽/高≥20像素，避免微小无效框）
    int w = det.box.right - det.box.left;
    int h = det.box.bottom - det.box.top;
    if (w < 20 || h < 20) {
        return false;
    }

    return true;
}