#pragma once
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <cstdint>

namespace wf {

struct ITaskContext;

struct TaskResult {
    bool success = true;
    std::string message;
};

// ITask is the base node of the workflow DAG.
// It defines the minimal execution contract (id + run) and
// optional metadata/lifecycle/cancellation hooks that executors
// or tools may leverage. New virtuals have defaults to keep
// existing tasks source-compatible.
class ITask {
public:
    virtual ~ITask() = default;

    // Identity (stable across a workflow run)
    virtual std::string id() const = 0;

    // Core execution. Implement task logic and return success/failure.
    virtual TaskResult run(ITaskContext& ctx) = 0;

    // --- Optional metadata (defaults provided) ---
    // Human-friendly type/kind, label and description.
    virtual std::string kind() const { return "Task"; }
    virtual std::string label() const { return id(); }
    virtual std::string description() const { return {}; }

    // Declared context inputs/outputs for validation/visualization.
    // Defaults to empty sets.
    virtual const std::vector<std::string>& inputs() const {
        static const std::vector<std::string> kEmpty;
        return kEmpty;
    }
    virtual const std::vector<std::string>& outputs() const {
        static const std::vector<std::string> kEmpty;
        return kEmpty;
    }
    // Optional tags for grouping/filtering.
    virtual const std::vector<std::string>& tags() const {
        static const std::vector<std::string> kEmpty;
        return kEmpty;
    }

    // Hints for schedulers/executors. Not enforced by default executor.
    virtual int maxRetries() const { return 0; }              // 0 = no retry
    virtual std::uint32_t retryBackoffMs() const { return 0; } // fixed backoff
    virtual std::uint32_t timeoutMs() const { return 0; }      // 0 = no timeout
    virtual bool continueOnFailure() const { return false; }
    // Barrier task suggests no parallel successors until it completes.
    virtual bool isBarrier() const { return false; }

    // Lifecycle hooks; executor may call around run(). No-ops by default.
    virtual void onStart(ITaskContext&) {}
    virtual void onSuccess(ITaskContext&) {}
    virtual void onFailure(ITaskContext&, const std::string&) {}

    // Cooperative cancellation support for long-running tasks.
    void requestCancel() { cancelled_.store(true, std::memory_order_relaxed); }
    bool isCancelled() const { return cancelled_.load(std::memory_order_relaxed); }

protected:
    ITask() = default;

private:
    std::atomic<bool> cancelled_{false};
};

} // namespace wf

