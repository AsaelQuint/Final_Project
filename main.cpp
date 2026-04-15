#include <iostream>
#include "Task.h"
#include "MasterDispatcher.h"
#include "SchedulingHeuristic.h"

int main() {
    std::cout << "Starting Multi-Core Scheduling Simulation..." << std::endl;

    // 1. Initialize Dispatcher with 4 cores
    MasterDispatcher dispatcher(4);

    // 2. Set the Algorithm (You can swap this to LLSF_Heuristic to compare!)
    HIB_Heuristic hib_strategy;
    dispatcher.set_heuristic(&hib_strategy);

    // 3. Create some dummy tasks (ID, ExecTime, MinSep, Deadline, Instructions)
    Task t1(1, 10, 0, 50, 500);
    Task t2(2, 5,  0, 20, 200);
    Task t3(3, 15, 0, 80, 800);
    Task t4(4, 8,  0, 40, 400);
    Task t5(5, 20, 0, 100, 1000);

    // 4. The Simulation Loop
    int global_clock = 0;
    
    // Simulate tasks arriving at time 0
    dispatcher.receive_task(&t1, global_clock);
    dispatcher.receive_task(&t2, global_clock);
    dispatcher.receive_task(&t3, global_clock);
    dispatcher.receive_task(&t4, global_clock);
    dispatcher.receive_task(&t5, global_clock);

    // Run until all tasks are finished
    while (!dispatcher.is_system_idle()) {
        dispatcher.dispatch(global_clock);
        dispatcher.tick_cores();
        global_clock++;
    }

    std::cout << "Simulation completed in " << global_clock << " ticks." << std::endl;

    return 0;
}