/**
 * @file InteractiveSolver.cpp
 * @brief Interactive solver implementation with HybridSolver and OpenGL renderer
 */

#include "renderer/InteractiveSolver.h"
#include "solver/Solver.h"
#include "solver/HybridSolver.h"
#include <iostream>
#include <random>
#include <chrono>
#include <sstream>

namespace cube_solver {

// Initializes the interactive window, OpenGL context, solvers, and input callbacks
bool InteractiveSolver::initialize(int width, int height) {
    std::cout << "Initializing Interactive Solver..." << std::endl;
    
    // Initialize renderer
    renderer = std::make_unique<CubeRenderer>();
    if (!renderer->initialize(width, height, "Rubik's Cube Solver - Interactive")) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    // Wire up action keys
    renderer->onActionKey = [this](int key) {
        if (key == GLFW_KEY_SPACE) {
            this->solve();
        } else if (key == GLFW_KEY_S) {
            this->scramble(7);
        } else if (key == GLFW_KEY_G) {
            this->reset();
        } else if (key == GLFW_KEY_1) {
            this->activeSolver = SolverAlgorithm::IDA_STAR;
            std::cout << "Selected IDA* Solver" << std::endl;
        } else if (key == GLFW_KEY_2) {
            this->activeSolver = SolverAlgorithm::HYBRID;
            std::cout << "Selected Hybrid Solver" << std::endl;
        } else if (key == GLFW_KEY_3) {
            this->activeSolver = SolverAlgorithm::SIMPLE_IDDFS;
            std::cout << "Selected Simple IDDFS Solver" << std::endl;
        }
    };
    
    // Initialize solvers
    idaStarSolver = std::make_unique<KorfSolver>(true);  // symmetry reduction enabled
    hybridSolver = std::make_unique<HybridSolver>(true);
    simpleIDDFSSolver = std::make_unique<SimpleIDDFSSolver>();
    
    // Reset cube to solved state
    cube = RubiksCube();
    
    printHelp();
    
    return true;
}

void InteractiveSolver::shutdown() {
    std::cout << "Shutting down..." << std::endl;
    if (isSolving) {
        // Wait for solver thread if it's still running
        if (solveThread.joinable()) {
            solveThread.join();
        }
    }
    if (renderer) {
        renderer->shutdown();
    }
}

// Main application loop tick: handles input, background solver checks, and rendering
bool InteractiveSolver::update() {
    // Handle input
    handleUserInput();
    
    // Check if background solver finished
    if (mode == ApplicationMode::SOLVING && !isSolving) {
        if (solveThread.joinable()) {
            solveThread.join();
        }
        
        if (solveSuccess) {
            std::cout << "Solution found! Playing sequence..." << std::endl;
            
            // Transfer moves from thread-safe queue to sequence
            std::vector<std::pair<Face, MoveType>> moves;
            std::stringstream ss;
            
            auto moveToString = [](Face f, MoveType t) -> std::string {
                std::string s;
                switch(f) {
                    case Face::U: s = "U"; break; case Face::D: s = "D"; break;
                    case Face::L: s = "L"; break; case Face::R: s = "R"; break;
                    case Face::F: s = "F"; break; case Face::B: s = "B"; break;
                }
                if (t == MoveType::Prime) s += "'";
                if (t == MoveType::Double) s += "2";
                return s;
            };

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                ss << "Solution (" << solutionQueue.size() << " moves): ";
                while (!solutionQueue.empty()) {
                    auto m = solutionQueue.front();
                    moves.push_back(m);
                    ss << moveToString(m.first, m.second) << " ";
                    solutionQueue.pop();
                }
            }
            renderer->setSolutionText(ss.str());
            playSequence(moves);
        } else {
            std::cerr << "Failed to find solution" << std::endl;
            mode = ApplicationMode::IDLE;
        }
    }
    
    // Update animation
    updateAnimation();
    
    // Render
    bool windowOpen = renderer->render(cube);
    
    return windowOpen;
}

