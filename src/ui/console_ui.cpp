#include "../../include/ui/console_ui.hpp"
#include <iostream>
#include <iomanip>

namespace ui {

void printProgressBar(int percent, int width) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    int filled = (percent * width) / 100;
    std::cout << "\r[";
    for (int i = 0; i < filled; ++i) std::cout << "#";
    for (int i = filled; i < width; ++i) std::cout << "-";
    std::cout << "] ";
    std::cout << std::setw(3) << percent << "%";
    std::cout.flush();
}

void printSpinnerFrame(int frame) {
    static const char* frames = "|/-\\";
    char c = frames[frame % 4];
    std::cout << "\r" << c << std::flush;
}

void printTask(const Task& t) {
    const char* mark = "[ ]";
    if (t.status == InProgress) mark = "[>]";
    else if (t.status == Done) mark = "[\xE2\x9C\x93]"; // Unicode checkmark ✓ (UTF-8 encoded)
    std::cout << mark << " " << t.name << "\n";
}

void printTaskList(const std::vector<Task>& tasks) {
    for (size_t i = 0; i < tasks.size(); ++i) {
        printTask(tasks[i]);
    }
}

void markTaskDoneAndPrint(Task& t) {
    t.status = Done;
    printTask(t);
}

void markTaskInProgressAndPrint(Task& t) {
    t.status = InProgress;
    printTask(t);
}

} // namespace ui
