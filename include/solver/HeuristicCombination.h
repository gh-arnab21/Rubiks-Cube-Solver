/**
 * @file HeuristicCombination.h
 * @brief Advanced heuristic combination strategies
 * 
 * Different ways to combine multiple pattern databases:
 * 1. Maximum: h(n) = max(h1(n), h2(n), h3(n)) - admissible, loose
 * 2. Weighted sum: h(n) = w1*h1(n) + w2*h2(n) + ... - can be tighter
 * 3. Additive: h(n) = h1(n) + h2(n) - only valid if disjoint
 * 4. Korf's combination: weighted sum optimized via linear programming
 */

#ifndef HEURISTIC_COMBINATION_H
#define HEURISTIC_COMBINATION_H

#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include "cube/RubiksCube.h"
#include "database/PatternDatabase.h"

namespace cube_solver {
/**
 * @enum HeuristicMode
 * @brief How to combine heuristics from multiple PDBs
 */
enum class HeuristicMode : uint8_t {
    MAX,              ///< h(n) = max(all PDB values)
    WEIGHTED_SUM,     ///< h(n) = weighted sum (can exceed actual distance!)
    ADDITIVE_DISJOINT,///< h(n) = sum of disjoint PDB values
    KORF_COMBINATION, ///< Weighted sum with careful weights (40, 40, 40 corners/edges)
};

/**
 * @class HeuristicCombination
 * @brief Combines multiple PDB heuristics into single estimate
 */
class HeuristicCombination {
public:
    /**
     * @brief Initialize with pattern databases
     */
    explicit HeuristicCombination(HeuristicMode mode = HeuristicMode::MAX);
    
    /**
     * @brief Set the databases to use
     */
    void setDatabases(PatternDatabase* corner,
                      PatternDatabase* edge1,
                      PatternDatabase* edge2,
                      PatternDatabase* edgePerm);
    
    /**
     * @brief Get combined heuristic value
     */
    uint8_t getHeuristic(const RubiksCube& cube) const;
    
    /**
     * @brief Set weights for weighted combination (only used for WEIGHTED_SUM mode)
     */
    void setWeights(uint8_t cornerWeight, uint8_t edge1Weight, 
                   uint8_t edge2Weight, uint8_t edgePermWeight);
    
private:
    HeuristicMode mode;
    PatternDatabase* cornerPDB = nullptr;
    PatternDatabase* edge1PDB = nullptr;
    PatternDatabase* edge2PDB = nullptr;
    PatternDatabase* edgePermPDB = nullptr;
    
    uint8_t cornerWeight = 1;
    uint8_t edge1Weight = 1;
    uint8_t edge2Weight = 1;
    uint8_t edgePermWeight = 1;
    
    uint8_t getMaxHeuristic(const RubiksCube& cube) const;
    uint8_t getWeightedSumHeuristic(const RubiksCube& cube) const;
    uint8_t getAdditiveHeuristic(const RubiksCube& cube) const;
    uint8_t getKorfHeuristic(const RubiksCube& cube) const;
};

} // namespace cube_solver

#endif // HEURISTIC_COMBINATION_H
