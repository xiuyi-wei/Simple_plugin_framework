#include <iostream>
#include <memory>
#include "core/PluginManager.hpp"
#include "core/ISimple.hpp"

using namespace core;

int main() {
    auto& pm = PluginManager::get();

#if defined(_WIN32)
    const char* pluginPath = "../../plugins/simple.dll"; // app is placed in build/bin
#else
    const char* pluginPath = "../plugins/libsimple.so"; // app is placed in build/bin
#endif

    if (!pm.loadPlugin(pluginPath)) {
        std::cerr << "Cannot load plugin: " << pluginPath << std::endl;
        return 1;
    }

    auto obj = pm.create("clsidSimple");
    if (!obj) return 1;

    auto simple = std::dynamic_pointer_cast<core::ISimple>(obj);
    if (simple) {
    int res = simple->add(3, 7);
    std::cout << "result = " << res << std::endl;
    }

    pm.unloadAll();
    return 0;
}
