#include <iostream>
#include <vector>
#include <future>
#include <opencv2/opencv.hpp>

#include "model_wrapper.h"
#include "video_capture.h"
#include "tracker_wrapper.h"
#include "result_processor.h"
#include "common_utils.h"

int main(int argc, char** argv) {
    // 参数检查
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <模型路径> <摄像头ID>\n";
        return -1;
    }

    // 模块初始化
    VideoCaptureWrapper cap;
    Yolov8Model model;
    TrackerWrapper tracker;
    ResultProcessor processor;
    FPSCounter fps;
    std::vector<std::future<object_detect_result_list>> futures;
    const int thread_num = 6;
    bool running = true;

    // 初始化流程
    if (!cap.init(argv[2], 800, 600, 60) || 
        !model.init(argv[1], thread_num) || 
        !processor.init(model.get_thread_pool())) {
        std::cerr << "初始化失败\n";
        model.release();
        cap.release();
        return -1;
    }
    tracker.Init();
    cv::namedWindow("YOLOv8");

    // 主循环
    int frame_idx = 0;
    cv::Mat frame;
    while (running && cap.read_frame(frame)) {
        // 提交推理任务
        futures.push_back(model.submit_infer_task(frame));

        // 处理推理结果
        if (frame_idx++ >= thread_num && !futures.empty()) {
            auto& f = futures.front();
            if (f.valid()) {
                try {
                    auto results = f.get();
                    tracker.Update(results, frame.size());
                    processor.process_and_draw(results, frame, tracker, fps);
                    cv::imshow("YOLOv8", frame);
                    fps.increment_frame();
                } catch (...) { std::cerr << "处理帧失败\n"; }
            }
            futures.erase(futures.begin());
        }

        // 按键退出
        if (cv::waitKey(1) == 27) running = false;
    }

    // 资源释放
    futures.clear();
    model.release();
    cap.release();
    cv::destroyAllWindows();
    return 0;
}
    