#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include <opencv2/opencv.hpp>
#include <string>

// 视频捕获器（支持摄像头/视频文件）
class VideoCaptureWrapper {
public:
    VideoCaptureWrapper() = default;
    ~VideoCaptureWrapper();

    // 初始化：device可以是摄像头ID（如"0"）或视频文件路径
    bool init(const std::string& device, int width = 800, int height = 600, int fps = 60);
    // 读取一帧（成功返回true，frame存储结果）
    bool read_frame(cv::Mat& frame);
    // 释放资源
    void release();
    // 判断是否初始化成功
    bool is_opened() const;

private:
    cv::VideoCapture cap_;  // 隐藏的OpenCV捕获对象
    bool is_initialized_ = false;  // 初始化状态标记
};

#endif // VIDEO_CAPTURE_H