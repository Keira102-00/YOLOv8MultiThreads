#include <iostream>
#include <vector>
#include <future>
#include <opencv2/opencv.hpp>
#include <chrono>  // 用于计时

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

    // 计时相关变量 - 修正版
    int total_read_frame = 0;                  // 累计读取的帧数
    int total_completed_frame = 0;             // 累计完成（推理+跟踪+绘制+显示）的帧数
    auto global_start_time = std::chrono::steady_clock::now();  // 全局开始时间（不重置）
    
    // 用于计算每10帧的临时变量
    int last_completed_frame = 0;              // 上一次统计时的完成帧数
    auto last_stat_time = std::chrono::steady_clock::now();     // 上一次统计的时间点
    double temp_read_time = 0;                 // 临时累计：帧读取耗时
    double temp_submit_time = 0;               // 临时累计：提交任务耗时
    double temp_process_time = 0;              // 临时累计：处理结果耗时
    double temp_show_time = 0;                 // 临时累计：显示耗时

    auto last_frame_end_time = std::chrono::steady_clock::now();  // 上一帧结束时间

    // 主循环
    int frame_idx = 0;
    cv::Mat frame;
    while (running && cap.read_frame(frame)) {
        total_read_frame++;  // 累计读取的帧
        
        // 1. 统计帧读取耗时
        auto read_end_time = std::chrono::steady_clock::now();
        double read_cost = std::chrono::duration<double, std::milli>(read_end_time - last_frame_end_time).count();
        temp_read_time += read_cost;

        // 2. 提交推理任务 - 统计耗时
        auto submit_start_time = std::chrono::steady_clock::now();
        futures.push_back(model.submit_infer_task(frame));
        auto submit_end_time = std::chrono::steady_clock::now();
        double submit_cost = std::chrono::duration<double, std::milli>(submit_end_time - submit_start_time).count();
        temp_submit_time += submit_cost;

        // 3. 处理推理结果 + 4. 显示（只有处理完推理的帧才计入“完成帧”）
        if (frame_idx++ >= thread_num && !futures.empty()) {
            auto process_start_time = std::chrono::steady_clock::now();
            
            auto& f = futures.front();
            if (f.valid()) {
                try {
                    auto results = f.get();
                    tracker.Update(results, frame.size());
                    processor.process_and_draw(results, frame, tracker, fps);

                    // 处理完成后才显示并统计为“完成帧”
                    auto show_start_time = std::chrono::steady_clock::now();
                    cv::imshow("YOLOv8", frame);
                    fps.increment_frame();
                    auto show_end_time = std::chrono::steady_clock::now();
                    
                    // 统计显示耗时
                    double show_cost = std::chrono::duration<double, std::milli>(show_end_time - show_start_time).count();
                    temp_show_time += show_cost;

                    total_completed_frame++;  // 只有处理完的帧才累计
                } catch (...) { std::cerr << "处理帧失败\n"; }
            }
            futures.erase(futures.begin());

            // 统计处理结果耗时（包含推理等待+跟踪+绘制）
            auto process_end_time = std::chrono::steady_clock::now();
            double process_cost = std::chrono::duration<double, std::milli>(process_end_time - process_start_time).count();
            temp_process_time += process_cost;
        } else {
            // 未处理的帧不显示（或显示原始帧，但不计入完成帧）
            cv::imshow("YOLOv8", frame);
        }

        // 每10个完成帧打印一次统计（用完成帧计数，更准确）
        if (total_completed_frame % 10 == 0 && total_completed_frame != last_completed_frame) {
            // 计算最近10个完成帧的耗时
            auto current_stat_time = std::chrono::steady_clock::now();
            double stat_elapsed = std::chrono::duration<double>(current_stat_time - last_stat_time).count();
            int frame_since_last = total_completed_frame - last_completed_frame;

            // 计算平均耗时（最近10帧）
            double avg_read = temp_read_time / frame_since_last;
            double avg_submit = temp_submit_time / frame_since_last;
            double avg_process = temp_process_time / frame_since_last;
            double avg_show = temp_show_time / frame_since_last;
            double avg_total = avg_read + avg_submit + avg_process + avg_show;

            // 计算最近10帧的实际FPS（帧数量 / 耗时）
            double recent_fps = frame_since_last / stat_elapsed;

            // 打印统计信息
            printf("[fps性能统计] 已读取:%d | 已完成:%d | 平均总耗时: %.2fms | 最近FPS: %.1f\n",
                   total_read_frame, total_completed_frame, avg_total, recent_fps);
            printf("[fps监测]帧读取: %.2fms | 提交任务: %.2fms | 处理结果: %.2fms | 显示: %.2fms\n",
                   avg_read, avg_submit, avg_process, avg_show);

            // 重置临时累计值
            temp_read_time = 0;
            temp_submit_time = 0;
            temp_process_time = 0;
            temp_show_time = 0;
            last_completed_frame = total_completed_frame;
            last_stat_time = current_stat_time;
        }

        // 记录当前帧结束时间
        last_frame_end_time = std::chrono::steady_clock::now();

        // 按键退出（按ESC）
        if (cv::waitKey(1) == 27) running = false;
    }

    // -------------------------- 新增修改1：处理剩余未完成的推理任务 --------------------------
    // 避免futures.clear()时因未完成任务阻塞，确保全局统计能执行
    printf("\n正在等待剩余推理任务完成...\n");
    for (auto& f : futures) {
        if (f.valid()) {
            try {
                // 等待任务完成（超时1秒，防止无限阻塞）
                if (f.wait_for(std::chrono::seconds(1)) == std::future_status::ready) {
                    auto unused = f.get();  // 读取结果，释放future资源
                    total_completed_frame++; // 补加未统计的完成帧
                } else {
                    std::cerr << "部分推理任务超时未完成，已跳过\n";
                }
            } catch (...) {
                std::cerr << "处理剩余推理任务时出错\n";
            }
        }
    }
    futures.clear(); // 此时futures已无阻塞任务，可安全清理
    // ----------------------------------------------------------------------------------------

    // 最终全局统计
    auto global_end_time = std::chrono::steady_clock::now();
    double global_elapsed = std::chrono::duration<double>(global_end_time - global_start_time).count();
    double global_fps = total_completed_frame / global_elapsed;
    printf("\n[fps全局统计] 总完成帧: %d | 总耗时: %.2fs | 平均FPS: %.1f\n",
           total_completed_frame, global_elapsed, global_fps);
    
    // -------------------------- 新增修改2：强制刷新控制台输出 --------------------------
    fflush(stdout); // 确保全局统计信息能立即显示，避免卡在输出缓冲区
    // ----------------------------------------------------------------------------------------

    // 资源释放
    model.release();
    cap.release();
    cv::destroyAllWindows();
    return 0;
}