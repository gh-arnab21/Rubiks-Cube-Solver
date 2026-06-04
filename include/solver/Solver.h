#ifndef SOLVER_H
#define SOLVER_H

#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>

/**
 * @file Solver.h
 * @brief Abstract solver interface and implementations
 * 
 * Solver implementations:
 * 1. KorfSolver: IDA* with pattern databases (optimal, can be slow)
 * 2. TwoPhaseAlgorithm: Kociemba-style subgroup approach (faster, near-optimal)
 * 3. SimpleIDDFS: Basic depth-first search (for testing, very slow)
 * 
 * Common elements:
 * - All return solution as sequence of moves
 * - All use heuristics to prune search space
 * - All track nodes explored, time taken, etc.
 */

#include "cube/RubiksCube.h"
#include "database/PatternDatabase.h"

namespace cube_solver {

/**
 * @struct SolverStatistics
 * @brief Detailed solver statistics
 */
struct SolverStatistics {
    uint64_t nodesExplored = 0;      ///< Total nodes visited
    uint64_t nodesExpanded = 0;      ///< Nodes with children generated
    uint64_t pdbLookups = 0;         ///< Pattern database queries
    uint32_t maxDepthReached = 0;    ///< Deepest level in search
    std::vector<uint32_t> depthCounts;  ///< Nodes at each depth
    bool timedOut = false;           ///< Whether search hit timeout
    
    std::string toString() const;
};

/**
 * @struct SolutionResult
 * @brief Result of a solve attempt
 */
struct SolutionResult {
    bool found = false;                           ///< Whether solution was found
    std::vector<std::pair<Face, MoveType>> moves;  ///< Solution move sequence
    uint32_t moveCount = 0;                   ///< Number of moves in solution
    uint64_t nodesExplored = 0;              ///< For statistics
    std::chrono::milliseconds solveTime = std::chrono::milliseconds(0);  ///< Wall time to solve
    SolverStatistics stats;              ///< Detailed statistics
    
    std::string toString() const;
};

/**
 * @class Solver
 * @brief Abstract base class for cube solving algorithms
 */
class Solver {
public:
    virtual ~Solver() = default;
    
    /**
     * @brief Solve a scrambled cube
     * @param cube The cube to solve
     * @return Solution result (moves, statistics)
     */
    virtual SolutionResult solve(const RubiksCube& cube) = 0;
    
    /**
     * @brief Check if solver is ready (databases loaded, etc.)
     */
    virtual bool isReady() const = 0;
    
    /**
     * @brief Get human-readable name of solver
     */
    virtual std::string getName() const = 0;
};

/**
 * @class KorfSolver
 * @brief Richard Korf's optimal solver using IDA*
 * 
 * Algorithm overview:
 * - Iterative-Deepening A* (IDA*)
 * - Uses multiple pattern databases as admissible heuristics
 * - Guarantees optimal solution (minimum moves)
 * 
 * Pattern databases used:
 * 1. Corner PDB: 8! * 3^7 states (~42MB)
 * 2. Edge PDB 1: 7 edges (indices 0-6) from 12 positions (~244MB)
 * 3. Edge PDB 2: 7 edges (indices 6-11) from 12 positions (~244MB)
 * 4. Edge Permutation PDB: all 12 edges, permutation only (~228MB)
 * 
 * Total: ~758MB (can optimize with symmetry reduction)
 * 
 * Search strategy:
 * - Start with f(node) = g(node) + h(node) as depth threshold
 * - Do depth-first search pruning nodes where f exceeds threshold
 * - Find minimum f value that exceeded threshold
 * - Increase threshold and repeat
 * 
 * Optimizations:
 * - Max distance is known to be ≤ 20 moves
 * - Leaf nodes sorted by f-value (best-first within deepening)
 * - Move pruning (redundant move elimination)
 * - Non-recursive implementation
 */
class KorfSolver : public Solver {
public:
    /**
     * @brief Initialize Korf solver
     * @param useSymmetryReduction Whether to use memory-optimized PDBs
     * 
     * If useSymmetryReduction is true, requires implementation of
     * SymmetryReducedPatternDatabase (TODO item).
     */
    explicit KorfSolver(bool useSymmetryReduction = false);
    ~KorfSolver() override = default;
    
    /**
     * @brief Load pattern databases from files
     * @param dataDirectory Directory containing .pdb files
     * @return true if all databases loaded successfully
     */
    bool loadDatabases(const std::string& dataDirectory);
    
    /**
     * @brief Generate pattern databases from scratch
     * @return true if generation succeeded
     * 
     * Warning: Very time-consuming! Can take hours for all databases.
     * Databases are saved to disk after generation.
     */
    bool generateDatabases(const std::string& dataDirectory);
    
