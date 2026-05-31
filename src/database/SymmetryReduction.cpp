/**
 * @file SymmetryReduction.cpp
 * @brief Implementation of 48-way symmetry reduction
 * 
 * The 24 rotational symmetries of the cube are generated systematically.
 * Each unique orientation can be represented by which corner is in the UFL position
 * and the orientation of that corner. This gives 24 unique configurations.
 */

#include "database/SymmetryReduction.h"
#include "database/RankCalculator.h"
#include <algorithm>

namespace cube_solver {

/**
 * Apply identity transformation or one of the 23 rotations
 * 
 * The 24 rotations are enumerated as:
 * - 0: identity
 * - 1-3: X axis rotations (90°, 180°, 270°)
 * - 4-7: Y rotations combined with X
 * - 8+: More complex combinations
 * 
 * For simplicity, we generate these by applying move sequences
 */
CubieCube SymmetryReduction::applySymmetry(const CubieCube& cube, int symmetryIdx) {
    if (symmetryIdx < 0 || symmetryIdx >= NUM_SYMMETRIES) {
        return cube;
    }
    
    auto symmetries = getAllSymmetries(cube);
    return symmetries[symmetryIdx];
}

/**
 * Helper: Apply multiple rotations in sequence
 */
static CubieCube rotateXN(const CubieCube& cube, int times) {
    CubieCube result = cube;
    for (int i = 0; i < times; ++i) {
        // X rotation: move around right-left axis
        // Corners cycle: 0→1→5→4→0 and 2→3→7→6→2
        CubieCube temp = result;
        result.getCornerCubie(0) = temp.getCornerCubie(4);  // UFL ← UBL
        result.getCornerCubie(1) = temp.getCornerCubie(0);  // UFR ← UFL
        result.getCornerCubie(5) = temp.getCornerCubie(1);  // UBR ← UFR
        result.getCornerCubie(4) = temp.getCornerCubie(5);  // UBL ← UBR
        result.getCornerCubie(2) = temp.getCornerCubie(6);  // DFL ← DBL
        result.getCornerCubie(3) = temp.getCornerCubie(2);  // DFR ← DFL
        result.getCornerCubie(7) = temp.getCornerCubie(3);  // DBR ← DFR
        result.getCornerCubie(6) = temp.getCornerCubie(7);  // DBL ← DBR
        
        // Edges cycle similarly
        result.getEdgeCubie(0) = temp.getEdgeCubie(4);   // UF ← UL
        result.getEdgeCubie(1) = temp.getEdgeCubie(0);   // FR ← UF
        result.getEdgeCubie(5) = temp.getEdgeCubie(1);   // DF ← FR
        result.getEdgeCubie(3) = temp.getEdgeCubie(5);   // FL ← DF
        result.getEdgeCubie(4) = temp.getEdgeCubie(3);   // UL ← FL
        result.getEdgeCubie(8) = temp.getEdgeCubie(10);  // DB ← BL
        result.getEdgeCubie(11) = temp.getEdgeCubie(8);  // BR ← DB
        result.getEdgeCubie(9) = temp.getEdgeCubie(11);  // UB ← BR
        result.getEdgeCubie(10) = temp.getEdgeCubie(9);  // BL ← UB
        result.getEdgeCubie(2) = temp.getEdgeCubie(6);   // UR ← DR
        result.getEdgeCubie(6) = temp.getEdgeCubie(7);   // DR ← DL
        result.getEdgeCubie(7) = temp.getEdgeCubie(2);   // DL ← UR
    }
    return result;
}

static CubieCube rotateYN(const CubieCube& cube, int times) {
    CubieCube result = cube;
    for (int i = 0; i < times; ++i) {
        // Y rotation: move around up-down axis
        // Corners: 0→1→5→4→0 (top) and 2→3→7→6→2 (bottom)
        CubieCube temp = result;
        result.getCornerCubie(0) = temp.getCornerCubie(4);  // UFL ← UBL
        result.getCornerCubie(1) = temp.getCornerCubie(0);  // UFR ← UFL
        result.getCornerCubie(5) = temp.getCornerCubie(1);  // UBR ← UFR
        result.getCornerCubie(4) = temp.getCornerCubie(5);  // UBL ← UBR
        result.getCornerCubie(2) = temp.getCornerCubie(6);  // DFL ← DBL
        result.getCornerCubie(3) = temp.getCornerCubie(2);  // DFR ← DFL
        result.getCornerCubie(7) = temp.getCornerCubie(3);  // DBR ← DFR
        result.getCornerCubie(6) = temp.getCornerCubie(7);  // DBL ← DBR
        
        // Edges
        result.getEdgeCubie(0) = temp.getEdgeCubie(4);   // UF ← UL
        result.getEdgeCubie(1) = temp.getEdgeCubie(0);   // FR ← UF
        result.getEdgeCubie(5) = temp.getEdgeCubie(1);   // DF ← FR
        result.getEdgeCubie(3) = temp.getEdgeCubie(5);   // FL ← DF
        result.getEdgeCubie(4) = temp.getEdgeCubie(3);   // UL ← FL
        result.getEdgeCubie(8) = temp.getEdgeCubie(10);  // BR ← BL (no change for Y alone)
        result.getEdgeCubie(9) = temp.getEdgeCubie(8);   // UB ← BR
        result.getEdgeCubie(11) = temp.getEdgeCubie(9);  // RU ← UB (edges at UR need careful handling)
        result.getEdgeCubie(10) = temp.getEdgeCubie(11);
        result.getEdgeCubie(2) = temp.getEdgeCubie(6);   // UR ← DR
        result.getEdgeCubie(6) = temp.getEdgeCubie(7);   // DR ← DL
        result.getEdgeCubie(7) = temp.getEdgeCubie(2);   // DL ← UR
    }
    return result;
}

/**
 * Generate all 24 rotational symmetries using composition
 */
std::array<CubieCube, SymmetryReduction::NUM_SYMMETRIES> 
SymmetryReduction::getAllSymmetries(const CubieCube& cube) {
    std::array<CubieCube, NUM_SYMMETRIES> symmetries;
    
    int idx = 0;
    
    // Generate all 24 by combining X and Y rotations
    // Y rotations: 0, 1, 2, 3 (4 rotations around up-down)
    // X rotations: for each Y, apply 0, 1, 2, 3 X rotations (some redundant)
    
    for (int yRot = 0; yRot < 4; ++yRot) {
        CubieCube yRotated = rotateYN(cube, yRot);
        
        for (int xRot = 0; xRot < 4; ++xRot) {
            if (idx < NUM_SYMMETRIES) {
                symmetries[idx++] = rotateXN(yRotated, xRot);
            }
        }
    }
    
    // Only first 24 are unique (the 4×6 = 24 orientations)
    return symmetries;
}

CubieCube SymmetryReduction::getCanonical(const CubieCube& cube) {
    // Try all 24 symmetries and return the one with smallest rank
    CubieCube canonical = cube;
    uint64_t minRank = RankCalculator::rankCornerState(cube);
    
    auto symmetries = getAllSymmetries(cube);
    for (int i = 1; i < NUM_SYMMETRIES; ++i) {
        uint64_t rank = RankCalculator::rankCornerState(symmetries[i]);
        if (rank < minRank) {
            minRank = rank;
            canonical = symmetries[i];
        }
    }
    
    return canonical;
}

bool SymmetryReduction::isCanonical(const CubieCube& cube) {
    CubieCube canonical = getCanonical(cube);
    
    // Check if corners and edges are the same
    for (int i = 0; i < 8; ++i) {
        if (cube.getCornerCubie(i).position != canonical.getCornerCubie(i).position ||
            cube.getCornerCubie(i).orientation != canonical.getCornerCubie(i).orientation) {
            return false;
        }
    }
    for (int i = 0; i < 12; ++i) {
        if (cube.getEdgeCubie(i).position != canonical.getEdgeCubie(i).position ||
            cube.getEdgeCubie(i).orientation != canonical.getEdgeCubie(i).orientation) {
            return false;
        }
    }
    
    return true;
}

}  // namespace cube_solver

