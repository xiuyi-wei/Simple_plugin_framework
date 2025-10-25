#include <iostream>
#include <core/PluginManager.hpp>

int main() {
    auto& pm = PluginManager::instance();

    pm.loadPlugin("./plugins/libsimple.so");  // Linux下默认输出libsimple.so

    auto obj = pm.create("clsidSimple");
    if (!obj) return 1;

    auto simple = std::dynamic_pointer_cast<ISimple>(obj);
    if (simple) {
        int res = simple->add(3, 7);
        std::cout << "结果 = " << res << std::endl;
    }

    pm.unloadAll();
    return 0;
}
