#include "common_utils.h"
#include <cstdio>
#include <cerrno>
#include <cstdarg> 

// FPSCounter实现
FPSCounter::FPSCounter() : frame_count(0), current_fps(0.0f) {
    gettimeofday(&fps_start_time, nullptr);
}

void FPSCounter::increment_frame() {
    frame_count++;
}

float FPSCounter::get_current_fps() const{
    struct timeval now_time;
    gettimeofday(&now_time, nullptr);
    double elapsed = timeval_to_us(now_time) - timeval_to_us(fps_start_time);

    std::lock_guard<std::mutex> lock(fps_mutex);
    if (elapsed >= 1000000) {  // 每秒更新一次
        current_fps = frame_count * 1000000.0f / elapsed;
        frame_count = 0;
        gettimeofday(&fps_start_time, nullptr);
    }
    return current_fps;
}

// 线程安全打印实现
void safe_printf(const char* format, ...) {
    static std::mutex print_mutex;
    std::lock_guard<std::mutex> lock(print_mutex);  // 线程安全保护

    // 使用va_list处理可变参数
    va_list args;
    va_start(args, format);
    vprintf(format, args);  // 用vprintf处理格式化输出
    va_end(args);
    printf("\n");  // 补充换行（保持原行为）
}

// 时间转换实现
double timeval_to_us(const struct timeval& t) {
    return t.tv_sec * 1000000.0 + t.tv_usec;
}