#include "tracking.h"
#include <limits>

std::vector<std::pair<int, int>> hungarian_matching(const std::vector<Eigen::VectorXd>& detections, const std::vector<Track>& tracks) {
    std::vector<std::pair<int, int>> matches;
    std::vector<bool> detection_matched(detections.size(), false);
    std::vector<bool> track_matched(tracks.size(), false);
    
    // 增加最大距离阈值，允许更大范围内匹配
    const double max_distance = 500; 
    
    for (size_t i = 0; i < detections.size(); ++i) {
        double min_distance = std::numeric_limits<double>::max();
        int min_index = -1;
        
        for (size_t j = 0; j < tracks.size(); ++j) {
            if (!track_matched[j]) {
                Eigen::VectorXd state = tracks[j].get_state();
                double distance = (detections[i] - state).norm();
                if (distance < min_distance) {
                    min_distance = distance;
                    min_index = j;
                }
            }
        }
        
        if (min_index != -1 && min_distance < max_distance) {
            matches.emplace_back(i, min_index);
            detection_matched[i] = true;
            track_matched[min_index] = true;
        }
    }
    
    return matches;
}