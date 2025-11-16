#pragma once
#include "core/IObject.hpp"
#include <string>

namespace core {

class IJsonProcess : public IObject {
public:
    DEFINE_IID(IJsonProcess);
    virtual ~IJsonProcess() override = default;

    // Compare two JSON files and write a human-readable report to outPath.
    // Returns true if operation succeeded (report generated), false on error.
    virtual bool processJsonFiles(const std::string& srcPath,
                                  const std::string& desPath,
                                  const std::string& keywords,
                                  const std::string& processMethod) = 0;
};

} // namespace core
