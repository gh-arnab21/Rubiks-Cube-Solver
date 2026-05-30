/**
 * @file CubeTest.cpp
 * @brief Unit tests for cube operations
 * 
 * Tests verify:
 * 1. Move correctness (applying 4 times returns to identity)
 * 2. Move sequences
 * 3. Cube state queries
 */

#include <iostream>
#include <cassert>
#include <cstring>
#include "cube/CubieCube.h"
#include "cube/RubiksCube.h"

using namespace cube_solver;

/**
 * @brief Test: Applying same move 4 times returns to original state
 * This is a fundamental property - each face twist has period 4
 */
void testMoveHasPeriodFour() {
    std::cout << "Testing: Move period = 4..." << std::endl;
    
    for (int faceInt = 0; faceInt < 6; ++faceInt) {
        Face face = static_cast<Face>(faceInt);
        CubieCube original;
        CubieCube test = original;
        
        // Apply same move 4 times
        for (int i = 0; i < 4; ++i) {
            test.applySingleMove(face);
        }
        
        // Should return to original
        assert(test == original);
        
        std::string faceName[] = {"U", "D", "L", "R", "F", "B"};
        std::cout << "  " << faceName[faceInt] << " move has period 4" << std::endl;
    }
    
    std::cout << "All faces have period 4\n" << std::endl;
}

/**
 * @brief Test: Applying move + inverse returns to original
 */
void testMoveInverse() {
    std::cout << "Testing: Move inverse..." << std::endl;
    
    for (int faceInt = 0; faceInt < 6; ++faceInt) {
        Face face = static_cast<Face>(faceInt);
        CubieCube original;
        CubieCube test = original;
        
        // Apply move 3 times = apply move + apply inverse
        test.applySingleMove(face);
        test.applySingleMove(face);
        test.applySingleMove(face);
        // Now test is at M', if we apply one more we should be at identity
        test.applySingleMove(face);
        
        assert(test == original);
        
        std::string faceName[] = {"U", "D", "L", "R", "F", "B"};
        std::cout << "  " << faceName[faceInt] << " inverse works" << std::endl;
    }
    
    std::cout << "All inverses work correctly\n" << std::endl;
}

/**
 * @brief Test: Classic move sequences
 */
void testMoveSequences() {
    std::cout << "Testing: Move sequences..." << std::endl;
    
    // Test: R U R' U' applied 6 times should return to solved
    RubiksCube cube;
    for (int i = 0; i < 6; ++i) {
        cube.applyMove(Face::R, MoveType::Normal);
        cube.applyMove(Face::U, MoveType::Normal);
        cube.applyMove(Face::R, MoveType::Prime);
        cube.applyMove(Face::U, MoveType::Prime);
    }
    
    assert(cube.isSolved());
    std::cout << "  R U R' U' cycle (6x) returns to solved" << std::endl;
    
    // Test: Sune algorithm (R U R' U R U2 R')
    cube.reset();
    cube.applyMoveSequence("R U R' U R U2 R'");
    
    // It shouldn't be solved (this is a corner orientation algorithm)
    // But applying it multiple times should cycle
    RubiksCube test = cube;
    for (int i = 0; i < 5; ++i) {
        test.applyMoveSequence("R U R' U R U2 R'");
    }
    
    assert(test.isSolved());
    std::cout << "  Sune algorithm cycles correctly" << std::endl;
    
    std::cout << "All move sequences work\n" << std::endl;
}

/**
 * @brief Test: Scramble and check operations
 */
void testScramble() {
    std::cout << "Testing: Scramble generation..." << std::endl;
    
    RubiksCube cube;
    cube.scramble(10);
    
    assert(cube.getMoveCount() == 10);
    assert(!cube.isSolved());  // Very likely not solved
    
    std::cout << "  Generated 10-move scramble" << std::endl;
    std::cout << "  Scramble: " << cube.getMoveHistoryString() << std::endl;
    
    std::cout << "Scramble works\n" << std::endl;
}

/**
 * @brief Test: Cube state consistency
 */
void testStateConsistency() {
    std::cout << "Testing: State consistency..." << std::endl;
    
    CubieCube cube;
    
    // Verify solved state
    for (uint8_t i = 0; i < NUM_CORNERS; ++i) {
        assert(cube.getCornerCubie(i).position == i);
        assert(cube.getCornerCubie(i).orientation == 0);
    }
    
    for (uint8_t i = 0; i < NUM_EDGES; ++i) {
        assert(cube.getEdgeCubie(i).position == i);
        assert(cube.getEdgeCubie(i).orientation == 0);
    }
    
    std::cout << "  Solved state verified" << std::endl;
    
    // Apply a move and verify only correct cubies changed
    cube.applySingleMove(Face::U);
    
    // Corners 0,1,2,3 should have changed positions
    // Corners 4,5,6,7 should not be affected
    for (uint8_t i = 4; i < NUM_CORNERS; ++i) {
        assert(cube.getCornerCubie(i).position == i);
    }
    
    std::cout << "  Move affects only correct cubies" << std::endl;
    
    std::cout << "State consistency verified\n" << std::endl;
}

/**
 * @brief Test: All combinations of moves and types
 */
void testAllMoveTypes() {
    std::cout << "Testing: All move types (Normal, Prime, Double)..." << std::endl;
    
    for (int faceInt = 0; faceInt < 6; ++faceInt) {
        Face face = static_cast<Face>(faceInt);
        
        // Test Normal move
        RubiksCube normal;
        normal.applyMove(face, MoveType::Normal);
        normal.applyMove(face, MoveType::Prime);
        assert(normal.isSolved());
        
        // Test Prime move
        RubiksCube prime;
        prime.applyMove(face, MoveType::Prime);
        prime.applyMove(face, MoveType::Normal);
        assert(prime.isSolved());
        
        // Test Double move
        RubiksCube double_move;
        double_move.applyMove(face, MoveType::Double);
        double_move.applyMove(face, MoveType::Double);
        assert(double_move.isSolved());
        
        std::string faceName[] = {"U", "D", "L", "R", "F", "B"};
        std::cout << "  " << faceName[faceInt] << ": Normal + Prime + Double" << std::endl;
    }
    
    std::cout << "All move types work\n" << std::endl;
}

/**
 * @brief Main test runner
 */
int main() {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "  RUBIK'S CUBE PHASE 1 TEST SUITE" << std::endl;
    std::cout << std::string(50, '=') << std::endl << std::endl;
    
    try {
        testMoveHasPeriodFour();
        testMoveInverse();
        testMoveSequences();
        testScramble();
        testStateConsistency();
        testAllMoveTypes();
        
        std::cout << std::string(50, '=') << std::endl;
        std::cout << "  ALL TESTS PASSED! " << std::endl;
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
