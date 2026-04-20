#ifndef PERFORMANCE_REPORT_H
#define PERFORMANCE_REPORT_H

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include "Task.h"
#include "WorkerCore.h"

// Stores all metrics produced by one simulation run
struct SimulationMetrics {
    int makespan = 0;
    int completed_tasks = 0;
    int missed_deadlines = 0;
    double average_wait_time = 0.0;
    double average_turnaround_time = 0.0;
    double total_energy = 0.0;

    // Empirical measurements
    double empirical_speedup = 0.0;
    double empirical_efficiency = 0.0;

    // Theoretical Amdahl estimates
    double estimated_parallel_fraction = 0.0;
    double theoretical_amdahl_speedup = 0.0;

    // Useful workload structure metrics
    int total_work = 0;
    int critical_path = 0;
};

// Computes the overall critical path of the workload graph
inline int compute_critical_path_from_tasks(const std::vector<Task*>& tasks) {
    std::unordered_map<int, Task*> by_id;
    std::unordered_map<int, std::vector<Task*>> dependents;

    // Build lookup tables
    for (Task* t : tasks) {
        by_id[t->task_id] = t;
    }

    // Build reverse adjacency: task -> tasks that depend on it
    for (Task* t : tasks) {
        for (int dep_id : t->dependencies) {
            dependents[dep_id].push_back(t);
        }
    }

    std::unordered_map<int, int> memo;

    // DFS to compute longest path starting from a task
    std::function<int(Task*)> dfs = [&](Task* task) -> int {
        if (memo.count(task->task_id)) {
            return memo[task->task_id];
        }

        int best_child = 0;
        for (Task* child : dependents[task->task_id]) {
            best_child = std::max(best_child, dfs(child));
        }

        memo[task->task_id] = task->execution_time + best_child;
        return memo[task->task_id];
    };

    int best = 0;
    for (Task* t : tasks) {
        best = std::max(best, dfs(t));
    }

    return best;
}

// Computes all performance metrics after one simulation run
inline SimulationMetrics compute_metrics(const std::vector<Task*>& tasks,
                                         const std::vector<WorkerCore>& cores,
                                         int num_cores,
                                         int serial_baseline_makespan) {
    SimulationMetrics metrics;

    int total_wait = 0;
    int total_turnaround = 0;

    // Collect task-based metrics
    for (Task* t : tasks) {
        metrics.total_work += t->execution_time;

        if (t->state == TaskState::COMPLETED) {
            metrics.completed_tasks++;

            int wait_time = t->start_time - t->arrival_time;
            int turnaround = t->completion_time - t->arrival_time;

            total_wait += wait_time;
            total_turnaround += turnaround;

            if (t->completion_time > metrics.makespan) {
                metrics.makespan = t->completion_time;
            }

            if (t->completion_time > t->deadline) {
                metrics.missed_deadlines++;
            }
        }
    }

    // Compute averages
    if (metrics.completed_tasks > 0) {
        metrics.average_wait_time =
            static_cast<double>(total_wait) / metrics.completed_tasks;

        metrics.average_turnaround_time =
            static_cast<double>(total_turnaround) / metrics.completed_tasks;
    }

    // Compute graph-wide critical path
    metrics.critical_path = compute_critical_path_from_tasks(tasks);

    // Simplified Woo-Lee-style energy model:
    // active energy + idle energy + small dispatch overhead
    const double ACTIVE_POWER = 2.0;
    const double IDLE_POWER = 0.5;
    const double DISPATCH_OVERHEAD = 0.15;

    for (const auto& core : cores) {
        metrics.total_energy +=
            (core.total_active_time * ACTIVE_POWER) +
            (core.total_idle_time * IDLE_POWER) +
            (core.total_task_assignments * DISPATCH_OVERHEAD);
    }

    // Empirical speedup/efficiency
    if (metrics.makespan > 0) {
        metrics.empirical_speedup =
            static_cast<double>(serial_baseline_makespan) / metrics.makespan;

        metrics.empirical_efficiency =
            metrics.empirical_speedup / num_cores;
    }

    // Theoretical Amdahl estimate:
    // estimate serial fraction using critical path / total work
    if (metrics.total_work > 0) {
        double serial_fraction =
            static_cast<double>(metrics.critical_path) / metrics.total_work;

        if (serial_fraction < 0.0) serial_fraction = 0.0;
        if (serial_fraction > 1.0) serial_fraction = 1.0;

        metrics.estimated_parallel_fraction = 1.0 - serial_fraction;

        metrics.theoretical_amdahl_speedup =
            1.0 / ((1.0 - metrics.estimated_parallel_fraction) +
                   (metrics.estimated_parallel_fraction / num_cores));
    }

    return metrics;
}

// Prints the final results for one heuristic
inline void print_metrics(const std::string& name, const SimulationMetrics& m) {
    std::cout << "\n=== " << name << " Results ===\n";
    std::cout << "Makespan: " << m.makespan << "\n";
    std::cout << "Completed Tasks: " << m.completed_tasks << "\n";
    std::cout << "Missed Deadlines: " << m.missed_deadlines << "\n";
    std::cout << "Total Work: " << m.total_work << "\n";
    std::cout << "Critical Path: " << m.critical_path << "\n";

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Average Wait Time: " << m.average_wait_time << "\n";
    std::cout << "Average Turnaround Time: " << m.average_turnaround_time << "\n";
    std::cout << "Total Energy: " << m.total_energy << "\n";
    std::cout << "Empirical Speedup: " << m.empirical_speedup << "\n";
    std::cout << "Empirical Efficiency: " << m.empirical_efficiency << "\n";
    std::cout << "Estimated Parallel Fraction (P): " << m.estimated_parallel_fraction << "\n";
    std::cout << "Theoretical Amdahl Speedup: " << m.theoretical_amdahl_speedup << "\n";
}

#endif // PERFORMANCE_REPORT_H