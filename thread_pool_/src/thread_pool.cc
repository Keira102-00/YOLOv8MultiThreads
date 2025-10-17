#include "thread_pool.h"
#include <iostream>
#include <fstream>
#include <string>

// 检查板子型号是否支持模型初始化
bool check_board_model_support() {
    std::string board_name;
    std::ifstream file("/sys/ztl/board_name"); // path /sys/ztl/board_name
    
    if (file.is_open()) {
        std::getline(file, board_name);
        file.close();
        
        // 去除可能的空白字符
        board_name.erase(board_name.find_last_not_of(" \t\n\r") + 1);
        
        // 检查支持的板子型号
        const std::string supported_models[] = {"A588", "588", "576", "566", "568", "562"};
        for (const auto& model : supported_models) {
            if (board_name == model) {
                return true;
            }
        }
    }
    
    std::cout << "BERROR：模型初始化失败" << std::endl;
    return false;
}

ThreadPool::ThreadPool(size_t thread_count) {
    if (!check_board_model_support()) {
        std::exit(EXIT_FAILURE);  // 直接退出程序
    }
    
    // 创建指定数量的工作线程
    for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    // 停止线程池
    running_ = false;
    condition_.notify_all();
    
    // 等待所有线程完成
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ThreadPool::worker() {
    while (running_) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this]() {
                return !running_ || !tasks_.empty();
            });
            
            if (!running_ && tasks_.empty()) {
                return;
            }
            
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }
        
        if (task) {
            task();
        }
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    finished_condition_.wait(lock, [this]() {
        return tasks_.empty() && active_tasks_ == 0;
    });
}