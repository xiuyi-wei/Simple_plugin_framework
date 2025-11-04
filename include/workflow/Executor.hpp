#pragma once
#include <unordered_map>
#include <vector>
#include <queue>
#include <string>
#include <memory>
#include <atomic>
#include "workflow/Workflow.hpp"
#include "workflow/ITaskContext.hpp"
#include "runtime/EventBus.hpp"
#include "runtime/ThreadPool.hpp"

namespace wf {

class SimpleContext : public ITaskContext {
public:
    SimpleContext(rt::ILogger& l, rt::IClock& c, rt::IFileSystem& f)
        : logger_(l), clock_(c), fs_(f) {}
    std::string get(const std::string& key) const override {
        std::lock_guard<std::mutex> g(m_);
        auto it = kv_.find(key);
        if (it == kv_.end()) return std::string();
        return it->second;
    }
    void set(const std::string& key, std::string value) override {
        std::lock_guard<std::mutex> g(m_);
        kv_[key] = std::move(value);
    }
    rt::ILogger& logger() override { return logger_; }
    rt::IClock& clock() override { return clock_; }
    rt::IFileSystem& fs() override { return fs_; }
private:
    std::unordered_map<std::string, std::string> kv_;
    mutable std::mutex m_;
    rt::ILogger& logger_;
    rt::IClock& clock_;
    rt::IFileSystem& fs_;
};

class Executor {
public:
    Executor(rt::EventBus& bus, rt::ThreadPool& pool)
        : bus_(bus), pool_(pool) {}

    bool run(const WorkflowSpec& wf, ITaskContext& ctx);

private:
    rt::EventBus& bus_;
    rt::ThreadPool& pool_;
};

} // namespace wf
