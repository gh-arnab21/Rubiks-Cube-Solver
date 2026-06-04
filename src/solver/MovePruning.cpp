/**
 * @file MovePruning.cpp
 * @brief Move pruning implementation
 */

#include "solver/MovePruning.h"

namespace cube_solver {

constexpr bool MovePruning::PRUNING_TABLE[18][18];

bool MovePruning::isValidMoveSequence(Face prevFace, MoveType prevType,
                                       Face nextFace, MoveType nextType) {
    // Use pruning table for all rules
    int prevIdx = getPruningIndex(prevFace, prevType);
    int nextIdx = getPruningIndex(nextFace, nextType);
    
    return PRUNING_TABLE[prevIdx][nextIdx];
}

int MovePruning::reduceMovePair(Face face1, MoveType type1,
                                 Face face2, MoveType type2,
                                 std::pair<Face, MoveType>& result) {
    // If same face, combine
    if (face1 == face2) {
        if (type1 == MoveType::Normal && type2 == MoveType::Normal) {
            // U + U = U2
            result = {face1, MoveType::Double};
            return 1;
        } else if (type1 == MoveType::Normal && type2 == MoveType::Prime) {
            // U + U' = identity
            return 0;
        } else if (type1 == MoveType::Normal && type2 == MoveType::Double) {
            // U + U2 = U'
            result = {face1, MoveType::Prime};
            return 1;
        } else if (type1 == MoveType::Prime && type2 == MoveType::Normal) {
            // U' + U = identity
            return 0;
        } else if (type1 == MoveType::Prime && type2 == MoveType::Prime) {
            // U' + U' = U2
            result = {face1, MoveType::Double};
            return 1;
        } else if (type1 == MoveType::Prime && type2 == MoveType::Double) {
            // U' + U2 = U
            result = {face1, MoveType::Normal};
            return 1;
        } else if (type1 == MoveType::Double && type2 == MoveType::Normal) {
            // U2 + U = U'
            result = {face1, MoveType::Prime};
            return 1;
        } else if (type1 == MoveType::Double && type2 == MoveType::Prime) {
            // U2 + U' = U
            result = {face1, MoveType::Normal};
            return 1;
        } else if (type1 == MoveType::Double && type2 == MoveType::Double) {
            // U2 + U2 = identity
            return 0;
        }
    }
    
    // Different faces - return both
    // (would need to return 2 moves, but this function is for pair reduction)
    // For now, just return 0 to indicate "don't reduce"
    return 0;
}

} // namespace cube_solver
