// plugins/json_compare/JsonCompare.cpp
#include "core/IJsonProcess.hpp"
#include "core/PluginManager.hpp"
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>
#include <algorithm>
#include <memory>
#include <vector>

using json = nlohmann::ordered_json;  // 保持对象键的插入顺序

namespace {
// Traverse strictly along the provided path; arrays are expanded but no other
// keys at the same level are scanned.
void collect_by_path(const json& node,
                     const std::vector<std::string>& keys,
                     size_t idx,
                     std::vector<json>& results) {
    if (idx >= keys.size()) {
        results.push_back(node);
        return;
    }

    if (node.is_object()) {
        auto it = node.find(keys[idx]);
        if (it == node.end()) return;
        collect_by_path(it.value(), keys, idx + 1, results);
    } else if (node.is_array()) {
        for (const auto& element : node) {
            collect_by_path(element, keys, idx, results);
        }
    }
}
} // namespace

namespace core {

class JsonProcess : public IJsonProcess {
public:
    bool processJsonFiles(const std::string& srcContent,
                          std::string& outContent,
                          const std::string& keywords,
                          const std::string& processMethod) override {
        json srcJson;
        try {
            srcJson = json::parse(srcContent);
        } catch (const std::exception& e) {
            std::cerr << "[JsonProcess] parse failed: " << e.what() << "\n";
            return false;
        }

        json result = json::object();
        result["processMethod"] = processMethod;
        result["keywords"] = keywords;
        result["results"] = json::array();

        if (processMethod == "find") {
            std::cout << "[JsonProcess] processMethod: find\n";
            std::vector<std::string> keys;
            std::string token;
            std::string tempKeywords = keywords;
            size_t pos = 0;

            while ((pos = tempKeywords.find('/')) != std::string::npos) {
                token = tempKeywords.substr(0, pos);
                if (!token.empty()) {
                    keys.push_back(token);
                }
                tempKeywords.erase(0, pos + 1);
            }
            if (!tempKeywords.empty()) {
                keys.push_back(tempKeywords);
            }

            if (keys.empty()) {
                std::cerr << "[JsonProcess] keywords are empty\n";
                return false;
            }

            std::vector<json> results;
            if (!srcJson.is_object()) {
                std::cerr << "[JsonProcess] Root JSON is not an object\n";
            } else {
                auto it = srcJson.find(keys[0]);
                if (it == srcJson.end()) {
                    std::cerr << "[JsonProcess] Top-level key not found: "
                              << keys[0] << "\n";
                } else {
                    collect_by_path(it.value(), keys, 1, results);
                }
            }

            for (const auto& item : results) {
                result["results"].push_back(item);
            }
        } else {
            std::cerr << "[JsonProcess] Unsupported processMethod: "
                      << processMethod << "\n";
            return false;
        }

        outContent = result.dump(4);
        return true;
    }
};

} // namespace core

#if defined(_WIN32)
#  define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#  define PLUGIN_EXPORT extern "C"
#endif

PLUGIN_EXPORT void registerPlugin(core::PluginManager& m) {
    m.registerClass("clsidJsonProcess",
        []() -> std::shared_ptr<core::IObject> {
            return std::make_shared<core::JsonProcess>();
        }
    );

    m.registerFactory<core::IJsonProcess>("default_json_process", [](){
        return std::make_shared<core::JsonProcess>();
    });
}
