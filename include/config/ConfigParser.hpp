#pragma once
#include <string>
#include <vector>

namespace app {

struct Metric {
    std::string name;
    bool valid = false;
    std::string description;
    std::string scripts;
    std::string outputs;
};

struct SystemResourceMonitor {
    std::string description;
    std::string target_process;
    int interval_sec = 1;
    int duration_sec = 0;
    std::vector<Metric> metrics;
    std::string csv_glob;
    std::string plot_glob;
};

struct WorkflowInfo {
    std::string name;
    std::string version;
    std::string description;
};

struct ReportInfo {
    std::string combine_script;
    std::string final_archive;
};

struct Config {
    WorkflowInfo workflow;
    // std::optional isn't available in C++11; use a presence flag instead
    bool has_system_resource_monitor = false;
    SystemResourceMonitor system_resource_monitor;
    ReportInfo report;
};

// Parse JSON config at `path` into Config. Throws std::runtime_error on parse error.
Config parseConfigFromFile(const std::string& path);

// Print a compact representation to stdout (for debugging / main program use)
void printConfig(const Config& cfg, const std::string& header = "Config");

} // namespace app
