#include "solver/HybridSolver.h"
#include <iostream>

namespace cube_solver {

HybridSolver::HybridSolver(bool useSymmetryReduction) {
    twoPhaseSolver = std::make_unique<TwoPhase>();
    korfSolver = std::make_unique<KorfSolver>(useSymmetryReduction);
}

// Loads pattern databases for both Two-Phase and Korf solvers
bool HybridSolver::loadDatabases(const std::string& dataDirectory) {
    bool p1 = twoPhaseSolver->loadDatabases(dataDirectory);
    bool p2 = korfSolver->loadDatabases(dataDirectory);
    return p1 && p2;
}

// Generates missing pattern databases for both Two-Phase and Korf solvers
bool HybridSolver::generateDatabases(const std::string& dataDirectory) {
    bool p1 = twoPhaseSolver->generateDatabases(dataDirectory);
    bool p2 = korfSolver->generateDatabases(dataDirectory);
    return p1 && p2;
}

// Solves the cube by combining Two-Phase's fast G1 reduction with Korf's optimal completion
SolutionResult HybridSolver::solve(const RubiksCube& cube) {
    SolutionResult result;
    result.found = false;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // If Korf databases aren't loaded, fall back to pure Two-Phase solver
    // (which has its own built-in heuristics and doesn't need PDB files)
    if (!korfSolver->isReady()) {
        std::cout << "[HybridSolver] Korf databases not loaded, falling back to Two-Phase solver..." << std::endl;
        return twoPhaseSolver->solve(cube);
    }
    
    std::cout << "[HybridSolver] Starting Phase 1 (Two-Phase G1 reduction)..." << std::endl;
    // Step 1: Reach G1 using TwoPhase algorithm's phase 1 search
    auto phase1Moves = twoPhaseSolver->phase1Search(cube);
    
    // If phase1 returned empty and cube is not in G1, phase1 failed/timed out
    if (phase1Moves.empty() && !cube.isSolved()) {
        // Check if cube is already in G1
        RubiksCube testCube = cube;
        // Try direct Korf solve as fallback
        std::cout << "[HybridSolver] Phase 1 failed, trying direct Korf solve..." << std::endl;
        return korfSolver->solve(cube);
    }
    
    RubiksCube afterPhase1 = cube;
    for (const auto& move : phase1Moves) {
        afterPhase1.applyMove(move.first, move.second);
    }
    
    std::cout << "[HybridSolver] Phase 1 complete. Moves: " << phase1Moves.size() << std::endl;
    
    // Step 2: Solve the rest using Korf's IDA* algorithm
    std::cout << "[HybridSolver] Starting Phase 2 (Korf IDA* optimal completion)..." << std::endl;
    
    // Check if it's already solved just in case
    if (afterPhase1.isSolved()) {
        result.found = true;
        result.moves = phase1Moves;
        result.moveCount = result.moves.size();
        auto endTime = std::chrono::high_resolution_clock::now();
        result.solveTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        return result;
    }
    
    auto phase2Result = korfSolver->solve(afterPhase1);
    
    if (phase2Result.found) {
        result.found = true;
        result.moves = phase1Moves;
        result.moves.insert(result.moves.end(), phase2Result.moves.begin(), phase2Result.moves.end());
        result.moveCount = result.moves.size();
        
        result.nodesExplored = phase2Result.nodesExplored;
        result.stats = phase2Result.stats;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.solveTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    return result;
}


} // namespace cube_solver
