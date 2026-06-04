/**
 * @file HeuristicCombination.cpp
 * @brief Heuristic combination implementation
 */

#include "solver/HeuristicCombination.h"
#include <algorithm>

namespace cube_solver {

HeuristicCombination::HeuristicCombination(HeuristicMode mode)
    : mode(mode) {
}

void HeuristicCombination::setDatabases(PatternDatabase* corner,
                                         PatternDatabase* edge1,
                                         PatternDatabase* edge2,
                                         PatternDatabase* edgePerm) {
    cornerPDB = corner;
    edge1PDB = edge1;
    edge2PDB = edge2;
    edgePermPDB = edgePerm;
}

void HeuristicCombination::setWeights(uint8_t cornerWeight, uint8_t edge1Weight,
                                       uint8_t edge2Weight, uint8_t edgePermWeight) {
    this->cornerWeight = cornerWeight;
    this->edge1Weight = edge1Weight;
    this->edge2Weight = edge2Weight;
    this->edgePermWeight = edgePermWeight;
}

uint8_t HeuristicCombination::getHeuristic(const RubiksCube& cube) const {
    switch (mode) {
        case HeuristicMode::MAX:
            return getMaxHeuristic(cube);
        case HeuristicMode::WEIGHTED_SUM:
            return getWeightedSumHeuristic(cube);
        case HeuristicMode::ADDITIVE_DISJOINT:
            return getAdditiveHeuristic(cube);
        case HeuristicMode::KORF_COMBINATION:
            return getKorfHeuristic(cube);
        default:
            return 0;
    }
}

uint8_t HeuristicCombination::getMaxHeuristic(const RubiksCube& cube) const {
    uint8_t h = 0;
    
    if (cornerPDB && cornerPDB->isValid()) {
        h = std::max(h, cornerPDB->lookup(cube.getState()));
    }
    if (edge1PDB && edge1PDB->isValid()) {
        h = std::max(h, edge1PDB->lookup(cube.getState()));
    }
    if (edge2PDB && edge2PDB->isValid()) {
        h = std::max(h, edge2PDB->lookup(cube.getState()));
    }
    if (edgePermPDB && edgePermPDB->isValid()) {
        h = std::max(h, edgePermPDB->lookup(cube.getState()));
    }
    
    return h;
}

uint8_t HeuristicCombination::getWeightedSumHeuristic(const RubiksCube& cube) const {
    uint16_t h = 0;
    uint16_t totalWeight = 0;
    
    if (cornerPDB && cornerPDB->isValid()) {
        h += cornerPDB->lookup(cube.getState()) * cornerWeight;
        totalWeight += cornerWeight;
    }
    if (edge1PDB && edge1PDB->isValid()) {
        h += edge1PDB->lookup(cube.getState()) * edge1Weight;
        totalWeight += edge1Weight;
    }
    if (edge2PDB && edge2PDB->isValid()) {
        h += edge2PDB->lookup(cube.getState()) * edge2Weight;
        totalWeight += edge2Weight;
    }
    if (edgePermPDB && edgePermPDB->isValid()) {
        h += edgePermPDB->lookup(cube.getState()) * edgePermWeight;
        totalWeight += edgePermWeight;
    }
    
    if (totalWeight > 0) {
        h = (h + totalWeight / 2) / totalWeight;  // Round division
    }
    
    return std::min(h, (uint16_t)255);
}

uint8_t HeuristicCombination::getAdditiveHeuristic(const RubiksCube& cube) const {
    // WARNING: Only use if databases are guaranteed disjoint!
    // Corner database and edge databases don't overlap in piece coverage,
    // so this is somewhat valid, but edge1/edge2 might overlap.
    // For maximum admissibility, use max() instead.
    
    uint16_t h = 0;
    
    if (cornerPDB && cornerPDB->isValid()) {
        h += cornerPDB->lookup(cube.getState());
    }
    // For edges, use max of edge databases to avoid overlap
    uint8_t edgeH = 0;
    if (edge1PDB && edge1PDB->isValid()) {
        edgeH = std::max(edgeH, edge1PDB->lookup(cube.getState()));
    }
    if (edge2PDB && edge2PDB->isValid()) {
        edgeH = std::max(edgeH, edge2PDB->lookup(cube.getState()));
    }
    
    h += edgeH;
    
    return std::min(h, (uint16_t)255);
}

uint8_t HeuristicCombination::getKorfHeuristic(const RubiksCube& cube) const {
    // Korf's approach: carefully weighted sum
    // Weights optimized via linear programming to be as tight as possible
    // while remaining admissible
    // Default Korf weights: roughly equal importance to corner/edge distances
    
    uint8_t cornerH = (cornerPDB && cornerPDB->isValid()) 
        ? cornerPDB->lookup(cube.getState()) : 0;
    uint8_t edgeH = 0;
    
    if (edge1PDB && edge1PDB->isValid()) {
        edgeH = std::max(edgeH, edge1PDB->lookup(cube.getState()));
    }
    if (edge2PDB && edge2PDB->isValid()) {
        edgeH = std::max(edgeH, edge2PDB->lookup(cube.getState()));
    }
    
    // Simple approximation: use max (corners vs edges)
    // More sophisticated versions use linear combinations
    return std::max(cornerH, edgeH);
}

} // namespace cube_solver
