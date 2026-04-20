#include <iostream>
#include <vector>
#include <string>

#include "Task.h"
#include "MasterDispatcher.h"
#include "SchedulingHeuristic.h"
#include "TaskGenerator.h"
#include "PerformanceReport.h"

// Creates a deep copy of the original workload
// This is important because each heuristic should run on the same input,
// but each run changes task states, start times, and completion times
std::vector<Task*> clone_tasks(const std::vector<Task*>& source) {
    std::vector<Task*> copy;

    for (Task* t : source) {
        Task* cloned = new Task(
            t->task_id,
            t->execution_time,
            t->min_separation,
            t->deadline,
            t->instruction_count
        );

        // Copy dependency and arrival info
        cloned->dependencies = t->dependencies;
        cloned->arrival_time = t->arrival_time;

        copy.push_back(cloned);
    }

    return copy;
}

// Deletes all dynamically allocated tasks in a vector
void free_tasks(std::vector<Task*>& tasks) {
    for (Task* t : tasks) {
        delete t;
    }
    tasks.clear();
}

// Computes a simple serial baseline:
// total execution time if one processor handled all tasks sequentially
int run_serial_baseline(const std::vector<Task*>& workload) {
    int serial_time = 0;

    for (Task* t : workload) {
        serial_time += t->execution_time;
    }

    return serial_time;
}

// Runs one full simulation for one heuristic
SimulationMetrics run_simulation(const std::string& heuristic_name,
                                 SchedulingHeuristic* heuristic,
                                 const std::vector<Task*>& workload,
                                 int num_cores,
                                 int serial_baseline_makespan) {
    // Create a fresh dispatcher for this run
    MasterDispatcher dispatcher(num_cores);
    dispatcher.set_heuristic(heuristic);

    // Clone workload so each heuristic sees the same initial task set
    std::vector<Task*> sim_tasks = clone_tasks(workload);

    // Give the heuristic access to the full task set
    // so it can compute dependency graph metrics
    heuristic->set_task_pool(&sim_tasks);

    int global_clock = 0;
    std::size_t next_task_index = 0;

    // Continue while there are tasks not yet released OR the system is still busy
    while (next_task_index < sim_tasks.size() || !dispatcher.is_system_idle()) {
        // Release tasks whose arrival time matches the current clock
        while (next_task_index < sim_tasks.size() &&
               sim_tasks[next_task_index]->arrival_time == global_clock) {
            dispatcher.receive_task(sim_tasks[next_task_index], global_clock);
            next_task_index++;
        }

        // Schedule waiting tasks onto cores
        dispatcher.dispatch(global_clock);

        // Advance all cores by one tick
        dispatcher.tick_cores(global_clock);

        // Move to next time step
        global_clock++;
    }

    // Compute final metrics for this run
    SimulationMetrics metrics =
        compute_metrics(sim_tasks, dispatcher.get_cores(), num_cores, serial_baseline_makespan);

    // Print results for this heuristic
    print_metrics(heuristic_name, metrics);

    // Clean up cloned tasks
    free_tasks(sim_tasks);

    return metrics;
}

int main() {
    std::cout << "Starting Multi-Core Scheduling Simulation..." << std::endl;

    const int NUM_CORES = 4;
    const int GENERATION_TICKS = 30;

    // Configure workload generation
    TaskGeneratorConfig config;
    config.min_exec_time = 2;
    config.max_exec_time = 12;
    config.min_deadline_gap = 8;
    config.max_deadline_gap = 30;
    config.min_instructions = 100;
    config.max_instructions = 1500;
    config.poisson_lambda = 2.0;
    config.dependency_probability = 0.30;
    config.max_dependencies_per_task = 2;

    // Create generator with fixed seed so runs are reproducible
    TaskGenerator generator(42, config);

    std::vector<Task*> workload;

    // Generate workload over time
    // Each tick may produce 0, 1, or more new tasks
    for (int current_time = 0; current_time < GENERATION_TICKS; ++current_time) {
        std::vector<Task*> new_tasks =
            generator.generate_tasks_for_tick(current_time, workload);

        for (Task* t : new_tasks) {
            workload.push_back(t);
        }
    }

    std::cout << "Generated " << workload.size() << " tasks.\n";

    // Compute serial baseline for empirical speedup comparison
    int serial_baseline_makespan = run_serial_baseline(workload);
    std::cout << "Serial Baseline Makespan: " << serial_baseline_makespan << "\n";

    // Create one object for each heuristic
    HIB_Heuristic hib_strategy;
    LLSF_Heuristic llsf_strategy;
    DSB_Heuristic dsb_strategy;

    // Run all heuristics on the same workload
    run_simulation("HIB", &hib_strategy, workload, NUM_CORES, serial_baseline_makespan);
    run_simulation("LLSF", &llsf_strategy, workload, NUM_CORES, serial_baseline_makespan);
    run_simulation("DSB", &dsb_strategy, workload, NUM_CORES, serial_baseline_makespan);

    // Free the original workload after all runs finish
    free_tasks(workload);

    std::cout << "\nSimulation completed." << std::endl;
    return 0;
}