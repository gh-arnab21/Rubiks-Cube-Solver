/**
 * @file TwoPhase.cpp
 * @brief Two-Phase algorithm implementation
 */

#include "solver/TwoPhase.h"
#include "solver/MovePruning.h"
#include "database/RankCalculator.h"
#include <iostream>
#include <queue>
#include <stack>
#include <algorithm>
#include <filesystem>

namespace cube_solver {

TwoPhase::TwoPhase() {
    phase1PDB = std::make_unique<PatternDatabase>(PatternDatabase::Type::CORNER_PIECES);
    phase2PDB = std::make_unique<PatternDatabase>(PatternDatabase::Type::EDGE_PERMUTATION);
}

// Loads pre-computed pattern databases from disk for Phase 1 and Phase 2 heuristics
bool TwoPhase::loadDatabases(const std::string& dataDirectory) {
    std::cout << "Loading Two-Phase PDBs from " << dataDirectory << std::endl;
    
    if (!phase1PDB || !phase1PDB->loadFromFile(dataDirectory + "/phase1.pdb")) {
        std::cerr << "Failed to load phase 1 database" << std::endl;
        phase1Ready = false;
    } else {
        phase1Ready = true;
    }
    
    if (!phase2PDB || !phase2PDB->loadFromFile(dataDirectory + "/phase2.pdb")) {
        std::cerr << "Failed to load phase 2 database" << std::endl;
        phase2Ready = false;
    } else {
        phase2Ready = true;
    }
    
    return phase1Ready && phase2Ready;
}


// Automatically generates and saves Phase 1 and Phase 2 PDBs via BFS
bool TwoPhase::generateDatabases(const std::string& dataDirectory) {
    std::cout << "Generating Two-Phase PDBs..." << std::endl;
    
    std::filesystem::create_directories(dataDirectory);
    
    if (phase1PDB) {
        std::cout << "Phase 1 PDB generation..." << std::endl;
        if (!phase1PDB->generate()) {
            std::cerr << "Phase 1 PDB generation failed" << std::endl;
            phase1Ready = false;
        } else {
            phase1PDB->saveToFile(dataDirectory + "/phase1.pdb");
            phase1Ready = true;
        }
    }
    
    if (phase2PDB) {
        std::cout << "Phase 2 PDB generation..." << std::endl;
        if (!phase2PDB->generate()) {
            std::cerr << "Phase 2 PDB generation failed" << std::endl;
            phase2Ready = false;
        } else {
            phase2PDB->saveToFile(dataDirectory + "/phase2.pdb");
            phase2Ready = true;
        }
    }
    
    return phase1Ready && phase2Ready;
}

// Checks if the cube has reached the intermediate G1 state (corners and edges oriented, UD slice edges in place)
bool TwoPhase::isInGroup1(const CubieCube& cube) {
    // Check if all corners are oriented correctly
    for (int i = 0; i < 8; ++i) {
        if (cube.getCornerCubie(i).orientation != 0) {
            return false;  // Corner is twisted
        }
    }
    
    // Check if all edges are oriented correctly
    for (int i = 0; i < 12; ++i) {
        if (cube.getEdgeCubie(i).orientation != 0) {
            return false;  // Edge is flipped
        }
    }
    
    // Check if UD-slice edges are in correct slice (positions 8-11)
    // UD slice: FR(8), FL(9), BR(10), BL(11)
    for (int i = 0; i < 8; ++i) {
        const auto& edge = cube.getEdgeCubie(i);
        if (edge.position >= 8 && edge.position <= 11) {
            return false;  // An edge that should be in UD slice is elsewhere
        }
    }
    
    return true;  // In G1
}

// Searches for the shortest sequence to bring any scrambled cube into the G1 state
std::vector<std::pair<Face, MoveType>> TwoPhase::phase1Search(const RubiksCube& cube) {
    std::vector<std::pair<Face, MoveType>> solution;
    
    if (isInGroup1(cube.getState())) {
        return solution;
    }
    
    searchStartTime = std::chrono::high_resolution_clock::now();
    
    // Custom admissible heuristic for Phase 1:
    // Count twisted corners, flipped edges, and bad UD-slice edges.
    // Each move can fix at most 4 pieces of the same type.
    auto getG1Heuristic = [this](const CubieCube& state) -> uint8_t {
        if (phase1Ready && phase1PDB) {
            return phase1PDB->lookup(state);
        }
        
        int twistedCorners = 0;
        for (int i = 0; i < 8; ++i) {
            if (state.getCornerCubie(i).orientation != 0) twistedCorners++;
        }
        int flippedEdges = 0;
        for (int i = 0; i < 12; ++i) {
            if (state.getEdgeCubie(i).orientation != 0) flippedEdges++;
        }
        int badSlice = 0;
        for (int i = 0; i < 12; ++i) {
            int pos = state.getEdgeCubie(i).position;
            // UD slice edges (positions 8-11) should be in positions 8-11
            // Non-UD slice edges (positions 0-7) should be in positions 0-7
            bool isUDSliceEdge = (pos >= 8 && pos <= 11);
            bool isInUDSlice = (i >= 8 && i <= 11);
            if (isUDSliceEdge != isInUDSlice) badSlice++;
        }
        
        // Stronger heuristic: each move fixes at most 4 corners, 4 edges, or 2 slice edges
        int h = std::max({(twistedCorners + 3) / 4, (flippedEdges + 3) / 4, (badSlice + 1) / 2});
        return static_cast<uint8_t>(h);
    };
    
    struct Node {
        RubiksCube state;
        Face face;
        MoveType type;
        uint32_t depth;
    };
    
    uint32_t threshold = getG1Heuristic(cube.getState());
    std::vector<std::pair<Face, MoveType>> currentPath(20);
    
    while (threshold <= 12) { // Phase 1 is at most 12 moves
        uint32_t nextBound = UINT32_MAX;
        
        std::stack<Node> stack;
        stack.push({cube, Face::U, MoveType::Normal, 0});
        
        while (!stack.empty()) {
            if (isTimedOut()) {
                std::cerr << "Phase 1 search timed out after " << timeoutMs << "ms" << std::endl;
                return solution;  // Return empty — timed out
            }
            
            Node cur = stack.top();
            stack.pop();
            
            if (cur.depth > 0) {
                currentPath[cur.depth - 1] = {cur.face, cur.type};
            }
            
            if (cur.depth == threshold) {
                if (isInGroup1(cur.state.getState())) {
                    solution.clear();
                    for (uint32_t i = 0; i < cur.depth; ++i) {
                        solution.push_back(currentPath[i]);
                    }
                    return solution;
                }
            } else {
                Face lastFace = (cur.depth == 0) ? Face::U : cur.face;
                MoveType lastType = (cur.depth == 0) ? MoveType::Normal : cur.type;
                bool isRoot = (cur.depth == 0);
                
                for (int f = 5; f >= 0; --f) {
                    Face face = static_cast<Face>(f);
                    
                    for (int t = 2; t >= 0; --t) {
                        MoveType type = static_cast<MoveType>(t);
                        
                        if (!isRoot && !MovePruning::isValidMoveSequence(lastFace, lastType, face, type)) {
                            continue;
                        }
                        
                        RubiksCube next = cur.state;
                        next.applyMove(face, type);
                        
                        uint32_t h = getG1Heuristic(next.getState());
                        uint32_t f_val = cur.depth + 1 + h;
                        
                        if (f_val <= threshold) {
                            stack.push({next, face, type, cur.depth + 1});
                        } else if (f_val < nextBound) {
                            nextBound = f_val;
                        }
                    }
                }
            }
        }
        
        if (nextBound == UINT32_MAX) break;
        threshold = nextBound;
    }
    
    std::cerr << "Phase 1 search failed" << std::endl;
    return solution;
}

// Searches for the shortest sequence to fully solve the cube from a G1 state using restricted moves
std::vector<std::pair<Face, MoveType>> TwoPhase::phase2Search(const RubiksCube& startCube) {
    // Solve from G1 using restricted moves: {U, U', U2, D, D', D2, R2, L2, F2, B2}
    
    std::vector<std::pair<Face, MoveType>> solution;
    
    if (startCube.isSolved()) {
        return solution;  // Already solved
    }
    
    // Phase 2 restricted move set: 10 moves total
    // U (Normal, Prime, Double), D (Normal, Prime, Double), R2, L2, F2, B2
    struct Phase2Move {
        Face face;
        MoveType type;
    };
    static const Phase2Move PHASE2_MOVES[] = {
        {Face::U, MoveType::Normal}, {Face::U, MoveType::Prime}, {Face::U, MoveType::Double},
        {Face::D, MoveType::Normal}, {Face::D, MoveType::Prime}, {Face::D, MoveType::Double},
        {Face::R, MoveType::Double}, {Face::L, MoveType::Double},
        {Face::F, MoveType::Double}, {Face::B, MoveType::Double},
    };
    static const int NUM_PHASE2_MOVES = 10;
    
    // Phase 2 heuristic: use phase2PDB if available, else count displaced pieces
    auto getP2Heuristic = [this](const CubieCube& state) -> uint8_t {
        uint8_t h = 0;
        if (phase2Ready && phase2PDB) {
            h = std::max(h, phase2PDB->lookup(state));
        }
        if (phase1Ready && phase1PDB) {
            h = std::max(h, phase1PDB->lookup(state));
        }
        // Fallback: count displaced corners and edges
        int displaced = 0;
        for (int i = 0; i < 8; ++i) {
            if (state.getCornerCubie(i).position != i) displaced++;
        }
        for (int i = 0; i < 12; ++i) {
            if (state.getEdgeCubie(i).position != i) displaced++;
        }
        h = std::max(h, static_cast<uint8_t>((displaced + 7) / 8));
        return h;
    };
    
    struct Node {
        RubiksCube state;
        Face face;
        MoveType type;
        uint32_t depth;
    };
    
    uint32_t threshold = getP2Heuristic(startCube.getState());
    std::vector<std::pair<Face, MoveType>> currentPath(18);
    
    while (threshold <= 18) { // Phase 2 is at most 18 moves
        uint32_t nextBound = UINT32_MAX;
        
        std::stack<Node> stack;
        stack.push({startCube, Face::U, MoveType::Normal, 0});
        
        while (!stack.empty()) {
            if (isTimedOut()) {
                std::cerr << "Phase 2 search timed out after " << timeoutMs << "ms" << std::endl;
                return solution;  // Return empty — timed out
            }
            
            Node cur = stack.top();
            stack.pop();
            
            if (cur.depth > 0) {
                currentPath[cur.depth - 1] = {cur.face, cur.type};
            }
            
            if (cur.state.isSolved()) {
                solution.clear();
                for (uint32_t i = 0; i < cur.depth; ++i) {
                    solution.push_back(currentPath[i]);
                }
                return solution;
            }
            
            if (cur.depth < threshold) {
                bool isRoot = (cur.depth == 0);
                
                for (int m = NUM_PHASE2_MOVES - 1; m >= 0; --m) {
                    Face face = PHASE2_MOVES[m].face;
                    MoveType type = PHASE2_MOVES[m].type;
                    
                    // Apply move pruning for non-root nodes
                    if (!isRoot && !MovePruning::isValidMoveSequence(cur.face, cur.type, face, type)) {
                        continue;
                    }
                    
                    RubiksCube next = cur.state;
                    next.applyMove(face, type);
                    
                    uint32_t h = getP2Heuristic(next.getState());
                    uint32_t f_val = cur.depth + 1 + h;
                    
                    if (f_val <= threshold) {
                        stack.push({next, face, type, cur.depth + 1});
                    } else if (f_val < nextBound) {
                        nextBound = f_val;
                    }
                }
            }
        }
        
        if (nextBound == UINT32_MAX) break;
        threshold = nextBound;
    }
    
    std::cerr << "Phase 2 search failed" << std::endl;
    return solution;
}

uint8_t TwoPhase::getPhase1Heuristic(const CubieCube& state) const {
    if (!phase1Ready || !phase1PDB) return 0;
    return phase1PDB->lookup(state);
}

uint8_t TwoPhase::getPhase2Heuristic(const CubieCube& state) const {
    if (!phase2Ready || !phase2PDB) return 0;
    return phase2PDB->lookup(state);
}

// Solves the cube by combining Phase 1 (to G1) and Phase 2 (G1 to solved)
SolutionResult TwoPhase::solve(const RubiksCube& cube) {
    SolutionResult result;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Phase 1: Get to Group 1
    // (searchStartTime is set inside phase1Search)
    auto phase1Moves = phase1Search(cube);
    RubiksCube afterPhase1 = cube;
    for (const auto& move : phase1Moves) {
        afterPhase1.applyMove(move.first, move.second);
    }
    
    // If phase1 timed out and didn't reach G1, report failure
    if (phase1Moves.empty() && !afterPhase1.isSolved()) {
        // Check if we're at least in G1 already
        if (!isInGroup1(afterPhase1.getState())) {
            std::cerr << "Two-Phase: Phase 1 could not reach G1" << std::endl;
            result.found = false;
            auto endTime = std::chrono::high_resolution_clock::now();
            result.solveTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            return result;
        }
    }
    
    // Phase 2: Solve from Group 1
    // Reset timeout for phase 2
    searchStartTime = std::chrono::high_resolution_clock::now();
    auto phase2Moves = phase2Search(afterPhase1);
    
    // Combine solutions
    result.moves = phase1Moves;
    result.moves.insert(result.moves.end(), phase2Moves.begin(), phase2Moves.end());
    result.moveCount = result.moves.size();
    
    for (const auto& move : phase2Moves) {
        afterPhase1.applyMove(move.first, move.second);
    }
    result.found = afterPhase1.isSolved();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.solveTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    return result;
}

} // namespace cube_solver
