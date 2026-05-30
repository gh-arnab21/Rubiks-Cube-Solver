cmake_minimum_required(VERSION 3.15)
project(RubiksCubeSolver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/cube/CubieCube.cpp
    src/cube/RubiksCube.cpp
    src/database/RankCalculator.cpp
    src/database/PatternDatabase.cpp
    src/solver/Solver.cpp
)

# Create main executable
add_executable(cube_solver src/main.cpp ${SOURCES})

# Create test executable
add_executable(cube_test tests/cube_test.cpp ${SOURCES})

# Compiler flags for optimization and warnings
if(MSVC)
    target_compile_options(cube_solver PRIVATE /W4 /O2)
    target_compile_options(cube_test PRIVATE /W4 /O2)
else()
    target_compile_options(cube_solver PRIVATE -Wall -Wextra -Wpedantic -O2)
    target_compile_options(cube_test PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()

# Optional: Add testing
enable_testing()
add_test(NAME CubeOperations COMMAND cube_test)
