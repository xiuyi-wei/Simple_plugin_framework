#pragma once

#include <string>
#include <vector>

namespace core {

class PluginManager {
public:
    PluginManager() = default;
    ~PluginManager() = default;

    // Load a plugin from a file path (path may be platform-dependent)
    // Returns true if the plugin file exists and was "registered" successfully.
    bool loadPlugin(const std::string& path);

    // Unregister a plugin by index
    bool unloadPlugin(size_t index);

    // List registered plugin paths
    const std::vector<std::string>& plugins() const { return plugins_; }

private:
    std::vector<std::string> plugins_;
};

} // namespace core
