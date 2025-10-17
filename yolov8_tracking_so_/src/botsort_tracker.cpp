#include "tracking.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>

static bool s_is_authorized = false;
// 检查板子型号是否支持模型初始化
static bool check_board_model_support() {
    std::string board_name;
    std::ifstream file("/sys/ztl/board_name"); // path /sys/ztl/board_name
    
    if (file.is_open()) {
        std::getline(file, board_name);
        file.close();
        
        // 去除可能的空白字符
        board_name.erase(board_name.find_last_not_of(" \t\n\r") + 1);
        
        // 检查支持的板子型号
        const std::string supported_models[] = {"A588", "588", "576", "566", "568", "562"};
        for (const auto& model : supported_models) {
            if (board_name == model) {
                s_is_authorized=true;
                return true;
            }
        }
    }
    
    return false;
}
// 新增：授权检查卡点函数（在关键流程调用）
static void enforce_authorization() {
    if (!check_board_model_support()) {
        // 抛出异常中断算法流程，主程序捕获后输出特定错误
        throw std::runtime_error("BERROR:模型初始化错误");
    }
}

BoTSORTTracker::BoTSORTTracker() : object_count(0), last_movement_direction("Unknown") {
    enforce_authorization();
    class_counts[0] = 0;
    class_counts[1] = 0;
}

BoTSORTTracker::~BoTSORTTracker() {
    tracks.clear();
}

void BoTSORTTracker::update(const std::vector<Eigen::VectorXd>& detections, const std::vector<int>& cls_ids, const cv::Rect& region) {
    // 预测所有现有轨迹
    for (auto& track : tracks) {
        track.predict();
    }
    
    // 匈牙利匹配
    std::vector<std::pair<int, int>> matches = hungarian_matching(detections, tracks);
    
    // 更新匹配的轨迹
    for (const auto& match : matches) {
        int detection_index = match.first;
        int track_index = match.second;
        
        tracks[track_index].update(detections[detection_index]);
        
        Eigen::VectorXd current_position = tracks[track_index].get_state();
        int x = static_cast<int>(current_position(0));
        int y = static_cast<int>(current_position(1));
        
        if (region.contains(cv::Point(x, y))) {
            if (!tracks[track_index].has_entered_region) {
                tracks[track_index].has_entered_region = true;
                tracks[track_index].entry_position = current_position;
            }
        } else {
            if (tracks[track_index].has_entered_region && !tracks[track_index].has_left_region) {
                tracks[track_index].has_left_region = true;
                tracks[track_index].exit_position = current_position;
            }
        }
    }
    
    // 处理未匹配的检测，创建新轨迹
    std::vector<bool> detection_matched(detections.size(), false);
    for (const auto& match : matches) {
        detection_matched[match.first] = true;
    }
    
    for (size_t i = 0; i < detections.size(); ++i) {
        if (!detection_matched[i]) {
            tracks.emplace_back(detections[i], cls_ids[i]);
        }
    }
    
    // 移除长时间未更新的轨迹
    tracks.erase(std::remove_if(tracks.begin(), tracks.end(), 
        [](const Track& track) {
            return track.time_since_update > 1000;
        }), tracks.end());
    
    // 更新计数
    for (auto& track : tracks) {
        if (track.has_entered_region && track.has_left_region && !track.count_incremented) {
            double dx = track.exit_position(0) - track.entry_position(0);
            if (dx < -100) { // 从右向左移动
                if (track.cls_id == 0 || track.cls_id == 3) { // bottle或can
                    object_count++;
                    if (track.cls_id == 0) class_counts[0]++;
                    else if (track.cls_id == 3) class_counts[1]++;
                    track.count_incremented = true;
                    last_movement_direction = "Left";
                }
            }
        }
    }
}

void BoTSORTTracker::reset_counters() {
    object_count = 0;
    last_movement_direction = "Unknown";
    class_counts[0] = 0;
    class_counts[1] = 0;
    
    for (auto& track : tracks) {
        track.count_incremented = false;
    }
}