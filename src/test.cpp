#include <gtest/gtest.h>
#include "threadpool/ThreadPool.hpp"
#include "config/ConfigParser.hpp"
#include "ui/console_ui.hpp"

#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
