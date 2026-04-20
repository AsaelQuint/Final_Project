#ifndef TASK_GENERATOR_H
#define TASK_GENERATOR_H

#include <vector>
#include <random>
#include <algorithm>
#include "Task.h"

// Configuration values for workload generation
struct TaskGeneratorConfig {
    int min_exec_time = 2;
    int max_exec_time = 12;

    int min_deadline_gap = 8;
    int max_deadline_gap = 30;

    int min_instructions = 100;
    int max_instructions = 1500;

    double poisson_lambda = 2.0;          // average arrivals per tick
    double dependency_probability = 0.30; // chance of adding dependencies
    int max_dependencies_per_task = 2;
};

// Generates tasks dynamically as time advances
class TaskGenerator {
private:
    std::mt19937 rng;
    TaskGeneratorConfig config;
    int next_task_id;

public:
    TaskGenerator(unsigned int seed, const TaskGeneratorConfig& cfg)
        : rng(seed), config(cfg), next_task_id(1) {}

    // Generates a random number of tasks for the current tick
    std::vector<Task*> generate_tasks_for_tick(int current_time,
                                               const std::vector<Task*>& existing_tasks) {
        std::vector<Task*> new_tasks;

        // Random distributions used in this generator
        std::poisson_distribution<int> arrivals_dist(config.poisson_lambda);
        std::uniform_int_distribution<int> exec_dist(config.min_exec_time, config.max_exec_time);
        std::uniform_int_distribution<int> deadline_gap_dist(config.min_deadline_gap, config.max_deadline_gap);
        std::uniform_int_distribution<int> instruction_dist(config.min_instructions, config.max_instructions);
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

        // Number of arrivals this tick
        int arrivals = arrivals_dist(rng);

        for (int i = 0; i < arrivals; ++i) {
            int exec = exec_dist(rng);
            int deadline = current_time + deadline_gap_dist(rng);
            int instructions = instruction_dist(rng);

            // Create the task
            Task* t = new Task(next_task_id++, exec, 0, deadline, instructions);
            t->arrival_time = current_time;

            // Optionally assign dependencies from already existing tasks
            if (!existing_tasks.empty() && prob_dist(rng) < config.dependency_probability) {
                int max_deps = std::min(config.max_dependencies_per_task,
                                        static_cast<int>(existing_tasks.size()));

                std::uniform_int_distribution<int> num_deps_dist(1, max_deps);
                int deps_to_add = num_deps_dist(rng);

                std::vector<int> candidate_indices(existing_tasks.size());
                for (int j = 0; j < static_cast<int>(existing_tasks.size()); ++j) {
                    candidate_indices[j] = j;
                }

                std::shuffle(candidate_indices.begin(), candidate_indices.end(), rng);

                for (int j = 0; j < deps_to_add; ++j) {
                    t->dependencies.push_back(existing_tasks[candidate_indices[j]]->task_id);
                }
            }

            new_tasks.push_back(t);
        }

        return new_tasks;
    }
};

#endif // TASK_GENERATOR_H