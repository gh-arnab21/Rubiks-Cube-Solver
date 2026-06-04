/**
 * @file SymmetryReduction.cpp
 * @brief Implementation of 48-way symmetry reduction
 */

#include "database/SymmetryReduction.h"
#include "database/RankCalculator.h"
#include <algorithm>
#include <vector>

namespace cube_solver {

// Face indices: U=0, L=1, F=2, R=3, B=4, D=5
// Standard Corner Tuples (Clockwise starting from U/D)
static const std::array<std::array<uint8_t, 3>, 8> CORNER_TUPLES = {{
    {0, 1, 4}, // ULB = 0
    {0, 4, 3}, // URB = 1
    {0, 3, 2}, // URF = 2
    {0, 2, 1}, // ULF = 3
    {5, 4, 1}, // DLB = 4
    {5, 3, 4}, // DRB = 5
    {5, 2, 3}, // DRF = 6
    {5, 1, 2}  // DLF = 7
}};

// Standard Edge Tuples (Primary face first)
static const std::array<std::array<uint8_t, 2>, 12> EDGE_TUPLES = {{
    {0, 4}, // UB = 0
    {0, 3}, // UR = 1
    {0, 2}, // UF = 2
    {0, 1}, // UL = 3
    {5, 4}, // DB = 4
    {5, 3}, // DR = 5
    {5, 2}, // DF = 6
    {5, 1}, // DL = 7
    {4, 3}, // BR = 8
    {4, 1}, // BL = 9
    {2, 3}, // FR = 10
    {2, 1}  // FL = 11
}};

static int findCorner(uint8_t f0, uint8_t f1, uint8_t f2) {
    for (int i = 0; i < 8; ++i) {
        auto& t = CORNER_TUPLES[i];
        if ((t[0] == f0 || t[0] == f1 || t[0] == f2) &&
            (t[1] == f0 || t[1] == f1 || t[1] == f2) &&
            (t[2] == f0 || t[2] == f1 || t[2] == f2)) {
            return i;
        }
    }
    return -1;
}

static int findEdge(uint8_t f0, uint8_t f1) {
    for (int i = 0; i < 12; ++i) {
        auto& t = EDGE_TUPLES[i];
        if ((t[0] == f0 || t[0] == f1) && (t[1] == f0 || t[1] == f1)) {
            return i;
        }
    }
    return -1;
}

static CubieCube mapState(const CubieCube& cube, const std::array<uint8_t, 6>& mapping, bool isReflection) {
    CubieCube result;
    
    // Precompute slot destinations
    std::array<int, 8> T_corner;
    for (int i = 0; i < 8; ++i) {
        auto& t = CORNER_TUPLES[i];
        std::array<uint8_t, 3> mapped_t = {mapping[t[0]], mapping[t[1]], mapping[t[2]]};
        if (isReflection) std::swap(mapped_t[1], mapped_t[2]);
        T_corner[i] = findCorner(mapped_t[0], mapped_t[1], mapped_t[2]);
    }
    
    std::array<int, 12> T_edge;
    for (int i = 0; i < 12; ++i) {
        auto& t = EDGE_TUPLES[i];
        T_edge[i] = findEdge(mapping[t[0]], mapping[t[1]]);
    }
    
    // Process corners
    for (int i = 0; i < 8; ++i) {
        int dest_slot = T_corner[i];
        int old_p = cube.getCornerCubie(i).position;
        int old_ori = cube.getCornerCubie(i).orientation;
        
        int new_p = T_corner[old_p];
        uint8_t old_ud_face = CORNER_TUPLES[old_p][old_ori];
        uint8_t new_ud_face = mapping[old_ud_face];
        
        int new_ori = 0;
        for (int o = 0; o < 3; ++o) {
            if (CORNER_TUPLES[new_p][o] == new_ud_face) {
                new_ori = o;
                break;
            }
        }
        
        // Reflection inverts orientation twist (if twisted clockwise, reflection is counter-clockwise)
        if (isReflection && new_ori != 0) {
            new_ori = 3 - new_ori;
        }
        
        result.getCornerCubie(dest_slot) = Cubie(new_p, new_ori);
    }
    
    // Process edges
    for (int i = 0; i < 12; ++i) {
        int dest_slot = T_edge[i];
        int old_p = cube.getEdgeCubie(i).position;
        int old_ori = cube.getEdgeCubie(i).orientation;
        
        int new_p = T_edge[old_p];
        uint8_t old_primary = EDGE_TUPLES[old_p][old_ori];
        uint8_t new_primary = mapping[old_primary];
        
        int new_ori = 0;
        if (EDGE_TUPLES[new_p][1] == new_primary) {
            new_ori = 1;
        }
        
        result.getEdgeCubie(dest_slot) = Cubie(new_p, new_ori);
    }
    
    return result;
}

CubieCube SymmetryReduction::rotateX(const CubieCube& cube) {
    // R-axis clockwise: U->B(4), B->D(5), D->F(2), F->U(0), L->L(1), R->R(3)
    return mapState(cube, {4, 1, 0, 3, 5, 2}, false);
}

CubieCube SymmetryReduction::rotateY(const CubieCube& cube) {
    // U-axis clockwise: U->U(0), D->D(5), F->R(3), R->B(4), B->L(1), L->F(2)
    return mapState(cube, {0, 2, 3, 4, 1, 5}, false);
}

CubieCube SymmetryReduction::rotateZ(const CubieCube& cube) {
    // F-axis clockwise: U->R(3), R->D(5), D->L(1), L->U(0), F->F(2), B->B(4)
    return mapState(cube, {3, 0, 2, 5, 4, 1}, false);
}

CubieCube SymmetryReduction::reflectLR(const CubieCube& cube) {
    // Reflection across L/R plane: L<->R, others stay
    return mapState(cube, {0, 3, 2, 1, 4, 5}, true);
}

std::array<CubieCube, SymmetryReduction::NUM_SYMMETRIES> 
SymmetryReduction::getAllSymmetries(const CubieCube& cube) {
    std::array<CubieCube, NUM_SYMMETRIES> symmetries;
    
    // Generate all 48 by combining X, Y, and reflection
    // Y rotations: 4
    // X rotations: 4 (but only 3 new per Y, since Y^4 = I)
    // Actually, simple generation:
    // Any orientation can be reached by moving U face to 6 possible faces (using X, Z)
    // then rotating around that face 4 times (using Y).
    // This gives 24 rotations.
    // Then multiply by reflection for 48.
    
    CubieCube c = cube;
    int idx = 0;
    
    // 6 face orientations for U
    for (int i = 0; i < 6; ++i) {
        // Rotate around Y 4 times
        for (int j = 0; j < 4; ++j) {
            symmetries[idx++] = c;
            symmetries[idx++] = reflectLR(c);
            c = rotateY(c);
        }
        
        // Move a different face to U
        if (i == 0) c = rotateX(c); // F to U
        else if (i == 1) c = rotateX(c); // D to U
        else if (i == 2) c = rotateX(c); // B to U (now we have covered U, F, D, B)
        else if (i == 3) {
            c = rotateX(c); // Back to U
            c = rotateZ(c); // L to U
        }
        else if (i == 4) {
            c = rotateZ(c);
            c = rotateZ(c); // R to U
        }
    }
    
    return symmetries;
}

CubieCube SymmetryReduction::applySymmetry(const CubieCube& cube, int symmetryIdx) {
    if (symmetryIdx < 0 || symmetryIdx >= NUM_SYMMETRIES) {
        return cube;
    }
    auto symmetries = getAllSymmetries(cube);
    return symmetries[symmetryIdx];
}

CubieCube SymmetryReduction::getCanonical(const CubieCube& cube) {
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
    return cube == canonical;
}

}  // namespace cube_solver

