// plugins/json_compare/JsonCompare.cpp
#include "core/IJsonProcess.hpp"
#include "core/PluginManager.hpp"
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>
#include <algorithm>
#include <memory>
#include <vector>
#include <cctype>

using json = nlohmann::ordered_json;  // 保持对象键的插入顺序

namespace {

std::string trim(const std::string& input) {
    auto begin = std::find_if_not(input.begin(), input.end(),
                                  [](unsigned char c){ return std::isspace(c); });
    auto end = std::find_if_not(input.rbegin(), input.rend(),
                                [](unsigned char c){ return std::isspace(c); }).base();
    if (begin >= end) return "";
    return std::string(begin, end);
}

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : path) {
        if (ch == '/') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

std::string joinPath(const std::vector<std::string>& parts) {
    std::string joined;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) joined.push_back('/');
        joined += parts[i];
    }
    return joined;
}

bool parseAddKeywords(const std::string& keywords,
                      std::vector<std::string>& prefix,
                      std::vector<std::string>& target) {
    const std::vector<std::string> separators = {"->", "|", ","};
    size_t pos = std::string::npos;
    std::string sep;
    for (const auto& candidate : separators) {
        pos = keywords.find(candidate);
        if (pos != std::string::npos) {
            sep = candidate;
            break;
        }
    }
    if (pos == std::string::npos) return false;

    auto prefixStr = trim(keywords.substr(0, pos));
    auto targetStr = trim(keywords.substr(pos + sep.size()));
    prefix = splitPath(prefixStr);
    target = splitPath(targetStr);
    return !prefix.empty() && !target.empty();
}

bool ensurePathExists(const json& node,
                      const std::vector<std::string>& path,
                      size_t idx) {
    if (idx >= path.size()) return true;
    if (!node.is_object()) return false;
    auto it = node.find(path[idx]);
    if (it == node.end()) return false;
    return ensurePathExists(*it, path, idx + 1);
}

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
            const auto keys = splitPath(keywords);

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
        } else if (processMethod == "add") {
            std::cout << "[JsonProcess] processMethod: add\n";
            std::vector<std::string> prefix;
            std::vector<std::string> target;
            if (!parseAddKeywords(keywords, prefix, target)) {
                std::cerr << "[JsonProcess] Invalid add keywords, use `prefix|target`\n";
                return false;
            }

            if (!srcJson.is_object()) {
                std::cerr << "[JsonProcess] Root JSON must be object for add\n";
                return false;
            }

            const auto& firstKey = target.front();
            auto it = srcJson.find(firstKey);
            if (it == srcJson.end()) {
                std::cerr << "[JsonProcess] Target key not found: " << firstKey << "\n";
                return false;
            }

            if (!ensurePathExists(*it, target, 1)) {
                std::cerr << "[JsonProcess] Target path not found under key: "
                          << firstKey << "\n";
                return false;
            }

            json movedBranch = *it;
            srcJson.erase(it);

            json* current = &srcJson;
            for (const auto& key : prefix) {
                if (current->is_null()) *current = json::object();
                if (!current->is_object()) {
                    std::cerr << "[JsonProcess] Cannot create prefix under non-object: "
                              << key << "\n";
                    return false;
                }
                current = &((*current)[key]);
            }
            if (current->is_null()) *current = json::object();
            if (!current->is_object()) {
                std::cerr << "[JsonProcess] Prefix terminates at non-object node\n";
                return false;
            }

            (*current)[firstKey] = movedBranch;

            std::vector<std::string> combined = prefix;
            combined.insert(combined.end(), target.begin(), target.end());
            result["results"] = json::array();
            result["newPath"] = joinPath(combined);
            result["updatedJson"] = srcJson;
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
