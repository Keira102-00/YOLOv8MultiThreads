#ifndef MODEL_WRAPPER_H
#define MODEL_WRAPPER_H

#include <string>
#include <vector>
#include <future>
#include <opencv2/opencv.hpp>
#include "yolov8.h"  // 底层模型头文件
#include "thread_pool.h"  // 第三方线程池头文件

// 模型推理器（封装多线程模型、推理逻辑）
class Yolov8Model {
public:
    Yolov8Model() = default;
    ~Yolov8Model();

    // 初始化：模型路径 + 线程数（推理线程池大小）
    bool init(const std::string& model_path, int thread_num = 6);
    // 提交推理任务（异步，返回future）
    std::future<object_detect_result_list> submit_infer_task(const cv::Mat& frame);
    // 释放模型资源
    void release();
    // 判断模型是否初始化成功
    bool is_inited() const;

    ThreadPool* get_thread_pool() const {
        return thread_pool_;  // 返回私有线程池指针
    }

private:
    std::vector<rknn_app_context_t> model_contexts_;  // 多线程模型上下文
    ThreadPool* thread_pool_ = nullptr;               // 推理线程池
    bool is_inited_ = false;                          // 初始化状态标记

    // 内部推理函数（供线程池调用）
    object_detect_result_list infer_internal(const cv::Mat& frame, rknn_app_context_t& ctx);
    // 轮询获取可用模型上下文（线程安全）
    rknn_app_context_t& get_next_context();
};

#endif // MODEL_WRAPPER_H
