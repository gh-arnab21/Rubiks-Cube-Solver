#include "cube/RubiksCube.h"
#include "database/RankCalculator.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <ostream>

namespace cube_solver {

// ============ INITIALIZATION ============

RubiksCube::RubiksCube() {
    state.reset();
}

RubiksCube::RubiksCube(const CubieCube& cubieState) : state(cubieState) {
}

void RubiksCube::reset() {
    state.reset();
    moveHistory.clear();
}

// ============ MOVE OPERATIONS ============

void RubiksCube::applyMove(Face face, MoveType moveType) {
    // TODO: Apply move and track in history
    state.applyMove(face, moveType);
    moveHistory.push_back({face, moveType});
}

bool RubiksCube::applyMoveSequence(const std::string& moveSequence) {
    // TODO: Parse and apply move sequence
    // Format: "R U R' U' R U2 R'" (space-separated)
    // Each move: [U|L|F|R|B|D][optional: '|2]
    //
    // Implementation:
    // 1. Split by whitespace
    // 2. For each token:
    //    a. Parse face letter (first char)
    //    b. Check for ' or 2 modifier
    //    c. Call applyMove
    // 3. Return false if any parsing fails
    
    std::istringstream iss(moveSequence);
    std::string moveStr;
    
    while (iss >> moveStr) {
        Face face;
        MoveType moveType;
        
        if (!parseMove(moveStr, face, moveType)) {
            return false;  // Invalid move
        }
        
        applyMove(face, moveType);
    }
    
    return true;
}

void RubiksCube::scramble(uint32_t numMoves) {
    // TODO: Generate random scramble avoiding consecutive redundant moves
    // 
    // Move pruning rules to implement:
    // 1. Can't repeat same face (e.g., R R is invalid, must be R2 or R followed by different face)
    // 2. Can avoid some commutative redundancy (e.g., prefer R U over U R for generation)
    //
    // Algorithm:
    // 1. Initialize lastFace = -1 (no last face)
    // 2. For i = 0 to numMoves-1:
    //    a. Generate random face != lastFace
    //    b. Generate random moveType (Normal/Prime/Double)
    //    c. Apply move
    //    d. Update lastFace
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> faceDist(0, 5);
    std::uniform_int_distribution<> typeDist(0, 2);
    
    Face lastFace = Face::U;  // Start with arbitrary face
    
    for (uint32_t i = 0; i < numMoves; ++i) {
        Face nextFace;
        do {
            nextFace = static_cast<Face>(faceDist(gen));
        } while (nextFace == lastFace);  // Avoid repeating same face
        
        MoveType moveType = static_cast<MoveType>(typeDist(gen));
        applyMove(nextFace, moveType);
        lastFace = nextFace;
    }
}

// ============ STATE QUERIES ============

bool RubiksCube::isSolved() const {
    return state.isSolved();
}

// ============ HEURISTIC & SOLVER INTEGRATION ============

uint64_t RubiksCube::getCornerRank() const {
    // TODO: Implement corner state ranking
    // Use RankCalculator::rankCornerState
    return RankCalculator::rankCornerState(state);
}

uint64_t RubiksCube::getEdgeRank(const std::array<uint8_t, 7>& edgeIndices) const {
    // TODO: Implement 7-edge state ranking
    // Use RankCalculator::rankEdgeState
    return RankCalculator::rankEdgeState(state, edgeIndices);
}

uint64_t RubiksCube::getEdgeOrientationRank() const {
    // TODO: Implement edge orientation ranking
    // Extract edge orientations and rank them
    // This is used when separating orientation and permutation databases
    
    auto orientations = state.getEdgeOrientations();
    std::vector<uint8_t> ori(orientations.begin(), orientations.end());
    return RankCalculator::rankOrientations(ori, 1);  // 1 = max orientation for edges
}

// ============ MOVE HISTORY ============

std::string RubiksCube::getMoveHistoryString() const {
    // TODO: Convert move history to standard notation string
    // Format: "R U R' U' R U2 R'" (space-separated)
    //
    // Implementation:
    // For each move in moveHistory:
    //   1. Convert face to character (U,L,F,R,B,D)
    //   2. Add modifier: nothing for Normal, ' for Prime, 2 for Double
    //   3. Add space (except last)
    
    std::string result;
    for (size_t i = 0; i < moveHistory.size(); ++i) {
        if (i > 0) result += " ";
        
        // Convert face to character
        char faceChar;
        switch (moveHistory[i].first) {
            case Face::U: faceChar = 'U'; break;
            case Face::L: faceChar = 'L'; break;
            case Face::F: faceChar = 'F'; break;
            case Face::R: faceChar = 'R'; break;
            case Face::B: faceChar = 'B'; break;
            case Face::D: faceChar = 'D'; break;
            default: faceChar = '?'; break;
        }
        result += faceChar;
        
        // Add move type modifier
        switch (moveHistory[i].second) {
            case MoveType::Normal:
                break;  // No modifier
            case MoveType::Prime:
                result += '\'';
                break;
            case MoveType::Double:
                result += '2';
                break;
        }
    }
    
    return result;
}

// ============ HELPER FUNCTIONS ============

bool RubiksCube::parseMove(const std::string& moveStr, Face& face, MoveType& moveType) {
    // TODO: Parse move notation (e.g., "R", "U'", "F2")
    //
    // Algorithm:
    // 1. Check length (1-2 characters)
    // 2. First character is face: U/L/F/R/B/D
    // 3. If length > 1:
    //    - If second char is ' -> Prime move
    //    - If second char is 2 -> Double move
    //    - Otherwise -> Invalid
    // 4. Return true on success, false on invalid input
    
    if (moveStr.empty() || moveStr.length() > 2) {
        return false;  // Invalid length
    }
    
    // Parse face
    switch (moveStr[0]) {
        case 'U': face = Face::U; break;
        case 'L': face = Face::L; break;
        case 'F': face = Face::F; break;
        case 'R': face = Face::R; break;
        case 'B': face = Face::B; break;
        case 'D': face = Face::D; break;
        default: return false;  // Invalid face
    }
    
    // Parse move type
    moveType = MoveType::Normal;  // Default
    if (moveStr.length() == 2) {
        switch (moveStr[1]) {
            case '\'':
                moveType = MoveType::Prime;
                break;
            case '2':
                moveType = MoveType::Double;
                break;
            default:
                return false;  // Invalid modifier
        }
    }
    
    return true;
}

// ============ OUTPUT ============

std::ostream& operator<<(std::ostream& os, const RubiksCube& cube) {
    os << "RubiksCube State:\n";
    os << "Move Count: " << cube.moveHistory.size() << "\n";
    os << "Move History: " << cube.getMoveHistoryString() << "\n";
    os << "Solved: " << (cube.isSolved() ? "YES" : "NO") << "\n";
    os << "\nCube State:\n" << cube.state;
    return os;
}

} // namespace cube_solver
