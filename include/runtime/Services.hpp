#pragma once
#include <string>
#include <chrono>

namespace rt {

struct ILogger {
    virtual ~ILogger() = default;
    virtual void info(const std::string& msg) = 0;
    virtual void warn(const std::string& msg) = 0;
    virtual void error(const std::string& msg) = 0;
};

struct IClock {
    virtual ~IClock() = default;
    virtual std::chrono::steady_clock::time_point now() const = 0;
};

struct IFileSystem {
    virtual ~IFileSystem() = default;
    virtual bool exists(const std::string& path) const = 0;
};

// Minimal concrete implementations for immediate use
class StdLogger : public ILogger {
public:
    void info(const std::string& msg) override;
    void warn(const std::string& msg) override;
    void error(const std::string& msg) override;
};

class SteadyClock : public IClock {
public:
    std::chrono::steady_clock::time_point now() const override;
};

class LocalFS : public IFileSystem {
public:
    bool exists(const std::string& path) const override;
};

} // namespace rt