    SolutionResult solve(const RubiksCube& cube) override;
    bool isReady() const override { return databasesLoaded; }
    std::string getName() const override { return "Korf IDA*"; }
    
private:
    // ============ INTERNAL STATE ============
    
    std::unique_ptr<PatternDatabase> cornerPDB;
    std::unique_ptr<PatternDatabase> edgePDB1;  // 7 edges, indices 0-6
    std::unique_ptr<PatternDatabase> edgePDB2;  // 7 edges, indices 6-11
    std::unique_ptr<PatternDatabase> edgePermPDB;  // 12 edges permutation only
    
    bool databasesLoaded;
    bool useSymmetryReduction;
    
    // ============ SEARCH IMPLEMENTATION ============
    
    /**
     * @brief IDA* search function
     * @param cube Current cube state
     * @param gValue Moves applied so far (depth)
     * @param threshold Current f-value threshold
     * @param solution Output parameter for solution moves
     * @param lastMove Previous move (for pruning)
     * @return Minimum f-value that exceeded threshold, or 0 if solution found
     * 
     * Recursive implementation:
     * 1. Compute f(n) = g(n) + h(n)
     * 2. If f(n) > threshold, return f(n) (pruned)
     * 3. If cube solved, add to solution and return 0
     * 4. For each valid next move:
     *    - Recurse with increased depth
     *    - Track minimum f exceeded
     * 5. Return minimum f exceeded
     * 
     * Non-recursive variant (preferred):
     * - Use explicit stack instead of recursion
     * - Better cache locality and control
     */
    uint32_t idaSearch(const RubiksCube& cube,
                      uint32_t threshold,
                      std::vector<std::pair<Face, MoveType>>& solution);
    
    /**
     * @brief Compute heuristic value for a cube state
     * @return Estimated distance to solved state
     * 
     * Implementation:
     * - Get corner PDB value
     * - Get edge PDB 1 value
     * - Get edge PDB 2 value
     * - Return maximum (additive heuristics must be used carefully!)
     * 
     * Note: Can't simply add PDB values as they overlap!
     * Some approaches:
     * 1. Use max of independent PDBs
     * 2. Use weighted sum
     * 3. Use sophisticated combination (Korf mentions this)
     */
    uint8_t getHeuristic(const RubiksCube& cube) const;
    
};

/**
 * @class TwoPhaseAlgorithm
 * @brief Kociemba's Two-Phase Algorithm
 * 
 * More complex algorithm that divides solving into two phases:
 * 
 * Phase 1: Move to Group 1
 * - Ensure all edge orientations correct
 * - Move some edges to specific slices
 * - Uses smaller PDBs specific to this phase
 * 
 * Phase 2: Solve from Group 1
 * - Fewer possible moves (like last layer solving)
 * - Uses different PDBs
 * 
 * Benefits:
 * - Often faster than single IDA*
 * - Uses less memory (phase-specific databases are smaller)
 * - Can solve hard cubes more efficiently
 * 
 * TODO: Implement after core IDA* is working
 */
class TwoPhaseAlgorithm : public Solver {
public:
    TwoPhaseAlgorithm();
    ~TwoPhaseAlgorithm() override = default;
    
    bool loadDatabases(const std::string& dataDirectory);
    bool generateDatabases(const std::string& dataDirectory);
    
    SolutionResult solve(const RubiksCube& cube) override;
    bool isReady() const override { return false; }  // TODO: implement
    std::string getName() const override { return "Kociemba Two-Phase"; }
    
private:
    // TODO: Implement phase-specific databases and search
};

/**
 * @class SimpleIDDFSSolver
 * @brief Simple iterative-deepening depth-first search (no heuristic)
 * 
 * Used for:
 * - Testing that core cube logic works
 * - Baseline performance comparison
 * - Solving simple scrambles (probably won't finish for hard ones)
 * 
 * Much slower than IDA* but doesn't require PDB generation.
 */
class SimpleIDDFSSolver : public Solver {
public:
    SimpleIDDFSSolver() = default;
    ~SimpleIDDFSSolver() override = default;
    
    SolutionResult solve(const RubiksCube& cube) override;
    bool isReady() const override { return true; }  // No setup needed
    std::string getName() const override { return "Simple IDDFS"; }
    
private:
    // Maximum depth to search (safeguard against infinite search)
    static constexpr uint32_t MAX_DEPTH = 20;
    
    /**
     * @brief Simple IDDFS with no heuristic
     */
    bool searchDepth(RubiksCube cube,
                    uint32_t currentDepth,
                    uint32_t maxDepth,
                    Face lastFace,
                    std::vector<std::pair<Face, MoveType>>& solution);
};

} // namespace cube_solver

#endif // SOLVER_H