#pragma once
#include <string>
#include <memory>
#include "workflow/ITask.hpp"
#include "workflow/ITaskContext.hpp"
#include "runtime/Services.hpp"
#include <cstdlib>

namespace wf {
// ShellTask 接口保持“通用”，只知道 script_path + args[]
struct ArgSpec {
    std::string value;               // 原始值（可含 {var}）
    std::vector<std::string> filters; // 过滤器名按顺序应用
};

class ShellTask : public ITask {
public:
    ShellTask(std::string id,
              std::string script_path,
              std::vector<ArgSpec> args,
              std::string outKey = {},
              std::string outValue = {},
              bool checkExists = false)
    : id_(std::move(id)),
      script_path_(std::move(script_path)),
      args_(std::move(args)),
      outKey_(std::move(outKey)),
      outValue_(std::move(outValue)),
      checkExists_(checkExists) {}

    std::string id() const override { return id_; }

    TaskResult run(ITaskContext& ctx) override {
        // 1) 校验脚本存在 & 可执行
        if (!ctx.fs().exists(script_path_)) {
            return {false, "script not found: " + script_path_};
        }
        // if (!ctx.fs().executable(script_path_)) {
        //     return {false, "script not executable: " + script_path_};
        // }

        // 2) 解析并过滤参数
        std::vector<std::string> resolved;
        resolved.reserve(args_.size());
        for (auto& a : args_) {
            std::string s = substituteVars(a.value, ctx);     // {var} -> 实值
            s = applyFilters(s, a.filters, ctx);               // 依次过滤
            resolved.push_back(s);
        }

        // 3) 组装命令行（安全起见给每个参数包一层 quote）
        std::string cmd = quote(script_path_);
        for (auto& s : resolved) cmd += " " + quote(s);

        // POSIX：bash -lc 执行；Windows 可切换到 "cmd /C"
        std::string shellCmd = "bash -lc " + quote(cmd);
        int rc = std::system(shellCmd.c_str());
        if (rc != 0) return {false, "shell exit != 0"};

        // 4) 输出校验与写回上下文
        if (!outKey_.empty()) {
            std::string ov = substituteVars(outValue_, ctx);
            ov = applyFilters(ov, {/*可选：如 normpath */}, ctx);
            if (checkExists_ && !ctx.fs().exists(ov)) {
                return {false, "expected output not found: " + ov};
            }
            ctx.set(outKey_, ov);
        }
        return {true, {}};
    }

private:
    static std::string quote(const std::string& s) {
        // 简单 POSIX 引号；Win 下可做另一套
        std::string t = s;
        // 单引号转义：' -> '\''  （这里给个保守实现）
        size_t pos = 0;
        while ((pos = t.find('\'', pos)) != std::string::npos) {
            t.replace(pos, 1, "'\"'\"'");
            pos += 5;
        }
        return "'" + t + "'";
    }

   static std::string applyFilters(std::string v,
                                const std::vector<std::string>& filters,
                                ITaskContext& ctx)
    {
        for (const auto& raw : filters) {
            // 拆解过滤器名与参数，如 "join:subdir" -> name=join, arg=subdir
            auto pos = raw.find(':');
            std::string name = (pos == std::string::npos) ? raw : raw.substr(0, pos);
            std::string arg  = (pos == std::string::npos) ? ""  : raw.substr(pos + 1);

            // 去掉多余空格
            auto trim = [](std::string& s) {
                while (!s.empty() && std::isspace(s.front())) s.erase(s.begin());
                while (!s.empty() && std::isspace(s.back())) s.pop_back();
            };
            trim(name); trim(arg);

            if (name == "abspath") {
                v = ctx.fs().absolute(v);
            } else if (name == "normpath") {
                v = ctx.fs().normalize(v);
            } else if (name == "ensure_dir") {
                ctx.fs().ensureDir(v);   // 若无则创建（你可在 fs() 内部实现）
            } else if (name == "join") {
                if (!arg.empty()) v = ctx.fs().join(v, arg);
            } else if (name == "dirname") {
                v = ctx.fs().dirname(v);
            } else if (name == "basename") {
                v = ctx.fs().basename(v);
            } else if (name == "default") {
                if (v.empty()) v = arg;
            } else if (name == "quote") {
                // 如果过滤链中就要立即quote
                if (!v.empty()) {
                    std::string q = "'";
                    for (char c : v) {
                        if (c == '\'') q += "'\"'\"'";
                        else q.push_back(c);
                    }
                    q += "'";
                    v = q;
                }
            } else if (name == "lower") {
                // std::transform(v.begin(), v.end(), v.begin(),
                //             [](unsigned char c){ return std::tolower(c); });
            } else if (name == "upper") {
                // std::transform(v.begin(), v.end(), v.begin(),
                //             [](unsigned char c){ return std::toupper(c); });
            } else if (name == "env") {
                // 支持 env:PATH_NAME 或 env:VAR=default
                std::string var = arg, def;
                auto eq = arg.find('=');
                if (eq != std::string::npos) {
                    var = arg.substr(0, eq);
                    def = arg.substr(eq + 1);
                }
                const char* val = std::getenv(var.c_str());
                v = val ? std::string(val) : def;
            } else {
              //  std::cerr << "[WARN] Unknown filter: " << name << std::endl;
            }
        }

        return v;
    }


    static std::string substituteVars(const std::string& in, ITaskContext& ctx) {
        // 极简实现：替换 {var} 为 ctx.get(var)
        std::string out; out.reserve(in.size());
        for (size_t i=0; i<in.size(); ) {
            if (in[i]=='{' ) {
                size_t j = in.find('}', i+1);
                if (j != std::string::npos) {
                    std::string key = in.substr(i+1, j-i-1);
                    out += ctx.get(key); // 不存在则返回空串或原样
                    i = j+1;
                } else {
                    out += in[i++];
                }
            } else {
                out += in[i++];
            }
        }
        return out;
    }

private:
    std::string id_;
    std::string script_path_;
    std::vector<ArgSpec> args_;
    std::string outKey_, outValue_;
    bool checkExists_;
};

} // namespace wf
