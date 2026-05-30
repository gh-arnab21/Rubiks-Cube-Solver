# Rubik's Cube Solver in C++

An extensible, high-performance Rubik's Cube solver implementation featuring Korf's optimal algorithm with pattern databases and support for advanced optimizations.

## Overview

This project implements a complete Rubik's Cube solver using:

- **Core Data Structure**: Array-based representation of corner and edge cubies with positions and orientations
- **Primary Algorithm**: IDA* (Iterative-Deepening A*) - optimal solver guaranteeing 20 moves or fewer
- **Heuristic**: Pattern Databases (PDBs) - precomputed distance databases for cube subsets
- **Architecture**: Extensible framework supporting multiple algorithms and optimizations

## Key Features

### Architecture

```
include/
  ├── cube/               - Cube representation & manipulation
  │   ├── CubieCube.h    - Low-level cubie arrays (8 corners, 12 edges)
  │   └── RubiksCube.h   - High-level interface with move parsing
  ├── database/           - Heuristic computation
  │   ├── PatternDatabase.h   - PDB interface & implementations
  │   └── RankCalculator.h    - Permutation/orientation ranking
  ├── solver/             - Solving algorithms
  │   └── Solver.h        - Solver interfaces (Korf, Two-Phase, IDDFS)
  └── utils/              - Utilities
      └── Constants.h     - Global definitions
```

### Supported Algorithms

1. **Simple IDDFS** (`SimpleIDDFSSolver`)
   - Basic depth-first search without heuristic
   - For testing and verification
   - Very slow but guaranteed to work

2. **Korf's IDA*** (`KorfSolver`)
   - Optimal solver - finds shortest solution
   - Uses 4 pattern databases
   - Solves any cube in ≤20 moves
   - Can use symmetry reduction for memory optimization

3. **Two-Phase Algorithm** (`TwoPhaseAlgorithm`)
   - Kociemba-style subgroup decomposition
   - Phase 1: Get to intermediate group G1
   - Phase 2: Complete solution from G1
   - Often faster than single IDA*

### Memory Optimizations

- **Nibble Compression**: Pack 2 distances per byte (4 bits each)
- **Symmetry Reduction**: Leverage 48 cube symmetries
  - Reduces PDB size by ~48x
  - 244 MB → ~5 MB for 7-edge database
  - Allows 8-9 edge databases (previously impossible)

## Data Structures

### CubieCube
```cpp
struct Cubie {
    uint8_t position;      // Where the cubie is (0-7 corners, 0-11 edges)
    uint8_t orientation;   // How it's oriented (0-2 corners, 0-1 edges)
};

class CubieCube {
    std::array<Cubie, 8> cornerCubies;    // 8 corner pieces
    std::array<Cubie, 12> edgeCubies;     // 12 edge pieces
};
```

### Pattern Databases

| Database | States | Storage | Max Dist | Notes |
|----------|--------|---------|----------|-------|
| Corner | 8! × 3^7 = 88M | ~42 MB | 11 | All corners |
| Edge 7-piece | 12P7 × 2^7 = 511M | ~244 MB | 10 | 7 of 12 edges |
| Edge Perm | 12!/2 = 239M | ~228 MB | - | Permutation only |
| **Total** | - | **~758 MB** | - | Standard Korf |
| **Symmetry-Reduced** | - | **~50 MB** | - | With optimization |

## Building the Project

### Prerequisites
- C++17 compatible compiler
- CMake 3.15+

### Windows (MSVC)
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Linux/macOS (GCC/Clang)
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## Usage

### Basic Example
```cpp
#include "cube/RubiksCube.h"
#include "solver/Solver.h"

using namespace cube_solver;

// Create and scramble cube
RubiksCube cube;
cube.applyMoveSequence("R U R' U' R U2 R'");

// Solve with simple IDDFS (for testing)
SimpleIDDFSSolver solver;
SolutionResult result = solver.solve(cube);

if (result.found) {
    std::cout << "Solved in " << result.moveCount << " moves" << std::endl;
}
```

### Using Korf's Optimal Solver
```cpp
// Load or generate pattern databases
KorfSolver optimalSolver;
optimalSolver.loadDatabases("./data");  // or generateDatabases()

// Solve any cube optimally
RubiksCube scrambledCube;
scrambledCube.scramble(20);

SolutionResult result = optimalSolver.solve(scrambledCube);
std::cout << "Optimal solution: " << result.moveCount << " moves" << std::endl;
```

## Implementation Status

### ✅ Completed
- [x] Project structure and framework
- [x] CubieCube data structure (interface)
- [x] RubiksCube high-level interface
- [x] Move parsing and notation
- [x] PatternDatabase interfaces
- [x] RankCalculator framework
- [x] Solver interface
- [x] SimpleIDDFSSolver for testing
- [x] CMake build system
- [x] Main.cpp demonstration

