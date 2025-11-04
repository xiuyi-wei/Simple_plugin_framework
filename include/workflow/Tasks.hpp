#pragma once
#include <string>
#include <memory>
#include "workflow/ITask.hpp"
#include "workflow/ITaskContext.hpp"
#include "runtime/Services.hpp"
#include <cstdlib>

namespace wf {

class ConstTask : public ITask {
public:
    ConstTask(std::string id, std::string key, std::string value)
        : id_(std::move(id)), key_(std::move(key)), value_(std::move(value)) {}
    std::string id() const override { return id_; }
    TaskResult run(ITaskContext& ctx) override {
        ctx.set(key_, value_); return {true, {}};
    }
private:
    std::string id_, key_, value_;
};

class AddTask : public ITask {
public:
    AddTask(std::string id, std::string a, std::string b, std::string out)
        : id_(std::move(id)), a_(std::move(a)), b_(std::move(b)), out_(std::move(out)) {}
    std::string id() const override { return id_; }
    TaskResult run(ITaskContext& ctx) override {
        try {
            int ai = std::stoi(ctx.get(a_));
            int bi = std::stoi(ctx.get(b_));
            ctx.set(out_, std::to_string(ai + bi));
            return {true, {}};
        } catch(...) { return {false, "AddTask parse error"}; }
    }
private:
    std::string id_, a_, b_, out_;
};

class MulTask : public ITask {
public:
    MulTask(std::string id, std::string a, std::string b, std::string out)
        : id_(std::move(id)), a_(std::move(a)), b_(std::move(b)), out_(std::move(out)) {}
    std::string id() const override { return id_; }
    TaskResult run(ITaskContext& ctx) override {
        try {
            int ai = std::stoi(ctx.get(a_));
            int bi = std::stoi(ctx.get(b_));
            ctx.set(out_, std::to_string(ai * bi));
            return {true, {}};
        } catch(...) { return {false, "MulTask parse error"}; }
    }
private:
    std::string id_, a_, b_, out_;
};

// Execute a shell command; on success (exit code 0) optionally set an output key.
class ShellTask : public ITask {
public:
    ShellTask(std::string id,
              std::string cmd,
              std::string outKey = std::string(),
              std::string outValue = std::string(),
              bool checkExists = false)
        : id_(std::move(id)), cmd_(std::move(cmd)),
          outKey_(std::move(outKey)), outValue_(std::move(outValue)),
          checkExists_(checkExists) {}

    std::string id() const override { return id_; }
    TaskResult run(ITaskContext& ctx) override {
        int rc = std::system(cmd_.c_str());
        if (rc != 0) return {false, "ShellTask exit code != 0"};
        if (!outKey_.empty()) {
            if (checkExists_ && !outValue_.empty()) {
                if (!ctx.fs().exists(outValue_)) {
                    return {false, std::string("ShellTask expected output not found: ") + outValue_};
                }
            }
            ctx.set(outKey_, outValue_);
        }
        return {true, {}};
    }
private:
    std::string id_, cmd_, outKey_, outValue_;
    bool checkExists_ = false;
};

} // namespace wf
