/**
 * @file InteractiveSolver.h
 * @brief Interactive UI combining solver algorithms with visual renderer
 * 
 * Features:
 * - Real-time solver integration
 * - Interactive cube manipulation via keyboard
 * - Animated solution playback
 * - Solver algorithm selection (IDA*, Two-Phase, etc.)
 * - Statistics display
 * - Performance monitoring
 */

#ifndef INTERACTIVE_SOLVER_H
#define INTERACTIVE_SOLVER_H

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <future>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include "cube/RubiksCube.h"
#include "solver/Solver.h"
#include "renderer/CubeRenderer.h"

namespace cube_solver {

/**
 * @enum ApplicationMode
 * @brief Current application mode
 */
enum class ApplicationMode {
    IDLE,              ///< Waiting for user input
    SCRAMBLING,        ///< Playing back scramble
    SOLVING,           ///< Performing solve
    SOLVING_ANIMATED,  ///< Playing back solution
    PAUSED,            ///< Paused
};

/**
 * @enum SolverAlgorithm
 * @brief Available solver algorithms
 */
enum class SolverAlgorithm {
    IDA_STAR,          ///< Korf IDA*
    HYBRID,            ///< Hybrid (Two-Phase + Korf)
    SIMPLE_IDDFS,      ///< Simple IDDFS (slow but works)
};

/**
 * @class InteractiveSolver
 * @brief Main interactive application
 */
class InteractiveSolver {
public:
    /**
     * @brief Initialize the interactive solver
     * @param width Window width
     * @param height Window height
     * @return true if successful
     */
    bool initialize(int width, int height);
    
    /**
     * @brief Shutdown and clean up
     */
    void shutdown();
    
    /**
     * @brief Main application loop
     * @return true if should continue running
     */
    bool update();
    
    /**
     * @brief Set which solver to use
     */
    void setSolver(SolverAlgorithm algo) { activeSolver = algo; }
    
    /**
     * @brief Get current cube state
     */
    const RubiksCube& getCube() const { return cube; }
    
    /**
     * @brief Scramble the cube with random moves
     */
    void scramble(int numMoves = 20);
    
    /**
     * @brief Solve the cube
     */
    void solve();
    
    /**
     * @brief Reset cube to solved state
     */
    void reset();
    
    /**
     * @brief Load solver databases
     */
    bool loadSolverDatabases(const std::string& dataDirectory);
    
    /**
     * @brief Print UI help
     */
    static void printHelp();
    
private:
    // Renderer
    std::unique_ptr<CubeRenderer> renderer;
    
    // Solvers
    std::unique_ptr<Solver> idaStarSolver;
    std::unique_ptr<Solver> hybridSolver;
    std::unique_ptr<Solver> simpleIDDFSSolver;
    
    // Current state
    RubiksCube cube;
    ApplicationMode mode = ApplicationMode::IDLE;
    SolverAlgorithm activeSolver = SolverAlgorithm::HYBRID;
    
    // Animation state
    std::vector<std::pair<Face, MoveType>> currentSequence;
    size_t sequenceIndex = 0;
    float moveAnimationTime = 0.0f;
    float moveAnimationDuration = 1.0f;
    bool autoPlayAnimation = true;
    
    // Statistics
    struct Statistics {
        uint64_t nodesExplored = 0;
        uint32_t moveCount = 0;
        float solveTime = 0.0f;
        std::string solverUsed;
    } lastSolveStats;
    
    // Async solver state
    std::thread solveThread;
    std::atomic<bool> isSolving{false};
    std::mutex queueMutex;
    std::queue<std::pair<Face, MoveType>> solutionQueue;
    std::chrono::time_point<std::chrono::high_resolution_clock> solveStartTime;
    bool solveSuccess = false;
    
    /**
     * @brief Update animations and sequences
     */
    void updateAnimation();
    
    /**
     * @brief Handle keyboard input for cube manipulation
     */
    void handleUserInput();
    
    /**
     * @brief Get active solver instance
     */
    Solver* getActiveSolver();
    
    /**
     * @brief Play back a sequence of moves
     */
    void playSequence(const std::vector<std::pair<Face, MoveType>>& moves);
    
    /**
     * @brief Apply next move in sequence
     */
    void applyNextMove();
};

} // namespace cube_solver

#endif // INTERACTIVE_SOLVER_H
