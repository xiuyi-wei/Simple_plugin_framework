#pragma once
#include <string>
#include "workflow/Workflow.hpp"

namespace wf {

// Parse workflow spec from a JSON file path.
// Supported task types: "Const", "Add", "Mul".
WorkflowSpec parseWorkflowJson(const std::string& path);

} // namespace wf

