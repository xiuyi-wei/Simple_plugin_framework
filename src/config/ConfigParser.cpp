#include <fstream>
#include <iostream>
#include <stdexcept>
#include "config/ConfigParser.hpp"

// Use the nlohmann json header bundled with the json_compare plugin.
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace app {

Config parseConfigFromFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) throw std::runtime_error("Cannot open config file: " + path);

    json j;
    try {
        ifs >> j;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }

    Config cfg;

    if (j.contains("workflow")) {
        auto &w = j["workflow"];
        cfg.workflow.name = w.value("name", "");
        cfg.workflow.version = w.value("version", "");
        cfg.workflow.description = w.value("description", "");
    }

    if (j.contains("system_resource_monitor")) {
        auto &s = j["system_resource_monitor"];
        SystemResourceMonitor sm;
        sm.description = s.value("description", "");
        if (s.contains("inputs")) {
            auto &inp = s["inputs"];
            sm.target_process = inp.value("target_process", "");
            sm.interval_sec = inp.value("interval_sec", 1);
            sm.duration_sec = inp.value("duration_sec", 0);
        }

        if (s.contains("metrics") && s["metrics"].is_array()) {
            for (auto &m : s["metrics"]) {
                Metric mm;
                mm.name = m.value("name", "");
                mm.valid = m.value("valid", false);
                mm.description = m.value("description", "");
                mm.scripts = m.value("scripts", "");
                mm.outputs = m.value("outputs", "");
                sm.metrics.push_back(std::move(mm));
            }
        }

        if (s.contains("outputs")) {
            auto &o = s["outputs"];
            sm.csv_glob = o.value("csv_file", "");
            sm.plot_glob = o.value("plot_file", "");
        }

        cfg.system_resource_monitor = std::move(sm);
        cfg.has_system_resource_monitor = true;
    }

    if (j.contains("report")) {
        auto &r = j["report"];
        cfg.report.combine_script = r.value("combine_script", "");
        cfg.report.final_archive = r.value("final_archive", "");
    }

    return cfg;
}

void printConfig(const Config& cfg, const std::string& header) {
    std::cout << "---- " << header << " ----\n";
    std::cout << "Workflow:\n";
    std::cout << "  name: " << cfg.workflow.name << "\n";
    std::cout << "  version: " << cfg.workflow.version << "\n";
    std::cout << "  description: " << cfg.workflow.description << "\n";

    if (cfg.has_system_resource_monitor) {
        auto &s = cfg.system_resource_monitor;
        std::cout << "SystemResourceMonitor:\n";
        std::cout << "  description: " << s.description << "\n";
        std::cout << "  inputs:\n";
        std::cout << "    target_process: " << s.target_process << "\n";
        std::cout << "    interval_sec: " << s.interval_sec << "\n";
        std::cout << "    duration_sec: " << s.duration_sec << "\n";
        std::cout << "  metrics:\n";
        for (const auto &m : s.metrics) {
            std::cout << "    - name: " << m.name << ", valid: " << (m.valid?"true":"false") << "\n";
            std::cout << "      description: " << m.description << "\n";
            std::cout << "      scripts: " << m.scripts << "\n";
            std::cout << "      outputs: " << m.outputs << "\n";
        }
        std::cout << "  outputs:\n";
        std::cout << "    csv_file: " << s.csv_glob << "\n";
        std::cout << "    plot_file: " << s.plot_glob << "\n";
    }

    std::cout << "Report:\n";
    std::cout << "  combine_script: " << cfg.report.combine_script << "\n";
    std::cout << "  final_archive: " << cfg.report.final_archive << "\n";
    std::cout << "-------------------------\n";
}

} // namespace app
