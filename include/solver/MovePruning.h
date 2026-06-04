/**
 * @file MovePruning.h
 * @brief Move pruning and redundancy elimination
 * 
 * Eliminates redundant move sequences to reduce search branching:
 * - Same face twice in a row (FF, UU, etc.) → combine into F2, U2
 * - Opposite faces with specific orderings (DU, RU, BF, etc.)
 * - Three-move cycles that are redundant
 */

#pragma once

#include <cstdint>
#include "cube/RubiksCube.h"

namespace cube_solver {

/**
 * @class MovePruning
 * @brief Utilities for move pruning in search
 */
class MovePruning {
public:
    /**
     * @brief Check if a move sequence is valid (not redundant)
     * @param prevFace Previous face moved (or INVALID if root)
     * @param prevType Previous move type (or Normal)
     * @param nextFace Next face to move
     * @param nextType Next move type
     * @return true if the sequence is not redundant
     * 
     * Rules:
     * 1. Same face + different types: U followed by U' or U2 is redundant
     *    (should be combined: U,U' → I; U,U2 → U'; U,U → U2)
     * 2. Opposite faces with ordering:
     *    - (U,D), (D,D), (L,R), (R,R), (F,B), (B,B)
     *    - By convention: always allow low→high, never high→low
     *    - Example: allow U→D but not D→U
     * 3. Three-move sequences that undo:
     *    - (U,D,U), (F,B,F) etc. can sometimes be reordered
     *    - For now: simple 2-move rule is sufficient
     */
    static bool isValidMoveSequence(Face prevFace, MoveType prevType,
                                     Face nextFace, MoveType nextType);
    
    /**
     * @brief Get the canonical (reduced) move sequence
     * @param move1 First move
     * @param move2 Second move (applied after move1)
     * @param result Output: sequence of moves after reduction
     * @return Number of resulting moves (0-2)
     * 
     * Example: (U, Normal) + (U, Normal) → (U, Double) [1 move]
     *          (U, Normal) + (U, Prime) → [] [0 moves - identity]
     *          (U, Prime) + (U, Normal) → (U, Double) [1 move]
     */
    static int reduceMovePair(Face face1, MoveType type1,
                               Face face2, MoveType type2,
                               std::pair<Face, MoveType>& result);
    
    static constexpr bool PRUNING_TABLE[18][18] = {
        // Pruning table: rows = prev move (Face * 3 + Type), cols = next move
        // Face enum: U=0, D=1, L=2, R=3, F=4, B=5
        // Type enum: Normal=0, Prime=1, Double=2
        // Rule: Reject same face (handled here)
        // Rule: Reject opposite face if prev > next (e.g. D rejects U, R rejects L, B rejects F)
        
        // Row 0-2: prev = U (allows D)
        {false, false, false,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true},
        {false, false, false,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true},
        {false, false, false,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true},
        
        // Row 3-5: prev = D (rejects U to avoid D->U / U->D duplicate)
        {false, false, false,  false, false, false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true},
        {false, false, false,  false, false, false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true},
        {false, false, false,  false, false, false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true},
        
        // Row 6-8: prev = L (allows R)
        {true,  true,  true,   true,  true,  true,  false, false, false, true,  true,  true,  true,  true,  true,  true,  true,  true},
        {true,  true,  true,   true,  true,  true,  false, false, false, true,  true,  true,  true,  true,  true,  true,  true,  true},
        {true,  true,  true,   true,  true,  true,  false, false, false, true,  true,  true,  true,  true,  true,  true,  true,  true},
        
        // Row 9-11: prev = R (rejects L to avoid R->L / L->R duplicate)
        {true,  true,  true,   true,  true,  true,  false, false, false, false, false, false, true,  true,  true,  true,  true,  true},
        {true,  true,  true,   true,  true,  true,  false, false, false, false, false, false, true,  true,  true,  true,  true,  true},
        {true,  true,  true,   true,  true,  true,  false, false, false, false, false, false, true,  true,  true,  true,  true,  true},
        
        // Row 12-14: prev = F (allows B)
        {true,  true,  true,   true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  true,  true},
        {true,  true,  true,   true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  true,  true},
        {true,  true,  true,   true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  true,  true},
        
        // Row 15-17: prev = B (rejects F to avoid B->F / F->B duplicate)
        {true,  true,  true,   true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false, false},
        {true,  true,  true,   true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false, false},
        {true,  true,  true,   true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false, false},
    };
    
    /**
     * @brief Get index into pruning table for a face/type pair
     */
    static inline int getPruningIndex(Face face, MoveType type) {
        return static_cast<int>(face) * 3 + static_cast<int>(type);
    }
};

} // namespace cube_solver
