#pragma once
#include "core/IObject.hpp"
#include <string>

namespace core {

class IJsonProcess : public IObject {
public:
    DEFINE_IID(IJsonProcess);
    virtual ~IJsonProcess() override = default;

    // Process JSON content and return a generated report through outContent.
    // Returns true if operation succeeded, false on error.
    virtual bool processJsonFiles(const std::string& srcContent,
                                  std::string& outContent,
                                  const std::string& keywords,
                                  const std::string& processMethod) = 0;
};

} // namespace core
