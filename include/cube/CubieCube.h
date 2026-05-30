#ifndef CUBIE_CUBE_H
#define CUBIE_CUBE_H

#include <cstdint>
#include <array>
#include <ostream>

/**
 * @file CubieCube.h
 * @brief Core representation of a Rubik's Cube using cubie arrays
 * 
 * This file defines:
 * - Cubie struct: represents a single cubie with position and orientation
 * - CubieCube class: represents the complete state using two arrays
 *   - cornerCubies[8]: corner pieces with indices 0-7 and orientations 0-2
 *   - edgeCubies[12]: edge pieces with indices 0-11 and orientations 0-1
 * 
 * Design choice: Separate arrays for corners and edges for:
 * - Clarity and ease of manipulation
 * - Efficient pattern database indexing (can compute corner and edge hashes separately)
 * - Allows future decomposition strategies (Subgroup/Two-Phase algorithms)
 */

#include "utils/Constants.h"

namespace cube_solver {

/**
 * @struct Cubie
 * @brief Represents a single cubie (corner or edge piece)
 */
struct Cubie {
    uint8_t position;      ///< Where this cubie is located (0-7 for corners, 0-11 for edges)
    uint8_t orientation;   ///< How it's oriented (0-2 for corners, 0-1 for edges)
    
    Cubie() = default;
    Cubie(uint8_t pos, uint8_t ori) : position(pos), orientation(ori) {}
    
    bool operator==(const Cubie& other) const {
        return position == other.position && orientation == other.orientation;
    }
};

/**
 * @class CubieCube
 * @brief Represents the complete state of a Rubik's Cube using two arrays
 * 
 * Architecture:
 * - Stores corner cubies in a separate array from edge cubies
 * - Each array element contains (position, orientation) pair
 * - Solved state: cubies are in their "home" positions with orientation 0
 * 
 * Usage pattern:
 * 1. Initialize with solved state (identity)
 * 2. Apply moves to transform state
 * 3. Use state for pattern database lookups
 * 4. Check if solved (all cubies in home position with orientation 0)
 */
class CubieCube {
public:
    // ============ INITIALIZATION ============
    
    /**
     * @brief Default constructor - initializes to solved state
     */
    CubieCube();
    
    /**
     * @brief Copy constructor
     */
    CubieCube(const CubieCube& other) = default;
    
    /**
     * @brief Assignment operator
     */
    CubieCube& operator=(const CubieCube& other) = default;
    
    /**
     * @brief Initialize to solved state (identity element)
     * 
     * Logic to implement:
     * - Set each corner cubie's position to its index (0-7)
     * - Set each corner cubie's orientation to 0
     * - Set each edge cubie's position to its index (0-11)
     * - Set each edge cubie's orientation to 0
     */
    void reset();
    
    // ============ MOVE OPERATIONS ============
    
    /**
     * @brief Apply a single face twist to the cube
     * @param face The face to twist (U, L, F, R, B, D)
     * @param moveType The type of move (Normal 90°, Prime 270°, Double 180°)
     * 
     * Logic to implement:
     * - Permute corner cubies according to face rotation
     * - Update corner orientations (rotation about axis perpendicular to face)
     * - Permute edge cubies according to face rotation
     * - Update edge orientations (flip if applicable)
     * 
     * Each face rotation moves 4 corners and 4 edges in a cycle.
     * Orientations change differently per face:
     * - U, D moves: don't change corner/edge orientations
     * - L, R moves: change corner orientation (twist around vertical axis)
     * - F, B moves: change both corner and edge orientations
     * 
     * TODO: Implement rotation tables for each face
     * Consider precomputing move tables: moveTable[face][moveType]
     */
    void applyMove(Face face, MoveType moveType);
    
