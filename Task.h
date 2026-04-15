#ifndef TASK_H
#define TASK_H

#include <vector>

enum class TaskState {
    WAITING,
    RUNNING,
    COMPLETED,
    DROPPED
};

struct Task {
    int task_id;
    int execution_time;    // e_i: Worst-case execution time
    int min_separation;    // g_i: Minimum separation time
    int deadline;          // d_i: Relative deadline
    
    // For heuristic scheduling (HIB, DSB)
    std::vector<int> dependencies; 
    int instruction_count; 
    
    // Simulation tracking
    int arrival_time;
    int start_time;
    int completion_time;
    TaskState state;

    // Constructor for easy initialization
    Task(int id, int exec, int sep, int dead, int instructions)
        : task_id(id), execution_time(exec), min_separation(sep), 
          deadline(dead), instruction_count(instructions), 
          arrival_time(0), start_time(-1), completion_time(-1), 
          state(TaskState::WAITING) {}
};

#endif // TASK_H