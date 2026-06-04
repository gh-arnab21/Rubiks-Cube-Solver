/**
 * @file main_interactive.cpp
 * @brief Main entry point for interactive Rubik's Cube solver
 */

#include <iostream>
#include <string>
#include "renderer/InteractiveSolver.h"

using namespace cube_solver;

int main(int argc, char* argv[]) {
    std::cout << "Starting Rubik's Cube Interactive Solver..." << std::endl;
    
    // Create and initialize the interactive solver
    InteractiveSolver app;
    
    if (!app.initialize(1200, 800)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    // Try to load solver databases if they exist
    app.loadSolverDatabases("./databases");
    
    // Scramble the cube initially
    app.scramble(15);
    
    // Main application loop
    while (app.update()) {
        // Continue running
    }
    
    // Cleanup
    app.shutdown();
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}
