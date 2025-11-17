#pragma once
#include <string>

// Common base for plugin interfaces
namespace core {

struct IObject {
    virtual ~IObject() = default;
};

} // namespace core

// Macro to define a simple IID getter for interface types
#define DEFINE_IID(name) \
    static const char* IID() { return #name; }
