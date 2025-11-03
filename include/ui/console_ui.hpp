#ifndef CONSOLE_UI_HPP
#define CONSOLE_UI_HPP

#include <string>
#include <vector>

namespace ui {

enum TaskStatus {
    Pending = 0,
    InProgress = 1,
    Done = 2
};

struct Task {
    std::string name;
    TaskStatus status;
};

// Print a horizontal progress bar for percent [0..100]. Does not add a newline.
void printProgressBar(int percent, int width = 40);

// Print a spinner frame (call repeatedly to animate). Does not add a newline.
void printSpinnerFrame(int frame);

// Print a single task as one line. Uses symbols: [ ] pending, [>] in-progress, [✓] done
void printTask(const Task& t);

// Print a list of tasks (each on its own line).
void printTaskList(const std::vector<Task>& tasks);

// Convenience helpers
void markTaskDoneAndPrint(Task& t);
void markTaskInProgressAndPrint(Task& t);

} // namespace ui

#endif // CONSOLE_UI_HPP