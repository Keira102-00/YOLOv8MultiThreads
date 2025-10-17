#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <sys/time.h>
#include <atomic>
#include <string>
#include <mutex>

// FPS统计器（封装FPS计算逻辑，隐藏内部变量）
class FPSCounter {
public:
    FPSCounter();
    ~FPSCounter() = default;

    // 每处理一帧调用一次（线程安全）
    void increment_frame();
    // 获取当前FPS（每秒自动更新）
    float get_current_fps() const;

private:
    mutable std::atomic<int> frame_count;    // 原子变量，保证线程安全
    mutable struct timeval fps_start_time;   // 统计窗口起始时间
    mutable float current_fps;               // 当前FPS值
    mutable std::mutex fps_mutex;            // 保护FPS计算的互斥锁
};

// 线程安全打印（避免多线程打印乱码）
void safe_printf(const char* format, ...); 

// 时间转换：timeval → 微秒
double timeval_to_us(const struct timeval& t);

#endif // COMMON_UTILS_H