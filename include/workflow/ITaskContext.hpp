#pragma once
#include <string>
#include <unordered_map>
#include <memory>

namespace rt { struct ILogger; struct IClock; struct IFileSystem; }

namespace wf {

// Lightweight key-value bag + service accessors
struct ITaskContext {
    virtual ~ITaskContext() = default;

    virtual std::string get(const std::string& key) const = 0;
    virtual void set(const std::string& key, std::string value) = 0;

    virtual rt::ILogger& logger() = 0;
    virtual rt::IClock& clock() = 0;
    virtual rt::IFileSystem& fs() = 0;
};

} // namespace wf
