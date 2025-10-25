// plugins/json_compare/JsonCompare.cpp
#include "core/IComparator.hpp"
#include "core/PluginManager.hpp"    // 现阶段沿用你的框架，仍用 registerPlugin 入口
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <set>
#include <algorithm>
#include <sstream>
#include <memory>

using json = nlohmann::json;

namespace {

// 工具函数（与原程序一致）
static std::string indent(int n){ return std::string(n, ' '); }

static void print_value(std::ostream& os, const json& v){
    os << v.dump(); // 让 nlohmann 负责转义/格式
}

enum class Status { OK, DIFF, ERR };

static Status leaf_status(const json* L, const json* R){
    if (!L || !R) return Status::ERR;
    if (L->type() != R->type()) return Status::ERR;
    return (*L == *R) ? Status::OK : Status::DIFF;
}

static const char* sym(Status s){
    switch(s){
        case Status::OK:  return u8"√";
        case Status::DIFF:return u8"×";
        default:          return u8"E";
    }
}

// 递归比较并以“左为主”输出报告
static void dump_compare(const json& L, const json& R, std::ostream& os, int pad, const std::string& key="") {
    const bool hasKey = !key.empty();

    // 类型不一致：整块标 E
    if (L.type() != R.type()){
        if (L.is_object()){
            os << indent(pad) << (hasKey? ("\""+key+"\": ") : "") << "{ // TYPE_MISMATCH golden: "
               << R.type_name() << " " << sym(Status::ERR) << "\n";

            std::set<std::string> keysL, keysR;
            for (auto& [k,_] : L.items()) keysL.insert(k);
            for (auto& [k,_] : R.items()) keysR.insert(k);

            for (auto& k : keysL){
                const json* r = R.contains(k) ? &R.at(k) : nullptr;
                dump_compare(L.at(k), r? *r : json(), os, pad+2, k);
            }
            for (auto& k : keysR){
                if (!L.contains(k)){
                    os << indent(pad+2) << "\"" << k << "\": <missing> // golden: "
                       << R.at(k).dump() << " " << sym(Status::ERR) << "\n";
                }
            }
            os << indent(pad) << "}\n";
            return;
        }
        if (L.is_array()){
            os << indent(pad) << (hasKey? ("\""+key+"\": ") : "") << "[ // TYPE_MISMATCH golden: "
               << R.type_name() << " " << sym(Status::ERR) << "\n";
            for (size_t i=0;i<L.size();++i){
                const json* r = R.is_array() && i<R.size()? &R[i] : nullptr;
                dump_compare(L[i], r? *r : json(), os, pad+2);
            }
            os << indent(pad) << "]\n";
            return;
        }
        // 我方是标量，右边是对象/数组
        os << indent(pad);
        if (hasKey) os << "\"" << key << "\": ";
        print_value(os, L);
        os << " // golden: " << R.type_name() << " " << sym(Status::ERR) << "\n";
        return;
    }

    // 同类型
    if (!L.is_object() && !L.is_array()){
        // 叶子：直接对比
        Status st = leaf_status(&L, &R);
        os << indent(pad);
        if (hasKey) os << "\"" << key << "\": ";
        print_value(os, L);
        os << " // golden: " << R.dump() << " " << sym(st) << "\n";
        return;
    }

    if (L.is_array()){
        Status st = (L.size()==R.size()? Status::OK : Status::ERR);
        os << indent(pad);
        if (hasKey) os << "\"" << key << "\": ";
        os << "[";
        if (st==Status::ERR) os << " // LEN " << L.size() << " vs " << R.size() << " " << sym(st);
        os << "\n";

        size_t n = L.size() > R.size() ? L.size() : R.size();

        for (size_t i=0;i<n;++i){
            const json* l = (i<L.size()? &L[i] : nullptr);
            const json* r = (i<R.size()? &R[i] : nullptr);
            if (l && r){
                dump_compare(*l, *r, os, pad+2);
            }else if (l && !r){
                os << indent(pad+2);
                print_value(os, *l);
                os << " // golden: <missing> " << sym(Status::ERR) << "\n";
            }else{
                os << indent(pad+2) << "<missing> // golden: " << r->dump() << " " << sym(Status::ERR) << "\n";
            }
        }
        os << indent(pad) << "]\n";
        return;
    }

    if (L.is_object()){
        os << indent(pad);
        if (hasKey) os << "\"" << key << "\": ";
        os << "{\n";

        std::set<std::string> keysL, keysR;
        for (auto& [k,_] : L.items()) keysL.insert(k);
        for (auto& [k,_] : R.items()) keysR.insert(k);

        for (auto& k : keysL){
            const json* r = R.contains(k) ? &R.at(k) : nullptr;
            if (r) dump_compare(L.at(k), *r, os, pad+2, k);
            else{
                os << indent(pad+2) << "\"" << k << "\": ";
                print_value(os, L.at(k));
                os << " // golden: <missing> " << sym(Status::ERR) << "\n";
            }
        }
        for (auto& k : keysR){
            if (!L.contains(k)){
                os << indent(pad+2) << "\"" << k << "\": <missing> // golden: "
                   << R.at(k).dump() << " " << sym(Status::ERR) << "\n";
            }
        }

        os << indent(pad) << "}\n";
        return;
    }
}

} // anonymous namespace

namespace core {

// 插件实现：把原 main() 中的逻辑包进接口方法
class JsonComparator : public IComparator {
public:
    bool compareFiles(const std::string& oursPath,
                      const std::string& goldenPath,
                      const std::string& outPath) override {
        json L, R;
        try{
            std::ifstream fl(oursPath), fr(goldenPath);
            if(!fl || !fr){
                std::cerr << "[JsonComparator] 打开文件失败: "
                          << oursPath << " / " << goldenPath << "\n";
                return false;
            }
            fl >> L; fr >> R;
        }catch(const std::exception& e){
            std::cerr << "[JsonComparator] 解析失败: " << e.what() << "\n";
            return false;
        }

        std::ofstream ofs(outPath);
        if(!ofs){
            std::cerr << "[JsonComparator] 无法写入: " << outPath << "\n";
            return false;
        }

        ofs << "对照报告（左=ours，右=golden；叶子行尾显示 √/×/E）\n";
        ofs << "说明：√=相同，×=同类型但值不同，E=结构/类型/缺失错误\n\n";
        dump_compare(L, R, ofs, 0);

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
    m.registerClass("clsidJsonComparator",
        []() -> std::shared_ptr<core::IObject> {
            return std::make_shared<core::JsonComparator>();
        }
    );
}
