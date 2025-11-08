#include "workflow/WorkflowParser.hpp"
#include "workflow/Tasks.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <cstdlib>
#if defined(_WIN32)
#  include <direct.h>
#else
#  include <unistd.h>
#endif

namespace wf {

using json = nlohmann::json;
// struct ArgSpec {
//     std::string value;               // 原始值（可含 {var}）
//     std::vector<std::string> filters; // 过滤器名按顺序应用
// };

WorkflowSpec parseWorkflowJson(const std::string& path) {
    // Load file to string to allow stripping comments
    std::ifstream ifs(path);
    if (!ifs) throw std::runtime_error("Cannot open workflow file: " + path);
    std::string content, line;
    content.reserve(4096);
    // Strip // and /* */ comments while preserving strings
    {
        std::ostringstream oss;
        bool in_string = false; bool in_block_comment = false; bool in_line_comment = false; bool esc = false;
        char prev = '\0';
        std::string filebuf((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        for (size_t i = 0; i < filebuf.size(); ++i) {
            char c = filebuf[i];
            char next = (i + 1 < filebuf.size() ? filebuf[i+1] : '\0');
            if (in_line_comment) {
                if (c == '\n') { in_line_comment = false; oss.put(c); }
                continue;
            }
            if (in_block_comment) {
                if (c == '*' && next == '/') { in_block_comment = false; ++i; }
                continue;
            }
            if (!in_string) {
                if (c == '/' && next == '/') { in_line_comment = true; ++i; continue; }
                if (c == '/' && next == '*') { in_block_comment = true; ++i; continue; }
            }
            if (c == '"' && !esc) { in_string = !in_string; }
            esc = (c == '\\' && !esc);
            if (c != '\\') esc = false; // only the immediate backslash escapes
            oss.put(c);
            prev = c;
        }
        content = oss.str();
    }
    json j = json::parse(content);

    WorkflowSpec spec;
    if (j.contains("final_key")) spec.final_key = j.value("final_key", std::string());
    // Initial variables (optional)
    if (j.contains("vars") && j["vars"].is_object()) {
        for (auto it = j["vars"].begin(); it != j["vars"].end(); ++it) {
            if (it.value().is_string()) spec.vars[it.key()] = it.value().get<std::string>();
        }
    }

    if (!j.contains("tasks") || !j["tasks"].is_array())
        throw std::runtime_error("workflow.json missing tasks array");

    auto expand_vars = [&](std::string s) {
        // Replace occurrences of {key} with spec.vars[key]
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '{') {
                size_t end = s.find('}', i + 1);
                if (end != std::string::npos) {
                    std::string key = s.substr(i + 1, end - i - 1);
                    auto it = spec.vars.find(key);
                    if (it != spec.vars.end()) out += it->second; else out.append(s, i, end - i + 1);
                    i = end;
                    continue;
                }
            }
            out.push_back(s[i]);
        }
        return out;
    };

    auto quote = [](const std::string& s) {
        // naive quoting to handle spaces
        if (s.find_first_of(" \t\"'") == std::string::npos) return s;
        std::string q = "\"";
        for (char c : s) {
            if (c == '"') q += "\\\""; else q += c;
        }
        q += '"';
        return q;
    };

    auto is_abs = [](const std::string& p) -> bool {
        if (p.empty()) return false;
    #if defined(_WIN32)
        if (p.size() >= 2 && std::isalpha(static_cast<unsigned char>(p[0])) && p[1] == ':') return true; // C:\ or C:/
        if (p.size() >= 2 && p[0] == '\\' && p[1] == '\\') return true; // UNC
        return (p[0] == '/' || p[0] == '\\');
    #else
        return p[0] == '/';
    #endif
    };

    auto make_abs = [&](std::string p) -> std::string {
        if (is_abs(p)) return p;
        char buf[4096] = {0};
    #if defined(_WIN32)
        if (_getcwd(buf, sizeof(buf)-1) == nullptr) return p; // fallback
        std::string cwd(buf);
        if (!cwd.empty() && (cwd.back() != '/' && cwd.back() != '\\')) cwd.push_back('/');
        return cwd + p;
    #else
        if (getcwd(buf, sizeof(buf)-1) == nullptr) return p;
        std::string cwd(buf);
        if (!cwd.empty() && cwd.back() != '/') cwd.push_back('/');
        return cwd + p;
    #endif
    };

    for (auto& t : j["tasks"]) {
        std::string id = t.value("id", "");
        std::string type = t.value("type", "");
        if (id.empty() || type.empty())
            throw std::runtime_error("task missing id/type");
        if (type == "Const") {
            auto& p = t["params"];
            // spec.tasks.push_back(std::make_shared<ConstTask>(id, p.value("key", ""), p.value("value", "")));
        } else if (type == "Add") {
            auto& p = t["params"];
            // spec.tasks.push_back(std::make_shared<AddTask>(id, p.value("a", ""), p.value("b", ""), p.value("out", "")));
        } else if (type == "Mul") {
            auto& p = t["params"];
            // spec.tasks.push_back(std::make_shared<MulTask>(id, p.value("a", ""), p.value("b", ""), p.value("out", "")));
        } else if (type == "Shell") {
            // Support both legacy { params: { cmd, out_key, out_value, check_exists } }
            // and new schema with script_path + args + out fields on task root.
            if (t.contains("script_path")) {
                std::string script = expand_vars(t.value("script_path", std::string()));
                // Build arguments
                std::vector<ArgSpec> args;
                if (t.contains("args") && t["args"].is_array()) {
                    for (auto& a : t["args"]) {
                        ArgSpec arg;
                        arg.value = a.value("value", std::string());
                        if (a.contains("filters") && a["filters"].is_array()) {
                            for (auto& f : a["filters"]) {
                                if (!f.is_string()) continue;
                                std::string fname = f.get<std::string>();
                                arg.filters.push_back(fname);
                            }
                        }
                        args.push_back(arg);
                    }
                }
                // Expand out fields
                const std::string outKey = t.value("out_key", std::string());
                std::string outValue = expand_vars(t.value("out_value", std::string()));
                const bool check = t.value("check_exists", false);

                // Assemble command string
                std::string cmd = quote(script);
                spec.tasks.push_back(std::make_shared<ShellTask>(id, cmd, args,outKey, outValue, check));
            } else {
                // Legacy schema
                auto& p = t["params"];
                const std::string cmd = p.value("cmd", "");
                const std::string outKey = p.value("out_key", "");
                const std::string outValue = p.value("out_value", "");
                const bool check = p.value("check_exists", false);
                std::vector<ArgSpec> args;
                spec.tasks.push_back(std::make_shared<ShellTask>(id, cmd, args, outKey, outValue, check));
            }
        } else {
            throw std::runtime_error(std::string("unsupported task type: ") + type);
        }
    }

    if (!j.contains("edges") || !j["edges"].is_array())
        throw std::runtime_error("workflow.json missing edges array");
    for (auto& e : j["edges"]) {
        if (!e.is_array() || e.size() != 2)
            throw std::runtime_error("edge must be [from,to]");
        spec.edges.push_back({ e[0].get<std::string>(), e[1].get<std::string>() });
    }
    return spec;
}

} // namespace wf
