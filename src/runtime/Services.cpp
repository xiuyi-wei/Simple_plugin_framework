#include "../../include/runtime/Services.hpp"
#include <iostream>
#include <sys/stat.h>

namespace rt {

void StdLogger::info(const std::string& msg)  { std::cout << "[INFO] "  << msg << std::endl; }
void StdLogger::warn(const std::string& msg)  { std::cout << "[WARN] "  << msg << std::endl; }
void StdLogger::error(const std::string& msg) { std::cerr << "[ERROR] " << msg << std::endl; }

std::chrono::steady_clock::time_point SteadyClock::now() const {
    return std::chrono::steady_clock::now();
}

static bool fileExistsPortable(const std::string& path) {
#if defined(_WIN32)
    struct _stat64 st{};
    return _stat64(path.c_str(), &st) == 0;
#else
    struct stat st{};
    return stat(path.c_str(), &st) == 0;
#endif
}

bool LocalFS::exists(const std::string& path) const {
    return fileExistsPortable(path);
}

} // namespace rt

