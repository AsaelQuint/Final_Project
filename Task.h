#ifndef TASK_H
#define TASK_H

#include <vector>

// Represents the current state of a task in the simulator
enum class TaskState {
    WAITING,
    RUNNING,
    COMPLETED,
    DROPPED
};

// Represents one schedulable task in the system
struct Task {
    int task_id;
    int execution_time;    // WCET / execution cost
    int min_separation;    // minimum separation between releases
    int deadline;          // absolute deadline in this simulator

    // Dependency / heuristic data
    std::vector<int> dependencies;  // task IDs that must finish first
    int instruction_count;          // used by HIB

    // Runtime tracking fields
    int arrival_time;
    int start_time;
    int completion_time;
    TaskState state;

    // Graph-derived metrics used by advanced heuristics
    int graph_height;          // depth from source side
    int critical_path_length;  // longest path from this task to sink side

    // Constructor
    Task(int id, int exec, int sep, int dead, int instructions)
        : task_id(id),
          execution_time(exec),
          min_separation(sep),
          deadline(dead),
          instruction_count(instructions),
          arrival_time(0),
          start_time(-1),
          completion_time(-1),
          state(TaskState::WAITING),
          graph_height(0),
          critical_path_length(exec) {}
};

#endif // TASK_H