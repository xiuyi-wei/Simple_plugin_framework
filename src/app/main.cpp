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

    pm.unloadAll();
    return 0;
}
