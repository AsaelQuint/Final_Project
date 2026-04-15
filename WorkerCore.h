#ifndef WORKER_CORE_H
#define WORKER_CORE_H

#include "Task.h"

class WorkerCore {
public:
    int core_id;
    bool is_idle;
    Task* current_task;
    int time_until_free;

    // Metrics for Woo-lee power consumption model
    int total_active_time;
    int total_idle_time;

    WorkerCore(int id) : core_id(id), is_idle(true), current_task(nullptr), 
                         time_until_free(0), total_active_time(0), total_idle_time(0) {}

    // Simulates one tick of the clock
    void tick() {
        if (!is_idle) {
            time_until_free--;
            total_active_time++;
            if (time_until_free <= 0) {
                current_task->state = TaskState::COMPLETED;
                is_idle = true;
                current_task = nullptr;
            }
        } else {
            total_idle_time++;
        }
    }

    void assign_task(Task* t) {
        is_idle = false;
        current_task = t;
        current_task->state = TaskState::RUNNING;
        time_until_free = t->execution_time;
    }
};

#endif // WORKER_CORE_H