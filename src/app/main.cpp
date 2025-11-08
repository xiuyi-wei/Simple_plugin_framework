#include <iostream>
#include <memory>
#include "core/PluginManager.hpp"
#include "core/ISimple.hpp"
#include "core/IComparator.hpp"
#include "config/ConfigParser.hpp"
#if defined(_WIN32)
#  include <windows.h>
#endif
#include "ui/console_ui.hpp"
#include "workflow/Workflow.hpp"
#include "workflow/Executor.hpp"
#include "workflow/WorkflowParser.hpp"
#include "workflow/ITask.hpp"
#include "runtime/EventBus.hpp"
#include "runtime/ThreadPool.hpp"
#include "runtime/Services.hpp"
#include "workflow/Workflow.hpp"
#include "workflow/Executor.hpp"
#include "runtime/EventBus.hpp"
#include "runtime/ThreadPool.hpp"
#include "runtime/Services.hpp"
#include <thread>
#include <chrono>
#include <fstream>
#include "nlohmann/json.hpp"

using namespace core;

// Print parsed workflow.json information (vars, tasks, edges, final_key)
int main() 
{
    printf("Demo Workflow Parser\n");
    try {
        rt::LocalFS fs; rt::StdLogger logger; rt::SteadyClock clock;
        if (!fs.exists("workflow.json")) {
            std::cout << "workflow.json not found in working directory." << std::endl;
            return 0;
        }

        // Parse using workflow parser (builds WorkflowSpec)
        auto spec = wf::parseWorkflowJson("workflow.json");

        // Print summary
        std::cout << "=== Workflow Summary ===" << std::endl;
        if (!spec.final_key.empty())
            std::cout << "final_key: " << spec.final_key << std::endl;
        else
            std::cout << "final_key: (none)" << std::endl;

        // Print vars
        std::cout << "vars (" << spec.vars.size() << "):" << std::endl;
        for (const auto& kv : spec.vars) {
            std::cout << "  " << kv.first << " = " << kv.second << std::endl;
        }

        // Print tasks (IDs only; detailed fields come from JSON below)
        std::cout << "tasks (" << spec.tasks.size() << "):" << std::endl;
        for (const auto& t : spec.tasks) {
            std::cout << "  id=" << t->id() << std::endl;
        }

        // Print edges
        std::cout << "edges (" << spec.edges.size() << "):" << std::endl;
        for (const auto& e : spec.edges) {
            std::cout << "  [" << e.from << ", " << e.to << "]" << std::endl;
        }
         std::cout << "\n=== Executing Workflow ===" << std::endl;
        try {
            rt::LocalFS fs; rt::StdLogger logger; rt::SteadyClock clock;
            if (!fs.exists("workflow.json")) {
                std::cout << "workflow.json not found in working directory." << std::endl;
                return 0;
            }
            // === 执行 ===
            wf::SimpleContext ctx(logger, clock, fs);
            rt::EventBus eventBus;
            rt::ThreadPool threadPool(std::thread::hardware_concurrency());
            wf::Executor exec(eventBus, threadPool);
            auto r = exec.run(spec, ctx);
  
            if (!spec.final_key.empty()) {
                std::cout << "Final(" << spec.final_key << ") = " << r << std::endl;
            } else {
                std::cout << "Workflow succeeded." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "workflow.json error: " << e.what() << std::endl;
        }


    } catch (const std::exception& e) {
        std::cerr << "workflow.json error: " << e.what() << std::endl;
    }
        // // For richer task details, read the raw JSON to show type/script/args
        // try {
        //     std::ifstream ifs("workflow.json");
        //     nlohmann::json j; ifs >> j;
        //     if (j.contains("tasks") && j["tasks"].is_array()) {
        //         std::cout << "\n=== Task Details ===" << std::endl;
        //         for (const auto& t : j["tasks"]) {
        //             std::string id = t.value("id", "");
        //             std::string type = t.value("type", "");
        //             std::cout << "- id=" << id << ", type=" << type << std::endl;
        //             if (type == "Shell") {
        //                 if (t.contains("script_path")) {
        //                     std::cout << "  script_path: " << t.value("script_path", "") << std::endl;
        //                     if (t.contains("args") && t["args"].is_array()) {
        //                         int idx = 0;
        //                         for (const auto& a : t["args"]) {
        //                             std::cout << "  arg[" << idx++ << "]: value='" << a.value("value","") << "'";
        //                             if (a.contains("filters") && a["filters"].is_array()) {
        //                                 std::cout << ", filters=[";
        //                                 bool first = true;
        //                                 for (const auto& f : a["filters"]) {
        //                                     if (!first) std::cout << ", ";
        //                                     first = false;
        //                                     std::cout << f.get<std::string>();
        //                                 }
        //                                 std::cout << "]";
        //                             }
        //                             std::cout << std::endl;
        //                         }
        //                     }
        //                     if (t.contains("out_key")) std::cout << "  out_key: " << t.value("out_key","") << std::endl;
        //                     if (t.contains("out_value")) std::cout << "  out_value: " << t.value("out_value","") << std::endl;
        //                     if (t.contains("check_exists")) std::cout << "  check_exists: " << (t.value("check_exists", false)?"true":"false") << std::endl;
        //                 }
        //             }
        //         }
        //     }
        // } catch (...) {
        //     // If printing JSON details fails, continue silently
        // }
   
        
    return 0;
}
