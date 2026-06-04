/**
 * @file TwoPhase.h
 * @brief Kociemba's Two-Phase Algorithm for optimal cube solving
 * 
 * The two-phase algorithm decomposes the problem into:
 * Phase 1: Reach Group G1 where:
 *   - All corners properly oriented (corner orientation = 0)
 *   - All edges properly oriented (edge orientation = 0)
 *   - UD slice edges (FR, FL, BR, BL) are in correct slice (positions 8-11)
 *   - Phase 1 uses moves: U, U2, U', D, D2, D', R, R2, R', L, L2, L', F, F2, F', B, B2, B'
 *
 * Phase 2: Solve from G1 using restricted moves:
 *   - Only: U, U2, U', D, D2, D', R2, L2, F2, B2
 *   - This is effectively solving corners + permuting edges
 * 
 * Two separate pattern databases optimize each phase separately.
 * Result: Often faster than single IDA*, especially for hard cases.
 */

#ifndef TWOPHASE_H
#define TWOPHASE_H

#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include "cube/RubiksCube.h"
#include "cube/CubieCube.h"
#include "database/PatternDatabase.h"
#include "solver/Solver.h"

namespace cube_solver {

/**
 * @class TwoPhaseAlgorithm
 * @brief Kociemba-style two-phase solver
 */
class TwoPhase : public Solver {
public:
    TwoPhase();
    ~TwoPhase() override = default;
    
    /**
     * @brief Load phase-specific databases
     */
    bool loadDatabases(const std::string& dataDirectory);
    
    bool generateDatabases(const std::string& dataDirectory);
    
    SolutionResult solve(const RubiksCube& cube) override;
    bool isReady() const override { return phase1Ready && phase2Ready; }
    std::string getName() const override { return "Kociemba Two-Phase"; }
    
    /**
     * @brief Perform phase 1 search (get to G1)
     */
    std::vector<std::pair<Face, MoveType>> phase1Search(const RubiksCube& cube);
    
    /**
     * @brief Set timeout
     */
    void setTimeout(uint32_t ms) { timeoutMs = ms; }
    
private:
    // Phase 1: Orientation & Slice PDB
    std::unique_ptr<PatternDatabase> phase1PDB;
    bool phase1Ready = false;
    
    // Phase 2: Corners & Edge permutation PDB
    std::unique_ptr<PatternDatabase> phase2PDB;
    bool phase2Ready = false;
    
    uint32_t timeoutMs = 30000;  // 30 second default timeout
    std::chrono::time_point<std::chrono::high_resolution_clock> searchStartTime;
    
    /**
     * @brief Check if search has exceeded timeout
     */
    bool isTimedOut() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStartTime).count();
        return elapsed > timeoutMs;
    }
    
    /**
     * @brief Check if cube is in Group 1 (solved or ready for phase 2)
     * All corners must be oriented, all edges must be oriented,
     * UD-slice edges must be in positions 8-11
     */
    static bool isInGroup1(const CubieCube& cube);
    
    /**
     * @brief Perform phase 2 search (solve from G1)
     * @param startCube Cube state after phase 1
     */
    std::vector<std::pair<Face, MoveType>> phase2Search(const RubiksCube& startCube);
    
    /**
     * @brief Get heuristic for phase 1
     */
    uint8_t getPhase1Heuristic(const CubieCube& state) const;
    
    /**
     * @brief Get heuristic for phase 2
     */
    uint8_t getPhase2Heuristic(const CubieCube& state) const;
};

} // namespace cube_solver

#endif // TWOPHASE_H
