#include "cube/CubieCube.h"
#include "utils/Constants.h"
#include <algorithm>
#include <ostream>

namespace cube_solver {

// ============ CUBIE CUBE CONSTRUCTOR ============

CubieCube::CubieCube() {
    reset();
}

// ============ INITIALIZATION ============

void CubieCube::reset() {
    // Initialize all cubies to solved state
    // For corners and edges:
    //   - Cubie at position i should be: Cubie(position=i, orientation=0)
    // This represents the "solved" or "identity" state of the cube
    
    for (uint8_t i = 0; i < NUM_CORNERS; ++i) {
        cornerCubies[i] = Cubie(i, 0);
    }
    
    for (uint8_t i = 0; i < NUM_EDGES; ++i) {
        edgeCubies[i] = Cubie(i, 0);
    }
}

// ============ MOVE OPERATIONS ============

void CubieCube::applyMove(Face face, MoveType moveType) {
    // Implement move application
    // For moveType:
    //   - Normal: apply single 90° twist
    //   - Prime: apply single twist 3 times (or create inverse)
    //   - Double: apply single twist 2 times
    
    switch (moveType) {
        case MoveType::Normal:
            applySingleMove(face);
            break;
        case MoveType::Prime:
            // Apply same move 3 times to get inverse
            applySingleMove(face);
            applySingleMove(face);
            applySingleMove(face);
            break;
        case MoveType::Double:
            applySingleMove(face);
            applySingleMove(face);
            break;
    }
}

void CubieCube::applySingleMove(Face face) {
    // Single 90-degree clockwise face twist
    // Each face cycles 4 corners and 4 edges
    // Implementation based on reference: benbotto/rubiks-cube-cracker
    
    switch (face) {
        case Face::U: {
            // UP FACE: 90° clockwise when viewed from top
            // Corner cycle: 0(ULB) → 1(URB) → 2(URF) → 3(ULF) → 0
            Cubie cornerTmp = cornerCubies[0];
            cornerCubies[0] = cornerCubies[3];
            cornerCubies[3] = cornerCubies[2];
            cornerCubies[2] = cornerCubies[1];
            cornerCubies[1] = cornerTmp;
            
            // Edge cycle: 3(UL) → 0(UB) → 1(UR) → 2(UF) → 3
            Cubie edgeTmp = edgeCubies[3];
            edgeCubies[3] = edgeCubies[2];
            edgeCubies[2] = edgeCubies[1];
            edgeCubies[1] = edgeCubies[0];
            edgeCubies[0] = edgeTmp;
            
            // U move: no orientation changes
            break;
        }
        
        case Face::D: {
            // DOWN FACE: 90° clockwise when viewed from bottom
            // Corner cycle: 4(DLB) → 7(DLF) → 6(DRF) → 5(DRB) → 4
            Cubie cornerTmp = cornerCubies[4];
            cornerCubies[4] = cornerCubies[5];
            cornerCubies[5] = cornerCubies[6];
            cornerCubies[6] = cornerCubies[7];
            cornerCubies[7] = cornerTmp;
            
            // Edge cycle: 4(DB) → 7(DL) → 6(DF) → 5(DR) → 4
            Cubie edgeTmp = edgeCubies[4];
            edgeCubies[4] = edgeCubies[5];
            edgeCubies[5] = edgeCubies[6];
            edgeCubies[6] = edgeCubies[7];
            edgeCubies[7] = edgeTmp;
            
            // D move: no orientation changes
            break;
        }
        
        case Face::L: {
            // LEFT FACE: 90° clockwise when viewed from left
            // Corner cycle: 0(ULB) → 3(ULF) → 7(DLF) → 4(DLB) → 0
            uint8_t ori0 = cornerCubies[0].orientation;
            uint8_t ori3 = cornerCubies[3].orientation;
            uint8_t ori7 = cornerCubies[7].orientation;
            uint8_t ori4 = cornerCubies[4].orientation;
            
            // Now cycle positions: 0←4←7←3←0
            Cubie cornerTmp = cornerCubies[0];
            cornerCubies[0] = cornerCubies[4];  // ori4 goes to position 0
            cornerCubies[4] = cornerCubies[7];  // ori7 goes to position 4
            cornerCubies[7] = cornerCubies[3];  // ori3 goes to position 7
            cornerCubies[3] = cornerTmp;        // ori0 goes to position 3
            
            // Apply orientation changes based on where pieces moved TO
            cornerCubies[0].orientation = (ori4 + 2) % 3;  // ori4 at position 0 twists +2
            cornerCubies[4].orientation = (ori7 + 1) % 3;  // ori7 at position 4 twists +1
            cornerCubies[7].orientation = (ori3 + 2) % 3;  // ori3 at position 7 twists +2
            cornerCubies[3].orientation = (ori0 + 1) % 3;  // ori0 at position 3 twists +1
            
            // Edge cycle: 3(UL) → 11(FL) → 7(DL) → 9(BL) → 3
            Cubie edgeTmp = edgeCubies[3];
            edgeCubies[3] = edgeCubies[9];
            edgeCubies[9] = edgeCubies[7];
            edgeCubies[7] = edgeCubies[11];
            edgeCubies[11] = edgeTmp;
            
            // L move: no edge orientation changes
            break;
        }
        
        case Face::R: {
            // RIGHT FACE: 90° clockwise when viewed from right
            // Corner cycle: 1(URB) → 5(DRB) → 6(DRF) → 2(URF) → 1
            uint8_t ori1 = cornerCubies[1].orientation;
            uint8_t ori2 = cornerCubies[2].orientation;
            uint8_t ori5 = cornerCubies[5].orientation;
            uint8_t ori6 = cornerCubies[6].orientation;
            
            // Now cycle positions: 1←2←6←5←1
            Cubie cornerTmp = cornerCubies[1];
            cornerCubies[1] = cornerCubies[2];  // ori2 goes to position 1
            cornerCubies[2] = cornerCubies[6];  // ori6 goes to position 2
            cornerCubies[6] = cornerCubies[5];  // ori5 goes to position 6
            cornerCubies[5] = cornerTmp;        // ori1 goes to position 5
            
            // Apply orientation changes based on where pieces moved TO
            cornerCubies[1].orientation = (ori2 + 1) % 3;  // ori2 at position 1 twists +1
            cornerCubies[2].orientation = (ori6 + 2) % 3;  // ori6 at position 2 twists +2
            cornerCubies[6].orientation = (ori5 + 1) % 3;  // ori5 at position 6 twists +1
            cornerCubies[5].orientation = (ori1 + 2) % 3;  // ori1 at position 5 twists +2
            
            // Edge cycle: 1(UR) → 8(BR) → 5(DR) → 10(FR) → 1
            Cubie edgeTmp = edgeCubies[1];
            edgeCubies[1] = edgeCubies[10];
            edgeCubies[10] = edgeCubies[5];
            edgeCubies[5] = edgeCubies[8];
            edgeCubies[8] = edgeTmp;
            
            // R move: no edge orientation changes
            break;
        }
        
        case Face::F: {
            // FRONT FACE: 90° clockwise when viewed from front
            // Corner cycle: 3(ULF) → 2(URF) → 6(DRF) → 7(DLF) → 3
            uint8_t ori3 = cornerCubies[3].orientation;
            uint8_t ori2 = cornerCubies[2].orientation;
            uint8_t ori7 = cornerCubies[7].orientation;
            uint8_t ori6 = cornerCubies[6].orientation;
            
            // Now cycle positions: 3←7←6←2←3
            Cubie cornerTmp = cornerCubies[3];
            cornerCubies[3] = cornerCubies[7];  // ori7 goes to position 3
            cornerCubies[7] = cornerCubies[6];  // ori6 goes to position 7
            cornerCubies[6] = cornerCubies[2];  // ori2 goes to position 6
            cornerCubies[2] = cornerTmp;        // ori3 goes to position 2
            
            // Apply orientation changes based on where pieces moved TO
            cornerCubies[3].orientation = (ori7 + 2) % 3;  // ori7 at position 3 twists +2
            cornerCubies[2].orientation = (ori3 + 1) % 3;  // ori3 at position 2 twists +1
            cornerCubies[7].orientation = (ori6 + 1) % 3;  // ori6 at position 7 twists +1
            cornerCubies[6].orientation = (ori2 + 2) % 3;  // ori2 at position 6 twists +2
            
            // Edge cycle: 2(UF) → 10(FR) → 6(DF) → 11(FL) → 2
            Cubie edgeTmp = edgeCubies[2];
            edgeCubies[2] = edgeCubies[11];
            edgeCubies[11] = edgeCubies[6];
            edgeCubies[6] = edgeCubies[10];
            edgeCubies[10] = edgeTmp;
            
            // F move: edge orientations FLIP (0→1, 1→0)
            edgeCubies[2].orientation ^= 1;
            edgeCubies[11].orientation ^= 1;
            edgeCubies[6].orientation ^= 1;
            edgeCubies[10].orientation ^= 1;
            break;
        }
        
        case Face::B: {
            // BACK FACE: 90° clockwise when viewed from back
            // Corner cycle: 1(URB) → 0(ULB) → 4(DLB) → 5(DRB) → 1
            uint8_t ori1 = cornerCubies[1].orientation;
            uint8_t ori0 = cornerCubies[0].orientation;
            uint8_t ori5 = cornerCubies[5].orientation;
            uint8_t ori4 = cornerCubies[4].orientation;
            
            // Now cycle positions: 1←5←4←0←1
            Cubie cornerTmp = cornerCubies[1];
            cornerCubies[1] = cornerCubies[5];  // ori5 goes to position 1
            cornerCubies[5] = cornerCubies[4];  // ori4 goes to position 5
            cornerCubies[4] = cornerCubies[0];  // ori0 goes to position 4
            cornerCubies[0] = cornerTmp;        // ori1 goes to position 0
            
            // Apply orientation changes based on where pieces moved TO
            cornerCubies[1].orientation = (ori5 + 2) % 3;  // ori5 at position 1 twists +2
            cornerCubies[0].orientation = (ori1 + 1) % 3;  // ori1 at position 0 twists +1
            cornerCubies[5].orientation = (ori4 + 1) % 3;  // ori4 at position 5 twists +1
            cornerCubies[4].orientation = (ori0 + 2) % 3;  // ori0 at position 4 twists +2
            
            // Edge cycle: 0(UB) → 9(BL) → 4(DB) → 8(BR) → 0
            Cubie edgeTmp = edgeCubies[0];
            edgeCubies[0] = edgeCubies[8];
            edgeCubies[8] = edgeCubies[4];
            edgeCubies[4] = edgeCubies[9];
            edgeCubies[9] = edgeTmp;
            
            // B move: edge orientations FLIP
            edgeCubies[0].orientation ^= 1;
            edgeCubies[8].orientation ^= 1;
            edgeCubies[4].orientation ^= 1;
            edgeCubies[9].orientation ^= 1;
            break;
        }
        
        default:
            break;
    }
}

void CubieCube::applyInverseMove(Face face, MoveType moveType) {
    // Inverse move: X' inverse is X, X inverse is X', X2 inverse is X2
    // Simple approach: compute inverse mapping
    
    MoveType inverseType = MoveType::Prime;  // Default: initialize to avoid warning
    switch (moveType) {
        case MoveType::Normal:
            inverseType = MoveType::Prime;  // Normal inverse is Prime
            break;
        case MoveType::Prime:
            inverseType = MoveType::Normal;  // Prime inverse is Normal
            break;
        case MoveType::Double:
            inverseType = MoveType::Double;  // Double inverse is itself
            break;
    }
    applyMove(face, inverseType);
}

// ============ STATE QUERIES ============

bool CubieCube::isSolved() const {
    // Check if cube is solved
    // A cube is solved when:
    // - All corners are in their home positions with orientation 0
    // - All edges are in their home positions with orientation 0
    
    for (uint8_t i = 0; i < NUM_CORNERS; ++i) {
        if (cornerCubies[i].position != i || cornerCubies[i].orientation != 0) {
            return false;
        }
    }
    
    for (uint8_t i = 0; i < NUM_EDGES; ++i) {
        if (edgeCubies[i].position != i || edgeCubies[i].orientation != 0) {
            return false;
        }
    }
    
    return true;
}

std::array<uint8_t, NUM_CORNERS> CubieCube::getCornerPermutation() const {
    // Extract permutation from cubies
    // Return array where index represents home position
    // and value represents which cubie is there
    
    std::array<uint8_t, NUM_CORNERS> perm;
    for (uint8_t i = 0; i < NUM_CORNERS; ++i) {
        perm[i] = cornerCubies[i].position;
    }
    return perm;
}

std::array<uint8_t, NUM_CORNERS> CubieCube::getCornerOrientations() const {
    // Extract orientations from corner cubies
    
    std::array<uint8_t, NUM_CORNERS> ori;
    for (uint8_t i = 0; i < NUM_CORNERS; ++i) {
        ori[i] = cornerCubies[i].orientation;
    }
    return ori;
}

std::array<uint8_t, NUM_EDGES> CubieCube::getEdgePermutation() const {
    // Extract permutation from edge cubies
    
    std::array<uint8_t, NUM_EDGES> perm;
    for (uint8_t i = 0; i < NUM_EDGES; ++i) {
        perm[i] = edgeCubies[i].position;
    }
    return perm;
}

std::array<uint8_t, NUM_EDGES> CubieCube::getEdgeOrientations() const {
    // Extract orientations from edge cubies
    
    std::array<uint8_t, NUM_EDGES> ori;
    for (uint8_t i = 0; i < NUM_EDGES; ++i) {
        ori[i] = edgeCubies[i].orientation;
    }
    return ori;
}

// ============ UTILITY ============

bool CubieCube::operator==(const CubieCube& other) const {
    // Compare two cube states for equality
    
    for (uint8_t i = 0; i < NUM_CORNERS; ++i) {
        if (cornerCubies[i].position != other.cornerCubies[i].position ||
            cornerCubies[i].orientation != other.cornerCubies[i].orientation) {
            return false;
        }
    }
    
    for (uint8_t i = 0; i < NUM_EDGES; ++i) {
        if (edgeCubies[i].position != other.edgeCubies[i].position ||
            edgeCubies[i].orientation != other.edgeCubies[i].orientation) {
            return false;
        }
    }
    
    return true;
}

std::ostream& operator<<(std::ostream& os, const CubieCube& cube) {
    // Output cube state for debugging
    os << "Corners:\n";
    for (uint8_t i = 0; i < NUM_CORNERS; ++i) {
        os << "  Pos " << (int)i << ": pos=" << (int)cube.cornerCubies[i].position
           << ", ori=" << (int)cube.cornerCubies[i].orientation << "\n";
    }
    os << "Edges:\n";
    for (uint8_t i = 0; i < NUM_EDGES; ++i) {
        os << "  Pos " << (int)i << ": pos=" << (int)cube.edgeCubies[i].position
           << ", ori=" << (int)cube.edgeCubies[i].orientation << "\n";
    }
    return os;
}

} // namespace cube_solver
