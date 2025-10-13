#ifndef TRACKING_H
#define TRACKING_H

#include <vector>
#include <eigen3/Eigen/Dense>
#include <opencv2/opencv.hpp>

// Forward declarations
class KalmanFilter;
class Track;
class BoTSORTTracker;

// KalmanFilter class for state estimation
class KalmanFilter {
public:
    KalmanFilter();
    void predict();
    void update(const Eigen::VectorXd& z);
    Eigen::VectorXd x;
    Eigen::MatrixXd F, H, Q, R, P;
};

// Track class for managing individual object tracks
class Track {
public:
    Track(const Eigen::VectorXd& detection, int cls_id);
    void predict();
    void update(const Eigen::VectorXd& detection);
    Eigen::VectorXd get_state() const;
    std::string getMovementDirection() const;
    
    static int next_id;
    int id, age, time_since_update;
    KalmanFilter kf;
    bool has_entered_region, has_left_region;
    Eigen::VectorXd entry_position, exit_position, prev_position;
    int cls_id;
    bool count_incremented;
};

// Hungarian matching algorithm
std::vector<std::pair<int, int>> hungarian_matching(const std::vector<Eigen::VectorXd>& detections, const std::vector<Track>& tracks);

// BoTSORT tracker class
class BoTSORTTracker {
public:
    BoTSORTTracker();
    ~BoTSORTTracker();
    
    void update(const std::vector<Eigen::VectorXd>& detections, const std::vector<int>& cls_ids, const cv::Rect& region);
    const std::vector<Track>& get_tracks() const { return tracks; }
    
    // Utility methods
    int get_object_count() const { return object_count; }
    const std::string& get_last_direction() const { return last_movement_direction; }
    const int* get_class_counts() const { return class_counts; }
    
    // Reset counters
    void reset_counters();
    
private:
    std::vector<Track> tracks;
    int object_count;
    std::string last_movement_direction;
    int class_counts[2]; // Support for 2 classes
};

#endif // TRACKING_H