/**
 * @file main.cpp
 * @brief Entry point and demonstration of Rubik's Cube solver
 * 
 * This demonstrates:
 * 1. Basic cube manipulation (applying moves)
 * 2. Cube state queries
 * 3. Running simple solver (IDDFS - useful for testing)
 * 4. Integration with pattern databases (once implemented)
 */

#include <iostream>
#include <memory>
#include "cube/RubiksCube.h"
#include "solver/Solver.h"

using namespace cube_solver;

/**
 * @brief Print welcome message and usage information
 */
void printWelcome() {
    std::cout << "=================================" << std::endl;
    std::cout << "  Rubik's Cube Solver in C++" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << std::endl;
    std::cout << "This program solves Rubik's Cube using:" << std::endl;
    std::cout << "  - Core algorithm: IDA* (Iterative-Deepening A*)" << std::endl;
    std::cout << "  - Heuristic: Pattern Databases (Korf's approach)" << std::endl;
    std::cout << "  - Optimization: Symmetry reduction and subgroup decomposition" << std::endl;
    std::cout << std::endl;
}

/**
 * @brief Demonstrate basic cube operations
 */
void demonstrateBasicOperations() {
    std::cout << "=== Basic Cube Operations ===" << std::endl;
    
    // Create solved cube
    RubiksCube cube;
    std::cout << "Created solved cube" << std::endl;
    std::cout << "Is solved? " << (cube.isSolved() ? "YES" : "NO") << std::endl;
    
    // Apply some moves
    std::cout << "\nApplying moves: R U R' U'" << std::endl;
    if (cube.applyMoveSequence("R U R' U'")) {
        std::cout << "Moves applied successfully" << std::endl;
        std::cout << "Is solved? " << (cube.isSolved() ? "YES" : "NO") << std::endl;
        std::cout << "Move history: " << cube.getMoveHistoryString() << std::endl;
        std::cout << "Move count: " << cube.getMoveCount() << std::endl;
    } else {
        std::cout << "Error parsing moves" << std::endl;
    }
    
    std::cout << std::endl;
}

/**
 * @brief Demonstrate scrambling and solving
 */
void demonstrateSolving() {
    std::cout << "=== Scramble and Solve ===" << std::endl;
    
    // Create and scramble cube
    RubiksCube cube;
    std::cout << "Scrambling with 10 random moves..." << std::endl;
    cube.scramble(10);
    std::cout << "Scramble: " << cube.getMoveHistoryString() << std::endl;
    std::cout << "Is solved? " << (cube.isSolved() ? "YES" : "NO") << std::endl;
    
    // Solve with simple IDDFS (for testing - slow!)
    std::cout << "\nSolving with Simple IDDFS (no heuristic)..." << std::endl;
    SimpleIDDFSSolver simpleSolver;
    
    SolutionResult result = simpleSolver.solve(cube);
    
    if (result.found) {
        std::cout << "Solution FOUND!" << std::endl;
        std::cout << "Moves: " << result.moveCount << std::endl;
        std::cout << "Time: " << result.solveTime.count() << " ms" << std::endl;
        std::cout << "Nodes explored: " << result.nodesExplored << std::endl;
    } else {
        std::cout << "No solution found (may require deeper search)" << std::endl;
    }
    
    std::cout << std::endl;
}

/**
 * @brief Demonstrate pattern database setup (once implemented)
 */
void demonstratePatternDatabases() {
    std::cout << "=== Pattern Databases ===" << std::endl;
    std::cout << "Pattern databases are used for fast heuristic computation." << std::endl;
    std::cout << std::endl;
    std::cout << "Required databases:" << std::endl;
    std::cout << "  1. Corner PDB: 8! * 3^7 = 88,179,840 states (~42 MB)" << std::endl;
    std::cout << "  2. Edge 7-piece PDB 1: 12P7 * 2^7 = 511,075,840 states (~244 MB)" << std::endl;
    std::cout << "  3. Edge 7-piece PDB 2: 12P7 * 2^7 = 511,075,840 states (~244 MB)" << std::endl;
    std::cout << "  4. Edge Permutation PDB: 12!/2 = 239,500,800 states (~228 MB)" << std::endl;
    std::cout << "  Total: ~758 MB (can be reduced to ~50 MB with symmetry reduction!)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Optimizations to implement:" << std::endl;
    std::cout << "  - Nibble compression: pack 2 distances per byte" << std::endl;
    std::cout << "  - Symmetry reduction: leverage 48 cube symmetries" << std::endl;
    std::cout << "  - Two-phase algorithm: divide into G0→G1→G2→G3→solved" << std::endl;
    std::cout << std::endl;
}

/**
 * @brief Print project structure and next steps
 */
void printNextSteps() {
    std::cout << "=== NEXT STEPS FOR DEVELOPMENT ===" << std::endl;
    std::cout << "1. Implement CubieCube::applySingleMove() for all faces (U,L,F,R,B,D)" << std::endl;
    std::cout << "   - Use cycle notation for corner/edge movements" << std::endl;
    std::cout << "   - Update orientations based on face type" << std::endl;
    std::cout << std::endl;
    std::cout << "2. Implement RankCalculator for state→index conversion" << std::endl;
    std::cout << "   - rankPermutation() using Lehmer code" << std::endl;
    std::cout << "   - rankOrientations() using base-N representation" << std::endl;
    std::cout << "   - rankPartialPermutation() for 7-edge subsets (complex!)" << std::endl;
    std::cout << std::endl;
    std::cout << "3. Implement PatternDatabase generation via BFS" << std::endl;
    std::cout << "   - Start from solved state" << std::endl;
    std::cout << "   - Apply all 18 moves to generate successors" << std::endl;
    std::cout << "   - Mark distances and save to disk" << std::endl;
    std::cout << std::endl;
    std::cout << "4. Implement KorfSolver::idaSearch() IDA* algorithm" << std::endl;
    std::cout << "   - Threshold = g(n) + h(n)" << std::endl;
    std::cout << "   - Do depth-first with pruning" << std::endl;
    std::cout << "   - Increase threshold iteratively" << std::endl;
    std::cout << std::endl;
    std::cout << "5. Optional: Implement advanced optimizations" << std::endl;
    std::cout << "   - SymmetryReducedPatternDatabase (48x memory savings!)" << std::endl;
    std::cout << "   - TwoPhaseAlgorithm (Kociemba subgroup decomposition)" << std::endl;
    std::cout << "   - Move pruning table generation" << std::endl;
    std::cout << std::endl;
}

/**
 * @brief Main entry point
 */
int main() {
    try {
        printWelcome();
        
        // Demonstrate basic operations
        demonstrateBasicOperations();
        
        // Demonstrate scrambling and solving (COMMENTED OUT to skip long Simple IDDFS)
        // demonstrateSolving();
        
        // Show PDB information
        demonstratePatternDatabases();
        
        // Print next development steps
        printNextSteps();
        
        std::cout << "Program completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
