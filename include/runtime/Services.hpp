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

// ===== 扩展文件系统接口（满足过滤器/ShellTask 需求） =====
struct IFileSystem {
    virtual ~IFileSystem() = default;

    // 基础
    virtual bool exists(const std::string& path) const = 0;

    // 路径处理
    virtual std::string join(const std::string& a, const std::string& b) const = 0;
    virtual std::string dirname(const std::string& p) const = 0;
    virtual std::string basename(const std::string& p) const = 0;
    virtual std::string normalize(const std::string& p) const = 0;
    virtual std::string absolute(const std::string& p) const = 0;

    // 目录保障
    virtual bool ensureDir(const std::string& dir) const = 0;        // 递归创建
    virtual bool ensureParentDir(const std::string& path) const = 0; // 为文件创建上级目录
};

// ====== Minimal concrete implementations ======
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

    std::string join(const std::string& a, const std::string& b) const override;
    std::string dirname(const std::string& p) const override;
    std::string basename(const std::string& p) const override;
    std::string normalize(const std::string& p) const override;
    std::string absolute(const std::string& p) const override;

    bool ensureDir(const std::string& dir) const override;
    bool ensureParentDir(const std::string& path) const override;
};

} // namespace rt
