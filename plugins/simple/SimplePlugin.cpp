#include <iostream>
#include <memory>
#include "core/ISimple.hpp"
#include "core/PluginManager.hpp"

using namespace core;

class CSimple : public ISimple {
public:
    CSimple() = default;
    ~CSimple() override = default;
    int add(int a, int b) override {
        std::cout << "[CSimple] add(" << a << ", " << b << ")" << std::endl;
        return a + b;
    }
};

#if defined(_WIN32)
  #define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
  #define PLUGIN_EXPORT extern "C"
#endif

// ★ PluginManager 期望的符号名：registerPlugin
//   具体签名请以你的 PluginManager::loadPlugin() 里 GetProcAddress 的声明为准：
//   常见两种：void registerPlugin(core::PluginManager&)
//            或   void registerPlugin(core::PluginManager*)
PLUGIN_EXPORT void registerPlugin(core::PluginManager& pm) {
    pm.registerClass("clsidSimple", [](){
        return std::shared_ptr<core::IObject>(new CSimple());
    });
}