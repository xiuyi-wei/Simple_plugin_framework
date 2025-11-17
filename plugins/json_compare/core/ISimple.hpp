#pragma once
#include "core/IObject.hpp"

namespace core {

class ISimple : public IObject {
public:
    DEFINE_IID(ISimple);
    virtual ~ISimple() override = default;
    virtual int add(int a, int b) = 0;
};

} // namespace core
