#ifndef SCHEDULING_HEURISTIC_H
#define SCHEDULING_HEURISTIC_H

#include <vector>
#include <algorithm>
#include "Task.h"
#include "WorkerCore.h"

// Abstract Base Class
class SchedulingHeuristic {
public:
    virtual ~SchedulingHeuristic() = default;
    
    // The core function every heuristic must implement
    virtual void schedule(std::vector<Task*>& waiting_queue, std::vector<WorkerCore>& cores, int current_time) = 0;
};

// --- Specific Heuristic Implementations ---

class HIB_Heuristic : public SchedulingHeuristic {
public:
    void schedule(std::vector<Task*>& waiting_queue, std::vector<WorkerCore>& cores, int current_time) override {
        // HIB: Height Instruction Count Based
        // Sort the waiting queue by instruction count (Descending)
        std::sort(waiting_queue.begin(), waiting_queue.end(), [](Task* a, Task* b) {
            return a->instruction_count > b->instruction_count;
        });

        // Assign to free cores
        for (auto& core : cores) {
            if (core.is_idle && !waiting_queue.empty()) {
                Task* t = waiting_queue.front();
                t->start_time = current_time;
                core.assign_task(t);
                waiting_queue.erase(waiting_queue.begin()); // Remove from queue
            }
        }
    }
};

class LLSF_Heuristic : public SchedulingHeuristic {
public:
    void schedule(std::vector<Task*>& waiting_queue, std::vector<WorkerCore>& cores, int current_time) override {
        // LLSF: Longest Latency Sub-Block First
        // Sort by longest execution time (Descending)
        std::sort(waiting_queue.begin(), waiting_queue.end(), [](Task* a, Task* b) {
            return a->execution_time > b->execution_time;
        });

        for (auto& core : cores) {
            if (core.is_idle && !waiting_queue.empty()) {
                Task* t = waiting_queue.front();
                t->start_time = current_time;
                core.assign_task(t);
                waiting_queue.erase(waiting_queue.begin());
            }
        }
    }
};

#endif // SCHEDULING_HEURISTIC_H