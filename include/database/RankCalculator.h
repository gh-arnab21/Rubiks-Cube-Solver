#ifndef RANK_CALCULATOR_H
#define RANK_CALCULATOR_H

#include <cstdint>
#include <array>
#include <vector>

/**
 * @file RankCalculator.h
 * @brief Rank calculation for converting cube states to database indices
 * 
 * Core problem:
 * A Rubik's Cube state needs to be converted to an integer index [0, database_size)
 * to lookup heuristic values in pattern databases.
 * 
 * Solution: Factorial Number System (Lehmer Code)
 * - Maps permutations to sequential ranks
 * - Maps orientation sequences to sequential ranks
 * - Korf's paper describes this in detail
 * 
 * Example:
 * The 8! = 40,320 corner permutations can be uniquely ranked 0-40,319
 * The 3^7 = 2,187 corner orientation combinations can be ranked 0-2,186
 * Combined: corners have 88,179,840 possible states
 * 
 * Complex case (Partial Permutations):
 * Ranking 7 edges selected from 12 positions is trickier.
 * Use combination indexing: which 7 edges are these among 12?
 * Then rank the permutation of those 7.
 * 
 * References:
 * - Korf's "Large-Scale Parallel Breadth-First Search" paper (AAAI 2005)
 *   Describes linear-time permutation ranking using "inversion vectors"
 * - Wikipedia: Lehmer code, Factorial number system
 * - Peter Taylor's Stack Exchange answer on edge permutation indexing
 */

#include "cube/CubieCube.h"
#include "utils/Constants.h"

namespace cube_solver {

/**
 * @class RankCalculator
 * @brief Static utility class for converting cube states to ranks
 */
class RankCalculator {
public:
    // ============ FULL PERMUTATION RANKING ============
    
    /**
     * @brief Rank a permutation using factorial number system
     * @param permutation Array of values 0-N-1 in some order
     * @param size Size of permutation
     * @return Rank in range [0, size!)
     * 
     * Algorithm (Lehmer code):
     * For each position i from 0 to N-1:
     *   Count how many values to the right are smaller than current value
     *   This count is the digit at position i in factorial base
     * 
     * Time: O(N^2) naive, O(N) with inversion vector optimization
     * 
     * Implementation approach:
     * 1. Use inversion vector (Korf's method for O(N) time)
     * 2. For each position, compute "inversion" (count of inversions at that point)
     * 3. Accumulate rank = sum of (inversion[i] * i!)
     */
    static uint64_t rankPermutation(const std::vector<uint8_t>& permutation);
    
    /**
     * @brief Unrank a permutation from its rank (inverse operation)
     * @param rank The rank value
     * @param size Size of permutation
     * @return The permutation corresponding to this rank
     * 
     * TODO: Implement inverse of rankPermutation
     */
    static std::vector<uint8_t> unrankPermutation(uint64_t rank, size_t size);
    
    // ============ ORIENTATION RANKING ============
    
    /**
     * @brief Rank an orientation configuration
     * @param orientations Array of orientation values (0-1 for edges, 0-2 for corners)
     * @param maxOrientation Maximum orientation value (2 for corners, 1 for edges)
     * @return Rank in range [0, maxOrientation^(size-1))
     * 
     * Key insight: The last orientation is redundant
     * - For corners: sum of 7 orientations mod 3 determines 8th
     * - For edges: XOR of 7 orientations determines 8th (mod 2)
     * 
     * Algorithm:
     * Only use first (size-1) orientations, treat as base-N number
     * rank = o[0] + o[1]*N + o[2]*N^2 + ... + o[N-2]*N^(N-2)
     * 
     * Time: O(N)
     */
    static uint64_t rankOrientations(const std::vector<uint8_t>& orientations, uint8_t maxOrientation);
    
    /**
     * @brief Unrank an orientation configuration
     * @param rank The rank value
     * @param size Size of orientation array
     * @param maxOrientation Maximum orientation value
     * @return The orientation array corresponding to this rank
     */
    static std::vector<uint8_t> unrankOrientations(uint64_t rank, size_t size, uint8_t maxOrientation);
    
    // ============ PARTIAL PERMUTATION RANKING ============
    
