#pragma once
#include <string>
#include "workflow/Workflow.hpp"

namespace wf {

// Parse workflow spec from a JSON file path.
// Supported task types: "Const", "Add", "Mul", "Shell".
// Accepts legacy format (Shell under params with cmd) and new format:
// {
//   "final_key": "...",
//   "vars": { "base_dir": "./workspace" },
//   "tasks": [
//     { "id":"create", "type":"Shell",
//       "script_path":"./scripts/create.sh",
//       "args":[ {"value":"{base_dir}", "filters":["abspath"]} ],
//       "out_key":"created_flag", "out_value":"{base_dir}/flag.txt", "check_exists":true }
//   ],
//   "edges": [["create","next"]]
// }
WorkflowSpec parseWorkflowJson(const std::string& path);

} // namespace wf
