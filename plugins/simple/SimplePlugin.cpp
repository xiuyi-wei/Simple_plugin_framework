#include "core/IObject.hpp"
#include <string>

using namespace core;

class SimplePlugin : public IObject {
public:
    SimplePlugin() = default;
    ~SimplePlugin() override = default;
    std::string name() const override { return "SimplePlugin"; }
};

// C-style factory so the host can create an instance when loading the library.
extern "C" IObject* create_plugin() {
    return new SimplePlugin();
}

// Optional destroy function
extern "C" void destroy_plugin(IObject* p) {
    delete p;
}
