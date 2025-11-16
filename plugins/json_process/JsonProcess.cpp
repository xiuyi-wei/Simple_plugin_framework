// plugins/json_compare/JsonCompare.cpp
#include "core/IJsonProcess.hpp"
#include "core/PluginManager.hpp"    // 现阶段沿用你的框架，仍用 registerPlugin 入口
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>
#include <memory>

using json = nlohmann::ordered_json;  // ★ 保持对象键的插入顺序
namespace{
        void find_keyValue_json(const json& j,
                                const std::vector<std::string>& keys,
                                size_t idx,
                                std::vector<json>& results)
        {
            if (idx >= keys.size()) return;

            if (j.is_object()) {
                for (auto& it : j.items()) {
                    if (it.key() == keys[idx]) {
                        if (idx == keys.size() - 1) {
                            results.push_back(it.value());
                        } else {
                            find_keyValue_json(it.value(), keys, idx + 1, results);
                        }
                    } else {
                        find_keyValue_json(it.value(), keys, idx, results);
                    }
                }
            }
            else if (j.is_array()) {
                for (const auto& el : j) {
                    find_keyValue_json(el, keys, idx, results);
                }
            }
        }
};

namespace core {

// 插件实现：把对比能力挂到 IJsonProcess
class JsonProcess : public IJsonProcess {
public:
    bool processJsonFiles(const std::string& srcPath,
                          const std::string& desPath,
                          const std::string& keywords,
                          const std::string& processMethod) override {
#if defined(_WIN32)                    
        Wow64DisableWow64FsRedirection(NULL);
#endif
        std::cout << "[WD] " << std::filesystem::current_path() << "\n";
         auto srcFixed = std::filesystem::absolute(srcPath);
        auto desFixed = std::filesystem::absolute(desPath);

        std::cerr << "[PLUGIN] fixed src=" << srcFixed.string() << "\n";
        std::cerr << "[PLUGIN] fixed des=" << desFixed.string() << "\n";

        std::ifstream fs(srcFixed);

        // 参数检查
        std::cout << "[Debug] Try load JSON from: " << srcPath << std::endl;
        std::ifstream test(srcPath);
        std::cout << "[Debug] File good()? " << test.good() << std::endl;
        std::cout << "[Debug] File size: " << test.rdbuf()->in_avail() << std::endl;

        json src_path_json, des_path_json;
        try{
            std::ifstream fs(srcFixed), fr(desFixed);
            if(!fs || !fr){
                std::cerr << "[JsonProcess] openfile failed: "
                          << srcFixed << " / " << desFixed << "\n";
                return false;
            }
            fs >> src_path_json; fr >> des_path_json;
        }catch(const std::exception& e){
            std::cerr << "[JsonProcess] parse failed: " << e.what() << "\n";
            return false;
        }

        std::ofstream ofs(desFixed);
        if(!ofs){
            std::cerr << "[JsonProcess] write failed: " << desFixed << "\n";
            return false;
        }

        // 简单示例：根据 processMethod 进行不同处理
        if(processMethod == "find"){
            size_t pos = 0;
            std::vector<std::string> keys;
            std::string token;
            std::string tempKeywords = keywords;
            // 这里怎么抽象出json的结果的值呢，因为可能是多种类型
            std::vector<json> results;
            while ((pos = tempKeywords.find('/')) != std::string::npos) {
                token = tempKeywords.substr(0, pos);
                keys.push_back(token);
                tempKeywords = tempKeywords.substr( pos + 1);
            }
            keys.push_back(tempKeywords); // Add the last segment after the final '/'
            find_keyValue_json(src_path_json,
                                keys,
                                0,
                                results);
        }
        return true;
    }
};

} // namespace core

// —— 导出注册函数（保持与你的 PluginManager::loadPlugin 查找的符号一致）——
#if defined(_WIN32)
  #define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
  #define PLUGIN_EXPORT extern "C"
#endif

PLUGIN_EXPORT void registerPlugin(core::PluginManager& m) {
    m.registerClass("clsidJsonProcess",
        []() -> std::shared_ptr<core::IObject> {
            return std::make_shared<core::JsonProcess>();
        }
    );

    // Also register typed factory for IJsonProcess under a stable name
    m.registerFactory<core::IJsonProcess>("default_json_process", [](){
        return std::make_shared<core::JsonProcess>();
    });
}
