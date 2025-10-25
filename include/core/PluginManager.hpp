#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <vector>

#include "core/IObject.hpp"

// Platform-specific includes for dynamic loading
#if defined(_WIN32)
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

// Factory type used by plugins
#if defined(_WIN32)
using CreateFunc = std::function<std::shared_ptr<core::IObject>()>;
#else
using CreateFunc = std::function<std::shared_ptr<core::IObject>()>;
#endif

namespace core {

class PluginManager {
public:
    static PluginManager& instance() {
        static PluginManager inst;
        return inst;
    }

    // Alternate accessor to avoid symbol collisions with macros named 'instance'
    static PluginManager& get() {
        return instance();
    }

    void registerClass(const std::string& clsid, CreateFunc func) {
        registry_[clsid] = std::move(func);
        std::cout << "[PluginManager] register: " << clsid << std::endl;
    }

    std::shared_ptr<IObject> create(const std::string& clsid) {
        auto it = registry_.find(clsid);
        if (it != registry_.end()) return it->second();
        std::cerr << "[PluginManager] not found: " << clsid << std::endl;
        return nullptr;
    }

    // Load a plugin from a shared library (.so/.dll)
    bool loadPlugin(const std::string& path) {
#if defined(_WIN32)
        HMODULE handle = LoadLibraryA(path.c_str());
        if (!handle) {
            std::cerr << "[PluginManager] cannot load: " << path << std::endl;
            return false;
        }

        using RegFunc = void(*)(PluginManager&);
        RegFunc func = reinterpret_cast<RegFunc>(GetProcAddress(handle, "registerPlugin"));
        if (!func) {
            std::cerr << "[PluginManager] registerPlugin() not found" << std::endl;
            FreeLibrary(handle);
            return false;
        }

        func(*this);
        handles_.push_back(reinterpret_cast<void*>(handle));
        std::cout << "[PluginManager] loaded: " << path << std::endl;
        return true;
#else
        void* handle = dlopen(path.c_str(), RTLD_NOW);
        if (!handle) {
            std::cerr << "[PluginManager] cannot load: " << path
                      << " -> " << dlerror() << std::endl;
            return false;
        }

        using RegFunc = void(*)(PluginManager&);
        RegFunc func = reinterpret_cast<RegFunc>(dlsym(handle, "registerPlugin"));
        if (!func) {
            std::cerr << "[PluginManager] registerPlugin() not found" << std::endl;
            dlclose(handle);
            return false;
        }

        func(*this);
        handles_.push_back(handle);
        std::cout << "[PluginManager] loaded: " << path << std::endl;
        return true;
#endif
    }

    void unloadAll() {
#if defined(_WIN32)
        for (void* h : handles_) FreeLibrary(reinterpret_cast<HMODULE>(h));
#else
        for (void* h : handles_) dlclose(h);
#endif
        handles_.clear();
        registry_.clear();
    }

private:
    std::unordered_map<std::string, CreateFunc> registry_;
    std::vector<void*> handles_;
};

} // namespace core

// Simple free function accessor (avoids any identifier collision with macros)
inline core::PluginManager& pluginManager() {
    static core::PluginManager inst;
    return inst;
}
