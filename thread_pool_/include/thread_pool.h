#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <memory>

class ThreadPool {
public:
    // 构造函数，指定线程数
    explicit ThreadPool(size_t thread_count);
    
    // 析构函数
    ~ThreadPool();
    
    // 禁止拷贝和移动
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    
    // 提交任务到线程池
    template<class F, class... Args>
    auto put(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

    // 获取任务结果
    template<class T>
    T get(std::future<T>& future);
    
    // 等待所有任务完成
    void wait();
    
    // 获取当前线程数
    size_t thread_count() const { return threads_.size(); }

private:
    // 工作线程函数
    void worker();
    
    // 线程池是否运行中
    std::atomic<bool> running_{true};
    
    // 工作线程
    std::vector<std::thread> threads_;
    
    // 任务队列
    std::queue<std::function<void()>> tasks_;
    
    // 同步变量
    std::mutex mutex_;
    std::condition_variable condition_;
    std::condition_variable finished_condition_;
    std::atomic<size_t> active_tasks_{0};
};

// 提交任务的实现
template<class F, class... Args>
auto ThreadPool::put(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<decltype(f(args...))> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.emplace([task, this]() {
            active_tasks_++;
            try {
                (*task)();
            } catch (...) {
                // 异常处理
            }
            active_tasks_--;
            finished_condition_.notify_one();
        });
    }
    
    condition_.notify_one();
    return result;
}

// 获取任务结果的实现
template<class T>
T ThreadPool::get(std::future<T>& future) {
    return future.get();
}

#endif // THREAD_POOL_H