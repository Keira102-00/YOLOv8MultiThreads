#include "result_processor.h"
#include "common_utils.h"
#include <unordered_map>
#include <vector>
#include <future>
#include <cerrno>

bool ResultProcessor::init(ThreadPool* thread_pool) {
    if (!thread_pool) {
        safe_printf("ResultProcessor: thread pool is null");
        return false;
    }
    draw_thread_pool_ = thread_pool;
    is_inited_ = true;
    safe_printf("ResultProcessor initialized");
    return true;
}

void ResultProcessor::process_and_draw(const object_detect_result_list& od_results,
                                       cv::Mat& frame,
                                       const TrackerWrapper& tracker,
                                       const FPSCounter& fps_counter) {
    if (!is_inited_ || frame.empty()) {
        safe_printf("ResultProcessor: not inited or frame is empty");
        return;
    }

    // 1. 建立「检测框-追踪ID」映射
    std::unordered_map<int, int> detection_to_track_id;
    build_detection_track_map(od_results, tracker, detection_to_track_id);

    // 2. 线程池并行绘制所有检测结果
    std::vector<std::future<void>> draw_futures;
    for (int i = 0; i < od_results.count; i++) {
        const auto& det = od_results.results[i];
        if (det.cls_id < 0 || det.cls_id >= 80) continue;

        // 获取当前检测框对应的追踪ID（无则为-1）
        int track_id = (detection_to_track_id.find(i) != detection_to_track_id.end()) 
                       ? detection_to_track_id[i] : -1;

        // 提交绘制任务（捕获拷贝，避免引用失效）
        draw_futures.push_back(draw_thread_pool_->put([this, det, track_id, &frame]() {
            draw_single_detection(det, track_id, frame);
        }));
    }

    // 3. 等待所有绘制任务完成
    for (auto& fut : draw_futures) {
        if (fut.valid()) {
            try {
                fut.get();
            } catch (const std::exception& e) {
                // safe_printf("ResultProcessor: draw task exception: " + std::string(e.what()));
                safe_printf("ResultProcessor: draw task exception: %s", e.what());
            }
        }
    }

    // 4. 绘制全局统计信息（FPS、追踪数）
    std::vector<TrackResult> tracks;
    tracker.GetTrackResults(tracks);
    float current_fps = fps_counter.get_current_fps();

    // 绘制FPS
    cv::putText(frame, cv::format("FPS: %.2f", current_fps), 
                cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 
                2.0, cv::Scalar(90, 40, 0), 2.5, cv::LINE_AA);
    // 绘制追踪数
    cv::putText(frame, cv::format("Tracked: %ld", tracks.size()), 
                cv::Point(10, 100), cv::FONT_HERSHEY_SIMPLEX, 
                1.5, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

    // 5. 固定输出尺寸（简化显示逻辑）
    cv::resize(frame, frame, cv::Size(1280, 720), 0, 0, cv::INTER_CUBIC);
}

// 建立「检测框-追踪ID」映射：通过中心距离匹配
void ResultProcessor::build_detection_track_map(const object_detect_result_list& od_results,
                                                const TrackerWrapper& tracker,
                                                std::unordered_map<int, int>& detection_to_track_id) {
    std::vector<TrackResult> tracks;
    tracker.GetTrackResults(tracks);

    // 遍历所有追踪目标，匹配检测框
    for (const auto& track : tracks) {
        int track_id = track.track_id;
        cv::Rect track_rect = track.bbox;
        int track_center_x = track_rect.x + track_rect.width / 2;
        int track_center_y = track_rect.y + track_rect.height / 2;

        // 遍历所有检测框，找最近的未匹配检测框
        for (int i = 0; i < od_results.count; i++) {
            const auto& det = od_results.results[i];
            if (detection_to_track_id.find(i) != detection_to_track_id.end()) continue;

            int det_center_x = (det.box.left + det.box.right) / 2;
            int det_center_y = (det.box.top + det.box.bottom) / 2;
            int distance = abs(track_center_x - det_center_x) + abs(track_center_y - det_center_y);

            // 距离阈值（可后续改为配置参数）
            if (distance < 30) {
                detection_to_track_id[i] = track_id;
                break;
            }
        }
    }
}

// 单检测结果绘制：检测框+类别+概率+追踪ID+人体骨架（仅person）
void ResultProcessor::draw_single_detection(const object_detect_result& det,
                                            int track_id,
                                            cv::Mat& frame) {
    int x1 = det.box.left;
    int y1 = det.box.top;
    int x2 = det.box.right;
    int y2 = det.box.bottom;
    const char* cls_name = coco_cls_names_[det.cls_id];

    // 1. 绘制检测框
    cv::rectangle(frame, cv::Point(x1, y1), cv::Point(x2, y2), 
                  cv::Scalar(255, 0, 0, 255), 2);

    // 2. 绘制类别、概率、追踪ID
    char text[256] = {0};
    int text_y = (y1 - 20) > 0 ? (y1 - 20) : 20;
    if (track_id != -1) {
        sprintf(text, "ID:%d %s %.1f%%", track_id, cls_name, det.prop * 100);
    } else {
        sprintf(text, "%s %.1f%%", cls_name, det.prop * 100);
    }
    cv::putText(frame, text, cv::Point(x1, text_y), 
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);

    // 4. 线程安全打印检测结果
    if (track_id != -1) {
        safe_printf(cv::format("ID:%d %s @ (%d, %d, %d, %d) Conf:%.3f", 
                            track_id, cls_name, x1, y1, x2, y2, det.prop).c_str());
    } else {
        safe_printf(cv::format("%s @ (%d, %d, %d, %d) Conf:%.3f", 
                            cls_name, x1, y1, x2, y2, det.prop).c_str());
    }
}