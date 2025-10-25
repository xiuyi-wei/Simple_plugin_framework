#include <iostream>
#include <memory>
#include "core/PluginManager.hpp"
#include "core/ISimple.hpp"
#include "core/IComparator.hpp"

using namespace core;

int main() {
    auto& pm = PluginManager::get();

#if defined(_WIN32)
    const char* pluginPath = "./plugins/json_compare.dll"; // app is placed in build/bin
#else
    const char* pluginPath = "./plugins/libjson_compare.so"; // app is placed in build/bin
#endif

    if (!pm.loadPlugin(pluginPath)) {
        std::cerr << "Cannot load plugin: " << pluginPath << std::endl;
        return 1;
    }

    auto obj = pm.create("clsidJsonComparator");
    if (!obj) return 1;

    auto comp = std::dynamic_pointer_cast<core::IComparator>(obj);
    if (comp) {
        comp->compareFiles("file1.json","file2.json","report.txt");
    }

    pm.unloadAll();
    return 0;
}
