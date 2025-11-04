#pragma once
#include <string>
#include <memory>

namespace wf {

struct ITaskContext;

struct TaskResult {
    bool success = true;
    std::string message;
};

class ITask {
public:
    virtual ~ITask() = default;
    virtual std::string id() const = 0;
    virtual TaskResult run(ITaskContext& ctx) = 0;
};

} // namespace wf

