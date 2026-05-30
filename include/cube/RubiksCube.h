#ifndef RUBIKS_CUBE_H
#define RUBIKS_CUBE_H

#include "CubieCube.h"
#include <vector>
#include <string>
#include <memory>

/**
 * @file RubiksCube.h
 * @brief High-level interface for Rubik's Cube operations
 * 
 * This class wraps CubieCube and provides:
 * - Move sequencing (e.g., "R U R' U'")
 * - Move notation parsing
 * - Scrambling
 * - State serialization/deserialization
 * - Heuristic evaluation hooks
 */

namespace cube_solver {

/**
 * @class RubiksCube
 * @brief High-level interface for cube manipulation and queries
 */
class RubiksCube {
public:
    // ============ INITIALIZATION ============
    
    /**
     * @brief Default constructor - creates solved cube
     */
    RubiksCube();
    
    /**
     * @brief Copy constructor
     */
    RubiksCube(const RubiksCube& other) = default;
    
    /**
     * @brief Create from CubieCube state
     */
    explicit RubiksCube(const CubieCube& state);
    
    /**
     * @brief Reset to solved state
     */
    void reset();
    
    // ============ MOVE OPERATIONS ============
    
    /**
     * @brief Apply a single move by face and type
     * @param face The face to twist
     * @param moveType The type of move
     */
    void applyMove(Face face, MoveType moveType);
    
    /**
     * @brief Apply a sequence of moves from a string
     * @param moveSequence String like "R U R' U' R U2 R'" (space-separated)
     * 
     * Logic to implement:
     * - Parse string into individual moves
     * - Each move is a letter (U,L,F,R,B,D) optionally followed by ' or 2
     * - Apply moves in sequence
     * - Return true if parsing succeeded, false if invalid
     */
    bool applyMoveSequence(const std::string& moveSequence);
    
    /**
     * @brief Apply a scramble of random moves
     * @param numMoves Number of random moves to apply
     * 
     * Logic to implement:
     * - Generate random valid moves (considering move pruning rules)
     * - Apply each move
     * - Avoid redundant consecutive moves (same face, or commutative)
     */
    void scramble(uint32_t numMoves);
    
    // ============ STATE QUERIES ============
    
    /**
     * @brief Check if cube is solved
     */
    bool isSolved() const;
    
    /**
     * @brief Get underlying CubieCube state
     */
    const CubieCube& getState() const { return state; }
    
    /**
     * @brief Get mutable reference to state
     */
    CubieCube& getState() { return state; }
    
    // ============ HEURISTIC & SOLVER INTEGRATION ============
    
    /**
     * @brief Compute corner permutation rank for pattern database
     * @return Rank in range [0, 8!*3^7)
     * 
     * Logic to implement:
     * - Extract corner permutation
     * - Extract corner orientations
     * - Convert both to ranks using factorial number system
     * - Combine into single rank value
     * 
     * References:
     * - Korf's paper on factorial number system
     * - Wikipedia: Lehmer code for permutation ranking
     */
    uint64_t getCornerRank() const;
    
    /**
     * @brief Compute edge permutation rank for 7-edge pattern database
     * @param edgeIndices Indices of the 7 edges to consider (e.g., {0,1,2,3,4,5,6})
     * @return Rank in range [0, 12P7*2^7)
     * 
     * Logic to implement:
     * - Extract specified 7 edge positions from the full edge array
     * - Extract orientations of those 7 edges
     * - Convert permutation to rank (partial permutation - only 7 of 12)
     * - Convert orientations to rank
     * - Combine ranks
     * 
     * This is more complex than corner ranking because we're selecting
     * 7 edges out of 12 positions (not all 12)
     */
    uint64_t getEdgeRank(const std::array<uint8_t, 7>& edgeIndices) const;
    
    /**
     * @brief Compute edge orientation only (no permutation)
     * @return Rank for edge orientation database
     * 
     * Some algorithms use separate databases for orientation vs permutation
     */
    uint64_t getEdgeOrientationRank() const;
    
    // ============ MOVE HISTORY / SOLUTION TRACKING ============
    
    /**
     * @brief Get move history as vector
     */
    const std::vector<std::pair<Face, MoveType>>& getMoveHistory() const {
        return moveHistory;
    }
    
    /**
     * @brief Clear move history
     */
    void clearMoveHistory() { moveHistory.clear(); }
    
    /**
     * @brief Convert move history to string representation
     */
    std::string getMoveHistoryString() const;
    
    /**
     * @brief Get number of moves applied
     */
    uint32_t getMoveCount() const { return moveHistory.size(); }
    
    // ============ UTILITY ============
    
    /**
     * @brief Output stream operator
     */
    friend std::ostream& operator<<(std::ostream& os, const RubiksCube& cube);
    
private:
    // ============ INTERNAL STATE ============
    
    /**
     * @brief Core cube representation using cubies
     */
    CubieCube state;
    
    /**
     * @brief History of moves applied for solution tracking
     * 
     * TODO: Consider if this should be in the solver instead
     * This is useful for:
     * - Tracking solution path
     * - Verifying optimality
     * - Undoing moves
     * - Move sequence generation
     */
    std::vector<std::pair<Face, MoveType>> moveHistory;
    
    // ============ HELPER FUNCTIONS ============
    
    /**
     * @brief Parse a single move notation string (e.g., "R", "U'", "F2")
     * @param moveStr The move string
     * @param face Output parameter for parsed face
     * @param moveType Output parameter for parsed move type
     * @return true if parsing succeeded
     */
    static bool parseMove(const std::string& moveStr, Face& face, MoveType& moveType);
};

} // namespace cube_solver

#endif // RUBIKS_CUBE_H