    /**
     * @brief Rank a partial permutation (k items from N positions)
     * @param positions Array where positions[i] tells us which position item i occupies
     * @param k Number of items
     * @param n Total number of positions
     * @return Rank in range [0, P(n,k) = n!/(n-k)!)
     * 
     * More complex than full permutation.
     * 
     * Example: rank the permutation of 7 edges among 12 positions
     * - First decide which 7 edges are involved: C(12,7) combinations
     * - Then rank the permutation of those 7: 7! permutations
     * - Total: C(12,7) * 7! = 3,991,680
     * 
     * Algorithm approach:
     * 1. Identify which k items are present (combination index)
     * 2. Rank the permutation of just those k items
     * 3. Combine: rank = combination_index + permutation_index * C(n,k)
     * 
     * TODO: Implement partial permutation ranking
     * References:
     * - Peter Taylor's Stack Exchange answer
     * - Korf's paper mentions this for edge databases
     */
    static uint64_t rankPartialPermutation(const std::vector<uint8_t>& positions,
                                          uint8_t k, uint8_t n);
    
    // ============ CORNER RANKING ============
    
    /**
     * @brief Rank corner permutation and orientation into single index
     * @param cube The cube state
     * @return Combined rank in range [0, 8! * 3^7)
     * 
     * Implementation:
     * 1. Extract corner positions and orientations
     * 2. Rank the permutation (8! possibilities)
     * 3. Rank the orientations (3^7 possibilities)
     * 4. Combine: combined_rank = permutation_rank + orientation_rank * 8!
     */
    static uint64_t rankCornerState(const CubieCube& cube);
    
    // ============ EDGE PERMUTATION RANKING ============
    
    /**
     * @brief Rank all 12 edge permutations
     * @param cube The cube state
     * @return Rank in range [0, 12!/2) ≈ 239 million
     * 
     * Accounts for parity constraint (only even permutations reachable)
     * Size: 239M states ≈ 119 MB storage with nibbles
     */
    static uint64_t rankEdgePermutation(const CubieCube& cube);
    
    /**
     * @brief Rank a 7-edge subset permutation
     * @param cube The cube state
     * @param edgeIndices Which 7 edges to rank (e.g., [0,1,2,3,4,5,6])
     * @return Rank in range [0, P(12,7)) ≈ 479 million
     * 
     * Used by Benbotto's strategy: 2 databases with different 7-edge subsets
     * Size: 479M states ≈ 250 MB per database with nibbles
     */
    static uint64_t rankPartialEdgePermutation(const CubieCube& cube, const std::array<uint8_t, 7>& edgeIndices);
    
    // ============ EDGE STATE RANKING ============
    
    /**
     * @brief Rank 7 edge permutation and orientation into single index
     * @param cube The cube state
     * @param edgeIndices Which 7 edges to consider (from 0-11)
     * @return Combined rank in range [0, P(12,7) * 2^7)
     * 
     * Implementation:
     * 1. Extract positions and orientations of specified edges
     * 2. Rank the partial permutation (P(12,7) possibilities)
     * 3. Rank the orientations (2^7 possibilities)
     * 4. Combine
     * 
     * This is the complex one! Partial permutations are tricky.
     */
    static uint64_t rankEdgeState(const CubieCube& cube, const std::array<uint8_t, 7>& edgeIndices);
    
    // ============ UTILITY FUNCTIONS ============
    
    /**
     * @brief Compute N!
     */
    static uint64_t factorial(uint8_t n);
    
    /**
     * @brief Compute nCk (binomial coefficient)
     */
    static uint64_t binomial(uint8_t n, uint8_t k);
    
    /**
     * @brief Compute nPk (permutations of k items from n)
     */
    static uint64_t permutation(uint8_t n, uint8_t k);
    
private:
    // TODO: Consider precomputing and caching factorial values
    static constexpr uint64_t FACTORIALS[] = {
        1LL,                    // 0!
        1LL,                    // 1!
        2LL,                    // 2!
        6LL,                    // 3!
        24LL,                   // 4!
        120LL,                  // 5!
        720LL,                  // 6!
        5040LL,                 // 7!
        40320LL,                // 8!
        362880LL,               // 9!
        3628800LL,              // 10!
        39916800LL,             // 11!
        479001600LL             // 12!
    };
};

} // namespace cube_solver

#endif // RANK_CALCULATOR_H
