// plugins/json_compare/JsonCompare.cpp
#include "core/IComparator.hpp"
#include "core/PluginManager.hpp"    // 现阶段沿用你的框架，仍用 registerPlugin 入口
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>
#include <memory>

using json = nlohmann::ordered_json;  // ★ 保持对象键的插入顺序

namespace {

// 工具函数
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

// 递归比较并以“左为主”输出报告（对象按插入顺序遍历）
static void dump_compare(const json& L, const json& R, std::ostream& os, int pad, const std::string& key="") {
    const bool hasKey = !key.empty();

    // 类型不一致：整块标 E
    if (L.type() != R.type()){
        if (L.is_object()){
            os << indent(pad) << (hasKey? ("\""+key+"\": ") : "") << "{ // TYPE_MISMATCH golden: "
               << R.type_name() << " " << sym(Status::ERR) << "\n";

            // 先按 L 的插入顺序打印（右侧缺失则标记）
            for (auto it = L.begin(); it != L.end(); ++it){
                const std::string& k = it.key();
                const json* r = R.contains(k) ? &R.at(k) : nullptr;
                dump_compare(it.value(), r? *r : json(), os, pad+2, k);
            }
            // 再补 R 独有键（按 R 的插入顺序）
            for (auto it = R.begin(); it != R.end(); ++it){
                const std::string& k = it.key();
                if (!L.contains(k)){
                    os << indent(pad+2) << "\"" << k << "\": <missing> // golden: "
                       << it.value().dump() << " " << sym(Status::ERR) << "\n";
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

        // size_t n = std::max(L.size(), R.size());
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

        // 先按 L 的插入顺序
        for (auto it = L.begin(); it != L.end(); ++it){
            const std::string& k = it.key();
            const json* r = R.contains(k) ? &R.at(k) : nullptr;
            if (r) dump_compare(it.value(), *r, os, pad+2, k);
            else{
                os << indent(pad+2) << "\"" << k << "\": ";
                print_value(os, it.value());
                os << " // golden: <missing> " << sym(Status::ERR) << "\n";
            }
        }
        // 再补 R 独有键（按 R 的插入顺序）
        for (auto it = R.begin(); it != R.end(); ++it){
            const std::string& k = it.key();
            if (!L.contains(k)){
                os << indent(pad+2) << "\"" << k << "\": <missing> // golden: "
                   << it.value().dump() << " " << sym(Status::ERR) << "\n";
            }
        }

        os << indent(pad) << "}\n";
        return;
    }
}

static std::string htmlEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default:  out.push_back(ch); break;
        }
    }
    return out;
}

static std::string makeHtmlReport(const std::string& plain,
                                  const std::string& oursPath,
                                  const std::string& goldenPath) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n"
         << "<meta charset=\"utf-8\" />\n"
         << "<title>JSON Compare Report</title>\n"
         << "<style>\n"
         << "body{font-family:Consolas,Menlo,monospace;background:#f5f5f7;margin:0;padding:24px;}\n"
         << "h1{font-size:20px;margin-bottom:8px;}\n"
         << ".meta{margin-bottom:16px;color:#333;font-size:14px;}\n"
         << ".legend span{display:inline-block;margin-right:16px;}\n"
         << ".legend .ok{color:#118011;}\n"
         << ".legend .diff{color:#b84a00;}\n"
         << ".legend .err{color:#c70024;}\n"
         << "pre{background:#fff;border:1px solid #d0d0d5;border-radius:6px;padding:16px;"
         << "overflow:auto;line-height:1.4;font-size:13px;white-space:pre-wrap;}\n"
         << "</style>\n</head>\n<body>\n";
    html << "<h1>JSON 对比报告</h1>\n";
    html << "<div class=\"meta\"><div><strong>OURS:</strong> " << oursPath << "</div>\n";
    html << "<div><strong>GOLDEN:</strong> " << goldenPath << "</div></div>\n";
    html << "<div class=\"legend\"><span class=\"ok\">√ 相同</span>"
         << "<span class=\"diff\">× 值不同</span>"
         << "<span class=\"err\">E 结构/缺失</span></div>\n";
    html << "<pre>" << htmlEscape(plain) << "</pre>\n";
    html << "</body>\n</html>\n";
    return html.str();
}

} // anonymous namespace

namespace core {

// 插件实现：把对比能力挂到 IComparator
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

		std::ostringstream plain;
		plain << "对照报告（左=ours，右=golden；对象按插入顺序，叶子尾�?�?×/E）\n";
		plain << "说明：√=相同，�?同类型值不同，E=结构/类型/缺失错误\n\n";
		dump_compare(L, R, plain, 0);

		ofs << makeHtmlReport(plain.str(), oursPath, goldenPath);

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

    // Also register typed factory for IComparator under a stable name
    m.registerFactory<core::IComparator>("default_json_compare", [](){
        return std::make_shared<core::JsonComparator>();
    });
}
