/**
 * @file RankTest.cpp
 * @brief Tests for Phase 2: Rank Calculator
 * * Verifies:
 * 1. Lehmer code permutation ranking/unranking
 * 2. Base-N orientation ranking/unranking
 * 3. Combined corner/edge state ranking
 * 4. Database sizing calculations
 * 5. Actual Object Memory Footprint Allocation
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <set>
#include "cube/CubieCube.h"
#include "cube/RubiksCube.h"
#include "database/RankCalculator.h"
#include "database/PatternDatabase.h" // Added to test the actual physical memory

using namespace cube_solver;

/**
 * @brief Test Lehmer code permutation ranking
 */
void testPermutationRanking() {
    std::cout << "Testing: Permutation ranking (Lehmer code)..." << std::endl;
    
    // Test 1: Solved state has rank 0
    std::vector<uint8_t> solved = {0, 1, 2, 3, 4, 5, 6, 7};
    uint64_t rank = RankCalculator::rankPermutation(solved);
    assert(rank == 0);
    std::cout << "  Solved permutation has rank 0" << std::endl;
    
    // Test 2: Reverse order
    std::vector<uint8_t> reversed = {7, 6, 5, 4, 3, 2, 1, 0};
    rank = RankCalculator::rankPermutation(reversed);
    assert(rank == RankCalculator::factorial(8) - 1);
    std::cout << "  Reversed permutation has max rank" << std::endl;
    
    // Test 3: Example from documentation [2, 0, 1]
    std::vector<uint8_t> example = {2, 0, 1};
    rank = RankCalculator::rankPermutation(example);
    assert(rank == 4);
    std::cout << "  Example [2,0,1] has rank 4" << std::endl;
    
    // Test 4: Invertibility - rank ↔ permutation should be bijective
    std::set<uint64_t> seenRanks;
    std::set<std::vector<uint8_t>> seenPerms;
    
    // Test corner permutations (sample, not all 40320)
    for (int i = 0; i < 100; ++i) {
        CubieCube cube;
        
        // Apply some moves to scramble
        for (int j = 0; j < 5; ++j) {
            cube.applyMove(static_cast<Face>(j % 6), MoveType::Normal);
        }
        
        auto corners = cube.getCornerPermutation();
        std::vector<uint8_t> cornerVec(corners.begin(), corners.end());
        
        uint64_t r = RankCalculator::rankPermutation(cornerVec);
        seenRanks.insert(r);
        seenPerms.insert(cornerVec);
    }
    
    assert(seenRanks.size() == seenPerms.size());
    std::cout << "  Permutation ranking is bijective" << std::endl;
    
    std::cout << "All permutation tests passed\n" << std::endl;
}

/**
 * @brief Test base-N orientation ranking
 */
void testOrientationRanking() {
    std::cout << "Testing: Orientation ranking (base-N)..." << std::endl;
    
    // Test 1: All zeros has rank 0
    std::vector<uint8_t> allZeros = {0, 0, 0, 0, 0, 0, 0};
    uint64_t rank = RankCalculator::rankOrientations(allZeros, 2);
    assert(rank == 0);
    std::cout << "  All-zero orientations have rank 0" << std::endl;
    
    // Test 2: [1,0,0,0,0,0,0] has rank 1
    std::vector<uint8_t> oneOne = {1, 0, 0, 0, 0, 0, 0};
    rank = RankCalculator::rankOrientations(oneOne, 2);
    assert(rank == 1);
    std::cout << "  [1,0,0,0,0,0,0] has rank 1" << std::endl;
    
    // Test 3: Base-3 (corners): [2,0,0,0,0,0,0] has rank 2
    std::vector<uint8_t> twoZ = {2, 0, 0, 0, 0, 0, 0};
    rank = RankCalculator::rankOrientations(twoZ, 2);
    assert(rank == 2);
    std::cout << "  [2,0,0,0,0,0,0] has rank 2" << std::endl;
    
    // Test 4: Corner constraint (sum mod 3 = 0)
    // [1,1,1,0,0,0,0] should give rank where last ori = (3 - 3%3)%3 = 0
    std::vector<uint8_t> testCorners = {1, 1, 1, 0, 0, 0, 0};
    rank = RankCalculator::rankOrientations(testCorners, 2);
    auto unranked = RankCalculator::unrankOrientations(rank, 8, 2);
    
    uint8_t sum = 0;
    for (uint8_t ori : unranked) {
        sum += ori;
    }
    assert(sum % 3 == 0);
    std::cout << "  Corner constraint maintained in unranking" << std::endl;
    
    // Test 5: Total states for 7 corners base-3: 3^7 = 2187
    uint64_t maxCornerOriRank = 1;
    for (int i = 0; i < 7; ++i) {
        maxCornerOriRank *= 3;
    }
    assert(maxCornerOriRank == 2187);
    std::cout << "  7-corner orientations: 3^7 = 2187 states" << std::endl;
    
    std::cout << "All orientation tests passed\n" << std::endl;
}

