#include "video_capture.h"
#include "common_utils.h"
#include <cctype>
#include <algorithm> // For std::all_of

VideoCaptureWrapper::~VideoCaptureWrapper() {
    release();
}

bool VideoCaptureWrapper::init(const std::string& device, int width, int height, int fps) {
    // 1. 检查是否是网络摄像头（RTSP URL）
    const std::string rtsp_prefix = "rtsp://";
    bool is_network_cam = (device.length() >= rtsp_prefix.length()) && 
                          (device.substr(0, rtsp_prefix.length()) == rtsp_prefix);
    
    // 2. 检查是否是USB摄像头（设备名全为数字）
    bool is_usb_cam = !device.empty() && std::all_of(device.begin(), device.end(), ::isdigit);
    
    // 3. 根据不同类型的设备进行初始化
    if (is_network_cam) {
        if (!cap_.open(device)) {
            safe_printf("Failed to open network camera: %s", device.c_str());
            return false;
        }
        safe_printf("Network camera opened successfully: %s", device.c_str());
    } 
    else if (is_usb_cam) {
        int cam_id = std::stoi(device);
        if (!cap_.open(cam_id, cv::CAP_V4L2)) {
            safe_printf("Failed to open USB camera: /dev/video%d", cam_id);
            return false;
        }
        cap_.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH, width);
        cap_.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT, height);
        cap_.set(cv::VideoCaptureProperties::CAP_PROP_FPS, fps);
        
        safe_printf("USB camera /dev/video%d opened successfully.", cam_id);
    } 
    else { // 否则，尝试作为本地视频文件打开
        if (!cap_.open(device)) {
            safe_printf("Failed to open video file: %s", device.c_str());
            return false;
        }
        safe_printf("Video file opened successfully: %s", device.c_str());
    }
    int actual_w = cap_.get(cv::CAP_PROP_FRAME_WIDTH);
    int actual_h = cap_.get(cv::CAP_PROP_FRAME_HEIGHT);
    safe_printf("实际处理分辨率: %dx%d", actual_w, actual_h);
    is_initialized_ = true;
    return true;
}

bool VideoCaptureWrapper::read_frame(cv::Mat& frame) {
    if (!is_initialized_ || !cap_.isOpened()) {
        safe_printf("Video capture is not initialized or not opened.");
        return false;
    }
    return cap_.read(frame);
}

void VideoCaptureWrapper::release() {
    if (is_initialized_ && cap_.isOpened()) {
        cap_.release();
        is_initialized_ = false;
        safe_printf("Video capture released.");
    }
}

bool VideoCaptureWrapper::is_opened() const {
    return is_initialized_ && cap_.isOpened();
}