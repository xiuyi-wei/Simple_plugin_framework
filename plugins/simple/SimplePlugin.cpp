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

// 插件导出注册函数，宿主会在加载后调用 registerPlugin
extern "C" void registerPlugin(PluginManager& m) {
    m.registerClass("clsidSimple", []() -> std::shared_ptr<IObject> {
        return std::make_shared<CSimple>();
    });
    std::cout << "[SimplePlugin] registered CSimple" << std::endl;
}