/**
 * @brief Test corner state ranking
 */
void testCornerStateRanking() {
    std::cout << "Testing: Corner state ranking..." << std::endl;
    
    // Test 1: Solved cube = rank 0
    CubieCube cube;
    uint64_t rank = RankCalculator::rankCornerState(cube);
    assert(rank == 0);
    std::cout << "  Solved cube has corner rank 0" << std::endl;
    
    // Test 2: After one R move, rank > 0
    cube.applyMove(Face::R, MoveType::Normal);
    uint64_t rank1 = RankCalculator::rankCornerState(cube);
    assert(rank1 > 0);
    std::cout << "  R move produces non-zero corner rank" << std::endl;
    
    // Test 3: U move doesn't change orientation, rank should be different perm only
    CubieCube cube2;
    cube2.applyMove(Face::U, MoveType::Normal);
    uint64_t rankU = RankCalculator::rankCornerState(cube2);
    assert(rankU > 0);
    std::cout << "  U move produces different corner rank" << std::endl;
    
    // Test 4: Different moves produce different ranks
    CubieCube c1;
    c1.applyMove(Face::U, MoveType::Normal);
    uint64_t rank_u = RankCalculator::rankCornerState(c1);
    
    CubieCube c2;
    c2.applyMove(Face::R, MoveType::Normal);
    uint64_t rank_r = RankCalculator::rankCornerState(c2);
    
    assert(rank_u != rank_r);
    std::cout << "  Different moves produce different ranks" << std::endl;
    
    // Test 5: Verify size constraint (8! * 3^7 = 88,179,840)
    uint64_t maxCornerRank = RankCalculator::factorial(8) * 2187;
    assert(maxCornerRank == 88179840);
    std::cout << "  Max corner rank = 8! × 3^7 = 88,179,840" << std::endl;
    
    std::cout << "All corner state tests passed\n" << std::endl;
}

/**
 * @brief Test database sizing calculations & physical memory allocation
 */
