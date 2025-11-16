#include <iostream>
#include <memory>
#include "core/PluginManager.hpp"
#include "core/ISimple.hpp"
#include "core/IComparator.hpp"
#include "core/IJsonProcess.hpp"
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
#include <sstream>
#include "nlohmann/json.hpp"
#include <filesystem>
using namespace core;

// Print parsed workflow.json information (vars, tasks, edges, final_key)
int main() 
{
    // printf("Demo Workflow Parser\n");
    // try {
    //     rt::LocalFS fs; rt::StdLogger logger; rt::SteadyClock clock;
    //     if (!fs.exists("workflow.json")) {
    //         std::cout << "workflow.json not found in working directory." << std::endl;
    //         return 0;
    //     }

    //     // Parse using workflow parser (builds WorkflowSpec)
    //     auto spec = wf::parseWorkflowJson("workflow.json");

    //     // Print summary
    //     std::cout << "=== Workflow Summary ===" << std::endl;
    //     if (!spec.final_key.empty())
    //         std::cout << "final_key: " << spec.final_key << std::endl;
    //     else
    //         std::cout << "final_key: (none)" << std::endl;

    //     // Print vars
    //     std::cout << "vars (" << spec.vars.size() << "):" << std::endl;
    //     for (const auto& kv : spec.vars) {
    //         std::cout << "  " << kv.first << " = " << kv.second << std::endl;
    //     }

    //     // Print tasks (IDs only; detailed fields come from JSON below)
    //     std::cout << "tasks (" << spec.tasks.size() << "):" << std::endl;
    //     for (const auto& t : spec.tasks) {
    //         std::cout << "  id=" << t->id() << std::endl;
    //     }

    //     // Print edges
    //     std::cout << "edges (" << spec.edges.size() << "):" << std::endl;
    //     for (const auto& e : spec.edges) {
    //         std::cout << "  [" << e.from << ", " << e.to << "]" << std::endl;
    //     }
    //      std::cout << "\n=== Executing Workflow ===" << std::endl;
    //     try {
    //         rt::LocalFS fs; rt::StdLogger logger; rt::SteadyClock clock;
    //         if (!fs.exists("workflow.json")) {
    //             std::cout << "workflow.json not found in working directory." << std::endl;
    //             return 0;
    //         }
    //         // === 执行 ===
    //         wf::SimpleContext ctx(logger, clock, fs);
    //         rt::EventBus eventBus;
    //         rt::ThreadPool threadPool(std::thread::hardware_concurrency());
    //         wf::Executor exec(eventBus, threadPool);
    //         auto r = exec.run(spec, ctx);
  
    //         if (!spec.final_key.empty()) {
    //             std::cout << "Final(" << spec.final_key << ") = " << r << std::endl;
    //         } else {
    //             std::cout << "Workflow succeeded." << std::endl;
    //         }
    //     } catch (const std::exception& e) {
    //         std::cerr << "workflow.json error: " << e.what() << std::endl;
    //     }


    // } catch (const std::exception& e) {
    //     std::cerr << "workflow.json error: " << e.what() << std::endl;
    // }
    // std::cout << "Working dir = " << std::filesystem::current_path() << "\n";
     // 加载插件
    core::PluginManager::instance().loadPlugin("plugins/json_process.dll");

    // 测试下JsonProcess插件
    auto jsonProcess = core::PluginManager::instance()
        .createTyped<IJsonProcess>("default_json_process");

    std::string srcPath = "input.json";
    std::string desPath = "output.json";

    auto srcAbs = std::filesystem::weakly_canonical(srcPath).string();
    auto desAbs = std::filesystem::weakly_canonical(desPath).string();
    std::cout << "[Abs paths] " << srcAbs << " -> " << desAbs << "\n";

    std::ifstream srcStream(srcAbs);
    if (!srcStream) {
        std::cerr << "[Main] Failed to open " << srcAbs << "\n";
        return -1;
    }
    std::ostringstream buffer;
    buffer << srcStream.rdbuf();
    const std::string srcContent = buffer.str();
    std::cout << "[Main] Loaded " << srcContent.size() << " bytes from src file\n";

    if (jsonProcess) {
        std::string pluginOutput;
        bool ok = jsonProcess->processJsonFiles(
            srcContent,
            pluginOutput,
            "key1/key2",
            "find");
        if (ok) {
            std::ofstream out(desAbs);
            if (!out) {
                std::cerr << "[Main] Failed to open " << desAbs << " for writing\n";
                return -1;
            }
            out << pluginOutput;
            std::cout << "[Main] Plugin output saved to " << desAbs << "\n";
        } else {
            std::cerr << "[Main] Plugin processing failed\n";
        }
    } else {
        std::cerr << "Failed to create JsonProcess instance." << std::endl;
    }
        
    return 0;
}
