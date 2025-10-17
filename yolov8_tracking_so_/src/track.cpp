#include "tracking.h"

int Track::next_id = 0;

Track::Track(const Eigen::VectorXd& detection, int cls_id) {
    kf.x.head(2) = detection;
    id = next_id++;
    age = 0;
    time_since_update = 0;
    has_entered_region = false;
    has_left_region = false;
    entry_position = Eigen::VectorXd::Zero(2);
    exit_position = Eigen::VectorXd::Zero(2);
    prev_position = detection;
    this->cls_id = cls_id;
    count_incremented = false;
}

void Track::predict() {
    kf.predict();
    age++;
    time_since_update++;
}

void Track::update(const Eigen::VectorXd& detection) {
    prev_position = kf.x.head(2);
    kf.update(detection);
    time_since_update = 0;
}

Eigen::VectorXd Track::get_state() const {
    return kf.x.head(2);
}

std::string Track::getMovementDirection() const {
    if (!has_entered_region || !has_left_region) return "Unknown";
    double dx = exit_position(0) - entry_position(0);
    return dx > 0 ? "Right" : "Left";
}