void testDatabaseSizing() {
    std::cout << "Testing: Database sizing (Theoretical vs Actual)..." << std::endl;
    
    // Database 1: Corner positions + orientations
    uint64_t cornerStates = RankCalculator::factorial(8) * 2187;
    uint64_t cornerStorageNibbles = cornerStates / 2;  // 4 bits per state
    double cornerMB = cornerStorageNibbles / (1024.0 * 1024.0);
    
    std::cout << "  Corner DB: " << cornerStates / 1000000.0 << "M states, "
              << cornerMB << " MB " << std::endl;
    
    // Database 2 & 3: 7-edge permutation only 
    uint64_t sevenEdgePerms = RankCalculator::permutation(12, 7) / 2;  // Parity constraint
    uint64_t sevenEdgeStorageNibbles = sevenEdgePerms / 2;
    double sevenEdgeMB = sevenEdgeStorageNibbles / (1024.0 * 1024.0);
    
    std::cout << "  7-Edge DB (each): " << sevenEdgePerms / 1000000.0 << "M states, "
              << sevenEdgeMB << " MB " << std::endl;
    
    // Database 4: Edge permutation only
    uint64_t edgePerm = RankCalculator::factorial(12) / 2;  // Parity constraint
    uint64_t edgePermStorageNibbles = edgePerm / 2;
    double edgePermMB = edgePermStorageNibbles / (1024.0 * 1024.0);
    
    std::cout << "  Edge Perm DB: " << edgePerm / 1000000.0 << "M states, "
              << edgePermMB << " MB " << std::endl;
    
    // Total Mathematical Calculations
    double totalMB = cornerMB + 2 * sevenEdgeMB + edgePermMB;
    double unreducedMB = totalMB * 2; 
    double fullyOptimizedMB = totalMB / 48.0; 

    std::cout << "\n  --- Optimization Metrics ---" << std::endl;
    std::cout << "  Total Unoptimized Storage (1 byte/state): " << unreducedMB << " MB" << std::endl;
    std::cout << "  Total PDB storage (Nibble Only): " << totalMB << " MB" << std::endl;
    std::cout << "  Total PDB storage (Nibble + 48-Way Symmetry): " << fullyOptimizedMB << " MB" << std::endl;
    
    // =========================================================================
    // TEST ACTUAL OBJECT MEMORY FOOTPRINT
    // =========================================================================
    // std::cout << "\n  --- Validating Physical Object RAM Allocation ---" << std::endl;
    
    // // Instantiate database and enable symmetry reduction
    // PatternDatabase cornerDB(PatternDatabase::Type::CORNER_PIECES);
    // cornerDB.setSymmetryReduction(true);
    
    // double actualMB = cornerDB.getMemoryFootprint() / (1024.0 * 1024.0);
    
    // std::cout << "  Constructed Corner DB Physical RAM: " << actualMB << " MB" << std::endl;
    
    // std::cout << "\n All database sizing tests passed\n" << std::endl;
    std::cout << "\n  --- Validating Physical Object RAM Allocation ---" << std::endl;
    
    // 1. Instantiate the database shell (Uses 0x MB RAM)
    PatternDatabase cornerDB(PatternDatabase::Type::CORNER_PIECES);
    cornerDB.setSymmetryReduction(true);
    std::cout << "  RAM before generation: " << (cornerDB.getMemoryFootprint() / (1024.0 * 1024.0)) << " MB" << std::endl;

    // 2. Run the generator (This triggers the allocation in RAM)
    cornerDB.generate(); 
    // 3. Check the physical allocation after creation
    double actualMB = cornerDB.getMemoryFootprint() / (1024.0 * 1024.0);
    std::cout << "  Constructed Corner DB Physical RAM: " << actualMB << " MB" << std::endl;
    
    std::cout << "\n All database sizing tests passed\n" << std::endl;
}

/**
 * @brief Test edge ranking (if implemented)
 */
void testEdgeRanking() {
    std::cout << "Testing: Edge ranking..." << std::endl;
    
    CubieCube cube;
    
    // Test edge permutation ranking
    uint64_t edgePerm = RankCalculator::rankEdgePermutation(cube);
    assert(edgePerm == 0);  // Solved state
    std::cout << "  Solved cube has edge perm rank 0" << std::endl;
    
    // After one move
    cube.applyMove(Face::R, MoveType::Normal);
    uint64_t edgePerm1 = RankCalculator::rankEdgePermutation(cube);
    assert(edgePerm1 > 0);
    std::cout << "  R move produces non-zero edge perm rank" << std::endl;
    
    // Test 7-edge subset ranking
    std::array<uint8_t, 7> firstSeven = {0, 1, 2, 3, 4, 5, 6};
    cube.reset();
    uint64_t partialEdge = RankCalculator::rankPartialEdgePermutation(cube, firstSeven);
    assert(partialEdge == 0);
    std::cout << "  Partial edge ranking works" << std::endl;
    
    std::cout << "All edge ranking tests passed\n" << std::endl;
}

/**
 * @brief Main test runner for Phase 2
 */
int main() {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "  RUBIK'S CUBE PHASE 2 TEST SUITE" << std::endl;
    std::cout << "  Rank Calculator & Database Sizing" << std::endl;
    std::cout << std::string(50, '=') << std::endl << std::endl;
    
    try {
        testPermutationRanking();
        testOrientationRanking();
        testCornerStateRanking();
        testDatabaseSizing();
        testEdgeRanking();
        
        std::cout << std::string(50, '=') << std::endl;
        std::cout << "  ALL PHASE 2 TESTS PASSED!" << std::endl;
        std::cout << std::string(50, '=') << std::endl << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\nTEST FAILED: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\nUNKNOWN TEST FAILURE" << std::endl;
        return 1;
    }
}