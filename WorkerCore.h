#ifndef WORKER_CORE_H
#define WORKER_CORE_H

#include "Task.h"

// Represents one simulated processing core
class WorkerCore {
public:
    int core_id;
    bool is_idle;
    Task* current_task;
    int time_until_free;

    // Statistics used for reporting / energy model
    int total_active_time;
    int total_idle_time;
    int total_task_assignments;

    // Constructor initializes the core in idle state
    WorkerCore(int id)
        : core_id(id),
          is_idle(true),
          current_task(nullptr),
          time_until_free(0),
          total_active_time(0),
          total_idle_time(0),
          total_task_assignments(0) {}

    // Simulates one clock tick on this core
    void tick(int current_time) {
        if (!is_idle) {
            // Core is doing useful work
            time_until_free--;
            total_active_time++;

            // If task finishes on this tick, mark it complete
            if (time_until_free <= 0) {
                current_task->state = TaskState::COMPLETED;
                current_task->completion_time = current_time;
                is_idle = true;
                current_task = nullptr;
            }
        } else {
            // Core remains idle for this tick
            total_idle_time++;
        }
    }

    // Assigns a task to the core
    void assign_task(Task* t) {
        is_idle = false;
        current_task = t;
        current_task->state = TaskState::RUNNING;
        time_until_free = t->execution_time;
        total_task_assignments++;
    }
};

#endif // WORKER_CORE_H

