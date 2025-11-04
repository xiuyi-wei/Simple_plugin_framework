#include "workflow/WorkflowParser.hpp"
#include "workflow/Tasks.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <stdexcept>

namespace wf {

using json = nlohmann::json;

WorkflowSpec parseWorkflowJson(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) throw std::runtime_error("Cannot open workflow file: " + path);
    json j; ifs >> j;

    WorkflowSpec spec;
    if (j.contains("final_key")) spec.final_key = j.value("final_key", std::string());

    if (!j.contains("tasks") || !j["tasks"].is_array())
        throw std::runtime_error("workflow.json missing tasks array");

    for (auto& t : j["tasks"]) {
        std::string id = t.value("id", "");
        std::string type = t.value("type", "");
        if (id.empty() || type.empty())
            throw std::runtime_error("task missing id/type");
        auto& p = t["params"];
        if (type == "Const") {
            spec.tasks.push_back(std::make_shared<ConstTask>(id, p.value("key", ""), p.value("value", "")));
        } else if (type == "Add") {
            spec.tasks.push_back(std::make_shared<AddTask>(id, p.value("a", ""), p.value("b", ""), p.value("out", "")));
        } else if (type == "Mul") {
            spec.tasks.push_back(std::make_shared<MulTask>(id, p.value("a", ""), p.value("b", ""), p.value("out", "")));
        } else if (type == "Shell") {
            const std::string cmd = p.value("cmd", "");
            const std::string outKey = p.value("out_key", "");
            const std::string outValue = p.value("out_value", "");
            const bool check = p.value("check_exists", false);
            spec.tasks.push_back(std::make_shared<ShellTask>(id, cmd, outKey, outValue, check));
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
