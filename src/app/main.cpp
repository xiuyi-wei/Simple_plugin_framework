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

    std::cout << "[WD] " << std::filesystem::current_path() << "\n";

    std::ifstream fs("G:/HKAVISION/test/Simple_plugin_framework11111111/input.json");
    std::cout << "[Test G:/ path] good=" << fs.good()  << "\n";

    std::ifstream fs2("G:\\HKAVISION\\test\\Simple_plugin_framework11111111\\input.json");
    std::cout << "[Test G:\\\\ path] good=" << fs2.good()  << "\n";

    std::ifstream fs3("G://HKAVISION//test//Simple_plugin_framework11111111//input.json");
    std::cout << "[Test G:// path] good=" << fs3.good()  << "\n";

    // 测试下JsonProcess插件
    auto jsonProcess = core::PluginManager::instance()
        .createTyped<IJsonProcess>("default_json_process");

    std::string srcPath = "input.json";
    std::string desPath = "output.json";

    auto p = std::filesystem::absolute(srcPath);
    std::cout << "[After normalize] " << p.string() << "\n";
    // std::ifstream fs(p);
    std::ifstream test(p);
    std::string line;
    int count = 0;
    while (std::getline(test, line)) {
        std::cout << "[Line] " << line << "\n";
        count++;
    }

    auto srcAbs = std::filesystem::weakly_canonical(srcPath).string();
    auto desAbs = std::filesystem::weakly_canonical(desPath).string();
    std::cout << "[Abs paths] " << srcAbs << " -> " << desAbs << "\n";
    std::cout << "[File lines] " << count << "\n";
    if (jsonProcess) {
        jsonProcess->processJsonFiles(
            srcAbs,
            desAbs,
            "key1/key2",
            "find"
        );
    } else {
        std::cerr << "Failed to create JsonProcess instance." << std::endl;
    }
        
    return 0;
}
