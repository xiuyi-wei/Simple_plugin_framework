#include "workflow/Executor.hpp"
#include <unordered_map>
#include <atomic>

namespace wf {

bool Executor::run(const WorkflowSpec& spec, ITaskContext& ctx) {
    // Build adjacency and indegree
    std::unordered_map<std::string, int> indeg;
    std::unordered_map<std::string, std::vector<std::string>> adj;
    std::unordered_map<std::string, std::shared_ptr<ITask>> tasks;
    for (auto& t : spec.tasks) {
        tasks[t->id()] = t;
        indeg[t->id()] = 0;
    }
    for (auto& e : spec.edges) {
        adj[e.from].push_back(e.to);
        indeg[e.to]++;
    }

    std::atomic<bool> ok{true};
    std::mutex mtx;
    std::queue<std::string> ready;
    for (auto& kv : indeg) if (kv.second == 0) ready.push(kv.first);

    std::atomic<int> remaining{ static_cast<int>(tasks.size()) };
    std::condition_variable cv;

    auto submitNode = [&](const std::string& id){
        pool_.submit([&, id]{
            rt::Event ev{ "task_started", id, true, "" };
            bus_.publish(ev);
            auto it = tasks.find(id);
            auto res = it->second->run(ctx);
            rt::Event ev2{ "task_finished", id, res.success, res.message };
            bus_.publish(ev2);
            if (!res.success) ok.store(false);

            // update indegrees of neighbors
            std::lock_guard<std::mutex> lg(mtx);
            for (auto& v : adj[id]) {
                if (--indeg[v] == 0) {
                    ready.push(v);
                    cv.notify_one();
                }
            }
            remaining--;
            cv.notify_one();
        });
    };

    while (remaining > 0) {
        std::string next;
        {
            std::unique_lock<std::mutex> lk(mtx);
            if (ready.empty()) {
                cv.wait(lk, [&]{ return !ready.empty() || remaining.load() == 0; });
            }
            if (!ready.empty()) { next = ready.front(); ready.pop(); }
        }
        if (!next.empty()) submitNode(next);
    }
    return ok.load();
}

} // namespace wf
