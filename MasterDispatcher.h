#ifndef MASTER_DISPATCHER_H
#define MASTER_DISPATCHER_H

#include <vector>
#include "Task.h"
#include "WorkerCore.h"
#include "SchedulingHeuristic.h"

// Central controller that owns the worker cores and waiting queue
class MasterDispatcher {
private:
    std::vector<WorkerCore> cores;
    std::vector<Task*> waiting_queue;
    SchedulingHeuristic* active_heuristic;

public:
    // Creates the requested number of cores
    MasterDispatcher(int num_cores) : active_heuristic(nullptr) {
        for (int i = 0; i < num_cores; ++i) {
            cores.push_back(WorkerCore(i));
        }
    }

    // Chooses which heuristic to use
    void set_heuristic(SchedulingHeuristic* heuristic) {
        active_heuristic = heuristic;
    }

    // Adds a task to the waiting queue when it arrives
    void receive_task(Task* t, int current_time) {
        t->arrival_time = current_time;
        waiting_queue.push_back(t);
    }

    // Calls the active heuristic to assign tasks to idle cores
    void dispatch(int current_time) {
        if (active_heuristic && !waiting_queue.empty()) {
            active_heuristic->schedule(waiting_queue, cores, current_time);
        }
    }

    // Advances every core by one simulation tick
    void tick_cores(int current_time) {
        for (auto& core : cores) {
            core.tick(current_time);
        }
    }

    // Returns true only when no tasks are waiting and all cores are idle
    bool is_system_idle() const {
        if (!waiting_queue.empty()) return false;

        for (const auto& core : cores) {
            if (!core.is_idle) return false;
        }

        return true;
    }

    // Gives access to core stats for performance reporting
    std::vector<WorkerCore>& get_cores() {
        return cores;
    }

    // Gives read-only access to the waiting queue if needed
    const std::vector<Task*>& get_waiting_queue() const {
        return waiting_queue;
    }
};

#endif // MASTER_DISPATCHER_H