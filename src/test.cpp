#include <gtest/gtest.h>
#include "threadpool/ThreadPool.hpp"
#include "config/ConfigParser.hpp"
#include "ui/console_ui.hpp"
#include "core/PluginManager.hpp"
#include "core/IJsonProcess.hpp"
#include "core/IComparator.hpp"
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

using namespace std::chrono_literals;

namespace {

bool ensurePluginLoaded(const std::string& relPath) {
    static std::unordered_set<std::string> loaded;
    if (loaded.count(relPath)) return true;
    if (!std::filesystem::exists(relPath)) {
        std::cerr << "[Tests] Plugin missing: " << relPath << std::endl;
        return false;
    }
    if (core::PluginManager::instance().loadPlugin(relPath)) {
        loaded.insert(relPath);
        return true;
    }
    return false;
}

std::string readFile(const std::string& path) {
    std::ifstream ifs(path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

std::filesystem::path uniqueTempFile(const std::string& prefix, const std::string& ext) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto fileName = prefix + "_" + std::to_string(now) + ext;
    return std::filesystem::temp_directory_path() / fileName;
}

} // namespace

TEST(ConfigParserTests, ParseFile) {
    auto cfg = app::parseConfigFromFile("file1.json");
    EXPECT_EQ(cfg.workflow.name, "Engine System Monitor Pipeline");
    EXPECT_TRUE(cfg.has_system_resource_monitor);
}

TEST(ConsoleUITests, TaskListAndProgress) {
    std::vector<ui::Task> tasks;
    tasks.push_back(ui::Task{"t1", ui::Pending});
    tasks.push_back(ui::Task{"t2", ui::InProgress});
    ui::printTaskList(tasks);
    ui::printProgressBar(42, 10);
    ui::printSpinnerFrame(1);
    SUCCEED();
}

TEST(ThreadPoolTests, HighConcurrencySubmission) {
    tp::ThreadPool pool(4);
    const int N = 1000;
    std::atomic<int> counter(0);
    std::vector<std::future<void>> futures;
    futures.reserve(N);
    for (int i = 0; i < N; ++i) {
        auto f = pool.submit([&counter]{ counter.fetch_add(1); });
        futures.push_back(std::move(f));
    }
    for (auto &f : futures) f.get();
    EXPECT_EQ(counter.load(), N);
}

TEST(JsonProcessTests, FindExtractsExpectedValue) {
    ASSERT_TRUE(ensurePluginLoaded("plugins/json_process.dll"));
    auto jsonProcess = core::PluginManager::instance()
        .createTyped<core::IJsonProcess>("default_json_process");
    ASSERT_TRUE(jsonProcess);

    const std::string content = readFile("input.json");
    ASSERT_FALSE(content.empty());

    std::string output;
    ASSERT_TRUE(jsonProcess->processJsonFiles(content, output, "key1/key2", "find"));

    auto parsed = nlohmann::json::parse(output);
    EXPECT_EQ(parsed["processMethod"], "find");
    ASSERT_FALSE(parsed["results"].empty());
    EXPECT_EQ(parsed["results"][0], "VALUE-A");
}

TEST(JsonProcessTests, AddWrapsBranchUnderPrefix) {
    ASSERT_TRUE(ensurePluginLoaded("plugins/json_process.dll"));
    auto jsonProcess = core::PluginManager::instance()
        .createTyped<core::IJsonProcess>("default_json_process");
    ASSERT_TRUE(jsonProcess);

    const std::string content = readFile("input.json");
    ASSERT_FALSE(content.empty());

    std::string output;
    ASSERT_TRUE(jsonProcess->processJsonFiles(
        content, output, "key11/key12|key1/key2", "add"));

    auto parsed = nlohmann::json::parse(output);
    EXPECT_EQ(parsed["processMethod"], "add");
    EXPECT_EQ(parsed["newPath"], "key11/key12/key1/key2");

    ASSERT_TRUE(parsed.contains("updatedJson"));
    auto updated = parsed["updatedJson"];
    EXPECT_FALSE(updated.contains("key1")); // moved under prefix
    ASSERT_TRUE(updated.contains("key11"));
    ASSERT_TRUE(updated["key11"].contains("key12"));
    auto leaf = updated["key11"]["key12"]["key1"]["key2"];
    EXPECT_EQ(leaf, "VALUE-A");
}

TEST(JsonCompareTests, GeneratesHtmlReport) {
    ASSERT_TRUE(ensurePluginLoaded("plugins/json_compare.dll"));
    auto comparator = core::PluginManager::instance()
        .createTyped<core::IComparator>("default_json_compare");
    ASSERT_TRUE(comparator);

    auto ours = uniqueTempFile("json_compare_ours", ".json");
    auto golden = uniqueTempFile("json_compare_golden", ".json");
    auto report = uniqueTempFile("json_compare_report", ".html");

    {
        std::ofstream ofs(ours);
        ofs << R"({"value":1,"arr":[1,2]})";
    }
    {
        std::ofstream ofs(golden);
        ofs << R"({"value":2,"arr":[1,3]})";
    }

    ASSERT_TRUE(comparator->compareFiles(
        ours.string(), golden.string(), report.string()));

    const std::string html = readFile(report.string());
    EXPECT_NE(html.find("<html"), std::string::npos);
    EXPECT_NE(html.find("JSON 对比报告"), std::string::npos);
    EXPECT_NE(html.find("GOLDEN"), std::string::npos);
    EXPECT_NE(html.find("&lt;"), std::string::npos); // ensure escaping applied

    std::error_code ec;
    std::filesystem::remove(ours, ec);
    std::filesystem::remove(golden, ec);
    std::filesystem::remove(report, ec);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