    /**
     * @brief Apply a single 90-degree clockwise twist of a face
     * @param face The face to twist
     * 
     * Helper function for applyMove. Implement this first, then use it
     * for prime and double moves via repeated application.
     * 
     * Implementation approach:
     * 1. Create temporary copies of affected cubies
     * 2. Rotate positions in a cycle (4 cubies per face)
     * 3. Update orientations based on face type
     * 4. Write back to cube state
     */
    void applySingleMove(Face face);
    
    /**
     * @brief Apply inverse move (undo operation)
     * @param face The face to twist
     * @param moveType The type of move to invert
     * 
     * Can be implemented as applying prime move 3 times (inefficient)
     * or creating inverse move table (efficient)
     */
    void applyInverseMove(Face face, MoveType moveType);
    
    // ============ STATE QUERIES ============
    
    /**
     * @brief Check if the cube is in solved state
     * @return true if all cubies are in home positions with orientation 0
     * 
     * Logic to implement:
     * - Check each corner: position == index && orientation == 0
     * - Check each edge: position == index && orientation == 0
     */
    bool isSolved() const;
    
    /**
     * @brief Get the current permutation state of corners
     * @return Array of corner positions
     */
    std::array<uint8_t, NUM_CORNERS> getCornerPermutation() const;
    
    /**
     * @brief Get the current orientation state of corners
     * @return Array of corner orientations
     */
    std::array<uint8_t, NUM_CORNERS> getCornerOrientations() const;
    
    /**
     * @brief Get the current permutation state of edges
     * @return Array of edge positions
     */
    std::array<uint8_t, NUM_EDGES> getEdgePermutation() const;
    
    /**
     * @brief Get the current orientation state of edges
     * @return Array of edge orientations
     */
    std::array<uint8_t, NUM_EDGES> getEdgeOrientations() const;
    
    /**
     * @brief Get corner cubie at a specific position
     */
    const Cubie& getCornerCubie(uint8_t position) const {
        return cornerCubies[position];
    }

    /**
     * @brief Get mutable corner cubie reference
     */
    Cubie& getCornerCubie(uint8_t position) {
        return cornerCubies[position];
    }
    
    /**
     * @brief Get edge cubie at a specific position
     */
    const Cubie& getEdgeCubie(uint8_t position) const {
        return edgeCubies[position];
    }

    /**
     * @brief Get mutable edge cubie reference
     */
    Cubie& getEdgeCubie(uint8_t position) {
        return edgeCubies[position];
    }
    
    // ============ UTILITY ============
    
    /**
     * @brief Check equality
     */
    bool operator==(const CubieCube& other) const;
    
    /**
     * @brief Check inequality
     */
    bool operator!=(const CubieCube& other) const {
        return !(*this == other);
    }
    
    /**
     * @brief Output stream operator for debugging
     */
    friend std::ostream& operator<<(std::ostream& os, const CubieCube& cube);
    
private:
    // ============ INTERNAL STATE ============
    
    /**
     * @brief Corner cubies array
     * 
     * Index represents the position on the cube (0-7 corresponding to CornerPos enum)
     * Value contains the cubie ID and orientation at that position
     * 
     * Solved state example:
     * cornerCubies[ULB] = Cubie(position=0, orientation=0)  // ULB cubie at ULB position
     * cornerCubies[URB] = Cubie(position=1, orientation=0)  // URB cubie at URB position
     * ...
     * 
     * Scrambled state example:
     * cornerCubies[ULB] = Cubie(position=2, orientation=1)  // URF cubie at ULB position, twisted
     * ...
     */
    std::array<Cubie, NUM_CORNERS> cornerCubies;
    
    /**
     * @brief Edge cubies array
     * 
     * Index represents the position on the cube (0-11 corresponding to EdgePos enum)
     * Value contains the cubie ID and orientation at that position
     * 
     * Similar structure to cornerCubies
     */
    std::array<Cubie, NUM_EDGES> edgeCubies;
};

} // namespace cube_solver

#endif // CUBIE_CUBE_H