void InteractiveSolver::scramble(int numMoves) {
    std::cout << "Scrambling with " << numMoves << " random moves..." << std::endl;
    
    std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> faceDist(0, 5);
    std::uniform_int_distribution<int> typeDist(0, 2);
    
    for (int i = 0; i < numMoves; ++i) {
        Face face = static_cast<Face>(faceDist(rng));
        MoveType type = static_cast<MoveType>(typeDist(rng));
        cube.applyMove(face, type);
        renderer->queueAnimation(face, type);
    }
    
    std::cout << "Cube scrambled. Difficulty: " << numMoves << " moves" << std::endl;
}

// Launches the currently selected solver algorithm in a background thread
void InteractiveSolver::solve() {
    if (cube.isSolved()) {
        std::cout << "Cube already solved!" << std::endl;
        return;
    }
    
    if (mode == ApplicationMode::SOLVING) {
        std::cout << "Solver already running..." << std::endl;
        return;
    }
    
    mode = ApplicationMode::SOLVING;
    
    Solver* solver = getActiveSolver();
    if (!solver) {
        std::cerr << "No solver available" << std::endl;
        mode = ApplicationMode::IDLE;
        return;
    }
    
    // If the selected solver isn't ready (missing PDB files), warn and use it anyway
    // (the HybridSolver and TwoPhase have built-in fallback heuristics)
    if (!solver->isReady()) {
        std::cout << "[WARNING] " << solver->getName() << " databases not loaded." << std::endl;
        std::cout << "          Solving with built-in heuristics (may be slower)..." << std::endl;
    }
    
    std::cout << "Solving with " << solver->getName() << " in background thread..." << std::endl;
    solveStartTime = std::chrono::high_resolution_clock::now();
    isSolving = true;
    solveSuccess = false;
    
    // Launch solver in background thread
    RubiksCube capturedCube = cube;
    if (solveThread.joinable()) {
        solveThread.join();
    }
    
    solveThread = std::thread([this, solver, capturedCube]() {
        SolutionResult result = solver->solve(capturedCube);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - solveStartTime);
        
        if (result.found) {
            std::cout << "\nSolution found in " << result.moveCount << " moves" << std::endl;
            std::cout << "Time: " << elapsed.count() << "ms" << std::endl;
            std::cout << "Nodes explored: " << result.nodesExplored << std::endl;
            
            lastSolveStats.moveCount = result.moveCount;
            lastSolveStats.solveTime = elapsed.count() / 1000.0f;
            lastSolveStats.nodesExplored = result.nodesExplored;
            lastSolveStats.solverUsed = solver->getName();
            
            // Push sequence to thread-safe queue
            std::lock_guard<std::mutex> lock(queueMutex);
            for (const auto& move : result.moves) {
                solutionQueue.push(move);
            }
            solveSuccess = true;
        } else {
            std::cerr << "\nSolver failed or timed out after " << elapsed.count() << "ms" << std::endl;
            std::cerr << "Try pressing [G] to reset, then [S] to scramble with fewer moves." << std::endl;
            solveSuccess = false;
        }
        
        isSolving = false;
    });
}

// Resets the cube to its fully solved state and clears animation sequences
void InteractiveSolver::reset() {
    cube = RubiksCube();
    mode = ApplicationMode::IDLE;
    currentSequence.clear();
    sequenceIndex = 0;
    
    // Reset all cubie transforms back to identity grid positions
    int id = 0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z) {
                renderer->renderCubies[id].gridX = x;
                renderer->renderCubies[id].gridY = y;
                renderer->renderCubies[id].gridZ = z;
                renderer->renderCubies[id].baseTransform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
                id++;
            }
    renderer->animQueue.clear();
    renderer->setSolutionText("");
    
    std::cout << "Cube reset to solved state" << std::endl;
}

