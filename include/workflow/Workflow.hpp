#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include "workflow/ITask.hpp"

namespace wf {

class ITask; // forward decl for shared_ptr reference

struct Edge { std::string from; std::string to; };

struct WorkflowSpec {
    std::vector<std::shared_ptr<ITask>> tasks;
    std::vector<Edge> edges;
    std::string final_key; // optional: key to print after run
    // Optional: initial variables to seed into ITaskContext before run
    std::unordered_map<std::string, std::string> vars;
};

} // namespace wf
