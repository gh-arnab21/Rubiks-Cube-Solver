/**
 * @file RankCalculator.cpp
 * @brief Rank calculation for cube states
 * 
 * Implements:
 * - Lehmer code (factorial number system) for permutation ranking
 * - Base-N representation for orientation ranking
 * - Combined corner/edge state ranking for pattern database indexing
 * 
 * Following Korf's IDA* and Benbotto's reference implementation
 */

#include "database/RankCalculator.h"
#include <algorithm>

namespace cube_solver {

// ============ STATIC DEFINITIONS ============

constexpr uint64_t RankCalculator::FACTORIALS[];

// ============ PERMUTATION RANKING (LEHMER CODE) ============

uint64_t RankCalculator::rankPermutation(const std::vector<uint8_t>& permutation) {
    /**
     * Rank a permutation using Lehmer code (factorial number system)
     * 
     * Algorithm (Korf's inversion vector method, O(N²)):
     * For each position i:
     *   - Count how many smaller values appear to the RIGHT
     *   - This count is the factorial base digit at position i
     * Combined: rank = sum(digit[i] * (N-1-i)!)
     * 
     * Example: [2, 0, 1]
     *   Position 0 (val 2): 2 smaller to right → digit 2
     *   Position 1 (val 0): 0 smaller to right → digit 0
     *   Position 2 (val 1): 0 smaller to right → digit 0
     *   Rank = 2*2! + 0*1! + 0*0! = 4
     */
    
    if (permutation.empty()) return 0;
    
    size_t n = permutation.size();
    uint64_t rank = 0;
    
    for (size_t i = 0; i < n; ++i) {
        uint8_t inversions = 0;
        for (size_t j = i + 1; j < n; ++j) {
            if (permutation[j] < permutation[i]) {
                inversions++;
            }
        }
        rank += inversions * factorial(n - i - 1);
    }
    
    return rank;
}

std::vector<uint8_t> RankCalculator::unrankPermutation(uint64_t rank, size_t size) {
    /**
     * Generate permutation from rank (inverse of rankPermutation)
     */
    
    std::vector<uint8_t> result(size);
    std::vector<uint8_t> available(size);
    
    for (uint8_t i = 0; i < size; ++i) {
        available[i] = i;
    }
    
    for (size_t i = 0; i < size; ++i) {
        uint64_t fact = factorial(size - i - 1);
        uint8_t index = (fact > 0) ? (rank / fact) : 0;
        rank %= fact;
        
        if (index < available.size()) {
            result[i] = available[index];
            available.erase(available.begin() + index);
        }
    }
    
    return result;
}

// ============ ORIENTATION RANKING (BASE-N) ============

uint64_t RankCalculator::rankOrientations(const std::vector<uint8_t>& orientations, uint8_t maxOrientation) {
    /**
     * Rank orientations using base-N representation
     * 
     * Only use first (N-1) orientations - last is determined by constraint:
     * - Corners: sum ≡ 0 (mod 3)
     * - Edges: XOR ≡ 0 (mod 2)
     */
    
    if (orientations.size() < 2) return 0;
    
    uint8_t base = maxOrientation + 1;
    uint64_t rank = 0;
    uint64_t basePower = 1;
    
    for (size_t i = 0; i < orientations.size() - 1; ++i) {
        rank += orientations[i] * basePower;
        basePower *= base;
    }
    
    return rank;
}

std::vector<uint8_t> RankCalculator::unrankOrientations(uint64_t rank, size_t size, uint8_t maxOrientation) {
    /**
     * Generate orientations from rank (inverse of rankOrientations)
     */
    
    std::vector<uint8_t> result(size);
    uint8_t base = maxOrientation + 1;
    
    for (size_t i = 0; i < size - 1; ++i) {
        result[i] = rank % base;
        rank /= base;
    }
    
    // Calculate last orientation from constraint
    uint8_t sum = 0;
    for (size_t i = 0; i < size - 1; ++i) {
        sum += result[i];
    }
    result[size - 1] = (base - (sum % base)) % base;
    
    return result;
}

// ============ CORNER STATE RANKING ============

uint64_t RankCalculator::rankCornerState(const CubieCube& cube) {
    /**
     * Rank complete corner state (permutation + orientation)
     * 
     * Size: 8! × 3^7 = 40,320 × 2,187 = 88,179,840 ≈ 88M states
     * Storage with nibbles: 44 MB
     * 
     * rank = perm_rank + ori_rank * 8!
     */
    
    auto cornerPerm = cube.getCornerPermutation();
    auto cornerOri = cube.getCornerOrientations();
    
    std::vector<uint8_t> permVec(cornerPerm.begin(), cornerPerm.end());
    std::vector<uint8_t> oriVec(cornerOri.begin(), cornerOri.end());
    
    uint64_t permRank = rankPermutation(permVec);
    uint64_t oriRank = rankOrientations(oriVec, 2);  // max orientation 2 for corners
    
    uint64_t eightFactorial = factorial(8);
    return permRank + oriRank * eightFactorial;
}

// ============ EDGE PERMUTATION RANKING ============

uint64_t RankCalculator::rankEdgePermutation(const CubieCube& cube) {
    /**
     * Rank all 12 edge permutations
     * 
     * Size: 12! / 2 = 239M (parity constraint)
     * Storage: 119 MB per nibble
     */
    
    auto edgePerm = cube.getEdgePermutation();
    std::vector<uint8_t> permVec(edgePerm.begin(), edgePerm.end());
    
    uint64_t rank = rankPermutation(permVec);
    return rank / 2;  // Account for parity
}

// ============ PARTIAL PERMUTATION RANKING ============

uint64_t RankCalculator::rankPartialEdgePermutation(const CubieCube& cube, const std::array<uint8_t, 7>& edgeIndices) {
    /**
     * Rank a 7-edge subset permutation
     * 
     * Size: 12P7 = 479,001,600 ≈ 479M states
     * Storage: 250 MB per database
     */
    
    auto edgePerm = cube.getEdgePermutation();
    
    std::vector<uint8_t> selectedPositions;
    for (uint8_t idx : edgeIndices) {
        selectedPositions.push_back(edgePerm[idx]);
    }
    
    return rankPermutation(selectedPositions);
}

// ============ EDGE STATE RANKING ============

uint64_t RankCalculator::rankEdgeState(const CubieCube& cube, const std::array<uint8_t, 7>& edgeIndices) {
    /**
     * Rank complete 7-edge state (permutation + orientation)
     * 
     * Size: 12P7 × 2^7 = 479M × 128 ≈ 61B
     * Storage with nibbles: ~128 MB per database
     */
    
    uint64_t permRank = rankPartialEdgePermutation(cube, edgeIndices);
    
    auto edgeOri = cube.getEdgeOrientations();
    std::vector<uint8_t> selectedOrientations;
    for (uint8_t idx : edgeIndices) {
        selectedOrientations.push_back(edgeOri[idx]);
    }
    
    uint64_t oriRank = rankOrientations(selectedOrientations, 1);  // max orientation 1 for edges
    
    // P(12,7) = 12*11*10*9*8*7*6
    uint64_t p127 = permutation(12, 7);
    return permRank + oriRank * p127;
}

// ============ UTILITY FUNCTIONS ============

uint64_t RankCalculator::factorial(uint8_t n) {
    if (n > 12) return 0;
    return FACTORIALS[n];
}

uint64_t RankCalculator::binomial(uint8_t n, uint8_t k) {
    /**
     * Compute C(n, k) = n! / (k! * (n-k)!)
     */
    if (k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n - k) k = n - k;
    
    uint64_t result = 1;
    for (uint8_t i = 0; i < k; ++i) {
        result *= (n - i);
        result /= (i + 1);
    }
    return result;
}

uint64_t RankCalculator::permutation(uint8_t n, uint8_t k) {
    /**
     * Compute P(n, k) = n! / (n-k)!
     * 
     * Example: P(12, 7) = 12 * 11 * 10 * 9 * 8 * 7 * 6
     */
    if (k > n) return 0;
    
    uint64_t result = 1;
    for (uint8_t i = 0; i < k; ++i) {
        result *= (n - i);
    }
    return result;
}

} // namespace cube_solver
