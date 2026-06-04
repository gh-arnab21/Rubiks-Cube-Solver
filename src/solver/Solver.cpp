#include "solver/Solver.h"
#include "solver/MovePruning.h"
#include "database/RankCalculator.h"
#include <iostream>
#include <chrono>
#include <queue>
#include <stack>
#include <set>
#include <string>
#include <algorithm>
#include <utility>
#include <filesystem>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace cube_solver {

// ============ SOLUTION RESULT ============

std::string SolutionResult::toString() const {
    std::string result;
    result += "Solution found: " + std::string(found ? "YES" : "NO") + "\n";
    result += "Moves: " + std::to_string(moveCount) + "\n";
    result += "Nodes explored: " + std::to_string(nodesExplored) + "\n";
    result += "Time: " + std::to_string(solveTime.count()) + "ms\n";
    return result;
}

// ============ KORF SOLVER ============

KorfSolver::KorfSolver(bool useSymmetryReduction)
    : databasesLoaded(false), useSymmetryReduction(useSymmetryReduction) {
    // Initialize solver with pattern database instances
    cornerPDB = std::make_unique<PatternDatabase>(PatternDatabase::Type::CORNER_PIECES);
    edgePDB1 = std::make_unique<PatternDatabase>(PatternDatabase::Type::EDGE_GROUP_1);
    edgePDB2 = std::make_unique<PatternDatabase>(PatternDatabase::Type::EDGE_GROUP_2);
    edgePermPDB = std::make_unique<PatternDatabase>(PatternDatabase::Type::EDGE_PERMUTATION);
    
    if (useSymmetryReduction) {
        cornerPDB->setSymmetryReduction(true);
    }
}

// Loads pattern databases from disk, required for IDA* heuristic estimation
bool KorfSolver::loadDatabases(const std::string& dataDirectory) {
    // TODO: Load all pattern databases from disk
    // Format: dataDirectory/corner.pdb, edge1.pdb, edge2.pdb, edgeperm.pdb
    
    std::cout << "Loading pattern databases from " << dataDirectory << std::endl;
    
    if (!cornerPDB->loadFromFile(dataDirectory + "/corner.pdb")) {
        std::cerr << "Failed to load corner database" << std::endl;
        return false;
    }
    
    if (edgePDB1 && !edgePDB1->loadFromFile(dataDirectory + "/edge1.pdb")) {
        std::cerr << "Failed to load edge1 database" << std::endl;
        return false;
    }
    
    if (edgePDB2 && !edgePDB2->loadFromFile(dataDirectory + "/edge2.pdb")) {
        std::cerr << "Failed to load edge2 database" << std::endl;
        return false;
    }
    
    if (edgePermPDB && !edgePermPDB->loadFromFile(dataDirectory + "/edgeperm.pdb")) {
        std::cerr << "Failed to load edge permutation database" << std::endl;
        return false;
    }
    
    databasesLoaded = true;
    return true;
}

// Generates and saves all pattern databases (Corner, Edges) via BFS
bool KorfSolver::generateDatabases(const std::string& dataDirectory) {
    std::cout << "Generating pattern databases (sequentially for compatibility)..." << std::endl;
    
    std::filesystem::create_directories(dataDirectory);
    
    bool allSuccess = true;
    
    if (cornerPDB) {
        std::cout << "Generating corner PDB..." << std::endl;
        if (!cornerPDB->generate()) {
            allSuccess = false;
        } else {
            cornerPDB->saveToFile(dataDirectory + "/corner.pdb");
        }
    }
    if (edgePDB1) {
        std::cout << "Generating edge PDB 1..." << std::endl;
        if (!edgePDB1->generate()) {
            allSuccess = false;
        } else {
            edgePDB1->saveToFile(dataDirectory + "/edge1.pdb");
        }
    }
    if (edgePDB2) {
        std::cout << "Generating edge PDB 2..." << std::endl;
        if (!edgePDB2->generate()) {
            allSuccess = false;
        } else {
            edgePDB2->saveToFile(dataDirectory + "/edge2.pdb");
        }
    }
    if (edgePermPDB) {
        std::cout << "Generating edge permutation PDB..." << std::endl;
        if (!edgePermPDB->generate()) {
            allSuccess = false;
        } else {
            edgePermPDB->saveToFile(dataDirectory + "/edgeperm.pdb");
        }
    }
    
    if (allSuccess) {
        std::cout << "All databases generated successfully" << std::endl;
        databasesLoaded = true;
    } else {
        std::cerr << "Failed to generate one or more databases" << std::endl;
    }
    
    return allSuccess;
}

