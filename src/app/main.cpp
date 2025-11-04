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

using namespace core;

int main() {
    // Ensure console uses UTF-8 on Windows so Chinese text prints correctly
#if defined(_WIN32)
    // Switch the console output/input code page to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // Parse and print project JSON configs (file1.json, file2.json) for developer visibility
    try {
        auto cfg1 = app::parseConfigFromFile("file1.json");
        app::printConfig(cfg1, "file1.json");
    } catch (const std::exception& e) {
        std::cerr << "Warning: cannot parse file1.json: " << e.what() << std::endl;
    }

    // --- Console UI demo: task list, progress bar, and spinner ---
    std::cout << std::endl << "Console UI demo:\n";
    std::vector<ui::Task> tasks;
    tasks.push_back(ui::Task{"Load config", ui::Done});
    tasks.push_back(ui::Task{"Connect plugin", ui::Pending});
    tasks.push_back(ui::Task{"Compare files", ui::Pending});

    ui::printTaskList(tasks);

    // Simulate connecting plugin with progress bar
    ui::markTaskInProgressAndPrint(tasks[1]);
    for (int p = 0; p <= 100; p += 10) {
        ui::printProgressBar(p, 30);
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
    std::cout << std::endl;
    ui::markTaskDoneAndPrint(tasks[1]);

    // Simulate comparing files with a spinner
    ui::markTaskInProgressAndPrint(tasks[2]);
    for (int f = 0; f < 20; ++f) {
        ui::printSpinnerFrame(f);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;
    ui::markTaskDoneAndPrint(tasks[2]);
    std::cout << std::endl;

    auto& pm = PluginManager::get();

#if defined(_WIN32)
    const char* pluginPath = "./plugins/json_compare.dll"; // app is placed in build/bin
#else
    const char* pluginPath = "./plugins/libjson_compare.so"; // app is placed in build/bin
#endif

    if (!pm.loadPlugin(pluginPath)) {
        std::cerr << "Cannot load plugin: " << pluginPath << std::endl;
        return 1;
    }

    auto obj = pm.create("clsidJsonComparator");
    if (!obj) return 1;

    auto comp = std::dynamic_pointer_cast<core::IComparator>(obj);
    if (comp) {
        comp->compareFiles("file1.json","file2.json","report.txt");
    }

    // If configuration workflow.json exists, run it
    try {
        rt::LocalFS fs; rt::StdLogger logger; rt::SteadyClock clock;
        if (fs.exists("workflow.json")) {
            auto spec = wf::parseWorkflowJson("workflow.json");
            rt::EventBus bus; rt::ThreadPool pool(3); wf::SimpleContext ctx(logger, clock, fs);
            bus.subscribe([&](const rt::Event& e){
                if (e.type == "task_started")  std::cout << "[CFG] start: " << e.id << "\n";
                if (e.type == "task_finished") std::cout << "[CFG] done:  " << e.id << " ok=" << (e.success?"true":"false") << "\n";
            });
            wf::Executor exec(bus, pool);
            bool ok = exec.run(spec, ctx);
            std::cout << "Config flow result ok: " << (ok?"true":"false");
            if (!spec.final_key.empty()) std::cout << ", final=" << ctx.get(spec.final_key);
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "workflow.json error: " << e.what() << std::endl;
    }

    // --- Taskflow demo: compute 1*(2+1)+3 ---
    {
        struct ConstTask : public wf::ITask {
            std::string name, key, value;
            ConstTask(std::string id, std::string k, std::string v)
                : name(std::move(id)), key(std::move(k)), value(std::move(v)) {}
            std::string id() const override { return name; }
            wf::TaskResult run(wf::ITaskContext& ctx) override {
                ctx.set(key, value); return {true, {}};
            }
        };
        struct AddTask : public wf::ITask {
            std::string name, a, b, out;
            AddTask(std::string id, std::string ak, std::string bk, std::string ok)
                : name(std::move(id)), a(std::move(ak)), b(std::move(bk)), out(std::move(ok)) {}
            std::string id() const override { return name; }
            wf::TaskResult run(wf::ITaskContext& ctx) override {
                try {
                    int ai = std::stoi(ctx.get(a));
                    int bi = std::stoi(ctx.get(b));
                    ctx.set(out, std::to_string(ai + bi));
                    return {true, {}};
                } catch(...) { return {false, "AddTask parse error"}; }
            }
        };
        struct MulTask : public wf::ITask {
            std::string name, a, b, out;
            MulTask(std::string id, std::string ak, std::string bk, std::string ok)
                : name(std::move(id)), a(std::move(ak)), b(std::move(bk)), out(std::move(ok)) {}
            std::string id() const override { return name; }
            wf::TaskResult run(wf::ITaskContext& ctx) override {
                try {
                    int ai = std::stoi(ctx.get(a));
                    int bi = std::stoi(ctx.get(b));
                    ctx.set(out, std::to_string(ai * bi));
                    return {true, {}};
                } catch(...) { return {false, "MulTask parse error"}; }
            }
        };

        wf::WorkflowSpec f;
        // Nodes
        f.tasks.push_back(std::make_shared<ConstTask>("const1", "k1", "1"));
        f.tasks.push_back(std::make_shared<ConstTask>("const2", "k2", "2"));
        f.tasks.push_back(std::make_shared<ConstTask>("const3", "k3", "3"));
        f.tasks.push_back(std::make_shared<AddTask>("add", "k2", "k1", "sum"));        // sum = 2 + 1
        f.tasks.push_back(std::make_shared<MulTask>("mul", "k1", "sum", "prod"));        // prod = 1 * sum
        f.tasks.push_back(std::make_shared<AddTask>("plus3", "prod", "k3", "final"));   // final = prod + 3
        // Edges
        f.edges.push_back({"const2", "add"});
        f.edges.push_back({"const1", "add"});
        f.edges.push_back({"const1", "mul"});
        f.edges.push_back({"add", "mul"});
        f.edges.push_back({"mul", "plus3"});
        f.edges.push_back({"const3", "plus3"});

        rt::EventBus bus;
        rt::ThreadPool pool(3);
        rt::StdLogger logger; rt::SteadyClock clock; rt::LocalFS fs;
        wf::SimpleContext ctx(logger, clock, fs);
        bus.subscribe([&](const rt::Event& e){
            if (e.type == "task_started") std::cout << "[MATH] start: " << e.id << "\n";
            if (e.type == "task_finished") std::cout << "[MATH] done:  " << e.id << " ok=" << (e.success?"true":"false") << "\n";
        });
        wf::Executor exec(bus, pool);
        bool ok2 = exec.run(f, ctx);
        std::cout << "Math flow result ok: " << (ok2?"true":"false") << ", final=" << ctx.get("final") << std::endl;
    }

    pm.unloadAll();
    return 0;
}