// Automatically generates or loads pattern databases for IDA* and Hybrid solvers
bool InteractiveSolver::loadSolverDatabases(const std::string& dataDirectory) {
    std::cout << "Loading solver databases from " << dataDirectory << std::endl;
    
    bool success = true;
    
    if (auto* korf = dynamic_cast<KorfSolver*>(idaStarSolver.get())) {
        if (!korf->loadDatabases(dataDirectory)) {
            std::cerr << "Failed to load IDA* databases. Generating databases..." << std::endl;
            if (!korf->generateDatabases(dataDirectory)) {
                success = false;
            }
        }
    }
    
    if (auto* hybrid = dynamic_cast<HybridSolver*>(hybridSolver.get())) {
        if (!hybrid->loadDatabases(dataDirectory)) {
            std::cerr << "Failed to load Hybrid solver databases. Generating databases..." << std::endl;
            if (!hybrid->generateDatabases(dataDirectory)) {
                success = false;
            }
        }
    }
    
    return success;
}

void InteractiveSolver::printHelp() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  RUBIK'S CUBE SOLVER - INTERACTIVE MODE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "\nCube Moves (keyboard):" << std::endl;
    std::cout << "  [U/D/L/R/F/B]         : Clockwise face rotation" << std::endl;
    std::cout << "  [Shift + face key]     : Counter-clockwise (prime)" << std::endl;
    std::cout << "  [Ctrl  + face key]     : Double rotation" << std::endl;
    
    std::cout << "\nActions:" << std::endl;
    std::cout << "  [SPACE]               : Solve cube" << std::endl;
    std::cout << "  [S]                   : Scramble (7 moves)" << std::endl;
    std::cout << "  [G]                   : Reset to solved" << std::endl;
    std::cout << "  [ESC]                 : Exit" << std::endl;
    
    std::cout << "\nCamera:" << std::endl;
    std::cout << "  [Mouse drag]          : Orbit camera" << std::endl;
    
    std::cout << "\nSolver Selection:" << std::endl;
    std::cout << "  [1]                   : IDA* (Korf)" << std::endl;
    std::cout << "  [2]                   : Hybrid (Two-Phase + Korf)" << std::endl;
    std::cout << "  [3]                   : Simple IDDFS" << std::endl;
    
    std::cout << "\n" << std::string(60, '=') << "\n" << std::endl;
}

// Polls the renderer for any user-inputted moves (via keyboard) and applies them
void InteractiveSolver::handleUserInput() {
    auto moves = renderer->getQueuedMoves();
    for (const auto& move : moves) {
        cube.applyMove(move.first, move.second);
        renderer->queueAnimation(move.first, move.second);
    }
}

// Updates the continuous animation sequence, applying one move per set duration
void InteractiveSolver::updateAnimation() {
    if (currentSequence.empty() || !autoPlayAnimation) {
        return;
    }
    
    moveAnimationTime += 0.016f;  // ~60 FPS delta
    
    if (moveAnimationTime >= moveAnimationDuration) {
        applyNextMove();
        moveAnimationTime = 0.0f;
        
        if (sequenceIndex >= currentSequence.size()) {
            mode = ApplicationMode::IDLE;
            currentSequence.clear();
            sequenceIndex = 0;
            
            if (cube.isSolved()) {
                std::cout << "Solution complete!" << std::endl;
            }
        }
    }
}

Solver* InteractiveSolver::getActiveSolver() {
    switch (activeSolver) {
        case SolverAlgorithm::IDA_STAR:
            return idaStarSolver.get();
        case SolverAlgorithm::HYBRID:
            return hybridSolver.get();
        case SolverAlgorithm::SIMPLE_IDDFS:
            return simpleIDDFSSolver.get();
        default:
            return nullptr;
    }
}

void InteractiveSolver::playSequence(const std::vector<std::pair<Face, MoveType>>& moves) {
    currentSequence = moves;
    sequenceIndex = 0;
    moveAnimationTime = 0.0f;
    mode = ApplicationMode::SOLVING_ANIMATED;
    autoPlayAnimation = true;
    
    // Push all solution moves into the renderer's animation queue
    for (const auto& m : moves) {
        renderer->queueAnimation(m.first, m.second);
    }
}

void InteractiveSolver::applyNextMove() {
    if (sequenceIndex < currentSequence.size()) {
        const auto& move = currentSequence[sequenceIndex];
        cube.applyMove(move.first, move.second);
        sequenceIndex++;
    }
}

} // namespace cube_solver
