#pragma once
#include "core/IObject.hpp"
#include <string>

namespace core {

class IComparator : public IObject {
public:
    DEFINE_IID(IComparator);
    virtual ~IComparator() override = default;

    // Compare two JSON files and write a human-readable report to outPath.
    // Returns true if operation succeeded (report generated), false on error.
    virtual bool compareFiles(const std::string& oursPath,
                              const std::string& goldenPath,
                              const std::string& outPath) = 0;
};

} // namespace core