// Executes Iterative Deepening A* (IDA*) search to find the optimal (shortest) solution
SolutionResult KorfSolver::solve(const RubiksCube& cube) {
    // TODO: Implement IDA* search
    // 
    // Algorithm:
    // 1. Start with threshold = h(initial state)
    // 2. Do depth-first search with f(n) <= threshold
    // 3. Find minimum f value that exceeded threshold
    // 4. If found solution, return it
    // 5. Set threshold to minimum f exceeded
    // 6. Repeat until solution found
    
    SolutionResult result;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (!isReady()) {
        std::cerr << "Korf solver not ready - load databases first" << std::endl;
        result.found = false;
        return result;
    }
    
    RubiksCube workingCube = cube;
    std::vector<std::pair<Face, MoveType>> solution;
    uint32_t threshold = getHeuristic(workingCube);
    uint32_t minExceeded;
    result.nodesExplored = 0;
    
    while (true) {
        minExceeded = idaSearch(workingCube, threshold, solution);
        
        if (minExceeded == 0) {
            // Solution found!
            result.found = true;
            result.moves = solution;
            result.moveCount = solution.size();
            break;
        }
        
        if (minExceeded == UINT32_MAX) {
            // No solution within depth limit (or bad DB)
            result.found = false;
            break;
        }
        
        threshold = minExceeded;
        
        // Safety check: if threshold exceeds known maximum (20 moves)
        if (threshold > 20) {
            result.found = false;
            break;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.solveTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    return result;
}

// Depth-limited search for IDA*, explores branches until the f-value exceeds the threshold
uint32_t KorfSolver::idaSearch(const RubiksCube& cube,
                               uint32_t threshold,
                               std::vector<std::pair<Face, MoveType>>& solution) {
    
    if (cube.isSolved()) return 0;

    struct PrioritizedMove {
        RubiksCube nextCube;
        Face face;
        MoveType moveType;
        uint32_t fValue;
    };
    
    std::vector<PrioritizedMove> initialMoves;
    uint32_t globalNextBound = UINT32_MAX;
    
    // 1. Generate root children
    for (int faceInt = 0; faceInt < 6; ++faceInt) {
        Face face = static_cast<Face>(faceInt);
        for (int typeInt = 0; typeInt < 3; ++typeInt) {
            MoveType type = static_cast<MoveType>(typeInt);
            
            RubiksCube nextCube = cube;
            nextCube.applyMove(face, type);
            
            uint32_t estMoves = getHeuristic(nextCube);
            uint32_t fValue = 1 + estMoves;
            
            if (fValue <= threshold) {
                initialMoves.push_back({nextCube, face, type, fValue});
            } else if (fValue < globalNextBound) {
                globalNextBound = fValue;
            }
        }
    }
    
    // Sort so best moves are tried first
    std::sort(initialMoves.begin(), initialMoves.end(), [](const PrioritizedMove& a, const PrioritizedMove& b) {
        return a.fValue < b.fValue;
    });
    
    bool foundSolution = false;
    
    // 2. Parallel branch exploration
    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < (int)initialMoves.size(); ++i) {
        if (foundSolution) continue;
        
        uint32_t localNextBound = UINT32_MAX;
        std::vector<std::pair<Face, MoveType>> localSolution;
        bool localSolved = false;
        
        struct Node {
            RubiksCube currentCube;
            Face face;
            MoveType moveType;
            uint32_t depth;
        };
        
        std::stack<Node> nodeStack;
        nodeStack.push({initialMoves[i].nextCube, initialMoves[i].face, initialMoves[i].moveType, 1});
        
        std::vector<std::pair<Face, MoveType>> currentMoves(25);
        currentMoves[0] = {initialMoves[i].face, initialMoves[i].moveType};
        
        while (!nodeStack.empty() && !foundSolution) {
            Node curNode = nodeStack.top();
            nodeStack.pop();
            
            currentMoves[curNode.depth - 1] = {curNode.face, curNode.moveType};
            
            if (curNode.depth == threshold) {
                if (curNode.currentCube.isSolved()) {
                    localSolution.clear();
                    for (uint32_t d = 0; d < curNode.depth; ++d) {
                        localSolution.push_back(currentMoves[d]);
                    }
                    localSolved = true;
                    break;
                }
            } else {
                Face lastFace = curNode.face;
                MoveType lastType = curNode.moveType;
                
                struct LocalPrioritizedMove {
                    RubiksCube nextCube;
                    Face face;
                    MoveType moveType;
                    uint32_t fValue;
                    bool operator>(const LocalPrioritizedMove& other) const {
                        return fValue > other.fValue;
                    }
                };
                
                std::priority_queue<LocalPrioritizedMove, std::vector<LocalPrioritizedMove>, std::greater<LocalPrioritizedMove>> successors;
                
                for (int fInt = 0; fInt < 6; ++fInt) {
                    Face face = static_cast<Face>(fInt);
                    for (int tInt = 0; tInt < 3; ++tInt) {
                        MoveType type = static_cast<MoveType>(tInt);
                        
                        if (!MovePruning::isValidMoveSequence(lastFace, lastType, face, type)) {
                            continue;
                        }
                        
                        RubiksCube nextCube = curNode.currentCube;
                        nextCube.applyMove(face, type);
                        
                        uint32_t estMoves = getHeuristic(nextCube);
                        uint32_t fValue = curNode.depth + 1 + estMoves;
                        
                        if (fValue <= threshold) {
                            successors.push({nextCube, face, type, fValue});
                        } else if (fValue < localNextBound) {
                            localNextBound = fValue;
                        }
                    }
                }
                
                while (!successors.empty()) {
                    const auto& succ = successors.top();
                    nodeStack.push({succ.nextCube, succ.face, succ.moveType, curNode.depth + 1});
                    successors.pop();
                }
            }
        }
        
        if (localSolved) {
            #pragma omp critical
            {
                if (!foundSolution) {
                    solution = localSolution;
                    foundSolution = true;
                }
            }
        } else {
            #pragma omp critical
            {
                if (localNextBound < globalNextBound) {
                    globalNextBound = localNextBound;
                }
            }
        }
    }
    
    if (foundSolution) return 0;
    return globalNextBound;
}

// Computes the maximum admissible heuristic from all available pattern databases
uint8_t KorfSolver::getHeuristic(const RubiksCube& cube) const {
    // TODO: Combine heuristics from multiple PDBs
    // 
    // Approach:
    // Use maximum of independent PDBs:
    // h(n) = max(cornerPDB[n], edgePDB1[n], edgePDB2[n], ...)
    // 
    // Be careful: can't naively add PDBs as they overlap!
    // Some advanced techniques use weighted sums or other combinations.
    
    uint8_t h = 0;
    
    if (cornerPDB && cornerPDB->isValid()) {
        h = std::max(h, cornerPDB->lookup(cube.getState()));
    }
    
    if (edgePDB1 && edgePDB1->isValid()) {
        h = std::max(h, edgePDB1->lookup(cube.getState()));
    }
    
    if (edgePDB2 && edgePDB2->isValid()) {
        h = std::max(h, edgePDB2->lookup(cube.getState()));
    }
    
    return h;
}



// ============ TWO-PHASE ALGORITHM ============

TwoPhaseAlgorithm::TwoPhaseAlgorithm() {
    // Phase 1 and Phase 2 PDBs would be initialized here
}

bool TwoPhaseAlgorithm::loadDatabases(const std::string& dataDirectory) {
    // Load Phase 1 (Orientation & Slice) and Phase 2 (Permutation) PDBs
    return false;
}

bool TwoPhaseAlgorithm::generateDatabases(const std::string& dataDirectory) {
    // Generate Phase 1 and Phase 2 PDBs
    return false;
}

SolutionResult TwoPhaseAlgorithm::solve(const RubiksCube& cube) {
    SolutionResult result;
    result.found = false;
    
    // Kociemba Two-Phase Algorithm Outline:
    // Phase 1: Reach subgroup G1 = <U, D, R2, L2, F2, B2>
    // - All corner orientations = 0
    // - All edge orientations = 0
    // - UD slice edges (FR, FL, BR, BL) are in their correct slice
    
    // Phase 2: Solve from G1 using restricted moves
    // - Only allow U, U', U2, D, D', D2, R2, L2, F2, B2
    
    // (This requires specific Phase 1 and Phase 2 Pattern Databases to be fast,
    // otherwise it defaults to IDDFS which is too slow).
    
    return result;
}

// ============ SIMPLE IDDFS SOLVER ============

SolutionResult SimpleIDDFSSolver::solve(const RubiksCube& cube) {
    // TODO: Implement simple IDDFS (no heuristic)
    
    SolutionResult result;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Try increasing depth limits
    for (uint32_t maxDepth = 1; maxDepth <= MAX_DEPTH; ++maxDepth) {
        RubiksCube workingCube = cube;
        std::vector<std::pair<Face, MoveType>> solution;
        
        if (searchDepth(workingCube, 0, maxDepth, Face::U, solution)) {
            result.found = true;
            result.moves = solution;
            result.moveCount = solution.size();
            break;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.solveTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    return result;
}

bool SimpleIDDFSSolver::searchDepth(RubiksCube cube,
                                    uint32_t currentDepth,
                                    uint32_t maxDepth,
                                    Face lastFace,
                                    std::vector<std::pair<Face, MoveType>>& solution) {
    // TODO: Implement depth-limited search
    
    if (cube.isSolved()) {
        return true;  // Solution found
    }
    
    if (currentDepth >= maxDepth) {
        return false;  // Depth limit reached
    }
    
    // Try all valid moves
    for (int faceInt = 0; faceInt < 6; ++faceInt) {
        Face face = static_cast<Face>(faceInt);
        
        if (face == lastFace) continue;  // Skip redundant move
        
        for (int typeInt = 0; typeInt < 3; ++typeInt) {
            MoveType moveType = static_cast<MoveType>(typeInt);
            
            RubiksCube nextCube = cube;
            nextCube.applyMove(face, moveType);
            solution.push_back({face, moveType});
            
            if (searchDepth(nextCube, currentDepth + 1, maxDepth, face, solution)) {
                return true;
            }
            
            solution.pop_back();
        }
    }
    
    return false;
}

} // namespace cube_solver