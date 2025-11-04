#pragma once
#include <functional>
#include <vector>
#include <string>

namespace rt {

struct Event {
    std::string type; // e.g. task_started, task_finished
    std::string id;
    bool success = true;
    std::string message;
};

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    void subscribe(Handler h) { handlers_.push_back(std::move(h)); }
    void publish(const Event& e) {
        for (auto& h : handlers_) h(e);
    }
private:
    std::vector<Handler> handlers_;
};

} // namespace rt

