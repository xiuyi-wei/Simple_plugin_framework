#pragma once
#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace rt {

// Minimal, self-contained thread pool for workflow executor
class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

    void shutdown();

private:
    void workerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex m_;
    std::condition_variable cv_;
    std::atomic<bool> stopping_{false};
};

inline ThreadPool::ThreadPool(size_t threads) {
    if (threads == 0) threads = 2;
    workers_.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this]{ workerLoop(); });
    }
}

inline ThreadPool::~ThreadPool() { shutdown(); }

inline void ThreadPool::shutdown() {
    bool expected = false;
    if (!stopping_.compare_exchange_strong(expected, true)) return;
    cv_.notify_all();
    for (auto& t : workers_) if (t.joinable()) t.join();
}

inline void ThreadPool::workerLoop() {
    for(;;) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this]{ return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

template<class F, class... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using R = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<R()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<R> res = task->get_future();
    {
        std::lock_guard<std::mutex> g(m_);
        tasks_.emplace([task]{ (*task)(); });
    }
    cv_.notify_one();
    return res;
}

} // namespace rt

