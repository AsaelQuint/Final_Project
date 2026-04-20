#ifndef SCHEDULING_HEURISTIC_H
#define SCHEDULING_HEURISTIC_H

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include "Task.h"
#include "WorkerCore.h"

// Base class for all heuristics
// Each derived class implements its own schedule() method
class SchedulingHeuristic {
protected:
    // Pointer to the full task pool used for dependency / graph analysis
    std::vector<Task*>* task_pool;

    // Finds a task by ID inside the task pool
    Task* find_task_by_id(int task_id) const {
        if (!task_pool) return nullptr;

        for (Task* t : *task_pool) {
            if (t->task_id == task_id) {
                return t;
            }
        }
        return nullptr;
    }

    // Returns true only if all dependencies of a task are completed
    bool dependencies_satisfied(Task* task) const {
        for (int dep_id : task->dependencies) {
            Task* dep = find_task_by_id(dep_id);
            if (!dep || dep->state != TaskState::COMPLETED) {
                return false;
            }
        }
        return true;
    }

    // Computes graph metrics for every task:
    // - graph_height
    // - critical_path_length
    //
    // graph_height:
    //   number of dependency levels from source tasks
    //
    // critical_path_length:
    //   longest execution path from this task to the end of graph
    void compute_graph_metrics() {
        if (!task_pool) return;

        // Build reverse edges: task_id -> list of tasks that depend on it
        std::unordered_map<int, std::vector<Task*>> dependents;
        for (Task* t : *task_pool) {
            for (int dep_id : t->dependencies) {
                dependents[dep_id].push_back(t);
            }
        }

        std::unordered_map<int, int> memo_height;
        std::unordered_map<int, int> memo_cp;

        // Recursively compute height from source side
        std::function<int(Task*)> calc_height = [&](Task* task) -> int {
            if (memo_height.count(task->task_id)) {
                return memo_height[task->task_id];
            }

            if (task->dependencies.empty()) {
                memo_height[task->task_id] = 0;
                return 0;
            }

            int best = 0;
            for (int dep_id : task->dependencies) {
                Task* dep = find_task_by_id(dep_id);
                if (dep) {
                    best = std::max(best, calc_height(dep) + 1);
                }
            }

            memo_height[task->task_id] = best;
            return best;
        };

        // Recursively compute critical path from this task to sink side
        std::function<int(Task*)> calc_cp = [&](Task* task) -> int {
            if (memo_cp.count(task->task_id)) {
                return memo_cp[task->task_id];
            }

            int best_child = 0;
            for (Task* child : dependents[task->task_id]) {
                best_child = std::max(best_child, calc_cp(child));
            }

            memo_cp[task->task_id] = task->execution_time + best_child;
            return memo_cp[task->task_id];
        };

        // Fill graph_height for all tasks
        for (Task* t : *task_pool) {
            t->graph_height = calc_height(t);
        }

        // Fill critical_path_length for all tasks
        for (Task* t : *task_pool) {
            t->critical_path_length = calc_cp(t);
        }
    }

public:
    SchedulingHeuristic() : task_pool(nullptr) {}
    virtual ~SchedulingHeuristic() = default;

    // Called before simulation starts so the heuristic can inspect all tasks
    virtual void set_task_pool(std::vector<Task*>* tasks) {
        task_pool = tasks;
        compute_graph_metrics();
    }

    // Every heuristic must implement this method
    virtual void schedule(std::vector<Task*>& waiting_queue,
                          std::vector<WorkerCore>& cores,
                          int current_time) = 0;
};

// HIB: Height Instruction Count Based
// Priority:
// 1. Higher graph height
// 2. Higher instruction count
class HIB_Heuristic : public SchedulingHeuristic {
public:
    void schedule(std::vector<Task*>& waiting_queue,
                  std::vector<WorkerCore>& cores,
                  int current_time) override {
        std::vector<Task*> ready_tasks;

        // Only schedule tasks whose dependencies are satisfied
        for (Task* t : waiting_queue) {
            if (dependencies_satisfied(t)) {
                ready_tasks.push_back(t);
            }
        }

        // Sort tasks by HIB priority
        std::sort(ready_tasks.begin(), ready_tasks.end(),
            [](Task* a, Task* b) {
                if (a->graph_height == b->graph_height) {
                    return a->instruction_count > b->instruction_count;
                }
                return a->graph_height > b->graph_height;
            });

        // Assign highest-priority ready tasks to idle cores
        for (auto& core : cores) {
            if (core.is_idle && !ready_tasks.empty()) {
                Task* t = ready_tasks.front();
                t->start_time = current_time;
                core.assign_task(t);

                // Remove assigned task from waiting queue
                waiting_queue.erase(
                    std::remove(waiting_queue.begin(), waiting_queue.end(), t),
                    waiting_queue.end()
                );

                ready_tasks.erase(ready_tasks.begin());
            }
        }
    }
};

// LLSF: Longest Latency Sub-Block First
// In this implementation, priority is mainly based on execution time,
// using instruction count as a tiebreaker
class LLSF_Heuristic : public SchedulingHeuristic {
public:
    void schedule(std::vector<Task*>& waiting_queue,
                  std::vector<WorkerCore>& cores,
                  int current_time) override {
        std::vector<Task*> ready_tasks;

        for (Task* t : waiting_queue) {
            if (dependencies_satisfied(t)) {
                ready_tasks.push_back(t);
            }
        }

        // Sort by longest execution time first
        std::sort(ready_tasks.begin(), ready_tasks.end(),
            [](Task* a, Task* b) {
                if (a->execution_time == b->execution_time) {
                    return a->instruction_count > b->instruction_count;
                }
                return a->execution_time > b->execution_time;
            });

        for (auto& core : cores) {
            if (core.is_idle && !ready_tasks.empty()) {
                Task* t = ready_tasks.front();
                t->start_time = current_time;
                core.assign_task(t);

                waiting_queue.erase(
                    std::remove(waiting_queue.begin(), waiting_queue.end(), t),
                    waiting_queue.end()
                );

                ready_tasks.erase(ready_tasks.begin());
            }
        }
    }
};

// DSB: Dependent Sub-Block Based
// Priority:
// 1. Larger critical path length
// 2. More dependencies
// 3. Higher instruction count
class DSB_Heuristic : public SchedulingHeuristic {
public:
    void schedule(std::vector<Task*>& waiting_queue,
                  std::vector<WorkerCore>& cores,
                  int current_time) override {
        std::vector<Task*> ready_tasks;

        for (Task* t : waiting_queue) {
            if (dependencies_satisfied(t)) {
                ready_tasks.push_back(t);
            }
        }

        // Sort by DSB priority
        std::sort(ready_tasks.begin(), ready_tasks.end(),
            [](Task* a, Task* b) {
                if (a->critical_path_length == b->critical_path_length) {
                    if (a->dependencies.size() == b->dependencies.size()) {
                        return a->instruction_count > b->instruction_count;
                    }
                    return a->dependencies.size() > b->dependencies.size();
                }
                return a->critical_path_length > b->critical_path_length;
            });

        for (auto& core : cores) {
            if (core.is_idle && !ready_tasks.empty()) {
                Task* t = ready_tasks.front();
                t->start_time = current_time;
                core.assign_task(t);

                waiting_queue.erase(
                    std::remove(waiting_queue.begin(), waiting_queue.end(), t),
                    waiting_queue.end()
                );

                ready_tasks.erase(ready_tasks.begin());
            }
        }
    }
};

#endif // SCHEDULING_HEURISTIC_H