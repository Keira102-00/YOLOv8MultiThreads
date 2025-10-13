#include "model_wrapper.h"
#include "common_utils.h"
#include "image_utils.h"
#include <mutex>
#include <atomic>
#include <opencv2/opencv.hpp>

Yolov8Model::~Yolov8Model() {
    release();
}

bool Yolov8Model::init(const std::string& model_path, int thread_num) {
    // 初始化线程池
    thread_pool_ = new ThreadPool(thread_num);
    if (!thread_pool_) {
        safe_printf("Failed to create thread pool");
        return false;
    }

    // 初始化多线程模型上下文
    model_contexts_.clear();
    for (int i = 0; i < thread_num; ++i) {
        rknn_app_context_t ctx;
        memset(&ctx, 0, sizeof(rknn_app_context_t));
        int ret = init_yolov8_model(model_path.c_str(), &ctx);
        if (ret != 0) {
            safe_printf("Failed to init model context (thread %d), ret=%d", i, ret);
            release();  // 释放已初始化的上下文
            return false;
        }
        model_contexts_.push_back(ctx);
    }

    is_inited_ = true;
    safe_printf("Model initialized: %s, thread num: %d", model_path.c_str(), thread_num);
    return true;
}

std::future<object_detect_result_list> Yolov8Model::submit_infer_task(const cv::Mat& frame) {
    if (!is_inited_) {
        safe_printf("Model not initialized, cannot submit task");
        // 返回空future（应用层需判断valid()）
        return std::future<object_detect_result_list>();
    }

    // 提交异步推理任务
    return thread_pool_->put([this, frame]() {
        rknn_app_context_t& ctx = get_next_context();
        return infer_internal(frame, ctx);
    });
}

void Yolov8Model::release() {
    // 释放模型上下文
    for (auto& ctx : model_contexts_) {
        release_yolov8_model(&ctx);
    }
    model_contexts_.clear();

    // 释放线程池
    if (thread_pool_) {
        delete thread_pool_;
        thread_pool_ = nullptr;
    }

    is_inited_ = false;
    safe_printf("Model released");
}

bool Yolov8Model::is_inited() const {
    return is_inited_;
}

// 内部推理实现
object_detect_result_list Yolov8Model::infer_internal(const cv::Mat& frame, rknn_app_context_t& ctx) {
    object_detect_result_list od_results = {0};
    image_buffer_t src_image = {0};

    // 图像格式转换（BGR→RGB）
    cv::Mat rgb_frame;
    cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);

    // 初始化图像缓冲区
    src_image.width = rgb_frame.cols;
    src_image.height = rgb_frame.rows;
    src_image.format = IMAGE_FORMAT_RGB888;
    src_image.virt_addr = new unsigned char[rgb_frame.total() * 3];
    memcpy(src_image.virt_addr, rgb_frame.data, rgb_frame.total() * 3);

    // 调用底层推理接口
    int ret = inference_yolov8_model(&ctx, &src_image, &od_results);
    if (ret != 0) {
        // safe_printf("Infer failed, ret=" + std::to_string(ret));
        safe_printf("Infer failed, ret=%d", ret);
    }

    // 释放图像缓冲区
    delete[] src_image.virt_addr;
    src_image.virt_addr = nullptr;

    return od_results;
}

// 轮询获取模型上下文（线程安全）
rknn_app_context_t& Yolov8Model::get_next_context() {
    static std::atomic<int> ctx_index(0);
    static std::mutex ctx_mutex;

    std::lock_guard<std::mutex> lock(ctx_mutex);
    int idx = ctx_index++ % model_contexts_.size();
    return model_contexts_[idx];
}