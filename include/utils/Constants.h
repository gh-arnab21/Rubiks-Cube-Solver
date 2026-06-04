#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <array>

/**
 * @file Constants.h
 * @brief Global constants and enumerations for Rubik's Cube operations
 * 
 * Defines constants for cube operations, including:
 * - Face enum for cube faces (U, L, F, R, B, D)
 * - Move types (normal, prime, double)
 * - Cubie counts and positions
 * - Orientation ranges
 */

namespace cube_solver {

// ============ COLOR DEFINITIONS ============
enum class Color : uint8_t {
    WHITE = 0,
    YELLOW = 1,
    GREEN = 2,
    BLUE = 3,
    RED = 4,
    ORANGE = 5,
    SIZE = 6
};

// ============ FACE DEFINITIONS ============
enum class Face : uint8_t {
    U = 0,  // Up face
    L = 1,  // Left face
    F = 2,  // Front face
    R = 3,  // Right face
    B = 4,  // Back face
    D = 5,  // Down face
    SIZE = 6
};

// ============ MOVE DEFINITIONS ============
enum class MoveType : uint8_t {
    Normal = 0,   // Single 90-degree clockwise turn
    Prime = 1,    // 90-degree counter-clockwise turn (3 clockwise turns)
    Double = 2    // 180-degree turn (2 clockwise turns)
};

// ============ CUBIE DEFINITIONS ============
// Corner cubies: 8 positions (0-7)
// Edge cubies: 12 positions (0-11)
constexpr uint8_t NUM_CORNERS = 8;
constexpr uint8_t NUM_EDGES = 12;

// Orientation ranges
// Corners: 0, 1, or 2 (3 possible orientations)
// Edges: 0 or 1 (2 possible orientations)
constexpr uint8_t CORNER_ORIENTATION_MAX = 3;
constexpr uint8_t EDGE_ORIENTATION_MAX = 2;

// Corner position indices (relative to solved state)
enum class CornerPos : uint8_t {
    ULB = 0,  // Up-Left-Back
    URB = 1,  // Up-Right-Back
    URF = 2,  // Up-Right-Front
    ULF = 3,  // Up-Left-Front
    DLB = 4,  // Down-Left-Back
    DRB = 5,  // Down-Right-Back
    DRF = 6,  // Down-Right-Front
    DLF = 7   // Down-Left-Front
};

// Edge position indices (relative to solved state)
enum class EdgePos : uint8_t {
    UB = 0,   // Up-Back
    UR = 1,   // Up-Right
    UF = 2,   // Up-Front
    UL = 3,   // Up-Left
    DB = 4,   // Down-Back
    DR = 5,   // Down-Right
    DF = 6,   // Down-Front
    DL = 7,   // Down-Left
    BR = 8,   // Back-Right
    BL = 9,   // Back-Left
    FR = 10,  // Front-Right
    FL = 11   // Front-Left
};

// ============ PATTERN DATABASE DEFINITIONS ============
// Pattern database sizes (in number of states)
// Corner permutations: 8! = 40,320
// Corner orientations: 3^7 = 2,187 (8th corner orientation is determined by other 7)
// Total corner states: 40,320 * 2,187 = 88,179,840

// Edge permutations: 12P7 = 12!/(12-7)! = 3,991,680 (for 7 edges out of 12 positions)
// Edge orientations: 2^7 = 128 (8th edge orientation is determined)
// Total edge states per 7-edge database: 3,991,680 * 128 = 511,075,840

// Permutation limits for rank calculation
constexpr uint64_t CORNER_PERMUTATION_MAX = 40320;  // 8!
constexpr uint64_t CORNER_ORIENTATION_STATES = 2187;  // 3^7

constexpr uint64_t EDGE_7_PERMUTATION_MAX = 3991680;  // 12P7
constexpr uint64_t EDGE_7_ORIENTATION_STATES = 128;  // 2^7

// ============ MOVE CONSTANTS ============
// Total possible moves: 18 (6 faces * 3 move types)
constexpr uint8_t NUM_MOVES = 18;

// ============ MOVE CYCLE DEFINITIONS ============
// Each face rotation cycles 4 cubies. Use these constants for move implementation.
//
// CORNER CYCLES (positions affected by each face):
// U-face cycles: ULB(0) → URB(1) → URF(2) → ULF(3) → ULB
// D-face cycles: DLB(4) → DLF(7) → DRF(6) → DRB(5) → DLB  
// L-face cycles: ULB(0) → ULF(3) → DLF(7) → DLB(4) → ULB
// R-face cycles: URB(1) → DRB(5) → DRF(6) → URF(2) → URB
// F-face cycles: ULF(3) → URF(2) → DRF(6) → DLF(7) → ULF
// B-face cycles: URB(1) → ULB(0) → DLB(4) → DRB(5) → URB
//
// EDGE CYCLES (positions affected by each face):
// U-face cycles: UB(0) → UR(1) → UF(2) → UL(3) → UB
// D-face cycles: DB(4) → DL(7) → DF(6) → DR(5) → DB
// L-face cycles: UL(3) → FL(11) → DL(7) → BL(9) → UL
// R-face cycles: UR(1) → BR(8) → DR(5) → FR(10) → UR
// F-face cycles: UF(2) → FR(10) → DF(6) → FL(11) → UF
// B-face cycles: UB(0) → BL(9) → DB(4) → BR(8) → UB
//
// ORIENTATION CHANGES:
// U, D moves: No orientation changes (corners and edges stay at 0/1 orientation)
// L, R moves: Corner orientations increment by 1 (mod 3) going around cycle
// F, B moves: Corner orientations increment by 1 (mod 3) going around cycle
//             Edge orientations flip (0→1, 1→0) for edges in F/B cycles
// 
// This follows the "rotation about axis" rule:
// - U/D rotate about vertical axis → no corner twist
// - L/R rotate about left-right axis → corner twist
// - F/B rotate about front-back axis → corner twist AND edge flip

} // namespace cube_solver

#endif // CONSTANTS_H
