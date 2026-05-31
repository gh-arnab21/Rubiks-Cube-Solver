/**
 * @file SymmetryReduction.h
 * @brief 48-way symmetry reduction for pattern databases
 * 
 * The Rubik's Cube has 48 symmetries (24 rotations + 24 rotations-with-reflection).
 * Two states differing only by a symmetry have identical minimum distances to solved.
 * We store only ONE representative per symmetry class, reducing memory by 48×.
 * 
 * Example: Corner database shrinks from 88.2M states → 1.8M states (~0.9 MB)
 */

#pragma once

#include <cstdint>
#include <array>
#include "cube/CubieCube.h"

namespace cube_solver {

/**
 * @class SymmetryReduction
 * @brief Handles cube state symmetry transformations
 */
class SymmetryReduction {
public:
    static constexpr int NUM_SYMMETRIES = 24;  // Rotation group only (24 elements)
    
    /**
     * @brief Apply a symmetry operation to a cube state
     * @param cube Input cube state
     * @param symmetryIdx Index 0-23 representing rotation symmetry
     * @return Cube after applying rotation symmetry
     * 
     * Symmetry indices map to rotations:
     * 0-2:   X axis rotations (0°, 90°, 180°, 270°)
     * 3-5:   Y axis rotations
     * 6-23:  Combined rotations
     */
    static CubieCube applySymmetry(const CubieCube& cube, int symmetryIdx);
    
    /**
     * @brief Find the canonical (lexicographically smallest) representation
     * @param cube Input cube state
     * @return The cube state with smallest rank among all 24 rotations
     * 
     * This is the representative that will be stored in the database.
     * Using the smallest rank ensures we always get the same representative.
     */
    static CubieCube getCanonical(const CubieCube& cube);
    
    /**
     * @brief Get the rank of canonical form
     * @param cube Input cube state
     * @param rankFunction Function to compute rank for a cube
     * @return Rank of the canonical form
     * 
     * This should be called instead of rankFunction(cube) directly.
     */
    template <typename RankFunc>
    static uint64_t getCanonicalRank(const CubieCube& cube, RankFunc rankFunction) {
        CubieCube canonical = getCanonical(cube);
        return rankFunction(canonical);
    }
    
    /**
     * @brief Check if a cube is already in canonical form
     * @param cube Cube state to check
     * @return true if this is the canonical form, false otherwise
     */
    static bool isCanonical(const CubieCube& cube);
    
private:
    /**
     * @brief Generate a cube rotated around X axis (90° clockwise when facing right)
     */
    static CubieCube rotateX(const CubieCube& cube);
    
    /**
     * @brief Generate a cube rotated around Y axis (90° clockwise when looking down)
     */
    static CubieCube rotateY(const CubieCube& cube);
    
    /**
     * @brief Generate a cube rotated around Z axis (90° clockwise when facing front)
     */
    static CubieCube rotateZ(const CubieCube& cube);
    
    /**
     * @brief Generate all 24 rotational symmetries from a base cube
     * @param cube Input cube state
     * @return Array of 24 rotations of the input cube
     */
    static std::array<CubieCube, NUM_SYMMETRIES> getAllSymmetries(const CubieCube& cube);
};

}  // namespace cube_solver
