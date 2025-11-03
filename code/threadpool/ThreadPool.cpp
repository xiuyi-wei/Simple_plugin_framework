#include "../../include/threadpool/ThreadPool.hpp"
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <chrono>

namespace tp {

ThreadPool::ThreadPool(size_t threads) {
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 2;
    }

    rng_.seed(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    dist_ = std::uniform_int_distribution<size_t>(0, threads - 1);

    deques_.resize(threads);
    workers_.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back(std::bind(&ThreadPool::workerLoop, this, i));
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    bool expected = false;
    if (!stopping_.compare_exchange_strong(expected, true)) return;
    cond_.notify_all();
    for (std::thread &t : workers_) if (t.joinable()) t.join();
    workers_.clear();
}

void ThreadPool::resize(size_t threads) {
    // Simple resize: if increasing, add workers; if decreasing, set stopping and recreate
    std::unique_lock<std::mutex> lk(mutex_);
    if (threads == workers_.size()) return;
    if (threads > workers_.size()) {
        size_t old = workers_.size();
        deques_.resize(threads);
        for (size_t i = old; i < threads; ++i) {
            workers_.emplace_back(std::bind(&ThreadPool::workerLoop, this, i));
        }
    } else {
        // decreasing: signal all workers to stop and recreate smaller pool
        lk.unlock();
        shutdown();
        // move remaining tasks to new deques
        deques_.clear();
        deques_.resize(threads);
        stopping_ = false;
        workers_.reserve(threads);
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back(std::bind(&ThreadPool::workerLoop, this, i));
        }
    }
    dist_ = std::uniform_int_distribution<size_t>(0, workers_.size() - 1);
}

size_t ThreadPool::size() const {
    return workers_.size();
}

size_t ThreadPool::queueSize() const {
    // Best-effort snapshot
    size_t sum = 0;
    for (size_t i = 0; i < deques_.size(); ++i) {
        // no locking for perf; approximate
        sum += deques_[i].size();
    }
    return sum;
}

uint64_t ThreadPool::completedTaskCount() const {
    return completedTasks_.load();
}

void ThreadPool::workerLoop(size_t id) {
    while (!stopping_) {
        std::function<void()> task;
        // pop from own deque
        {
            std::unique_lock<std::mutex> lk(mutex_);
            if (!deques_[id].empty()) {
                task = std::move(deques_[id].front());
                deques_[id].pop_front();
            }
        }
        if (!task) {
            // attempt to steal from another worker
            size_t n = deques_.size();
            bool found = false;
            for (size_t attempt = 0; attempt < n; ++attempt) {
                size_t victim = (id + 1 + (rng_() % n)) % n;
                if (victim == id) continue;
                std::unique_lock<std::mutex> lk(mutex_);
                if (!deques_[victim].empty()) {
                    task = std::move(deques_[victim].back());
                    deques_[victim].pop_back();
                    found = true;
                    break;
                }
            }
            if (!found) {
                // wait for work
                std::unique_lock<std::mutex> lk(mutex_);
                cond_.wait(lk, [this, id]{
                    if (stopping_) return true;
                    for (size_t i=0;i<deques_.size();++i) if (!deques_[i].empty()) return true;
                    return false;
                });
                continue; // retry loop to pick up a task
            }
        }

        // execute task and forward exceptions to future (task should set packaged_task)
        try {
            task();
        } catch (...) {
            // exceptions should already be set in packaged_task -> future, so swallow here
        }
        completedTasks_.fetch_add(1);
    }
}

template<class F, class... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using R = typename std::result_of<F(Args...)>::type;
    auto taskPtr = std::make_shared<std::packaged_task<R()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<R> res = taskPtr->get_future();

    if (stopping_) throw std::runtime_error("ThreadPool is stopping, cannot submit");

    // push to a random worker's deque
    size_t idx = dist_(rng_);
    {
        std::unique_lock<std::mutex> lk(mutex_);
        deques_[idx].push_back([taskPtr]{ (*taskPtr)(); });
        totalTasks_.fetch_add(1);
    }
    cond_.notify_all();
    return res;
}

template<class F, class... Args>
auto ThreadPool::try_submit(F&& f, Args&&... args)
    -> Optional<std::future<typename std::result_of<F(Args...)>::type>>
{
    using R = typename std::result_of<F(Args...)>::type;
    auto taskPtr = std::make_shared<std::packaged_task<R()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<R> res = taskPtr->get_future();

    if (stopping_) return Optional<std::future<R>>();

    // best-effort push to a random worker without blocking
    size_t idx = dist_(rng_);
    {
        std::unique_lock<std::mutex> lk(mutex_);
        deques_[idx].push_back([taskPtr]{ (*taskPtr)(); });
        totalTasks_.fetch_add(1);
    }
    cond_.notify_all();
    return Optional<std::future<R>>(std::move(res));
}

} // namespace tp

