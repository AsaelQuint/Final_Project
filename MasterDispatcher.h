#ifndef MASTER_DISPATCHER_H
#define MASTER_DISPATCHER_H

#include <vector>
#include "Task.h"
#include "WorkerCore.h"
#include "SchedulingHeuristic.h"

class MasterDispatcher {
private:
    std::vector<WorkerCore> cores;
    std::vector<Task*> waiting_queue;
    SchedulingHeuristic* active_heuristic;

public:
    MasterDispatcher(int num_cores) : active_heuristic(nullptr) {
        for (int i = 0; i < num_cores; ++i) {
            cores.push_back(WorkerCore(i));
        }
    }

    void set_heuristic(SchedulingHeuristic* heuristic) {
        active_heuristic = heuristic;
    }

    void receive_task(Task* t, int current_time) {
        t->arrival_time = current_time;
        waiting_queue.push_back(t);
    }

    void dispatch(int current_time) {
        if (active_heuristic && !waiting_queue.empty()) {
            active_heuristic->schedule(waiting_queue, cores, current_time);
        }
    }

    void tick_cores() {
        for (auto& core : cores) {
            core.tick();
        }
    }

    bool is_system_idle() {
        if (!waiting_queue.empty()) return false;
        for (const auto& core : cores) {
            if (!core.is_idle) return false;
        }
        return true;
    }
};

#endif // MASTER_DISPATCHER_H