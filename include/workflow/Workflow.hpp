#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include "ITask.hpp"

namespace wf {

struct Edge { std::string from; std::string to; };

struct WorkflowSpec {
    std::vector<std::shared_ptr<ITask>> tasks;
    std::vector<Edge> edges;
    std::string final_key; // optional: key to print after run
};

} // namespace wf