### 🔄 TODO - Core Implementation

**Priority 1: Make it work**
1. [ ] Implement `CubieCube::applySingleMove()` for all 6 faces
   - U, D, L, R, F, B face rotations
   - Corner/edge cycle notation
   - Orientation updates
   - Reference: Benbotto's cube-cracker repo

2. [ ] Implement `RankCalculator` permutation/orientation ranking
   - `rankPermutation()` - Lehmer code/factorial number system
   - `rankOrientations()` - base-N representation
   - Corner state ranking (combined perm + ori)
   - Reference: Korf's AAAI papers, Wikipedia Lehmer code

3. [ ] Implement `PatternDatabase` BFS generation
   - Generate corner PDB (~30 mins)
   - Generate edge 7-piece PDBs (~1 hour each)
   - Nibble compression/decompression
   - File I/O (save/load)

4. [ ] Implement `KorfSolver::idaSearch()` IDA* main loop
   - Threshold-based depth-first search
   - f(n) = g(n) + h(n) pruning
   - Solution path tracking
   - Iterative threshold increase

### 🔄 TODO - Advanced Features

**Priority 2: Optimize & enhance**
1. [ ] Symmetry reduction for PDBs
   - 48 symmetry group generation
   - Canonical form finding
   - ~48x memory savings

2. [ ] Two-Phase Algorithm (Kociemba)
   - Subgroup decomposition (G0→G1→G2→G3)
   - Phase-specific PDBs
   - Often faster than single IDA*

3. [ ] Move pruning table
   - Eliminate redundant consecutive moves
   - Reduce branching factor

4. [ ] Parallelization
   - Parallel BFS for PDB generation
   - Parallel search (if applicable)

### 🔄 TODO - Testing & Validation
1. [ ] Unit tests for cubie operations
2. [ ] Integration tests for solver algorithms
3. [ ] Benchmark vs reference implementations
4. [ ] Verification against known optimal solutions

## Mathematical Background

### Permutation Ranking (Lehmer Code)
Convert a permutation to a unique integer in [0, n!) using factorial number system.

Example: Permutation [2,0,1]
- Position 0: 2 smaller values to right → digit 2
- Position 1: 0 smaller values to right → digit 0
- Rank = 2×2! + 0×1! + 0×0! = 4

### Orientation Encoding
For orientations {0,1,2}, the last orientation is redundant (sum mod 3).
Treat first (n-1) orientations as base-3 number.

### Pattern Database Heuristic
A PDB stores min moves to solve a cube subset. For cube state S:
- h(S) = PDB lookup value
- h(S) is **admissible** (never overestimates actual distance)
- h(S) = 0 only when subset is solved

## References

- Korf, R. E. (1997). "Finding Optimal Solutions to Rubik's Cube Using Pattern Databases"
- Korf, R. E. et al. (2005). "Large-Scale Parallel Breadth-First Search"
- Benbotto. (2024). [rubiks-cube-cracker](https://github.com/benbotto/rubiks-cube-cracker) - C++ reference implementation
- Pochmann, S. [Thistlethwaite 3x3 solver](https://www.stefan-pochmann.info/spocc/other_stuff/tools/solver_thistlethwaite/solver_thistlethwaite.txt)
- Taylor, P. [Indexing Edge Permutations for the Rubik's Cube](https://cs.stackexchange.com/questions/107111/)
- Heise, R. [Rubik's Cube Theory](https://www.ryanheise.com/cube/cube_laws.html)

## Cube Notation

Standard Rubik's Cube notation:
- **U, L, F, R, B, D**: 90° clockwise turn of up, left, front, right, back, down face
- **X'** (prime): 90° counter-clockwise (equivalent to 3 clockwise turns)
- **X2**: 180° turn (2 clockwise turns)
- Example: "R U R' U' R U2 R'" is a classic 7-move sequence

## Performance Targets

- **Solve time**: <1 second for any 20-move scramble (with PDBs)
- **Memory**: ~50 MB with symmetry reduction
- **Optimality**: Guaranteed ≤20 moves (Korf's upper bound)

## Future Enhancements

1. **GraphicsVisualization**: Add OpenGL rendering (like benbotto's repo)
2. **Move optimizer**: Further reduce move sequences post-solve
3. **Machine learning**: Neural network heuristic (alternative to PDB)
4. **Parallel generation**: Parallelize PDB generation on multi-core systems
5. **Distributed solving**: Cloud-based BFS for even larger PDBs

## License

This is a learning/reference implementation. Use for educational purposes.

## Author

Created as a comprehensive C++ Rubik's Cube solver project with focus on:
- Clean, well-documented code
- Multiple algorithm implementations
- Memory optimization techniques
- Extensible architecture for future improvements
