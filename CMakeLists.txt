cmake_minimum_required(VERSION 3.15)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(RubiksCubeSolver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Phase 1: Cube Operations
set(CUBE_SOURCES
    src/cube/CubieCube.cpp
    src/cube/RubiksCube.cpp
)

# Phase 2: Pattern Database & Ranking
set(DATABASE_SOURCES
    src/database/RankCalculator.cpp
    src/database/PatternDatabase.cpp
    src/database/SymmetryReduction.cpp
)

# Phase 3: Solver Algorithms (IDA*, Two-Phase)
set(SOLVER_SOURCES
    src/solver/Solver.cpp
    src/solver/MovePruning.cpp
    src/solver/HeuristicCombination.cpp
    src/solver/TwoPhase.cpp
    src/solver/HybridSolver.cpp
)

# Phase 4-5: Renderer and Interactive UI
set(RENDERER_SOURCES
    src/renderer/CubeRenderer.cpp
    src/renderer/InteractiveSolver.cpp
)

# All source files
set(ALL_SOURCES
    ${CUBE_SOURCES}
    ${DATABASE_SOURCES}
    ${SOLVER_SOURCES}
    ${RENDERER_SOURCES}
)

# Create test executables
add_executable(cube_test tests/cube_test.cpp ${ALL_SOURCES})
add_executable(rank_test tests/rank_test.cpp ${ALL_SOURCES})

# Create interactive solver executable
add_executable(solver_interactive src/main_interactive.cpp ${ALL_SOURCES})

# Compiler flags for optimization and warnings
if(MSVC)
    target_compile_options(cube_test PRIVATE /W4 /O2)
    target_compile_options(rank_test PRIVATE /W4 /O2)
    target_compile_options(solver_interactive PRIVATE /W4 /O2)
else()
    target_compile_options(cube_test PRIVATE -Wall -Wextra -Wpedantic -O2)
    target_compile_options(rank_test PRIVATE -Wall -Wextra -Wpedantic -O2)
    target_compile_options(solver_interactive PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()

# Find required packages
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
    target_link_libraries(cube_test PRIVATE OpenMP::OpenMP_CXX)
    target_link_libraries(rank_test PRIVATE OpenMP::OpenMP_CXX)
    target_link_libraries(solver_interactive PRIVATE OpenMP::OpenMP_CXX)
endif()

target_link_libraries(cube_test PRIVATE glfw GLEW::GLEW glm::glm)
target_link_libraries(rank_test PRIVATE glfw GLEW::GLEW glm::glm)
target_link_libraries(solver_interactive PRIVATE glfw GLEW::GLEW glm::glm)

# Testing
enable_testing()
add_test(NAME CubeTests COMMAND cube_test)
add_test(NAME RankTests COMMAND rank_test)

# Print summary
message(STATUS "")
message(STATUS "=== Rubik's Cube Solver - Complete Implementation ===")
message(STATUS "")
message(STATUS "✓ Phase 1: Cube Operations (Complete)")
message(STATUS "  - CubieCube state representation")
message(STATUS "  - All 18 face moves with rotations")
message(STATUS "")
message(STATUS "✓ Phase 2: Pattern Databases (Complete)")
message(STATUS "  - Lehmer code permutation ranking")
message(STATUS "  - Base-N orientation ranking")
message(STATUS "  - Nibble compression (50% savings)")
message(STATUS "  - 48-way symmetry reduction (48× savings)")
message(STATUS "")
message(STATUS "✓ Phase 3: Solver Algorithms")
message(STATUS "  - Korf IDA* with PDB heuristics")
message(STATUS "  - Kociemba Two-Phase algorithm")
message(STATUS "  - Enhanced move pruning")
message(STATUS "  - Multiple heuristic modes")
message(STATUS "")
message(STATUS "✓ Phase 4-5: Visualization & Interactive UI")
message(STATUS "  - OpenGL renderer with Phong shading")
message(STATUS "  - Quaternion-based rotations")
message(STATUS "  - SLERP animations")
message(STATUS "  - Procedural stickers")
message(STATUS "  - Levitation effect")
message(STATUS "")
message(STATUS "Build targets:")
message(STATUS "  - rank_test: Phase 1-2 testing")
message(STATUS "  - cube_test: Cube operations testing")
message(STATUS "  - solver_interactive: Full interactive solver")
message(STATUS "")

# Optional: Add testing
enable_testing()
add_test(NAME CubeOperations COMMAND cube_test)
