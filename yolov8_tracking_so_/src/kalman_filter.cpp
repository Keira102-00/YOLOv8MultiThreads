#include "tracking.h"

KalmanFilter::KalmanFilter() {
    F = Eigen::MatrixXd::Identity(4, 4);
    F(0, 2) = 1;
    F(1, 3) = 1;
    H = Eigen::MatrixXd::Zero(2, 4);
    H(0, 0) = 1;
    H(1, 1) = 1;
    // 增大过程噪声协方差，允许更快适应快速运动
    Q = Eigen::MatrixXd::Identity(4, 4) * 0.1; 
    // 减小测量噪声协方差，提高测量值可信度
    R = Eigen::MatrixXd::Identity(2, 2) * 0.01; 
    P = Eigen::MatrixXd::Identity(4, 4) * 100;
    x = Eigen::VectorXd::Zero(4);
}

void KalmanFilter::predict() {
    x = F * x;
    P = F * P * F.transpose() + Q;
}

void KalmanFilter::update(const Eigen::VectorXd& z) {
    Eigen::VectorXd y = z - H * x;
    Eigen::MatrixXd S = H * P * H.transpose() + R;
    Eigen::MatrixXd K = P * H.transpose() * S.inverse();
    x = x + K * y;
    P = (Eigen::MatrixXd::Identity(4, 4) - K * H) * P;
}